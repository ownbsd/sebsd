# $FreeBSD: src/sys/modules/ip6fw/Makefile,v 1.3 2002/01/11 15:48:48 ru Exp $

.PATH: ${.CURDIR}/../../netinet6

KMOD=	ip6fw
SRCS=	ip6_fw.c

CFLAGS+= -DIPV6FIREWALL

#If you want it verbose
#CFLAGS+= -DIPV6FIREWALL_VERBOSE
#CFLAGS+= -DIPV6FIREWALL_VERBOSE_LIMIT=100
#
#If you want it to pass all packets by default
#CFLAGS+= -DIPV6FIREWALL_DEFAULT_TO_ACCEPT

.include <bsd.kmod.mk>
