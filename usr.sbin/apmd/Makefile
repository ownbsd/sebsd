# $FreeBSD: src/usr.sbin/apmd/Makefile,v 1.8 2004/01/21 13:38:17 ru Exp $

PROG=	apmd
MAN=	apmd.8
MANSUBDIR= /i386
SRCS=	apmd.c apmdlex.l apmdparse.y y.tab.h

DPADD=	${LIBL}
LDADD=	-ll

YFLAGS+=-v
CFLAGS+=-I. -I${.CURDIR} #-DYY_STACK_USED
# for debug:
#CFLAGS+= -g -DDEBUG

CLEANFILES= y.output

test:
	./apmd -d -f etc/apmd.conf -n

.include <bsd.prog.mk>
