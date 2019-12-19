/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file yasadmin.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/ioctl.h>

#include "opts.h"
#include "msg.h"
#include "common.h"
#include "ioctl.h"
#include "procfs.h"
#include "yash.h"

/* Job operations */
static int suspend_job(int argc, char **argv);
static int resume_job(int argc, char **argv);
static int kill_job(int argc, char **argv);

static struct option job_op_options[] =
{
	{ "help",	0,	0,	'h' },
	{ "zone",	1,	0,	'z' },
	{ "job",	1,	0,	'j' },
	
	{0, 0, 0, 0}
};

static const char *job_op_short_opts = "hz:j:";

static subcmd_map_t subcmds[] = {
	{ "kill",	 kill_job     },
	{ "listall",	 list_all     },
	{ "listjobs",	 list_jobs    },
	{ "listthreads", list_threads },
	{ "listzones",	 list_zones   },
	{ "resume",	 resume_job   },
	{ "shell",	 start_yash   },
	{ "suspend",	 suspend_job  }
};

const char *MSG_YA_ADMIN_USAGE;
const char *MSG_JOB_OP_USAGE[];

#define	ZID_SET	1
#define	JID_SET	2
#define ALL_SET	(ZID_SET | JID_SET)

static int
parse_jobop_argv(int argc, char **argv, ya_job_op_t *jop)
{
	int	c;
	int	opt_index = 0;
	long	lval;
	int	opt_set	  = 0;
	
	while(1) {
		c = getopt_long(argc, argv, job_op_short_opts,
				job_op_options, &opt_index);

		if (c == -1)
			break;

		switch (c) {
		case 'z':
			if (!optarg)
				return EINVAL;
			
			lval = arg_to_ulong(optarg);
			if (lval == -1)
				return EINVAL;

			jop->zone_id = (unsigned long)lval;
			opt_set |= ZID_SET;
			break;
		case 'j':
			if (!optarg)
				return EINVAL;
			
			lval = arg_to_ulong(optarg);
			if (lval == -1)
				return EINVAL;
			
			jop->job_id = (unsigned long)lval;
			opt_set |= JID_SET;
			break;

		case 'h':
			usage(MSG_JOB_OP_USAGE[jop->op]);
			exit(0);
			
		case '?':
		default:
			return EINVAL;
		}
	}

	if ((opt_set & ALL_SET) != ALL_SET)
		return EINVAL;

	return 0;
}

static int
suspend_job(int argc, char **argv)
{
	ya_job_op_t jop;

	jop.op = SUSPEND_JOB;
	if (parse_jobop_argv(argc, argv, &jop))
		return EINVAL;

	return send_ctl(YA_IOCTL_SUSPEND_JOB, &jop);
}

static int
resume_job(int argc, char **argv)
{
	ya_job_op_t jop;

	jop.op = RESUME_JOB;
	if (parse_jobop_argv(argc, argv, &jop))
		return EINVAL;

	return send_ctl(YA_IOCTL_RESUME_JOB, &jop);
}

static int
kill_job(int argc, char **argv)
{
	ya_job_op_t jop;

	jop.op = KILL_JOB;
	if (parse_jobop_argv(argc, argv, &jop))
		return EINVAL;

	return send_ctl(YA_IOCTL_KILL_JOB, &jop);
}

int
main(int argc, char **argv)
{
	subcmd_map_t	*subcmd;
	int		 ret;
	
	if (argc < 2) {
		usage(MSG_YA_ADMIN_USAGE);
		return EINVAL;
	}

	argv++; argc--;
	subcmd = search_subcmd(subcmds, nr_of_subcmds(subcmds), argv[0]);

	if (!subcmd) {
		usage(MSG_YA_ADMIN_USAGE);
		return EINVAL;
	}

	ret = subcmd->func(argc, argv);
	if (ret) {
		usage(MSG_YA_ADMIN_USAGE);
		return ret;
	}

	return 0;
}

const char *MSG_YA_ADMIN_USAGE = {
	"usage: yasadmin <subcommand> [options] [arguments]\n"
	"YASched admininstration client, version 1.0.0\n"
	"Type 'yasadmin <subcommand> --help' for help on a specific "
	"subcommand.\n\n"
	"Available subcommands:\n"
	"    kill\n"
	"    suspend\n"
	"    resume\n"
};

const char *MSG_JOB_OP_USAGE[] = {
	"usage: yasadmin kill -z zid -j jid\n"
	"       or\n"
	"       yasadmin kill --zone zid --job jid\n",

	"usage: yasadmin suspend -z zid -j jid\n"
	"       or\n"
	"       yasadmin suspend --zone zid --job jid\n",

	"usage: yasadmin resume -z zid -j jid\n"
	"       or\n"
	"       yasadmin resume --zone zid --job jid\n"
};
