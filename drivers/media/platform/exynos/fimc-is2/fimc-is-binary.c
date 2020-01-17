/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/firmware.h>

#include "fimc-is-binary.h"
#include "exynos-fimc-is-sensor.h"

/* the version storage of each library binary */
struct lib_ver { char v[LIBRARY_VER_LEN + 1]; };
static struct lib_ver lib_ver_s[IS_BIN_LIB_HINT_RTA + 1];
/* the version stroage of each sensor setfile by position */
struct set_ver { char v[SETFILE_VER_LEN + 1]; };
static struct set_ver set_ver_s[SENSOR_POSITION_MAX];

static const char * const bin_names[] = { "DDK", "RTA", "SETFILE" };

static char *library_get_buf(const struct is_bin_ver_info *info, unsigned int hint)
{
	struct lib_ver *s = (struct lib_ver *)info->s;

	return s[hint].v;
}

static unsigned int library_get_name_idx(unsigned int hint)
{

	if (hint == IS_BIN_LIB_HINT_RTA)
		return 1; /* RTA */

	return 0; /* DDK */
}

static char *setfile_get_buf(const struct is_bin_ver_info *info, unsigned int hint)
{
	struct set_ver *s = (struct set_ver *)info->s;

	return s[hint].v;
}

static unsigned int setfile_get_name_idx(unsigned int hint)
{
	return 2; /* SETFILE */
}

static const struct is_bin_ver_info bin_ver_info[] = {
	{
		.type		= IS_BIN_LIBRARY,
		.maxhint	= IS_BIN_LIB_HINT_RTA + 1,
		.offset		= LIBRARY_VER_OFS,
		.length		= LIBRARY_VER_LEN,
		.s		= &lib_ver_s,
		.get_buf	= library_get_buf,
		.get_name_idx	= library_get_name_idx,
	},
	{
		.type		= IS_BIN_SETFILE,
		.maxhint	= SENSOR_POSITION_MAX,
		.offset		= SETFILE_VER_OFS,
		.length		= SETFILE_VER_LEN,
		.s		= &set_ver_s,
		.get_buf	= setfile_get_buf,
		.get_name_idx	= setfile_get_name_idx,
	},
	{},
};

static noinline_for_stack long get_file_size(struct file *file)
{
	struct kstat st;
	u32 request_mask = (STATX_MODE | STATX_SIZE);

	if (vfs_getattr(&file->f_path, &st, request_mask, KSTAT_QUERY_FLAGS))
		return -1;
	if (!S_ISREG(st.mode))
		return -1;
	if (st.size != (long)st.size)
		return -1;

	return st.size;
}

static int read_file_contents(struct file *fp, struct fimc_is_binary *bin)
{
	long size;
	char *buf;
	int ret;
	loff_t pos = 0;

	size = get_file_size(fp);
	if (size <= 0)
		return -EBADF;

	/* if a buffer for binary is already allocated */
	if (bin->data) {
		if (!bin->size)
			return -EINVAL;

		buf = bin->data;

		/* shrink read size to fit the size of a given buffer */
		if (size > bin->size) {
			pr_crit("%s: will read only %ld bytes from a file (%ld)",
				__func__, bin->size, size);
			size = bin->size;
		}
	} else {
		buf = bin->alloc(size);
		if (!buf)
			return -ENOMEM;
	}

	ret = kernel_read(fp, buf, size, &pos);
	if (ret != size) {
		if (!bin->data)
			bin->free(buf);
		return ret;
	}

	bin->data = buf;
	bin->size = size;

	return 0;
}

static int write_file_contents(struct file *fp, struct fimc_is_binary *bin)
{
	ssize_t ret;
	ssize_t count;
	loff_t pos = 0;
	char *buf = (char *)bin->data;

	do {
		count = min_t(ssize_t, PAGE_SIZE, bin->size - pos);
		ret = kernel_write(fp, buf, count, &pos);
		if (ret > 0) {
			pos += ret;
			buf += ret;
		} else {
			break;
		}
	} while (pos < bin->size);

	return 0;
}

/**
  * get_filesystem_binary: copy a binary in userland to a given buffer
  * @filename: filenam of binary file including full path
  * @bin: pointer to fimc_is_binary structure
  **/
int get_filesystem_binary(const char *filename, struct fimc_is_binary *bin)
{
	struct file *fp;
	int ret = 0;

	fp = filp_open(filename, O_RDONLY, 0);
	if (!IS_ERR_OR_NULL(fp)) {
		ret = read_file_contents(fp, bin);
		fput(fp);
	} else {
		ret = PTR_ERR(fp);
	}

	return ret;
}

/**
  * put_filesystem_binary: copy a given buffer to a file in userland
  * @filename: filenam of binary file including full path
  * @bin: pointer to fimc_is_binary structure
  **/
int put_filesystem_binary(const char *filename, struct fimc_is_binary *bin, u32 flags)
{
	struct file *fp;
	int ret = 0;

	fp = filp_open(filename, flags, 0666);
	if (!IS_ERR_OR_NULL(fp)) {
		ret = write_file_contents(fp, bin);
		fput(fp);
	} else {
		ret = PTR_ERR(fp);
	}

	return ret;
}

