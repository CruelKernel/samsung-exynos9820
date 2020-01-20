/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
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

#include <linux/cred.h>
#include <linux/crypto.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <crypto/sha.h>

#include "sysdep.h"
#include "tz_cred.h"
#include "tzdev.h"

static const struct tz_uuid kernel_client_uuid = {
	0xca36da22, 0xba15, 0x573e, {0x83, 0x23, 0x8c, 0x91, 0x72, 0x15, 0xe9, 0x9e}
};

static void tz_format_uuid_v5(struct tz_uuid *uuid, const uint8_t hash[UUID_SIZE])
{
	/* convert UUID to local byte order */
	memcpy(uuid, hash, sizeof(struct tz_uuid));
	uuid->time_low = ntohl(uuid->time_low);
	uuid->time_mid = ntohs(uuid->time_mid);
	uuid->time_hi_and_version = ntohs(uuid->time_hi_and_version);

	/* put in the variant and version bits */
	uuid->time_hi_and_version &= UUID_TIME_HI_MASK;
	uuid->time_hi_and_version |= (UUID_VERSION << UUID_VERSION_SHIFT);
	uuid->clock_seq_and_node[0] &= UUID_CLOCK_SEC_MASK;
	/* according to rfc4122 the two most significant bits
	 * (bits 6 and 7) of the clockSeq must be zero and one, respectively */
	uuid->clock_seq_and_node[0] |= 0x80;
}

static int tz_format_cred_user(struct tz_cred *cred)
{
	struct scatterlist sg;
	uint8_t hash[SHA1_DIGEST_SIZE];
	char *pathname;
	char *p;
	size_t len;
	int ret = 0;

	pathname = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!pathname)
		return -ENOMEM;

	down_read(&current->mm->mmap_sem);
	p = d_path(&current->mm->exe_file->f_path, pathname, PATH_MAX);
	up_read(&current->mm->mmap_sem);

	if (IS_ERR(p)) {
		ret = PTR_ERR(p);
		goto out;
	}

	len = strlen(p);

	/* Calculate hash of process ELF pathname. We calculate SHA1 hash of
	 * pathname string and truncate 32 bytes to 20 ones. This truncated hash
	 * is being fixed by tz_iwsock_format_uuid_v5 to present valid UUID. */
	sg_init_one(&sg, p, len);
	ret = sysdep_crypto_sha1(hash, &sg, p, len);
	if (ret)
		goto out;

	cred->pid = current->tgid;
	cred->uid = __kuid_val(current_uid());
	cred->gid = __kgid_val(current_gid());

	tz_format_uuid_v5(&cred->uuid, hash);
out:
	kfree(pathname);

	return ret;
}

static int tz_format_cred_kernel(struct tz_cred *cred)
{
	cred->pid = current->real_parent->pid;
	cred->uid = __kuid_val(current_uid());
	cred->gid = __kgid_val(current_gid());

	memcpy(&cred->uuid, &kernel_client_uuid, sizeof(struct tz_uuid));

	return 0;
}

int tz_format_cred(struct tz_cred *cred)
{
	BUILD_BUG_ON(sizeof(pid_t) != sizeof(cred->pid));
	BUILD_BUG_ON(sizeof(uid_t) != sizeof(cred->uid));
	BUILD_BUG_ON(sizeof(gid_t) != sizeof(cred->gid));

	return current->mm ? tz_format_cred_user(cred) : tz_format_cred_kernel(cred);
}
