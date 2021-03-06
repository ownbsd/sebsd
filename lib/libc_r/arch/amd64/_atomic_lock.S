/*
 * Copyright (c) 1995-1998 John Birrell <jb@cimlogic.com.au>.
 * All rights reserved.
 * copyright Douglas Santry 1996
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the above copyright is retained
 * in the source form.
 *
 * THIS SOFTWARE IS PROVIDED BY Douglas Santry AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE   
 * ARE DISCLAIMED.  IN NO EVENT SHALL Douglas Santry OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL   
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS      
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF  
 * SUCH DAMAGE.    
 */

#if defined(LIBC_RCS) && !defined(lint)
	.text
	.asciz "$FreeBSD: src/lib/libc_r/arch/amd64/_atomic_lock.S,v 1.6 2003/06/02 22:30:59 peter Exp $"
#endif /* LIBC_RCS and not lint */

#include <machine/asm.h>

/*
 * Atomicly lock a location with an identifier provided the location
 * is not currently locked.
 *
 * long _atomic_lock(long *);
 * eax will contain the return value (zero if lock obtained).
 */
ENTRY(_atomic_lock)
	movl	$1, %eax
	xchgq	%rax, (%rdi)
	ret

