/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file yasub.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char *MSG_YA_SUB_USAGE;

int
main(int argc, char **argv)
{
	int ret;

	/* TODO: get zone id from CLI argument! xxx luke xxx */
	setenv("YA_SCHED_ZONE_ID", "0" ,1);

	ret = execvp(argv[1], &argv[1]);

	/* TODO: check errno and output some useful msg */

	return ret;
}


const char *MSG_YA_SUB_USAGE = {
	"usage: yasub <executable>\n"
};
