/*
 * Copyright(C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/miscdevice.h>
#include <linux/dma-buf-container.h>

#include "dma-buf-container.h"

#define MAX_BUFCON_BUFS 32
#define MAX_BUFCON_SRC_BUFS (MAX_BUFCON_BUFS - 1)

static int dmabuf_container_get_user_data(unsigned int cmd, void __user *arg,
					  int fds[])
{
	int __user *ufds;
	int count, ret;

#ifdef CONFIG_COMPAT
	if (cmd == DMA_BUF_COMPAT_IOCTL_MERGE) {
		struct compat_dma_buf_merge __user *udata = arg;
		compat_uptr_t compat_ufds;

		ret = get_user(compat_ufds, &udata->dma_bufs);
		ret |= get_user(count, &udata->count);
		ufds = compat_ptr(compat_ufds);
	} else
#endif
	{
		struct dma_buf_merge __user *udata = arg;

		ret = get_user(ufds, &udata->dma_bufs);
		ret |= get_user(count, &udata->count);
	}

	if (ret) {
		pr_err("%s: failed to read data from user\n", __func__);
		return -EFAULT;
	}

	if ((count < 1) || (count > MAX_BUFCON_SRC_BUFS)) {
		pr_err("%s: invalid buffer count %u\n", __func__, count);
		return -EINVAL;
	}

	if (copy_from_user(fds, ufds, sizeof(fds[0]) * count)) {
		pr_err("%s: failed to read %u dma_bufs from user\n",
		       __func__, count);
		return -EFAULT;
	}

	return count;
}

static int dmabuf_container_put_user_data(unsigned int cmd, void __user *arg,
					  struct dma_buf *merged)
{
	int fd = get_unused_fd_flags(O_CLOEXEC);
	int ret;

	if (fd < 0) {
		pr_err("%s: failed to get new fd\n", __func__);
		return fd;
	}

#ifdef CONFIG_COMPAT
	if (cmd == DMA_BUF_COMPAT_IOCTL_MERGE) {
		struct compat_dma_buf_merge __user *udata = arg;

		ret = put_user(fd, &udata->dmabuf_container);
	} else
#endif
	{
		struct dma_buf_merge __user *udata = arg;

		ret = put_user(fd, &udata->dmabuf_container);
	}

	if (ret) {
		pr_err("%s: failed to store dmabuf_container fd to user\n",
		       __func__);

		put_unused_fd(fd);
		return ret;
	}

	fd_install(fd, merged->file);

	return 0;
}

/*
 * struct dma_buf_container - container description
 * @table:	dummy sg_table for container
 * @count:	the number of the buffers
 * @dmabuf_mask: bit-mask of dma-bufs in @dmabufs.
 *               @dmabuf_mask is 0(unmasked) on creation of a dma-buf container.
 * @dmabufs:	dmabuf array representing each buffers
 */
struct dma_buf_container {
	struct sg_table	table;
	int		count;
	u32		dmabuf_mask;
	struct dma_buf	*dmabufs[0];
};

static void dmabuf_container_put_dmabuf(struct dma_buf_container *container)
{
	int i;

	for (i = 0; i < container->count; i++)
		dma_buf_put(container->dmabufs[i]);
}

static void dmabuf_container_dma_buf_release(struct dma_buf *dmabuf)
{
	dmabuf_container_put_dmabuf(dmabuf->priv);

	kfree(dmabuf->priv);
}

static struct sg_table *dmabuf_container_map_dma_buf(
				    struct dma_buf_attachment *attachment,
				    enum dma_data_direction direction)
{
	struct dma_buf_container *container = attachment->dmabuf->priv;

	return &container->table;
}

static void dmabuf_container_unmap_dma_buf(struct dma_buf_attachment *attach,
					   struct sg_table *table,
					   enum dma_data_direction direction)
{
}

static void *dmabuf_container_dma_buf_kmap(struct dma_buf *dmabuf,
					   unsigned long offset)
{
	return NULL;
}

static int dmabuf_container_mmap(struct dma_buf *dmabuf,
				 struct vm_area_struct *vma)
{
	pr_err("%s: dmabuf container does not support mmap\n", __func__);

	return -EACCES;
}

static struct dma_buf_ops dmabuf_container_dma_buf_ops = {
	.map_dma_buf = dmabuf_container_map_dma_buf,
	.unmap_dma_buf = dmabuf_container_unmap_dma_buf,
	.release = dmabuf_container_dma_buf_release,
	.map_atomic = dmabuf_container_dma_buf_kmap,
	.map = dmabuf_container_dma_buf_kmap,
	.mmap = dmabuf_container_mmap,
};

static bool is_dmabuf_container(struct dma_buf *dmabuf)
{
	return dmabuf->ops == &dmabuf_container_dma_buf_ops;
}

