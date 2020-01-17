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
 */

#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/random.h>
#include <linux/err.h>
#include <sdp/dek_common.h>
#include <sdp/dek_ioctl.h>
#include <sdp/dek_aes.h>
#include <sdp/kek_pack.h>

/*
 * Need to move this to defconfig
 */
#define CONFIG_PUB_CRYPTO
//#define CONFIG_SDP_IOCTL_PRIV

#ifdef CONFIG_PUB_CRYPTO
#include <sdp/pub_crypto_emul.h>
#endif

#define DEK_LOG_COUNT		100
#define DEK_LOG_BUF_SIZE	1024
#define LOG_ENTRY_BUF_SIZE	512

//#if defined(CONFIG_SDP) && !defined(CONFIG_FSCRYPT_SDP)
//extern void ecryptfs_mm_drop_cache(int userid, int engineid);
//#endif

/* Log buffer */
struct log_struct {
	int len;
	char buf[DEK_LOG_BUF_SIZE];
	struct list_head list;
	spinlock_t list_lock;
};
struct log_struct log_buffer;
static int log_count;

/* Wait queue */
wait_queue_head_t wq;
static int flag;

static struct workqueue_struct *queue_log_workqueue;
typedef struct __log_entry_t {
	int engineId;
	char buffer[LOG_ENTRY_BUF_SIZE];
	struct work_struct work;
} log_entry_t;

int dek_is_sdp_uid(uid_t uid)
{
	int userid = uid / PER_USER_RANGE;

	return is_kek_pack(userid);
}
EXPORT_SYMBOL(dek_is_sdp_uid);

int is_system_server(void)
{
	uid_t uid = from_kuid(&init_user_ns, current_uid());

	switch (uid) {
#if 0
	case 0: //root
		DEK_LOGD("allowing root to access SDP device files\n");
#endif
	case 1000:
		return 1;
	default:
		break;
	}

	return 0;
}

int is_root(void)
{
	uid_t uid = from_kuid(&init_user_ns, current_uid());

	switch (uid) {
	case 0: //root
		//DEK_LOGD("allowing root to access SDP device files\n");
		return 1;
	default:
		;
	}

	return 0;
}

int is_current_adbd(void)
{
	DEK_LOGD("current->comm : %s\n", current->comm);
#if 1
	if (is_root()) {
		// epmd/vold are 4 length string
		if (strlen(current->comm) == 4)
			if (strcmp(current->comm, "adbd"))
				return 1;
	}

	return 0;
#else
	return 1;
#endif
}

int is_current_epmd(void)
{
	DEK_LOGD("current->comm : %s\n", current->comm);
#if 1
	if (is_root()) {
		// epmd/vold are 4 length string
		if (strlen(current->comm) == 4)
			if (strcmp(current->comm, "vold") ||  strcmp(current->comm, "epmd"))
				return 1;
	}

	return 0;
#else
	return 1;
#endif
}

static int zero_out(char *buf, unsigned int len)
{
	char zero = 0;
	int retry_cnt = 3;
	int i;

retry:
	if (retry_cnt > 0) {
		memset((void *)buf, zero, len);
		for (i = 0; i < len; ++i) {
			zero |= buf[i];
			if (zero) {
				DEK_LOGE("the memory was not properly zeroed\n");
				retry_cnt--;
				goto retry;
			}
		}
	} else {
		DEK_LOGE("FATAL : can't zero out!!\n");
		return -1;
	}

	return 0;
}

static int dek_open_evt(struct inode *inode, struct file *file)
{
	return 0;
}

static int dek_release_evt(struct inode *ignored, struct file *file)
{
	return 0;
}

static int dek_open_req(struct inode *inode, struct file *file)
{
	return 0;
}

static int dek_release_req(struct inode *ignored, struct file *file)
{
	return 0;
}

#if DEK_DEBUG
void hex_key_dump(const char* tag, uint8_t *data, size_t data_len)
{
	static const char *hex = "0123456789ABCDEF";
	static const char delimiter = ' ';
	int i;
	char *buf;
	size_t buf_len;

	if (tag == NULL || data == NULL || data_len <= 0) {
		return;
	}

	buf_len = data_len * 3;
	buf = (char *)kmalloc(buf_len, GFP_ATOMIC);
	if (buf == NULL) {
		return;
	}
	for (i= 0 ; i < data_len ; i++) {
		buf[i*3 + 0] = hex[(data[i] >> 4) & 0x0F];
		buf[i*3 + 1] = hex[(data[i]) & 0x0F];
		buf[i*3 + 2] = delimiter;
	}
	buf[buf_len - 1] = '\0';
	printk(KERN_ERR
		"[%s] %s(len=%d) : %s\n", "DEK_DBG", tag, data_len, buf);
	kfree(buf);
}
#endif

#ifdef CONFIG_SDP_KEY_DUMP
void key_dump(unsigned char *buf, int len)
{
	int i;

	printk("len=%d: ", len);
	for (i = 0; i < len; ++i) {
		if ((i%16) == 0)
			printk("\n");
		printk("%02X ", (unsigned char)buf[i]);
	}
	printk("\n");
}

static void kek_dump(int engine_id, int kek_type, const char *kek_name)
{
	kek_t *kek;
	int ret;

	kek = get_kek(engine_id, kek_type, &ret);
	if (kek) {
		printk("dek: %s: ", kek_name);
		key_dump(kek->buf, kek->len);
		put_kek(kek);
	} else {
		printk("dek: %s: empty\n", kek_name);
	}
}

static void dump_all_keys(int engine_id)
{
	kek_dump(engine_id, KEK_TYPE_SYM, "KEK_TYPE_SYM");
	kek_dump(engine_id, KEK_TYPE_RSA_PUB, "KEK_TYPE_RSA_PUB");
	kek_dump(engine_id, KEK_TYPE_RSA_PRIV, "KEK_TYPE_RSA_PRIV");
	kek_dump(engine_id, KEK_TYPE_DH_PUB, "KEK_TYPE_DH_PUB");
	kek_dump(engine_id, KEK_TYPE_DH_PRIV, "KEK_TYPE_DH_PRIV");
	kek_dump(engine_id, KEK_TYPE_ECDH256_PUB, "KEK_TYPE_ECDH256_PUB");
	kek_dump(engine_id, KEK_TYPE_ECDH256_PRIV, "KEK_TYPE_ECDH256_PRIV");
}
#endif

