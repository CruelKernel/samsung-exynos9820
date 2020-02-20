/*
 * include/linux/vib_notifier.h
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __VIB_NOTIFIER_H__
#define __VIB_NOTIFIER_H__

#define VIB_NOTIFIER_OFF	(0)
#define VIB_NOTIFIER_ON	(1)

extern int vib_notifier_register(struct notifier_block *n);
extern int vib_notifier_unregister(struct notifier_block *nb);
extern int vib_notifier_notify(void);
#if IS_ENABLED(CONFIG_VIB_NOTIFIER_TEST)
extern void vib_notifier_test(void);
#endif

#endif /* __VIB_NOTIFIER_H__ */
