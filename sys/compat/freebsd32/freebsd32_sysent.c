/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * $FreeBSD: src/sys/compat/freebsd32/freebsd32_sysent.c,v 1.52 2006/02/28 19:39:52 ps Exp $
 * created from FreeBSD
 */

#include "opt_compat.h"

#include <bsm/audit_kevents.h>
#include <sys/param.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/mount.h>
#include <compat/freebsd32/freebsd32.h>
#include <compat/freebsd32/freebsd32_proto.h>

#define AS(name) (sizeof(struct name) / sizeof(register_t))

#ifdef COMPAT_43
#define compat(n, name) n, (sy_call_t *)__CONCAT(o,name)
#else
#define compat(n, name) 0, (sy_call_t *)nosys
#endif

#ifdef COMPAT_FREEBSD4
#define compat4(n, name) n, (sy_call_t *)__CONCAT(freebsd4_,name)
#else
#define compat4(n, name) 0, (sy_call_t *)nosys
#endif

/* The casts are bogus but will do for now. */
struct sysent freebsd32_sysent[] = {
	{ SYF_MPSAFE | 0, (sy_call_t *)nosys, AUE_NULL },	/* 0 = syscall */
	{ SYF_MPSAFE | AS(sys_exit_args), (sy_call_t *)sys_exit, AUE_EXIT },	/* 1 = exit */
	{ SYF_MPSAFE | 0, (sy_call_t *)fork, AUE_FORK },	/* 2 = fork */
	{ SYF_MPSAFE | AS(read_args), (sy_call_t *)read, AUE_READ },	/* 3 = read */
	{ SYF_MPSAFE | AS(write_args), (sy_call_t *)write, AUE_WRITE },	/* 4 = write */
	{ SYF_MPSAFE | AS(open_args), (sy_call_t *)open, AUE_OPEN_RWTC },	/* 5 = open */
	{ SYF_MPSAFE | AS(close_args), (sy_call_t *)close, AUE_CLOSE },	/* 6 = close */
	{ SYF_MPSAFE | AS(freebsd32_wait4_args), (sy_call_t *)freebsd32_wait4, AUE_WAIT4 },	/* 7 = freebsd32_wait4 */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 8 = obsolete old creat */
	{ SYF_MPSAFE | AS(link_args), (sy_call_t *)link, AUE_LINK },	/* 9 = link */
	{ SYF_MPSAFE | AS(unlink_args), (sy_call_t *)unlink, AUE_UNLINK },	/* 10 = unlink */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 11 = obsolete execv */
	{ SYF_MPSAFE | AS(chdir_args), (sy_call_t *)chdir, AUE_CHDIR },	/* 12 = chdir */
	{ SYF_MPSAFE | AS(fchdir_args), (sy_call_t *)fchdir, AUE_FCHDIR },	/* 13 = fchdir */
	{ SYF_MPSAFE | AS(mknod_args), (sy_call_t *)mknod, AUE_MKNOD },	/* 14 = mknod */
	{ SYF_MPSAFE | AS(chmod_args), (sy_call_t *)chmod, AUE_CHMOD },	/* 15 = chmod */
	{ SYF_MPSAFE | AS(chown_args), (sy_call_t *)chown, AUE_CHOWN },	/* 16 = chown */
	{ SYF_MPSAFE | AS(obreak_args), (sy_call_t *)obreak, AUE_NULL },	/* 17 = break */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_getfsstat_args),freebsd32_getfsstat), AUE_GETFSSTAT },	/* 18 = old freebsd32_getfsstat */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 19 = obsolete olseek */
	{ SYF_MPSAFE | 0, (sy_call_t *)getpid, AUE_GETPID },	/* 20 = getpid */
	{ AS(mount_args), (sy_call_t *)mount, AUE_MOUNT },	/* 21 = mount */
	{ AS(unmount_args), (sy_call_t *)unmount, AUE_UMOUNT },	/* 22 = unmount */
	{ SYF_MPSAFE | AS(setuid_args), (sy_call_t *)setuid, AUE_SETUID },	/* 23 = setuid */
	{ SYF_MPSAFE | 0, (sy_call_t *)getuid, AUE_GETUID },	/* 24 = getuid */
	{ SYF_MPSAFE | 0, (sy_call_t *)geteuid, AUE_GETEUID },	/* 25 = geteuid */
	{ SYF_MPSAFE | AS(ptrace_args), (sy_call_t *)ptrace, AUE_PTRACE },	/* 26 = ptrace */
	{ SYF_MPSAFE | AS(freebsd32_recvmsg_args), (sy_call_t *)freebsd32_recvmsg, AUE_RECVMSG },	/* 27 = freebsd32_recvmsg */
	{ SYF_MPSAFE | AS(freebsd32_sendmsg_args), (sy_call_t *)freebsd32_sendmsg, AUE_SENDMSG },	/* 28 = freebsd32_sendmsg */
	{ SYF_MPSAFE | AS(freebsd32_recvfrom_args), (sy_call_t *)freebsd32_recvfrom, AUE_RECVFROM },	/* 29 = freebsd32_recvfrom */
	{ SYF_MPSAFE | AS(accept_args), (sy_call_t *)accept, AUE_ACCEPT },	/* 30 = accept */
	{ SYF_MPSAFE | AS(getpeername_args), (sy_call_t *)getpeername, AUE_GETPEERNAME },	/* 31 = getpeername */
	{ SYF_MPSAFE | AS(getsockname_args), (sy_call_t *)getsockname, AUE_GETSOCKNAME },	/* 32 = getsockname */
	{ SYF_MPSAFE | AS(access_args), (sy_call_t *)access, AUE_ACCESS },	/* 33 = access */
	{ SYF_MPSAFE | AS(chflags_args), (sy_call_t *)chflags, AUE_CHFLAGS },	/* 34 = chflags */
	{ SYF_MPSAFE | AS(fchflags_args), (sy_call_t *)fchflags, AUE_FCHFLAGS },	/* 35 = fchflags */
	{ SYF_MPSAFE | 0, (sy_call_t *)sync, AUE_SYNC },	/* 36 = sync */
	{ SYF_MPSAFE | AS(kill_args), (sy_call_t *)kill, AUE_KILL },	/* 37 = kill */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 38 = ostat */
	{ SYF_MPSAFE | 0, (sy_call_t *)getppid, AUE_GETPPID },	/* 39 = getppid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 40 = olstat */
	{ SYF_MPSAFE | AS(dup_args), (sy_call_t *)dup, AUE_DUP },	/* 41 = dup */
	{ SYF_MPSAFE | 0, (sy_call_t *)pipe, AUE_PIPE },	/* 42 = pipe */
	{ SYF_MPSAFE | 0, (sy_call_t *)getegid, AUE_GETEGID },	/* 43 = getegid */
	{ SYF_MPSAFE | AS(profil_args), (sy_call_t *)profil, AUE_PROFILE },	/* 44 = profil */
	{ SYF_MPSAFE | AS(ktrace_args), (sy_call_t *)ktrace, AUE_KTRACE },	/* 45 = ktrace */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigaction_args),freebsd32_sigaction), AUE_SIGACTION },	/* 46 = old freebsd32_sigaction */
	{ SYF_MPSAFE | 0, (sy_call_t *)getgid, AUE_GETGID },	/* 47 = getgid */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigprocmask_args),freebsd32_sigprocmask), AUE_SIGPROCMASK },	/* 48 = old freebsd32_sigprocmask */
	{ SYF_MPSAFE | AS(getlogin_args), (sy_call_t *)getlogin, AUE_GETLOGIN },	/* 49 = getlogin */
	{ SYF_MPSAFE | AS(setlogin_args), (sy_call_t *)setlogin, AUE_SETLOGIN },	/* 50 = setlogin */
	{ SYF_MPSAFE | AS(acct_args), (sy_call_t *)acct, AUE_ACCT },	/* 51 = acct */
	{ compat(SYF_MPSAFE | 0,freebsd32_sigpending), AUE_SIGPENDING },	/* 52 = old freebsd32_sigpending */
	{ SYF_MPSAFE | AS(freebsd32_sigaltstack_args), (sy_call_t *)freebsd32_sigaltstack, AUE_SIGPENDING },	/* 53 = freebsd32_sigaltstack */
	{ SYF_MPSAFE | AS(ioctl_args), (sy_call_t *)ioctl, AUE_IOCTL },	/* 54 = ioctl */
	{ SYF_MPSAFE | AS(reboot_args), (sy_call_t *)reboot, AUE_REBOOT },	/* 55 = reboot */
	{ SYF_MPSAFE | AS(revoke_args), (sy_call_t *)revoke, AUE_REVOKE },	/* 56 = revoke */
	{ SYF_MPSAFE | AS(symlink_args), (sy_call_t *)symlink, AUE_SYMLINK },	/* 57 = symlink */
	{ SYF_MPSAFE | AS(readlink_args), (sy_call_t *)readlink, AUE_READLINK },	/* 58 = readlink */
	{ SYF_MPSAFE | AS(freebsd32_execve_args), (sy_call_t *)freebsd32_execve, AUE_EXECVE },	/* 59 = freebsd32_execve */
	{ SYF_MPSAFE | AS(umask_args), (sy_call_t *)umask, AUE_UMASK },	/* 60 = umask */
	{ SYF_MPSAFE | AS(chroot_args), (sy_call_t *)chroot, AUE_CHROOT },	/* 61 = chroot */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 62 = obsolete ofstat */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 63 = obsolete ogetkerninfo */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 64 = obsolete ogetpagesize */
	{ SYF_MPSAFE | AS(msync_args), (sy_call_t *)msync, AUE_MSYNC },	/* 65 = msync */
	{ SYF_MPSAFE | 0, (sy_call_t *)vfork, AUE_VFORK },	/* 66 = vfork */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 67 = obsolete vread */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 68 = obsolete vwrite */
	{ SYF_MPSAFE | AS(sbrk_args), (sy_call_t *)sbrk, AUE_SBRK },	/* 69 = sbrk */
	{ SYF_MPSAFE | AS(sstk_args), (sy_call_t *)sstk, AUE_SSTK },	/* 70 = sstk */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 71 = obsolete ommap */
	{ SYF_MPSAFE | AS(ovadvise_args), (sy_call_t *)ovadvise, AUE_O_VADVISE },	/* 72 = vadvise */
	{ SYF_MPSAFE | AS(munmap_args), (sy_call_t *)munmap, AUE_MUNMAP },	/* 73 = munmap */
	{ SYF_MPSAFE | AS(mprotect_args), (sy_call_t *)mprotect, AUE_MPROTECT },	/* 74 = mprotect */
	{ SYF_MPSAFE | AS(madvise_args), (sy_call_t *)madvise, AUE_MADVISE },	/* 75 = madvise */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 76 = obsolete vhangup */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 77 = obsolete vlimit */
	{ SYF_MPSAFE | AS(mincore_args), (sy_call_t *)mincore, AUE_MINCORE },	/* 78 = mincore */
	{ SYF_MPSAFE | AS(getgroups_args), (sy_call_t *)getgroups, AUE_GETGROUPS },	/* 79 = getgroups */
	{ SYF_MPSAFE | AS(setgroups_args), (sy_call_t *)setgroups, AUE_SETGROUPS },	/* 80 = setgroups */
	{ SYF_MPSAFE | 0, (sy_call_t *)getpgrp, AUE_GETPGRP },	/* 81 = getpgrp */
	{ SYF_MPSAFE | AS(setpgid_args), (sy_call_t *)setpgid, AUE_SETPGRP },	/* 82 = setpgid */
	{ SYF_MPSAFE | AS(freebsd32_setitimer_args), (sy_call_t *)freebsd32_setitimer, AUE_SETITIMER },	/* 83 = freebsd32_setitimer */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 84 = obsolete owait */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 85 = obsolete oswapon */
	{ SYF_MPSAFE | AS(freebsd32_getitimer_args), (sy_call_t *)freebsd32_getitimer, AUE_GETITIMER },	/* 86 = freebsd32_getitimer */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 87 = obsolete ogethostname */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 88 = obsolete osethostname */
	{ SYF_MPSAFE | 0, (sy_call_t *)getdtablesize, AUE_GETDTABLESIZE },	/* 89 = getdtablesize */
	{ SYF_MPSAFE | AS(dup2_args), (sy_call_t *)dup2, AUE_DUP2 },	/* 90 = dup2 */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 91 = getdopt */
	{ SYF_MPSAFE | AS(fcntl_args), (sy_call_t *)fcntl, AUE_FCNTL },	/* 92 = fcntl */
	{ SYF_MPSAFE | AS(freebsd32_select_args), (sy_call_t *)freebsd32_select, AUE_SELECT },	/* 93 = freebsd32_select */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 94 = setdopt */
	{ SYF_MPSAFE | AS(fsync_args), (sy_call_t *)fsync, AUE_FSYNC },	/* 95 = fsync */
	{ SYF_MPSAFE | AS(setpriority_args), (sy_call_t *)setpriority, AUE_SETPRIORITY },	/* 96 = setpriority */
	{ SYF_MPSAFE | AS(socket_args), (sy_call_t *)socket, AUE_SOCKET },	/* 97 = socket */
	{ SYF_MPSAFE | AS(connect_args), (sy_call_t *)connect, AUE_CONNECT },	/* 98 = connect */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 99 = obsolete oaccept */
	{ SYF_MPSAFE | AS(getpriority_args), (sy_call_t *)getpriority, AUE_GETPRIORITY },	/* 100 = getpriority */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 101 = obsolete osend */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 102 = obsolete orecv */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 103 = obsolete osigreturn */
	{ SYF_MPSAFE | AS(bind_args), (sy_call_t *)bind, AUE_BIND },	/* 104 = bind */
	{ SYF_MPSAFE | AS(setsockopt_args), (sy_call_t *)setsockopt, AUE_SETSOCKOPT },	/* 105 = setsockopt */
	{ SYF_MPSAFE | AS(listen_args), (sy_call_t *)listen, AUE_LISTEN },	/* 106 = listen */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 107 = obsolete vtimes */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigvec_args),freebsd32_sigvec), AUE_O_SIGVEC },	/* 108 = old freebsd32_sigvec */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigblock_args),freebsd32_sigblock), AUE_O_SIGBLOCK },	/* 109 = old freebsd32_sigblock */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigsetmask_args),freebsd32_sigsetmask), AUE_O_SIGSETMASK },	/* 110 = old freebsd32_sigsetmask */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigsuspend_args),freebsd32_sigsuspend), AUE_SIGSUSPEND },	/* 111 = old freebsd32_sigsuspend */
	{ compat(SYF_MPSAFE | AS(ofreebsd32_sigstack_args),freebsd32_sigstack), AUE_O_SIGSTACK },	/* 112 = old freebsd32_sigstack */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 113 = obsolete orecvmsg */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 114 = obsolete osendmsg */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 115 = obsolete vtrace */
	{ SYF_MPSAFE | AS(freebsd32_gettimeofday_args), (sy_call_t *)freebsd32_gettimeofday, AUE_GETTIMEOFDAY },	/* 116 = freebsd32_gettimeofday */
	{ SYF_MPSAFE | AS(freebsd32_getrusage_args), (sy_call_t *)freebsd32_getrusage, AUE_GETRUSAGE },	/* 117 = freebsd32_getrusage */
	{ SYF_MPSAFE | AS(getsockopt_args), (sy_call_t *)getsockopt, AUE_GETSOCKOPT },	/* 118 = getsockopt */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 119 = resuba */
	{ SYF_MPSAFE | AS(freebsd32_readv_args), (sy_call_t *)freebsd32_readv, AUE_READV },	/* 120 = freebsd32_readv */
	{ SYF_MPSAFE | AS(freebsd32_writev_args), (sy_call_t *)freebsd32_writev, AUE_WRITEV },	/* 121 = freebsd32_writev */
	{ SYF_MPSAFE | AS(freebsd32_settimeofday_args), (sy_call_t *)freebsd32_settimeofday, AUE_SETTIMEOFDAY },	/* 122 = freebsd32_settimeofday */
	{ SYF_MPSAFE | AS(fchown_args), (sy_call_t *)fchown, AUE_FCHOWN },	/* 123 = fchown */
	{ SYF_MPSAFE | AS(fchmod_args), (sy_call_t *)fchmod, AUE_FCHMOD },	/* 124 = fchmod */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 125 = obsolete orecvfrom */
	{ SYF_MPSAFE | AS(setreuid_args), (sy_call_t *)setreuid, AUE_SETREUID },	/* 126 = setreuid */
	{ SYF_MPSAFE | AS(setregid_args), (sy_call_t *)setregid, AUE_SETREGID },	/* 127 = setregid */
	{ SYF_MPSAFE | AS(rename_args), (sy_call_t *)rename, AUE_RENAME },	/* 128 = rename */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 129 = obsolete otruncate */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 130 = obsolete ftruncate */
	{ SYF_MPSAFE | AS(flock_args), (sy_call_t *)flock, AUE_FLOCK },	/* 131 = flock */
	{ SYF_MPSAFE | AS(mkfifo_args), (sy_call_t *)mkfifo, AUE_MKFIFO },	/* 132 = mkfifo */
	{ SYF_MPSAFE | AS(sendto_args), (sy_call_t *)sendto, AUE_SENDTO },	/* 133 = sendto */
	{ SYF_MPSAFE | AS(shutdown_args), (sy_call_t *)shutdown, AUE_SHUTDOWN },	/* 134 = shutdown */
	{ SYF_MPSAFE | AS(socketpair_args), (sy_call_t *)socketpair, AUE_SOCKETPAIR },	/* 135 = socketpair */
	{ AS(mkdir_args), (sy_call_t *)mkdir, AUE_MKDIR },	/* 136 = mkdir */
	{ AS(rmdir_args), (sy_call_t *)rmdir, AUE_RMDIR },	/* 137 = rmdir */
	{ AS(freebsd32_utimes_args), (sy_call_t *)freebsd32_utimes, AUE_UTIMES },	/* 138 = freebsd32_utimes */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 139 = obsolete 4.2 sigreturn */
	{ SYF_MPSAFE | AS(freebsd32_adjtime_args), (sy_call_t *)freebsd32_adjtime, AUE_ADJTIME },	/* 140 = freebsd32_adjtime */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 141 = obsolete ogetpeername */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 142 = obsolete ogethostid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 143 = obsolete sethostid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 144 = obsolete getrlimit */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 145 = obsolete setrlimit */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 146 = obsolete killpg */
	{ SYF_MPSAFE | 0, (sy_call_t *)setsid, AUE_SETSID },	/* 147 = setsid */
	{ SYF_MPSAFE | AS(quotactl_args), (sy_call_t *)quotactl, AUE_QUOTACTL },	/* 148 = quotactl */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 149 = obsolete oquota */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 150 = obsolete ogetsockname */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 151 = sem_lock */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 152 = sem_wakeup */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 153 = asyncdaemon */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 154 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 155 = nfssvc */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 156 = obsolete ogetdirentries */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_statfs_args),freebsd32_statfs), AUE_STATFS },	/* 157 = old freebsd32_statfs */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_fstatfs_args),freebsd32_fstatfs), AUE_FSTATFS },	/* 158 = old freebsd32_fstatfs */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 159 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 160 = nosys */
	{ SYF_MPSAFE | AS(getfh_args), (sy_call_t *)getfh, AUE_NFS_GETFH },	/* 161 = getfh */
	{ SYF_MPSAFE | AS(getdomainname_args), (sy_call_t *)getdomainname, AUE_O_GETDOMAINNAME },	/* 162 = getdomainname */
	{ SYF_MPSAFE | AS(setdomainname_args), (sy_call_t *)setdomainname, AUE_O_SETDOMAINNAME },	/* 163 = setdomainname */
	{ SYF_MPSAFE | AS(uname_args), (sy_call_t *)uname, AUE_NULL },	/* 164 = uname */
	{ SYF_MPSAFE | AS(sysarch_args), (sy_call_t *)sysarch, AUE_NULL },	/* 165 = sysarch */
	{ SYF_MPSAFE | AS(rtprio_args), (sy_call_t *)rtprio, AUE_NULL },	/* 166 = rtprio */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 167 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 168 = nosys */
	{ SYF_MPSAFE | AS(freebsd32_semsys_args), (sy_call_t *)freebsd32_semsys, AUE_SEMSYS },	/* 169 = freebsd32_semsys */
	{ SYF_MPSAFE | AS(freebsd32_msgsys_args), (sy_call_t *)freebsd32_msgsys, AUE_MSGSYS },	/* 170 = freebsd32_msgsys */
	{ SYF_MPSAFE | AS(freebsd32_shmsys_args), (sy_call_t *)freebsd32_shmsys, AUE_SHMSYS },	/* 171 = freebsd32_shmsys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 172 = nosys */
	{ SYF_MPSAFE | AS(freebsd32_pread_args), (sy_call_t *)freebsd32_pread, AUE_PREAD },	/* 173 = freebsd32_pread */
	{ SYF_MPSAFE | AS(freebsd32_pwrite_args), (sy_call_t *)freebsd32_pwrite, AUE_PWRITE },	/* 174 = freebsd32_pwrite */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 175 = nosys */
	{ SYF_MPSAFE | AS(ntp_adjtime_args), (sy_call_t *)ntp_adjtime, AUE_ADJTIME },	/* 176 = ntp_adjtime */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 177 = sfork */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 178 = getdescriptor */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 179 = setdescriptor */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 180 = nosys */
	{ SYF_MPSAFE | AS(setgid_args), (sy_call_t *)setgid, AUE_SETGID },	/* 181 = setgid */
	{ SYF_MPSAFE | AS(setegid_args), (sy_call_t *)setegid, AUE_SETEGID },	/* 182 = setegid */
	{ SYF_MPSAFE | AS(seteuid_args), (sy_call_t *)seteuid, AUE_SETEUID },	/* 183 = seteuid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 184 = lfs_bmapv */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 185 = lfs_markv */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 186 = lfs_segclean */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 187 = lfs_segwait */
	{ SYF_MPSAFE | AS(freebsd32_stat_args), (sy_call_t *)freebsd32_stat, AUE_STAT },	/* 188 = freebsd32_stat */
	{ SYF_MPSAFE | AS(freebsd32_fstat_args), (sy_call_t *)freebsd32_fstat, AUE_FSTAT },	/* 189 = freebsd32_fstat */
	{ SYF_MPSAFE | AS(freebsd32_lstat_args), (sy_call_t *)freebsd32_lstat, AUE_LSTAT },	/* 190 = freebsd32_lstat */
	{ SYF_MPSAFE | AS(pathconf_args), (sy_call_t *)pathconf, AUE_PATHCONF },	/* 191 = pathconf */
	{ SYF_MPSAFE | AS(fpathconf_args), (sy_call_t *)fpathconf, AUE_FPATHCONF },	/* 192 = fpathconf */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 193 = nosys */
	{ SYF_MPSAFE | AS(__getrlimit_args), (sy_call_t *)getrlimit, AUE_GETRLIMIT },	/* 194 = getrlimit */
	{ SYF_MPSAFE | AS(__setrlimit_args), (sy_call_t *)setrlimit, AUE_SETRLIMIT },	/* 195 = setrlimit */
	{ SYF_MPSAFE | AS(getdirentries_args), (sy_call_t *)getdirentries, AUE_GETDIRENTRIES },	/* 196 = getdirentries */
	{ SYF_MPSAFE | AS(freebsd32_mmap_args), (sy_call_t *)freebsd32_mmap, AUE_MMAP },	/* 197 = freebsd32_mmap */
	{ SYF_MPSAFE | 0, (sy_call_t *)nosys, AUE_NULL },	/* 198 = __syscall */
	{ SYF_MPSAFE | AS(freebsd32_lseek_args), (sy_call_t *)freebsd32_lseek, AUE_LSEEK },	/* 199 = freebsd32_lseek */
	{ SYF_MPSAFE | AS(freebsd32_truncate_args), (sy_call_t *)freebsd32_truncate, AUE_TRUNCATE },	/* 200 = freebsd32_truncate */
	{ SYF_MPSAFE | AS(freebsd32_ftruncate_args), (sy_call_t *)freebsd32_ftruncate, AUE_FTRUNCATE },	/* 201 = freebsd32_ftruncate */
	{ SYF_MPSAFE | AS(freebsd32_sysctl_args), (sy_call_t *)freebsd32_sysctl, AUE_SYSCTL },	/* 202 = freebsd32_sysctl */
	{ SYF_MPSAFE | AS(mlock_args), (sy_call_t *)mlock, AUE_MLOCK },	/* 203 = mlock */
	{ SYF_MPSAFE | AS(munlock_args), (sy_call_t *)munlock, AUE_MUNLOCK },	/* 204 = munlock */
	{ SYF_MPSAFE | AS(undelete_args), (sy_call_t *)undelete, AUE_UNDELETE },	/* 205 = undelete */
	{ SYF_MPSAFE | AS(freebsd32_futimes_args), (sy_call_t *)freebsd32_futimes, AUE_FUTIMES },	/* 206 = freebsd32_futimes */
	{ SYF_MPSAFE | AS(getpgid_args), (sy_call_t *)getpgid, AUE_GETPGID },	/* 207 = getpgid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 208 = newreboot */
	{ SYF_MPSAFE | AS(poll_args), (sy_call_t *)poll, AUE_POLL },	/* 209 = poll */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 210 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 211 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 212 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 213 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 214 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 215 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 216 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 217 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 218 =  */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 219 =  */
	{ SYF_MPSAFE | AS(__semctl_args), (sy_call_t *)__semctl, AUE_SEMCTL },	/* 220 = __semctl */
	{ SYF_MPSAFE | AS(semget_args), (sy_call_t *)semget, AUE_SEMGET },	/* 221 = semget */
	{ SYF_MPSAFE | AS(semop_args), (sy_call_t *)semop, AUE_SEMOP },	/* 222 = semop */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 223 = semconfig */
	{ SYF_MPSAFE | AS(msgctl_args), (sy_call_t *)msgctl, AUE_MSGCTL },	/* 224 = msgctl */
	{ SYF_MPSAFE | AS(msgget_args), (sy_call_t *)msgget, AUE_MSGGET },	/* 225 = msgget */
	{ SYF_MPSAFE | AS(msgsnd_args), (sy_call_t *)msgsnd, AUE_MSGSND },	/* 226 = msgsnd */
	{ SYF_MPSAFE | AS(msgrcv_args), (sy_call_t *)msgrcv, AUE_MSGRCV },	/* 227 = msgrcv */
	{ SYF_MPSAFE | AS(shmat_args), (sy_call_t *)shmat, AUE_SHMAT },	/* 228 = shmat */
	{ SYF_MPSAFE | AS(shmctl_args), (sy_call_t *)shmctl, AUE_SHMCTL },	/* 229 = shmctl */
	{ SYF_MPSAFE | AS(shmdt_args), (sy_call_t *)shmdt, AUE_SHMDT },	/* 230 = shmdt */
	{ SYF_MPSAFE | AS(shmget_args), (sy_call_t *)shmget, AUE_SHMGET },	/* 231 = shmget */
	{ SYF_MPSAFE | AS(freebsd32_clock_gettime_args), (sy_call_t *)freebsd32_clock_gettime, AUE_NULL },	/* 232 = freebsd32_clock_gettime */
	{ SYF_MPSAFE | AS(freebsd32_clock_settime_args), (sy_call_t *)freebsd32_clock_settime, AUE_CLOCK_SETTIME },	/* 233 = freebsd32_clock_settime */
	{ SYF_MPSAFE | AS(freebsd32_clock_getres_args), (sy_call_t *)freebsd32_clock_getres, AUE_NULL },	/* 234 = freebsd32_clock_getres */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 235 = timer_create */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 236 = timer_delete */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 237 = timer_settime */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 238 = timer_gettime */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 239 = timer_getoverrun */
	{ SYF_MPSAFE | AS(freebsd32_nanosleep_args), (sy_call_t *)freebsd32_nanosleep, AUE_NULL },	/* 240 = freebsd32_nanosleep */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 241 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 242 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 243 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 244 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 245 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 246 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 247 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 248 = ntp_gettime */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 249 = nosys */
	{ SYF_MPSAFE | AS(minherit_args), (sy_call_t *)minherit, AUE_MINHERIT },	/* 250 = minherit */
	{ SYF_MPSAFE | AS(rfork_args), (sy_call_t *)rfork, AUE_RFORK },	/* 251 = rfork */
	{ SYF_MPSAFE | AS(openbsd_poll_args), (sy_call_t *)openbsd_poll, AUE_POLL },	/* 252 = openbsd_poll */
	{ SYF_MPSAFE | 0, (sy_call_t *)issetugid, AUE_ISSETUGID },	/* 253 = issetugid */
	{ SYF_MPSAFE | AS(lchown_args), (sy_call_t *)lchown, AUE_LCHOWN },	/* 254 = lchown */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 255 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 256 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 257 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 258 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 259 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 260 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 261 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 262 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 263 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 264 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 265 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 266 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 267 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 268 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 269 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 270 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 271 = nosys */
	{ SYF_MPSAFE | AS(getdents_args), (sy_call_t *)getdents, AUE_O_GETDENTS },	/* 272 = getdents */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 273 = nosys */
	{ SYF_MPSAFE | AS(lchmod_args), (sy_call_t *)lchmod, AUE_LCHMOD },	/* 274 = lchmod */
	{ SYF_MPSAFE | AS(lchown_args), (sy_call_t *)lchown, AUE_LCHOWN },	/* 275 = netbsd_lchown */
	{ SYF_MPSAFE | AS(freebsd32_lutimes_args), (sy_call_t *)freebsd32_lutimes, AUE_LUTIMES },	/* 276 = freebsd32_lutimes */
	{ SYF_MPSAFE | AS(msync_args), (sy_call_t *)msync, AUE_MSYNC },	/* 277 = netbsd_msync */
	{ SYF_MPSAFE | AS(nstat_args), (sy_call_t *)nstat, AUE_STAT },	/* 278 = nstat */
	{ SYF_MPSAFE | AS(nfstat_args), (sy_call_t *)nfstat, AUE_FSTAT },	/* 279 = nfstat */
	{ SYF_MPSAFE | AS(nlstat_args), (sy_call_t *)nlstat, AUE_LSTAT },	/* 280 = nlstat */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 281 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 282 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 283 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 284 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 285 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 286 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 287 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 288 = nosys */
	{ SYF_MPSAFE | AS(freebsd32_preadv_args), (sy_call_t *)freebsd32_preadv, AUE_PREADV },	/* 289 = freebsd32_preadv */
	{ SYF_MPSAFE | AS(freebsd32_pwritev_args), (sy_call_t *)freebsd32_pwritev, AUE_PWRITEV },	/* 290 = freebsd32_pwritev */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 291 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 292 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 293 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 294 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 295 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 296 = nosys */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_fhstatfs_args),freebsd32_fhstatfs), AUE_FHSTATFS },	/* 297 = old freebsd32_fhstatfs */
	{ SYF_MPSAFE | AS(fhopen_args), (sy_call_t *)fhopen, AUE_FHOPEN },	/* 298 = fhopen */
	{ SYF_MPSAFE | AS(fhstat_args), (sy_call_t *)fhstat, AUE_FHSTAT },	/* 299 = fhstat */
	{ SYF_MPSAFE | AS(modnext_args), (sy_call_t *)modnext, AUE_NULL },	/* 300 = modnext */
	{ SYF_MPSAFE | AS(freebsd32_modstat_args), (sy_call_t *)freebsd32_modstat, AUE_NULL },	/* 301 = freebsd32_modstat */
	{ SYF_MPSAFE | AS(modfnext_args), (sy_call_t *)modfnext, AUE_NULL },	/* 302 = modfnext */
	{ SYF_MPSAFE | AS(modfind_args), (sy_call_t *)modfind, AUE_NULL },	/* 303 = modfind */
	{ SYF_MPSAFE | AS(kldload_args), (sy_call_t *)kldload, AUE_MODLOAD },	/* 304 = kldload */
	{ SYF_MPSAFE | AS(kldunload_args), (sy_call_t *)kldunload, AUE_MODUNLOAD },	/* 305 = kldunload */
	{ SYF_MPSAFE | AS(kldfind_args), (sy_call_t *)kldfind, AUE_NULL },	/* 306 = kldfind */
	{ SYF_MPSAFE | AS(kldnext_args), (sy_call_t *)kldnext, AUE_NULL },	/* 307 = kldnext */
	{ SYF_MPSAFE | AS(kldstat_args), (sy_call_t *)kldstat, AUE_NULL },	/* 308 = kldstat */
	{ SYF_MPSAFE | AS(kldfirstmod_args), (sy_call_t *)kldfirstmod, AUE_NULL },	/* 309 = kldfirstmod */
	{ SYF_MPSAFE | AS(getsid_args), (sy_call_t *)getsid, AUE_GETSID },	/* 310 = getsid */
	{ SYF_MPSAFE | AS(setresuid_args), (sy_call_t *)setresuid, AUE_SETRESUID },	/* 311 = setresuid */
	{ SYF_MPSAFE | AS(setresgid_args), (sy_call_t *)setresgid, AUE_SETRESGID },	/* 312 = setresgid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 313 = obsolete signanosleep */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 314 = aio_return */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 315 = aio_suspend */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 316 = aio_cancel */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 317 = aio_error */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 318 = aio_read */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 319 = aio_write */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 320 = lio_listio */
	{ SYF_MPSAFE | 0, (sy_call_t *)yield, AUE_NULL },	/* 321 = yield */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 322 = obsolete thr_sleep */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 323 = obsolete thr_wakeup */
	{ SYF_MPSAFE | AS(mlockall_args), (sy_call_t *)mlockall, AUE_MLOCKALL },	/* 324 = mlockall */
	{ SYF_MPSAFE | 0, (sy_call_t *)munlockall, AUE_MUNLOCKALL },	/* 325 = munlockall */
	{ SYF_MPSAFE | AS(__getcwd_args), (sy_call_t *)__getcwd, AUE_NULL },	/* 326 = __getcwd */
	{ SYF_MPSAFE | AS(sched_setparam_args), (sy_call_t *)sched_setparam, AUE_NULL },	/* 327 = sched_setparam */
	{ SYF_MPSAFE | AS(sched_getparam_args), (sy_call_t *)sched_getparam, AUE_NULL },	/* 328 = sched_getparam */
	{ SYF_MPSAFE | AS(sched_setscheduler_args), (sy_call_t *)sched_setscheduler, AUE_NULL },	/* 329 = sched_setscheduler */
	{ SYF_MPSAFE | AS(sched_getscheduler_args), (sy_call_t *)sched_getscheduler, AUE_NULL },	/* 330 = sched_getscheduler */
	{ SYF_MPSAFE | 0, (sy_call_t *)sched_yield, AUE_NULL },	/* 331 = sched_yield */
	{ SYF_MPSAFE | AS(sched_get_priority_max_args), (sy_call_t *)sched_get_priority_max, AUE_NULL },	/* 332 = sched_get_priority_max */
	{ SYF_MPSAFE | AS(sched_get_priority_min_args), (sy_call_t *)sched_get_priority_min, AUE_NULL },	/* 333 = sched_get_priority_min */
	{ SYF_MPSAFE | AS(sched_rr_get_interval_args), (sy_call_t *)sched_rr_get_interval, AUE_NULL },	/* 334 = sched_rr_get_interval */
	{ SYF_MPSAFE | AS(utrace_args), (sy_call_t *)utrace, AUE_NULL },	/* 335 = utrace */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_sendfile_args),freebsd32_sendfile), AUE_SENDFILE },	/* 336 = old freebsd32_sendfile */
	{ SYF_MPSAFE | AS(kldsym_args), (sy_call_t *)kldsym, AUE_NULL },	/* 337 = kldsym */
	{ SYF_MPSAFE | AS(jail_args), (sy_call_t *)jail, AUE_NULL },	/* 338 = jail */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 339 = pioctl */
	{ SYF_MPSAFE | AS(sigprocmask_args), (sy_call_t *)sigprocmask, AUE_SIGPROCMASK },	/* 340 = sigprocmask */
	{ SYF_MPSAFE | AS(sigsuspend_args), (sy_call_t *)sigsuspend, AUE_SIGSUSPEND },	/* 341 = sigsuspend */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_sigaction_args),freebsd32_sigaction), AUE_SIGACTION },	/* 342 = old freebsd32_sigaction */
	{ SYF_MPSAFE | AS(sigpending_args), (sy_call_t *)sigpending, AUE_SIGPENDING },	/* 343 = sigpending */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_sigreturn_args),freebsd32_sigreturn), AUE_SIGRETURN },	/* 344 = old freebsd32_sigreturn */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 345 = sigtimedwait */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 346 = sigwaitinfo */
	{ SYF_MPSAFE | AS(__acl_get_file_args), (sy_call_t *)__acl_get_file, AUE_JAIL },	/* 347 = __acl_get_file */
	{ SYF_MPSAFE | AS(__acl_set_file_args), (sy_call_t *)__acl_set_file, AUE_NULL },	/* 348 = __acl_set_file */
	{ SYF_MPSAFE | AS(__acl_get_fd_args), (sy_call_t *)__acl_get_fd, AUE_NULL },	/* 349 = __acl_get_fd */
	{ SYF_MPSAFE | AS(__acl_set_fd_args), (sy_call_t *)__acl_set_fd, AUE_NULL },	/* 350 = __acl_set_fd */
	{ SYF_MPSAFE | AS(__acl_delete_file_args), (sy_call_t *)__acl_delete_file, AUE_NULL },	/* 351 = __acl_delete_file */
	{ SYF_MPSAFE | AS(__acl_delete_fd_args), (sy_call_t *)__acl_delete_fd, AUE_NULL },	/* 352 = __acl_delete_fd */
	{ SYF_MPSAFE | AS(__acl_aclcheck_file_args), (sy_call_t *)__acl_aclcheck_file, AUE_NULL },	/* 353 = __acl_aclcheck_file */
	{ SYF_MPSAFE | AS(__acl_aclcheck_fd_args), (sy_call_t *)__acl_aclcheck_fd, AUE_NULL },	/* 354 = __acl_aclcheck_fd */
	{ AS(extattrctl_args), (sy_call_t *)extattrctl, AUE_NULL },	/* 355 = extattrctl */
	{ AS(extattr_set_file_args), (sy_call_t *)extattr_set_file, AUE_NULL },	/* 356 = extattr_set_file */
	{ AS(extattr_get_file_args), (sy_call_t *)extattr_get_file, AUE_NULL },	/* 357 = extattr_get_file */
	{ AS(extattr_delete_file_args), (sy_call_t *)extattr_delete_file, AUE_NULL },	/* 358 = extattr_delete_file */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 359 = aio_waitcomplete */
	{ SYF_MPSAFE | AS(getresuid_args), (sy_call_t *)getresuid, AUE_NULL },	/* 360 = getresuid */
	{ SYF_MPSAFE | AS(getresgid_args), (sy_call_t *)getresgid, AUE_NULL },	/* 361 = getresgid */
	{ SYF_MPSAFE | 0, (sy_call_t *)kqueue, AUE_NULL },	/* 362 = kqueue */
	{ SYF_MPSAFE | AS(freebsd32_kevent_args), (sy_call_t *)freebsd32_kevent, AUE_NULL },	/* 363 = freebsd32_kevent */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 364 = __cap_get_proc */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 365 = __cap_set_proc */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 366 = __cap_get_fd */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 367 = __cap_get_file */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 368 = __cap_set_fd */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 369 = __cap_set_file */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 370 = lkmressys */
	{ AS(extattr_set_fd_args), (sy_call_t *)extattr_set_fd, AUE_NULL },	/* 371 = extattr_set_fd */
	{ AS(extattr_get_fd_args), (sy_call_t *)extattr_get_fd, AUE_NULL },	/* 372 = extattr_get_fd */
	{ AS(extattr_delete_fd_args), (sy_call_t *)extattr_delete_fd, AUE_NULL },	/* 373 = extattr_delete_fd */
	{ SYF_MPSAFE | AS(__setugid_args), (sy_call_t *)__setugid, AUE_NULL },	/* 374 = __setugid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 375 = nfsclnt */
	{ SYF_MPSAFE | AS(eaccess_args), (sy_call_t *)eaccess, AUE_NULL },	/* 376 = eaccess */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 377 = afs_syscall */
	{ AS(nmount_args), (sy_call_t *)nmount, AUE_NULL },	/* 378 = nmount */
	{ SYF_MPSAFE | 0, (sy_call_t *)kse_exit, AUE_NULL },	/* 379 = kse_exit */
	{ SYF_MPSAFE | AS(kse_wakeup_args), (sy_call_t *)kse_wakeup, AUE_NULL },	/* 380 = kse_wakeup */
	{ SYF_MPSAFE | AS(kse_create_args), (sy_call_t *)kse_create, AUE_NULL },	/* 381 = kse_create */
	{ SYF_MPSAFE | AS(kse_thr_interrupt_args), (sy_call_t *)kse_thr_interrupt, AUE_NULL },	/* 382 = kse_thr_interrupt */
	{ SYF_MPSAFE | 0, (sy_call_t *)kse_release, AUE_NULL },	/* 383 = kse_release */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 384 = __mac_get_proc */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 385 = __mac_set_proc */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 386 = __mac_get_fd */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 387 = __mac_get_file */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 388 = __mac_set_fd */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 389 = __mac_set_file */
	{ SYF_MPSAFE | AS(kenv_args), (sy_call_t *)kenv, AUE_NULL },	/* 390 = kenv */
	{ SYF_MPSAFE | AS(lchflags_args), (sy_call_t *)lchflags, AUE_LCHFLAGS },	/* 391 = lchflags */
	{ SYF_MPSAFE | AS(uuidgen_args), (sy_call_t *)uuidgen, AUE_NULL },	/* 392 = uuidgen */
	{ SYF_MPSAFE | AS(freebsd32_sendfile_args), (sy_call_t *)freebsd32_sendfile, AUE_NULL },	/* 393 = freebsd32_sendfile */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 394 = mac_syscall */
	{ SYF_MPSAFE | AS(getfsstat_args), (sy_call_t *)getfsstat, AUE_GETFSSTAT },	/* 395 = getfsstat */
	{ SYF_MPSAFE | AS(statfs_args), (sy_call_t *)statfs, AUE_STATFS },	/* 396 = statfs */
	{ SYF_MPSAFE | AS(fstatfs_args), (sy_call_t *)fstatfs, AUE_FSTATFS },	/* 397 = fstatfs */
	{ SYF_MPSAFE | AS(fhstatfs_args), (sy_call_t *)fhstatfs, AUE_NULL },	/* 398 = fhstatfs */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 399 = nosys */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 400 = ksem_close */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 401 = ksem_post */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 402 = ksem_wait */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 403 = ksem_trywait */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 404 = ksem_init */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 405 = ksem_open */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 406 = ksem_unlink */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 407 = ksem_getvalue */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 408 = ksem_destroy */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 409 = __mac_get_pid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 410 = __mac_get_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 411 = __mac_set_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 412 = extattr_set_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 413 = extattr_get_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 414 = extattr_delete_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 415 = __mac_execve */
	{ SYF_MPSAFE | AS(freebsd32_sigaction_args), (sy_call_t *)freebsd32_sigaction, AUE_SIGACTION },	/* 416 = freebsd32_sigaction */
	{ SYF_MPSAFE | AS(freebsd32_sigreturn_args), (sy_call_t *)freebsd32_sigreturn, AUE_SIGRETURN },	/* 417 = freebsd32_sigreturn */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 418 = __xstat */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 419 = __xfstat */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 420 = __xlstat */
	{ SYF_MPSAFE | AS(freebsd32_getcontext_args), (sy_call_t *)freebsd32_getcontext, AUE_NULL },	/* 421 = freebsd32_getcontext */
	{ SYF_MPSAFE | AS(freebsd32_setcontext_args), (sy_call_t *)freebsd32_setcontext, AUE_NULL },	/* 422 = freebsd32_setcontext */
	{ SYF_MPSAFE | AS(freebsd32_swapcontext_args), (sy_call_t *)freebsd32_swapcontext, AUE_NULL },	/* 423 = freebsd32_swapcontext */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 424 = swapoff */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 425 = __acl_get_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 426 = __acl_set_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 427 = __acl_delete_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 428 = __acl_aclcheck_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 429 = sigwait */
	{ SYF_MPSAFE | AS(thr_create_args), (sy_call_t *)thr_create, AUE_NULL },	/* 430 = thr_create */
	{ SYF_MPSAFE | AS(thr_exit_args), (sy_call_t *)thr_exit, AUE_NULL },	/* 431 = thr_exit */
	{ SYF_MPSAFE | AS(thr_self_args), (sy_call_t *)thr_self, AUE_NULL },	/* 432 = thr_self */
	{ SYF_MPSAFE | AS(thr_kill_args), (sy_call_t *)thr_kill, AUE_NULL },	/* 433 = thr_kill */
	{ SYF_MPSAFE | AS(_umtx_lock_args), (sy_call_t *)_umtx_lock, AUE_NULL },	/* 434 = _umtx_lock */
	{ SYF_MPSAFE | AS(_umtx_unlock_args), (sy_call_t *)_umtx_unlock, AUE_NULL },	/* 435 = _umtx_unlock */
	{ SYF_MPSAFE | AS(jail_attach_args), (sy_call_t *)jail_attach, AUE_NULL },	/* 436 = jail_attach */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 437 = extattr_list_fd */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 438 = extattr_list_file */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 439 = extattr_list_link */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 440 = kse_switchin */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 441 = ksem_timedwait */
	{ SYF_MPSAFE | AS(thr_suspend_args), (sy_call_t *)thr_suspend, AUE_NULL },	/* 442 = thr_suspend */
	{ SYF_MPSAFE | AS(thr_wake_args), (sy_call_t *)thr_wake, AUE_NULL },	/* 443 = thr_wake */
	{ SYF_MPSAFE | AS(kldunloadf_args), (sy_call_t *)kldunloadf, AUE_NULL },	/* 444 = kldunloadf */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 445 = audit */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 446 = auditon */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 447 = getauid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 448 = setauid */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 449 = getaudit */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 450 = setaudit */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 451 = getaudit_addr */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 452 = setaudit_addr */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 453 = auditctl */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 454 = _umtx_op */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 455 = thr_new */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 456 = sigqueue */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 457 = mq_open */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 458 = mq_setattr */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 459 = mq_timedreceive */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 460 = mq_timedsend */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 461 = mq_notify */
	{ 0, (sy_call_t *)nosys, AUE_NULL },			/* 462 = mq_unlink */
	{ SYF_MPSAFE | AS(abort2_args), (sy_call_t *)abort2, AUE_NULL },	/* 463 = abort2 */
};
