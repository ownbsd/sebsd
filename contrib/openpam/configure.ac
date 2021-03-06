dnl $P4: //depot/projects/openpam/configure.ac#2 $

AC_PREREQ(2.53)
AC_INIT([OpenPAM],[20050201],[des@freebsd.org],[openpam])
AC_CONFIG_SRCDIR([lib/pam_start.c])
AM_CONFIG_HEADER([config.h])

AC_CANONICAL_SYSTEM
AC_C_VOLATILE
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

LIB_MAJ=2
AC_SUBST(LIB_MAJ)
AC_DEFINE_UNQUOTED(LIB_MAJ, $LIB_MAJ, [OpenPAM library major number])

AC_MSG_CHECKING([whether loading unversioned modules support is enabled])
AC_ARG_ENABLE(unversioned-modules,
	AC_HELP_STRING([--disable-unversioned-modules],
		[support loading of unversioned modules]),
	[if test "$enableval" = "no"; then
		AC_DEFINE(DISABLE_UNVERSIONED_MODULES,
			1,
			[Whether loading unversioned modules support is disabled])
	fi
	AC_MSG_RESULT(no)],
	AC_MSG_RESULT(yes))

AC_MSG_CHECKING([for modules directory support])
AC_ARG_WITH(modules-dir,
	AC_HELP_STRING([--with-modules-dir=DIR],
		[OpenPAM modules directory]),
	[if test "$withval" != "no"; then
		OPENPAM_MODULES_DIR="$withval"
		AC_DEFINE_UNQUOTED(OPENPAM_MODULES_DIR,
			"$OPENPAM_MODULES_DIR",
			[OpenPAM modules directory])
		AC_MSG_RESULT($OPENPAM_MODULES_DIR)
	else
		OPENPAM_MODULES_DIR="$libdir"
		AC_MSG_RESULT(no)
	fi],
	[OPENPAM_MODULES_DIR="$libdir"
	AC_MSG_RESULT(no)])
AC_SUBST(OPENPAM_MODULES_DIR)

AC_MSG_CHECKING([whether to build example version of /bin/su])
AC_ARG_WITH(pam-su,
	AC_HELP_STRING([--with-pam-su],
		[compile example version of /bin/su]),
	,
	[with_pam_su=no])
AC_MSG_RESULT($with_pam_su)

AC_MSG_CHECKING([whether to build example version of pam_unix.so])
AC_ARG_WITH(pam-unix,
	AC_HELP_STRING([--with-pam-unix],
		[compile example version of pam_unix.so]),
	,
	[with_pam_unix=no])
AC_MSG_RESULT($with_pam_unix)

AM_CONDITIONAL(WITH_PAM_SU, test "x$with_pam_su" = "xyes")
AM_CONDITIONAL(WITH_PAM_UNIX, test "x$with_pam_unix" = "xyes")

AC_PROG_INSTALL

AC_CHECK_HEADERS(crypt.h)

AC_CHECK_FUNCS(fpurge)

DL_LIBS=
AC_CHECK_LIB(dl, dlopen, DL_LIBS=-ldl)
AC_SUBST(DL_LIBS)

CRYPT_LIBS=
AC_CHECK_LIB(crypt, crypt, CRYPT_LIBS=-lcrypt)
AC_SUBST(CRYPT_LIBS)

CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
AC_SUBST(CPPFLAGS)

CFLAGS="$CFLAGS -Werror"
AC_SUBST(CFLAGS)

AC_CONFIG_FILES([bin/Makefile
		bin/su/Makefile
		include/Makefile
		include/security/Makefile
		lib/Makefile
		modules/Makefile
		modules/pam_unix/Makefile
		modules/pam_deny/Makefile
		modules/pam_permit/Makefile
		doc/Makefile
		doc/man/Makefile
		Makefile])
AC_OUTPUT
