# $FreeBSD: src/sys/modules/le/Makefile,v 1.1 2006/01/31 22:34:13 marius Exp $

.PATH: ${.CURDIR}/../../dev/le

KMOD=	if_le
SRCS=	am7990.c am79900.c ${if_le_ledma} if_le_pci.c lance.c
SRCS+=	bus_if.h device_if.h ${ofw_bus_if} pci_if.h

.if ${MACHINE_ARCH} == "sparc64"
if_le_ledma=	if_le_ledma.c
ofw_bus_if=	ofw_bus_if.h
.endif

.include <bsd.kmod.mk>
