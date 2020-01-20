/*
 * PROCA certificate
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Hryhorii Tur, <hryhorii.tur@partner.samsung.com>
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

#ifndef _LINUX_PROCA_CERTIFICATE_H
#define _LINUX_PROCA_CERTIFICATE_H

#include <linux/proca.h>

int parse_proca_certificate(const char *certificate_buff,
			    const size_t buff_size,
			    struct proca_certificate *parsed_cert);

void deinit_proca_certificate(struct proca_certificate *certificate);

int compare_with_five_signature(const struct proca_certificate *certificate,
				const void *five_signature,
				const size_t five_signature_size);

int init_certificate_validation_hash(void);

int proca_certificate_copy(struct proca_certificate *dst,
			const struct proca_certificate *src);

#endif //_LINUX_PROCA_CERTIFICATE_H
