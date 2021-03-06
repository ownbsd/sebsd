# $FreeBSD: src/lib/libc_r/Makefile,v 1.43 2006/03/18 21:37:04 ru Exp $
#
# All library objects contain FreeBSD revision strings by default; they may be
# excluded as a space-saving measure.  To produce a library that does
# not contain these strings, add -DSTRIP_FBSDID (see <sys/cdefs.h>) to CFLAGS
# below.  Note, there are no IDs for syscall stubs whose sources are generated.
# To included legacy CSRG sccsid strings, add -DLIBC_SCCS and -DSYSLIBC_SCCS
# (for system call stubs) to CFLAGS below.  -DSYSLIBC_SCCS affects just the
# system call stubs.
LIB=c_r
SHLIB_MAJOR= 6
CFLAGS+=-DPTHREAD_KERNEL 
CFLAGS+=-I${.CURDIR}/../libc/include -I${.CURDIR}/uthread \
	-I${.CURDIR}/../../include

# Uncomment this if you want libc_r to contain debug information for
# thread locking.
CFLAGS+=-D_LOCK_DEBUG

# enable extra internal consistancy checks
CFLAGS+=-D_PTHREADS_INVARIANTS

PRECIOUSLIB=

.include "${.CURDIR}/uthread/Makefile.inc"
.include "${.CURDIR}/sys/Makefile.inc"

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "sparc64"
SYMLINKS+=lib${LIB}.a ${LIBDIR}/libpthread.a
.if !defined(NO_PIC)
SYMLINKS+=lib${LIB}.so ${SHLIBDIR}/libpthread.so
.endif
.if ${MK_PROFILE} != "no"
SYMLINKS+=lib${LIB}_p.a ${LIBDIR}/libpthread_p.a
.endif
.endif

.include <bsd.lib.mk>
