# $Id$
# $FreeBSD: src/usr.sbin/bluetooth/hccontrol/Makefile,v 1.3 2003/10/12 22:04:23 emax Exp $

PROG=		hccontrol
MAN=		hccontrol.8
SRCS=		send_recv.c link_policy.c link_control.c \
		host_controller_baseband.c info.c status.c node.c hccontrol.c \
		util.c
WARNS?=		2

DPADD=		${LIBBLUETOOTH}
LDADD=		-lbluetooth

.include <bsd.prog.mk>