/**
  * setup_binary_loader: customize an instance of the binary loader
  * @bin: pointer to fimc_is_binary structure
  * @retry_cnt: retry count of request_firmware
  * @retry_err: specific error number for retry of request_firmware
  * @alloc: custom allocation function for file system loading
  * @free: custom free function for file system loading
  **/
void setup_binary_loader(struct fimc_is_binary *bin,
				unsigned int retry_cnt, int retry_err,
				void *(*alloc)(unsigned long size),
				void (*free)(const void *buf))
{
	bin->retry_cnt = retry_cnt;
	bin->retry_err = retry_err;

	if (alloc && free) {
		bin->alloc = alloc;
		bin->free = free;
	} else {
		/* set vmalloc/vfree as a default */
		bin->alloc = &vmalloc;
		bin->free =  &vfree;
	}

	bin->customized = (unsigned long)bin;
}

 /**
  * request_binary: send loading request to the loader
  * @bin: pointer to fimc_is_binary structure
  * @path: path of binary file
  * @name: name of binary file
  * @device: device for which binary is being loaded
  **/
int request_binary(struct fimc_is_binary *bin, const char *path,
				const char *name, struct device *device)
{
	char *filename;
	unsigned int retry_cnt = 0;
	int retry_err = 0;
	int ret;

	bin->data = NULL;
	bin->size = 0;
	bin->fw = NULL;

	/* whether the loader is customized or not */
	if (bin->customized != (unsigned long)bin) {
		bin->alloc = &vmalloc;
		bin->free =  &vfree;
	} else {
		retry_cnt = bin->retry_cnt;
		retry_err = bin->retry_err;
	}

	/* read the requested binary from file system directly */
	if (path) {
		filename = __getname();
		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s%s", path, name);
		ret = get_filesystem_binary(filename, bin);
		__putname(filename);
		/* read successfully or don't want to go further more */
		if (!ret || !device)
			return ret;
	}

	/* ask to 'request_firmware' */
	do {
		ret = request_firmware(&bin->fw, name, device);

		if (!ret && bin->fw) {
			bin->data = (void *)bin->fw->data;
			bin->size = bin->fw->size;

			break;
		}

		/*
		 * if no specific error for retry is given;
		 * whatever any error is occurred, we should retry
		 */
		if (!bin->retry_err)
			retry_err = ret;
	} while (retry_cnt-- && (retry_err == ret));

	return ret;
}

/**
 * release_firmware: release the resource related to a binary
 * @bin: binary resource to release
**/
void release_binary(struct fimc_is_binary *bin)
{
	if (bin->fw)
		release_firmware(bin->fw);
	else if (bin->data)
		bin->free(bin->data);
}

/**
 * was_loaded_by: how was a binary loaded
 * @bin: pointer to fimc_is_binary structure
**/
int was_loaded_by(struct fimc_is_binary *bin)
{
	if (bin->fw)
		return 1;	/* by request_firmware */
	else if (bin->data)
		return 0;	/* from file system */
	else
		return -EINVAL;
}

/*
 * carve_binary_version: update the version of a binary to use in the future
 * @type: a binary type for IS
 * @hint: a hint to distinguish between same type binaries
 * @data: buffer of a loaded binary
 * @size: size of a loaded binary
 */
/* FIXME: void carve_binary_version(enum is_bin_type type, int hint, struct fimc_is_binary *bin) */
int carve_binary_version(enum is_bin_type type, unsigned int hint, void *data, size_t size)
{
	int ofs, len;
	char *buf;

	if (type >= ARRAY_SIZE(bin_ver_info)) {
		pr_err("unknown type binary(%d)\n", type);
		return -EINVAL;
	}

	if (hint >= bin_ver_info[type].maxhint) {
		pr_warn("invalid hint(%d) to carved-out\n", hint);
		return -EINVAL;
	}

	ofs = size - bin_ver_info[type].offset;
	if (ofs <= 0) {
		pr_warn("out of range offset(size: %d <= offset: %d)\n", size,
					bin_ver_info[type].offset);
		return -EINVAL;
	}

	len = bin_ver_info[type].length;
	if ((ofs + len) > size) {
		pr_warn("too long version length (binary: %d < version: %d)\n",
						size, (ofs + len));
		len -= ((ofs + len) - size);
	}

	buf = bin_ver_info[type].get_buf(&bin_ver_info[type], hint);
	memcpy(buf, &data[ofs], len);
	buf[len] = '\0';

	info("%s version: %s\n", bin_names[bin_ver_info[type].get_name_idx(hint)],
									buf);

	return 0;
}

/*
 * get_binary_version: retrun the version string of a binary
 * @type: a binary type for IS
 * @hint: a hint to distinguish between same type binaries
 *	library: DDK(0), RTA(1)
 *	setfile: sensor position
 */
char *get_binary_version(enum is_bin_type type, unsigned int hint)
{
	if (type >= ARRAY_SIZE(bin_ver_info)) {
		pr_err("unknown type binary(%d)\n", type);
		return NULL;
	}

	if (hint >= bin_ver_info[type].maxhint) {
		pr_warn("invalid binary hint(%d)\n", hint);
		return NULL;
	}

	return bin_ver_info[type].get_buf(&bin_ver_info[type], hint);
}
