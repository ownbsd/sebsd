/* $FreeBSD: src/usr.sbin/sysinstall/dist.h,v 1.66 2006/03/08 18:02:32 sam Exp $  */

#ifndef _DIST_H_INCLUDE
#define _DIST_H_INCLUDE

/* Bitfields for distributions - hope we never have more than 32! :-) */
#define DIST_BASE		0x00001
#define DIST_GAMES		0x00002
#define DIST_MANPAGES		0x00004
#define DIST_PROFLIBS		0x00008
#define DIST_DICT		0x00010
#define DIST_SRC		0x00020
#define DIST_DOC		0x00040
#define DIST_INFO		0x00080
#define DIST_XORG		0x00100
#define DIST_CATPAGES		0x00200
#define DIST_PORTS		0x00400
#define DIST_LOCAL		0x00800
#ifdef __amd64__
#define DIST_LIB32		0x01000
#endif
#define	DIST_KERNEL		0x02000
#define DIST_ALL		0xFFFFF

/* Subtypes for SRC distribution */
#define DIST_SRC_BASE		0x00001
#define DIST_SRC_CONTRIB	0x00002
#define DIST_SRC_GNU		0x00004
#define DIST_SRC_ETC		0x00008
#define DIST_SRC_GAMES		0x00010
#define DIST_SRC_INCLUDE	0x00020
#define DIST_SRC_LIB		0x00040
#define DIST_SRC_LIBEXEC	0x00080
#define DIST_SRC_TOOLS		0x00100
#define DIST_SRC_RELEASE	0x00200
#define DIST_SRC_SBIN		0x00400
#define DIST_SRC_SHARE		0x00800
#define DIST_SRC_SYS		0x01000
#define DIST_SRC_UBIN		0x02000
#define DIST_SRC_USBIN		0x04000
#define DIST_SRC_BIN		0x08000
#define DIST_SRC_SCRYPTO	0x10000
#define DIST_SRC_SSECURE	0x20000
#define DIST_SRC_SKERBEROS5	0x40000
#define DIST_SRC_RESCUE		0x80000
#define DIST_SRC_ALL		0xFFFFF

/* Subtypes for X.Org packages */
#define	DIST_XORG_CLIENTS	0x000001
#define	DIST_XORG_LIB		0x000002
#define DIST_XORG_MAN		0x000004
#define DIST_XORG_DOC		0x000008
#define DIST_XORG_IMAKE		0x000010

#define	DIST_XORG_SERVER	0x000100
#define	DIST_XORG_NESTSERVER	0x000200
#define	DIST_XORG_PRINTSERVER	0x000400
#define	DIST_XORG_VFBSERVER	0x000800

#define	DIST_XORG_FONTS_MISC	0x010000
#define DIST_XORG_FONTS_75	0x020000
#define DIST_XORG_FONTS_100	0x040000
#define DIST_XORG_FONTS_CYR	0x080000
#define DIST_XORG_FONTS_T1	0x100000
#define DIST_XORG_FONTS_TT	0x200000
#define DIST_XORG_FONTSERVER	0x400000

#define	DIST_XORG_MISC_ALL	0x00001f
#define	DIST_XORG_SERVER_ALL	0x000f00
#define DIST_XORG_FONTS_ALL	0x7f0000
#define DIST_XORG_ALL		\
	(DIST_XORG_MISC_ALL | DIST_XORG_SERVER_ALL | DIST_XORG_FONTS_ALL)

/* Subtypes for KERNEL distribution */
#define DIST_KERNEL_GENERIC	0x00001
#define DIST_KERNEL_SMP		0x00002
#define DIST_KERNEL_ALL		0xFFFFF

/* Canned distribution sets */

#define _DIST_XORG_FONTS_BASE \
	(DIST_XORG_FONTS_MISC | DIST_XORG_FONTS_75 | DIST_XORG_FONTS_100 | \
	 DIST_XORG_FONTS_TT)

#define _DIST_USER \
	( DIST_BASE | DIST_KERNEL | DIST_DOC | DIST_MANPAGES | DIST_DICT )

#define _DIST_DEVELOPER \
	( _DIST_USER | DIST_PROFLIBS | DIST_INFO | DIST_SRC )

#endif	/* _DIST_H_INCLUDE */
