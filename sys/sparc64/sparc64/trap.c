/*-
 * Copyright (c) 2001, Jake Burkholder
 * Copyright (C) 1994, David Greenman
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the University of Utah, and William Jolitz.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from: @(#)trap.c        7.4 (Berkeley) 5/13/91
 * 	from: FreeBSD: src/sys/i386/i386/trap.c,v 1.197 2001/07/19
 * $FreeBSD: src/sys/sparc64/sparc64/trap.c,v 1.79 2006/02/28 21:25:00 brueffer Exp $
 */

#include "opt_ddb.h"
#include "opt_ktr.h"
#include "opt_ktrace.h"
#include "opt_mac.h"

#include <sys/param.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/interrupt.h>
#include <sys/ktr.h>
#include <sys/lock.h>
#include <sys/mac.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/pioctl.h>
#include <sys/ptrace.h>
#include <sys/proc.h>
#include <sys/smp.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/sysent.h>
#include <sys/vmmeter.h>
#ifdef KTRACE
#include <sys/uio.h>
#include <sys/ktrace.h>
#endif
#include <security/audit/audit.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_extern.h>
#include <vm/vm_param.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>

#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/intr_machdep.h>
#include <machine/pcb.h>
#include <machine/smp.h>
#include <machine/trap.h>
#include <machine/tstate.h>
#include <machine/tte.h>
#include <machine/tlb.h>
#include <machine/tsb.h>
#include <machine/watch.h>

void trap(struct trapframe *tf);
void syscall(struct trapframe *tf);

static int trap_pfault(struct thread *td, struct trapframe *tf);

extern char copy_fault[];
extern char copy_nofault_begin[];
extern char copy_nofault_end[];

extern char fs_fault[];
extern char fs_nofault_begin[];
extern char fs_nofault_end[];
extern char fs_nofault_intr_begin[];
extern char fs_nofault_intr_end[];

extern char fas_fault[];
extern char fas_nofault_begin[];
extern char fas_nofault_end[];

extern char *syscallnames[];

const char *trap_msg[] = {
	"reserved",
	"instruction access exception",
	"instruction access error",
	"instruction access protection",
	"illtrap instruction",
	"illegal instruction",
	"privileged opcode",
	"floating point disabled",
	"floating point exception ieee 754",
	"floating point exception other",
	"tag overflow",
	"division by zero",
	"data access exception",
	"data access error",
	"data access protection",
	"memory address not aligned",
	"privileged action",
	"async data error",
	"trap instruction 16",
	"trap instruction 17",
	"trap instruction 18",
	"trap instruction 19",
	"trap instruction 20",
	"trap instruction 21",
	"trap instruction 22",
	"trap instruction 23",
	"trap instruction 24",
	"trap instruction 25",
	"trap instruction 26",
	"trap instruction 27",
	"trap instruction 28",
	"trap instruction 29",
	"trap instruction 30",
	"trap instruction 31",
	"fast instruction access mmu miss",
	"fast data access mmu miss",
	"interrupt",
	"physical address watchpoint",
	"virtual address watchpoint",
	"corrected ecc error",
	"spill",
	"fill",
	"fill",
	"breakpoint",
	"clean window",
	"range check",
	"fix alignment",
	"integer overflow",
	"syscall",
	"restore physical watchpoint",
	"restore virtual watchpoint",
	"kernel stack fault",
};

