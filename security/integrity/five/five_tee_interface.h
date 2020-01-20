/*
 * Interface for TEE Driver
 *
 * Copyright (C) 2016 Samsung Electronics, Inc.
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

#ifndef INTEGRITY_TEE_DRIVER_H
#define INTEGRITY_TEE_DRIVER_H

#include <linux/types.h>
#include <crypto/hash_info.h>

struct five_tee_driver_fns {
	int (*verify_hmac)(enum hash_algo algo,
			   const void *hash, size_t hash_len,
			   const void *label, size_t label_len,
			   const void *signature, size_t signature_len);

	int (*sign_hmac)(enum hash_algo algo,
			 const void *hash, size_t hash_len,
			 const void *label, size_t label_len,
			 void *signature, size_t *signature_len);
};

int register_five_tee_driver(
		struct five_tee_driver_fns *tee_driver_fns);
void unregister_five_tee_driver(void);

#endif
