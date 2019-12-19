/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file exit.c
 * \author Luke Huang
 */

#include <linux/module.h>

#include "lib.h"
#include "kutils.h"

static const int      num_of_nop = 64;
static volatile unsigned long  enter_hook;
static unsigned long  jmp_do_exit_hex;
static unsigned long *do_exit_start;
unsigned long         do_exit_reenter_point;
unsigned long	      kernel_do_exit;
extern		      void enter_exit_hook(void);
void		      enter_exit(void);
void		      jmp_do_exit(void);

asm (
	"      .text                               	\n"
	"      .globl enter_exit			\n"
	"  enter_exit:					\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"

        // TODO: in 3.10, this is %rdi, kernel version and compiler dependence, how to fix it?
        "      push	%rbx				\n"
        "      push	%rdi				\n"
	"      call     *enter_hook			\n"
        "      pop	%rdi				\n"
        "      pop	%rbx				\n"
	"      jmpq	*do_exit_reenter_point		\n"
	"						\n"
	"      .globl jmp_do_exit			\n"
	"  jmp_do_exit:					\n"
	"      nop; nop; nop; nop; nop; nop; nop; nop;	\n"
);

int
setup_enter_exit_hook(unsigned long hook)
{
	unsigned char *from;
	unsigned char *to;
	unsigned char *c;
	int	       i = 0;

	from = (unsigned char *)get_symbol_from_kallsyms("do_exit");
        kernel_do_exit = (unsigned long)from;

	if (unlikely(!from))
                goto err;

	to = (unsigned char *)&enter_exit;

	for (c = from; i < num_of_nop; i++, c++) {
                // e.g., 48 83 40	sub $0x40,%rsp
		if (*c == 0x48 && *(c + 1) == 0x83 && *(c + 2) == 0xec) {
                        change_privilege(PAGE_RDWR);
			memcpy(to, from, c - from + 4);
                        // TODO: xxx luke xxx, the number 16 depends on the instructions, how to avoid?
			memcpy(to + (c - from + 4), to + num_of_nop, 18);
                        change_privilege(PAGE_RDONLY);
			break;
		}
	}

	if (*to == 0x90) {
		printk("[%s] signature not found!\n", __FUNCTION__);
                goto err;
	}

        enter_hook	      = hook;
	do_exit_reenter_point = (unsigned long)c + 4;
	jmp_do_exit_hex	      = *(unsigned long *)&jmp_do_exit;
        // TODO: eb? e8? or ff 24 25? how to avoid assembler dependent issue? xxx luke xxx
        *(unsigned char *)&jmp_do_exit_hex = 0xe9;
	*(unsigned int *)((unsigned long)&jmp_do_exit_hex + 1L) =
		(unsigned int)((unsigned long)&enter_exit - (unsigned long)from - 5UL);
	do_exit_start	      = (unsigned long *)from;
        __ya_xchgq(do_exit_start, jmp_do_exit_hex);

	return 0;

err:
        return -ENOSYS;
}

void
remove_enter_exit_hook(void)
{
	if (likely(do_exit_start))
		__ya_xchgq(do_exit_start, jmp_do_exit_hex);
}
