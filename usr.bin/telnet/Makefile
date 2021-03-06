# $FreeBSD: src/usr.bin/telnet/Makefile,v 1.29 2006/03/17 18:54:39 ru Exp $

.include <bsd.own.mk>

TELNETDIR=	${.CURDIR}/../../contrib/telnet
.PATH:		${TELNETDIR}/telnet

PROG=		telnet

SRCS=		commands.c main.c network.c ring.c sys_bsd.c \
		telnet.c terminal.c utilities.c

CFLAGS+=	-DKLUDGELINEMODE -DUSE_TERMIO -DENV_HACK -DOPIE \
		-I${TELNETDIR} -I${TELNETDIR}/libtelnet/

LIBTELNET=	${.OBJDIR}/../../lib/libtelnet/libtelnet.a

DPADD=		${LIBTERMCAP} ${LIBTELNET}
LDADD=		-ltermcap ${LIBTELNET}

.if !defined(RELEASE_CRUNCH)
CFLAGS+=	-DINET6 -DIPSEC
DPADD+=		${LIBIPSEC}
LDADD+=		-lipsec
.else
.PATH:         ${TELNETDIR}/libtelnet
SRCS+=		genget.c getent.c misc.c
CFLAGS+=	-DHAS_CGETENT
.endif

.if !defined(RELEASE_CRUNCH)
.if ${MK_OPENSSL} != "no"
SRCS+=		authenc.c 
CFLAGS+=	-DENCRYPTION -DAUTHENTICATION -DIPSEC
DPADD+=		${LIBMP} ${LIBCRYPTO} ${LIBCRYPT} ${LIBIPSEC} ${LIBPAM}
LDADD+=		-lmp -lcrypto -lcrypt -lipsec ${MINUSLPAM}
.if ${MK_KERBEROS_SUPPORT} != "no"
CFLAGS+=	-DKRB5 -DFORWARD -Dnet_write=telnet_net_write
DPADD+=		${LIBKRB5} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN}
LDADD+=		-lkrb5 -lasn1 -lcom_err -lroken
.endif
.endif
.endif

.include <bsd.prog.mk>
