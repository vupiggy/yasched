/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file opts.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opts.h"

static int
comp_subcmd(const void *s1, const void *s2)
{
	subcmd_map_t *si1 = (subcmd_map_t *)s1;
	subcmd_map_t *si2 = (subcmd_map_t *)s2;
	
	return strcmp(si1->str, si2->str);
}

/**
 * @subcmds should be an array
 */
subcmd_map_t *
search_subcmd(subcmd_map_t	*subcmds,
	      unsigned long	 nmem,
	      const char	*str)
{
	subcmd_map_t key, *res;

	key.str = malloc(strlen(str) + 1);
	strcpy(key.str, str);
	
	qsort(subcmds, nmem, size_of_subcmd(), comp_subcmd);
	
	res = bsearch(&key, subcmds, nmem, size_of_subcmd(), comp_subcmd);

	free(key.str);

	return res;
}

