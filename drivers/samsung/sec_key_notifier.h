/*
 * drivers/debug/sec_key_notifier.h
 *
 * COPYRIGHT(C) 2016 Samsung Electronics Co., Ltd. All Right Reserved.
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

#ifndef __SEC_KEY_NOTIFIER_H__
#define __SEC_KEY_NOTIFIER_H__

struct sec_key_notifier_param {
	unsigned int keycode;
	int down;
};

int sec_kn_register_notifier(struct notifier_block *nb,
		const unsigned int *events, const size_t nr_events);

int sec_kn_unregister_notifier(struct notifier_block *nb,
		const unsigned int *events, const size_t nr_events);

#endif /* __SEC_KEY_NOTIFIER_H__ */
