# $FreeBSD: src/usr.sbin/tcpdchk/Makefile,v 1.12 2006/03/17 18:54:43 ru Exp $

.include <bsd.own.mk>

.PATH: ${.CURDIR}/../../contrib/tcp_wrappers

PROG=	tcpdchk
MAN=	tcpdchk.8
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c

CFLAGS+=-DREAL_DAEMON_DIR=\"/usr/libexec\" \
	-DSEVERITY=LOG_INFO -DRFC931_TIMEOUT=10 -DPROCESS_OPTIONS \
	-DHOSTS_DENY=\"/etc/hosts.deny\" -DHOSTS_ALLOW=\"/etc/hosts.allow\"
.if ${MK_INET6_SUPPORT} != "no"
CFLAGS+=-DINET6
.endif

DPADD=	${LIBWRAP}
LDADD=	-lwrap

.include <bsd.prog.mk>