static struct dma_buf_container *get_container(struct dma_buf *dmabuf)
{
	return dmabuf->priv;
}

static int get_dma_buf_count(struct dma_buf *dmabuf)
{
	return is_dmabuf_container(dmabuf) ? get_container(dmabuf)->count : 1;
}

static struct dma_buf *__dmabuf_container_get_buffer(struct dma_buf *dmabuf,
						     int index)
{
	struct dma_buf *out = is_dmabuf_container(dmabuf)
			      ? get_container(dmabuf)->dmabufs[index] : dmabuf;

	get_dma_buf(out);

	return out;
}

static struct dma_buf *dmabuf_container_export(struct dma_buf_container *bufcon)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	unsigned long size = 0;
	int i;

	for (i = 0; i < bufcon->count; i++)
		size += bufcon->dmabufs[i]->size;

	exp_info.ops = &dmabuf_container_dma_buf_ops;
	exp_info.size = size;
	exp_info.flags = O_RDWR;
	exp_info.priv = bufcon;

	return dma_buf_export(&exp_info);
}

static struct dma_buf *create_dmabuf_container(struct dma_buf *base,
					       struct dma_buf *src[], int count)
{
	struct dma_buf_container *container;
	struct dma_buf *merged;
	int total = 0;
	int i, isrc;
	int nelem;

	for (i = 0; i < count; i++)
		total += get_dma_buf_count(src[i]);
	total += get_dma_buf_count(base);

	if (total > MAX_BUFCON_BUFS) {
		pr_err("%s: too many (%u) dmabuf merge request\n",
		       __func__, total);
		return ERR_PTR(-EINVAL);
	}

	container = kzalloc(sizeof(*container) +
			    sizeof(container->dmabufs[0]) * total, GFP_KERNEL);
	if (!container)
		return ERR_PTR(-ENOMEM);

	nelem = get_dma_buf_count(base);
	for (i = 0; i < nelem; i++)
		container->dmabufs[i] = __dmabuf_container_get_buffer(base, i);

	for (isrc = 0; isrc < count; isrc++)
		for (i = 0; i < get_dma_buf_count(src[isrc]); i++)
			container->dmabufs[nelem++] =
				__dmabuf_container_get_buffer(src[isrc], i);

	container->count = nelem;

	merged = dmabuf_container_export(container);
	if (IS_ERR(merged)) {
		pr_err("%s: failed to export dmabuf container.\n", __func__);
		dmabuf_container_put_dmabuf(container);
		kfree(container);
	}

	return merged;
}

static struct dma_buf *dmabuf_container_create(struct dma_buf *dmabuf,
					       int fds[], int count)
{
	struct dma_buf *src[MAX_BUFCON_SRC_BUFS];
	struct dma_buf *merged;
	int i;

	for (i = 0; i < count; i++) {
		src[i] = dma_buf_get(fds[i]);
		if (IS_ERR(src[i])) {
			merged = src[i];
			pr_err("%s: failed to get dmabuf of fd %d @ %u/%u\n",
			       __func__, fds[i], i, count);

			goto err_get;
		}
	}

	merged = create_dmabuf_container(dmabuf, src, count);
	/*
	 * reference count of dma_bufs (file->f_count) in src[] are increased
	 * again in create_dmabuf_container(). So they should be decremented
	 * before return.
	 */
err_get:
	while (i-- > 0)
		dma_buf_put(src[i]);

	return merged;
}

long dma_buf_merge_ioctl(struct dma_buf *dmabuf,
			 unsigned int cmd, unsigned long arg)
{
	int fds[MAX_BUFCON_SRC_BUFS];
	int count;
	struct dma_buf *merged;
	long ret;

	count = dmabuf_container_get_user_data(cmd, (void __user *)arg, fds);
	if (count < 0)
		return count;

	merged = dmabuf_container_create(dmabuf, fds, count);
	if (IS_ERR(merged))
		return PTR_ERR(merged);

	ret = dmabuf_container_put_user_data(cmd, (void __user *)arg, merged);
	if (ret) {
		dma_buf_put(merged);
		return ret;
	}

	return 0;
}

int dmabuf_container_set_mask_user(struct dma_buf *dmabuf, unsigned long arg)
{
	u32 mask;

	if (get_user(mask, (u32 __user *)arg)) {
		pr_err("%s: failed to read mask from user\n", __func__);
		return -EFAULT;
	}

	return dmabuf_container_set_mask(dmabuf, mask);
}

