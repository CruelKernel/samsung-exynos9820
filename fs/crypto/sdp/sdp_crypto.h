/*
 *  Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SDP_CRYPTO_H_
#define SDP_CRYPTO_H_

#include <crypto/aead.h>
#include <linux/crypto.h>
#include <linux/init.h>

#define ROUND_UPX(i, x) (((i)+((x)-1))&~((x)-1))
#define SDP_CRYPTO_RNG_SEED_SIZE 16

/* Definitions for AEAD */
#define AEAD_IV_LEN 12
#define AEAD_AAD_LEN 16
#define AEAD_AUTH_LEN 16
#define AEAD_D32_PACK_DATA_LEN 32
#define AEAD_D64_PACK_DATA_LEN 64
#define AEAD_D32_PACK_TOTAL_LEN (AEAD_IV_LEN + AEAD_D32_PACK_DATA_LEN + AEAD_AUTH_LEN)
#define AEAD_D64_PACK_TOTAL_LEN (AEAD_IV_LEN + AEAD_D64_PACK_DATA_LEN + AEAD_AUTH_LEN)
#define AEAD_DATA_PACK_MAX_LEN AEAD_D64_PACK_TOTAL_LEN

struct __aead_data_32_pack {
	unsigned char iv[AEAD_IV_LEN];
	unsigned char data[AEAD_D32_PACK_DATA_LEN];
	unsigned char auth[AEAD_AUTH_LEN];
};

struct __aead_data_64_pack {
	unsigned char iv[AEAD_IV_LEN];
	unsigned char data[AEAD_D64_PACK_DATA_LEN];
	unsigned char auth[AEAD_AUTH_LEN];
};

/* Default Definitions for AES-GCM crypto */
typedef struct __aead_data_32_pack gcm_pack32;
typedef struct __aead_data_64_pack gcm_pack64;
typedef struct __gcm_pack {
	u32 type;
	u8 *iv;
	u8 *data;
	u8 *auth;
} gcm_pack;

#define SDP_CRYPTO_GCM_PACK32 0x01
#define SDP_CRYPTO_GCM_PACK64 0x02
#define CONV_TYPE_TO_DLEN(x) (x == SDP_CRYPTO_GCM_PACK32 ? \
		AEAD_D32_PACK_DATA_LEN : x == SDP_CRYPTO_GCM_PACK64 ? \
		AEAD_D64_PACK_DATA_LEN : 0)
#define CONV_TYPE_TO_PLEN(x) (x == SDP_CRYPTO_GCM_PACK32 ? \
		AEAD_D32_PACK_TOTAL_LEN : x == SDP_CRYPTO_GCM_PACK64 ? \
		AEAD_D64_PACK_TOTAL_LEN : 0)
#define CONV_DLEN_TO_TYPE(x) (x == AEAD_D32_PACK_DATA_LEN ? \
		SDP_CRYPTO_GCM_PACK32 : x == AEAD_D64_PACK_DATA_LEN ? \
		SDP_CRYPTO_GCM_PACK64 : 0)
#define CONV_PLEN_TO_TYPE(x) (x == AEAD_D32_PACK_TOTAL_LEN ? \
		SDP_CRYPTO_GCM_PACK32 : x == AEAD_D64_PACK_TOTAL_LEN ? \
		SDP_CRYPTO_GCM_PACK64 : 0)
#define SDP_CRYPTO_GCM_MAX_PLEN AEAD_DATA_PACK_MAX_LEN

#define SDP_CRYPTO_GCM_IV_LEN AEAD_IV_LEN
#define SDP_CRYPTO_GCM_AAD_LEN AEAD_AAD_LEN
#define SDP_CRYPTO_GCM_AUTH_LEN AEAD_AUTH_LEN
#define SDP_CRYPTO_GCM_DATA_LEN AEAD_D64_PACK_DATA_LEN
#define SDP_CRYPTO_GCM_DEFAULT_AAD "PROTECTED_BY_SDP" // Explicitly 16 bytes following SDP_CRYPTO_GCM_AAD_LEN
#define SDP_CRYPTO_GCM_DEFAULT_KEY_LEN 32

/* Definitions for Nonce */
#define MAX_EN_BUF_LEN AEAD_D32_PACK_TOTAL_LEN
#define SDP_CRYPTO_NEK_LEN SDP_CRYPTO_GCM_DEFAULT_KEY_LEN
#define SDP_CRYPTO_NEK_DRV_LABEL "NONCE_ENC_KEY"
#define SDP_CRYPTO_NEK_DRV_CONTEXT "NONCE_FOR_FEK"

#define SDP_CRYPTO_SHA512_OUTPUT_SIZE 64

/* Declarations for Open APIs*/
int sdp_crypto_generate_key(void *raw_key, int nbytes);
int sdp_crypto_hash_sha512(const u8 *data, u32 data_len, u8 *hashed);
int sdp_crypto_aes_gcm_encrypt(struct crypto_aead *tfm,
					u8 *data, size_t data_len, u8 *auth, u8 *iv);
int sdp_crypto_aes_gcm_decrypt(struct crypto_aead *tfm,
					u8 *data, size_t data_len, u8 *auth, u8 *iv);
int sdp_crypto_aes_gcm_encrypt_pack(struct crypto_aead *tfm, gcm_pack *pack);
int sdp_crypto_aes_gcm_decrypt_pack(struct crypto_aead *tfm, gcm_pack *pack);
struct crypto_aead *sdp_crypto_aes_gcm_key_setup(const u8 key[], size_t key_len);
void sdp_crypto_aes_gcm_key_free(struct crypto_aead *tfm);
int __init sdp_crypto_init(void);
void __exit sdp_crypto_exit(void);

#endif /* SDP_CRYPTO_H_ */
