# $Id$

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(AFS_EXTRA_DEFS) $(ROKEN_RENAME)

if KRB4
DEPLIB_krb4 = $(LIB_krb4) $(LIB_des)
krb4_am_workaround = $(INCLUDE_krb4)
else
DEPLIB_krb4  =
krb4_am_workaround = 
endif # KRB4
INCLUDES += $(krb4_am_workaround)

if KRB5
DEPLIB_krb5 = ../krb5/libkrb5.la 
krb5_am_workaround = $(INCLUDE_des) -I$(top_srcdir)/lib/krb5
else
DEPLIB_krb5  =
krb5_am_workaround = 
endif # KRB5
INCLUDES += $(krb5_am_workaround)


if AIX
AFSL_EXP = $(srcdir)/afsl.exp

if AIX4
AFS_EXTRA_LD = -bnoentry
else
AFS_EXTRA_LD = -e _nostart
endif

if AIX_DYNAMIC_AFS
if HAVE_DLOPEN
AIX_SRC = 
else
AIX_SRC = dlfcn.c
endif
AFS_EXTRA_LIBS = afslib.so
AFS_EXTRA_DEFS =
else
AIX_SRC = afslib.c
AFS_EXTRA_LIBS = 
AFS_EXTRA_DEFS = -DSTATIC_AFS
endif

else
AFSL_EXP =
AIX_SRC =
endif # AIX

libkafs_la_LIBADD = $(DEPLIB_krb5) ../roken/libroken.la $(DEPLIB_krb4)

lib_LTLIBRARIES = libkafs.la
libkafs_la_LDFLAGS = -version-info 4:0:4
foodir = $(libdir)
foo_DATA = $(AFS_EXTRA_LIBS)
# EXTRA_DATA = afslib.so

CLEANFILES= $(AFS_EXTRA_LIBS) $(ROKEN_SRCS)

include_HEADERS = kafs.h

if KRB5
afskrb5_c = afskrb5.c
endif

if KRB4
afskrb_c = afskrb.c
endif


if do_roken_rename
ROKEN_SRCS = resolve.c strtok_r.c strlcpy.c strsep.c
endif

libkafs_la_SOURCES =				\
	afssys.c				\
	$(afskrb_c)				\
	$(afskrb5_c)				\
	common.c				\
	$(AIX_SRC)				\
	kafs_locl.h				\
	afssysdefs.h				\
	$(ROKEN_SRCS)

#afslib_so_SOURCES = afslib.c

EXTRA_libkafs_la_SOURCES = afskrb.c afskrb5.c dlfcn.c afslib.c dlfcn.h

EXTRA_DIST = README.dlfcn afsl.exp afslib.exp

man_MANS = kafs.3

# AIX: this almost works with gcc, but somehow it fails to use the
# correct ld, use ld instead
afslib.so: afslib.o
	ld -o $@ -bM:SRE -bI:$(srcdir)/afsl.exp -bE:$(srcdir)/afslib.exp $(AFS_EXTRA_LD) afslib.o -lc

$(OBJECTS): ../../include/config.h

resolve.c:
	$(LN_S) $(srcdir)/../roken/resolve.c .

strtok_r.c:
	$(LN_S) $(srcdir)/../roken/strtok_r.c .

strlcpy.c:
	$(LN_S) $(srcdir)/../roken/strlcpy.c .

strsep.c:
	$(LN_S) $(srcdir)/../roken/strsep.c .
