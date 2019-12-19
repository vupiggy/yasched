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

#include <linux/module.h>
#include <asm/compat.h>

#define MAX_OFFSET		512	/* For finger-print search, give
					 * up if reaching the maximum.
					 */

#define volatile_uint(addr) ((volatile unsigned int *)(addr))
#define volatile_long(addr) ((volatile long *)(addr))

/* From  Intel's document, the  lock is  automatically implemented  if a
 * memory operand is referenced thus a lock prefix isn't necessary.
 */
#define __ya_xchgl(addr, val)						\
	do {								\
		change_privilege(PAGE_RDWR);				\
		asm volatile ("xchgl %k0, %1"				\
			      :"=r"(val)				\
			      :"m"(*volatile_uint(addr)), "0"(val)	\
		     	      :"memory");				\
		change_privilege(PAGE_RDONLY);				\
	} while(0)

#define __ya_xchgq(addr, val)						\
	do {								\
		change_privilege(PAGE_RDWR);				\
		asm volatile ("xchgq %0, %1"				\
			      :"=r"(val)				\
			      :"m"(*volatile_long(addr)), "0"(val)	\
		     	      :"memory");				\
		change_privilege(PAGE_RDONLY);				\
	} while(0)

typedef enum {
	PAGE_RDONLY = 0,
	PAGE_RDWR,
} privilege_t;

int change_privilege(privilege_t pr);

static inline unsigned long
__save_fault_addr(void)
{
        unsigned long __force_order;
	unsigned long addr;
	asm("movq %%cr2, %0":"=r"(addr), "=m"(__force_order));
	return addr;
}

static inline void print_hex_256(unsigned char *p)
{
        int i;
        unsigned char str[256] = {0};

        for(i = 0; i < sizeof(str); p++) {
                if (i != 0)
                        str[i++] = ' ';
                snprintf(&str[i], sizeof(str) - i, "%02x", *p);
                i += 2;
        }

        printk(KERN_WARNING "<%s>\n", str);
}

#if 0	// __force_order couldn't be defined as ``static'' anymore.  But
        // luckily nobody calls it,
static inline void
__load_fault_addr(unsigned long addr)
{
	asm("movq %0, %%cr2"::"r"(addr), "m"(__force_order));
}
#endif

struct user_arg_ptr {
#ifdef CONFIG_COMPAT
        bool is_compat;
#endif
        union {
                const char __user *const __user *native;
#ifdef CONFIG_COMPAT
                const compat_uptr_t __user *compat;
#endif
        } ptr;
};

#endif /* _YAS_COMMON_H_ */
