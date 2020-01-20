/*
 * FIVE State machine
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/task_integrity.h>
#include "five_audit.h"
#include "five_state.h"

enum task_integrity_state_cause {
	STATE_CAUSE_UNKNOWN,
	STATE_CAUSE_DIGSIG,
	STATE_CAUSE_DMV_PROTECTED,
	STATE_CAUSE_TRUSTED,
	STATE_CAUSE_HMAC,
	STATE_CAUSE_SYSTEM_LABEL,
	STATE_CAUSE_NOCERT,
	STATE_CAUSE_TAMPERED,
	STATE_CAUSE_MISMATCH_LABEL
};

struct task_verification_result {
	enum task_integrity_value new_tint;
	enum task_integrity_value prev_tint;
	enum task_integrity_state_cause cause;
};

static const char *task_integrity_state_str(
		enum task_integrity_state_cause cause)
{
	const char *str = "unknown";

	switch (cause) {
	case STATE_CAUSE_DIGSIG:
		str = "digsig";
		break;
	case STATE_CAUSE_DMV_PROTECTED:
		str = "dmv_protected";
		break;
	case STATE_CAUSE_TRUSTED:
		str = "trusted";
		break;
	case STATE_CAUSE_HMAC:
		str = "hmac";
		break;
	case STATE_CAUSE_SYSTEM_LABEL:
		str = "system_label";
		break;
	case STATE_CAUSE_NOCERT:
		str = "nocert";
		break;
	case STATE_CAUSE_MISMATCH_LABEL:
		str = "mismatch_label";
		break;
	case STATE_CAUSE_TAMPERED:
		str = "tampered";
		break;
	case STATE_CAUSE_UNKNOWN:
		str = "unknown";
		break;
	}

	return str;
}

static int is_system_label(struct integrity_label *label)
{
	if (label && label->len == 0)
		return 1; /* system label */

	return 0;
}

static inline int integrity_label_cmp(struct integrity_label *l1,
					struct integrity_label *l2)
{
	return 0;
}

static int verify_or_update_label(struct task_integrity *intg,
		struct integrity_iint_cache *iint)
{
	struct integrity_label *l;
	struct integrity_label *file_label = iint->five_label;
	int rc = 0;

	if (!file_label) /* digsig doesn't have label */
		return 0;

	if (is_system_label(file_label))
		return 0;

	spin_lock(&intg->value_lock);
	l = intg->label;

	if (l) {
		if (integrity_label_cmp(file_label, l)) {
			rc = -EPERM;
			goto out;
		}
	} else {
		struct integrity_label *new_label;

		new_label = kmalloc(sizeof(file_label->len) + file_label->len,
				GFP_ATOMIC);
		if (!new_label) {
			rc = -ENOMEM;
			goto out;
		}

		new_label->len = file_label->len;
		memcpy(new_label->data, file_label->data, new_label->len);
		intg->label = new_label;
	}

out:
	spin_unlock(&intg->value_lock);

	return rc;
}

static bool set_first_state(struct integrity_iint_cache *iint,
				struct task_integrity *integrity,
				struct task_verification_result *result)
{
	enum task_integrity_value tint = INTEGRITY_NONE;
	bool trusted_file = iint->five_flags & FIVE_TRUSTED_FILE;
	enum task_integrity_state_cause cause = STATE_CAUSE_UNKNOWN;

	result->new_tint = result->prev_tint = task_integrity_read(integrity);
	task_integrity_clear(integrity);

	switch (iint->five_status) {
	case FIVE_FILE_RSA:
		if (trusted_file) {
			cause = STATE_CAUSE_TRUSTED;
			tint = INTEGRITY_PRELOAD_ALLOW_SIGN;
		} else {
			cause = STATE_CAUSE_DIGSIG;
			tint = INTEGRITY_PRELOAD;
		}
		break;
	case FIVE_FILE_DMVERITY:
		cause = STATE_CAUSE_DMV_PROTECTED;
		tint = INTEGRITY_DMVERITY;
		break;
	case FIVE_FILE_HMAC:
		cause = STATE_CAUSE_HMAC;
		tint = INTEGRITY_MIXED;
		break;
	case FIVE_FILE_FAIL:
		cause = STATE_CAUSE_TAMPERED;
		tint = INTEGRITY_NONE;
		break;
	case FIVE_FILE_UNKNOWN:
		cause = STATE_CAUSE_NOCERT;
		tint = INTEGRITY_NONE;
		break;
	default:
		cause = STATE_CAUSE_NOCERT;
		tint = INTEGRITY_NONE;
		break;
	}

	task_integrity_set(integrity, tint);
	result->new_tint = tint;
	result->cause = cause;

	return true;
}

