/*
 *
 * drivers/staging/android/ion/ion.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/anon_inodes.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/memblock.h>
#include <linux/miscdevice.h>
#include <linux/export.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/idr.h>
#include <linux/sched/task.h>

#include "ion.h"
#include "ion_exynos.h"
#include "ion_debug.h"

static struct ion_device *internal_dev;
static int heap_id;

bool ion_buffer_cached(struct ion_buffer *buffer)
{
	return !!(buffer->flags & ION_FLAG_CACHED);
}

/* this function should only be called while dev->lock is held */
static void ion_buffer_add(struct ion_device *dev,
			   struct ion_buffer *buffer)
{
	struct rb_node **p = &dev->buffers.rb_node;
	struct rb_node *parent = NULL;
	struct ion_buffer *entry;
	struct task_struct *task;

	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct ion_buffer, node);

		if (buffer < entry) {
			p = &(*p)->rb_left;
		} else if (buffer > entry) {
			p = &(*p)->rb_right;
		} else {
			perrfn("buffer already found.");
			BUG();
		}
	}

	task = current;
	get_task_comm(buffer->task_comm, task->group_leader);
	get_task_comm(buffer->thread_comm, task);
	buffer->pid = task_pid_nr(task->group_leader);
	buffer->tid = task_pid_nr(task);

	rb_link_node(&buffer->node, parent, p);
	rb_insert_color(&buffer->node, &dev->buffers);
}

/* this function should only be called while dev->lock is held */
static struct ion_buffer *ion_buffer_create(struct ion_heap *heap,
					    struct ion_device *dev,
					    unsigned long len,
					    unsigned long flags)
{
	struct ion_buffer *buffer;
	int ret;
	long nr_alloc_cur, nr_alloc_peak;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	buffer->heap = heap;
	buffer->flags = flags;

	ret = heap->ops->allocate(heap, buffer, len, flags);

	if (ret) {
		if (!(heap->flags & ION_HEAP_FLAG_DEFER_FREE))
			goto err2;

		ion_heap_freelist_drain(heap, 0);
		ret = heap->ops->allocate(heap, buffer, len, flags);
		if (ret)
			goto err2;
	}

	if (!buffer->sg_table) {
		WARN_ONCE(1, "This heap needs to set the sgtable");
		ret = -EINVAL;
		goto err1;
	}

	buffer->dev = dev;
	buffer->size = len;

	INIT_LIST_HEAD(&buffer->iovas);
	mutex_init(&buffer->lock);
	mutex_lock(&dev->buffer_lock);
	ret = exynos_ion_alloc_fixup(dev, buffer);
	if (ret < 0) {
		mutex_unlock(&dev->buffer_lock);
		goto err1;
	}

	ion_buffer_add(dev, buffer);
	mutex_unlock(&dev->buffer_lock);
	nr_alloc_cur = atomic_long_add_return(len, &heap->total_allocated);
	nr_alloc_peak = atomic_long_read(&heap->total_allocated_peak);
	if (nr_alloc_cur > nr_alloc_peak)
		atomic_long_set(&heap->total_allocated_peak, nr_alloc_cur);
	return buffer;

err1:
	heap->ops->free(buffer);
err2:
	kfree(buffer);
	perrfn("failed to alloc (len %zu, flag %#lx) buffer from %s heap",
	       len, flags, heap->name);
	return ERR_PTR(ret);
}

void ion_buffer_destroy(struct ion_buffer *buffer)
{
	ion_event_begin();

	exynos_ion_free_fixup(buffer);
	if (buffer->kmap_cnt > 0) {
		pr_warn_once("%s: buffer still mapped in the kernel\n",
			     __func__);
		buffer->heap->ops->unmap_kernel(buffer->heap, buffer);
	}
	atomic_long_sub(buffer->size, &buffer->heap->total_allocated);
	buffer->heap->ops->free(buffer);

	ion_event_end(ION_EVENT_TYPE_FREE, buffer);

	kfree(buffer);
}

static void _ion_buffer_destroy(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct ion_device *dev = buffer->dev;

	mutex_lock(&dev->buffer_lock);
	rb_erase(&buffer->node, &dev->buffers);
	mutex_unlock(&dev->buffer_lock);

	if (heap->flags & ION_HEAP_FLAG_DEFER_FREE)
		ion_heap_freelist_add(heap, buffer);
	else
		ion_buffer_destroy(buffer);
}

