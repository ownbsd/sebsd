# $FreeBSD: src/release/doc/de_DE.ISO8859-1/relnotes/Makefile.inc,v 1.1 2002/03/12 18:35:52 ue Exp $
# $FreeBSDde: de-docproj/relnotes/de_DE.ISO8859-1/relnotes/Makefile.inc,v 1.2 2002/03/10 18:41:45 ue Exp $
# $Id$
# basiert auf: 1.2

.ifdef NO_LANGCODE_IN_DESTDIR
DESTDIR?=       ${DOCDIR}/relnotes/${.CURDIR:T}
.else
DESTDIR?=       ${DOCDIR}/de_DE.ISO8859-1/relnotes/${.CURDIR:T}
.endif

