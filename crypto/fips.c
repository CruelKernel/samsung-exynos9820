/*
 * FIPS 200 support.
 *
 * Copyright (c) 2008 Neil Horman <nhorman@tuxdriver.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <linux/export.h>
#include <linux/fips.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysctl.h>
#include "internal.h"

/* provide fips_enable always ON */
int fips_enabled = 1;
EXPORT_SYMBOL_GPL(fips_enabled);

/* 
	The next peace of code should be cut off. Due to:
	1) The revision provides FIPSed kernel only 
	2) CONFIG_CRYPTO_FIPS meaning is different comparing to vanilla kernel, "FIPS 200" -> "FIPS 140-2"
*/
#if 0
/* Process kernel command-line parameter at boot time. fips=0 or fips=1 */
static int fips_enable(char *str)
{
	fips_enabled = !!simple_strtol(str, NULL, 0);
	printk(KERN_INFO "fips mode: %s\n",
		fips_enabled ? "enabled" : "disabled");
	return 1;
}

__setup("fips=", fips_enable);
#endif

static struct ctl_table crypto_sysctl_table[] = {
	{
		.procname       = "fips_status",
		.data           = NULL,
		.maxlen         = sizeof(int),
		.mode           = 0444,
		.proc_handler   = proc_dointvec
	},
	{}
};

static struct ctl_table crypto_dir_table[] = {
	{
		.procname       = "crypto",
		.mode           = 0555,
		.child          = crypto_sysctl_table
	},
	{}
};

static struct ctl_table_header *crypto_sysctls;

static void crypto_proc_fips_init(void)
{
	crypto_sysctl_table[0].data = (void*)get_pointer_in_fips_err();
	crypto_sysctls = register_sysctl_table(crypto_dir_table);
}

static void crypto_proc_fips_exit(void)
{
	if (crypto_sysctls)
		unregister_sysctl_table(crypto_sysctls);
}

static int __init fips_init(void)
{
	crypto_proc_fips_init();
	return 0;
}

static void __exit fips_exit(void)
{
	crypto_proc_fips_exit();
}

module_init(fips_init);
module_exit(fips_exit);
