#!/bin/sh
# $FreeBSD: src/tools/regression/usr.bin/join/regress.t,v 1.1 2004/11/11 19:47:54 nik Exp $

cd `dirname $0`

m4 ../regress.m4 regress.sh | sh
