# $FreeBSD: src/gnu/usr.bin/gdb/arch/alpha/Makefile,v 1.3 2004/12/05 06:58:59 marcel Exp $

.if !defined(GDB_CROSS_DEBUGGER)
LIBSRCS+= core-regset.c solib.c solib-legacy.c solib-svr4.c
LIBSRCS+= alphabsd-nat.c
.endif
LIBSRCS+= alpha-mdebug-tdep.c alpha-tdep.c alphabsd-tdep.c alphafbsd-tdep.c

nm.h:
	echo '#include "alpha/nm-fbsd.h"' > ${.TARGET}

tm.h:
	echo '#include "alpha/tm-alpha.h"' > ${.TARGET}
