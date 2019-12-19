/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file exit.c
 * \author Luke Huang
 */

#include <linux/module.h>

#include "sched/sched.h"

extern atomic64_t		ya_count;
#define ENTER_EXIT_YA_COUNT	787

void
enter_exit_hook(void)
{
	ya_thread_t *thread;

	atomic64_add(ENTER_EXIT_YA_COUNT, &ya_count);
	thread = search_thread_by_task(current);

	/* NULL thread can be because:
	 *
	 * 1. current is not a yasched job
	 *
	 * 2. current is  a yasched job but execve()  call always failed
	 *    thus the thread entry has not been allocated yet.
	 */
	if (thread) {
		ya_del_thread(thread);
        }
	atomic64_sub(ENTER_EXIT_YA_COUNT, &ya_count);
}
