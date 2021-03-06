/*	$FreeBSD: src/contrib/ipfilter/lib/make_range.c,v 1.3 2005/12/30 11:52:24 guido Exp $	*/

/*
 * Copyright (C) 2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id$
 */
#include "ipf.h"


alist_t	*make_range(not, a1, a2)
int not;
struct in_addr a1, a2;
{
	alist_t *a;

	a = (alist_t *)calloc(1, sizeof(*a));
	if (a != NULL) {
		a->al_1 = a1.s_addr;
		a->al_2 = a2.s_addr;
		a->al_not = not;
	}
	return a;
}
