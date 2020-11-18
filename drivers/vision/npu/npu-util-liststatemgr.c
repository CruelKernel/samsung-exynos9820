/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/slab.h>

#include "npu-log.h"
#include "npu-util-liststatemgr.h"
#include "npu-util-autosleepthr.h"

const char *default_name = "Thr??";
#define NL_NAME(name) ((name == NULL) ? (default_name) : (name))

const char *LSM_STATE_NAMES[LSM_LIST_TYPE_INVALID + 1]
	= {"FREE", "REQUESTED", "PROCESSING", "COMPLETED", "STUCKED", "INVALID"};

/* Helper function: Initialize entry lists */
void __lsm_reset_list(struct lsm_internal_data *lsm)
{
	int i;

	BUG_ON(!lsm);
	/* Initialization of INVALID list is not really necessary,
	   but doing so for safety */
	for (i = 0; i < LSM_LIST_TYPE_INVALID; i++) {
		INIT_LIST_HEAD(&(lsm->list_for_state[i]));
	}
}

void __lsm_destroy(struct lsm_internal_data *lsm)
{
	int ret = 0;

	npu_info("start in __lsm_destroy[%s]\n", NL_NAME(lsm->name));

	BUG_ON(!lsm);

	if (lsm->is_ast_enabled) {
		/* Terminate the worker thread */
		ret = auto_sleep_thread_terminate(&(lsm->ast_worker));
		/* ret is usually the return value of thread function.
		so non-zero value does not mean error condition */
		npu_dbg("auto_sleep_thread_terminate on LSM [%s]. returns [%d]\n",
			NL_NAME(lsm->name), ret);
	}

	npu_info("complete in __lsm_destroy(%s)\n", NL_NAME(lsm->name));
	return;
}

struct lsm_base_entry *__lsm_get_entry(struct lsm_internal_data *lsm,
				       lsm_list_type_e type)
{
	/* The list for specified type */
	struct lsm_base_entry *ret_entry;
	struct list_head *target_list_head;

	BUG_ON(!lsm);
	BUG_ON(type < 0);
	BUG_ON(type >= LSM_LIST_TYPE_INVALID);

	target_list_head = &(lsm->list_for_state[type]);
	/* Check whether the list is empty */
	if (list_empty(target_list_head)) {
		npu_warn("LSM[%s]. empty on type[%d].\n",
			 NL_NAME(lsm->name), type);
		return NULL;
	} else {
		/* Return the first(Oldest) element */
		ret_entry = list_first_entry(target_list_head,
					     struct lsm_base_entry, list);
		list_del_init(&(ret_entry->list));
		return ret_entry;
	}
}

/* Helper function - Put entry in sorted list */
static int __put_entry_sorted(struct list_head *target_list_head,
			      struct lsm_base_entry *new_entry,
			      lsm_comparator_t compare)
{
	struct list_head *iter;
	struct list_head *prev = target_list_head;
	struct lsm_base_entry *cmp_entry;
	int		       index = 0;

	list_for_each(iter, target_list_head)
	{
		cmp_entry = list_entry(iter, struct lsm_base_entry, list);
		if (compare(&(new_entry->data_place), &(cmp_entry->data_place))) {
			/* The entry should be added before iter */
			list_add(&(new_entry->list), prev);
			return index;
		}
		/* Update prev */
		prev = iter;
		index++;
	}
	/* Reach at the end of list - Add it to tail */
	list_add_tail(&(new_entry->list), target_list_head);
	return index;
}

/* Helper function - Put entry in unsorted list */
static int __put_entry_unsorted(struct list_head *target_list_head,
				struct lsm_base_entry *new_entry)
{
	list_add_tail(&(new_entry->list), target_list_head);
	return 0;
}

int __lsm_put_entry(struct lsm_internal_data *lsm, lsm_list_type_e type,
		    struct lsm_base_entry *new_entry)
{
	int ret = 0;
	struct list_head *target_list_head;

	BUG_ON(!new_entry);
	BUG_ON(!lsm);
	BUG_ON(type < 0);
	BUG_ON(type >= LSM_LIST_TYPE_INVALID);

	target_list_head = &(lsm->list_for_state[type]);
	new_entry->state = type;

	/* Check whether the entry is properly detached from list */
	BUG_ON(!list_empty(&(new_entry->list)));

	/* Call helper, based on compare function availability */
	if (lsm->compare != NULL) {
		ret = __put_entry_sorted(target_list_head, new_entry,
					  (lsm_comparator_t)lsm->compare);
	} else {
		ret = __put_entry_unsorted(target_list_head, new_entry);
	}
	/* Call hook on success */
	if (ret == 0)
		if (lsm->put_hook)	{
			lsm->put_hook(type, &new_entry->data_place);
		}
	return ret;
}

int __lsm_move_entry(struct lsm_internal_data *lsm, lsm_list_type_e to,
		     struct lsm_base_entry *entry)
{
	BUG_ON(!lsm);
	BUG_ON(to < 0);
	BUG_ON(to >= LSM_LIST_TYPE_INVALID);
	BUG_ON(!entry);

	/* Remove from the current list */
	list_del_init(&(entry->list));

	/* Put it to target list */
	return __lsm_put_entry(lsm, to, entry);
}

void __lsm_send_signal(struct lsm_internal_data *lsm)
{
	BUG_ON(!lsm);

	auto_sleep_thread_signal(&lsm->ast_worker);
}

void __lsm_set_hook(struct lsm_internal_data *lsm, lsm_hook_t put_hook)
{
	BUG_ON(!lsm);
	lsm->put_hook = put_hook;
}

/* Unit test */
#ifdef CONFIG_VISION_UNITTEST
#define IDIOT_TESTCASE_IMPL "../npu/npu-util-liststatemgr.idiot"
#include "idiot-def.h"
#endif
