# $FreeBSD: src/lib/libftpio/Makefile,v 1.14 2005/07/22 17:19:01 kensmith Exp $

LIB=		ftpio
SHLIB_MAJOR=	6

SRCS=		ftpio.c ftperr.c
INCS=		ftpio.h
CFLAGS+=	-I${.CURDIR} -Wall
CFLAGS+=	-DINET6
MAN=		ftpio.3
CLEANFILES=	ftperr.c

ftperr.c:	ftp.errors
	@echo '#include <stdio.h>' > ${.TARGET}
	@echo '#include "ftpio.h"' >> ${.TARGET}
	@echo "struct ftperr ftpErrList[] = {" \ >>  ${.TARGET}
	@cat ${.ALLSRC} \
	  | grep -v ^# \
	  | sort \
	  | while read NUM STRING; do \
	    echo "  { $${NUM}, \"$${STRING}\" },"; \
	  done >> ${.TARGET}
	@echo "};" >> ${.TARGET}
	@echo -n "int const ftpErrListLength = " >> ${.TARGET}
	@echo "sizeof(ftpErrList) / sizeof(*ftpErrList);" >> ${.TARGET}

.include <bsd.lib.mk>