const int trap_sig[] = {
	SIGILL,			/* reserved */
	SIGILL,			/* instruction access exception */
	SIGILL,			/* instruction access error */
	SIGILL,			/* instruction access protection */
	SIGILL,			/* illtrap instruction */
	SIGILL,			/* illegal instruction */
	SIGBUS,			/* privileged opcode */
	SIGFPE,			/* floating point disabled */
	SIGFPE,			/* floating point exception ieee 754 */
	SIGFPE,			/* floating point exception other */
	SIGEMT,			/* tag overflow */
	SIGFPE,			/* division by zero */
	SIGILL,			/* data access exception */
	SIGILL,			/* data access error */
	SIGBUS,			/* data access protection */
	SIGBUS,			/* memory address not aligned */
	SIGBUS,			/* privileged action */
	SIGBUS,			/* async data error */
	SIGILL,			/* trap instruction 16 */
	SIGILL,			/* trap instruction 17 */
	SIGILL,			/* trap instruction 18 */
	SIGILL,			/* trap instruction 19 */
	SIGILL,			/* trap instruction 20 */
	SIGILL,			/* trap instruction 21 */
	SIGILL,			/* trap instruction 22 */
	SIGILL,			/* trap instruction 23 */
	SIGILL,			/* trap instruction 24 */
	SIGILL,			/* trap instruction 25 */
	SIGILL,			/* trap instruction 26 */
	SIGILL,			/* trap instruction 27 */
	SIGILL,			/* trap instruction 28 */
	SIGILL,			/* trap instruction 29 */
	SIGILL,			/* trap instruction 30 */
	SIGILL,			/* trap instruction 31 */
	SIGSEGV,		/* fast instruction access mmu miss */
	SIGSEGV,		/* fast data access mmu miss */
	-1,			/* interrupt */
	-1,			/* physical address watchpoint */
	-1,			/* virtual address watchpoint */
	-1,			/* corrected ecc error */
	SIGILL,			/* spill */
	SIGILL,			/* fill */
	SIGILL,			/* fill */
	SIGTRAP,		/* breakpoint */
	SIGILL,			/* clean window */
	SIGILL,			/* range check */
	SIGILL,			/* fix alignment */
	SIGILL,			/* integer overflow */
	SIGSYS,			/* syscall */
	-1,			/* restore physical watchpoint */
	-1,			/* restore virtual watchpoint */
	-1,			/* kernel stack fault */
};

CTASSERT(sizeof(struct trapframe) == 256);

int debugger_on_signal = 0;
SYSCTL_INT(_debug, OID_AUTO, debugger_on_signal, CTLFLAG_RW,
    &debugger_on_signal, 0, "");

