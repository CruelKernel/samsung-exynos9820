#include <linux/file.h>
#include <linux/fs.h>
#include <linux/xattr.h>
#include <linux/dcache.h>
#include "fat.h"

#ifndef CONFIG_FAT_VIRTUAL_XATTR_SELINUX_LABEL
#define CONFIG_FAT_VIRTUAL_XATTR_SELINUX_LABEL	("undefined")
#endif

static const char default_xattr[] = CONFIG_FAT_VIRTUAL_XATTR_SELINUX_LABEL;

static int can_support(const char *name)
{
	if (!name || strcmp(name, "security.selinux"))
		return -1;
	return 0;
}

ssize_t fat_listxattr(struct dentry *dentry, char *list, size_t size)
{
	return 0;
}

static int __fat_xattr_check_support(const char *name)
{
	if (can_support(name))
		return -EOPNOTSUPP;

	return 0;
}

ssize_t __fat_getxattr(const char *name, void *value, size_t size)
{
	if (can_support(name))
		return -EOPNOTSUPP;

	if ((size > strlen(default_xattr)+1) && value)
		strcpy(value, default_xattr);

	return strlen(default_xattr);
}

static int fat_xattr_get(const struct xattr_handler *handler,
		struct dentry *dentry, struct inode *inode,
		const char *name, void *buffer, size_t size)
{
	return __fat_getxattr(name, buffer, size);
}

static int fat_xattr_set(const struct xattr_handler *handler,
		struct dentry *dentry, struct inode *inode,
		const char *name, const void *value, size_t size,
		int flags)
{
	return __fat_xattr_check_support(name);
}

const struct xattr_handler fat_xattr_handler = {
	.prefix = "",  /* match anything */
	.get = fat_xattr_get,
	.set = fat_xattr_set,
};

const struct xattr_handler *fat_xattr_handlers[] = {
	&fat_xattr_handler,
	NULL
};

void setup_fat_xattr_handler(struct super_block *sb)
{
	sb->s_xattr = fat_xattr_handlers;
}