void *ion_buffer_kmap_get(struct ion_buffer *buffer)
{
	void *vaddr;

	ion_event_begin();

	if (buffer->kmap_cnt) {
		buffer->kmap_cnt++;
		return buffer->vaddr;
	}
	vaddr = buffer->heap->ops->map_kernel(buffer->heap, buffer);
	if (WARN_ONCE(!vaddr,
		      "heap->ops->map_kernel should return ERR_PTR on error"))
		return ERR_PTR(-EINVAL);
	if (IS_ERR(vaddr)) {
		perrfn("failed to alloc kernel address of %zu buffer",
		       buffer->size);
		return vaddr;
	}
	buffer->vaddr = vaddr;
	buffer->kmap_cnt++;

	ion_event_end(ION_EVENT_TYPE_KMAP, buffer);

	return vaddr;
}

void ion_buffer_kmap_put(struct ion_buffer *buffer)
{
	buffer->kmap_cnt--;
	if (!buffer->kmap_cnt) {
		buffer->heap->ops->unmap_kernel(buffer->heap, buffer);
		buffer->vaddr = NULL;
	}
}

#ifndef CONFIG_ION_EXYNOS
static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sg(table->sgl, sg, table->nents, i) {
		memcpy(new_sg, sg, sizeof(*sg));
		sg->dma_address = 0;
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static void free_duped_table(struct sg_table *table)
{
	sg_free_table(table);
	kfree(table);
}

static int ion_dma_buf_attach(struct dma_buf *dmabuf, struct device *dev,
			      struct dma_buf_attachment *attachment)
{
	struct sg_table *table;
	struct ion_buffer *buffer = dmabuf->priv;

	table = dup_sg_table(buffer->sg_table);
	if (IS_ERR(table))
		return -ENOMEM;

	attachment->priv = table;

	return 0;
}

static void ion_dma_buf_detatch(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attachment)
{
	free_duped_table(attachment->priv);
}

static struct sg_table *ion_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct sg_table *table = attachment->priv;
	unsigned long attrs = 0;

	if (!ion_buffer_cached(attachment->dmabuf->priv))
		attrs = DMA_ATTR_SKIP_CPU_SYNC;

	if (!dma_map_sg_attrs(attachment->dev, table->sgl, table->nents,
			      direction, attrs))
		return ERR_PTR(-ENOMEM);

	return table;
}

static void ion_unmap_dma_buf(struct dma_buf_attachment *attachment,
			      struct sg_table *table,
			      enum dma_data_direction direction)
{
	unsigned long attrs = 0;

	if (!ion_buffer_cached(attachment->dmabuf->priv))
		attrs = DMA_ATTR_SKIP_CPU_SYNC;

	dma_unmap_sg_attrs(attachment->dev, table->sgl, table->nents,
			   direction, attrs);
}
#endif /* !CONFIG_ION_EXYNOS */

static int ion_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct ion_buffer *buffer = dmabuf->priv;
	int ret = 0;

	ion_event_begin();

	if (!buffer->heap->ops->map_user) {
		perrfn("this heap does not define a method for mapping to userspace");
		return -EINVAL;
	}

	if ((buffer->flags & ION_FLAG_NOZEROED) != 0) {
		perrfn("mmap() to nozeroed buffer is not allowed");
		return -EACCES;
	}

	if ((buffer->flags & ION_FLAG_PROTECTED) != 0) {
		perrfn("mmap() to protected buffer is not allowed");
		return -EACCES;
	}

	if (!(buffer->flags & ION_FLAG_CACHED))
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	mutex_lock(&buffer->lock);
	/* now map it to userspace */
	ret = buffer->heap->ops->map_user(buffer->heap, buffer, vma);
	mutex_unlock(&buffer->lock);

	if (ret)
		perrfn("failure mapping buffer to userspace");

	ion_event_end(ION_EVENT_TYPE_MMAP, buffer);

	return ret;
}

static void ion_dma_buf_release(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;

	_ion_buffer_destroy(buffer);
}

static void *ion_dma_buf_kmap(struct dma_buf *dmabuf, unsigned long offset)
{
	struct ion_buffer *buffer = dmabuf->priv;

	return buffer->vaddr + offset * PAGE_SIZE;
}

static void ion_dma_buf_kunmap(struct dma_buf *dmabuf, unsigned long offset,
			       void *ptr)
{
}

