#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([OpenBSM], [1.0a5], [trustedbsd-audit@TrustesdBSD.org],[openbsm])
AC_REVISION([$P4: //depot/projects/trustedbsd/openbsm/configure.ac#16 $])
AC_CONFIG_SRCDIR([bin/auditreduce/auditreduce.c])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lbsm':
AC_CHECK_LIB([bsm], [main])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([endian.h mach/mach.h machine/endian.h sys/endian.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

AC_CHECK_MEMBER([struct ipc_perm.__key],
[AC_DEFINE(HAVE_IPC_PERM___KEY,, Define if ipc_perm.__key instead of key)],
[],[
#include <sys/types.h>
#include <sys/ipc.h>
])

AC_CHECK_MEMBER([struct ipc_perm.__seq],
[AC_DEFINE(HAVE_IPC_PERM___SEQ,, Define if ipc_perm.__seq instead of seq)],
[],[
#include <sys/types.h>
#include <sys/ipc.h>
])

AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([bzero ftruncate gettimeofday inet_ntoa memset strchr strerror strrchr strstr strtol strtoul])

# sys/queue.h exists on most systems, but its capabilities vary a great deal.
# test for LIST_FIRST, which appears to not exist in all of them, and is
# necessary for OpenBSM.
AC_TRY_LINK([
	#include <sys/queue.h>
], [
	struct foo {
		LIST_ENTRY(foo) foo_entries;
	};
	LIST_HEAD(, foo) foo_list;
	struct foo *foo;

	foo = LIST_FIRST(&foo_list);
], [
AC_DEFINE(HAVE_FULL_QUEUE_H,, Define if queue.h includes LIST_FIRST)
])

# Systems may not define key audit system calls, in which case libbsm cannot
# depend on them or it will generate link-time or run-time errors.  Test for
# just one.
AC_TRY_LINK([
	#include <stdlib.h>

	extern int auditon(int, void *, int);
], [
	int err;

	err = auditon(0, NULL, 0);
], [
AC_DEFINE(HAVE_AUDIT_SYSCALLS,, Define if audit system calls present)
have_audit_syscalls=true
], [
have_audit_syscalls=false
])
AM_CONDITIONAL(HAVE_AUDIT_SYSCALLS, $have_audit_syscalls)

AC_CONFIG_FILES([Makefile
                 bin/Makefile
                 bin/audit/Makefile
                 bin/auditd/Makefile
                 bin/auditreduce/Makefile
                 bin/praudit/Makefile
                 bsm/Makefile
                 libbsm/Makefile
                 man/Makefile
                 tools/Makefile])

AC_OUTPUT
