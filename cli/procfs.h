/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file procfs.h
 * \author Luke Huang
 */

#ifndef _YAS_PROCFS_H_
#define _YAS_PROCFS_H_

int	list_zones(int argc, char **argv);
int	list_jobs(int argc, char **argv);
int	list_threads(int argc, char **argv);
int	list_all(int argc, char **argv);

#endif /* _YAS_PROCFS_H_ */

