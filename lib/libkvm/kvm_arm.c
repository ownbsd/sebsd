/*	$NetBSD: kvm_powerpc.c,v 1.4 1998/02/03 06:50:07 mycroft Exp $	*/

/*-
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ARM machine dependent routines for kvm.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libkvm/kvm_arm.c,v 1.3 2005/10/03 14:21:14 cognet Exp $");

#include <sys/param.h>
#include <sys/elf32.h>
#include <sys/mman.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#include <machine/pmap.h>

#include <db.h>
#include <limits.h>
#include <kvm.h>
#include <stdlib.h>

#include "kvm_private.h"

struct vmstate {
	pd_entry_t *l1pt;
	void *mmapbase;
	size_t mmapsize;
};

static int
_kvm_maphdrs(kvm_t *kd, size_t sz)
{
	struct vmstate *vm = kd->vmst;

	/* munmap() previous mmap(). */
	if (vm->mmapbase != NULL) {
		munmap(vm->mmapbase, vm->mmapsize);
		vm->mmapbase = NULL;
	}

	vm->mmapsize = sz;
	vm->mmapbase = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, kd->pmfd, 0);
	if (vm->mmapbase == MAP_FAILED) {
		_kvm_err(kd, kd->program, "cannot mmap corefile");
		return (-1);
	}

	return (0);
}

/*
 * Translate a physical memory address to a file-offset in the crash-dump.
 */
static size_t
_kvm_pa2off(kvm_t *kd, uint64_t pa, off_t *ofs, size_t pgsz)
{
	Elf32_Ehdr *e = kd->vmst->mmapbase;
	Elf32_Phdr *p = (Elf32_Phdr*)((char*)e + e->e_phoff);
	int n = e->e_phnum;

	while (n && (pa < p->p_paddr || pa >= p->p_paddr + p->p_memsz))
		p++, n--;
	if (n == 0)
		return (0);

	*ofs = (pa - p->p_paddr) + p->p_offset;
	if (pgsz == 0)
		return (p->p_memsz - (pa - p->p_paddr));
	return (pgsz - ((size_t)pa & (pgsz - 1)));
}

void
_kvm_freevtop(kvm_t *kd)
{
	if (kd->vmst != 0) {
		if (kd->vmst->mmapbase != NULL)
			munmap(kd->vmst->mmapbase, kd->vmst->mmapsize);
		free(kd->vmst);
		kd->vmst = NULL;
	}
}

int
_kvm_initvtop(kvm_t *kd)
{
	struct vmstate *vm = _kvm_malloc(kd, sizeof(*vm));
	struct nlist nlist[2];
	u_long kernbase, physaddr, pa;
	pd_entry_t *l1pt;
	Elf32_Ehdr *ehdr;
	size_t hdrsz;
	
	if (vm == 0) {
		_kvm_err(kd, kd->program, "cannot allocate vm");
		return (-1);
	}
	kd->vmst = vm;
	vm->l1pt = NULL;
	if (_kvm_maphdrs(kd, sizeof(Elf32_Ehdr)) == -1)
		return (-1);
	ehdr = kd->vmst->mmapbase;
	hdrsz = ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum;
	if (_kvm_maphdrs(kd, hdrsz) == -1)
		return (-1);
	nlist[0].n_name = "kernbase";
	nlist[1].n_name = NULL;
	if (kvm_nlist(kd, nlist) != 0)
		kernbase = KERNBASE;
	else
		kernbase = nlist[0].n_value;

	nlist[0].n_name = "physaddr";
	if (kvm_nlist(kd, nlist) != 0) {
		_kvm_err(kd, kd->program, "couldn't get phys addr");
		return (-1);
	}
	physaddr = nlist[0].n_value;
	nlist[0].n_name = "kernel_l1pa";
	if (kvm_nlist(kd, nlist) != 0) {
		_kvm_err(kd, kd->program, "bad namelist");
		return (-1);
	}
	if (kvm_read(kd, (nlist[0].n_value - kernbase + physaddr), &pa,
	    sizeof(pa)) != sizeof(pa)) {
		_kvm_err(kd, kd->program, "cannot read kernel_l1pa");
		return (-1);
	}
	l1pt = _kvm_malloc(kd, L1_TABLE_SIZE);
	if (kvm_read(kd, pa, l1pt, L1_TABLE_SIZE) != L1_TABLE_SIZE) {
		_kvm_err(kd, kd->program, "cannot read l1pt");
		free(l1pt);
		return (-1);
	}
	vm->l1pt = l1pt;
	return 0;
}

