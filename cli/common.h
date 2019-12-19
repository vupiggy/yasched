/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file common.h
 * \author Luke Huang
 */

#ifndef _YAS_COMMON_H_
#define _YAS_COMMON_H_

#include <stdio.h>

static inline void
usage(const char *msg)
{
	fprintf(stderr, "%s", msg);
}

long arg_to_ulong(char *arg);

#endif /* _YAS_COMMON_H_ */