int dek_is_locked(int engine_id)
{
	if (is_kek(engine_id, KEK_TYPE_SYM))
		return 0;

	return 1;
}

int dek_generate_dek(int engine_id, dek_t *newDek)
{
	newDek->len = DEK_LEN;
	get_random_bytes(newDek->buf, newDek->len);

	if (newDek->len <= 0 || newDek->len > DEK_LEN) {
		zero_out((char *)newDek, sizeof(dek_t));
		return -EFAULT;
	}
#ifdef CONFIG_SDP_KEY_DUMP
	else {
		if (get_sdp_sysfs_key_dump()) {
			DEK_LOGD("DEK: ");
			key_dump(newDek->buf, newDek->len);
		}
	}
#endif

	return 0;
}

int dek_encrypt_dek_ecdh(dek_t *plainDek, dek_t *encDek, kek_t *drv_key)
{
	int res;
	int enc_type;
	unsigned int enc_plen;
	u8 enc_pbuf[SDP_CRYPTO_GCM_MAX_PLEN];

	ss_payload sspay;
	dek_t in;
	dek_t out;

	enc_type = CONV_DLEN_TO_TYPE(plainDek->len);
	if (unlikely(!enc_type))
		return -EINVAL;

#if DEK_DEBUG
	hex_key_dump("enc_ecdh: plainDek", (u8 *)plainDek, sizeof(dek_t));
#endif

	// Update Dummy DEK into input dek
	in.type = DEK_TYPE_DUMMY,
	in.len = 0;

	// Derive shared secret
	res = ecdh_deriveSS(&in, &out, drv_key);
	if (res || out.len != sizeof(ss_payload))
		return -EINVAL;

	// Update ss_payload
	memcpy(&sspay, out.buf, out.len);
	res = dek_aes_encrypt_key_raw(
			sspay.ss.buf, sspay.ss.len, plainDek->buf, plainDek->len, enc_pbuf, &enc_plen);
	if (res)
		goto clean;

	/*
	 * Update encDek which will be stored into file system
	 *
	 * Blob Format : [ DataK.pub | GCM Pack   ]
	 *               [ dhkt_t    | gcm_pack32 ]
	 *            or [ dhkt_t    | gcm_pack64 ]
	 */
	memset(encDek->buf, 0, DEK_MAXLEN);
	memcpy(encDek->buf, &sspay.dh, sizeof(dhk_t));
	memcpy((u8 *)encDek->buf + sizeof(dhk_t), enc_pbuf, enc_plen);
	encDek->len = sizeof(dhk_t) + enc_plen;
	encDek->type = DEK_TYPE_ECDH256_ENC;

#if DEK_DEBUG
	hex_key_dump("enc_ecdh: encDek", (u8 *)encDek, sizeof(dek_t));
#endif

clean:
	memset(sspay.ss.buf, 0, sspay.ss.len);
	memset(out.buf, 0, out.len);

	return res;
}

static int dek_encrypt_dek(int engine_id, dek_t *plainDek, dek_t *encDek)
{
	int ret = 0;
	kek_t *kek;

#ifdef CONFIG_SDP_KEY_DUMP
	if (get_sdp_sysfs_key_dump()) {
		DEK_LOGD("plainDek from user: ");
		key_dump(plainDek->buf, plainDek->len);
	}
#endif

	kek = get_kek(engine_id, KEK_TYPE_SYM, &ret);
	if (kek) {
		ret = dek_aes_encrypt_key(kek, plainDek->buf, plainDek->len, encDek->buf, &encDek->len);
		if (ret) {
			DEK_LOGE("aes encrypt failed\n");
			dek_add_to_log(engine_id, "aes encrypt failed");
		} else {
			encDek->type = DEK_TYPE_AES_ENC;
		}
		put_kek(kek);
	} else {
#ifdef CONFIG_PUB_CRYPTO
		/*
		 * Do an asymmetric crypto
		 */
		switch (get_sdp_sysfs_asym_alg()) {
		case SDPK_ALGOTYPE_ASYMM_ECDH:
			kek = get_kek(engine_id, KEK_TYPE_ECDH256_PUB, &ret);
			if (kek) {
				ret = dek_encrypt_dek_ecdh(plainDek, encDek, kek);
				put_kek(kek);
				break;
			} else{
				if (ret == -EACCES)
					return ret;

				DEK_LOGE("no KEK_TYPE_ECDH256_PUB : %d\n", engine_id);
				dek_add_to_log(engine_id, "encrypt failed, no KEK_TYPE_ECDH256_PUB");
			}
		// no ECDH, try DH
		/* no break */
		case SDPK_ALGOTYPE_ASYMM_DH:
			kek = get_kek(engine_id, KEK_TYPE_DH_PUB, &ret);
			if (kek) {
				ret = dh_encryptDEK(plainDek, encDek, kek);
				put_kek(kek);
				break;
			} else{
				if (ret == -EACCES)
					return ret;

				DEK_LOGE("no KEK_TYPE_DH_PUB : %d\n", engine_id);
				dek_add_to_log(engine_id, "encrypt failed, no KEK_TYPE_DH_PUB");
			}
		// no DH, try RSA
		/* no break */
		case SDPK_ALGOTYPE_ASYMM_RSA:
			kek = get_kek(engine_id, KEK_TYPE_RSA_PUB, &ret);
			if (kek) {
				ret = rsa_encryptByPub(plainDek, encDek, kek);
				put_kek(kek);
				break;
			} else{
				if (ret == -EACCES)
					return ret;

				DEK_LOGE("no KEK_TYPE_RSA_PUB : %d\n", engine_id);
				dek_add_to_log(engine_id, "encrypt failed, no KEK_TYPE_RSA_PUB");
			}
		// no RSA, return error;
		/* no break */
		default:
			DEK_LOGE("no ASYMM algo registered : %d\n", engine_id);
			dek_add_to_log(engine_id, "no ASYMM algo supported");
			return -EOPNOTSUPP;
		}
#else
		DEK_LOGE("pub crypto not supported : %d\n", engine_id);
		dek_add_to_log(engine_id, "encrypt failed, no key");
		return -EOPNOTSUPP;
#endif
	}

	if (ret)
		return ret;

	if (encDek->len <= 0 || encDek->len > DEK_MAXLEN) {
		DEK_LOGE("dek_encrypt_dek, incorrect len=%d\n", encDek->len);
		zero_out((char *)encDek, sizeof(dek_t));
		return -EFAULT;
	}
#ifdef CONFIG_SDP_KEY_DUMP
	else {
		if (get_sdp_sysfs_key_dump()) {
			DEK_LOGD("encDek to user: ");
			key_dump(encDek->buf, encDek->len);
		}
	}
#endif

	return 0;
}

