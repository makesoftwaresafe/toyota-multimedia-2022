/*
 *  include/linux/clkdev.h
 *
 *  Copyright (C) 2008 Russell King.
 *  Some modifications (__QNXNTO__) Copyright (c) 2017 QNX Software Systems.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Helper for the clk API to assist looking up a struct clk.
 */
#ifndef __CLKDEV_H
#define __CLKDEV_H

#include <asm/clkdev.h>

struct clk;
struct device;

struct clk_lookup {
	struct list_head	node;
	const char		*dev_id;
	const char		*con_id;
	struct clk		*clk;
	struct clk_hw		*clk_hw;
};

#define CLKDEV_INIT(d, n, c)	\
	{			\
		.dev_id = d,	\
		.con_id = n,	\
		.clk = c,	\
	}

#ifndef __QNXNTO__
struct clk_lookup *clkdev_alloc(struct clk *clk, const char *con_id,
    const char *dev_fmt, ...);
#else
struct clk_lookup *clkdev_alloc(struct clk *clk, const char *con_id,
	const char *dev_fmt, ...) __printf(3, 4);
#endif

void clkdev_add(struct clk_lookup *cl);
void clkdev_drop(struct clk_lookup *cl);

#ifndef __QNXNTO__
struct clk_lookup *clkdev_create(struct clk *clk, const char *con_id,
    const char *dev_fmt, ...);
#else
struct clk_lookup *clkdev_create(struct clk *clk, const char *con_id,
	const char *dev_fmt, ...) __printf(3, 4);
#endif

void clkdev_add_table(struct clk_lookup *, size_t);
int clk_add_alias(const char *, const char *, const char *, struct device *);
void clk_drop_alias(struct clk_lookup *cl);

#ifndef __QNXNTO__
int clk_register_clkdev(struct clk *, const char *, const char *, ...);
#else
int clk_register_clkdev(struct clk *, const char *, const char *, ...)
	__printf(3, 4);
#endif
int clk_register_clkdevs(struct clk *, struct clk_lookup *, size_t);

#ifdef CONFIG_COMMON_CLK
int __clk_get(struct clk *clk);
void __clk_put(struct clk *clk);
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/clkdev.h $ $Rev: 838597 $")
#endif
