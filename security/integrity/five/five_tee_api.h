/*
 * FIVE TEE API
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

#ifndef __LINUX_FIVE_TEE_API_H
#define __LINUX_FIVE_TEE_API_H

#include <crypto/hash_info.h>
#include <linux/types.h>

#ifdef CONFIG_FIVE_TEE_DRIVER
int verify_hash(enum hash_algo algo, const void *hash, size_t hash_len,
		const void *label, size_t label_len,
		const void *signature, size_t signature_len);
int sign_hash(enum hash_algo algo, const void *hash, size_t hash_len,
		const void *label, size_t label_len,
		void *signature, size_t *signature_len);
#else
int verify_hash(enum hash_algo algo, const void *hash, size_t hash_len,
		const void *label, size_t label_len,
		const void *signature, size_t signature_len)
{
	return -ENODEV;
}

int sign_hash(enum hash_algo algo, const void *hash, size_t hash_len,
		const void *label, size_t label_len,
		void *signature, size_t *signature_len)
{
	return -ENODEV;
}
#endif /* CONFIG_FIVE_TEE_DRIVER */

#endif /* __LINUX_FIVE_TEE_API_H */
