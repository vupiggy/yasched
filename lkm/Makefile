### Makefile ---

## Author: <lukehuang.ca@gmail.com>

ifneq ($(KERNELRELEASE),)	# Kernel's building system
obj-m := yasched.o
yasched-objs :=						\
	main.o			lib.o			\
	yacdev.o		inception.o		\
							\
	hook/timer.o		hook/page_fault.o	\
	hook/execve.o		hook/sched.o		\
	hook/exit.o		hook/fork.o		\
							\
	user/ioctl.o		user/procfs.o		\
							\
	mgmt/job.o					\
							\
	sched/sched.o					\

ifeq ($(ARCH),)
	ARCH=$(shell uname -m)
endif

ARCHDIR=arch/$(ARCH)
yasched-objs += $(ARCHDIR)/syscalls.o	${ARCHDIR}/page_fault.o	\
		$(ARCHDIR)/apic_timer.o	$(ARCHDIR)/vector.o	\
		$(ARCHDIR)/kutils.o	$(ARCHDIR)/sched.o	\
		$(ARCHDIR)/exit.o	$(ARCHDIR)/fork.o

EXTRA_CFLAGS += -I$(PWD) -I$(PWD)/../include -I$(PWD)/arch/$(shell uname -m)
# EXTRA_CFLAGS += -DINCEPTION_DEBUG=1

else		# !KERNELRELEASE (i.e, directory of lkm)

KBUILD_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
export PWD

default:
	@echo "  MK -C   $(KBUILD_DIR) modules"
	@$(MAKE) -C $(KBUILD_DIR) M=$(PWD) modules

clean:
	@echo "  RM 	*.o *~ core .depend .*.cmd *.ko *.mod.c tmp_versions Module.symvers modules.order"
	@$(RM) -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers modules.order *.unsigned
	@find . -name "*.o" -exec rm -f {} \;
	@find . -name "*.cmd" -exec rm -f {} \;

endif		# ifneq KERNELRELEASE

### Makefile ends here
