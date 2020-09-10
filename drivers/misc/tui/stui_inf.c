/* tui/stui_inf.c
 *
 * Samsung TUI HW Handler driver.
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <decon.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#ifdef USE_TEE_CLIENT_API
#include <tee_client_api.h>
#endif /* USE_TEE_CLIENT_API */
#include "stui_inf.h"

#define TUI_REE_EXTERNAL_EVENT      42
#define SESSION_CANCEL_DELAY        150
#define MAX_WAIT_CNT                10

static int tui_mode = STUI_MODE_OFF;
static int tui_blank_cnt;
static DEFINE_SPINLOCK(tui_lock);

#ifdef USE_TEE_CLIENT_API
static TEEC_UUID uuid = {
	.timeLow = 0x0,
	.timeMid = 0x0,
	.timeHiAndVersion = 0x0,
	.clockSeqAndNode = {0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x81},
};
#endif

int stui_inc_blank_ref(void)
{
	unsigned long fls;
	int ret_cnt;

	spin_lock_irqsave(&tui_lock, fls);
	ret_cnt = ++tui_blank_cnt;
	spin_unlock_irqrestore(&tui_lock, fls);
	return ret_cnt;
}
EXPORT_SYMBOL(stui_inc_blank_ref);

int stui_dec_blank_ref(void)
{
	unsigned long fls;
	int ret_cnt;

	spin_lock_irqsave(&tui_lock, fls);
	ret_cnt = --tui_blank_cnt;
	spin_unlock_irqrestore(&tui_lock, fls);
	return ret_cnt;
}
EXPORT_SYMBOL(stui_dec_blank_ref);

int stui_get_blank_ref(void)
{
	unsigned long fls;
	int ret_cnt;

	spin_lock_irqsave(&tui_lock, fls);
	ret_cnt = tui_blank_cnt;
	spin_unlock_irqrestore(&tui_lock, fls);
	return ret_cnt;
}
EXPORT_SYMBOL(stui_get_blank_ref);

void stui_set_blank_ref(int cnt)
{
	unsigned long fls;

	spin_lock_irqsave(&tui_lock, fls);
	tui_blank_cnt = cnt;
	spin_unlock_irqrestore(&tui_lock, fls);
}
EXPORT_SYMBOL(stui_set_blank_ref);

int stui_get_mode(void)
{
	unsigned long fls;
	int ret_mode;

	spin_lock_irqsave(&tui_lock, fls);
	ret_mode = tui_mode;
	spin_unlock_irqrestore(&tui_lock, fls);
	return ret_mode;
}
EXPORT_SYMBOL(stui_get_mode);

void stui_set_mode(int mode)
{
	unsigned long fls;

	spin_lock_irqsave(&tui_lock, fls);
	tui_mode = mode;
	spin_unlock_irqrestore(&tui_lock, fls);
}
EXPORT_SYMBOL(stui_set_mode);

int stui_set_mask(int mask)
{
	unsigned long fls;
	int ret_mode;

	spin_lock_irqsave(&tui_lock, fls);
	ret_mode = (tui_mode |= mask);
	spin_unlock_irqrestore(&tui_lock, fls);
	return ret_mode;
}
EXPORT_SYMBOL(stui_set_mask);

int stui_clear_mask(int mask)
{
	unsigned long fls;
	int ret_mode;

	spin_lock_irqsave(&tui_lock, fls);
	ret_mode = (tui_mode &= ~mask);
	spin_unlock_irqrestore(&tui_lock, fls);
	return ret_mode;
}
EXPORT_SYMBOL(stui_clear_mask);

#ifdef USE_TEE_CLIENT_API
static atomic_t canceling = ATOMIC_INIT(0);
int stui_cancel_session(void)
{
	TEEC_Context context;
	TEEC_Session session;
	TEEC_Result result;
	TEEC_Operation operation;
	int ret = -1;
	int count = 0;

	pr_info("[STUI] %s begin\n", __func__);

	if (!(STUI_MODE_ALL & stui_get_mode())) {
		pr_info("[STUI] session cancel is not needed\n");
		return 0;
	}

	if (atomic_cmpxchg(&canceling, 0, 1) != 0) {
		pr_info("[STUI] already canceling.\n");
		return 0;
	}

	result = TEEC_InitializeContext(NULL, &context);
	if (result != TEEC_SUCCESS) {
		pr_err("[STUI] TEEC_InitializeContext returned: 0x%x\n", result);
		goto out;
	}

	result = TEEC_OpenSession(&context, &session, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
	if (result != TEEC_SUCCESS) {
		pr_err("[STUI] TEEC_OpenSession returned: 0x%x\n", result);
		goto finalize_context;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&session, TUI_REE_EXTERNAL_EVENT, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		pr_err("[STUI] TEEC_InvokeCommand returned: 0x%x\n", result);
		goto close_session;
	} else {
		pr_info("[STUI] invoked cancel cmd\n");
	}

	TEEC_CloseSession(&session);
	TEEC_FinalizeContext(&context);

	while ((STUI_MODE_ALL & stui_get_mode()) && (count < MAX_WAIT_CNT)) {
		msleep(SESSION_CANCEL_DELAY);
		count++;
	}

	if (STUI_MODE_ALL & stui_get_mode()) {
		pr_err("[STUI] session was not cancelled yet\n");
	} else {
		pr_info("[STUI] session was cancelled successfully\n");
		ret = 0;
	}

	atomic_set(&canceling, 0);
	return ret;

close_session:
	TEEC_CloseSession(&session);
finalize_context:
	TEEC_FinalizeContext(&context);
out:
	atomic_set(&canceling, 0);
	pr_err("[STUI] %s end, ret=%d, result=0x%x\n", __func__, ret, result);

	return ret;
}
#else /* USE_TEE_CLIENT_API */
int stui_cancel_session(void)
{
	pr_err("[STUI] %s not supported\n", __func__);
	return -1;
}
#endif /* USE_TEE_CLIENT_API */

#ifdef CONFIG_SOC_EXYNOS3250

static int tui_mode_3250 = STUI_MODE_OFF;
static BLOCKING_NOTIFIER_HEAD(trustedui_nb_list);

int trustedui_nb_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&trustedui_nb_list, nb);
}

