/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/dsms.h>
#include <linux/errno.h>
#include <linux/llist.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/string.h>
#include "dsms_kernel_api.h"
#include "dsms_message_list.h"
#include "dsms_netlink.h"
#include "dsms_test.h"

struct dsms_message_node {
	struct dsms_message *message;
	struct llist_node llist;
};

__visible_for_testing atomic_t list_counter = ATOMIC_INIT(0);

static struct llist_head dsms_linked_messages = LLIST_HEAD_INIT(dsms_linked_messages);
static struct semaphore sem_count_message;

static struct dsms_message_node *create_node(struct dsms_message *message)
{
	struct dsms_message_node *node;

	node = kmalloc(sizeof(struct dsms_message_node), GFP_KERNEL);
	if (!node) {
		DSMS_LOG_ERROR("It was not possible to allocate memory for node.");
		return NULL;
	}

	node->message = message;
	return node;
}

struct dsms_message *get_dsms_message(void)
{
	struct dsms_message_node *node;
	struct dsms_message *message;

	down(&sem_count_message);
	if (atomic_read(&list_counter) == 0)
		return NULL;
	node = llist_entry(llist_del_first(&dsms_linked_messages),
					   struct dsms_message_node, llist);
	atomic_dec(&list_counter);
	message = node->message;
	kfree(node);

	return message;
}

int process_dsms_message(struct dsms_message *message)
{
	struct dsms_message_node *node;

	if (atomic_add_unless(&list_counter, 1, LIST_COUNT_LIMIT) == 0) {
		DSMS_LOG_ERROR("List counter has reached its limit.");
		return -EBUSY;
	}

	node = create_node(message);
	if (node == NULL) {
		atomic_dec(&list_counter);
		return -ENOMEM;
	}
	DSMS_LOG_DEBUG("Processing message {'%s', '%s' (%zu bytes), %lld}.",
		       message->feature_code, message->detail,
			   strnlen(message->detail, MAX_ALLOWED_DETAIL_LENGTH), message->value);
	llist_add(&node->llist, &dsms_linked_messages);
	up(&sem_count_message);
	return 0;
}

noinline void init_semaphore_list(void)
{
	sema_init(&sem_count_message, 0);
}

noinline int dsms_check_message_list_limit(void)
{
	return atomic_read(&list_counter) < LIST_COUNT_LIMIT ? DSMS_SUCCESS : DSMS_DENY;
}
