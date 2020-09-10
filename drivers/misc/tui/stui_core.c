/* tui/stui_core.c
 *
 * Samsung TUI HW Handler driver.
 *
 * Copyright (C) 2012-2018, Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>

#include "stui_core.h"
#include "stui_hal.h"
#include "stui_ioctl.h"
#include "stui_inf.h"

struct wake_lock tui_wakelock;

#ifdef SAMSUNG_TUI_TEST
static uint64_t g_fb_pa;

static void stui_write_signature(void)
{
	uint32_t *kaddr;
	struct page *page;

	page = pfn_to_page(g_fb_pa >> PAGE_SHIFT);
	kaddr = kmap(page);
	if (kaddr) {
		*kaddr = 0x01020304;
		pr_debug("[STUI] kaddr : %pK %x\n", kaddr, *kaddr);
		kunmap(page);
	} else {
		pr_err("[STUI] kmap failed\n");
	}
}
#endif

long stui_process_cmd(struct device *dev, struct file *f, unsigned int cmd, unsigned long arg)
{
	uint32_t ret = STUI_RET_OK;
	/* Handle command */
	switch (cmd) {
	case STUI_HW_IOCTL_START_TUI: {
		struct tui_hw_buffer __user *argp = (struct tui_hw_buffer __user *)arg;
		struct tui_hw_buffer buffer;

		pr_debug("[STUI] STUI_HW_IOCTL_START_TUI called\n");

		if (stui_get_mode() & STUI_MODE_ALL) {
			ret = STUI_RET_ERR_INTERNAL_ERROR;
			break;
		}

		/* allocate TUI frame buffer */
		pr_debug("[STUI] Allocating Framebuffer\n");
		memset(&buffer, 0, sizeof(struct tui_hw_buffer));
		if (stui_alloc_video_space(dev, &buffer)) {
			ret = STUI_RET_ERR_INTERNAL_ERROR;
			break;
		}

#ifdef SAMSUNG_TUI_TEST
		g_fb_pa = buffer.fb_physical;
		stui_write_signature();
#endif

		if (stui_i2c_protect(true) != 0) {
			pr_err("[STUI] stui_i2c_protect failed\n");
			goto clean_fb_prepare;
		}
		stui_set_mask(STUI_MODE_TOUCH_SEC);

		if (copy_to_user(argp, &buffer, sizeof(struct tui_hw_buffer))) {
			pr_err("[STUI] copy_to_user failed\n");
			goto clean_touch_lock;
		}
		/* Prepare display for TUI / Deactivate linux UI drivers */
		if (!stui_prepare_tui()) {
			stui_set_mask(STUI_MODE_DISPLAY_SEC);
			wake_lock(&tui_wakelock);
			break;
		}
clean_touch_lock:
		stui_clear_mask(STUI_MODE_TOUCH_SEC);
		stui_i2c_protect(false);

clean_fb_prepare:
		stui_free_video_space();
		ret = STUI_RET_ERR_INTERNAL_ERROR;
		break;
	}
	case STUI_HW_IOCTL_FINISH_TUI: {
		pr_debug("[STUI] STUI_HW_IOCTL_FINISH_TUI called\n");
		if (stui_get_mode() == STUI_MODE_OFF) {
			ret = STUI_RET_ERR_INTERNAL_ERROR;
			break;
		}
		/* Disable STUI driver / Activate linux UI drivers */
		if (stui_get_mode() & STUI_MODE_DISPLAY_SEC) {
			stui_clear_mask(STUI_MODE_DISPLAY_SEC);
			stui_finish_tui();
			stui_free_video_space();
			wake_unlock(&tui_wakelock);
		}
		if (stui_get_mode() & STUI_MODE_TOUCH_SEC) {
			stui_clear_mask(STUI_MODE_TOUCH_SEC);
			stui_i2c_protect(false);
		}
		stui_set_mode(STUI_MODE_OFF);
		break;
	}
#ifdef SAMSUNG_TUI_TEST
	case STUI_HW_IOCTL_GET_PHYS_ADDR: {
		uint64_t __user *argp = (uint64_t __user *)arg;

		if (copy_to_user(argp, &g_fb_pa, sizeof(uint64_t))) {
			pr_err("[STUI] copy_to_user failed\n");
			ret = STUI_RET_ERR_INTERNAL_ERROR;
		}
		break;
	}
#endif
	case STUI_HW_IOCTL_GET_RESOLUTION: {
		struct tui_hw_buffer __user *argp = (struct tui_hw_buffer __user *)arg;
		struct tui_hw_buffer buffer;

		pr_debug("[STUI] TUI_HW_IOCTL_GET_RESOLUTION called\n");
		memset(&buffer, 0, sizeof(struct tui_hw_buffer));
		if (stui_get_resolution(&buffer)) {
			ret = STUI_RET_ERR_INTERNAL_ERROR;
			break;
		}

		if (copy_to_user(argp, &buffer, sizeof(struct tui_hw_buffer)))
			pr_err("[STUI] copy_to_user failed\n");
		break;
	}
	default:
		pr_err("[STUI] stui_process_cmd(ERROR): Unknown command %d\n", cmd);
		break;
	}
	return ret;
}
