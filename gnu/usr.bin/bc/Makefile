# $FreeBSD: src/gnu/usr.bin/bc/Makefile,v 1.26 2006/03/17 18:54:22 ru Exp $

.include <bsd.own.mk>

BCDIR=	${.CURDIR}/../../../contrib/bc
.PATH: ${BCDIR}/bc ${BCDIR}/lib ${BCDIR}/doc ${BCDIR}/Examples

PROG=	bc
SRCS=	bc.y execute.c global.c load.c main.c scan.l storage.c util.c \
	number.c
CFLAGS+=-I. -I${.CURDIR} -I${BCDIR}/h -I${BCDIR}/bc
CFLAGS+=-DHAVE_CONFIG_H
DPADD=	${LIBREADLINE} ${LIBTERMCAP}
LDADD=	-lreadline -ltermcap

.if ${MK_EXAMPLES} != "no"
FILES=	ckbook.b pi.b primes.b twins.b
FILESDIR=	${SHAREDIR}/examples/bc
.endif

.include <bsd.prog.mk>
