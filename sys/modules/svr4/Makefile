# $FreeBSD: src/sys/modules/svr4/Makefile,v 1.29 2006/01/26 19:21:27 alc Exp $

MAINTAINER=	newton@FreeBSD.org

.PATH:	${.CURDIR}/../../${MACHINE_ARCH}/svr4 ${.CURDIR}/../../compat/svr4
KMOD=	svr4
SRCS=	svr4_sysent.c svr4_sysvec.c opt_compat.h opt_mac.h opt_svr4.h \
	vnode_if.h imgact_svr4.c svr4_signal.c svr4_fcntl.c \
	svr4_misc.c svr4_ioctl.c svr4_stat.c svr4_filio.c svr4_ttold.c \
	svr4_termios.c svr4_stream.c svr4_socket.c svr4_sockio.c \
	svr4_machdep.c svr4_resource.c svr4_ipc.c
OBJS=	svr4_locore.o 

CLEANFILES=	svr4_assym.h svr4_genassym.o

svr4_assym.h: svr4_genassym.o
.if exists(@)
svr4_assym.h: @/kern/genassym.sh
.endif
	sh @/kern/genassym.sh svr4_genassym.o > ${.TARGET}

svr4_locore.o: svr4_locore.s svr4_assym.h
	${CC} -c -x assembler-with-cpp -DLOCORE ${CFLAGS} \
		${.IMPSRC} -o ${.TARGET}

svr4_genassym.o: svr4_genassym.c svr4.h @ machine
	${CC} -c ${CFLAGS:N-fno-common} ${.IMPSRC}

.if !defined(KERNBUILDDIR)
opt_compat.h:
	echo "#define COMPAT_43 1" > opt_compat.h

.if defined(DEBUG)
opt_svr4.h:
	echo "#define DEBUG_SVR4 1" > opt_svr4.h
.endif
.endif

.include <bsd.kmod.mk>