int dmabuf_container_get_mask_user(struct dma_buf *dmabuf, unsigned long arg)
{
	u32 mask;
	int ret;

	ret = dmabuf_container_get_mask(dmabuf, &mask);
	if (ret < 0)
		return ret;

	if (put_user(mask, (u32 __user *)arg)) {
		pr_err("%s: failed to write mask to user\n", __func__);
		return -EFAULT;
	}

	return 0;
}

int dmabuf_container_set_mask(struct dma_buf *dmabuf, u32 mask)
{
	struct dma_buf_container *container;

	if (!is_dmabuf_container(dmabuf)) {
		pr_err("%s: given dmabuf is not dma-buf container\n", __func__);
		return -EINVAL;
	}

	container = get_container(dmabuf);

	if (mask & ~((1 << container->count) - 1)) {
		pr_err("%s: invalid mask %#x for %u buffers\n",
		       __func__, mask, container->count);
		return -EINVAL;
	}

	get_container(dmabuf)->dmabuf_mask = mask;

	return 0;
}
EXPORT_SYMBOL_GPL(dmabuf_container_set_mask);

int dmabuf_container_get_mask(struct dma_buf *dmabuf, u32 *mask)
{
	if (!is_dmabuf_container(dmabuf)) {
		pr_err("%s: given dmabuf is not dma-buf container\n", __func__);
		return -EINVAL;
	}

	*mask = get_container(dmabuf)->dmabuf_mask;

	return 0;
}
EXPORT_SYMBOL_GPL(dmabuf_container_get_mask);

int dmabuf_container_get_count(struct dma_buf *dmabuf)
{
	if (!is_dmabuf_container(dmabuf))
		return -EINVAL;

	return get_container(dmabuf)->count;
}
EXPORT_SYMBOL_GPL(dmabuf_container_get_count);

struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dmabuf, int index)
{
	struct dma_buf_container *container = get_container(dmabuf);

	if (!is_dmabuf_container(dmabuf))
		return NULL;

	if (WARN_ON(index >= container->count))
		return NULL;

	get_dma_buf(container->dmabufs[index]);

	return container->dmabufs[index];
}
EXPORT_SYMBOL_GPL(dmabuf_container_get_buffer);

struct dma_buf *dma_buf_get_any(int fd)
{
	struct dma_buf *dmabuf = dma_buf_get(fd);
	struct dma_buf *anybuf = __dmabuf_container_get_buffer(dmabuf, 0);

	dma_buf_put(dmabuf);

	return anybuf;
}
EXPORT_SYMBOL_GPL(dma_buf_get_any);

#ifdef CONFIG_DMA_BUF_CONTAINER_TEST

static int dmabuf_container_verify_one(struct dma_buf_container *container,
				       struct dma_buf *dmabuf, int idx)
{
	struct dma_buf_container *subset;
	int subidx;

	if (!is_dmabuf_container(dmabuf)) {
		if (container->dmabufs[idx] == dmabuf)
			return 1;
		return -ENOENT;
	}

	subset = get_container(dmabuf);

	for (subidx = 0; subidx < subset->count; subidx++)
		if (subset->dmabufs[subidx] != container->dmabufs[idx++])
			return -ENOENT;

	return subset->count;
}

static long dmabuf_container_verify(struct dma_buf_container *container,
				    struct dma_buf *dmabufs[], int count)
{
	int idx = 0, i = 0;

	while ((i < count) && (idx < container->count)) {
		int ret = dmabuf_container_verify_one(container,
						      dmabufs[i], idx);

		if (ret < 0) {
			pr_err("%s: verify failed @ container[%d]/dmabuf[%d]\n",
			       __func__, idx, i);
			return ret;
		}

		idx += ret;
		i++;
	}

	return 0;
}

struct bufcon_test_data {
	int *buf_fds;
	__s32 num_fd;
	__s32 bufcon_fd;
	__u32 total_size;
	__s32 reserved;
};

static long dmabuf_container_test(struct bufcon_test_data *data)
{
	struct dma_buf *container = dma_buf_get(data->bufcon_fd);
	struct dma_buf **dmabufs;
	int i;
	long ret = -EINVAL;

	if (IS_ERR(container)) {
		pr_err("%s: fd %d is not a dmabuf\n",
		       __func__, data->bufcon_fd);
		return PTR_ERR(container);
	}

	if (!is_dmabuf_container(container)) {
		pr_err("%s: fd %d is not a dmabuf container\n",
		       __func__, data->bufcon_fd);
		goto err_bufcon;
	}

	if (container->size != data->total_size) {
		pr_err("%s: the size of dmabuf container %zu is not %u\n",
		       __func__, container->size, data->total_size);
		goto err_bufcon;
	}

	dmabufs = kmalloc_array(data->num_fd, sizeof(dmabufs[0]), GFP_KERNEL);
	if (!dmabufs) {
		ret = -ENOMEM;
		goto err_bufcon;
	}

	for (i = 0; i < data->num_fd; i++) {
		dmabufs[i] = dma_buf_get(data->buf_fds[i]);
		if (IS_ERR(dmabufs[i])) {
			ret = PTR_ERR(dmabufs[i]);
			goto err;
		}
	}

	ret = dmabuf_container_verify(container->priv, dmabufs, data->num_fd);
err:
	while (i--)
		dma_buf_put(dmabufs[i]);
	kfree(dmabufs);
err_bufcon:
	dma_buf_put(container);

	return ret;
}

