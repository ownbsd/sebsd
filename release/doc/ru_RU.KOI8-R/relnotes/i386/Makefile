# The FreeBSD Russian Documentation Project
#
# $FreeBSD: src/release/doc/ru_RU.KOI8-R/relnotes/i386/Makefile,v 1.3 2004/09/02 06:22:17 den Exp $
# $FreeBSDru: frdp/release/doc/ru_RU.KOI8-R/relnotes/i386/Makefile,v 1.3 2004/09/02 05:50:33 den Exp $
#
# Original revision: 1.7

RELN_ROOT?= ${.CURDIR}/../../..

DOC?= article
FORMATS?= html
INSTALL_COMPRESSED?= gz
INSTALL_ONLY_COMPRESSED?=

JADEFLAGS+=	-V %generate-article-toc%

# SGML content
SRCS+=	article.sgml
SRCS+=	../common/relnotes.ent
SRCS+=	../common/new.sgml

URL_RELPREFIX?=	../../../../..

.include "${RELN_ROOT}/share/mk/doc.relnotes.mk"
.include "${DOC_PREFIX}/share/mk/doc.project.mk"