static void *ion_dma_buf_vmap(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;

	if (buffer->heap->ops->map_kernel) {
		mutex_lock(&buffer->lock);
		ion_buffer_kmap_get(buffer);
		mutex_unlock(&buffer->lock);
	}

	return buffer->vaddr;
}

static void ion_dma_buf_vunmap(struct dma_buf *dmabuf, void *ptr)
{
	struct ion_buffer *buffer = dmabuf->priv;

	if (buffer->heap->ops->map_kernel) {
		mutex_lock(&buffer->lock);
		ion_buffer_kmap_put(buffer);
		mutex_unlock(&buffer->lock);
	}
}

#ifndef CONFIG_ION_EXYNOS
static int ion_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{
	struct ion_buffer *buffer = dmabuf->priv;
	void *vaddr;
	struct dma_buf_attachment *att;

	/*
	 * TODO: Move this elsewhere because we don't always need a vaddr
	 */
	if (buffer->heap->ops->map_kernel) {
		mutex_lock(&buffer->lock);
		vaddr = ion_buffer_kmap_get(buffer);
		mutex_unlock(&buffer->lock);
	}

	mutex_lock(&dmabuf->lock);
	list_for_each_entry(att, &dmabuf->attachments, node) {
		struct sg_table *table = att->priv;

		dma_sync_sg_for_cpu(att->dev, table->sgl, table->nents,
				    direction);
	}
	mutex_unlock(&dmabuf->lock);

	return 0;
}

static int ion_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
				      enum dma_data_direction direction)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct dma_buf_attachment *att;

	if (buffer->heap->ops->map_kernel) {
		mutex_lock(&buffer->lock);
		ion_buffer_kmap_put(buffer);
		mutex_unlock(&buffer->lock);
	}

	mutex_lock(&dmabuf->lock);
	list_for_each_entry(att, &dmabuf->attachments, node) {
		struct sg_table *table = att->priv;

		dma_sync_sg_for_device(att->dev, table->sgl, table->nents,
				       direction);
	}
	mutex_unlock(&dmabuf->lock);

	return 0;
}
#endif

const struct dma_buf_ops ion_dma_buf_ops = {
#ifdef CONFIG_ION_EXYNOS
	.map_dma_buf = ion_exynos_map_dma_buf,
	.unmap_dma_buf = ion_exynos_unmap_dma_buf,
	.map_dma_buf_area = ion_exynos_map_dma_buf_area,
	.unmap_dma_buf_area = ion_exynos_unmap_dma_buf_area,
	.begin_cpu_access = ion_exynos_dma_buf_begin_cpu_access,
	.end_cpu_access = ion_exynos_dma_buf_end_cpu_access,
#else
	.attach = ion_dma_buf_attach,
	.detach = ion_dma_buf_detatch,
	.map_dma_buf = ion_map_dma_buf,
	.unmap_dma_buf = ion_unmap_dma_buf,
	.begin_cpu_access = ion_dma_buf_begin_cpu_access,
	.end_cpu_access = ion_dma_buf_end_cpu_access,
#endif
	.mmap = ion_mmap,
	.release = ion_dma_buf_release,
	.map_atomic = ion_dma_buf_kmap,
	.unmap_atomic = ion_dma_buf_kunmap,
	.map = ion_dma_buf_kmap,
	.unmap = ion_dma_buf_kunmap,
	.vmap = ion_dma_buf_vmap,
	.vunmap = ion_dma_buf_vunmap,
};

#define ION_EXPNAME_LEN (4 + 4 + 1) /* strlen("ion-") + strlen("2048") + '\0' */

int camera_heap_id;
int camera_contig_heap_id;

void exynos_ion_init_camera_heaps(void)
{
	struct ion_heap *heap;

	WARN_ON(camera_heap_id || camera_contig_heap_id);

	heap = ion_get_heap_by_name("camera_heap");
	if (heap)
		camera_heap_id = (int)heap->id;
	heap = ion_get_heap_by_name("camera_contig_heap");
	if (heap)
		camera_contig_heap_id = (int)heap->id;

	pr_info("%s: camera %d contig %d\n",
		__func__, camera_heap_id, camera_contig_heap_id);
}

