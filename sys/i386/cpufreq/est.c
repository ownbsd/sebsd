/*-
 * Copyright (c) 2004 Colin Percival
 * Copyright (c) 2005 Nate Lawson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/i386/cpufreq/est.c,v 1.10 2006/02/25 04:55:38 cperciva Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/smp.h>
#include <sys/systm.h>

#include "cpufreq_if.h"
#include <machine/md_var.h>

#include <contrib/dev/acpica/acpi.h>
#include <dev/acpica/acpivar.h>
#include "acpi_if.h"

/* Status/control registers (from the IA-32 System Programming Guide). */
#define MSR_PERF_STATUS		0x198
#define MSR_PERF_CTL		0x199

/* Register and bit for enabling SpeedStep. */
#define MSR_MISC_ENABLE		0x1a0
#define MSR_SS_ENABLE		(1<<16)

/* Frequency and MSR control values. */
typedef struct {
	uint16_t	freq;
	uint16_t	volts;
	uint16_t	id16;
	int		power;
} freq_info;

/* Identifying characteristics of a processor and supported frequencies. */
typedef struct {
	const char	*vendor;
	uint32_t	id32;
	uint32_t	bus_clk;
	freq_info	*freqtab;
} cpu_info;

struct est_softc {
	device_t	dev;
	int		acpi_settings;
	freq_info	*freq_list;
};

/* Convert MHz and mV into IDs for passing to the MSR. */
#define ID16(MHz, mV, bus_clk)				\
	(((MHz / bus_clk) << 8) | ((mV ? mV - 700 : 0) >> 4))
#define ID32(MHz_hi, mV_hi, MHz_lo, mV_lo, bus_clk)	\
	((ID16(MHz_lo, mV_lo, bus_clk) << 16) | (ID16(MHz_hi, mV_hi, bus_clk)))

/* Format for storing IDs in our table. */
#define FREQ_INFO(MHz, mV, bus_clk)			\
	{ MHz, mV, ID16(MHz, mV, bus_clk), CPUFREQ_VAL_UNKNOWN }
#define INTEL(tab, zhi, vhi, zlo, vlo, bus_clk)		\
	{ GenuineIntel, ID32(zhi, vhi, zlo, vlo, bus_clk), bus_clk, tab }

const char GenuineIntel[] = "GenuineIntel";

/* Default bus clock value for Centrino processors. */
#define INTEL_BUS_CLK		100

/* XXX Update this if new CPUs have more settings. */
#define EST_MAX_SETTINGS	10
CTASSERT(EST_MAX_SETTINGS <= MAX_SETTINGS);

/* Estimate in microseconds of latency for performing a transition. */
#define EST_TRANS_LAT		10

/*
 * Frequency (MHz) and voltage (mV) settings.  Data from the
 * Intel Pentium M Processor Datasheet (Order Number 252612), Table 5.
 *
 * Dothan processors have multiple VID#s with different settings for
 * each VID#.  Since we can't uniquely identify this info
 * without undisclosed methods from Intel, we can't support newer
 * processors with this table method.  If ACPI Px states are supported,
 * we get info from them.
 */