void
trap(struct trapframe *tf)
{
	struct thread *td;
	struct proc *p;
	int error;
	int sig;
	ksiginfo_t ksi;

	td = PCPU_GET(curthread);

	CTR4(KTR_TRAP, "trap: %p type=%s (%s) pil=%#lx", td,
	    trap_msg[tf->tf_type & ~T_KERNEL],
	    (TRAPF_USERMODE(tf) ? "user" : "kernel"), rdpr(pil));

	PCPU_LAZY_INC(cnt.v_trap);

	if ((tf->tf_tstate & TSTATE_PRIV) == 0) {
		KASSERT(td != NULL, ("trap: curthread NULL"));
		KASSERT(td->td_proc != NULL, ("trap: curproc NULL"));

		p = td->td_proc;
		td->td_pticks = 0;
		td->td_frame = tf;
		if (td->td_ucred != p->p_ucred)
			cred_update_thread(td);

		switch (tf->tf_type) {
		case T_DATA_MISS:
		case T_DATA_PROTECTION:
		case T_INSTRUCTION_MISS:
			sig = trap_pfault(td, tf);
			break;
		case T_FILL:
			sig = rwindow_load(td, tf, 2);
			break;
		case T_FILL_RET:
			sig = rwindow_load(td, tf, 1);
			break;
		case T_SPILL:
			sig = rwindow_save(td);
			break;
		default:
			if (tf->tf_type < 0 || tf->tf_type >= T_MAX ||
			    trap_sig[tf->tf_type] == -1)
				panic("trap: bad trap type");
			sig = trap_sig[tf->tf_type];
			break;
		}

		if (sig != 0) {
			/* Translate fault for emulators. */
			if (p->p_sysent->sv_transtrap != NULL) {
				sig = p->p_sysent->sv_transtrap(sig,
				    tf->tf_type);
			}
			if (debugger_on_signal &&
			    (sig == 4 || sig == 10 || sig == 11))
				kdb_enter("trapsig");
			ksiginfo_init_trap(&ksi);
			ksi.ksi_signo = sig;
			ksi.ksi_code = (int)tf->tf_type; /* XXX not POSIX */
			/* ksi.ksi_addr = ? */
			ksi.ksi_trapno = (int)tf->tf_type;
			trapsignal(td, &ksi);
		}

		userret(td, tf);
		mtx_assert(&Giant, MA_NOTOWNED);
 	} else {
		KASSERT((tf->tf_type & T_KERNEL) != 0,
		    ("trap: kernel trap isn't"));

#ifdef KDB
		if (kdb_active) {
			kdb_reenter();
			return;
		}
#endif

		switch (tf->tf_type & ~T_KERNEL) {
#ifdef KDB
		case T_BREAKPOINT:
		case T_KSTACK_FAULT:
			error = (kdb_trap(tf->tf_type, 0, tf) == 0);
			TF_DONE(tf);
			break;
#ifdef notyet
		case T_PA_WATCHPOINT:
		case T_VA_WATCHPOINT:
			error = db_watch_trap(tf);
			break;
#endif
#endif
		case T_DATA_MISS:
		case T_DATA_PROTECTION:
		case T_INSTRUCTION_MISS:
			error = trap_pfault(td, tf);
			break;
		case T_DATA_EXCEPTION:
		case T_MEM_ADDRESS_NOT_ALIGNED:
			if ((tf->tf_sfsr & MMU_SFSR_FV) != 0 &&
			    MMU_SFSR_GET_ASI(tf->tf_sfsr) == ASI_AIUP) {
				if (tf->tf_tpc >= (u_long)copy_nofault_begin &&
				    tf->tf_tpc <= (u_long)copy_nofault_end) {
					tf->tf_tpc = (u_long)copy_fault;
					tf->tf_tnpc = tf->tf_tpc + 4;
					error = 0;
					break;
				}
				if (tf->tf_tpc >= (u_long)fs_nofault_begin &&
				    tf->tf_tpc <= (u_long)fs_nofault_end) {
					tf->tf_tpc = (u_long)fs_fault;
					tf->tf_tnpc = tf->tf_tpc + 4;
					error = 0;
					break;
				}
			}
			error = 1;	
			break;
		case T_DATA_ERROR:
			/*
			 * handle PCI poke/peek as per UltraSPARC IIi
			 * User's Manual 16.2.1.
			 */
#define MEMBARSYNC_INST	((u_int32_t)0x8143e040)
			if (tf->tf_tpc > (u_long)fas_nofault_begin &&
			    tf->tf_tpc < (u_long)fas_nofault_end &&
			    *(u_int32_t *)tf->tf_tpc == MEMBARSYNC_INST &&
			    ((u_int32_t *)tf->tf_tpc)[-2] == MEMBARSYNC_INST) {
				cache_flush();
				cache_enable();
				tf->tf_tpc = (u_long)fas_fault;
				tf->tf_tnpc = tf->tf_tpc + 4;
				error = 0;
				break;
			}
#undef MEMBARSYNC_INST
			error = 1;
			break;
		default:
			error = 1;
			break;
		}

		if (error != 0)
			panic("trap: %s", trap_msg[tf->tf_type & ~T_KERNEL]);
	}
	CTR1(KTR_TRAP, "trap: td=%p return", td);
}

