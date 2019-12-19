/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file common.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "common.h"

long
arg_to_ulong(char *arg)
{
	long	 lval;
	char	*endptr;

	lval = strtol(arg, &endptr, 0);
	
	if ((errno != 0 && lval == 0) ||
	    (errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN))) {
		perror("strtol");
		return -1;
	}

	if (endptr == arg) {
		fprintf(stderr, "No digits were found as lvalrity argument\n");
		return -1;
	}

	if (*endptr != '\0') {
		fprintf(stderr, "Argument contains non-digit\n");
		return -1;
	}	

	return lval;
}
