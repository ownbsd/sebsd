# $FreeBSD: src/sys/modules/msdosfs/Makefile,v 1.22 2004/09/04 06:37:57 ru Exp $

.PATH: ${.CURDIR}/../../fs/msdosfs

KMOD=	msdosfs
SRCS=	opt_msdosfs.h vnode_if.h \
	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_vfsops.c msdosfs_vnops.c
EXPORT_SYMS=	msdosfs_iconv

.include <bsd.kmod.mk>
