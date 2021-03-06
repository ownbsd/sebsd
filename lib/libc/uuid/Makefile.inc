# $FreeBSD: src/lib/libc/uuid/Makefile.inc,v 1.4 2006/03/13 01:15:01 deischen Exp $

# DCE 1.1 UUID implementation sources

.PATH: ${.CURDIR}/uuid

SRCS+=	uuid_compare.c uuid_create.c uuid_create_nil.c uuid_equal.c \
	uuid_from_string.c uuid_hash.c uuid_is_nil.c uuid_to_string.c
SYM_MAPS+=	${.CURDIR}/uuid/Symbol.map

MAN+=	uuid.3
MLINKS+=uuid.3 uuid_compare.3
MLINKS+=uuid.3 uuid_create.3
MLINKS+=uuid.3 uuid_create_nil.3
MLINKS+=uuid.3 uuid_equal.3
MLINKS+=uuid.3 uuid_from_string.3
MLINKS+=uuid.3 uuid_hash.3
MLINKS+=uuid.3 uuid_is_nil.3
MLINKS+=uuid.3 uuid_to_string.3
