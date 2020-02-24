/*
 * Utility functions to work with PROCA certificate
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

#include "proca_log.h"
#include "proca_certificate.h"

#include <linux/slab.h>
#include <linux/string.h>
#include <linux/err.h>
#include <crypto/hash.h>
#include <crypto/sha.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 42)
#include "proca_certificate.asn1.h"
#else
#include "proca_certificate-asn1.h"
#endif

static struct crypto_shash *g_validation_shash;

int proca_certificate_get_app_name(void *context, size_t hdrlen,
				   unsigned char tag,
				   const void *value, size_t vlen)
{
	struct proca_certificate *parsed_cert = context;

	if (!value || !vlen)
		return -EINVAL;

	parsed_cert->app_name = kmalloc(vlen + 1, GFP_KERNEL);
	if (!parsed_cert->app_name)
		return -ENOMEM;

	memcpy(parsed_cert->app_name, value, vlen);
	parsed_cert->app_name[vlen] = '\0';
	parsed_cert->app_name_size = vlen;

	return 0;
}

int proca_certificate_get_five_signature_hash(void *context, size_t hdrlen,
					      unsigned char tag,
					      const void *value, size_t vlen)
{
	struct proca_certificate *parsed_cert = context;

	if (!value || !vlen)
		return -EINVAL;

	parsed_cert->five_signature_hash = kmalloc(vlen, GFP_KERNEL);
	if (!parsed_cert->five_signature_hash)
		return -ENOMEM;

	memcpy(parsed_cert->five_signature_hash, value, vlen);
	parsed_cert->five_signature_hash_size = vlen;

	return 0;
}

int parse_proca_certificate(const char *certificate_buff,
			    const size_t buff_size,
			    struct proca_certificate *parsed_cert)
{
	int rc = 0;

	memset(parsed_cert, 0, sizeof(*parsed_cert));

	rc = asn1_ber_decoder(&proca_certificate_decoder, parsed_cert,
			      certificate_buff,
			      buff_size);
	if (!parsed_cert->app_name || !parsed_cert->five_signature_hash) {
		PROCA_INFO_LOG("Failed to parse proca certificate.\n");
		deinit_proca_certificate(parsed_cert);
		return -EINVAL;
	}

	return rc;
}

void deinit_proca_certificate(struct proca_certificate *certificate)
{
	kfree(certificate->app_name);
	kfree(certificate->five_signature_hash);
}

int init_certificate_validation_hash(void)
{
	g_validation_shash = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(g_validation_shash)) {
		PROCA_WARN_LOG("can't alloc sha256 alg, rc - %ld.\n",
			PTR_ERR(g_validation_shash));
		return PTR_ERR(g_validation_shash);
	}
	return 0;
}

int compare_with_five_signature(const struct proca_certificate *certificate,
				const void *five_signature,
				const size_t five_signature_size)
{
	SHASH_DESC_ON_STACK(sdesc, g_validation_shash);
	char five_sign_hash[SHA256_DIGEST_SIZE];
	int rc = 0;

	if (sizeof(five_sign_hash) != certificate->five_signature_hash_size) {
		PROCA_DEBUG_LOG(
			"Size of five sign hash is invalid %zu, expected %zu",
			 certificate->five_signature_hash_size,
			 sizeof(five_sign_hash));
		return rc;
	}

	sdesc->tfm = g_validation_shash;
	sdesc->flags = 0;

	rc = crypto_shash_init(sdesc);
	if (rc != 0) {
		PROCA_WARN_LOG("crypto_shash_init failed, rc - %d.\n", rc);
		return 0;
	}

	rc = crypto_shash_digest(sdesc, five_signature,
				 five_signature_size, five_sign_hash);
	if (rc != 0) {
		PROCA_WARN_LOG("crypto_shash_digest failed, rc - %d.\n", rc);
		return 0;
	}

	return !memcmp(five_sign_hash,
		       certificate->five_signature_hash,
		       certificate->five_signature_hash_size);
}

int proca_certificate_copy(struct proca_certificate *dst,
			const struct proca_certificate *src)
{
	BUG_ON(!dst || !src);

	memset(dst, 0, sizeof(*dst));

	if (src->app_name) {
		/*
		 * app_name is NULL-terminated string with len == app_name_size,
		 * so we should duplicate app_name_size + 1 bytes
		 */
		dst->app_name = kmemdup(src->app_name, src->app_name_size + 1,
				GFP_KERNEL);
		dst->app_name_size = src->app_name_size;

		if (unlikely(!dst->app_name))
			return -ENOMEM;
	}

	if (src->five_signature_hash) {
		dst->five_signature_hash = kmemdup(
				src->five_signature_hash,
				src->five_signature_hash_size,
				GFP_KERNEL);
		dst->five_signature_hash_size = src->five_signature_hash_size;

		if (unlikely(!dst->five_signature_hash)) {
			kfree(dst->app_name);
			return -ENOMEM;
		}
	}

	return 0;
}
