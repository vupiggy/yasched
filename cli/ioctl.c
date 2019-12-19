/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 *
 * $Id$
 */

/**
 * \file ioctl.c
 * \author Luke Huang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/ioctl.h>

#include "ioctl.h"

int
send_ctl(unsigned int req, void *data)
{
	int fd = open(yacdev_control, O_WRONLY);

	if (fd == -1) {
		perror("open");
		return errno;
	}

	if (-1 == ioctl(fd, req, data)) {
		perror("ioctl");
		return errno;
	}

	close(fd);

	return 0;
}
