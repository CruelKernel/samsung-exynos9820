/*
 * FIVE keyring
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Yevgen Kopylov, <y.kopylov@samsung.com>
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

#include <linux/key-type.h>
#include <crypto/public_key.h>
#include <keys/asymmetric-type.h>
#include <crypto/hash_info.h>
#include "five.h"
#include "five_crypto_comp.h"
#include "five_porting.h"

static struct key *five_keyring;

#ifndef CONFIG_FIVE_TRUSTED_KEYRING
static const char *five_keyring_name = "_five";
#else
static const char *five_keyring_name = ".five";
#endif

/*
 * Request an asymmetric key.
 */
static struct key *five_request_asymmetric_key(uint32_t keyid)
{
	struct key *key;
	char name[12];

	snprintf(name, sizeof(name), "id:%08x", keyid);

	pr_debug("key search: \"%s\"\n", name);

	if (five_keyring) {
		/* search in specific keyring */
		key_ref_t kref;

		kref = keyring_search(make_key_ref(five_keyring, 1),
				      &key_type_asymmetric, name, true);
		if (IS_ERR(kref))
			key = ERR_CAST(kref);
		else
			key = key_ref_to_ptr(kref);
	} else {
		return ERR_PTR(-ENOKEY);
	}

	if (IS_ERR(key)) {
		switch (PTR_ERR(key)) {
			/* Hide some search errors */
		case -EACCES:
		case -ENOTDIR:
		case -EAGAIN:
			return ERR_PTR(-ENOKEY);
		default:
			return key;
		}
	}

	pr_debug("%s() = 0 [%x]\n", __func__, key_serial(key));

	return key;
}

static int five_asymmetric_verify(struct five_cert *cert,
				const char *data, int datalen)
{
	struct public_key_signature pks;
	struct five_cert_header *header;
	struct key *key;
	int ret = -ENOMEM;

	header = (struct five_cert_header *)cert->body.header->value;
	if (header->hash_algo >= HASH_ALGO__LAST)
		return -ENOPKG;

	key = five_request_asymmetric_key(__be32_to_cpu(header->key_id));
	if (IS_ERR(key))
		return PTR_ERR(key);

	memset(&pks, 0, sizeof(pks));

	pks.digest = (u8 *)data;
	pks.digest_size = datalen;
	ret = five_verify_signature(key, &pks, cert, header);

	key_put(key);
	pr_debug("%s() = %d\n", __func__, ret);
	return ret;
}

int five_digsig_verify(struct five_cert *cert,
			const char *digest, int digestlen)
{
	if (!five_keyring) {
		five_keyring =
			request_key(&key_type_keyring, five_keyring_name, NULL);
		if (IS_ERR(five_keyring)) {
			int err = PTR_ERR(five_keyring);

			pr_err("no %s keyring: %d\n", five_keyring_name, err);
			five_keyring = NULL;
			return err;
		}
	}

	return five_asymmetric_verify(cert, digest, digestlen);
}

static int __init five_load_x509_from_mem(const char *data, size_t size)
{
	key_ref_t key;
	int rc = 0;

	if (!five_keyring || size == 0)
		return -EINVAL;

	key = key_create_or_update(make_key_ref(five_keyring, 1),
		   "asymmetric",
		   NULL,
		   data,
		   size,
		   ((KEY_POS_ALL & ~KEY_POS_SETATTR) |
		    KEY_USR_VIEW | KEY_USR_READ),
		   KEY_ALLOC_NOT_IN_QUOTA);
	if (IS_ERR(key)) {
		rc = PTR_ERR(key);
		pr_err("Problem loading X.509 certificate (%d): %s\n",
			rc, "built-in");
	} else {
		pr_notice("Loaded X.509 cert '%s': %s\n",
		key_ref_to_ptr(key)->description, "built-in");
		key_ref_put(key);
	}

	return rc;
}

#ifdef CONFIG_FIVE_CERT_ENG
extern char five_local_ca_start_eng[];
extern char five_local_ca_end_eng[];

static int __init five_import_eng_key(void)
{
	size_t size = five_local_ca_end_eng - five_local_ca_start_eng;

	return five_load_x509_from_mem(five_local_ca_start_eng, size);
}
#else
static int __init five_import_eng_key(void)
{
	return 0;
}
#endif

#ifdef CONFIG_FIVE_CERT_USER
extern char five_local_ca_start_user[];
extern char five_local_ca_end_user[];

static int __init five_import_user_key(void)
{
	size_t size = five_local_ca_end_user - five_local_ca_start_user;

	return five_load_x509_from_mem(five_local_ca_start_user, size);
}
#else
static int __init five_import_user_key(void)
{
	return 0;
}
#endif

int __init five_load_built_x509(void)
{
	int rc;

	rc = five_import_eng_key();
	if (rc)
		return rc;

	rc = five_import_user_key();

	return rc;
}

int __init five_keyring_init(void)
{
	const struct cred *cred = current_cred();
	int err = 0;

	five_keyring = five_keyring_alloc(five_keyring_name, KUIDT_INIT(0),
		    KGIDT_INIT(0), cred,
		    ((KEY_POS_ALL & ~KEY_POS_SETATTR) |
		     KEY_USR_VIEW | KEY_USR_READ |
		     KEY_USR_SEARCH),
		    KEY_ALLOC_NOT_IN_QUOTA);
	if (IS_ERR(five_keyring)) {
		err = PTR_ERR(five_keyring);
		pr_info("Can't allocate %s keyring (%d)\n",
		five_keyring_name, err);
		five_keyring = NULL;
	}
	return err;
}
