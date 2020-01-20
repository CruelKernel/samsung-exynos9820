/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _LINUX_DEK_AES_H
#define _LINUX_DEK_AES_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>

int dek_aes_encrypt(kek_t *kek, unsigned char *src, unsigned char *dst, int len);
int dek_aes_decrypt(kek_t *kek, unsigned char *src, unsigned char *dst, int len);
int dek_aes_encrypt_key(kek_t *kek, unsigned char *key, unsigned int key_len,
						unsigned char *out, unsigned int *out_len);
int dek_aes_decrypt_key(kek_t *kek, unsigned char *ekey, unsigned int ekey_len,
						unsigned char *out, unsigned int *out_len);
int dek_aes_encrypt_key_raw(unsigned char *kek, unsigned int kek_len,
							unsigned char *key, unsigned int key_len,
							unsigned char *out, unsigned int *out_len);
int dek_aes_decrypt_key_raw(unsigned char *kek, unsigned int kek_len,
							unsigned char *ekey, unsigned int ekey_len,
							unsigned char *out, unsigned int *out_len);

#endif