unsigned int ion_parse_camera_heap_id(unsigned int heap_id_mask,
				      unsigned int flags)
{
	if (!camera_heap_id || !camera_contig_heap_id)
		return heap_id_mask;
	/*
	 * Buffer alloc request on "camera heap" id with ION_FLAG_PROTECTED
	 * should go to camera_contig heap.
	 * This is the exynos9820-specific requirement.
	 */
	if (heap_id_mask == (1 << camera_heap_id) && (flags & ION_FLAG_PROTECTED))
		return (1 << camera_contig_heap_id);

	/* User space cannot request camera_contig heap directly */
	if (heap_id_mask == (1 << camera_contig_heap_id))
		return 0;

	return heap_id_mask;
}

struct dma_buf *__ion_alloc(size_t len, unsigned int heap_id_mask,
			    unsigned int flags)
{
	struct ion_device *dev = internal_dev;
	struct ion_buffer *buffer = NULL;
	struct ion_heap *heap;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	char expname[ION_EXPNAME_LEN];
	struct dma_buf *dmabuf;

	ion_event_begin();

	pr_debug("%s: len %zu heap_id_mask %u flags %x\n", __func__,
		 len, heap_id_mask, flags);
	/*
	 * traverse the list of heaps available in this system in priority
	 * order.  If the heap type is supported by the client, and matches the
	 * request of the caller allocate from it.  Repeat until allocate has
	 * succeeded or all heaps have been tried
	 */
	len = PAGE_ALIGN(len);

	if (!len) {
		perrfn("zero size allocation - heapmask %#x, flags %#x",
		       heap_id_mask, flags);
		return ERR_PTR(-EINVAL);
	}

	heap_id_mask = ion_parse_camera_heap_id(heap_id_mask, flags);
	down_read(&dev->lock);
	plist_for_each_entry(heap, &dev->heaps, node) {
		/* if the caller didn't specify this heap id */
		if (!((1 << heap->id) & heap_id_mask))
			continue;
		buffer = ion_buffer_create(heap, dev, len, flags);
		if (!IS_ERR(buffer))
			break;
	}
	up_read(&dev->lock);

	if (!buffer) {
		perrfn("no matching heap found against heapmaks %#x", heap_id_mask);
		return ERR_PTR(-ENODEV);
	}

	if (IS_ERR(buffer))
		return ERR_CAST(buffer);

	snprintf(expname, ION_EXPNAME_LEN, "ion-%d", buffer->id);
	exp_info.exp_name = expname;

	exp_info.ops = &ion_dma_buf_ops;
	exp_info.size = buffer->size;
	exp_info.flags = O_RDWR;
	exp_info.priv = buffer;

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		perrfn("failed to export dmabuf (err %ld)", -PTR_ERR(dmabuf));
		_ion_buffer_destroy(buffer);
	}

	ion_event_end(ION_EVENT_TYPE_ALLOC, buffer);

	return dmabuf;
}

int ion_alloc(size_t len, unsigned int heap_id_mask, unsigned int flags)
{
	struct dma_buf *dmabuf = __ion_alloc(len, heap_id_mask, flags);
	int fd;

	if (IS_ERR(dmabuf))
		return PTR_ERR(dmabuf);

	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (fd < 0) {
		perrfn("failed to get dmabuf fd (err %d)",  -fd);
		dma_buf_put(dmabuf);
	}

	return fd;
}

int ion_query_heaps(struct ion_heap_query *query)
{
	struct ion_device *dev = internal_dev;
	struct ion_heap_data __user *buffer = u64_to_user_ptr(query->heaps);
	int ret = -EINVAL, cnt = 0, max_cnt;
	struct ion_heap *heap;
	struct ion_heap_data hdata;

	down_read(&dev->lock);
	if (!buffer) {
		query->cnt = dev->heap_cnt;
		ret = 0;
		goto out;
	}

	if (query->cnt <= 0) {
		perrfn("invalid heapdata count %u",  query->cnt);
		goto out;
	}

	max_cnt = query->cnt;

	plist_for_each_entry(heap, &dev->heaps, node) {
		memset(&hdata, 0, sizeof(hdata));

		strncpy(hdata.name, heap->name, MAX_HEAP_NAME);
		hdata.name[sizeof(hdata.name) - 1] = '\0';
		hdata.type = heap->type;
		hdata.heap_id = heap->id;

		if (heap->flags & ION_HEAP_FLAG_DEFER_FREE)
			hdata.heap_flags = ION_HEAPDATA_FLAGS_DEFER_FREE;

		if (heap->ops->query_heap)
			heap->ops->query_heap(heap, &hdata);

		if (copy_to_user(&buffer[cnt], &hdata, sizeof(hdata))) {
			ret = -EFAULT;
			goto out;
		}

		cnt++;
		if (cnt >= max_cnt)
			break;
	}

	query->cnt = cnt;
	ret = 0;
out:
	up_read(&dev->lock);
	return ret;
}

