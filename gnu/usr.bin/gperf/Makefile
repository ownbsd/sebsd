# $FreeBSD: src/gnu/usr.bin/gperf/Makefile,v 1.11 2004/02/17 01:49:20 ache Exp $

GPROFDIR=	${.CURDIR}/../../../contrib/gperf

.PATH: ${GPROFDIR}/src ${GPROFDIR}/lib ${GPROFDIR}/doc

SUBDIR=	doc

PROG_CXX=	gperf
SRCS=	bool-array.cc gen-perf.cc hash-table.cc iterator.cc key-list.cc \
	list-node.cc main.cc new.cc options.cc read-line.cc trace.cc \
	vectors.cc version.cc
SRCS+=  hash.cc

CXXFLAGS+=	-I${GPROFDIR}/lib -I${.CURDIR}

.include <bsd.prog.mk>
