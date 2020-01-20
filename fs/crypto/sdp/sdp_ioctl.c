/*
 * sdp_ioctl.c
 *
 */
#include <linux/uaccess.h>
#include "../fscrypt_private.h"

int fscrypt_sdp_ioctl_get_sdp_info(struct inode *inode, unsigned long arg)
{
	struct dek_arg_sdp_info req;
	struct fscrypt_info *ci;
	int result = 0;

	if (inode->i_crypt_info == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		return -EOPNOTSUPP;
	}

	memset(&req, 0, sizeof(struct dek_arg_sdp_info));
	req.engine_id = -1;
	req.type = -1;
	req.sdp_enabled = 1;

	ci = inode->i_crypt_info;
	if(!ci->ci_sdp_info) {
		DEK_LOGE("get_info: can't find sdp info\n");
	} else {
		DEK_LOGD("get_info: ci->i_crypt_info->sdp_flags: 0x%08x\n",
				ci->ci_sdp_info->sdp_flags);

		if (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE) {
			req.is_sensitive = 1;
			req.engine_id = ci->ci_sdp_info->engine_id;
			req.type = ci->ci_sdp_info->sdp_dek.type;
		}
		if (ci->ci_sdp_info->sdp_flags & SDP_IS_CHAMBER_DIR)
			req.is_chamber = 1;
	}

	if (copy_to_user((void __user *)arg, &req, sizeof(req))) {
		DEK_LOGE("get_info: failed to copy data to user\n");
		result = -EFAULT;
	}
	return result;
}

int fscrypt_sdp_ioctl_set_sdp_policy(struct inode *inode, unsigned long arg)
{
	dek_arg_set_sdp_policy_t req;
	int rc = 0;

	if (inode->i_crypt_info == NULL) {
		DEK_LOGE("no encryption context to the target..\n");
		rc = -EOPNOTSUPP;
	} else {

		if (!is_root()) {
			DEK_LOGE("set_policy: operation not permitted to non-root process\n");
			return -EPERM;
		}

		memset(&req, 0, sizeof(dek_arg_set_sdp_policy_t));
		if (copy_from_user(&req,
				(dek_arg_set_sdp_policy_t __user *)arg, sizeof(req))) {
			DEK_LOGE("set_policy: failed to copy data from user\n");
			return -EFAULT;
		}

		rc = fscrypt_sdp_set_sdp_policy(inode, req.engine_id);
		if (rc) {
			DEK_LOGE("set_policy: operation failed (err:%d)\n", rc);
			rc = -EFAULT;
		}
	}
	return rc;
}

