# 	@(#)Makefile	8.1 (Berkeley) 6/6/93
# $FreeBSD: src/usr.sbin/config/Makefile,v 1.35 2005/12/09 06:46:40 ru Exp $

PROG=	config
MAN=	config.5 config.8
SRCS=	config.y main.c lang.l mkmakefile.c mkheaders.c \
	mkoptions.c y.tab.h

WARNS?=	6
CFLAGS+= -I. -I${.CURDIR}

DPADD=	${LIBL}
LDADD=	-ll

mkmakefile.o: configvers.h

.include <bsd.prog.mk>
