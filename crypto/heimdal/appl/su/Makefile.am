# $Id$
# $FreeBSD: src/crypto/heimdal/appl/su/Makefile.am,v 1.2 2005/07/07 14:16:37 kensmith Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_krb4) $(INCLUDE_des)

.if defined(_FREEFALL_CONFIG)
CFLAGS+=-D_FREEFALL_CONFIG
.endif

bin_PROGRAMS = su
bin_SUIDS = su
su_SOURCES = su.c

LDADD = $(LIB_kafs) \
	$(top_builddir)/lib/krb5/libkrb5.la \
	$(LIB_krb4) \
	$(LIB_des) \
	$(top_builddir)/lib/asn1/libasn1.la \
	$(LIB_roken)