static int
trap_pfault(struct thread *td, struct trapframe *tf)
{
	struct vmspace *vm;
	struct pcb *pcb;
	struct proc *p;
	vm_offset_t va;
	vm_prot_t prot;
	u_long ctx;
	int flags;
	int type;
	int rv;

	if (td == NULL)
		return (-1);
	KASSERT(td->td_pcb != NULL, ("trap_pfault: pcb NULL"));
	KASSERT(td->td_proc != NULL, ("trap_pfault: curproc NULL"));
	KASSERT(td->td_proc->p_vmspace != NULL, ("trap_pfault: vmspace NULL"));

	p = td->td_proc;

	rv = KERN_SUCCESS;
	ctx = TLB_TAR_CTX(tf->tf_tar);
	pcb = td->td_pcb;
	type = tf->tf_type & ~T_KERNEL;
	va = TLB_TAR_VA(tf->tf_tar);

	CTR4(KTR_TRAP, "trap_pfault: td=%p pm_ctx=%#lx va=%#lx ctx=%#lx",
	    td, p->p_vmspace->vm_pmap.pm_context[PCPU_GET(cpuid)], va, ctx);

	if (type == T_DATA_PROTECTION) {
		prot = VM_PROT_WRITE;
		flags = VM_FAULT_DIRTY;
	} else {
		if (type == T_DATA_MISS)
			prot = VM_PROT_READ;
		else
			prot = VM_PROT_READ | VM_PROT_EXECUTE;
		flags = VM_FAULT_NORMAL;
	}

	if (ctx != TLB_CTX_KERNEL) {
		if ((tf->tf_tstate & TSTATE_PRIV) != 0 &&
		    (tf->tf_tpc >= (u_long)fs_nofault_intr_begin &&
		     tf->tf_tpc <= (u_long)fs_nofault_intr_end)) {
			tf->tf_tpc = (u_long)fs_fault;
			tf->tf_tnpc = tf->tf_tpc + 4;
			return (0);
		}

		/*
		 * This is a fault on non-kernel virtual memory.
		 */
		vm = p->p_vmspace;

		/*
		 * Keep swapout from messing with us during this
		 * critical time.
		 */
		PROC_LOCK(p);
		++p->p_lock;
		PROC_UNLOCK(p);

		/* Fault in the user page. */
		rv = vm_fault(&vm->vm_map, va, prot, flags);

		/*
		 * Now the process can be swapped again.
		 */
		PROC_LOCK(p);
		--p->p_lock;
		PROC_UNLOCK(p);
	} else {
		/*
		 * This is a fault on kernel virtual memory.  Attempts to
		 * access kernel memory from user mode cause privileged
		 * action traps, not page fault.
		 */
		KASSERT(tf->tf_tstate & TSTATE_PRIV,
		    ("trap_pfault: fault on nucleus context from user mode"));

		/*
		 * Don't have to worry about process locking or stacks in the
		 * kernel.
		 */
		rv = vm_fault(kernel_map, va, prot, VM_FAULT_NORMAL);
	}

	CTR3(KTR_TRAP, "trap_pfault: return td=%p va=%#lx rv=%d",
	    td, va, rv);
	if (rv == KERN_SUCCESS)
		return (0);
	if (ctx != TLB_CTX_KERNEL && (tf->tf_tstate & TSTATE_PRIV) != 0) {
		if (tf->tf_tpc >= (u_long)fs_nofault_begin &&
		    tf->tf_tpc <= (u_long)fs_nofault_end) {
			tf->tf_tpc = (u_long)fs_fault;
			tf->tf_tnpc = tf->tf_tpc + 4;
			return (0);
		}
		if (tf->tf_tpc >= (u_long)copy_nofault_begin &&
		    tf->tf_tpc <= (u_long)copy_nofault_end) {
			tf->tf_tpc = (u_long)copy_fault;
			tf->tf_tnpc = tf->tf_tpc + 4;
			return (0);
		}
	}
	return ((rv == KERN_PROTECTION_FAILURE) ? SIGBUS : SIGSEGV);
}

/* Maximum number of arguments that can be passed via the out registers. */
#define	REG_MAXARGS	6

/*
 * Syscall handler. The arguments to the syscall are passed in the o registers
 * by the caller, and are saved in the trap frame. The syscall number is passed
 * in %g1 (and also saved in the trap frame).
 */
void
syscall(struct trapframe *tf)
{
	struct sysent *callp;
	struct thread *td;
	register_t args[8];
	register_t *argp;
	struct proc *p;
	u_long code;
	u_long tpc;
	int reg;
	int regcnt;
	int narg;
	int error;

	td = PCPU_GET(curthread);
	KASSERT(td != NULL, ("trap: curthread NULL"));
	KASSERT(td->td_proc != NULL, ("trap: curproc NULL"));

	p = td->td_proc;

	PCPU_LAZY_INC(cnt.v_syscall);

	narg = 0;
	error = 0;
	reg = 0;
	regcnt = REG_MAXARGS;

	td->td_pticks = 0;
	td->td_frame = tf;
	if (td->td_ucred != p->p_ucred)
		cred_update_thread(td);
	if (p->p_flag & P_SA)
		thread_user_enter(td);
	code = tf->tf_global[1];

	/*
	 * For syscalls, we don't want to retry the faulting instruction
	 * (usually), instead we need to advance one instruction.
	 */
	tpc = tf->tf_tpc;
	TF_DONE(tf);

	if (p->p_sysent->sv_prepsyscall) {
		/*
		 * The prep code is MP aware.
		 */
#if 0
		(*p->p_sysent->sv_prepsyscall)(tf, args, &code, &params);
#endif	
	} else 	if (code == SYS_syscall || code == SYS___syscall) {
		code = tf->tf_out[reg++];
		regcnt--;
	}

 	if (p->p_sysent->sv_mask)
 		code &= p->p_sysent->sv_mask;

	if (code >= p->p_sysent->sv_size)
		code = 0;
	callp = &p->p_sysent->sv_table[code];

	narg = callp->sy_narg & SYF_ARGMASK;

	if (narg <= regcnt) {
		argp = &tf->tf_out[reg];
		error = 0;
	} else {
		KASSERT(narg <= sizeof(args) / sizeof(args[0]),
		    ("Too many syscall arguments!"));
		argp = args;
		bcopy(&tf->tf_out[reg], args, sizeof(args[0]) * regcnt);
		error = copyin((void *)(tf->tf_out[6] + SPOFF +
		    offsetof(struct frame, fr_pad[6])),
		    &args[regcnt], (narg - regcnt) * sizeof(args[0]));
	}

	CTR5(KTR_SYSC, "syscall: td=%p %s(%#lx, %#lx, %#lx)", td,
	    syscallnames[code], argp[0], argp[1], argp[2]);

	/*
	 * Try to run the syscall without the MP lock if the syscall
	 * is MP safe.
	 */
	if ((callp->sy_narg & SYF_MPSAFE) == 0)
		mtx_lock(&Giant);

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSCALL))
		ktrsyscall(code, narg, argp);
