# $FreeBSD: src/sys/boot/ia64/ski/Makefile,v 1.19 2006/03/17 18:54:35 ru Exp $

NO_MAN=

.include <bsd.own.mk>

PROG=		skiload
NEWVERSWHAT=	"ia64 SKI boot" ${MACHINE_ARCH}
STRIP=		# We must not strip skiload at install time.

SRCS=		acpi_stub.c bootinfo.c conf.c copy.c delay.c devicename.c \
		efi_stub.c elf_freebsd.c exit.c main.c pal_stub.S sal_stub.c \
		skiconsole.c skifs.c ssc.c start.S time.c vers.c

CFLAGS+=	-DLOADER
CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-I${.CURDIR}/../../..
CFLAGS+=	-I${.CURDIR}/../../efi/include
CFLAGS+=	-I${.CURDIR}/../../efi/include/${MACHINE_ARCH}
LDFLAGS=	-Wl,-T${.CURDIR}/ldscript.ia64

.if ${MK_FORTH} != "no"
CFLAGS+=	-DBOOT_FORTH
CFLAGS+=	-I${.CURDIR}/../../ficl
CFLAGS+=	-I${.CURDIR}/../../ficl/${MACHINE_ARCH}
LIBFICL=	${.OBJDIR}/../../ficl/libficl.a
BOOT_FORTH=	yes
.endif

# Always add MI sources (needs BOOT_FORTH)
.PATH: ${.CURDIR}/../../common
CFLAGS+=	-I${.CURDIR}/../../common
.include "${.CURDIR}/../../common/Makefile.inc"

DPADD=	${LIBFICL} ${LIBSTAND}
LDADD=	${LIBFICL} -lstand

CLEANFILES=	vers.c ${PROG}.help

vers.c: ${.CURDIR}/../../common/newvers.sh ${.CURDIR}/version
	sh ${.CURDIR}/../../common/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}

${PROG}.help: help.common
	cat ${.ALLSRC} | awk -f ${.CURDIR}/../../common/merge_help.awk \
	    > ${.TARGET}

FILES=	${PROG}.help

.include <bsd.prog.mk>
