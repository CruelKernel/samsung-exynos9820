/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/dsms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "dsms_access_control.h"
#include "dsms_init.h"
#include "dsms_kernel_api.h"
#include "dsms_message_list.h"
#include "dsms_rate_limit.h"
#include "dsms_test.h"

__visible_for_testing struct dsms_message *create_message(const char *feature_code,
		const char *detail,
		int64_t value)
{
	int len_detail = 0;
	struct dsms_message *message;

	message = kmalloc(sizeof(struct dsms_message), GFP_KERNEL);
	if (!message) {
		DSMS_LOG_ERROR("It was not possible to allocate memory for message.");
		return NULL;
	}

	message->feature_code = kmalloc_array(FEATURE_CODE_LENGTH + 1,
					sizeof(char), GFP_KERNEL);
	if (!message->feature_code) {
		DSMS_LOG_ERROR("It was not possible to allocate memory for feature code.");
		kfree(message);
		return NULL;
	}
	strncpy(message->feature_code, feature_code, sizeof(char) *
						     FEATURE_CODE_LENGTH);
	message->feature_code[FEATURE_CODE_LENGTH] = '\0';

	len_detail = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH) + 1;
	message->detail = kmalloc_array(len_detail, sizeof(char), GFP_KERNEL);
	if (!message->detail) {
		DSMS_LOG_ERROR("It was not possible to allocate memory for detail.");
		kfree(message->feature_code);
		kfree(message);
		return NULL;
	}
	strncpy(message->detail, detail, len_detail);
	message->detail[len_detail - 1] = '\0';
	message->value = value;
	return message;
}

noinline int dsms_send_message(const char *feature_code,
		const char *detail,
		int64_t value)
{
	void *address;
	int ret = DSMS_DENY;
	size_t len;
	struct dsms_message *message;

	if (!feature_code) {
		DSMS_LOG_ERROR("Invalid feature code.");
		ret = -EINVAL;
		goto exit_send;
	}

	if (!detail)
		detail = "";

	len = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH);
	DSMS_LOG_DEBUG("{'%s', '%s' (%zu bytes), %lld}",
		       feature_code, detail, len, value);

	if (!dsms_is_initialized()) {
		DSMS_LOG_ERROR("DSMS not initialized yet.");
		ret = -EACCES;
		goto exit_send;
	}

	ret = dsms_check_message_list_limit();
	if (ret != DSMS_SUCCESS)
		goto exit_send;

	ret = dsms_check_message_rate_limit();
	if (ret != DSMS_SUCCESS)
		goto exit_send;

	address = __builtin_return_address(CALLER_FRAME);
	ret = dsms_verify_access(address);
	if (ret != DSMS_SUCCESS)
		goto exit_send;

	message = create_message(feature_code, detail, value);
	if (message == NULL)
		goto exit_send;

	ret = process_dsms_message(message);

	if (ret != 0) {
		kfree(message->detail);
		kfree(message->feature_code);
		kfree(message);
	}

exit_send:
	return ret;
}