#define TEST_IOC_TEST _IOR('T', 0, struct bufcon_test_data)

static long dmabuf_container_test_ioctl(struct file *filp, unsigned int cmd,
					unsigned long arg)
{
	struct bufcon_test_data __user *udata = (void __user *)arg;
	struct bufcon_test_data data;
	int __user *buf_fds;
	long ret = 0;

	if (cmd != TEST_IOC_TEST) {
		pr_err("%s: unknown cmd %#x\n", __func__, cmd);
		return -ENOTTY;
	}

	if (copy_from_user(&data, udata, sizeof(data))) {
		pr_err("%s: failed to read user data\n", __func__);
		return -EFAULT;
	}

	if ((data.num_fd < 1) || (data.num_fd > MAX_BUFCON_BUFS)) {
		pr_err("%s: invalid number of dma-buf fds %d\n",
		       __func__, data.num_fd);
		return -EINVAL;
	}

	buf_fds = data.buf_fds;

	data.buf_fds = kcalloc(data.num_fd, sizeof(*data.buf_fds), GFP_KERNEL);
	if (!data.buf_fds)
		return -ENOMEM;

	if (copy_from_user(data.buf_fds, buf_fds,
			   sizeof(data.buf_fds[0]) * data.num_fd)) {
		pr_err("%s: failed to read fd list\n", __func__);
		ret = -EFAULT;
		goto err;
	}

	ret = dmabuf_container_test(&data);
err:
	kfree(data.buf_fds);

	return ret;
}

#ifdef CONFIG_COMPAT
struct compat_bufcon_test_data {
	compat_uptr_t buf_fds;
	__s32 num_fd;
	__s32 bufcon_fd;
	__u32 total_size;
	__s32 reserved;
};

#define COMPAT_TEST_IOC_TEST _IOR('T', 0, struct compat_bufcon_test_data)

static long dmabuf_container_test_compat_ioctl(struct file *filp,
					       unsigned int cmd,
					       unsigned long arg)
{
	struct compat_bufcon_test_data __user *udata = compat_ptr(arg);
	struct bufcon_test_data data;
	compat_uptr_t buf_fds;
	long ret = 0;

	if (cmd != COMPAT_TEST_IOC_TEST) {
		pr_err("%s: unknown cmd %#x\n", __func__, cmd);
		return -ENOTTY;
	}

	ret = get_user(buf_fds, &udata->buf_fds);
	ret |= get_user(data.num_fd, &udata->num_fd);
	ret |= get_user(data.bufcon_fd, &udata->bufcon_fd);
	ret |= get_user(data.total_size, &udata->total_size);
	if (ret) {
		pr_err("%s: failed to read user data\n", __func__);
		return -EFAULT;
	}

	if ((data.num_fd < 1) || (data.num_fd > MAX_BUFCON_BUFS)) {
		pr_err("%s: invalid number of dma-buf fds %d\n",
		       __func__, data.num_fd);
		return -EINVAL;
	}

	data.buf_fds = kcalloc(data.num_fd, sizeof(*data.buf_fds), GFP_KERNEL);
	if (!data.buf_fds)
		return -ENOMEM;

	if (copy_from_user(data.buf_fds, compat_ptr(buf_fds),
			   sizeof(data.buf_fds[0]) * data.num_fd)) {
		pr_err("%s: failed to read fd list\n", __func__);
		ret = -EFAULT;
		goto err;
	}

	ret = dmabuf_container_test(&data);
err:
	kfree(data.buf_fds);

	return 0;
}
#endif

static const struct file_operations dmabuf_container_test_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = dmabuf_container_test_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dmabuf_container_test_compat_ioctl,
#endif
};

static struct miscdevice dmabuf_container_test_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "bufcon-test",
	.fops = &dmabuf_container_test_fops,
};

static int __init dmabuf_container_test_init(void)
{
	int ret = misc_register(&dmabuf_container_test_dev);

	if (ret) {
		pr_err("%s: failed to register %s\n", __func__,
		       dmabuf_container_test_dev.name);
		return ret;
	}

	return 0;
}
device_initcall(dmabuf_container_test_init);

#endif /* CONFIG_DMA_BUF_CONTAINER_TEST */
