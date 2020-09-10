/* tui/stui_hal.h
 *
 * Samsung TUI HW Handler driver.
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __STUI_HAL_H_
#define __STUI_HAL_H_

#include "stui_ioctl.h"

int stui_i2c_protect(bool is_protect);
int stui_prepare_tui(void);
void stui_finish_tui(void);
void stui_free_video_space(void);
int stui_alloc_video_space(struct device *dev, struct tui_hw_buffer *buffer);
int stui_get_resolution(struct tui_hw_buffer *buffer);

#endif /* __STUI_HAL_H_ */
