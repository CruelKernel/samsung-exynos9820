/*
 * TEE Driver
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

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/firmware.h>

#include "tee_client_api.h"
#include "five_ta_uuid.h"
#include "five_tee_interface.h"
#include "teec_operation.h"

#ifdef CONFIG_TEE_DRIVER_DEBUG
#include <linux/uaccess.h>
#endif

static DEFINE_MUTEX(itee_driver_lock);
static char is_initialized;
static TEEC_Context *context;
static TEEC_Session *session;
static DEFINE_SPINLOCK(tee_msg_lock);
static LIST_HEAD(tee_msg_queue);
struct task_struct *tee_msg_task;

#define MAX_HASH_LEN 64
/* Maximum length of data integrity label.
 * This limit is applied because:
 * 1. TEEgris doesn't support signing data longer than 480 bytes;
 * 2. The label's length is limited to 3965 byte according to the data
 * transmission protocol between five_tee_driver and TA.
 */
#define FIVE_LABEL_MAX_LEN 256

struct tci_msg {
	uint8_t hash_algo;
	uint8_t hash[MAX_HASH_LEN];
	uint8_t signature[MAX_HASH_LEN];
	uint16_t label_len;
	uint8_t label[0];
} __attribute__((packed));

#define CMD_SIGN   1
#define CMD_VERIFY 2

#define SEND_CMD_RETRY 1

static int load_trusted_app(void);
static void unload_trusted_app(void);
static int send_cmd(unsigned int cmd,
		enum hash_algo algo,
		const void *hash,
		size_t hash_len,
		const void *label,
		size_t label_len,
		void *signature,
		size_t *signature_len);
static int send_cmd_with_retry(unsigned int cmd,
		enum hash_algo algo,
		const void *hash,
		size_t hash_len,
		const void *label,
		size_t label_len,
		void *signature,
		size_t *signature_len);

struct tee_msg {
	struct completion *comp;
	unsigned int cmd;
	enum hash_algo algo;
	const void *hash;
	size_t hash_len;
	const void *label;
	size_t label_len;
	void *signature;
	size_t *signature_len;
	int rc;
	struct list_head queue;
};

static int tee_msg_thread(void *arg)
{
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop()) {
			set_current_state(TASK_RUNNING);
			break;
		}
		if (list_empty(&tee_msg_queue))
			schedule();
		set_current_state(TASK_RUNNING);

		spin_lock(&tee_msg_lock);
		while (!list_empty(&tee_msg_queue)) {
			struct tee_msg *send_cmd_args;
			int rc;

			send_cmd_args = list_entry(tee_msg_queue.next,
					    struct tee_msg, queue);
			list_del_init(&send_cmd_args->queue);
			spin_unlock(&tee_msg_lock);

			rc = send_cmd_with_retry(send_cmd_args->cmd,
					send_cmd_args->algo,
					send_cmd_args->hash,
					send_cmd_args->hash_len,
					send_cmd_args->label,
					send_cmd_args->label_len,
					send_cmd_args->signature,
					send_cmd_args->signature_len);
			send_cmd_args->rc = rc;
			// when processing tee_iovec comp is not NULL
			// only for last cmd in array
			if (send_cmd_args->comp)
				complete(send_cmd_args->comp);
			spin_lock(&tee_msg_lock);
		}
		spin_unlock(&tee_msg_lock);
	}
	mutex_lock(&itee_driver_lock);
	unload_trusted_app();
	mutex_unlock(&itee_driver_lock);

	return 0;
}