int dek_encrypt_dek_efs(int engine_id, dek_t *plainDek, dek_t *encDek)
{
	return dek_encrypt_dek(engine_id, plainDek, encDek);
}

int dek_decrypt_dek_ecdh(dek_t *encDek, dek_t *plainDek, kek_t *drv_key)
{
	int res;
	int dec_type;
	unsigned int dec_plen;
	u8 dec_pbuf[SDP_CRYPTO_GCM_MAX_PLEN];

	ssk_t ss;
	dek_t in;
	dek_t out;

	dec_plen = encDek->len - sizeof(dhk_t);
	dec_type = CONV_PLEN_TO_TYPE(dec_plen);
	if (unlikely(!dec_type))
		return -EINVAL;

#if DEK_DEBUG
	hex_key_dump("dec_ecdh: encDek", (u8 *)encDek, sizeof(dek_t));
#endif

	// Update DataK.pub into input dek
	memcpy(in.buf, encDek->buf, sizeof(dhk_t));
	in.type = DEK_TYPE_ECDH256_ENC;
	in.len = sizeof(dhk_t);

	// Derive shared secret
	res = ecdh_deriveSS(&in, &out, drv_key);
	if (res || out.len != sizeof(ssk_t))
		return -EINVAL;

	// Update ss_payload
	memcpy(&ss, out.buf, out.len);

	// Update GCM pack buffer
	memcpy(dec_pbuf, (u8 *)encDek->buf + sizeof(dhk_t), dec_plen);

	// Clean up plain key buffer
	memset(plainDek->buf, 0, DEK_MAXLEN);

	res = dek_aes_decrypt_key_raw(
			ss.buf, ss.len, dec_pbuf, dec_plen, plainDek->buf, &plainDek->len);
	if (res)
		goto clean;

	plainDek->type = DEK_TYPE_PLAIN;

#if DEK_DEBUG
	hex_key_dump("dec_ecdh: plainDek->buf", plainDek->buf,plainDek->len);
#endif

clean:
	memset(ss.buf, 0, ss.len);
	memset(out.buf, 0, out.len);

	return res;
}

static int dek_decrypt_dek(int engine_id, dek_t *encDek, dek_t *plainDek)
{
	int dek_type = encDek->type;
	kek_t *kek = NULL;
	int ret = 0;

#ifdef CONFIG_SDP_KEY_DUMP
	if (get_sdp_sysfs_key_dump()) {
		DEK_LOGD("encDek from user: ");
		key_dump(encDek->buf, encDek->len);
	}
#endif
	switch (dek_type) {
	case DEK_TYPE_AES_ENC:
	{
		kek = get_kek(engine_id, KEK_TYPE_SYM, &ret);
		if (kek) {
			ret = dek_aes_decrypt_key(kek, encDek->buf, encDek->len, plainDek->buf, &plainDek->len);
			if (ret) {
				DEK_LOGE("aes decrypt failed\n");
				dek_add_to_log(engine_id, "aes decrypt failed");
			} else {
				plainDek->type = DEK_TYPE_PLAIN;
			}
			put_kek(kek);
		} else {
			DEK_LOGE("no KEK_TYPE_SYM for id: %d\n", engine_id);
			dek_add_to_log(engine_id, "decrypt failed, no KEK_TYPE_SYM");
			return -EIO;
		}
		return ret;
	}
	case DEK_TYPE_RSA_ENC:
	{
#ifdef CONFIG_PUB_CRYPTO
		kek = get_kek(engine_id, KEK_TYPE_RSA_PRIV, &ret);
		if (kek) {
			ret = rsa_decryptByPair(encDek, plainDek, kek);
			put_kek(kek);
		} else{
			DEK_LOGE("no KEK_TYPE_RSA_PRIV for id: %d\n", engine_id);
			dek_add_to_log(engine_id, "decrypt failed, no KEK_TYPE_RSA_PRIV");
			return -EIO;
		}
#else
		DEK_LOGE("Not supported key type: %d\n", encDek->type);
		dek_add_to_log(engine_id, "decrypt failed, DH type not supported");
		return -EOPNOTSUPP;
#endif
		return ret;
	}
	case DEK_TYPE_DH_ENC:
	{
#ifdef CONFIG_PUB_CRYPTO
		kek = get_kek(engine_id, KEK_TYPE_DH_PRIV, &ret);
		if (kek) {
			ret = dh_decryptEDEK(encDek, plainDek, kek);
			put_kek(kek);
		} else{
			DEK_LOGE("no KEK_TYPE_DH_PRIV for id: %d\n", engine_id);
			dek_add_to_log(engine_id, "decrypt failed, no KEK_TYPE_DH_PRIV");
			return -EIO;
		}
#else
		DEK_LOGE("Not supported key type: %d\n", encDek->type);
		dek_add_to_log(engine_id, "decrypt failed, DH type not supported");
		return -EOPNOTSUPP;
#endif
		return ret;
	}
	case DEK_TYPE_ECDH256_ENC:
	{
#ifdef CONFIG_PUB_CRYPTO
		kek = get_kek(engine_id, KEK_TYPE_ECDH256_PRIV, &ret);
		if (kek) {
			ret = dek_decrypt_dek_ecdh(encDek, plainDek, kek);
			put_kek(kek);
		} else{
			DEK_LOGE("no KEK_TYPE_ECDH256_PRIV for id: %d\n", engine_id);
			dek_add_to_log(engine_id, "decrypt failed, no KEK_TYPE_ECDH256_PRIV");
			return -EIO;
		}
#else
		DEK_LOGE("Not supported key type: %d\n", encDek->type);
		dek_add_to_log(engine_id, "decrypt failed, ECDH type not supported");
		return -EOPNOTSUPP;
#endif
		return ret;
	}
	default:
	{
		DEK_LOGE("Unsupported edek type: %d\n", encDek->type);
		dek_add_to_log(engine_id, "decrypt failed, unsupported key type");
		return -EFAULT;
	}
	}
}

