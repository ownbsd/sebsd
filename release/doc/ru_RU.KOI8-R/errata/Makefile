# The FreeBSD Russian Documentation Project
#
# $FreeBSDru: frdp/release/doc/ru_RU.KOI8-R/errata/Makefile,v 1.2 2004/06/29 07:55:42 den Exp $
# $FreeBSD: src/release/doc/ru_RU.KOI8-R/errata/Makefile,v 1.1 2004/09/10 13:56:40 den Exp $
#
# Original revision: 1.5
#

RELN_ROOT?= ${.CURDIR}/../..
.ifdef NO_LANGCODE_IN_DESTDIR
DESTDIR?= ${DOCDIR}/errata
.else
DESTDIR?= ${DOCDIR}/ru_RU.KOI8-R/errata
.endif

DOC?= article
FORMATS?= html
INSTALL_COMPRESSED?= gz
INSTALL_ONLY_COMPRESSED?=

# SGML content
SRCS+=	article.sgml

.include "${RELN_ROOT}/share/mk/doc.relnotes.mk"
.include "${DOC_PREFIX}/share/mk/doc.project.mk"
