# $FreeBSD: src/usr.bin/tar/Makefile,v 1.25 2006/03/08 02:49:12 kientzle Exp $

PROG=	bsdtar
VERSION=	1.2.41
SRCS=	bsdtar.c getdate.y matching.c read.c tree.c util.c write.c
WARNS?=	5
DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz
CFLAGS+=	-DPACKAGE_VERSION=\"${VERSION}\"
CFLAGS+=	-I${.CURDIR}
SYMLINKS=	bsdtar ${BINDIR}/tar
MLINKS=	bsdtar.1 tar.1

.include <bsd.prog.mk>