int dek_decrypt_dek_efs(int engine_id, dek_t *encDek, dek_t *plainDek)
{
	return dek_decrypt_dek(engine_id, encDek, plainDek);
}

int dek_encrypt_fek(unsigned char *master_key, unsigned int master_key_len,
					unsigned char *fek, unsigned int fek_len,
					unsigned char *efek, unsigned int *efek_len)
{
	return dek_aes_encrypt_key_raw(master_key, master_key_len,
								fek, fek_len,
								efek, efek_len);
}

int dek_decrypt_fek(unsigned char *master_key, unsigned int master_key_len,
					unsigned char *efek, unsigned int efek_len,
					unsigned char *fek, unsigned int *fek_len)
{
	return dek_aes_decrypt_key_raw(master_key, master_key_len,
								efek, efek_len,
								fek, fek_len);
}

static int dek_on_boot(dek_arg_on_boot *evt)
{
	int ret = 0;
	int engine_id = evt->engine_id;
	int user_id = evt->user_id;

	if ((evt->SDPK_Rpub.len > KEK_MAXLEN) ||
		(evt->SDPK_Dpub.len > KEK_MAXLEN) ||
		(evt->SDPK_EDpub.len > KEK_MAXLEN)) {
		DEK_LOGE("Invalid args\n");
		DEK_LOGE("SDPK_Rpub.len : %d\n", evt->SDPK_Rpub.len);
		DEK_LOGE("SDPK_Dpub.len : %d\n", evt->SDPK_Dpub.len);
		DEK_LOGE("SDPK_EDpub.len : %d\n", evt->SDPK_EDpub.len);
	return -EINVAL;
	}

	if (!is_kek_pack(engine_id)) {
		ret = add_kek_pack(engine_id, user_id);
		if (ret && ret != -EEXIST) {
			DEK_LOGE("add_kek_pack failed\n");
			return ret;
		}

		ret = 0;
		add_kek(engine_id, &evt->SDPK_Rpub);
		add_kek(engine_id, &evt->SDPK_Dpub);
		add_kek(engine_id, &evt->SDPK_EDpub);

#ifdef CONFIG_SDP_KEY_DUMP
		if (get_sdp_sysfs_key_dump()) {
			dump_all_keys(engine_id);
		}
#endif
	}

	return ret;
}

static int dek_on_device_locked(dek_arg_on_device_locked *evt)
{
//#if defined(CONFIG_SDP) && !defined(CONFIG_FSCRYPT_SDP)
//	int user_id = evt->user_id;
//#endif
	int engine_id = evt->engine_id;

	del_kek(engine_id, KEK_TYPE_SYM);
	del_kek(engine_id, KEK_TYPE_RSA_PRIV);
	del_kek(engine_id, KEK_TYPE_DH_PRIV);
	del_kek(engine_id, KEK_TYPE_ECDH256_PRIV);

//#if defined(CONFIG_SDP) && !defined(CONFIG_FSCRYPT_SDP)
//	ecryptfs_mm_drop_cache(user_id, engine_id);
//#endif

#ifdef CONFIG_FSCRYPT_SDP
	fscrypt_sdp_cache_drop_inode_mappings(engine_id);
#endif

#ifdef CONFIG_SDP_KEY_DUMP
	if (get_sdp_sysfs_key_dump()) {
		dump_all_keys(engine_id);
	}
#endif

	return 0;
}

static int dek_on_device_unlocked(dek_arg_on_device_unlocked *evt)
{
	int engine_id = evt->engine_id;

	if ((evt->SDPK_sym.len > KEK_MAXLEN) ||
		(evt->SDPK_Rpri.len > KEK_MAXLEN) ||
		(evt->SDPK_Dpri.len > KEK_MAXLEN) ||
			(evt->SDPK_EDpri.len > KEK_MAXLEN)) {
		DEK_LOGE("%s Invalid args\n", __func__);
		DEK_LOGE("SDPK_sym.len : %d\n", evt->SDPK_sym.len);
		DEK_LOGE("SDPK_Rpri.len : %d\n", evt->SDPK_Rpri.len);
		DEK_LOGE("SDPK_Dpri.len : %d\n", evt->SDPK_Dpri.len);
		DEK_LOGE("SDPK_EDpri.len : %d\n", evt->SDPK_EDpri.len);
		return -EINVAL;
	}

	add_kek(engine_id, &evt->SDPK_sym);
	add_kek(engine_id, &evt->SDPK_Rpri);
	add_kek(engine_id, &evt->SDPK_Dpri);
	add_kek(engine_id, &evt->SDPK_EDpri);

#ifdef CONFIG_SDP_KEY_DUMP
	if (get_sdp_sysfs_key_dump()) {
		dump_all_keys(engine_id);
	}
#endif

	return 0;
}

