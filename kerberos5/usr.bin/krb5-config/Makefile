# $FreeBSD: src/kerberos5/usr.bin/krb5-config/Makefile,v 1.15 2005/02/24 22:24:24 nectar Exp $

SCRIPTS=krb5-config
MAN=	krb5-config.1

CLEANFILES= krb5-config

krb5-config: krb5-config.in
	sed -e "s,@PACKAGE\@,FreeBSD heimdal,g" \
	    -e "s,@VERSION\@,0.6.3,g" \
	    -e "s,@prefix\@,/usr,g" \
	    -e "s,@exec_prefix\@,/usr,g" \
	    -e "s,@libdir\@,${LIBDIR},g" \
	    -e "s,@includedir\@,${INCLUDEDIR},g" \
	    -e "s,@LIB_crypt\@,-lcrypt,g" \
	    -e "s,@LIB_dbopen\@,,g" \
	    -e "s,@LIB_des_appl\@,-lcrypto,g" \
	    -e "s,@LIBS\@,-lcom_err,g" \
	    -e "s,@INCLUDE_des@,,g" \
	    ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>

.PATH: ${KRB5DIR}/tools
