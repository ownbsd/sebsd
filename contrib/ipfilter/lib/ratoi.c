/*	$FreeBSD: src/contrib/ipfilter/lib/ratoi.c,v 1.3 2005/12/30 11:52:24 guido Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id$
 */

#include "ipf.h"


int	ratoi(ps, pi, min, max)
char 	*ps;
int	*pi, min, max;
{
	int i;
	char *pe;

	i = (int)strtol(ps, &pe, 0);
	if (*pe != '\0' || i < min || i > max)
		return 0;
	*pi = i;
	return 1;
}