static int dek_on_user_added(dek_arg_on_user_added *evt)
{
	int ret;
	int engine_id = evt->engine_id;
	int user_id = evt->user_id;

	if ((evt->SDPK_Rpub.len > KEK_MAXLEN) ||
		(evt->SDPK_Dpub.len > KEK_MAXLEN) ||
		(evt->SDPK_EDpub.len > KEK_MAXLEN)) {
		DEK_LOGE("Invalid args\n");
		DEK_LOGE("SDPK_Rpub.len : %d\n", evt->SDPK_Rpub.len);
		DEK_LOGE("SDPK_Dpub.len : %d\n", evt->SDPK_Dpub.len);
		DEK_LOGE("SDPK_EDpub.len : %d\n", evt->SDPK_EDpub.len);
		return -EINVAL;
	}

	ret = add_kek_pack(engine_id, user_id);
	if (ret && ret != -EEXIST) {
		DEK_LOGE("add_kek_pack failed\n");
		return ret;
	}

	ret = 0;
	add_kek(engine_id, &evt->SDPK_Rpub);
	add_kek(engine_id, &evt->SDPK_Dpub);
	add_kek(engine_id, &evt->SDPK_EDpub);

#ifdef CONFIG_SDP_KEY_DUMP
	if (get_sdp_sysfs_key_dump()) {
		dump_all_keys(engine_id);
	}
#endif

	return ret;
}

static int dek_on_user_removed(dek_arg_on_user_removed *evt)
{
	del_kek_pack(evt->engine_id);

	return 0;
}

// I'm thinking... if minor id can represent persona id

static long dek_do_ioctl_evt(unsigned int minor, unsigned int cmd,
		unsigned long arg) {
	long ret = 0;
	void __user *ubuf = (void __user *)arg;
	void *cleanup = NULL;
	unsigned int size = 0;

	switch (cmd) {
	/*
	 * Event while booting.
	 *
	 * This event comes per persona, the driver is responsible to
	 * verify things good whether it's compromised.
	 */
	case DEK_ON_BOOT: {
		dek_arg_on_boot *evt = kzalloc(sizeof(dek_arg_on_boot), GFP_KERNEL);

		DEK_LOGD("DEK_ON_BOOT\n");

		if (evt == NULL) {
			ret = -ENOMEM;
			goto err;
		}
		cleanup = evt;
		size = sizeof(dek_arg_on_boot);

		if (copy_from_user(evt, ubuf, size)) {
			DEK_LOGE("can't copy from user evt\n");
			ret = -EFAULT;
			goto err;
		}
		ret = dek_on_boot(evt);
		if (ret < 0) {
			dek_add_to_log(evt->engine_id, "Boot failed");
			goto err;
		}
		dek_add_to_log(evt->engine_id, "Booted");
		break;
	}
	/*
	 * Event when device is locked.
	 *
	 * Nullify private key which prevents decryption.
	 */
	case DEK_ON_DEVICE_LOCKED: {
		dek_arg_on_device_locked *evt = kzalloc(sizeof(dek_arg_on_device_locked), GFP_KERNEL);

		DEK_LOGD("DEK_ON_DEVICE_LOCKED\n");
		if (evt == NULL) {
			ret = -ENOMEM;
			goto err;
		}
		cleanup = evt;
		size = sizeof(dek_arg_on_device_locked);

		if (copy_from_user(evt, ubuf, size)) {
			DEK_LOGE("can't copy from user evt\n");
			ret = -EFAULT;
			goto err;
		}
		ret = dek_on_device_locked(evt);
		if (ret < 0) {
			dek_add_to_log(evt->engine_id, "Lock failed");
			goto err;
		}
		dek_add_to_log(evt->engine_id, "Locked");
		break;
	}
	/*
	 * Event when device unlocked.
	 *
	 * Read private key and decrypt with user password.
	 */
	case DEK_ON_DEVICE_UNLOCKED: {
		dek_arg_on_device_unlocked *evt = kzalloc(sizeof(dek_arg_on_device_unlocked), GFP_KERNEL);

		DEK_LOGD("DEK_ON_DEVICE_UNLOCKED\n");
		if (evt == NULL) {
			ret = -ENOMEM;
			goto err;
		}
		cleanup = evt;
		size = sizeof(dek_arg_on_device_unlocked);

		if (copy_from_user(evt, ubuf, size)) {
			DEK_LOGE("can't copy from user evt\n");
			ret = -EFAULT;
			goto err;
		}
		ret = dek_on_device_unlocked(evt);
		if (ret < 0) {
			dek_add_to_log(evt->engine_id, "Unlock failed");
			goto err;
		}
		dek_add_to_log(evt->engine_id, "Unlocked");
		break;
	}
	/*
	 * Event when new user(persona) added.
	 *
	 * Generate RSA public key and encrypt private key with given
	 * password. Also pub-key and encryped priv-key have to be stored
	 * in a file system.
	 */
	case DEK_ON_USER_ADDED: {
		dek_arg_on_user_added *evt = kzalloc(sizeof(dek_arg_on_user_added), GFP_KERNEL);

		DEK_LOGD("DEK_ON_USER_ADDED\n");
		if (evt == NULL) {
			ret = -ENOMEM;
			goto err;
		}
		cleanup = evt;
		size = sizeof(dek_arg_on_user_added);

		if (copy_from_user(evt, ubuf, size)) {
			DEK_LOGE("can't copy from user evt\n");
			ret = -EFAULT;
			goto err;
		}
		ret = dek_on_user_added(evt);
		if (ret < 0) {
			dek_add_to_log(evt->engine_id, "Add user failed");
			goto err;
		}
		dek_add_to_log(evt->engine_id, "Added user");
		break;
	}
	/*
	 * Event when user is removed.
	 *
	 * Remove pub-key file & encrypted priv-key file.
	 */
	case DEK_ON_USER_REMOVED: {
		dek_arg_on_user_removed *evt = kzalloc(sizeof(dek_arg_on_user_removed), GFP_KERNEL);

		DEK_LOGD("DEK_ON_USER_REMOVED\n");
		if (evt == NULL) {
			ret = -ENOMEM;
			goto err;
		}
		cleanup = evt;
		size = sizeof(dek_arg_on_user_removed);

		if (copy_from_user(evt, ubuf, size)) {
			DEK_LOGE("can't copy from user evt\n");
			ret = -EFAULT;
			goto err;
		}
		ret = dek_on_user_removed(evt);
		if (ret < 0) {
			dek_add_to_log(evt->engine_id, "Remove user failed");
			goto err;
		}
		dek_add_to_log(evt->engine_id, "Removed user");
		break;
	}
	/*
	 * Event when password changed.
	 *
	 * Encrypt SDPK_Rpri with new password and store it.
	 */
	case DEK_ON_CHANGE_PASSWORD: {
		DEK_LOGD("DEK_ON_CHANGE_PASSWORD << deprecated. SKIP\n");
		ret = 0;
		dek_add_to_log(0, "Changed password << deprecated");
		break;
	}

	case DEK_DISK_CACHE_CLEANUP: {
		dek_arg_disk_cache_cleanup *evt = kzalloc(sizeof(dek_arg_disk_cache_cleanup), GFP_KERNEL);

		DEK_LOGD("DEK_DISK_CACHE_CLEANUP\n");
		if (evt == NULL) {
			ret = -ENOMEM;
			goto err;
		}
		cleanup = evt;
		size = sizeof(dek_arg_disk_cache_cleanup);

		if (copy_from_user(evt, ubuf, size)) {
			DEK_LOGE("can't copy from user evt\n");
			ret = -EFAULT;
			goto err;
		}

//#if defined(CONFIG_SDP) && !defined(CONFIG_FSCRYPT_SDP)
//		ecryptfs_mm_drop_cache(evt->user_id, evt->engine_id);
//#endif
#ifdef CONFIG_FSCRYPT_SDP
		fscrypt_sdp_cache_drop_inode_mappings(evt->engine_id);
#endif
		ret = 0;
		dek_add_to_log(evt->engine_id, "Disk cache clean up");
		break;
	}
	default:
		DEK_LOGE("%s case default\n", __func__);
		ret = -EINVAL;
		break;
	}

err:
	if (cleanup) {
		zero_out((char *)cleanup, size);
		kfree(cleanup);
	}
	return ret;
}