static bool set_next_state(struct integrity_iint_cache *iint,
			   struct task_integrity *integrity,
			   struct task_verification_result *result)
{
	bool is_newstate = false;
	bool has_digsig = (iint->five_status == FIVE_FILE_RSA);
	bool dmv_protected = (iint->five_status == FIVE_FILE_DMVERITY);
	struct integrity_label *label = iint->five_label;
	enum task_integrity_state_cause cause = STATE_CAUSE_UNKNOWN;
	enum task_integrity_value state_tint = INTEGRITY_NONE;

	result->new_tint = result->prev_tint = task_integrity_read(integrity);

	if (has_digsig)
		return is_newstate;

	if (five_get_cache_status(iint) == FIVE_FILE_UNKNOWN ||
			five_get_cache_status(iint) == FIVE_FILE_FAIL) {
		spin_lock(&integrity->value_lock);

		if (five_get_cache_status(iint) == FIVE_FILE_UNKNOWN)
			cause = STATE_CAUSE_NOCERT;
		else
			cause = STATE_CAUSE_TAMPERED;

		state_tint = INTEGRITY_NONE;
		is_newstate = true;
		goto out;
	}

	if (verify_or_update_label(integrity, iint)) {
		spin_lock(&integrity->value_lock);
		cause = STATE_CAUSE_MISMATCH_LABEL;
		state_tint = INTEGRITY_NONE;
		is_newstate = true;
		goto out;
	}

	spin_lock(&integrity->value_lock);
	switch (integrity->value) {
	case INTEGRITY_PRELOAD_ALLOW_SIGN:
		if (dmv_protected) {
			cause = STATE_CAUSE_DMV_PROTECTED;
			state_tint = INTEGRITY_DMVERITY_ALLOW_SIGN;
		} else if (is_system_label(label)) {
			cause = STATE_CAUSE_SYSTEM_LABEL;
			state_tint = INTEGRITY_MIXED_ALLOW_SIGN;
		} else {
			cause = STATE_CAUSE_HMAC;
			state_tint = INTEGRITY_MIXED;
		}
		is_newstate = true;
		break;
	case INTEGRITY_PRELOAD:
		if (dmv_protected) {
			cause = STATE_CAUSE_DMV_PROTECTED;
			state_tint = INTEGRITY_DMVERITY;
		} else {
			cause = STATE_CAUSE_HMAC;
			state_tint = INTEGRITY_MIXED;
		}
		is_newstate = true;
		break;
	case INTEGRITY_MIXED_ALLOW_SIGN:
		if (!dmv_protected && !is_system_label(label)) {
			cause = STATE_CAUSE_HMAC;
			state_tint = INTEGRITY_MIXED;
			is_newstate = true;
		}
		break;
	case INTEGRITY_DMVERITY:
		if (!dmv_protected) {
			cause = STATE_CAUSE_HMAC;
			state_tint = INTEGRITY_MIXED;
			is_newstate = true;
		}
		break;
	case INTEGRITY_DMVERITY_ALLOW_SIGN:
		if (!dmv_protected) {
			if (is_system_label(label)) {
				cause = STATE_CAUSE_SYSTEM_LABEL;
				state_tint = INTEGRITY_MIXED_ALLOW_SIGN;
			} else {
				cause = STATE_CAUSE_HMAC;
				state_tint = INTEGRITY_MIXED;
			}
			is_newstate = true;
		}
		break;
	case INTEGRITY_MIXED:
		break;
	case INTEGRITY_NONE:
		break;
	default:
		// Unknown state
		cause = STATE_CAUSE_UNKNOWN;
		state_tint = INTEGRITY_NONE;
		is_newstate = true;
	}

out:

	if (is_newstate) {
		__task_integrity_set(integrity, state_tint);
		result->new_tint = state_tint;
		result->cause = cause;
	}
	spin_unlock(&integrity->value_lock);

	return is_newstate;
}

int five_state_proceed(struct task_integrity *integrity,
			struct file_verification_result *file_result)
{
	int rc = file_result->check;
	struct integrity_iint_cache *iint = file_result->iint;
	enum five_hooks fn = file_result->fn;
	struct task_struct *task = file_result->task;
	struct file *file = file_result->file;
	bool is_newstate;
	struct task_verification_result task_result = {};

	if (rc == 0 && !iint)
		return 0;

	if (rc) {
		enum task_integrity_value new_tint, prev_tint;
		enum task_integrity_state_cause cause = STATE_CAUSE_UNKNOWN;

		prev_tint = task_integrity_read(integrity);
		task_integrity_reset(integrity);
		new_tint = task_integrity_read(integrity);
		five_audit_verbose(task, file, five_get_string_fn(fn),
			prev_tint, new_tint,
			task_integrity_state_str(cause), rc);

		return -EACCES;
	}

	if (fn == BPRM_CHECK)
		is_newstate = set_first_state(iint, integrity, &task_result);
	else
		is_newstate = set_next_state(iint, integrity, &task_result);

	if (is_newstate) {
		five_audit_verbose(task, file, five_get_string_fn(fn),
			task_result.prev_tint, task_result.new_tint,
			task_integrity_state_str(task_result.cause), rc);
	}

	return 0;
}

