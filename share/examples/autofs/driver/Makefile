# $Id$
# $FreeBSD: src/share/examples/autofs/driver/Makefile,v 1.5 2004/12/21 08:47:08 ru Exp $

PROG=autodriver

SRCS=		autodriver.c
NO_MAN=
WARNS=		4
CFLAGS+=	-g
BINDIR?=	/sbin

DPADD+=		${.OBJDIR}/../libautofs/libautofs.a
#LDADD+=		-lautofs
LDADD+=		${.OBJDIR}/../libautofs/libautofs.a
LDFLAGS+=	-L${.OBJDIR}/../libautofs
CFLAGS+=	-I${.CURDIR}/../libautofs

.include <bsd.prog.mk>