static long dek_do_ioctl_req(unsigned int minor, unsigned int cmd,
		unsigned long arg) {
	long ret = 0;
	void __user *ubuf = (void __user *)arg;

	switch (cmd) {
	case DEK_IS_KEK_AVAIL: {
	dek_arg_is_kek_avail req;

	DEK_LOGD("DEK_IS_KEK_AVAIL\n");

	memset(&req, 0, sizeof(dek_arg_is_kek_avail));
	if (copy_from_user(&req, ubuf, sizeof(req))) {
		DEK_LOGE("can't copy from user\n");
		ret = -EFAULT;
		goto err;
	}

	req.ret = is_kek_available(req.engine_id, req.kek_type);
	if (req.ret < 0) {
		DEK_LOGE("is_kek_available(id:%d, kek:%d) error\n",
			req.engine_id, req.kek_type);
		ret = -ENOENT;
		goto err;
	}

	if (copy_to_user(ubuf, &req, sizeof(req))) {
		DEK_LOGE("can't copy to user req\n");
		zero_out((char *)&req, sizeof(dek_arg_is_kek_avail));
		ret = -EFAULT;
		goto err;
	}

	ret = 0;
	}
	break;
	/*
	 * Request to generate DEK.
	 * Generate DEK and return to the user
	 */
	case DEK_GENERATE_DEK: {
		dek_arg_generate_dek req;

		DEK_LOGD("DEK_GENERATE_DEK\n");

		memset(&req, 0, sizeof(dek_arg_generate_dek));
		if (copy_from_user(&req, ubuf, sizeof(req))) {
			DEK_LOGE("can't copy from user req\n");
			ret = -EFAULT;
			goto err;
		}
		dek_generate_dek(req.engine_id, &req.dek);
		if (copy_to_user(ubuf, &req, sizeof(req))) {
			DEK_LOGE("can't copy to user req\n");
			zero_out((char *)&req, sizeof(dek_arg_generate_dek));
			ret = -EFAULT;
			goto err;
		}
		zero_out((char *)&req, sizeof(dek_arg_generate_dek));
		break;
	}
	/*
	 * Request to encrypt given DEK.
	 *
	 * encrypt dek and return to the user
	 */
	case DEK_ENCRYPT_DEK: {
		dek_arg_encrypt_dek req;
		dek_t *tempPlain_dek, *tempEnc_dek;
		DEK_LOGD("DEK_ENCRYPT_DEK\n");

		memset(&req, 0, sizeof(dek_arg_encrypt_dek));
		if(copy_from_user(&req, ubuf, sizeof(req))) {
			DEK_LOGE("can't copy from user req\n");
			zero_out((char *)&req, sizeof(dek_arg_encrypt_dek));
			ret = -EFAULT;
			goto err;
		}
		if(req.plain_dek.len <= 0 || req.plain_dek.len > DEK_MAXLEN) {
			DEK_LOGE("Incorrect dek len\n");
			zero_out((char *)&req, sizeof(dek_arg_encrypt_dek));
			ret = -EFAULT;
			goto err;
		}
		tempPlain_dek = kmalloc(sizeof(dek_t), GFP_NOFS);
		if (tempPlain_dek == NULL) {
			return -ENOMEM;
		}
		tempEnc_dek = kmalloc(sizeof(dek_t), GFP_NOFS);
		if (tempEnc_dek == NULL) {
			kzfree(tempPlain_dek);
			return -ENOMEM;
		}

		tempPlain_dek->type = req.plain_dek.type;
		tempPlain_dek->len = req.plain_dek.len;
		memcpy(tempPlain_dek->buf, req.plain_dek.buf, req.plain_dek.len);

		ret = dek_encrypt_dek(req.engine_id,
				tempPlain_dek, tempEnc_dek);

		memset(tempPlain_dek->buf, 0, DEK_MAXLEN);

		if (ret < 0) {
			DEK_LOGE("DEK_ENCRYPT_DEK: failed to encrypt dek! (err:%d)\n", ret);
			zero_out((char *)&req, sizeof(dek_arg_encrypt_dek));
			kzfree(tempPlain_dek);
			kzfree(tempEnc_dek);
			goto err;
		}

		req.enc_dek.type = tempEnc_dek->type;
		req.enc_dek.len = tempEnc_dek->len;
		memcpy(req.enc_dek.buf, tempEnc_dek->buf, tempEnc_dek->len);

		if(copy_to_user(ubuf, &req, sizeof(req))) {
			DEK_LOGE("can't copy to user req\n");
			zero_out((char *)&req, sizeof(dek_arg_encrypt_dek));
			ret = -EFAULT;
			kzfree(tempPlain_dek);
			kzfree(tempEnc_dek);
			goto err;
		}
		zero_out((char *)&req, sizeof(dek_arg_encrypt_dek));

		kzfree(tempPlain_dek);
		kzfree(tempEnc_dek);

		break;
	}
	/*
	 * Request to decrypt given DEK.
	 *
	 * Decrypt dek and return to the user.
	 * When device is locked, private key is not available, so
	 * the driver must return EPERM or some kind of error.
	 */
	case DEK_DECRYPT_DEK: {
		dek_arg_decrypt_dek req;
		dek_t *tempPlain_dek, *tempEnc_dek;

		DEK_LOGD("DEK_DECRYPT_DEK\n");

		memset(&req, 0, sizeof(dek_arg_decrypt_dek));
		if(copy_from_user(&req, ubuf, sizeof(req))) {
			DEK_LOGE("can't copy from user req\n");
			zero_out((char *)&req, sizeof(dek_arg_decrypt_dek));
			ret = -EFAULT;
			goto err;
		}
		if(req.enc_dek.len <= 0 || req.enc_dek.len > DEK_MAXLEN) {
			DEK_LOGE("Incorrect dek len\n");
			zero_out((char *)&req, sizeof(dek_arg_decrypt_dek));
			ret = -EFAULT;
			goto err;
		}
		tempPlain_dek = kmalloc(sizeof(dek_t), GFP_NOFS);
		if (tempPlain_dek == NULL) {
			return -ENOMEM;
		}
		tempEnc_dek = kmalloc(sizeof(dek_t), GFP_NOFS);
		if (tempEnc_dek == NULL) {
			kzfree(tempPlain_dek);
			return -ENOMEM;
		}		
		tempEnc_dek->type = req.enc_dek.type;
		tempEnc_dek->len = req.enc_dek.len;
		memcpy(tempEnc_dek->buf, req.enc_dek.buf, req.enc_dek.len);

		ret = dek_decrypt_dek(req.engine_id,
				tempEnc_dek, tempPlain_dek);

		if (ret < 0) {
			DEK_LOGE("DEK_DECRYPT_DEK: failed to decrypt dek! (err:%d)\n", ret);
			zero_out((char *)&req, sizeof(dek_arg_decrypt_dek));
			kzfree(tempPlain_dek);
			kzfree(tempEnc_dek);
			goto err;
		}

		req.plain_dek.type = tempPlain_dek->type;
		req.plain_dek.len = tempPlain_dek->len;
		memcpy(req.plain_dek.buf, tempPlain_dek->buf, tempPlain_dek->len);
		memset(tempPlain_dek->buf, 0, DEK_MAXLEN);

		if(copy_to_user(ubuf, &req, sizeof(req))) {
			DEK_LOGE("can't copy to user req\n");
			zero_out((char *)&req, sizeof(dek_arg_decrypt_dek));
			ret = -EFAULT;
			kzfree(tempPlain_dek);
			kzfree(tempEnc_dek);
			goto err;
		}
		zero_out((char *)&req, sizeof(dek_arg_decrypt_dek));

		kzfree(tempPlain_dek);
		kzfree(tempEnc_dek);
		break;
	}

	default:
		DEK_LOGE("%s case default\n", __func__);
		ret = -EINVAL;
		break;
	}

	return ret;
err:
	return ret;
}

