/* linux/drivers/video/decon_display/s6e3fa0_gamma.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * Haowe Li <haowei.li@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __LCD_CTRL_H__
#define __LCD_CTRL_H__

#include "decon_lcd.h"

void lcd_init(int id, struct decon_lcd *lcd);
void lcd_enable(int id);
void lcd_disable(int id);
int lcd_gamma_ctrl(int id, unsigned int backlightlevel);
int lcd_gamma_update(int id);
int lcd_dump(int id);

#endif /* __LCD_CTRL_H__ */