static freq_info PM17_130[] = {
	/* 130nm 1.70GHz Pentium M */
	FREQ_INFO(1700, 1484, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1004, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM16_130[] = {
	/* 130nm 1.60GHz Pentium M */
	FREQ_INFO(1600, 1484, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1420, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1164, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM15_130[] = {
	/* 130nm 1.50GHz Pentium M */
	FREQ_INFO(1500, 1484, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1452, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1356, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1228, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM14_130[] = {
	/* 130nm 1.40GHz Pentium M */
	FREQ_INFO(1400, 1484, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1436, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1308, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1180, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM13_130[] = {
	/* 130nm 1.30GHz Pentium M */
	FREQ_INFO(1300, 1388, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1356, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1292, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1260, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM13_LV_130[] = {
	/* 130nm 1.30GHz Low Voltage Pentium M */
	FREQ_INFO(1300, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1100, 1100, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1020, INTEL_BUS_CLK),
	FREQ_INFO( 900, 1004, INTEL_BUS_CLK),
	FREQ_INFO( 800,  988, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM12_LV_130[] = {
	/* 130 nm 1.20GHz Low Voltage Pentium M */
	FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1100, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
	FREQ_INFO( 900, 1020, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1004, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM11_LV_130[] = {
	/* 130 nm 1.10GHz Low Voltage Pentium M */
	FREQ_INFO(1100, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1164, INTEL_BUS_CLK),
	FREQ_INFO( 900, 1100, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
	FREQ_INFO( 600,  956, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM11_ULV_130[] = {
	/* 130 nm 1.10GHz Ultra Low Voltage Pentium M */
	FREQ_INFO(1100, 1004, INTEL_BUS_CLK),
	FREQ_INFO(1000,  988, INTEL_BUS_CLK),
	FREQ_INFO( 900,  972, INTEL_BUS_CLK),
	FREQ_INFO( 800,  956, INTEL_BUS_CLK),
	FREQ_INFO( 600,  844, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM10_ULV_130[] = {
	/* 130 nm 1.00GHz Ultra Low Voltage Pentium M */
	FREQ_INFO(1000, 1004, INTEL_BUS_CLK),
	FREQ_INFO( 900,  988, INTEL_BUS_CLK),
	FREQ_INFO( 800,  972, INTEL_BUS_CLK),
	FREQ_INFO( 600,  844, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};

/*
 * Data from "Intel Pentium M Processor on 90nm Process with
 * 2-MB L2 Cache Datasheet", Order Number 302189, Table 5.
 */
static freq_info PM_765A_90[] = {
	/* 90 nm 2.10GHz Pentium M, VID #A */
	FREQ_INFO(2100, 1340, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_765B_90[] = {
	/* 90 nm 2.10GHz Pentium M, VID #B */
	FREQ_INFO(2100, 1324, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1260, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_765C_90[] = {
	/* 90 nm 2.10GHz Pentium M, VID #C */
	FREQ_INFO(2100, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_765E_90[] = {
	/* 90 nm 2.10GHz Pentium M, VID #E */
	FREQ_INFO(2100, 1356, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1292, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1196, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755A_90[] = {
	/* 90 nm 2.00GHz Pentium M, VID #A */
	FREQ_INFO(2000, 1340, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1292, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1196, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755B_90[] = {
	/* 90 nm 2.00GHz Pentium M, VID #B */
	FREQ_INFO(2000, 1324, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755C_90[] = {
	/* 90 nm 2.00GHz Pentium M, VID #C */
	FREQ_INFO(2000, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755D_90[] = {
	/* 90 nm 2.00GHz Pentium M, VID #D */
	FREQ_INFO(2000, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1800, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1196, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745A_90[] = {
	/* 90 nm 1.80GHz Pentium M, VID #A */
	FREQ_INFO(1800, 1340, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1292, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745B_90[] = {
	/* 90 nm 1.80GHz Pentium M, VID #B */
	FREQ_INFO(1800, 1324, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745C_90[] = {
	/* 90 nm 1.80GHz Pentium M, VID #C */
	FREQ_INFO(1800, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1260, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745D_90[] = {
	/* 90 nm 1.80GHz Pentium M, VID #D */
	FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735A_90[] = {
	/* 90 nm 1.70GHz Pentium M, VID #A */
	FREQ_INFO(1700, 1340, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735B_90[] = {
	/* 90 nm 1.70GHz Pentium M, VID #B */
	FREQ_INFO(1700, 1324, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735C_90[] = {
	/* 90 nm 1.70GHz Pentium M, VID #C */
	FREQ_INFO(1700, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735D_90[] = {
	/* 90 nm 1.70GHz Pentium M, VID #D */
	FREQ_INFO(1700, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725A_90[] = {
	/* 90 nm 1.60GHz Pentium M, VID #A */
	FREQ_INFO(1600, 1340, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1132, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725B_90[] = {
	/* 90 nm 1.60GHz Pentium M, VID #B */
	FREQ_INFO(1600, 1324, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1260, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1196, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1132, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725C_90[] = {
	/* 90 nm 1.60GHz Pentium M, VID #C */
	FREQ_INFO(1600, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1244, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725D_90[] = {
	/* 90 nm 1.60GHz Pentium M, VID #D */
	FREQ_INFO(1600, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715A_90[] = {
	/* 90 nm 1.50GHz Pentium M, VID #A */
	FREQ_INFO(1500, 1340, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1228, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1148, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715B_90[] = {
	/* 90 nm 1.50GHz Pentium M, VID #B */
	FREQ_INFO(1500, 1324, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1148, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715C_90[] = {
	/* 90 nm 1.50GHz Pentium M, VID #C */
	FREQ_INFO(1500, 1308, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1212, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1132, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715D_90[] = {
	/* 90 nm 1.50GHz Pentium M, VID #D */
	FREQ_INFO(1500, 1276, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_778_90[] = {
	/* 90 nm 1.60GHz Low Voltage Pentium M */
	FREQ_INFO(1600, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1500, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1100, INTEL_BUS_CLK),
	FREQ_INFO(1300, 1084, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1068, INTEL_BUS_CLK),
	FREQ_INFO(1100, 1052, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 900, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_758_90[] = {
	/* 90 nm 1.50GHz Low Voltage Pentium M */
	FREQ_INFO(1500, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1400, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1300, 1100, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1084, INTEL_BUS_CLK),
	FREQ_INFO(1100, 1068, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 900, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_738_90[] = {
	/* 90 nm 1.40GHz Low Voltage Pentium M */
	FREQ_INFO(1400, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1300, 1116, INTEL_BUS_CLK),
	FREQ_INFO(1200, 1100, INTEL_BUS_CLK),
	FREQ_INFO(1100, 1068, INTEL_BUS_CLK),
	FREQ_INFO(1000, 1052, INTEL_BUS_CLK),
	FREQ_INFO( 900, 1036, INTEL_BUS_CLK),
	FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
	FREQ_INFO( 600,  988, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_773G_90[] = {
	/* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #G */
	FREQ_INFO(1300,  956, INTEL_BUS_CLK),
	FREQ_INFO(1200,  940, INTEL_BUS_CLK),
	FREQ_INFO(1100,  924, INTEL_BUS_CLK),
	FREQ_INFO(1000,  908, INTEL_BUS_CLK),
	FREQ_INFO( 900,  876, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773H_90[] = {
	/* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #H */
	FREQ_INFO(1300,  940, INTEL_BUS_CLK),
	FREQ_INFO(1200,  924, INTEL_BUS_CLK),
	FREQ_INFO(1100,  908, INTEL_BUS_CLK),
	FREQ_INFO(1000,  892, INTEL_BUS_CLK),
	FREQ_INFO( 900,  876, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773I_90[] = {
	/* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #I */
	FREQ_INFO(1300,  924, INTEL_BUS_CLK),
	FREQ_INFO(1200,  908, INTEL_BUS_CLK),
	FREQ_INFO(1100,  892, INTEL_BUS_CLK),
	FREQ_INFO(1000,  876, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773J_90[] = {
	/* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #J */
	FREQ_INFO(1300,  908, INTEL_BUS_CLK),
	FREQ_INFO(1200,  908, INTEL_BUS_CLK),
	FREQ_INFO(1100,  892, INTEL_BUS_CLK),
	FREQ_INFO(1000,  876, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773K_90[] = {
	/* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #K */
	FREQ_INFO(1300,  892, INTEL_BUS_CLK),
	FREQ_INFO(1200,  892, INTEL_BUS_CLK),
	FREQ_INFO(1100,  876, INTEL_BUS_CLK),
	FREQ_INFO(1000,  860, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773L_90[] = {
	/* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #L */
	FREQ_INFO(1300,  876, INTEL_BUS_CLK),
	FREQ_INFO(1200,  876, INTEL_BUS_CLK),
	FREQ_INFO(1100,  860, INTEL_BUS_CLK),
	FREQ_INFO(1000,  860, INTEL_BUS_CLK),
	FREQ_INFO( 900,  844, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753G_90[] = {
	/* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #G */
	FREQ_INFO(1200,  956, INTEL_BUS_CLK),
	FREQ_INFO(1100,  940, INTEL_BUS_CLK),
	FREQ_INFO(1000,  908, INTEL_BUS_CLK),
	FREQ_INFO( 900,  892, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753H_90[] = {
	/* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #H */
	FREQ_INFO(1200,  940, INTEL_BUS_CLK),
	FREQ_INFO(1100,  924, INTEL_BUS_CLK),
	FREQ_INFO(1000,  908, INTEL_BUS_CLK),
	FREQ_INFO( 900,  876, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753I_90[] = {
	/* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #I */
	FREQ_INFO(1200,  924, INTEL_BUS_CLK),
	FREQ_INFO(1100,  908, INTEL_BUS_CLK),
	FREQ_INFO(1000,  892, INTEL_BUS_CLK),
	FREQ_INFO( 900,  876, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753J_90[] = {
	/* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #J */
	FREQ_INFO(1200,  908, INTEL_BUS_CLK),
	FREQ_INFO(1100,  892, INTEL_BUS_CLK),
	FREQ_INFO(1000,  876, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753K_90[] = {
	/* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #K */
	FREQ_INFO(1200,  892, INTEL_BUS_CLK),
	FREQ_INFO(1100,  892, INTEL_BUS_CLK),
	FREQ_INFO(1000,  876, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753L_90[] = {
	/* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #L */
	FREQ_INFO(1200,  876, INTEL_BUS_CLK),
	FREQ_INFO(1100,  876, INTEL_BUS_CLK),
	FREQ_INFO(1000,  860, INTEL_BUS_CLK),
	FREQ_INFO( 900,  844, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};

static freq_info PM_733JG_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #G */
	FREQ_INFO(1100,  956, INTEL_BUS_CLK),
	FREQ_INFO(1000,  940, INTEL_BUS_CLK),
	FREQ_INFO( 900,  908, INTEL_BUS_CLK),
	FREQ_INFO( 800,  876, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JH_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #H */
	FREQ_INFO(1100,  940, INTEL_BUS_CLK),
	FREQ_INFO(1000,  924, INTEL_BUS_CLK),
	FREQ_INFO( 900,  892, INTEL_BUS_CLK),
	FREQ_INFO( 800,  876, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JI_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #I */
	FREQ_INFO(1100,  924, INTEL_BUS_CLK),
	FREQ_INFO(1000,  908, INTEL_BUS_CLK),
	FREQ_INFO( 900,  892, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JJ_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #J */
	FREQ_INFO(1100,  908, INTEL_BUS_CLK),
	FREQ_INFO(1000,  892, INTEL_BUS_CLK),
	FREQ_INFO( 900,  876, INTEL_BUS_CLK),
	FREQ_INFO( 800,  860, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JK_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #K */
	FREQ_INFO(1100,  892, INTEL_BUS_CLK),
	FREQ_INFO(1000,  876, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JL_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #L */
	FREQ_INFO(1100,  876, INTEL_BUS_CLK),
	FREQ_INFO(1000,  876, INTEL_BUS_CLK),
	FREQ_INFO( 900,  860, INTEL_BUS_CLK),
	FREQ_INFO( 800,  844, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733_90[] = {
	/* 90 nm 1.10GHz Ultra Low Voltage Pentium M */
	FREQ_INFO(1100,  940, INTEL_BUS_CLK),
	FREQ_INFO(1000,  924, INTEL_BUS_CLK),
	FREQ_INFO( 900,  892, INTEL_BUS_CLK),
	FREQ_INFO( 800,  876, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};
static freq_info PM_723_90[] = {
	/* 90 nm 1.00GHz Ultra Low Voltage Pentium M */
	FREQ_INFO(1000,  940, INTEL_BUS_CLK),
	FREQ_INFO( 900,  908, INTEL_BUS_CLK),
	FREQ_INFO( 800,  876, INTEL_BUS_CLK),
	FREQ_INFO( 600,  812, INTEL_BUS_CLK),
	FREQ_INFO(   0,    0, 1),
};

static cpu_info ESTprocs[] = {
	INTEL(PM17_130,		1700, 1484, 600, 956, INTEL_BUS_CLK),
	INTEL(PM16_130,		1600, 1484, 600, 956, INTEL_BUS_CLK),
	INTEL(PM15_130,		1500, 1484, 600, 956, INTEL_BUS_CLK),
	INTEL(PM14_130,		1400, 1484, 600, 956, INTEL_BUS_CLK),
	INTEL(PM13_130,		1300, 1388, 600, 956, INTEL_BUS_CLK),
	INTEL(PM13_LV_130,	1300, 1180, 600, 956, INTEL_BUS_CLK),
	INTEL(PM12_LV_130,	1200, 1180, 600, 956, INTEL_BUS_CLK),
	INTEL(PM11_LV_130,	1100, 1180, 600, 956, INTEL_BUS_CLK),
	INTEL(PM11_ULV_130,	1100, 1004, 600, 844, INTEL_BUS_CLK),
	INTEL(PM10_ULV_130,	1000, 1004, 600, 844, INTEL_BUS_CLK),
	INTEL(PM_765A_90,	2100, 1340, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_765B_90,	2100, 1324, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_765C_90,	2100, 1308, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_765E_90,	2100, 1356, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_755A_90,	2000, 1340, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_755B_90,	2000, 1324, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_755C_90,	2000, 1308, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_755D_90,	2000, 1276, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_745A_90,	1800, 1340, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_745B_90,	1800, 1324, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_745C_90,	1800, 1308, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_745D_90,	1800, 1276, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_735A_90,	1700, 1340, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_735B_90,	1700, 1324, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_735C_90,	1700, 1308, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_735D_90,	1700, 1276, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_725A_90,	1600, 1340, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_725B_90,	1600, 1324, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_725C_90,	1600, 1308, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_725D_90,	1600, 1276, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_715A_90,	1500, 1340, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_715B_90,	1500, 1324, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_715C_90,	1500, 1308, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_715D_90,	1500, 1276, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_778_90,	1600, 1116, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_758_90,	1500, 1116, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_738_90,	1400, 1116, 600, 988, INTEL_BUS_CLK),
	INTEL(PM_773G_90,	1300,  956, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_773H_90,	1300,  940, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_773I_90,	1300,  924, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_773J_90,	1300,  908, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_773K_90,	1300,  892, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_773L_90,	1300,  876, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_753G_90,	1200,  956, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_753H_90,	1200,  940, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_753I_90,	1200,  924, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_753J_90,	1200,  908, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_753K_90,	1200,  892, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_753L_90,	1200,  876, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733JG_90,	1100,  956, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733JH_90,	1100,  940, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733JI_90,	1100,  924, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733JJ_90,	1100,  908, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733JK_90,	1100,  892, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733JL_90,	1100,  876, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_733_90,	1100,  940, 600, 812, INTEL_BUS_CLK),
	INTEL(PM_723_90,	1000,  940, 600, 812, INTEL_BUS_CLK),
	{ NULL, 0, 0, NULL },
};

static void	est_identify(driver_t *driver, device_t parent);
static int	est_features(driver_t *driver, u_int *features);
static int	est_probe(device_t parent);
static int	est_attach(device_t parent);
static int	est_detach(device_t parent);
static int	est_get_info(device_t dev);
static int	est_acpi_info(device_t dev, freq_info **freqs);
static int	est_table_info(device_t dev, uint64_t msr, uint32_t bus_clk,
		    freq_info **freqs);
static freq_info *est_get_current(freq_info *freq_list);
static int	est_settings(device_t dev, struct cf_setting *sets, int *count);
static int	est_set(device_t dev, const struct cf_setting *set);
static int	est_get(device_t dev, struct cf_setting *set);
static int	est_type(device_t dev, int *type);

static device_method_t est_methods[] = {
	/* Device interface */
	DEVMETHOD(device_identify,	est_identify),
	DEVMETHOD(device_probe,		est_probe),
	DEVMETHOD(device_attach,	est_attach),
	DEVMETHOD(device_detach,	est_detach),

	/* cpufreq interface */
	DEVMETHOD(cpufreq_drv_set,	est_set),
	DEVMETHOD(cpufreq_drv_get,	est_get),
	DEVMETHOD(cpufreq_drv_type,	est_type),
	DEVMETHOD(cpufreq_drv_settings,	est_settings),

	/* ACPI interface */
	DEVMETHOD(acpi_get_features,	est_features),

	{0, 0}
};

static driver_t est_driver = {
	"est",
	est_methods,
	sizeof(struct est_softc),
};

static devclass_t est_devclass;
DRIVER_MODULE(est, cpu, est_driver, est_devclass, 0, 0);

static int
est_features(driver_t *driver, u_int *features)
{

	/* Notify the ACPI CPU that we support direct access to MSRs */
	*features = ACPI_CAP_PERF_MSRS;
	return (0);
}

static void
est_identify(driver_t *driver, device_t parent)
{
	device_t child;
	u_int p[4];

	/* Make sure we're not being doubly invoked. */
	if (device_find_child(parent, "est", -1) != NULL)
		return;

	/* Check that CPUID is supported and the vendor is Intel.*/
	if (cpu_high == 0 || strcmp(cpu_vendor, GenuineIntel) != 0)
		return;

	/* Read capability bits and check if the CPU supports EST. */
	do_cpuid(1, p);
	if ((p[2] & 0x80) == 0)
		return;

	/*
	 * We add a child for each CPU since settings must be performed
	 * on each CPU in the SMP case.
	 */
	child = BUS_ADD_CHILD(parent, 0, "est", -1);
	if (child == NULL)
		device_printf(parent, "add est child failed\n");
}

static int
est_probe(device_t dev)
{
	device_t perf_dev;
	uint64_t msr;
	int error, type;
        
	if (resource_disabled("est", 0))
		return (ENXIO);

	/*
	 * If the ACPI perf driver has attached and is not just offering
	 * info, let it manage things.
	 */
	perf_dev = device_find_child(device_get_parent(dev), "acpi_perf", -1);
	if (perf_dev && device_is_attached(perf_dev)) {
		error = CPUFREQ_DRV_TYPE(perf_dev, &type);
		if (error == 0 && (type & CPUFREQ_FLAG_INFO_ONLY) == 0)
			return (ENXIO);
	}

	/* Attempt to enable SpeedStep if not currently enabled. */
	msr = rdmsr(MSR_MISC_ENABLE);
	if ((msr & MSR_SS_ENABLE) == 0) {
		wrmsr(MSR_MISC_ENABLE, msr | MSR_SS_ENABLE);
		if (bootverbose)
			device_printf(dev, "enabling SpeedStep\n");

		/* Check if the enable failed. */
		msr = rdmsr(MSR_MISC_ENABLE);
		if ((msr & MSR_SS_ENABLE) == 0) {
			device_printf(dev, "failed to enable SpeedStep\n");
			return (ENXIO);
		}
	}

	device_set_desc(dev, "Enhanced SpeedStep Frequency Control");
	return (0);
}

static int
est_attach(device_t dev)
{
	struct est_softc *sc;

	sc = device_get_softc(dev);
	sc->dev = dev;

	/* Check CPU for supported settings. */
	if (est_get_info(dev))
		return (ENXIO);

	cpufreq_register(dev);
	return (0);
}

static int
est_detach(device_t dev)
{
	struct est_softc *sc;

	sc = device_get_softc(dev);
	if (sc->acpi_settings)
		free(sc->freq_list, M_DEVBUF);
	return (ENXIO);
}

/*
 * Probe for supported CPU settings.  First, check our static table of
 * settings.  If no match, try using the ones offered by acpi_perf
 * (i.e., _PSS).  We use ACPI second because some systems (IBM R/T40
 * series) export both legacy SMM IO-based access and direct MSR access
 * but the direct access specifies invalid values for _PSS.
 */
static int
est_get_info(device_t dev)
{
	struct est_softc *sc;
	uint64_t msr;
	int error;

	sc = device_get_softc(dev);
	msr = rdmsr(MSR_PERF_STATUS);
	error = est_table_info(dev, msr, INTEL_BUS_CLK, &sc->freq_list);
	if (error)
		error = est_acpi_info(dev, &sc->freq_list);

	if (error) {
		printf(
	"est: CPU supports Enhanced Speedstep, but is not recognized.\n"
	"est: cpu_vendor %s, msr %0jx\n", cpu_vendor, msr);
		return (ENXIO);
	}

	return (0);
}

static int
est_acpi_info(device_t dev, freq_info **freqs)
{
	struct est_softc *sc;
	struct cf_setting *sets;
	freq_info *table;
	device_t perf_dev;
	int count, error, i;

	perf_dev = device_find_child(device_get_parent(dev), "acpi_perf", -1);
	if (perf_dev == NULL || !device_is_attached(perf_dev))
		return (ENXIO);

	/* Fetch settings from acpi_perf. */
	sc = device_get_softc(dev);
	table = NULL;
	sets = malloc(MAX_SETTINGS * sizeof(*sets), M_TEMP, M_NOWAIT);
	if (sets == NULL)
		return (ENOMEM);
	error = CPUFREQ_DRV_SETTINGS(perf_dev, sets, &count);
	if (error)
		goto out;

	/* Parse settings into our local table format. */
	table = malloc((count + 1) * sizeof(freq_info), M_DEVBUF, M_NOWAIT);
	if (table == NULL) {
		error = ENOMEM;
		goto out;
	}
	for (i = 0; i < count; i++) {
		/*
		 * TODO: Figure out validity checks for id16.  Linux checks
		 * that the control and status values match.
		 */
		table[i].freq = sets[i].freq;
		table[i].volts = sets[i].volts;
		table[i].id16 = sets[i].spec[0];
		table[i].power = sets[i].power;
	}

	/* Mark end of table with a terminator. */
	bzero(&table[i], sizeof(freq_info));

	sc->acpi_settings = TRUE;
	*freqs = table;
	error = 0;

out:
	if (sets)
		free(sets, M_TEMP);
	if (error && table)
		free(table, M_DEVBUF);
	return (error);
}

static int
est_table_info(device_t dev, uint64_t msr, uint32_t bus_clk, freq_info **freqs)
{
	cpu_info *p;
	uint32_t id;

	/* Find a table which matches (vendor, id, bus_clk). */
	id = msr >> 32;
	for (p = ESTprocs; p->id32 != 0; p++) {
		if (strcmp(p->vendor, cpu_vendor) == 0 && p->id32 == id &&
		    p->bus_clk == bus_clk)
			break;
	}
	if (p->id32 == 0)
		return (EOPNOTSUPP);

	/* Make sure the current setpoint is valid. */
	if (est_get_current(p->freqtab) == NULL) {
		device_printf(dev, "current setting not found in table\n");
		return (EOPNOTSUPP);
	}

	*freqs = p->freqtab;
	return (0);
}

static freq_info *
est_get_current(freq_info *freq_list)
{
	freq_info *f;
	int i;
	uint16_t id16;

	/*
	 * Try a few times to get a valid value.  Sometimes, if the CPU
	 * is in the middle of an asynchronous transition (i.e., P4TCC),
	 * we get a temporary invalid result.
	 */
	for (i = 0; i < 5; i++) {
		id16 = rdmsr(MSR_PERF_STATUS) & 0xffff;
		for (f = freq_list; f->id16 != 0; f++) {
			if (f->id16 == id16)
				return (f);
		}
		DELAY(100);
	}
	return (NULL);
}

static int
est_settings(device_t dev, struct cf_setting *sets, int *count)
{
	struct est_softc *sc;
	freq_info *f;
	int i;

	sc = device_get_softc(dev);
	if (*count < EST_MAX_SETTINGS)
		return (E2BIG);

	i = 0;
	for (f = sc->freq_list; f->freq != 0; f++, i++) {
		sets[i].freq = f->freq;
		sets[i].volts = f->volts;
		sets[i].power = f->power;
		sets[i].lat = EST_TRANS_LAT;
		sets[i].dev = dev;
	}
	*count = i;

	return (0);
}

static int
est_set(device_t dev, const struct cf_setting *set)
{
	struct est_softc *sc;
	freq_info *f;
	uint64_t msr;

	/* Find the setting matching the requested one. */
	sc = device_get_softc(dev);
	for (f = sc->freq_list; f->freq != 0; f++) {
		if (f->freq == set->freq)
			break;
	}
	if (f->freq == 0)
		return (EINVAL);

	/* Read the current register, mask out the old, set the new id. */
	msr = rdmsr(MSR_PERF_CTL);
	msr = (msr & ~0xffff) | f->id16;
	wrmsr(MSR_PERF_CTL, msr);

	/* Wait a short while for the new setting.  XXX Is this necessary? */
	DELAY(EST_TRANS_LAT);

	return (0);
}

static int
est_get(device_t dev, struct cf_setting *set)
{
	struct est_softc *sc;
	freq_info *f;

	sc = device_get_softc(dev);
	f = est_get_current(sc->freq_list);
	if (f == NULL)
		return (ENXIO);

	set->freq = f->freq;
	set->volts = f->volts;
	set->power = f->power;
	set->lat = EST_TRANS_LAT;
	set->dev = dev;
	return (0);
}

static int
est_type(device_t dev, int *type)
{

	if (type == NULL)
		return (EINVAL);

	*type = CPUFREQ_TYPE_ABSOLUTE;
	return (0);
}