#endif
	if (error == 0) {
		td->td_retval[0] = 0;
		td->td_retval[1] = 0;

		STOPEVENT(p, S_SCE, narg);	/* MP aware */

		PTRACESTOP_SC(p, td, S_PT_SCE);

		AUDIT_SYSCALL_ENTER(code, td);
#ifdef MAC
		error = mac_thread_syscall_enter(td, argp, code);
		if (error == 0)
			error = (*callp->sy_call)(td, argp);
		mac_thread_syscall_exit(td, argp, code, error);
#else
		error = (*callp->sy_call)(td, argp);
#endif
		AUDIT_SYSCALL_EXIT(error, td);

		CTR5(KTR_SYSC, "syscall: p=%p error=%d %s return %#lx %#lx ", p,
		    error, syscallnames[code], td->td_retval[0],
		    td->td_retval[1]);
	}
	
	/*
	 * MP SAFE (we may or may not have the MP lock at this point)
	 */
	switch (error) {
	case 0:
		tf->tf_out[0] = td->td_retval[0];
		tf->tf_out[1] = td->td_retval[1];
		tf->tf_tstate &= ~TSTATE_XCC_C;
		break;

	case ERESTART:
		/*
		 * Undo the tpc advancement we have done above, we want to
		 * reexecute the system call.
		 */
		tf->tf_tpc = tpc;
		tf->tf_tnpc -= 4;
		break;

	case EJUSTRETURN:
		break;

	default:
 		if (p->p_sysent->sv_errsize) {
 			if (error >= p->p_sysent->sv_errsize)
  				error = -1;	/* XXX */
   			else
  				error = p->p_sysent->sv_errtbl[error];
		}
		tf->tf_out[0] = error;
		tf->tf_tstate |= TSTATE_XCC_C;
		break;
	}

	/*
	 * Release Giant if we had to get it.  Don't use mtx_owned(),
	 * we want to catch broken syscalls.
	 */
	if ((callp->sy_narg & SYF_MPSAFE) == 0)
		mtx_unlock(&Giant);

	/*
	 * Handle reschedule and other end-of-syscall issues
	 */
	userret(td, tf);

#ifdef KTRACE
	if (KTRPOINT(td, KTR_SYSRET))
		ktrsysret(code, error, td->td_retval[0]);
#endif
	/*
	 * This works because errno is findable through the
	 * register set.  If we ever support an emulation where this
	 * is not the case, this code will need to be revisited.
	 */
	STOPEVENT(p, S_SCX, code);

	PTRACESTOP_SC(p, td, S_PT_SCX);

	WITNESS_WARN(WARN_PANIC, NULL, "System call %s returning",
	    (code >= 0 && code < SYS_MAXSYSCALL) ? syscallnames[code] : "???");
	mtx_assert(&sched_lock, MA_NOTOWNED);
	mtx_assert(&Giant, MA_NOTOWNED);
}
