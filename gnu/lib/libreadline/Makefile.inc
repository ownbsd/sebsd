# $FreeBSD: src/gnu/lib/libreadline/Makefile.inc,v 1.12 2005/07/22 17:18:58 kensmith Exp $

SHLIB_MAJOR=6
RL_LIBRARY_VERSION=5.0

SRCDIR= ${.CURDIR}/../../../../contrib/libreadline
.PATH: ${SRCDIR} ${.CURDIR}/..

LOCAL_CFLAGS= -DRL_LIBRARY_VERSION='"$(RL_LIBRARY_VERSION)"'
CFLAGS+=-I${.CURDIR}/.. -I${SRCDIR} -DHAVE_CONFIG_H ${LOCAL_CFLAGS}

HISTSRC=history.c histexpand.c histfile.c histsearch.c shell.c mbutil.c

.include "../Makefile.inc"
