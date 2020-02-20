/*
 * drivers/debug/sec_key_notifier.c
 *
 * COPYRIGHT(C) 2016-2019 Samsung Electronics Co., Ltd. All Right Reserved.
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

#define pr_fmg(fmt) KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/module.h>
#include <linux/input.h>
#include <linux/notifier.h>
#include <linux/slab.h>

#include <linux/sec_debug.h>

#include "sec_key_notifier.h"

static DEFINE_SPINLOCK(sec_kn_event_lock);

static ATOMIC_NOTIFIER_HEAD(sec_kn_notifier_list);

static atomic_t sec_kn_acceptable_event[KEY_MAX] __read_mostly;

static void inline update_acceptable_event(unsigned int event_code, bool is_add)
{
	if (is_add)
		atomic_inc(&(sec_kn_acceptable_event[event_code]));
	else
		atomic_dec(&(sec_kn_acceptable_event[event_code]));
}

int sec_kn_register_notifier(struct notifier_block *nb,
		const unsigned int *events, const size_t nr_events)
{
	size_t i;

	for (i = 0; i < nr_events; i++)
		update_acceptable_event(events[i], true);

	return atomic_notifier_chain_register(&sec_kn_notifier_list, nb);
}

int sec_kn_unregister_notifier(struct notifier_block *nb,
		const unsigned int *events, const size_t nr_events)
{
	size_t i;

	for (i = 0; i < nr_events; i++)
		update_acceptable_event(events[i], false);

	return atomic_notifier_chain_unregister(&sec_kn_notifier_list, nb);
}

static inline bool is_event_supported(unsigned int event_type,
		unsigned int event_code)
{
	bool ret;

	if (event_type != EV_KEY || event_code >= KEY_MAX)
		return false;

	ret = !!atomic_read(&(sec_kn_acceptable_event[event_code]));

	return ret;
}

static void sec_kn_event(struct input_handle *handle, unsigned int event_type,
		unsigned int event_code, int value)
{
	struct sec_key_notifier_param param = {
		.keycode = event_code,
		.down = value,
	};

	if (!is_event_supported(event_type, event_code))
		return;

	spin_lock(&sec_kn_event_lock);

	atomic_notifier_call_chain(&sec_kn_notifier_list, 0, &param);

	spin_unlock(&sec_kn_event_lock);
}

static int sec_kn_connect(struct input_handler *handler, struct input_dev *dev,
		const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "sec_key_notifier";

	error = input_register_handle(handle);
	if (error)
		goto err_free_handle;

	error = input_open_device(handle);
	if (error)
		goto err_unregister_handle;

	return 0;

err_unregister_handle:
	input_unregister_handle(handle);
err_free_handle:
	kfree(handle);
	return error;
}

static void sec_kn_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id sec_kn_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{},
};

static struct input_handler sec_kn_handler = {
	.event		= sec_kn_event,
	.connect	= sec_kn_connect,
	.disconnect	= sec_kn_disconnect,
	.name		= "sec_key_notifier",
	.id_table	= sec_kn_ids,
};

static int __init sec_kn_init(void)
{
	int err;
	size_t i;

	for (i = 0; i < KEY_MAX; i++)
		atomic_set(&(sec_kn_acceptable_event[i]), 0);

	spin_lock_init(&sec_kn_event_lock);

	err = input_register_handler(&sec_kn_handler);

	return err;
}

static void __exit sec_kn_exit(void)
{
	input_unregister_handler(&sec_kn_handler);
}

arch_initcall(sec_kn_init);
module_exit(sec_kn_exit);