struct ion_heap *ion_get_heap_by_name(const char *heap_name)
{
	struct ion_device *dev = internal_dev;
	struct ion_heap *heap;

	plist_for_each_entry(heap, &dev->heaps, node) {
		if (strlen(heap_name) != strlen(heap->name))
			continue;
		if (strcmp(heap_name, heap->name) == 0)
			return heap;
	}

	return NULL;
}

static const struct file_operations ion_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = ion_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ion_ioctl,
#endif
};

static int debug_shrink_set(void *data, u64 val)
{
	struct ion_heap *heap = data;
	struct shrink_control sc;
	int objs;

	sc.gfp_mask = GFP_HIGHUSER;
	sc.nr_to_scan = val;

	if (!val) {
		objs = heap->shrinker.count_objects(&heap->shrinker, &sc);
		sc.nr_to_scan = objs;
	}

	heap->shrinker.scan_objects(&heap->shrinker, &sc);
	return 0;
}

static int debug_shrink_get(void *data, u64 *val)
{
	struct ion_heap *heap = data;
	struct shrink_control sc;
	int objs;

	sc.gfp_mask = GFP_HIGHUSER;
	sc.nr_to_scan = 0;

	objs = heap->shrinker.count_objects(&heap->shrinker, &sc);
	*val = objs;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_shrink_fops, debug_shrink_get,
			debug_shrink_set, "%llu\n");

void ion_device_add_heap(struct ion_heap *heap)
{
	struct dentry *debug_file;
	struct ion_device *dev = internal_dev;

	if (!heap->ops->allocate || !heap->ops->free)
		perrfn("can not add heap with invalid ops struct.");

	spin_lock_init(&heap->free_lock);
	heap->free_list_size = 0;

	if (heap->flags & ION_HEAP_FLAG_DEFER_FREE)
		ion_heap_init_deferred_free(heap);

	if ((heap->flags & ION_HEAP_FLAG_DEFER_FREE) || heap->ops->shrink)
		ion_heap_init_shrinker(heap);

	heap->dev = dev;
	down_write(&dev->lock);
	heap->id = heap_id++;
	/*
	 * use negative heap->id to reverse the priority -- when traversing
	 * the list later attempt higher id numbers first
	 */
	plist_node_init(&heap->node, -heap->id);
	plist_add(&heap->node, &dev->heaps);

	if (heap->shrinker.count_objects && heap->shrinker.scan_objects) {
		char debug_name[64];

		snprintf(debug_name, 64, "%s_shrink", heap->name);
		debug_file = debugfs_create_file(
			debug_name, 0644, dev->debug_root, heap,
			&debug_shrink_fops);
		if (!debug_file) {
			char buf[256], *path;

			path = dentry_path(dev->debug_root, buf, 256);
			perr("Failed to create heap shrinker debugfs at %s/%s",
			     path, debug_name);
		}
	}

	ion_debug_heap_init(heap);

	dev->heap_cnt++;
	up_write(&dev->lock);
}
EXPORT_SYMBOL(ion_device_add_heap);

static int ion_device_create(void)
{
	struct ion_device *idev;
	int ret;

	idev = kzalloc(sizeof(*idev), GFP_KERNEL);
	if (!idev)
		return -ENOMEM;

	idev->dev.minor = MISC_DYNAMIC_MINOR;
	idev->dev.name = "ion";
	idev->dev.fops = &ion_fops;
	idev->dev.parent = NULL;
	ret = misc_register(&idev->dev);
	if (ret) {
		perr("ion: failed to register misc device.");
		kfree(idev);
		return ret;
	}

	idev->debug_root = debugfs_create_dir("ion", NULL);
	if (!idev->debug_root) {
		perr("ion: failed to create debugfs root directory.");
		goto debugfs_done;
	}

	ion_debug_initialize(idev);

debugfs_done:
	exynos_ion_fixup(idev);
	idev->buffers = RB_ROOT;
	mutex_init(&idev->buffer_lock);
	init_rwsem(&idev->lock);
	plist_head_init(&idev->heaps);
	internal_dev = idev;
	return 0;
}
subsys_initcall(ion_device_create);
