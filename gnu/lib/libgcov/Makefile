# $FreeBSD: src/gnu/lib/libgcov/Makefile,v 1.4 2006/03/18 21:37:04 ru Exp $

NO_PROFILE=
.include <bsd.own.mk>
.include "${.CURDIR}/../../usr.bin/cc/Makefile.tgt"

GCCDIR=	${.CURDIR}/../../../contrib/gcc
.PATH: ${GCCDIR}/config/${GCC_CPU} ${GCCDIR}

LIB=		gcov

CFLAGS+=	-DIN_GCC -DIN_LIBGCC2 -D__GCC_FLOAT_NOT_NEEDED
CFLAGS+=	-D_PTHREADS -DGTHREAD_USE_WEAK
CFLAGS+=	-I${.CURDIR}/../../usr.bin/cc/cc_tools \
		-I${GCCDIR}/config -I${GCCDIR} -I.
#
# Library members defined in libgcov.c.
SYMS=		_gcov _gcov_merge_add _gcov_merge_delta _gcov_merge_single

OBJS=		${SYMS:S/$/.o/}
OBJS_T=		${SYMS:S/$/.o/}
OBJS_P=		${SYMS:S/$/.po/}
OBJS_S=		${SYMS:S/$/.So/}

COMMONHDRS=	tm.h tconfig.h gcov-iov.h
CLEANFILES+=	${COMMONHDRS}

${COMMONHDRS}: ${.CURDIR}/../../usr.bin/cc/cc_tools/Makefile
	${MAKE} -f ${.ALLSRC} MFILE=${.ALLSRC} GCCDIR=${GCCDIR} ${.TARGET}

${OBJS} beforedepend: ${COMMONHDRS}

${OBJS_T}: libgcov.c
	${CC} -c ${CFLAGS} -DL${.PREFIX} -o ${.TARGET} ${.ALLSRC:M*.c}

.if !defined(NO_PIC)
${OBJS_S}: libgcov.c
	${CC} -c ${PICFLAG} ${CFLAGS} -DL${.PREFIX} -o ${.TARGET} ${.ALLSRC:M*.c}
.endif

.if ${MK_PROFILE} != "no"
${OBJS_P}: libgcc2.c
	${CC} -c -p ${CFLAGS} -DL${.PREFIX} -o ${.TARGET} ${.ALLSRC:M*.c}
.endif

.include <bsd.lib.mk>
