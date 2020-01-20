/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/bsearch.h>
#include <linux/dsms.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

#include "dsms_access_control.h"
#include "dsms_debug.h"

typedef int (*cmp_fn_t)(const void *key, const void *element);

/*
 * compare_to_policy_entry_key - compare a function name with the function
 * name of a policy entry
 * @function_name: function name
 * @entry: pointer to the policy entry
 *
 * Returns lexicographic order of the two compared function names
 */
static int compare_policy_entries(const char *function_name,
		const struct dsms_policy_entry *entry)
{
	return strncmp(function_name, entry->function_name, KSYM_NAME_LEN);
}

/*
 * find_policy_entry - find policy entry
 * @function_name: function name to find
 *
 * Returns a pointer to the policy entry for the given function,
 * or NULL if not found.
 */
static struct dsms_policy_entry *find_policy_entry(const char *function_name)
{
	void *entry;

	// see: https://github.com/torvalds/linux/blob/master/lib/bsearch.c
	entry = bsearch(function_name,
					dsms_policy,
					dsms_policy_size(),
					(sizeof *dsms_policy),
					(cmp_fn_t) compare_policy_entries);

	return (struct dsms_policy_entry *)entry;
}

int dsms_verify_access(const void *address)
{
	const char *symname;
	unsigned long symsize;
	unsigned long offset;
	char *modname;
	char function_name[KSYM_NAME_LEN+1];
	int index;

#ifdef DSMS_DEBUG_WHITELIST
	printk(DSMS_DEBUG_TAG "dsms_verify_access: "
		"Caller function is %pS (%pF)", address, address);
#endif

	if (!address) {
#ifdef DSMS_DEBUG_ENABLE
		printk(DSMS_DEBUG_TAG "DENY: invalid caller address.");
#endif
		return DSMS_DENY;
	}
	
	symname = kallsyms_lookup((unsigned long)address,
					&symsize, &offset, &modname, function_name);
	if (!symname) {
#ifdef DSMS_DEBUG_ENABLE
		printk(DSMS_DEBUG_TAG "DENY: caller address not in kallsyms.");
#endif
		return DSMS_DENY;
	}

	function_name[KSYM_NAME_LEN] = 0;
#ifdef DSMS_DEBUG_WHITELIST
	printk(DSMS_DEBUG_TAG "kallsyms caller modname = %s, function_name = '%s',"
			" offset = 0x%lx", modname, function_name, offset);
#endif

	if (modname != NULL) {
#ifdef DSMS_DEBUG_ENABLE
		printk(DSMS_DEBUG_TAG "DENY: function '%s' is "
			"not a kernel symbol", function_name);
#endif
		return DSMS_DENY; // not a kernel symbol
	}
	
#ifdef DSMS_WHITELIST_IGNORE_NAME_SUFFIXES_ENABLE
	for (index = 0; index < KSYM_NAME_LEN; index++)
		if ((function_name[index] == '.') || (function_name[index] == 0))
			break;
	function_name[index] = 0;
#endif //DSMS_WHITELIST_IGNORE_NAME_SUFFIXES_ENABLE

	if (find_policy_entry(function_name) == NULL) {
#ifdef DSMS_DEBUG_ENABLE
		printk(DSMS_DEBUG_TAG "DENY: function '%s': is "
			"not allowed by policy", function_name);
#endif
		return DSMS_DENY;
	}

	return DSMS_SUCCESS;
}
