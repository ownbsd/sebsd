/*-
 * Copyright (c) 1980, 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1980, 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount.c	8.25 (Berkeley) 5/8/95";
#endif
static const char rcsid[] =
  "$FreeBSD: src/sbin/mount/mount.c,v 1.83 2006/03/03 02:46:15 keramida Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"
#include "mntopts.h"
#include "pathnames.h"

/* `meta' options */
#define MOUNT_META_OPTION_FSTAB		"fstab"
#define MOUNT_META_OPTION_CURRENT	"current"

int debug, fstab_style, verbose;

char   *catopt(char *, const char *);
struct statfs *getmntpt(const char *);
int	hasopt(const char *, const char *);
int	ismounted(struct fstab *, struct statfs *, int);
int	isremountable(const char *);
void	mangle(char *, int *, char **);
char   *update_options(char *, char *, int);
int	mountfs(const char *, const char *, const char *,
			int, const char *, const char *, const char *);
void	remopt(char *, const char *);
void	prmount(struct statfs *);
void	putfsent(const struct statfs *);
void	usage(void);
char   *flags2opts(int);

/* Map from mount options to printable formats. */
static struct opt {
	int o_opt;
	const char *o_name;
} optnames[] = {
	{ MNT_ASYNC,		"asynchronous" },
	{ MNT_EXPORTED,		"NFS exported" },
	{ MNT_LOCAL,		"local" },
	{ MNT_NOATIME,		"noatime" },
	{ MNT_NOEXEC,		"noexec" },
	{ MNT_NOSUID,		"nosuid" },
	{ MNT_NOSYMFOLLOW,	"nosymfollow" },
	{ MNT_QUOTA,		"with quotas" },
	{ MNT_RDONLY,		"read-only" },
	{ MNT_SYNCHRONOUS,	"synchronous" },
	{ MNT_UNION,		"union" },
	{ MNT_NOCLUSTERR,	"noclusterr" },
	{ MNT_NOCLUSTERW,	"noclusterw" },
	{ MNT_SUIDDIR,		"suiddir" },
	{ MNT_SOFTDEP,		"soft-updates" },
	{ MNT_MULTILABEL,	"multilabel" },
	{ MNT_ACLS,		"acls" },
	{ 0, NULL }
};

/*
 * List of VFS types that can be remounted without becoming mounted on top
 * of each other.
 * XXX Is this list correct?
 */
static const char *
remountable_fs_names[] = {
	"ufs", "ffs", "ext2fs",
	0
};

static int
use_mountprog(const char *vfstype)
{
	/* XXX: We need to get away from implementing external mount
	 *      programs for every filesystem, and move towards having
	 *	each filesystem properly implement the nmount() system call.
	 */
	unsigned int i;
	const char *fs[] = {
	"cd9660", "mfs", "msdosfs", "nfs", "nfs4", "ntfs",
	"nwfs", "nullfs", "portalfs", "smbfs", "udf", "umapfs",
	"unionfs",
	NULL
	};

	for (i=0; fs[i] != NULL; ++i) {
		if (strcmp(vfstype, fs[i]) == 0)
			return 1;
	}
	
	return 0;
}

static int
exec_mountprog(const char *name, const char *execname,
	char *const argv[])
{
	pid_t pid;
	int status;

	switch (pid = fork()) {
	case -1:				/* Error. */
		warn("fork");
		exit (1);
	case 0:					/* Child. */
		/* Go find an executable. */
		execvP(execname, _PATH_SYSPATH, argv);
		if (errno == ENOENT) {
			warn("exec %s not found in %s", execname,
			    _PATH_SYSPATH);
		}
		exit(1);
	default:				/* Parent. */
		if (waitpid(pid, &status, 0) < 0) {
			warn("waitpid");
			return (1);
		}

		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0)
				return (WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			warnx("%s: %s", name, sys_siglist[WTERMSIG(status)]);
			return (1);
		}
		break;
	}

	return (0);
}

