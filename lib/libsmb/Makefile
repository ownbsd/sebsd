# $FreeBSD: src/lib/libsmb/Makefile,v 1.10 2006/03/16 15:18:16 ru Exp $

CONTRIBDIR=	${.CURDIR}/../../contrib/smbfs
.PATH: ${CONTRIBDIR}/lib/smb

LIB=	smb
SHLIB_MAJOR=	2
DPADD=	${LIBKICONV}
LDADD=	-lkiconv

SRCS=	rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
	print.c \
	kiconv.c \
	nb.c nb_name.c nb_net.c nbns_rq.c

WARNS?=	1
CFLAGS+= -DSMB_CFG_FILE=\"/etc/nsmb.conf\" -I${CONTRIBDIR}/include

.include <bsd.lib.mk>
