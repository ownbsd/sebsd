# $FreeBSD: src/lib/libpam/modules/pam_passwdqc/Makefile,v 1.6 2003/03/09 20:06:36 obrien Exp $

SRCDIR=	${.CURDIR}/../../../../contrib/pam_modules/pam_passwdqc
.PATH: ${SRCDIR}

LIB=	pam_passwdqc
SRCS=	pam_passwdqc.c passwdqc_check.c passwdqc_random.c wordset_4k.c
MAN=	pam_passwdqc.8

WARNS?=	0
CFLAGS+= -I${SRCDIR}

DPADD=	${LIBCRYPT}
LDADD=	-lcrypt

.include <bsd.lib.mk>