/* from arm/pmap.c */
#define	L1_IDX(va)		(((vm_offset_t)(va)) >> L1_S_SHIFT)
/* from arm/pmap.h */
#define	L1_TYPE_INV	0x00		/* Invalid (fault) */
#define	L1_TYPE_C	0x01		/* Coarse L2 */
#define	L1_TYPE_S	0x02		/* Section */
#define	L1_TYPE_F	0x03		/* Fine L2 */
#define	L1_TYPE_MASK	0x03		/* mask of type bits */

#define	l1pte_section_p(pde)	(((pde) & L1_TYPE_MASK) == L1_TYPE_S)
#define	l1pte_valid(pde)	((pde) != 0)
#define	l2pte_valid(pte)	((pte) != 0)
#define l2pte_index(v)		(((v) & L2_ADDR_BITS) >> L2_S_SHIFT)


int
_kvm_kvatop(kvm_t *kd, u_long va, off_t *pa)
{
	u_long offset = va & (PAGE_SIZE - 1);
	struct vmstate *vm = kd->vmst;
	pd_entry_t pd;
	pt_entry_t pte;
	u_long pte_pa;

	if (vm->l1pt == NULL)
		return (_kvm_pa2off(kd, va, pa, PAGE_SIZE));
	pd = vm->l1pt[L1_IDX(va)];
	if (!l1pte_valid(pd))
		goto invalid;
	if (l1pte_section_p(pd)) {
		/* 1MB section mapping. */
		*pa = ((u_long)pd & L1_S_ADDR_MASK) + (va & L1_S_OFFSET);
		return  (_kvm_pa2off(kd, *pa, pa, L1_S_SIZE));
	}
	pte_pa = (pd & L1_ADDR_MASK) + l2pte_index(va) * sizeof(pte);
	_kvm_pa2off(kd, pte_pa, (off_t *)&pte_pa, L1_S_SIZE);
	if (lseek(kd->pmfd, pte_pa, 0) == -1) {
		_kvm_syserr(kd, kd->program, "_kvm_kvatop: lseek");
		goto invalid;
	}
	if (read(kd->pmfd, &pte, sizeof(pte)) != sizeof (pte)) {
		_kvm_syserr(kd, kd->program, "_kvm_kvatop: read");
		goto invalid;
	}
	if (!l2pte_valid(pte)) {
		goto invalid;
	}
	if ((pte & L2_TYPE_MASK) == L2_TYPE_L) {
		*pa = (pte & L2_L_FRAME) | (va & L2_L_OFFSET);
		return (_kvm_pa2off(kd, *pa, pa, L2_L_SIZE));
	}
	*pa = (pte & L2_S_FRAME) | (va & L2_S_OFFSET);
	return (_kvm_pa2off(kd, *pa, pa, PAGE_SIZE));
invalid:
	_kvm_err(kd, 0, "Invalid address (%x)", va);
	return 0;
}

/*
 * Machine-dependent initialization for ALL open kvm descriptors,
 * not just those for a kernel crash dump.  Some architectures
 * have to deal with these NOT being constants!  (i.e. m68k)
 */
int
_kvm_mdopen(kd)
	kvm_t	*kd;
{

#ifdef FBSD_NOT_YET
	kd->usrstack = USRSTACK;
	kd->min_uva = VM_MIN_ADDRESS;
	kd->max_uva = VM_MAXUSER_ADDRESS;
#endif

	return (0);
}
