/*-
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2000
 *	Poul-Henning Kamp.  All rights reserved.
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
 *	@(#)kernfs_vfsops.c	8.10 (Berkeley) 5/14/95
 * From: FreeBSD: src/sys/miscfs/kernfs/kernfs_vfsops.c 1.36
 *
 * $FreeBSD: src/sys/fs/devfs/devfs_vfsops.c,v 1.48 2005/09/24 07:03:09 phk Exp $
 */

#include "opt_devfs.h"
#include "opt_mac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/mac.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/sx.h>
#include <sys/vnode.h>
#include <sys/limits.h>

#include <fs/devfs/devfs.h>

static struct unrhdr	*devfs_unr;

MALLOC_DEFINE(M_DEVFS, "DEVFS", "DEVFS data");

static vfs_mount_t	devfs_mount;
static vfs_unmount_t	devfs_unmount;
static vfs_root_t	devfs_root;
static vfs_statfs_t	devfs_statfs;

/*
 * Mount the filesystem
 */
static int
devfs_mount(struct mount *mp, struct thread *td)
{
	int error;
	struct devfs_mount *fmp;
	struct vnode *rvp;

	if (devfs_unr == NULL)
		devfs_unr = new_unrhdr(0, INT_MAX, NULL);

	error = 0;

	if (mp->mnt_flag & (MNT_UPDATE | MNT_ROOTFS))
		return (EOPNOTSUPP);

	fmp = malloc(sizeof *fmp, M_DEVFS, M_WAITOK | M_ZERO);
	fmp->dm_idx = alloc_unr(devfs_unr);
	sx_init(&fmp->dm_lock, "devfsmount");

	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_kern_flag |= MNTK_MPSAFE;
#ifdef MAC
	mp->mnt_flag |= MNT_MULTILABEL;
#endif
	fmp->dm_mount = mp;
	mp->mnt_data = (void *) fmp;
	vfs_getnewfsid(mp);

	fmp->dm_rootdir = devfs_vmkdir(fmp, NULL, 0, NULL, DEVFS_ROOTINO);

	error = devfs_root(mp, LK_EXCLUSIVE, &rvp, td);
	if (error) {
		sx_destroy(&fmp->dm_lock);
		free_unr(devfs_unr, fmp->dm_idx);
		free(fmp, M_DEVFS);
		return (error);
	}

	VOP_UNLOCK(rvp, 0, td);

	vfs_mountedfrom(mp, "devfs");

	return (0);
}

static int
devfs_unmount(struct mount *mp, int mntflags, struct thread *td)
{
	int error;
	int flags = 0;
	struct devfs_mount *fmp;

	fmp = VFSTODEVFS(mp);
	/* There is 1 extra root vnode reference from devfs_mount(). */
	error = vflush(mp, 1, flags, td);
	if (error)
		return (error);
	sx_xlock(&fmp->dm_lock);
	devfs_cleanup(fmp);
	devfs_rules_cleanup(fmp);
	sx_xunlock(&fmp->dm_lock);
	mp->mnt_data = NULL;
	sx_destroy(&fmp->dm_lock);
	free_unr(devfs_unr, fmp->dm_idx);
	free(fmp, M_DEVFS);
	return 0;
}

/* Return locked reference to root.  */

static int
devfs_root(struct mount *mp, int flags, struct vnode **vpp, struct thread *td)
{
	int error;
	struct vnode *vp;
	struct devfs_mount *dmp;

	dmp = VFSTODEVFS(mp);
	error = devfs_allocv(dmp->dm_rootdir, mp, &vp, td);
	if (error)
		return (error);
	vp->v_vflag |= VV_ROOT;
	*vpp = vp;
	return (0);
}

static int
devfs_statfs(struct mount *mp, struct statfs *sbp, struct thread *td)
{

	sbp->f_flags = 0;
	sbp->f_bsize = DEV_BSIZE;
	sbp->f_iosize = DEV_BSIZE;
	sbp->f_blocks = 2;		/* 1K to keep df happy */
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_files = 0;
	sbp->f_ffree = 0;
	return (0);
}

static struct vfsops devfs_vfsops = {
	.vfs_mount =		devfs_mount,
	.vfs_root =		devfs_root,
	.vfs_statfs =		devfs_statfs,
	.vfs_unmount =		devfs_unmount,
};

VFS_SET(devfs_vfsops, devfs, VFCF_SYNTHETIC);