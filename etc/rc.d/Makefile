# $NetBSD: Makefile,v 1.16 2001/01/14 15:37:22 minoura Exp $
# $FreeBSD: src/etc/rc.d/Makefile,v 1.68 2006/03/17 18:54:21 ru Exp $

.include <bsd.own.mk>

FILES=	DAEMON LOGIN NETWORKING SERVERS \
	abi accounting addswap adjkerntz amd \
	apm apmd archdep atm1 atm2 atm3 auditd \
	bgfsck bluetooth bootparams bsnmpd \
	ccd cleanvar cleartmp cron \
	devd devfs dhclient \
	dmesg dumpon \
	early.sh encswap \
	fsck ftpd \
	gbde geli geli2 \
	hcsecd \
	hostapd hostname \
	ike \
	inetd initrandom \
	ip6addrctl ip6fw ipfilter ipfs ipfw ipmon \
	ipnat ipsec ipxrouted isdnd \
	jail \
	kadmind kerberos kernel keyserv kldxref kpasswdd \
	ldconfig local localpkg lpd \
	mixer motd mountcritlocal mountcritremote \
	mountd moused mroute6d mrouted msgs \
	named natd netif netoptions \
	network_ipv6 newsyslog nfsclient nfsd \
	nfslocking nfsserver nisdomain nsswitch ntpd ntpdate \
	othermta \
	pcvt pf pflog pfsync \
	powerd power_profile ppp pppoed pwcheck \
	quota \
	ramdisk ramdisk-own random rarpd resolv root \
	route6d routed routing rpcbind rtadvd rwho \
	savecore sdpd securelevel sendmail \
	serial sppp swap1 \
	syscons sysctl syslogd \
	timed tmp \
	ugidfw \
	var virecover \
	watchdogd wpa_supplicant \
	ypbind yppasswdd ypserv \
	ypset ypupdated ypxfrd

.if ${MK_OPENSSH} != "no"
FILES+=	sshd
.endif

FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
