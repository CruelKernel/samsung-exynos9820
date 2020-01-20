/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <asm/setup.h>
#include <asm/memblock.h>
#include <linux/trace_seq.h>
#include <linux/seq_file.h>
#include <linux/init.h>

unsigned long get_ddr_size(void)
{
	unsigned int i;
	unsigned long ret = 0;

	for(i=0; i< meminfo.nr_banks; i++)
		ret += meminfo.bank[i].size;

	return ret;
}
static int sec_ddrsize_proc_show(struct seq_file *m, void *v)
{
	unsigned int total_size = get_ddr_size();
	seq_printf(m, "%d\n", total_size >> (20));
	return 0;
}

static int sec_ddrsize_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_ddrsize_proc_show, NULL);
}

static const struct file_operations sec_ddrsize_proc_fops = {
	.open	= sec_ddrsize_proc_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release= single_release,
};

static int __init samsung_proc_ddrsize_init(void)
{
	proc_create("sec_ddrsize", 0, NULL, &sec_ddrsize_proc_fops);

	return 0;
}
late_initcall(samsung_proc_ddrsize_init);
