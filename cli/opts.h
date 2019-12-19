/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file opts.h
 * \author Luke Huang
 */

#ifndef _YAS_OPTS_H_
#define _YAS_OPTS_H_

#include <getopt.h>

typedef int (*subcmd_func)(int argc, char **argv);

typedef struct subcmd_map {
	char		*str;
	subcmd_func 	 func;
} subcmd_map_t;

#define nr_of_subcmds(subcmds)	(sizeof(subcmds) / sizeof(subcmds[0]))
#define size_of_subcmd()	(sizeof(subcmd_map_t))

subcmd_map_t *search_subcmd(subcmd_map_t	*subcmds,
			    unsigned long	 nmem,
			    const char		*str);

#endif /* _YAS_OPTS_H_ */

