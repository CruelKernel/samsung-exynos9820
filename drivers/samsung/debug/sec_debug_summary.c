/*
 * drivers/staging/samsung/sec_debug_summary.c
 *
 * driver supporting debug functions for Samsung device
 *
 * COPYRIGHT(C) 2006-2018 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sec_debug.h>


struct sec_debug_summary *secdbg_summary;
struct sec_debug_summary_data_ap *secdbg_apss;
char *sec_debug_arch_desc;

int sec_debug_summary_add_infomon(char *name, unsigned int size, phys_addr_t pa)
{
	if (!secdbg_apss)
		return -ENOMEM;

	if (secdbg_apss->info_mon.idx >= ARRAY_SIZE(secdbg_apss->info_mon.var))
		return -ENOMEM;

	pr_info("%s: %2d: name: %s / paddr: %llx\n", __func__, secdbg_apss->info_mon.idx, name, pa);

	strlcpy(secdbg_apss->info_mon.var[secdbg_apss->info_mon.idx].name,
				name, sizeof(secdbg_apss->info_mon.var[0].name));
	secdbg_apss->info_mon.var[secdbg_apss->info_mon.idx].sizeof_type = size;
	secdbg_apss->info_mon.var[secdbg_apss->info_mon.idx].var_paddr = pa;

	secdbg_apss->info_mon.idx++;


	return 0;
}

static int __init summary_init_infomon(void)
{
	ADD_STR_TO_INFOMON(linux_banner);

	sec_debug_summary_add_infomon("Kernel cmdline", -1, __pa(saved_command_line));
	sec_debug_summary_add_infomon("Hardware name", -1, __pa(sec_debug_arch_desc));

	return 0;
}

#define DUMP_SUMMARY_MAX_SIZE	(0x300000)

int __init sec_debug_summary_init(void)
{
	char *sec_summary_log_buf;

	pr_info("%s: start\n", __func__);

	sec_summary_log_buf = sec_debug_get_debug_base(SDN_MAP_DUMP_SUMMARY);
	if (!sec_summary_log_buf) {
		pr_info("%s: no summary buffer\n", __func__);

		return 0;
	}

	secdbg_summary = (struct sec_debug_summary *)sec_summary_log_buf;
	memset(secdbg_summary, 0, sizeof(struct sec_debug_summary));

	secdbg_apss = &secdbg_summary->priv.apss;

	secdbg_summary->apss = (struct sec_debug_summary_data_ap *)
		(virt_to_phys(&secdbg_summary->priv.apss) & 0xFFFFFFFF);

	pr_info("apss(%lx)\n", (unsigned long)secdbg_summary->apss);

	strlcpy(secdbg_apss->name, "APSS", sizeof(secdbg_apss->name) + 1);
	strlcpy(secdbg_apss->state, "Init", sizeof(secdbg_apss->state) + 1);
	secdbg_apss->nr_cpus = CONFIG_NR_CPUS;

	summary_init_infomon();


	/* fill magic nubmer last to ensure data integrity when the magic
	 * numbers are written
	 */
	secdbg_summary->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	secdbg_summary->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	secdbg_summary->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	secdbg_summary->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;

	return 0;
}

subsys_initcall_sync(sec_debug_summary_init);
