#
# $FreeBSD: src/usr.sbin/auditd/Makefile,v 1.1 2006/02/02 10:15:30 rwatson Exp $
#

OPENBSMDIR=${.CURDIR}/../../contrib/openbsm
.PATH: ${OPENBSMDIR}/bin/auditd

PROG=	auditd
SRCS=	auditd.c audit_warn.c
MAN=	auditd.8

DPADD=	${LIBBSM}
LDADD=	-lbsm

.include <bsd.prog.mk>
