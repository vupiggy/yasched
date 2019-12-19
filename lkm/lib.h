/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file lib.h
 * \author Luke Huang
 */

#ifndef _YAS_LIB_H_
#define _YAS_LIB_H_
	
int verify_symbol(unsigned long addr, const char *symbol);

void *memmem(const void		*haystack,
	     size_t	 	 haystack_len,
	     const void		*needle,
	     size_t	 	 needle_len);

unsigned long get_symbol_from_kallsyms(const unsigned char *symbol_str);

#endif /* _YAS_LIB_H_ */