int
main(int argc, char *argv[])
{
	const char *mntfromname, **vfslist, *vfstype;
	struct fstab *fs;
	struct statfs *mntbuf;
	FILE *mountdfp;
	pid_t pid;
	int all, ch, i, init_flags, mntsize, rval, have_fstab, ro;
	char *cp, *ep, *options;
	char *ltext = NULL;

	all = init_flags = 0;
	ro = 0;
	options = NULL;
	vfslist = NULL;
	vfstype = "ufs";
	while ((ch = getopt(argc, argv, "adF:fo:prwt:uvl:")) != -1)
		switch (ch) {
		case 'a':
			all = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'F':
			setfstab(optarg);
			break;
		case 'f':
			init_flags |= MNT_FORCE;
			break;
		case 'o':
			if (*optarg)
				options = catopt(options, optarg);
			break;
		case 'p':
			fstab_style = 1;
			verbose = 1;
			break;
		case 'r':
			options = catopt(options, "ro");
			ro = 1;
			break;
		case 't':
			if (vfslist != NULL)
				errx(1, "only one -t option may be specified");
			vfslist = makevfslist(optarg);
			vfstype = optarg;
			break;
		case 'u':
			init_flags |= MNT_UPDATE;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			options = catopt(options, "noro");
			break;
		case 'l':
			ltext = strdup(optarg);
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

#define	BADTYPE(type)							\
	(strcmp(type, FSTAB_RO) &&					\
	    strcmp(type, FSTAB_RW) && strcmp(type, FSTAB_RQ))

	if ((init_flags & MNT_UPDATE) && (ro == 0))
		options = catopt(options, "noro");
 
	rval = 0;
	switch (argc) {
	case 0:
		if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0)
			err(1, "getmntinfo");
		if (all) {
			while ((fs = getfsent()) != NULL) {
				if (BADTYPE(fs->fs_type))
					continue;
				if (checkvfsname(fs->fs_vfstype, vfslist))
					continue;
				if (hasopt(fs->fs_mntops, "noauto"))
					continue;
				if (!(init_flags & MNT_UPDATE) &&
				    ismounted(fs, mntbuf, mntsize))
					continue;
				options = update_options(options, fs->fs_mntops,
				    mntbuf->f_flags);
				if (mountfs(fs->fs_vfstype, fs->fs_spec,
				    fs->fs_file, init_flags, options,
				    fs->fs_mntops, NULL))
					rval = 1;
			}
		} else if (fstab_style) {
			for (i = 0; i < mntsize; i++) {
				if (checkvfsname(mntbuf[i].f_fstypename, vfslist))
					continue;
				putfsent(&mntbuf[i]);
			}
		} else {
			for (i = 0; i < mntsize; i++) {
				if (checkvfsname(mntbuf[i].f_fstypename,
				    vfslist))
					continue;
				prmount(&mntbuf[i]);
			}
		}
		exit(rval);
	case 1:
		if (vfslist != NULL)
			usage();

		rmslashes(*argv, *argv);
		if (init_flags & MNT_UPDATE) {
			mntfromname = NULL;
			have_fstab = 0;
			if ((mntbuf = getmntpt(*argv)) == NULL)
				errx(1, "not currently mounted %s", *argv);
			/*
			 * Only get the mntflags from fstab if both mntpoint
			 * and mntspec are identical. Also handle the special
			 * case where just '/' is mounted and 'spec' is not
			 * identical with the one from fstab ('/dev' is missing
			 * in the spec-string at boot-time).
			 */
			if ((fs = getfsfile(mntbuf->f_mntonname)) != NULL) {
				if (strcmp(fs->fs_spec,
				    mntbuf->f_mntfromname) == 0 &&
				    strcmp(fs->fs_file,
				    mntbuf->f_mntonname) == 0) {
					have_fstab = 1;
					mntfromname = mntbuf->f_mntfromname;
				} else if (argv[0][0] == '/' &&
				    argv[0][1] == '\0') {
					fs = getfsfile("/");
					have_fstab = 1;
					mntfromname = fs->fs_spec;
				}
			}
			if (have_fstab) {
				options = update_options(options, fs->fs_mntops,
				    mntbuf->f_flags);
			} else {
				mntfromname = mntbuf->f_mntfromname;
				options = update_options(options, NULL,
				    mntbuf->f_flags);
			}
			rval = mountfs(mntbuf->f_fstypename, mntfromname,
			    mntbuf->f_mntonname, init_flags, options, 0, NULL);
			break;
		}
		if ((fs = getfsfile(*argv)) == NULL &&
		    (fs = getfsspec(*argv)) == NULL)
			errx(1, "%s: unknown special file or file system",
			    *argv);
		if (BADTYPE(fs->fs_type))
			errx(1, "%s has unknown file system type",
			    *argv);
		rval = mountfs(fs->fs_vfstype, fs->fs_spec, fs->fs_file,
		    init_flags, options, fs->fs_mntops, ltext);
		break;
	case 2:
		/*
		 * If -t flag has not been specified, the path cannot be
		 * found, spec contains either a ':' or a '@', then assume
		 * that an NFS file system is being specified ala Sun.
		 * Check if the hostname contains only allowed characters
		 * to reduce false positives.  IPv6 addresses containing
		 * ':' will be correctly parsed only if the separator is '@'.
		 * The definition of a valid hostname is taken from RFC 1034.
		 */
		if (vfslist == NULL && ((ep = strchr(argv[0], '@')) != NULL ||
		    (ep = strchr(argv[0], ':')) != NULL)) {
			if (*ep == '@') {
				cp = ep + 1;
				ep = cp + strlen(cp);
			} else
				cp = argv[0];
			while (cp != ep) {
				if (!isdigit(*cp) && !isalpha(*cp) &&
				    *cp != '.' && *cp != '-' && *cp != ':')
					break;
				cp++;
			}
			if (cp == ep)
				vfstype = "nfs";
		}
		rval = mountfs(vfstype,
		    argv[0], argv[1], init_flags, options, NULL, ltext);
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	/*
	 * If the mount was successfully, and done by root, tell mountd the
	 * good news.  Pid checks are probably unnecessary, but don't hurt.
	 */
	if (rval == 0 && getuid() == 0 &&
	    (mountdfp = fopen(_PATH_MOUNTDPID, "r")) != NULL) {
		if (fscanf(mountdfp, "%d", &pid) == 1 &&
		     pid > 0 && kill(pid, SIGHUP) == -1 && errno != ESRCH)
			err(1, "signal mountd");
		(void)fclose(mountdfp);
	}

	exit(rval);
}

int
ismounted(struct fstab *fs, struct statfs *mntbuf, int mntsize)
{
	char realfsfile[PATH_MAX];
	int i;

	if (fs->fs_file[0] == '/' && fs->fs_file[1] == '\0')
		/* the root file system can always be remounted */
		return (0);

	/* The user may have specified a symlink in fstab, resolve the path */
	if (realpath(fs->fs_file, realfsfile) == NULL) {
		/* Cannot resolve the path, use original one */
		strlcpy(realfsfile, fs->fs_file, sizeof(realfsfile));
	}

	for (i = mntsize - 1; i >= 0; --i)
		if (strcmp(realfsfile, mntbuf[i].f_mntonname) == 0 &&
		    (!isremountable(fs->fs_vfstype) ||
		     strcmp(fs->fs_spec, mntbuf[i].f_mntfromname) == 0))
			return (1);
	return (0);
}

int
isremountable(const char *vfsname)
{
	const char **cp;

	for (cp = remountable_fs_names; *cp; cp++)
		if (strcmp(*cp, vfsname) == 0)
			return (1);
	return (0);
}

int
hasopt(const char *mntopts, const char *option)
{
	int negative, found;
	char *opt, *optbuf;

	if (option[0] == 'n' && option[1] == 'o') {
		negative = 1;
		option += 2;
	} else
		negative = 0;
	optbuf = strdup(mntopts);
	found = 0;
	for (opt = optbuf; (opt = strtok(opt, ",")) != NULL; opt = NULL) {
		if (opt[0] == 'n' && opt[1] == 'o') {
			if (!strcasecmp(opt + 2, option))
				found = negative;
		} else if (!strcasecmp(opt, option))
			found = !negative;
	}
	free(optbuf);
	return (found);
}

int
mountfs(const char *vfstype, const char *spec, const char *name, int flags,
	const char *options, const char *mntopts, __unused const char *ltext)
{
	char *argv[100];
	struct statfs sf;
	int argc, i, ret;
	char *optbuf, execname[PATH_MAX], mntpath[PATH_MAX];

	/* resolve the mountpoint with realpath(3) */
	(void)checkpath(name, mntpath);
	name = mntpath;

	if (mntopts == NULL)
		mntopts = "";
	optbuf = catopt(strdup(mntopts), options);

	if (strcmp(name, "/") == 0)
		flags |= MNT_UPDATE;
	if (flags & MNT_FORCE)
		optbuf = catopt(optbuf, "force");
	if (flags & MNT_RDONLY)
		optbuf = catopt(optbuf, "ro");
	/*
	 * XXX
	 * The mount_mfs (newfs) command uses -o to select the
	 * optimization mode.  We don't pass the default "-o rw"
	 * for that reason.
	 */
	if (flags & MNT_UPDATE)
		optbuf = catopt(optbuf, "update");

	/* Compatibility glue. */
	if (strcmp(vfstype, "msdos") == 0)
		vfstype = "msdosfs";

	/* Construct the name of the appropriate mount command */
	(void)snprintf(execname, sizeof(execname), "mount_%s", vfstype);

	argc = 0;
	argv[argc++] = execname;
	mangle(optbuf, &argc, argv);
	argv[argc++] = strdup(spec);
	argv[argc++] = strdup(name);
#if 0
	/*
	 * XXXRW: Temporarily disabled due to nmount conversion.
	 */
	if (ltext != NULL) {
		argv[argc++] = "-l";
		argv[argc++] = ltext;
	}
	argv[argc] = NULL;
#endif

	if (debug) {
		(void)printf("exec: mount_%s", vfstype);
		for (i = 1; i < argc; i++)
			(void)printf(" %s", argv[i]);
		(void)printf("\n");
		return (0);
	}

	if (use_mountprog(vfstype)) {
		ret = exec_mountprog(name, execname, argv);
	} else {
		ret = mount_fs(vfstype, argc, argv); 
	}

	free(optbuf);

	if (verbose) {
		if (statfs(name, &sf) < 0) {
			warn("statfs %s", name);
			return (1);
		}
		if (fstab_style)
			putfsent(&sf);
		else
			prmount(&sf);
	}

	return (0);
}

void
prmount(struct statfs *sfp)
{
	int flags;
	unsigned int i;
	struct opt *o;
	struct passwd *pw;

	(void)printf("%s on %s (%s", sfp->f_mntfromname, sfp->f_mntonname,
	    sfp->f_fstypename);

	flags = sfp->f_flags & MNT_VISFLAGMASK;
	for (o = optnames; flags && o->o_opt; o++)
		if (flags & o->o_opt) {
			(void)printf(", %s", o->o_name);
			flags &= ~o->o_opt;
		}
	/*
	 * Inform when file system is mounted by an unprivileged user
	 * or privileged non-root user.
	 */
	if ((flags & MNT_USER) != 0 || sfp->f_owner != 0) {
		(void)printf(", mounted by ");
		if ((pw = getpwuid(sfp->f_owner)) != NULL)
			(void)printf("%s", pw->pw_name);
		else
			(void)printf("%d", sfp->f_owner);
	}
	if (verbose) {
		if (sfp->f_syncwrites != 0 || sfp->f_asyncwrites != 0)
			(void)printf(", writes: sync %ju async %ju",
			    (uintmax_t)sfp->f_syncwrites,
			    (uintmax_t)sfp->f_asyncwrites);
		if (sfp->f_syncreads != 0 || sfp->f_asyncreads != 0)
			(void)printf(", reads: sync %ju async %ju",
			    (uintmax_t)sfp->f_syncreads,
			    (uintmax_t)sfp->f_asyncreads);
		if (sfp->f_fsid.val[0] != 0 || sfp->f_fsid.val[1] != 0) {
			printf(", fsid ");
			for (i = 0; i < sizeof(sfp->f_fsid); i++)
				printf("%02x", ((u_char *)&sfp->f_fsid)[i]);
		}
	}
	(void)printf(")\n");
}

struct statfs *
getmntpt(const char *name)
{
	struct statfs *mntbuf;
	int i, mntsize;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = mntsize - 1; i >= 0; i--) {
		if (strcmp(mntbuf[i].f_mntfromname, name) == 0 ||
		    strcmp(mntbuf[i].f_mntonname, name) == 0)
			return (&mntbuf[i]);
	}
	return (NULL);
}

