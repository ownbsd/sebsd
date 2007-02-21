/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2000
 *	Poul-Henning Kamp.  All rights reserved.
 * Copyright (c) 2002
 *	Dima Dorfman.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kernfs.h	8.6 (Berkeley) 3/29/95
 * From: FreeBSD: src/sys/miscfs/kernfs/kernfs.h 1.14
 *
 * $FreeBSD: src/sys/fs/devfs/devfs.h,v 1.28 2005/09/24 07:03:09 phk Exp $
 */

#ifndef _FS_DEVFS_DEVFS_H_
#define	_FS_DEVFS_DEVFS_H_

#define	DEVFS_MAGIC	0xdb0a087a

/*
 * Identifiers.  The ruleset and rule numbers are 16-bit values.  The
 * "rule ID" is a combination of the ruleset and rule number; it
 * should be able to univocally describe a rule in the system.  In
 * this implementation, the upper 16 bits of the rule ID is the
 * ruleset number; the lower 16 bits, the rule number within the
 * aforementioned ruleset.
 */
typedef uint16_t devfs_rnum;
typedef uint16_t devfs_rsnum;
typedef uint32_t devfs_rid;

/*
 * Identifier manipulators.
 */
#define	rid2rsn(rid)	((rid) >> 16)
#define	rid2rn(rid)	((rid) & 0xffff)
#define	mkrid(rsn, rn)	((rn) | ((rsn) << 16))

/*
 * Plain DEVFS rule.  This gets shared between kernel and userland
 * verbatim, so it shouldn't contain any pointers or other kernel- or
 * userland-specific values.
 */
struct devfs_rule {
	uint32_t dr_magic;			/* Magic number. */
	devfs_rid dr_id;			/* Identifier. */

	/*
	 * Conditions under which this rule should be applied.  These
	 * are ANDed together since OR can be simulated by using
	 * multiple rules.  dr_icond determines which of the other
	 * variables we should process.
	 */
	int	dr_icond;
#define	DRC_DSWFLAGS	0x001
#define	DRC_PATHPTRN	0x002
	int	dr_dswflags;			/* cdevsw flags to match. */
#define	DEVFS_MAXPTRNLEN	200
	char	dr_pathptrn[DEVFS_MAXPTRNLEN];	/* Pattern to match path. */

	/*
	 * Things to change.  dr_iacts determines which of the other
	 * variables we should process.
	 */
	int	dr_iacts;
#define	DRA_BACTS	0x001
#define	DRA_UID		0x002
#define	DRA_GID		0x004
#define	DRA_MODE	0x008
#define	DRA_INCSET	0x010
	int	dr_bacts;			/* Boolean (on/off) action. */
#define	DRB_HIDE	0x001			/* Hide entry (DE_WHITEOUT). */
#define	DRB_UNHIDE	0x002			/* Unhide entry. */
	uid_t	dr_uid;
	gid_t	dr_gid;
	mode_t	dr_mode;
	devfs_rsnum dr_incset;			/* Included ruleset. */
};

/*
 * Rule-related ioctls.
 */
#define	DEVFSIO_RADD		_IOWR('D', 0, struct devfs_rule)
#define	DEVFSIO_RDEL		_IOW('D', 1, devfs_rid)
#define	DEVFSIO_RAPPLY		_IOW('D', 2, struct devfs_rule)
#define	DEVFSIO_RAPPLYID	_IOW('D', 3, devfs_rid)
#define	DEVFSIO_RGETNEXT       	_IOWR('D', 4, struct devfs_rule)

#define	DEVFSIO_SUSE		_IOW('D', 10, devfs_rsnum)
#define	DEVFSIO_SAPPLY		_IOW('D', 11, devfs_rsnum)
#define	DEVFSIO_SGETNEXT	_IOWR('D', 12, devfs_rsnum)

/* XXX: DEVFSIO_RS_GET_INFO for refcount, active if any, etc. */

#ifdef _KERNEL

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_DEVFS);
#endif

struct devfs_dirent {
	struct cdev_priv	*de_cdp;
	int			de_inode;
	int			de_flags;
#define	DE_WHITEOUT	0x1
#define	DE_DOT		0x2
#define	DE_DOTDOT	0x4
	struct dirent 		*de_dirent;
	TAILQ_ENTRY(devfs_dirent) de_list;
	TAILQ_HEAD(, devfs_dirent) de_dlist;
	struct devfs_dirent	*de_dir;
	int			de_links;
	mode_t			de_mode;
	uid_t			de_uid;
	gid_t			de_gid;
	struct label		*de_label;
	struct timespec 	de_atime;
	struct timespec 	de_mtime;
	struct timespec 	de_ctime;
	struct vnode 		*de_vnode;
	char 			*de_symlink;
};

struct devfs_mount {
	u_int			dm_idx;
	struct mount		*dm_mount;
	struct devfs_dirent	*dm_rootdir;
	unsigned		dm_generation;
	struct sx		dm_lock;
	devfs_rsnum		dm_ruleset;
};

#define DEVFS_ROOTINO 2

extern unsigned devfs_rule_depth;

#define VFSTODEVFS(mp)	((struct devfs_mount *)((mp)->mnt_data))

void devfs_rules_apply(struct devfs_mount *dm, struct devfs_dirent *de);
void devfs_rules_cleanup (struct devfs_mount *dm);
int devfs_rules_ioctl(struct devfs_mount *dm, u_long cmd, caddr_t data, struct thread *td);
int devfs_allocv (struct devfs_dirent *de, struct mount *mp, struct vnode **vpp, struct thread *td);
struct cdev **devfs_itod (int inode);
struct devfs_dirent **devfs_itode (struct devfs_mount *dm, int inode);
void devfs_delete(struct devfs_mount *dm, struct devfs_dirent *de);
void devfs_populate (struct devfs_mount *dm);
void devfs_cleanup (struct devfs_mount *dm);
struct devfs_dirent *devfs_newdirent (char *name, int namelen);
struct devfs_dirent *devfs_vmkdir (struct devfs_mount *, char *name, int namelen, struct devfs_dirent *dotdot, u_int inode);
struct devfs_dirent *devfs_find (struct devfs_dirent *dd, const char *name, int namelen);

#endif /* _KERNEL */

#endif /* !_FS_DEVFS_DEVFS_H_ */