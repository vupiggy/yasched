/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file common.c
 * \author Luke Huang
 */

#include "kutils.h"

int
change_privilege(privilege_t pr)
{
	long	cr0;
	long	old_cr0;
	
	switch (pr) {
	case PAGE_RDONLY:
#ifdef CONFIG_X86_64
		asm volatile ("movq %%cr0, %0":"=r"(old_cr0));
		cr0 = old_cr0;
		cr0 |= ~0xfffffffffffeffff;
		asm volatile ("movq %0, %%cr0"::"r"(cr0));
		break;
#else
		asm volatile ("mov %%cr0, %0":"=r"(old_cr0));
		cr0 = old_cr0;
		cr0 |= ~0xfffeffff;
		asm volatile ("mov %0, %%cr0"::"r"(cr0));
		break;
#endif
	case PAGE_RDWR:
#ifdef CONFIG_X86_64
		asm volatile ("movq %%cr0, %0":"=r"(old_cr0));
		cr0 = old_cr0;
		cr0 &= 0xfffffffffffeffff;
		asm volatile ("movq %0, %%cr0"::"r"(cr0));
		break;
#else
		asm volatile ("mov %%cr0, %0":"=r"(old_cr0));
		cr0 = old_cr0;
		cr0 &= 0xfffeffff;
		asm volatile ("mov %0, %%cr0"::"r"(cr0));
		break;
#endif
	default:
		return -1;
	}
	return 0;
}

