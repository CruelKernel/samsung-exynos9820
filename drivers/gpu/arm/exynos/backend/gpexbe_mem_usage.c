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
#include <gpexbe_mem_usage.h>

/* Uses */
#include <gpex_utils.h>
#include <device/mali_kbase_device.h>

static ssize_t show_kernel_sysfs_gpu_memory(char *buf)
{
	struct list_head *entry;
	const struct list_head *kbdev_list;
	ssize_t ret = 0;
	uint64_t gpu_mem_used = 0;
	bool buffer_full = false;
	const ssize_t buf_size = PAGE_SIZE;
	const int padding = 100;

	kbdev_list = kbase_device_get_list();

	if (buf == NULL || kbdev_list == NULL)
		return ret;

	list_for_each (entry, kbdev_list) {
		struct kbase_device *kbdev = NULL;
		struct kbase_context *kctx;

		if (buffer_full)
			break;

		kbdev = list_entry(entry, struct kbase_device, entry);

		ret += scnprintf(buf + ret, buf_size - ret, "%9s %9s %12s\n", "tgid", "pid",
				 "bytes_used");

		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry (kctx, &kbdev->kctx_list, kctx_list_link) {
			if (ret + padding > buf_size) {
				buffer_full = true;
				break;
			}

			gpu_mem_used = atomic_read(&(kctx->used_pages)) * PAGE_SIZE;
			ret += snprintf(buf + ret, buf_size - ret, "%9d %9d %12llu\n", kctx->tgid,
					kctx->pid, gpu_mem_used);
		}
		mutex_unlock(&kbdev->kctx_list_lock);
	}
	kbase_device_put_list(kbdev_list);

	if (buffer_full)
		ret += scnprintf(buf + ret, buf_size - ret, "error: buffer is full\n");

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_gpu_memory);

int gpexbe_mem_usage_init(void)
{
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_memory, show_kernel_sysfs_gpu_memory);
	return 0;
}

void gpexbe_mem_usage_term(void)
{
	return;
}