int is_kek_available(int engine_id, int kek_type)
{
	return is_kek(engine_id, kek_type);
}

static long dek_ioctl_evt(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	unsigned int minor;

	if (!is_system_server()) {
		DEK_LOGE("Current process can't access evt device\n");
#if 0
		/* TODO: fix build issue first */
		DEK_LOGE("Current process info :: uid=%u gid=%u euid=%u egid=%u suid=%u sgid=%u "
				"fsuid=%u fsgid=%u\n",
				current_uid(), current_gid(), current_euid(),
				current_egid(), current_suid(), current_sgid(),
				current_fsuid(), current_fsgid());
#endif
		dek_add_to_log(000, "Access denied to evt device");
		return -EACCES;
	}

	minor = iminor(file->f_path.dentry->d_inode);
	return dek_do_ioctl_evt(minor, cmd, arg);
}

static long dek_ioctl_req(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	unsigned int minor;
#if 0
	if (!is_container_app() && !is_root()) {
		DEK_LOGE("Current process can't access req device\n");
		DEK_LOGE("Current process info :: uid=%u gid=%u euid=%u egid=%u suid=%u sgid=%u "
				"fsuid=%u fsgid=%u\n",
				current_uid(), current_gid(), current_euid(),
				current_egid(), current_suid(), current_sgid(),
				current_fsuid(), current_fsgid());
		dek_add_to_log(000, "Access denied to req device");
		return -EACCES;
	}
#endif

	minor = iminor(file->f_path.dentry->d_inode);
	return dek_do_ioctl_req(minor, cmd, arg);
}

/*
 * DAR engine log
 */

static int dek_open_log(struct inode *inode, struct file *file)
{
	DEK_LOGD("dek_open_log\n");
	return 0;
}

static int dek_release_log(struct inode *ignored, struct file *file)
{
	DEK_LOGD("dek_release_log\n");
	return 0;
}

