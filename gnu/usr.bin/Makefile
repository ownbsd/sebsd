# $FreeBSD: src/gnu/usr.bin/Makefile,v 1.88 2006/03/17 18:54:22 ru Exp $

.include <bsd.own.mk>

SUBDIR= bc \
	${_binutils} \
	${_cc} \
	cpio \
	${_cvs} \
	dc \
	dialog \
	diff \
	diff3 \
	${_gdb} \
	${_gperf} \
	grep \
	${_groff} \
	gzip \
	${_man} \
	patch \
	${_rcs} \
	sdiff \
	send-pr \
	sort \
	${_texinfo}
 
.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "powerpc"
MK_GDB=	no	# not yet
.endif

.if ${MK_CXX} != "no"
.if ${MACHINE_ARCH} != "powerpc"
_gperf=		gperf
.endif
.if ${MK_GROFF} != "no"
_groff=		groff
.endif
.endif

.if ${MK_CVS} != "no"
_cvs=		cvs
.endif

.if ${MK_MAN} != "no"
_man=		man
.endif

.if ${MK_INFO} != "no"
_texinfo=	texinfo
.endif

.if ${MK_TOOLCHAIN} != "no"
_binutils=	binutils
_cc=		cc
.if ${MK_GDB} != "no"
_gdb=		gdb
.endif
.endif
.if ${MK_RCS} != "no"
_rcs=		rcs
.endif

.include <bsd.subdir.mk>