char *
catopt(char *s0, const char *s1)
{
	size_t i;
	char *cp;

	if (s1 == NULL || *s1 == '\0')
		return s0;

	if (s0 && *s0) {
		i = strlen(s0) + strlen(s1) + 1 + 1;
		if ((cp = malloc(i)) == NULL)
			errx(1, "malloc failed");
		(void)snprintf(cp, i, "%s,%s", s0, s1);
	} else
		cp = strdup(s1);

	if (s0)
		free(s0);
	return (cp);
}

void
mangle(options, argcp, argv)
	char *options;
	int *argcp;
	char **argv;
{
	char *p, *s;
	int argc;

	argc = *argcp;
	for (s = options; (p = strsep(&s, ",")) != NULL;)
		if (*p != '\0') {
			if (strcmp(p, "noauto") == 0) {
				/*
				 * Do not pass noauto option to nmount().
				 * or external mount program.  noauto is
				 * only used to prevent mounting a filesystem
				 * when 'mount -a' is specified, and is
				 * not a real mount option.
				 */
				continue;
			} else if (strcmp(p, "userquota") == 0) {
				continue;
			} else if (strcmp(p, "groupquota") == 0) {
				continue;
			} else if (*p == '-') {
				argv[argc++] = p;
				p = strchr(p, '=');
				if (p != NULL) {
					*p = '\0';
					argv[argc++] = p+1;
				}
			} else {
				argv[argc++] = strdup("-o");
				argv[argc++] = p;
			}
		}

	*argcp = argc;
}


