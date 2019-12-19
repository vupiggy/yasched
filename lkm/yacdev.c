/* -*- mode: C; c-file-style: "Linux" -*-
 *
 * Copyright (c) 2018 Luke Huang <lukehuang.ca@gmail.com>
 */

/**
 * \file yacdev.c
 * \author Luke Huang
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

static int	yacdev_major_devno;
static int	yacdev_minor_devno = 0;
static int	yacdev_devs_num	   = 1;

typedef struct {
	struct cdev	cdev;
} yacdev_t;
static yacdev_t yacdev;

extern long yacdev_ioctl(struct file	*filp,
                         unsigned int	 cmd,
                         unsigned long	 arg);

static const struct file_operations yacdev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = yacdev_ioctl,
};

int
create_char_device(void)
{
	int	res;
	dev_t	dev = 0;

	res = alloc_chrdev_region(&dev, yacdev_minor_devno,
				  yacdev_devs_num, "yacdev");
	if (res < 0) {
		printk("[%s-%d] Couldn't allocate character device region.\n",
		       __FUNCTION__, __LINE__);
		return res;
	}

	yacdev_major_devno = MAJOR(dev);
	dev = MKDEV(yacdev_major_devno, yacdev_minor_devno);
	cdev_init(&yacdev.cdev, &yacdev_fops);
	yacdev.cdev.owner = THIS_MODULE;

	if ((res = cdev_add(&yacdev.cdev, dev, 1)) < 0) {
		printk("[%s-%d] Couldn't add character device\n",
		       __FUNCTION__, __LINE__);
		return res;
	}

	return 0;
}

void
destroy_char_device(void)
{
	dev_t dev = MKDEV(yacdev_major_devno, yacdev_minor_devno);

	cdev_del (&yacdev.cdev);
	unregister_chrdev_region(dev, yacdev_devs_num);
}