int fscrypt_sdp_ioctl_set_sensitive(struct inode *inode, unsigned long arg)
{
	dek_arg_set_sensitive_t req;
	int result = 0;

	if (inode->i_crypt_info == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		struct fscrypt_info *ci = inode->i_crypt_info;

		if (ci->ci_sdp_info &&
				(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
			DEK_LOGE("already sensitive file\n");
			return 0;
		}
		if (S_ISDIR(inode->i_mode) && !is_root()) {
#ifdef CONFIG_SDP_KEY_DUMP
			if (get_sdp_sysfs_key_dump()) {
				DEK_LOGD("Temporarily allowed to process not vold.");
			} else {
#endif
			DEK_LOGE("Only vold as root process can set sensitive directory\n");
			return -EPERM;
#ifdef CONFIG_SDP_KEY_DUMP
			}
#endif
		}

		memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
		if (copy_from_user(&req,
				(dek_arg_set_sensitive_t __user *)arg, sizeof(req))) {
			DEK_LOGE("can't copy from user\n");
			memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
			result = -EFAULT;
		} else {
			int rc = fscrypt_sdp_set_sensitive(inode, req.engine_id, NULL);

			if (rc) {
				DEK_LOGE("failed to set sensitive rc(%d)\n", rc);
				memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
				return -EFAULT;
			}
			memset(&req, 0, sizeof(dek_arg_set_sensitive_t));
		}
	}
	return result;
}

int fscrypt_sdp_ioctl_set_protected(struct inode *inode, unsigned long arg)
{
	dek_arg_set_protected_t req;
	int result = 0;

	if (inode->i_crypt_info == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		int rc;

		if (S_ISDIR(inode->i_mode) && !is_root()) {
#ifdef CONFIG_SDP_KEY_DUMP
			if (get_sdp_sysfs_key_dump()) {
				DEK_LOGD("Temporarily allowed to process not vold.");
			} else {
#endif
			DEK_LOGE("Only vold as root process can set protected directory\n");
			return -EPERM;
#ifdef CONFIG_SDP_KEY_DUMP
			}
#endif
		}

		memset(&req, 0, sizeof(dek_arg_set_protected_t));
		if (copy_from_user(&req,
				(dek_arg_set_protected_t __user *)arg, sizeof(req))) {
			DEK_LOGE("set_protected: failed to copy data from user\n");
			return -EFAULT;
		}

		rc = fscrypt_sdp_set_protected(inode, req.engine_id);
		if (rc) {
			DEK_LOGE("failed to set protected rc(%d)\n", rc);
			result = -EFAULT;
		}
	}
	return result;
}

int fscrypt_sdp_ioctl_add_chamber_directory(struct inode *inode, unsigned long arg)
{
	int result = 0;

	if (inode->i_crypt_info == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		dek_arg_add_chamber_t req;
		struct fscrypt_info *ci = inode->i_crypt_info;

		if (!S_ISDIR(inode->i_mode)) {
			DEK_LOGE("Not directory\n");
			return -EOPNOTSUPP;
		}

		if (ci->ci_sdp_info &&
				ci->ci_sdp_info->sdp_flags & SDP_IS_CHAMBER_DIR) {
			DEK_LOGE("Already chamber directory\n");
			return 0;
		}
		if (!is_root()) {
			DEK_LOGE("Permission denied: only epm process can call this\n");
			return -EPERM;
		}

		memset(&req, 0, sizeof(dek_arg_add_chamber_t));
		if (copy_from_user(&req,
				(dek_arg_add_chamber_t __user *)arg, sizeof(req))) {
			DEK_LOGE("can't copy from user\n");
			memset(&req, 0, sizeof(dek_arg_add_chamber_t));
			result = -EFAULT;
		} else {
			int rc = fscrypt_sdp_add_chamber_directory(req.engine_id, inode);

			if (rc) {
				DEK_LOGE("failed to add chamber rc(%d)\n", rc);
				memset(&req, 0, sizeof(dek_arg_add_chamber_t));
				return -EFAULT;
			}
			memset(&req, 0, sizeof(dek_arg_add_chamber_t));
		}
	}
	return result;
}

int fscrypt_sdp_ioctl_remove_chamber_directory(struct inode *inode)
{
	int result = 0;

	if (inode->i_crypt_info == NULL) {
		DEK_LOGE("No encryption context to the target..\n");
		result = -EOPNOTSUPP;
	} else {
		int rc;
		struct fscrypt_info *ci = inode->i_crypt_info;

		if (!ci->ci_sdp_info ||
				!(ci->ci_sdp_info->sdp_flags & SDP_IS_CHAMBER_DIR)) {
			DEK_LOGE("Not chamber directory\n");
			return 0;
		}
		if (!is_root()) {
			DEK_LOGE("Permission denied: only epm process can call this\n");
			return -EPERM;
		}
		rc = fscrypt_sdp_remove_chamber_directory(inode);
		if (rc) {
			DEK_LOGE("failed to remove chamber rc(%d)\n", rc);
			result = -EFAULT;
		}
	}
	return result;
}

/*
 * -ENOTTY will be returned if this ioctl is not related to SDP
 */
int fscrypt_sdp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file_inode(filp);

	if (!fscrypt_has_encryption_key(inode)) {
		//Not allowed without i_crypt_info, go to the default ioctl
		return -ENOTTY;
	}

	switch (cmd) {
	case FS_IOC_GET_SDP_INFO:
		return fscrypt_sdp_ioctl_get_sdp_info(inode, arg);
	case FS_IOC_SET_SDP_POLICY:
		return fscrypt_sdp_ioctl_set_sdp_policy(inode, arg);
	case FS_IOC_SET_SENSITIVE:
		return fscrypt_sdp_ioctl_set_sensitive(inode, arg);
	case FS_IOC_SET_PROTECTED:
		return fscrypt_sdp_ioctl_set_protected(inode, arg);
	case FS_IOC_ADD_CHAMBER:
		return fscrypt_sdp_ioctl_add_chamber_directory(inode, arg);
	case FS_IOC_REMOVE_CHAMBER:
		return fscrypt_sdp_ioctl_remove_chamber_directory(inode);
	default:
		return -ENOTTY;
	}
}
