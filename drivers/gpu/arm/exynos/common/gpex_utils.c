/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

/* Implements */
#include <gpex_utils.h>

/* Uses */
#include <linux/device.h>
#include <linux/sysfs.h>

#include <mali_kbase.h>

#define MAX_ATTRS 128
#define SYSFS_KOBJECT_GROUP_NAME "gpu"

struct _utils_info {
	struct device *dev;
	struct kbase_device *kbdev;
	int debug_level;
	sysfs_device_read_func show_gpu_model_cb;
};

static struct _utils_info utils_info;

static struct kobject *external_kobj;
static struct kobj_attribute kobj_attrs[MAX_ATTRS];
static struct attribute *attrs[MAX_ATTRS];
static int kobj_last_attr_idx;

static struct device_attribute device_attrs[MAX_ATTRS];
static int device_last_attr_idx;

/************************************************************************
 * DEVICE SYSFS functions
 ************************************************************************/
static int gpex_utils_sysfs_device_init(void);
static void gpex_utils_sysfs_device_files_remove(int remove_cnt);
static void gpex_utils_sysfs_device_term(void);

static int gpex_utils_sysfs_device_init(void)
{
	device_last_attr_idx = 0;
	return 0;
}

/* Returns number of kobject files added so far */
int gpex_utils_sysfs_device_file_add(char *name, sysfs_device_read_func read_func,
				     sysfs_device_write_func write_func)
{
	int permission = 0;

	if (!read_func && !write_func) {
		/* TODO: print error. no funcs */
		return -EINVAL;
	}

	if (read_func)
		permission |= S_IRUGO;

	if (write_func)
		permission |= S_IWUSR;

	device_attrs[device_last_attr_idx].attr.name = name;
	device_attrs[device_last_attr_idx].attr.mode = permission;
	device_attrs[device_last_attr_idx].show = read_func;
	device_attrs[device_last_attr_idx].store = write_func;

	device_last_attr_idx++;

	return device_last_attr_idx;
}

int gpex_utils_sysfs_device_files_create(void)
{
	int i = 0;

	for (i = 0; i < device_last_attr_idx; i++) {
		if (device_create_file(utils_info.dev, &device_attrs[i])) {
			GPU_LOG(MALI_EXYNOS_ERROR, "couldn't create sysfs file [debug_level]\n");
			goto error_cleanup;
		}
	}

	return 0;

error_cleanup:
	gpex_utils_sysfs_device_files_remove(i);

	return -1;
}

static void gpex_utils_sysfs_device_files_remove(int remove_cnt)
{
	int i = 0;

	for (i = 0; i < remove_cnt; i++) {
		device_remove_file(utils_info.dev, &device_attrs[i]);
	}
}

static void gpex_utils_sysfs_device_term(void)
{
	gpex_utils_sysfs_device_files_remove(device_last_attr_idx);
}

/************************************************************************
 * KOBJECT SYSFS functions
 ************************************************************************/

static int gpex_utils_sysfs_kobject_init(void);
static void gpex_utils_sysfs_kobject_term(void);

static int gpex_utils_sysfs_kobject_init(void)
{
	kobj_last_attr_idx = 0;
	external_kobj = kobject_create_and_add(SYSFS_KOBJECT_GROUP_NAME, kernel_kobj);

	if (!external_kobj) {
		GPU_LOG(MALI_EXYNOS_ERROR, "couldn't create Kobj for group [KERNEL - GPU]\n");
	}

	return 0;
}

/* Returns number of kobject files added so far */
int gpex_utils_sysfs_kobject_file_add(char *name, sysfs_kobject_read_func read_func,
				      sysfs_kobject_write_func write_func)
{
	int permission = 0;

	if (!read_func && !write_func) {
		/* TODO: print error. no funcs */
		return -EINVAL;
	}

	if (read_func)
		permission |= S_IRUGO;

	if (write_func)
		permission |= S_IWUSR;

	kobj_attrs[kobj_last_attr_idx].attr.name = name;
	kobj_attrs[kobj_last_attr_idx].attr.mode = permission;
	kobj_attrs[kobj_last_attr_idx].show = (sysfs_kobject_read_func)read_func;
	kobj_attrs[kobj_last_attr_idx].store = (sysfs_kobject_write_func)write_func;

	attrs[kobj_last_attr_idx] = &kobj_attrs[kobj_last_attr_idx].attr;
	kobj_last_attr_idx++;

	return kobj_last_attr_idx;
}

int gpex_utils_sysfs_kobject_files_create(void)
{
	int err = 0;

	static struct attribute_group attr_group = {
		.attrs = attrs,
	};

	err = sysfs_create_group(external_kobj, &attr_group);

	return err;
}

static void gpex_utils_sysfs_kobject_term(void)
{
	if (external_kobj)
		kobject_put(external_kobj);
}

/************************************************************************
 * MALI EXYNOS UTILS SYSFS FUNCTIONS
 ************************************************************************/

void gpex_utils_sysfs_set_gpu_model_callback(sysfs_device_read_func show_gpu_model_fn)
{
	utils_info.show_gpu_model_cb = show_gpu_model_fn;
}

static ssize_t show_gpu_model(char *buf)
{
	if (utils_info.show_gpu_model_cb)
		return utils_info.show_gpu_model_cb(utils_info.dev, NULL, buf);

	return 0;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_gpu_model);

static ssize_t show_debug_level(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "[Current] %d (%d ~ %d)",
			utils_info.debug_level, MALI_EXYNOS_DEBUG_START + 1,
			MALI_EXYNOS_DEBUG_END - 1);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_debug_level);
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_debug_level);

static ssize_t set_debug_level(const char *buf, size_t count)
{
	int debug_level, ret;

	ret = kstrtoint(buf, 0, &debug_level);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((debug_level <= MALI_EXYNOS_DEBUG_START) || (debug_level >= MALI_EXYNOS_DEBUG_END)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid debug level (%d)\n", __func__,
			debug_level);
		return -ENOENT;
	}

	utils_info.debug_level = debug_level;

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_debug_level);
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_debug_level);

/****************
 * GETTERS
 * *************/
int gpex_utils_get_debug_level(void)
{
	return utils_info.debug_level;
}

struct device *gpex_utils_get_device(void)
{
	return utils_info.dev;
}

struct kbase_device *gpex_utils_get_kbase_device(void)
{
	return utils_info.kbdev;
}

/************************************************************************
 * INIT and TERM functions
 ************************************************************************/

int gpex_utils_init(struct device **dev)
{
	utils_info.dev = *dev;
	utils_info.kbdev = container_of(dev, struct kbase_device, dev);

	utils_info.debug_level = WARNING;

	utils_info.show_gpu_model_cb = NULL;

	gpex_utils_sysfs_kobject_init();
	gpex_utils_sysfs_device_init();

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(debug_level, show_debug_level, set_debug_level);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(debug_level, show_debug_level, set_debug_level);

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_model, show_gpu_model);

	return 0;
}

void gpex_utils_term(void)
{
	gpex_utils_sysfs_kobject_term();
	gpex_utils_sysfs_device_term();
	utils_info.dev = NULL;
	utils_info.show_gpu_model_cb = NULL;
}
