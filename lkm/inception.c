/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file inception.c
 * \author Christopher Nolan ;) (Just Kidding! Actually I am
 *                               NOT a big fan of his movies)
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>

#include <asm/atomic.h>
#include <asm/ptrace.h>

atomic64_t		ya_count;

extern int		switch_syscalls(void);
extern void		restore_syscalls(void);

extern int		switch_pgfault_handler(unsigned long agent);
extern void		restore_pgfault_handler(void);

extern int		switch_smp_apic_timer_interrupt(unsigned long agent);
extern void		restore_smp_apic_timer_interrupt(void);

extern int		switch_schedule_tail(unsigned long agent);
extern void		restore_schedule_tail(void);

extern int		setup_enter_schedule_hook(unsigned long agent);
extern void		remove_enter_schedule_hook(void);
extern int		setup_leave_schedule_hook(unsigned long agent);
extern void		remove_leave_schedule_hook(void);

extern int		switch_schedule_call(unsigned long agent);
extern void		restore_schedule_call(void);

extern int		setup_enter_exit_hook(unsigned long agent);
extern void		remove_enter_exit_hook(void);

extern void		sat_interrupt_agent(struct pt_regs *regs);
extern void		schedule_call_agent(void);
extern asmlinkage void	pgfault_agent(struct pt_regs	*pt_regs,
				     unsigned long	 error_code);
extern void		schedule_tail_agent(struct task_struct *prev);
extern void __sched	enter_schedule_hook(void);
extern void __sched	leave_schedule_hook(void);
extern void		enter_exit_hook(void);

extern long		do_fork_agent(unsigned long	 clone_flags,
				      unsigned long	 stack_start,
				      struct pt_regs	*regs,
				      unsigned long	 stack_size,
				      int __user	*parent_tidptr,
				      int __user	*child_tidptr);
extern int		switch_do_fork(unsigned long agent);
extern void		restore_do_fork(void);

int
perform_inception(void)
{
	atomic64_set(&ya_count, 0);

	if (switch_syscalls() != 0)
		goto syscalls_err;

	if (switch_pgfault_handler((unsigned long)&pgfault_agent) != 0)
		goto pgfault_err;

	if (switch_schedule_call((unsigned long)&schedule_call_agent))
		goto sched_err;

	if (switch_smp_apic_timer_interrupt((unsigned long)&sat_interrupt_agent))
		goto timer_err;

	if (switch_schedule_tail((unsigned long)schedule_tail_agent))
		goto sched_tail_err;

	if (setup_enter_schedule_hook((unsigned long)&enter_schedule_hook))
		goto enter_sched_err;

	if (setup_leave_schedule_hook((unsigned long)&leave_schedule_hook))
		goto leave_sched_err;

	if (setup_enter_exit_hook((unsigned long)&enter_exit_hook))
		goto exit_err;

	if (switch_do_fork((unsigned long)&do_fork_agent))
		goto fork_err;

	return 0;

fork_err:
	remove_enter_exit_hook();

exit_err:
	remove_leave_schedule_hook();

leave_sched_err:
	remove_enter_schedule_hook();

enter_sched_err:
	restore_schedule_tail();

sched_tail_err:
	restore_smp_apic_timer_interrupt();

timer_err:
	restore_schedule_call();


sched_err:
	restore_pgfault_handler();


pgfault_err:
	restore_syscalls();

syscalls_err:
	return -EINVAL;
}

void
remove_inception(void)
{
	long long c;

	restore_do_fork();
	remove_enter_exit_hook();
	remove_leave_schedule_hook();
	remove_enter_schedule_hook();
	restore_schedule_tail();
	restore_smp_apic_timer_interrupt();
	restore_schedule_call();
	restore_pgfault_handler();
	restore_syscalls();

	while((c = atomic64_read(&ya_count))) {
		printk("Waiting for the usage count down to zero: %lld\n", c);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ << 1);
	}
	schedule();
}
