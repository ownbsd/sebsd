# $FreeBSD: src/secure/usr.sbin/sshd/Makefile,v 1.47 2006/03/17 18:54:32 ru Exp $
#

.include <bsd.own.mk>

PROG=	sshd
SRCS=	sshd.c auth-rhosts.c auth-passwd.c auth-rsa.c auth-rh-rsa.c \
	sshpty.c sshlogin.c servconf.c serverloop.c \
	auth.c auth1.c auth2.c auth-options.c session.c \
	auth-chall.c auth2-chall.c groupaccess.c \
	auth-skey.c auth-bsdauth.c auth2-hostbased.c auth2-kbdint.c \
	auth2-none.c auth2-passwd.c auth2-pubkey.c \
	monitor_mm.c monitor.c monitor_wrap.c kexdhs.c kexgexs.c \
	auth-krb5.c \
	auth2-gss.c gss-serv.c gss-serv-krb5.c \
	loginrec.c auth-pam.c auth-shadow.c auth-sia.c md5crypt.c \
	audit.c audit-bsm.c \
	gss-genr.c

# gss-genr.c really belongs in libssh; see src/secure/lib/libssh/Makefile

MAN=	sshd.8 sshd_config.5
CFLAGS+=-I${SSHDIR}

DPADD=	${LIBSSH} ${LIBUTIL} ${LIBZ} ${LIBWRAP} ${LIBPAM}
LDADD=	-lssh -lutil -lz -lwrap ${MINUSLPAM}

.if ${MK_AUDIT} != "no"
CFLAGS+= -DUSE_BSM_AUDIT
DPADD+=  ${LIBBSM}
LDADD+=  -lbsm
.endif

.if ${MK_KERBEROS_SUPPORT} != "no"
CFLAGS+= -DGSSAPI -DHAVE_GSSAPI_GSSAPI_H=1 -DKRB5 -DHEIMDAL
DPADD+=	 ${LIBGSSAPI} ${LIBKRB5} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN}
LDADD+=	 -lgssapi -lkrb5 -lasn1 -lcom_err -lroken
.endif

.if defined(X11BASE)
CFLAGS+= -DXAUTH_PATH=\"${X11BASE}/bin/xauth\"
.endif

DPADD+=	${LIBCRYPTO} ${LIBCRYPT}
LDADD+=	-lcrypto -lcrypt

.include <bsd.prog.mk>

.PATH:	${SSHDIR}
