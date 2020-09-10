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
__visible_for_testing int compare_policy_entries(const char *function_name,
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
__visible_for_testing struct dsms_policy_entry *find_policy_entry(const char *function_name)
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

	dsms_log_write(LOG_DEBUG, "dsms_verify_access: "
		       "Caller function is %pS (%pF)", address, address);

	if (!address) {
		dsms_log_write(LOG_ERROR, "DENY: invalid caller address.");
		return DSMS_DENY;
	}

	symname = kallsyms_lookup((unsigned long)address,
				  &symsize, &offset, &modname, function_name);
	if (!symname) {
		dsms_log_write(LOG_ERROR, "DENY: caller address not in kallsyms.");
		return DSMS_DENY;
	}

	function_name[KSYM_NAME_LEN] = 0;
	dsms_log_write(LOG_DEBUG, "kallsyms caller modname = %s, function_name = '%s',"
		       " offset = 0x%lx", modname, function_name, offset);

	if (modname != NULL) {
		dsms_log_write(LOG_ERROR, "DENY: function '%s' is "
				   "not a kernel symbol", function_name);
		return DSMS_DENY; // not a kernel symbol
	}

	if (should_ignore_whitelist_suffix()) {
		for (index = 0; index < KSYM_NAME_LEN; index++)
			if ((function_name[index] == '.') || (function_name[index] == 0))
				break;
		function_name[index] = 0;
	}

	if (find_policy_entry(function_name) == NULL) {
		dsms_log_write(LOG_ERROR, "DENY: function '%s': is "
			       "not allowed by policy", function_name);
		return DSMS_DENY;
	}

	return DSMS_SUCCESS;
}
