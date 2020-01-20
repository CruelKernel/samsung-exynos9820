/*
 * fscrypt_ddar.c
 *
 *  Created on: Oct 15, 2018
 *      Author: olic.moon
 */

#include "fscrypt_private.h"

extern int dd_submit_bio(struct dd_info *info, struct bio *bio);

int dd_test_and_inherit_context(
		struct fscrypt_context *ctx,
		struct inode *parent, struct inode *child,
		struct fscrypt_info *ci)
{
	// check if parent directory or file is ddar protected
	if (ci && ci->ci_dd_info) {
		dd_verbose("policy.flag:%x", ci->ci_dd_info->policy.flags);
		ctx->knox_flags |= (ci->ci_dd_info->policy.flags << FSCRYPT_KNOX_FLG_DDAR_SHIFT) & FSCRYPT_KNOX_FLG_DDAR_MASK;

		return dd_create_crypt_context(child, &ci->ci_dd_info->policy);
	} else {
		return 0;
	}
}

int update_encryption_context_with_dd_policy(
		struct inode *inode,
		const struct dd_policy *policy)
{
	struct fscrypt_context ctx;
	int ret;

	dd_info("update encryption context with dd policy ino:%ld flag:%x\n", inode->i_ino, policy->flags);

	/**
	 * i_rwsem needs to be acquired while setting policy so that new files
	 * cannot be created in the directory during or after the empty_dir() check
	 */
	inode_lock(inode);

	ret = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (ret == -ENODATA) {
		dd_error("failed to set dd policy. empty fscrypto context\n");
		ret = -EFAULT;
	} else if (ret == sizeof(ctx)) {
		ctx.knox_flags |= policy->flags << FSCRYPT_KNOX_FLG_DDAR_SHIFT & FSCRYPT_KNOX_FLG_DDAR_MASK;
		dd_verbose("fscrypt_context.knox_flag:0x%08x\n", ctx.knox_flags);
		ret = inode->i_sb->s_cop->set_context(inode, &ctx, sizeof(ctx), NULL);
	} else {
		dd_error("failed to set dd policy. get_context rc:%d\n", ret);
		ret = -EEXIST;
	}

	ret = dd_create_crypt_context(inode, policy);

	inode_unlock(inode);

	if (!ret) {
		struct fscrypt_info	*ci = inode->i_crypt_info;
		ci->ci_dd_info = alloc_dd_info(inode);
		if (IS_ERR(ci->ci_dd_info)) {
			dd_error("%s - failed to alloc dd info:%ld\n", __func__, inode->i_ino);
			ret = -ENOMEM;
			ci->ci_dd_info = NULL;
		}
	}
	return ret;
}

int dd_oem_page_crypto_inplace(struct dd_info *info, struct page *page, int dir)
{
	return fscrypt_do_page_crypto(info->inode,
			(dir == WRITE) ? FS_ENCRYPT:FS_DECRYPT, 0, page, page, PAGE_SIZE, 0, GFP_NOFS);
}

int fscrypt_dd_encrypted_inode(const struct inode *inode)
{
	struct fscrypt_info *ci = NULL;
	if (!inode)
		return 0;

	ci = inode->i_crypt_info;
	if (!S_ISREG(inode->i_mode))
		return 0;

	if (ci && ci->ci_dd_info) {
		if (dd_policy_encrypted(ci->ci_dd_info->policy.flags))
			return 1;
	}

	return 0;
}

void *dd_get_info(const struct inode *inode)
{
	if (!inode->i_crypt_info)
		return NULL;
	return inode->i_crypt_info->ci_dd_info;
}

int fscrypt_dd_decrypt_page(struct inode *inode, struct page *page)
{
	return dd_page_crypto(inode->i_crypt_info->ci_dd_info, DD_DECRYPT, page, page);
}

long fscrypt_dd_ioctl(unsigned int cmd, unsigned long *arg, struct inode *inode)
{
	switch (cmd) {
	case FS_IOC_GET_DD_POLICY: {
		struct dd_info *info;

		if (!fscrypt_dd_encrypted_inode(inode))
			return -ENOENT;

		info = dd_get_info(inode);
		if (copy_to_user((void __user *) (*arg), &info->policy,
				sizeof(struct dd_policy)))
			return -EFAULT;
		return 0;
	}
	case FS_IOC_SET_DD_POLICY: {
		struct dd_policy policy;

		if (fscrypt_dd_encrypted_inode(inode))
			return -EEXIST;

		if (copy_from_user(&policy, (struct dd_policy __user *) (*arg),
				sizeof(policy))) {
			return -EFAULT;
		}

		return update_encryption_context_with_dd_policy(inode, &policy);
	}
	}
	return 0;
}

int fscrypt_dd_submit_bio(struct inode *inode, struct bio *bio)
{
	return dd_submit_bio(dd_get_info(inode), bio);
}