int trustedui_nb_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&trustedui_nb_list, nb);
}

int trustedui_nb_send_event(unsigned long val, void *v)
{
	pr_info("%s:cmd = %ld mode = %d\n", __func__, val, (*(int *)v));
	return blocking_notifier_call_chain(&trustedui_nb_list, val, v);
}

void trustedui_set_mode(int mode)
{
	unsigned long flags;

	spin_lock_irqsave(&tui_lock, flags);
	if (tui_mode_3250 == mode) {
		spin_unlock_irqrestore(&tui_lock, flags);
		return;
	}
	pr_info("%s:mode[%d->%d]\n", __func__, tui_mode_3250, mode);
	tui_mode_3250 = mode;
	spin_unlock_irqrestore(&tui_lock, flags);
	trustedui_nb_send_event(TRUSTEDUI_MODE_CHANGE, (void *)&mode);
}
EXPORT_SYMBOL(trustedui_set_mode);

#endif /* CONFIG_SOC_EXYNOS3250 */

static int stui_notifier_callback(struct notifier_block *self, unsigned long event, void *data);

static struct notifier_block stui_fb_notif = {
	.notifier_call = stui_notifier_callback,
};

static struct notifier_block stui_reboot_nb = {
	.notifier_call = stui_notifier_callback,
};

static int stui_is_decon_in_state_tui(const struct fb_info *const info)
{
	struct decon_win *win = NULL;
	struct decon_device *decon = NULL;

	if (info == NULL) {
		pr_err("[STUI] %s: fb_info is null.\n", __func__);
		return 0;
	}

	win = info->par;
	if (win == NULL) {
		pr_err("[STUI] %s: decon_win is null.\n", __func__);
		return 0;
	}

	decon = win->decon;
	if (decon == NULL) {
		pr_err("[STUI] %s: decon is null.\n", __func__);
		return 0;
	}

	pr_debug("[STUI] %s: decon=%d, state=%d\n", __func__, decon->id, decon->state);
	return (decon->state == DECON_STATE_TUI);
}

static int stui_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	(void) data;

	pr_debug("[STUI] %s (start): event=%lu\n", __func__, event);

	if (!(self == &stui_fb_notif && event == FB_EARLY_EVENT_BLANK)
		&& self != &stui_reboot_nb) {
		return NOTIFY_OK;
	}

	if (self == &stui_fb_notif && data != NULL) {
		const struct fb_event *event_data = data;
		const struct fb_info *info = event_data->info;

		if (stui_is_decon_in_state_tui(info) == 0) {
			pr_err("[STUI] %s is not state TUI\n", __func__);
			return NOTIFY_OK;
		}
	}

	stui_cancel_session();
	pr_debug("[STUI] %s (finish)\n", __func__);
	return NOTIFY_OK;
}

int stui_register_on_events(void)
{
	int ret = -1;
	pr_debug("[STUI] %s (start)\n", __func__);
	ret = fb_register_client(&stui_fb_notif);
	if (ret != 0) {
		pr_err("[STUI] Unable to register fb notifier (%d)\n", ret);
		return ret;
	}

	ret = register_reboot_notifier(&stui_reboot_nb);
	if (ret != 0) {
		pr_err("[STUI] Unable to register reboot notifier (%d)\n", ret);
		fb_unregister_client(&stui_fb_notif);
		return ret;
	}

	pr_debug("[STUI] %s (finish)\n", __func__);
	return ret;
}

void stui_unregister_from_events(void)
{
	pr_debug("[STUI] %s (start)\n", __func__);
	fb_unregister_client(&stui_fb_notif);
	unregister_reboot_notifier(&stui_reboot_nb);
	pr_debug("[STUI] %s (finish)\n", __func__);
}
