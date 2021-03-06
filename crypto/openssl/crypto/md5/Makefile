#
# SSLeay/crypto/md5/Makefile
#

DIR=    md5
TOP=    ../..
CC=     cc
CPP=    $(CC) -E
INCLUDES=-I.. -I$(TOP) -I../../include
CFLAG=-g
INSTALL_PREFIX=
OPENSSLDIR=     /usr/local/ssl
INSTALLTOP=/usr/local/ssl
MAKEDEPPROG=	makedepend
MAKEDEPEND=	$(TOP)/util/domd $(TOP) -MD $(MAKEDEPPROG)
MAKEFILE=       Makefile
AR=             ar r

MD5_ASM_OBJ=

CFLAGS= $(INCLUDES) $(CFLAG)
ASFLAGS= $(INCLUDES) $(ASFLAG)

GENERAL=Makefile
TEST=md5test.c
APPS=

LIB=$(TOP)/libcrypto.a
LIBSRC=md5_dgst.c md5_one.c
LIBOBJ=md5_dgst.o md5_one.o $(MD5_ASM_OBJ)

SRC= $(LIBSRC)

EXHEADER= md5.h
HEADER= md5_locl.h $(EXHEADER)

ALL=    $(GENERAL) $(SRC) $(HEADER)

top:
	(cd ../..; $(MAKE) DIRS=crypto SDIRS=$(DIR) sub_all)

all:    lib

lib:    $(LIBOBJ)
	$(AR) $(LIB) $(LIBOBJ)
	$(RANLIB) $(LIB) || echo Never mind.
	@touch lib

# elf
asm/mx86-elf.s: asm/md5-586.pl ../perlasm/x86asm.pl
	(cd asm; $(PERL) md5-586.pl elf $(CFLAGS) > mx86-elf.s)

# a.out
asm/mx86-out.o: asm/mx86unix.cpp
	$(CPP) -DOUT asm/mx86unix.cpp | as -o asm/mx86-out.o

# bsdi
asm/mx86bsdi.o: asm/mx86unix.cpp
	$(CPP) -DBSDI asm/mx86unix.cpp | sed 's/ :/:/' | as -o asm/mx86bsdi.o

asm/mx86unix.cpp: asm/md5-586.pl ../perlasm/x86asm.pl
	(cd asm; $(PERL) md5-586.pl cpp >mx86unix.cpp)

asm/md5-sparcv8plus.o: asm/md5-sparcv9.S
	$(CC) $(ASFLAGS) -DMD5_BLOCK_DATA_ORDER -c \
		-o asm/md5-sparcv8plus.o asm/md5-sparcv9.S

# Old GNU assembler doesn't understand V9 instructions, so we
# hire /usr/ccs/bin/as to do the job. Note that option is called
# *-gcc27, but even gcc 2>=8 users may experience similar problem
# if they didn't bother to upgrade GNU assembler. Such users should
# not choose this option, but be adviced to *remove* GNU assembler
# or upgrade it.
asm/md5-sparcv8plus-gcc27.o: asm/md5-sparcv9.S
	$(CC) $(ASFLAGS) -DMD5_BLOCK_DATA_ORDER -E asm/md5-sparcv9.S | \
		/usr/ccs/bin/as -xarch=v8plus - -o asm/md5-sparcv8plus-gcc27.o

asm/md5-sparcv9.o: asm/md5-sparcv9.S
	$(CC) $(ASFLAGS) -DMD5_BLOCK_DATA_ORDER -c \
		-o asm/md5-sparcv9.o asm/md5-sparcv9.S

files:
	$(PERL) $(TOP)/util/files.pl Makefile >> $(TOP)/MINFO

links:
	@$(PERL) $(TOP)/util/mklink.pl ../../include/openssl $(EXHEADER)
	@$(PERL) $(TOP)/util/mklink.pl ../../test $(TEST)
	@$(PERL) $(TOP)/util/mklink.pl ../../apps $(APPS)

install:
	@for i in $(EXHEADER) ; \
	do  \
	(cp $$i $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i; \
	chmod 644 $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i ); \
	done;

tags:
	ctags $(SRC)

tests:

lint:
	lint -DLINT $(INCLUDES) $(SRC)>fluff

depend:
	$(MAKEDEPEND) -- $(CFLAG) $(INCLUDES) $(DEPFLAG) -- $(PROGS) $(LIBSRC)

dclean:
	$(PERL) -pe 'if (/^# DO NOT DELETE THIS LINE/) {print; exit(0);}' $(MAKEFILE) >Makefile.new
	mv -f Makefile.new $(MAKEFILE)

clean:
	rm -f asm/mx86unix.cpp asm/*-elf.* *.o asm/*.o *.obj lib tags core .pure .nfs* *.old *.bak fluff

# DO NOT DELETE THIS LINE -- make depend depends on it.

md5_dgst.o: ../../include/openssl/bio.h ../../include/openssl/crypto.h
md5_dgst.o: ../../include/openssl/e_os2.h ../../include/openssl/err.h
md5_dgst.o: ../../include/openssl/fips.h ../../include/openssl/lhash.h
md5_dgst.o: ../../include/openssl/md5.h ../../include/openssl/opensslconf.h
md5_dgst.o: ../../include/openssl/opensslv.h ../../include/openssl/safestack.h
md5_dgst.o: ../../include/openssl/stack.h ../../include/openssl/symhacks.h
md5_dgst.o: ../md32_common.h md5_dgst.c md5_locl.h
md5_one.o: ../../include/openssl/crypto.h ../../include/openssl/e_os2.h
md5_one.o: ../../include/openssl/md5.h ../../include/openssl/opensslconf.h
md5_one.o: ../../include/openssl/opensslv.h ../../include/openssl/safestack.h
md5_one.o: ../../include/openssl/stack.h ../../include/openssl/symhacks.h
md5_one.o: md5_one.c
