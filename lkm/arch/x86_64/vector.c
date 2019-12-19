/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file vector.c
 * \author Luke Huang
 */

#include <linux/kernel.h>

struct idtr {
	unsigned short	limit;
	unsigned long	base;
} __attribute__ ((packed));

struct idt {
	u16			offset_low;
	u16			segment;
	u16			ist : 3, zero : 5, type : 5, dpl : 2, p : 1;
	
	/* x86_64 specific */
	u16			offset_middle;
	u32			offset_high;
	u32			zero1;
} __attribute__ ((packed));
	
unsigned long
irq_vector_entry(int v)
{
	struct idtr 	 idtr;
	struct idt	*id;

	asm("sidt %0":"=m"(idtr));

	id = (struct idt *)((unsigned long)idtr.base + 16 * v);

	return (((unsigned long)id->offset_high) << 32) 	|
		(((id->offset_middle << 16) | id->offset_low) 	&
		 0x00000000ffffffff);
}