static ssize_t dek_read_log(struct file *file, char __user *buffer, size_t len, loff_t *off)
{
	int ret = 0;
	struct log_struct *tmp = NULL;
	char log_buf[DEK_LOG_BUF_SIZE];
	int log_buf_len;

	if (list_empty(&log_buffer.list)) {
		DEK_LOGD("process %i (%s) going to sleep\n",
				current->pid, current->comm);
		flag = 0;
		wait_event_interruptible(wq, flag != 0);

	}
	flag = 0;

	spin_lock(&log_buffer.list_lock);
	if (!list_empty(&log_buffer.list)) {
		tmp = list_first_entry(&log_buffer.list, struct log_struct, list);
		memcpy(&log_buf, tmp->buf, tmp->len);
		log_buf_len = tmp->len;
		list_del(&tmp->list);
		kfree(tmp);
		log_count--;
		spin_unlock(&log_buffer.list_lock);

		ret = copy_to_user(buffer, log_buf, log_buf_len);
		if (ret) {
			DEK_LOGE("dek_read_log, copy_to_user fail, ret=%d, len=%d\n",
					ret, log_buf_len);
			return -EFAULT;
		}
		len = log_buf_len;
		*off = log_buf_len;
	} else {
		spin_unlock(&log_buffer.list_lock);
		DEK_LOGD("dek_read_log, list empty\n");
		len = 0;
	}

	return len;
}

void queue_log_work(struct work_struct *log_work)
{
	log_entry_t *logStruct = container_of(log_work, log_entry_t, work);
	int engine_id = logStruct->engineId;
	char *buffer = logStruct->buffer;
	struct timespec ts;
	struct log_struct *tmp = (struct log_struct *)kmalloc(sizeof(struct log_struct), GFP_KERNEL);

	if (tmp) {
		INIT_LIST_HEAD(&tmp->list);

		getnstimeofday(&ts);
		tmp->len = sprintf(tmp->buf, "%ld.%.3ld|%d|%s|%d|%s\n",
				(long)ts.tv_sec,
				(long)ts.tv_nsec / 1000000,
				current->pid,
				current->comm,
				engine_id,
				buffer);

		spin_lock(&log_buffer.list_lock);
		list_add_tail(&(tmp->list), &(log_buffer.list));
		log_count++;
		if (log_count > DEK_LOG_COUNT) {
			DEK_LOGD("dek_add_to_log - exceeded DEK_LOG_COUNT\n");
			tmp = list_first_entry(&log_buffer.list, struct log_struct, list);
			list_del(&tmp->list);
			kfree(tmp);
			log_count--;
		}
		spin_unlock(&log_buffer.list_lock);

		DEK_LOGD("process %i (%s) awakening the readers, log_count=%d\n",
				current->pid, current->comm, log_count);
		flag = 1;
		wake_up_interruptible(&wq);
	} else {
		DEK_LOGE("dek_add_to_log - failed to allocate buffer\n");
	}

	kfree(logStruct);
}

void dek_add_to_log(int engine_id, char *buffer)
{
	log_entry_t *temp = (log_entry_t *)kmalloc(sizeof(log_entry_t), GFP_ATOMIC);
	int len;
	if (!temp) {
		DEK_LOGE("failed to allocate memory for log entry\n");
		return;
	}
	temp->engineId = engine_id;
	len = (strlen(buffer) > LOG_ENTRY_BUF_SIZE) ? (LOG_ENTRY_BUF_SIZE - 1) : strlen(buffer);
	memcpy(temp->buffer, buffer, len);
	temp->buffer[len] = '\0';
	INIT_WORK(&temp->work, queue_log_work);
	queue_work(queue_log_workqueue, &temp->work);
}

const struct file_operations dek_fops_evt = {
		.owner = THIS_MODULE,
		.open = dek_open_evt,
		.release = dek_release_evt,
		.unlocked_ioctl = dek_ioctl_evt,
		.compat_ioctl = dek_ioctl_evt,
};

static struct miscdevice dek_misc_evt = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "dek_evt",
		.fops = &dek_fops_evt,
};

const struct file_operations dek_fops_req = {
		.owner = THIS_MODULE,
		.open = dek_open_req,
		.release = dek_release_req,
		.unlocked_ioctl = dek_ioctl_req,
		.compat_ioctl = dek_ioctl_req,
};

static struct miscdevice dek_misc_req = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "dek_req",
		.fops = &dek_fops_req,
};

const struct file_operations dek_fops_log = {
		.owner = THIS_MODULE,
		.open = dek_open_log,
		.release = dek_release_log,
		.read = dek_read_log,
};

static struct miscdevice dek_misc_log = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "dek_log",
		.fops = &dek_fops_log,
};

static int __init dek_init(void)
{
	int ret;

	ret = misc_register(&dek_misc_evt);
	if (unlikely(ret)) {
		DEK_LOGE("failed to register misc_evt device!\n");
		return ret;
	}
	ret = misc_register(&dek_misc_req);
	if (unlikely(ret)) {
		DEK_LOGE("failed to register misc_req device!\n");
		return ret;
	}

	ret = dek_create_sysfs_asym_alg(dek_misc_req.this_device);
	if (unlikely(ret)) {
		DEK_LOGE("failed to create sysfs_asym_alg device!\n");
		return ret;
	}

	ret = misc_register(&dek_misc_log);
	if (unlikely(ret)) {
		DEK_LOGE("failed to register misc_log device!\n");
		return ret;
	}

	ret = dek_create_sysfs_key_dump(dek_misc_log.this_device);
	if (unlikely(ret)) {
		DEK_LOGE("failed to create sysfs_key_dump device!\n");
		return ret;
	}

	queue_log_workqueue = alloc_workqueue("queue_log_workqueue", WQ_HIGHPRI, 0);
	if (!queue_log_workqueue) {
		DEK_LOGE("failed to allocate queue_log_workqueue\n");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&log_buffer.list);
	spin_lock_init(&log_buffer.list_lock);
	init_waitqueue_head(&wq);

	init_kek_pack();

	printk("dek: initialized\n");
	dek_add_to_log(000, "Initialized");

#ifdef CONFIG_FSCRYPT_SDP
	fscrypt_sdp_cache_init();
#endif

	return 0;
}

static void __exit dek_exit(void)
{
	printk("dek: unloaded\n");
}

module_init(dek_init)
module_exit(dek_exit)

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SDP DEK");