static int send_cmd(unsigned int cmd,
		enum hash_algo algo,
		const void *hash,
		size_t hash_len,
		const void *label,
		size_t label_len,
		void *signature,
		size_t *signature_len)
{
	TEEC_Operation operation = {};
	TEEC_SharedMemory shmem = {0};
	TEEC_Result rc;
	uint32_t origin;
	struct tci_msg *msg = NULL;
	size_t msg_len;
	size_t sig_len;
	const bool inout_direction = cmd == CMD_SIGN ? true : false;

	if (!hash || !hash_len ||
			!signature || !signature_len || !(*signature_len))
		return -EINVAL;

	msg_len = sizeof(*msg) + label_len;
	if (label_len > FIVE_LABEL_MAX_LEN || msg_len > PAGE_SIZE)
		return -EINVAL;

	switch (algo) {
	case HASH_ALGO_SHA1:
		if (hash_len != SHA1_DIGEST_SIZE)
			return -EINVAL;
		sig_len = SHA1_DIGEST_SIZE;
		break;
	case HASH_ALGO_SHA256:
		if (hash_len != SHA256_DIGEST_SIZE)
			return -EINVAL;
		sig_len = SHA256_DIGEST_SIZE;
		break;
	case HASH_ALGO_SHA512:
		if (hash_len != SHA512_DIGEST_SIZE)
			return -EINVAL;
		sig_len = SHA512_DIGEST_SIZE;
		break;
	default:
		return -EINVAL;
	}

	if (cmd == CMD_SIGN && sig_len > *signature_len)
		return -EINVAL;
	if (cmd == CMD_VERIFY && sig_len != *signature_len)
		return -EINVAL;

	mutex_lock(&itee_driver_lock);
	if (!is_initialized) {
		rc = load_trusted_app();
		pr_info("FIVE: Initialize trusted app, ret: %d\n", rc);
		if (rc) {
			mutex_unlock(&itee_driver_lock);
			rc = -ESRCH;
			goto out;
		}
	}

	shmem.buffer = NULL;
	shmem.size = msg_len;
	shmem.flags = TEEC_MEM_INPUT;
	if (inout_direction)
		shmem.flags |= TEEC_MEM_OUTPUT;

	rc = TEEC_AllocateSharedMemory(context, &shmem);
	if (rc != TEEC_SUCCESS || shmem.buffer == NULL) {
		mutex_unlock(&itee_driver_lock);
		pr_err("FIVE: TEEC_AllocateSharedMemory is failed rc=%d\n", rc);
		rc = -ENOMEM;
		goto out;
	}

	msg = (struct tci_msg *)shmem.buffer;

	msg->hash_algo = algo;
	memcpy(msg->hash, hash, hash_len);
	msg->label_len = label_len;
	if (label_len)
		memcpy(msg->label, label, label_len);

	if (cmd == CMD_VERIFY)
		memcpy(msg->signature, signature, sig_len);

	FillOperationSharedMem(&shmem, &operation, inout_direction);

	rc = TEEC_InvokeCommand(session, cmd, &operation, &origin);

	mutex_unlock(&itee_driver_lock);

	if (rc == TEEC_SUCCESS) {
		if (origin != TEEC_ORIGIN_TRUSTED_APP) {
			rc = -EIO;
			pr_err("FIVE: TEEC_InvokeCommand is failed rc=%d origin=%u\n",
								rc, origin);
		}
	} else {
		pr_err("FIVE: TEEC_InvokeCommand is failed rc=%d origin=%u\n",
								rc, origin);
	}

	if (rc == TEEC_SUCCESS && cmd == CMD_SIGN) {
		memcpy(signature, msg->signature, sig_len);
		*signature_len = sig_len;
	}

	TEEC_ReleaseSharedMemory(&shmem);
out:
	return rc;
}

static int send_cmd_kthread(unsigned int cmd,
		enum hash_algo algo,
		const void *hash,
		size_t hash_len,
		const void *label,
		size_t label_len,
		void *signature,
		size_t *signature_len)
{
	struct completion cmd_sent;
	struct tee_msg cmd_msg;

	init_completion(&cmd_sent);

	cmd_msg.comp = &cmd_sent;
	cmd_msg.cmd = cmd;
	cmd_msg.algo = algo;
	cmd_msg.hash = hash;
	cmd_msg.hash_len = hash_len;
	cmd_msg.label = label;
	cmd_msg.label_len = label_len;
	cmd_msg.signature = signature;
	cmd_msg.signature_len = signature_len;
	cmd_msg.rc = -EBADMSG;

	spin_lock(&tee_msg_lock);
	list_add_tail(&cmd_msg.queue, &tee_msg_queue);
	spin_unlock(&tee_msg_lock);
	wake_up_process(tee_msg_task);
	wait_for_completion(&cmd_sent);
	return cmd_msg.rc;
}

static int send_cmd_with_retry(unsigned int cmd,
				enum hash_algo algo,
				const void *hash,
				size_t hash_len,
				const void *label,
				size_t label_len,
				void *signature,
				size_t *signature_len)
{
	int rc;
	unsigned int retry_num = SEND_CMD_RETRY;

	do {
		bool need_retry = false;

		rc = send_cmd(cmd, algo,
				hash, hash_len, label, label_len,
						signature, signature_len);

		need_retry = (rc == TEEC_ERROR_COMMUNICATION ||
						rc == TEEC_ERROR_TARGET_DEAD);
		if (need_retry && retry_num) {
			pr_err("FIVE: TA got the fatal error rc=%d. Try again\n",
								rc);
			mutex_lock(&itee_driver_lock);
			unload_trusted_app();
			mutex_unlock(&itee_driver_lock);
		} else {
			break;
		}
	} while (retry_num--);

	if (rc == TEEC_ERROR_ACCESS_DENIED)
		pr_err("FIVE: TA got TEEC_ERROR_ACCESS_DENIED rc=%d\n", rc);

	return rc;
}

static int verify_hmac(const struct tee_iovec *verify_args)
{
	return send_cmd_kthread(CMD_VERIFY, verify_args->algo,
				verify_args->hash, verify_args->hash_len,
				verify_args->label, verify_args->label_len,
				verify_args->signature,
				(size_t *)&verify_args->signature_len);
}

static int sign_hmac(struct tee_iovec *sign_args)
{
	return send_cmd_kthread(CMD_SIGN, sign_args->algo,
				sign_args->hash, sign_args->hash_len,
				sign_args->label, sign_args->label_len,
				sign_args->signature,
				&sign_args->signature_len);
}