char *
update_options(opts, fstab, curflags)
	char *opts;
	char *fstab;
	int curflags;
{
	char *o, *p;
	char *cur;
	char *expopt, *newopt, *tmpopt;

	if (opts == NULL)
		return strdup("");

	/* remove meta options from list */
	remopt(fstab, MOUNT_META_OPTION_FSTAB);
	remopt(fstab, MOUNT_META_OPTION_CURRENT);
	cur = flags2opts(curflags);

	/*
	 * Expand all meta-options passed to us first.
	 */
	expopt = NULL;
	for (p = opts; (o = strsep(&p, ",")) != NULL;) {
		if (strcmp(MOUNT_META_OPTION_FSTAB, o) == 0)
			expopt = catopt(expopt, fstab);
		else if (strcmp(MOUNT_META_OPTION_CURRENT, o) == 0)
			expopt = catopt(expopt, cur);
		else
			expopt = catopt(expopt, o);
	}
	free(cur);
	free(opts);

	/*
	 * Remove previous contradictory arguments. Given option "foo" we
	 * remove all the "nofoo" options. Given "nofoo" we remove "nonofoo"
	 * and "foo" - so we can deal with possible options like "notice".
	 */
	newopt = NULL;
	for (p = expopt; (o = strsep(&p, ",")) != NULL;) {
		if ((tmpopt = malloc( strlen(o) + 2 + 1 )) == NULL)
			errx(1, "malloc failed");

		strcpy(tmpopt, "no");
		strcat(tmpopt, o);
		remopt(newopt, tmpopt);
		free(tmpopt);

		if (strncmp("no", o, 2) == 0)
			remopt(newopt, o+2);

		newopt = catopt(newopt, o);
	}
	free(expopt);

	return newopt;
}

