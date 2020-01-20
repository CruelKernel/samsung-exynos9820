/**
 * sdp_xattr.c
 *
 * get/set sdp context to xattr
 */
#include "fscrypto_sdp_xattr_private.h"
#include <linux/xattr.h>

#define FSCRYPT_SDP_FS_TYPE_EXT4_STR "ext4"
#define FSCRYPT_SDP_FS_TYPE_F2FS_STR "f2fs"
#define FSCRYPT_SDP_XATTR_NAME_ENCRYPTION_SDP_CONTEXT "sdp"

//Must sync with ext4/xattr.h
#define FSCRYPT_SDP_EXT4_XATTR_INDEX_ENCRYPTION 9
extern int ext4_xattr_set(struct inode *, int, const char *, const void *, size_t, int);
extern int ext4_xattr_get(struct inode *, int, const char *, void *, size_t);
//Must sync with f2fs/xattr.h
#define FSCRYPT_SDP_F2FS_XATTR_INDEX_ENCRYPTION 9
#ifdef CONFIG_F2FS_FS_XATTR
extern int f2fs_setxattr(struct inode *, int, const char *,
				const void *, size_t, struct page *, int);
extern int f2fs_getxattr(struct inode *, int, const char *, void *,
						size_t, struct page *);
#else
static inline int f2fs_setxattr(struct inode *inode, int index,
		const char *name, const void *value, size_t size,
		struct page *page, int flags)
{
	return -EOPNOTSUPP;
}
static inline int f2fs_getxattr(struct inode *inode, int index,
			const char *name, void *buffer,
			size_t buffer_size, struct page *dpage)
{
	return -EOPNOTSUPP;
}
#endif

int fscrypt_sdp_get_context(struct inode *inode, void *ctx, size_t len)
{
	const char *fs_type_name = inode->i_mapping->host->i_sb->s_type->name;

	if (!strcmp(FSCRYPT_SDP_FS_TYPE_EXT4_STR, fs_type_name)) {
		return ext4_xattr_get(inode, FSCRYPT_SDP_EXT4_XATTR_INDEX_ENCRYPTION,
				FSCRYPT_SDP_XATTR_NAME_ENCRYPTION_SDP_CONTEXT, ctx, len);
	} else if (!strcmp(FSCRYPT_SDP_FS_TYPE_F2FS_STR, fs_type_name)) {
		return f2fs_getxattr(inode, FSCRYPT_SDP_F2FS_XATTR_INDEX_ENCRYPTION,
				FSCRYPT_SDP_XATTR_NAME_ENCRYPTION_SDP_CONTEXT, ctx, len, NULL);
	}

	return -ENOTTY;
}

static int __fscrypt_sdp_set_context(struct inode *inode, void *ctx, size_t len, bool is_lock)
{
	int retval = -ENOTTY;
	const char *fs_type_name = inode->i_mapping->host->i_sb->s_type->name;

	if (is_lock) inode_lock(inode);
	if (!strcmp(FSCRYPT_SDP_FS_TYPE_EXT4_STR, fs_type_name)) {
		retval = ext4_xattr_set(inode, FSCRYPT_SDP_EXT4_XATTR_INDEX_ENCRYPTION,
				FSCRYPT_SDP_XATTR_NAME_ENCRYPTION_SDP_CONTEXT, ctx, len, 0);
	} else if (!strcmp(FSCRYPT_SDP_FS_TYPE_F2FS_STR, fs_type_name)) {
		retval = f2fs_setxattr(inode, FSCRYPT_SDP_F2FS_XATTR_INDEX_ENCRYPTION,
				FSCRYPT_SDP_XATTR_NAME_ENCRYPTION_SDP_CONTEXT, ctx, len, 0, XATTR_CREATE);
	}
	if (is_lock) inode_unlock(inode);

	return retval;
}

int fscrypt_sdp_set_context(struct inode *inode, void *ctx, size_t len)
{
	return __fscrypt_sdp_set_context(inode, ctx, len, true);
}

int fscrypt_sdp_set_context_nolock(struct inode *inode, void *ctx, size_t len)
{
	return __fscrypt_sdp_set_context(inode, ctx, len, false);
}
