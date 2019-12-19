/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file procfs.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "opts.h"
#include "common.h"

#define YA_PROCFS_PATH	"/proc/yasched"

static struct option list_zones_opts[] =
{
	{ "help",	0,	0,	'h' },
	
	{0, 0, 0, 0}
};
static const char *list_zones_short_opts = "h";

static int
parse_list_zones_argv(int argc, char **argv)
{
	int	c;
	int	opt_index = 0;

	for(;;) {
		c = getopt_long(argc, argv, list_zones_short_opts,
				list_zones_opts, &opt_index);

		if (c == -1)
			break;

		switch(c) {
		case 'h':
			/* output the help msg for listzone cmd */
			exit(0);
		default:
			/* output the help msg for listzone cmd */
			return EINVAL;
		}
	}

	return 0;
}

int
list_zones(int argc, char **argv)
{
	DIR		*dp;
	struct dirent	*dirp;
	
	if (parse_list_zones_argv(argc, argv))
		return EINVAL;

	if ((dp = opendir(YA_PROCFS_PATH)) == NULL) {
		perror("opendir");
		return errno;
	}
	
	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_type == DT_DIR &&
		    !strncmp(dirp->d_name, "zone-", 5))
			printf("%s\n", &dirp->d_name[5]);
	}

	return 0;
}

#if 0
static struct option list_jobs_opts[] =
{
	{ "help",	0,	0,	'h' },
	{ "zone",	0,	0,	'z' },
	
	{0, 0, 0, 0}
};
static const char *list_job_short_opts = "hz:";
#endif
int
list_jobs(int argc, char **argv)
{
	return 0;
}

#if 0
static struct option list_threads_opts[] =
{
	{ "help",	0,	0,	'h' },
	{ "zone",	0,	0,	'z' },
	{ "job",	0,	0,	'j' },
	
	{0, 0, 0, 0}
};
static const char *list_thread_short_opts = "hz:j:";
#endif
int
list_threads(int argc, char **argv)
{
	return 0;
}

int
list_all(int argc, char **argv)
{
	return 0;
}