void
remopt(string, opt)
	char *string;
	const char *opt;
{
	char *o, *p, *r;

	if (string == NULL || *string == '\0' || opt == NULL || *opt == '\0')
		return;

	r = string;

	for (p = string; (o = strsep(&p, ",")) != NULL;) {
		if (strcmp(opt, o) != 0) {
			if (*r == ',' && *o != '\0')
				r++;
			while ((*r++ = *o++) != '\0')
			    ;
			*--r = ',';
		}
	}
	*r = '\0';
}

void
usage()
{

	(void)fprintf(stderr, "%s\n%s\n%s\n",
"usage: mount [-adfpruvw] [-F fstab] [-o options] [-t ufs | external_type]",
"       mount [-dfpruvw] special | node",
"       mount [-dfpruvw] [-o options] [-t ufs | external_type] special node");
	exit(1);
}

void
putfsent(ent)
	const struct statfs *ent;
{
	struct fstab *fst;
	char *opts;

	opts = flags2opts(ent->f_flags);

	/*
	 * "rw" is not a real mount option; this is why we print NULL as "rw"
	 * if opts is still NULL here.
	 */
	printf("%s\t%s\t%s %s", ent->f_mntfromname, ent->f_mntonname,
	    ent->f_fstypename, opts ? opts : "rw");
	free(opts);

	if ((fst = getfsspec(ent->f_mntfromname)))
		printf("\t%u %u\n", fst->fs_freq, fst->fs_passno);
	else if ((fst = getfsfile(ent->f_mntonname)))
		printf("\t%u %u\n", fst->fs_freq, fst->fs_passno);
	else if (strcmp(ent->f_fstypename, "ufs") == 0) {
		if (strcmp(ent->f_mntonname, "/") == 0)
			printf("\t1 1\n");
		else
			printf("\t2 2\n");
	} else
		printf("\t0 0\n");
}


char *
flags2opts(flags)
	int flags;
{
	char *res;

	res = NULL;

	if (flags & MNT_RDONLY)		res = catopt(res, "ro");
	if (flags & MNT_SYNCHRONOUS)	res = catopt(res, "sync");
	if (flags & MNT_NOEXEC)		res = catopt(res, "noexec");
	if (flags & MNT_NOSUID)		res = catopt(res, "nosuid");
	if (flags & MNT_UNION)		res = catopt(res, "union");
	if (flags & MNT_ASYNC)		res = catopt(res, "async");
	if (flags & MNT_NOATIME)	res = catopt(res, "noatime");
	if (flags & MNT_NOCLUSTERR)	res = catopt(res, "noclusterr");
	if (flags & MNT_NOCLUSTERW)	res = catopt(res, "noclusterw");
	if (flags & MNT_NOSYMFOLLOW)	res = catopt(res, "nosymfollow");
	if (flags & MNT_SUIDDIR)	res = catopt(res, "suiddir");
	if (flags & MNT_MULTILABEL)	res = catopt(res, "multilabel");
	if (flags & MNT_ACLS)		res = catopt(res, "acls");

	return res;
}
