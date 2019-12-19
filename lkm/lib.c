/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file lib.c
 * \author Luke Huang
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/mount.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/seq_file.h>
#include <linux/err.h>
#include <linux/kallsyms.h>

#include <asm/string.h>

int
verify_symbol(unsigned long addr, const char *symbol)
{
	return 1;
}

void *
memmem(const void		*haystack,
       size_t			 haystack_len,
       const void		*needle,
       size_t			 needle_len)
{
	const char		*begin;
	const char *const	last_possible = (const char *)haystack +
						haystack_len - needle_len;

	if (needle_len == 0)
		/* The first occurrence of the empty string is deemed to
		 * occur at the beginning of the string.
		 */
		return (void *)haystack;

	if (unlikely(haystack_len < needle_len))
		return NULL;

	for (begin = (const char *) haystack;
	     begin <= last_possible; ++begin)
		if (begin[0] ==	((const char *) needle)[0]	      &&
		    !memcmp((const void *) &begin[1],
			    (const void *) ((const char *) needle + 1),
			    needle_len - 1))
			return (void *) begin;

	return NULL;
}

struct kallsym_iter
{
        loff_t pos;
        unsigned long value;
        unsigned int nameoff; /* If iterating in core kernel symbols */
        char type;
        char name[KSYM_NAME_LEN];
        char module_name[MODULE_NAME_LEN];
        int exported;
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17))
#  define	LOCK_SEQFILE(m)		mutex_lock(&m->lock)
#  define	UNLOCK_SEQFILE(m)	mutex_unlock(&m->lock)
#else
#  define	LOCK_SEQFILE(m)		down(&m->sem)
#  define	UNLOCK_SEQFILE(m)	up(&m->sem)
#endif

#ifndef KSYM_SYMBOL_LEN
#define KSYM_SYMBOL_LEN (sizeof("%s+%#lx/%#lx [%s]") + (KSYM_NAME_LEN - 1) + \
			 2*(BITS_PER_LONG*3/10) + (MODULE_NAME_LEN - 1) + 1)
#endif

unsigned long
get_symbol_from_kallsyms(const unsigned char *symbol_str)
{
	int			 err;
	struct kallsym_iter	*iter;
	struct seq_file		*m;
	struct file		*kallsyms_file;
	loff_t			 index = 0;
	void			*old_buf;
	void			*p;
	unsigned long		 addr  = 0;

	kallsyms_file = filp_open("/proc/kallsyms", O_RDONLY, 0);
	if (IS_ERR(kallsyms_file)) {
		err = PTR_ERR(kallsyms_file);
		printk("[%s] filp_open failed %d\n", __FUNCTION__, err);
		goto out;
	}

	m = kallsyms_file->private_data;

	LOCK_SEQFILE(m);
			
	old_buf = m->buf; /* Could be NULL, we don't care */
	/* TODO: ENOMEM */
	m->buf = kmalloc(m->size = KSYM_SYMBOL_LEN, GFP_KERNEL);
	iter = m->private;
			
	while (1) {
		p = m->op->start(m, &index);
		err = PTR_ERR(p);
		if (!p || IS_ERR(p)) {
			addr = err;
			break;
		}
		m->count = 0;
		m->op->show(m, p);
		/* Terribly hackish, if the format
		 * changes, we will have to follow.
		 */
		if (!strncmp(m->buf + sizeof("%s+%#lx/%#lx [%s]") + 1,
			     symbol_str,
			     strlen(symbol_str))) {
			m->buf[2 * sizeof(void *)] = 0;
			sscanf(m->buf, "%lx", &addr);
			break;
		}
		m->op->stop(m, p);
		index++;
	}
		
	m->op->stop(m, p);
	m->count = 0;
	m->size = 0;
	kfree(m->buf);
	m->buf = old_buf;

	UNLOCK_SEQFILE(m);
	
	filp_close(kallsyms_file, NULL);

out:

	return addr;
}