static int load_trusted_app(void)
{
	TEEC_Result rc;
	uint32_t origin;

	context = kzalloc(sizeof(*context), GFP_KERNEL);
	if (!context) {
		pr_err("FIVE: Can't allocate context\n");
		goto error;
	}

	rc = TEEC_InitializeContext(NULL, context);
	if (rc) {
		pr_err("FIVE: Can't initialize context rc=%d\n", rc);
		goto error;
	}

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session) {
		pr_err("FIVE: Can't allocate session\n");
		goto error;
	}

	rc = TEEC_OpenSession(context, session,
				&five_ta_uuid, 0, NULL, NULL, &origin);
	if (rc) {
		pr_err("FIVE: Can't open session rc=%d origin=%u\n",
								rc, origin);
		goto error;
	}

	is_initialized = 1;

	return 0;
error:
	TEEC_FinalizeContext(context);
	kfree(session);
	kfree(context);
	session = NULL;
	context = NULL;

	return -1;
}

static int register_tee_driver(void)
{
	struct five_tee_driver_fns fn = {
		.verify_hmac = verify_hmac,
		.sign_hmac = sign_hmac,
	};

	return register_five_tee_driver(&fn);
}

static void unregister_tee_driver(void)
{
	unregister_five_tee_driver();
	/* Don't close session with TA when       */
	/* tee_integrity_driver has been unloaded */
}

static void unload_trusted_app(void)
{
	is_initialized = 0;
	TEEC_CloseSession(session);
	TEEC_FinalizeContext(context);
	kfree(session);
	kfree(context);
	session = NULL;
	context = NULL;
}

#ifdef CONFIG_TEE_DRIVER_DEBUG

static ssize_t tee_driver_write(
		struct file *file, const char __user *buf,
		size_t count, loff_t *pos)
{
	uint8_t hash[SHA1_DIGEST_SIZE] = {};
	uint8_t signature[SHA1_DIGEST_SIZE] = {};
	char command;
	size_t signature_len = sizeof(signature);

	if (get_user(command, buf))
		return -EFAULT;

	switch (command) {
	case '1':
		pr_info("register_tee_driver: %d\n", register_tee_driver());
		break;
	case '2': {
		struct tee_iovec sign_args = {
			.algo = HASH_ALGO_SHA1,
			.hash = hash,
			.hash_len = sizeof(hash),
			.signature = signature,
			.signature_len = signature_len,
			.label_len = sizeof("label"),
			.label = "label"
		};

		pr_info("sign_hmac: %d\n", sign_hmac(&sign_args));
		break;
	}
	case '3': {
		struct tee_iovec verify_args = {
			.algo = HASH_ALGO_SHA1,
			.hash = hash,
			.hash_len = sizeof(hash),
			.signature = signature,
			.signature_len = signature_len,
			.label_len = sizeof("label"),
			.label = "label"
		};

		pr_info("verify_hmac: %d\n", verify_hmac(&verify_args));
		break;
	}
	case '4':
		pr_info("unregister_tee_driver\n");
		unregister_tee_driver();
		mutex_lock(&itee_driver_lock);
		unload_trusted_app();
		mutex_unlock(&itee_driver_lock);
		break;
	default:
		pr_err("FIVE: %s: unknown cmd: %hhx\n", __func__, command);
		return -EINVAL;
	}

	return count;
}

static const struct file_operations tee_driver_fops = {
	.owner = THIS_MODULE,
	.write = tee_driver_write
};

static int __init init_fs(void)
{
	struct dentry *debug_file = NULL;
	umode_t umode =
		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	debug_file = debugfs_create_file(
			"integrity_tee_driver", umode, NULL, NULL,
							    &tee_driver_fops);
	if (IS_ERR_OR_NULL(debug_file))
		goto error;

	return 0;
error:
	if (debug_file)
		return -PTR_ERR(debug_file);

	return -EEXIST;
}
#else
static inline int __init init_fs(void)
{
	return 0;
}
#endif

static int __init tee_driver_init(void)
{
	int rc = 0;

	mutex_init(&itee_driver_lock);

#ifdef CONFIG_FIVE_EARLY_LOAD_TRUSTED_APP
	rc = load_trusted_app();
	pr_info("FIVE: Initialize trusted app in early boot ret: %d\n", rc);
#endif

	tee_msg_task = kthread_run(tee_msg_thread, NULL, "five_tee_msg_thread");
	if (IS_ERR(tee_msg_task)) {
		rc = PTR_ERR(tee_msg_task);
		pr_err("FIVE: Can't create tee_msg_task: %d\n", rc);
		goto out;
	}
	rc = register_tee_driver();
	if (rc) {
		pr_err("FIVE: Can't register tee_driver\n");
		goto out;
	}
	rc = init_fs();
	if (rc) {
		pr_err("FIVE: Can't initialize debug FS\n");
		goto out;
	}

out:
	return rc;
}

static void __exit tee_driver_exit(void)
{
	unregister_tee_driver();
	kthread_stop(tee_msg_task);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FIVE TEE Driver");

module_init(tee_driver_init);
module_exit(tee_driver_exit);
