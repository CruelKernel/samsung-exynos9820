/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include "dsms_kernel_api.h"
#include "dsms_message_list.h"
#include "dsms_netlink.h"

static struct dsms_message *current_message;
static struct task_struct *dsms_sender_thread;

static DEFINE_MUTEX(dsms_wait_daemon_mutex);

static int dsms_start_sender_thread(struct sk_buff *skb, struct genl_info *info);

/*
 * DSMS netlink policy creation for the possible fields for the communication.
 */
static struct nla_policy dsms_netlink_policy[DSMS_ATTR_COUNT + 1] = {
	[DSMS_VALUE] = { .type = NLA_U64 },
	[DSMS_FEATURE_CODE] = { .type = NLA_STRING, .len = FEATURE_CODE_LENGTH + 1},
	[DSMS_DETAIL] = { .type = NLA_STRING, .len = MAX_ALLOWED_DETAIL_LENGTH + 1},
	[DSMS_DAEMON_READY] = { .type = NLA_U32 },
};

/*
 * Definition of the netlink operations handled by the dsms kernel and
 * the daemon of dsms, for this case the DSMS_MSG_CMD operation will be handled
 * dsms_start_sender_thread function.
 */
static const struct genl_ops dsms_kernel_ops[] = {
	{
		.cmd = DSMS_MSG_CMD,
		.doit = dsms_start_sender_thread,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
		.policy = dsms_netlink_policy,
#endif
	},
};

struct genl_multicast_group dsms_group[] = {
	{
		.name = DSMS_GROUP,
	},
};

/*
 * Descriptor of DSMS Generic Netlink family
 */
static struct genl_family dsms_family = {
	.name = DSMS_FAMILY,
	.version = 1,
	.maxattr = DSMS_ATTR_MAX,
	.module = THIS_MODULE,
	.ops = dsms_kernel_ops,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	.policy = dsms_netlink_policy,
#endif
	.mcgrps = dsms_group,
	.n_mcgrps = ARRAY_SIZE(dsms_group),
	.n_ops = ARRAY_SIZE(dsms_kernel_ops),
};

int prepare_userspace_communication(void)
{
	current_message = NULL;
	dsms_sender_thread = NULL;
	return genl_register_family(&dsms_family);
}

int remove_userspace_communication(void)
{
	if (mutex_is_locked(&dsms_wait_daemon_mutex))
		mutex_unlock(&dsms_wait_daemon_mutex);
	kthread_stop(dsms_sender_thread);
	return genl_unregister_family(&dsms_family);
}

noinline int dsms_send_netlink_message(struct dsms_message *message)
{
	struct sk_buff *skb;
	void *msg_head;
	int ret = 0;
	int detail_len;

	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (skb == NULL) {
		DSMS_LOG_DEBUG("It was not possible to allocate memory for the message.");
		return -ENOMEM;
	}

	// Creation of the message header
	msg_head = genlmsg_put(skb, 0, 0,
			       &dsms_family, 0, DSMS_MSG_CMD);
	if (msg_head == NULL) {
		DSMS_LOG_DEBUG("It was not possible to create the message head.");
		return -ENOMEM;
	}

	ret = nla_put(skb, DSMS_VALUE, sizeof(message->value), &message->value);
	if (ret) {
		DSMS_LOG_DEBUG("It was not possible to add field DSMS_VALUE to the message.");
		return ret;
	}

	ret = nla_put(skb, DSMS_FEATURE_CODE,
		      FEATURE_CODE_LENGTH + 1, message->feature_code);
	if (ret) {
		DSMS_LOG_DEBUG("It was not possible to add field DSMS_FEATURE_CODE to the message.");
		return ret;
	}

	detail_len = strnlen(message->detail, MAX_ALLOWED_DETAIL_LENGTH);
	ret = nla_put(skb, DSMS_DETAIL, detail_len + 1, message->detail);
	if (ret) {
		DSMS_LOG_DEBUG("It was not possible to add field DSMS_DETAIL to the message.");
		return ret;
	}

	genlmsg_end(skb, msg_head);
	ret = genlmsg_multicast(&dsms_family, skb, 0, 0, GFP_KERNEL);
	if (ret) {
		DSMS_LOG_DEBUG("It was not possible to send the message.");
		return ret;
	}

	return 0;
}

static int dsms_send_messages_thread(void *unused)
{
	int ret;

	msleep(2000);
	while (1) {
		if (!current_message && !kthread_should_stop())
			current_message = get_dsms_message();

		if (kthread_should_stop())
			do_exit(0);

		if (!current_message) {
			DSMS_LOG_DEBUG("There is no message in the list.");
			continue;
		}

		ret = dsms_send_netlink_message(current_message);
		if (ret < 0) {
			DSMS_LOG_ERROR("Error while send a message? %d.", ret);
			mutex_lock(&dsms_wait_daemon_mutex);
			msleep(2000);
			continue;
		}

		kfree(current_message->feature_code);
		kfree(current_message->detail);
		kfree(current_message);
		current_message = NULL;
	}
	return 0;
}

static int dsms_start_sender_thread(struct sk_buff *skb, struct genl_info *info)
{
	if (!dsms_sender_thread) {
		dsms_sender_thread = kthread_run(dsms_send_messages_thread, NULL, "dsms_kthread");
		if (!dsms_sender_thread)
			DSMS_LOG_ERROR("It was not possible to create the dsms thread.");
	} else {
		if (mutex_is_locked(&dsms_wait_daemon_mutex))
			mutex_unlock(&dsms_wait_daemon_mutex);
	}
	return 0;
}