/*
 * drivers/staging/android/ion/ion_debug.c
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
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
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/debugfs.h>
#include <linux/oom.h>
#include <linux/sort.h>

#include "ion.h"
#include "ion_exynos.h"
#include "ion_debug.h"

#define ION_MAX_EVENT_LOG	1024
#define ION_EVENT_CLAMP_ID(id) ((id) & (ION_MAX_EVENT_LOG - 1))

#define ion_debug_print(s, fmt, ...) \
	((s) ? seq_printf(s, fmt, ##__VA_ARGS__) :\
	 pr_info(fmt, ##__VA_ARGS__))

static atomic_t eventid;

static char * const ion_event_name[] = {
	"alloc",
	"free",
	"mmap",
	"kmap",
	"map_dma_buf",
	"unmap_dma_buf",
	"begin_cpu_access",
	"end_cpu_access",
	"iovmm_map",
};

static struct ion_event {
	unsigned char heapname[8];
	unsigned long data;
	ktime_t begin;
	ktime_t done;
	size_t size;
	enum ion_event_type type;
	int buffer_id;
} eventlog[ION_MAX_EVENT_LOG];

static void ion_buffer_dump_flags(struct seq_file *s, unsigned long flags)
{
	if (flags & ION_FLAG_CACHED)
		seq_puts(s, "cached");
	else
		seq_puts(s, "noncached");

	if (flags & ION_FLAG_NOZEROED)
		seq_puts(s, "|nozeroed");

	if (flags & ION_FLAG_PROTECTED)
		seq_puts(s, "|protected");

	if (flags & ION_FLAG_MAY_HWRENDER)
		seq_puts(s, "|may_hwrender");
}

#ifdef CONFIG_ION_DEBUG_EVENT_RECORD
void ion_event_record(enum ion_event_type type,
		      struct ion_buffer *buffer, ktime_t begin)
{
	int idx = ION_EVENT_CLAMP_ID(atomic_inc_return(&eventid));
	struct ion_event *event = &eventlog[idx];

	event->buffer_id = buffer->id;
	event->type = type;
	event->begin = begin;
	event->done = ktime_get();
	event->size = buffer->size;
	event->data = (type == ION_EVENT_TYPE_FREE) ?
		buffer->private_flags & ION_PRIV_FLAG_SHRINKER_FREE :
		buffer->flags;
	strlcpy(event->heapname, buffer->heap->name, sizeof(event->heapname));
}
#endif

inline bool ion_debug_event_show_single(struct seq_file *s,
				struct ion_event *event)
{
	struct timeval tv = ktime_to_timeval(event->begin);
	long elapsed = ktime_us_delta(event->done, event->begin);

	if (elapsed == 0)
		return false;

	seq_printf(s, "[%06ld.%06ld] ", tv.tv_sec, tv.tv_usec);
	seq_printf(s, "%17s %8s %10d %13zd %10ld",
		   ion_event_name[event->type], event->heapname,
		   event->buffer_id, event->size / SZ_1K, elapsed);

	if (elapsed > 100 * USEC_PER_MSEC)
		seq_puts(s, " *");

	switch (event->type) {
	case ION_EVENT_TYPE_ALLOC:
		seq_puts(s, "  ");
		ion_buffer_dump_flags(s, event->data);
		break;
	case ION_EVENT_TYPE_FREE:
		if (event->data)
			seq_puts(s, " shrinker");
		break;
	default:
		break;
	}

	seq_puts(s, "\n");

	return true;
}

static int ion_debug_event_show(struct seq_file *s, void *unused)
{
	int i;
	int idx = ION_EVENT_CLAMP_ID(atomic_read(&eventid) + 1);

	seq_printf(s, "%15s %17s %18s %10s %13s %10s %24s\n",
		   "timestamp", "type", "heap", "buffer_id", "size (kb)",
		   "time (us)", "remarks");
	seq_puts(s, "-------------------------------------------");
	seq_puts(s, "-------------------------------------------");
	seq_puts(s, "-----------------------------------------\n");

	for (i = idx; i < ION_MAX_EVENT_LOG; i++)
		if (!ion_debug_event_show_single(s, &eventlog[i]))
			break;

	for (i = 0; i < idx; i++)
		if (!ion_debug_event_show_single(s, &eventlog[i]))
			break;

	return 0;
}

static int ion_debug_event_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_event_show, inode->i_private);
}

static const struct file_operations debug_event_fops = {
	.open = ion_debug_event_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int contig_heap_cmp(const void *l, const void *r)
{
	struct ion_buffer *left = *((struct ion_buffer **)l);
	struct ion_buffer *right = *((struct ion_buffer **)r);

	return ((unsigned long)sg_phys(left->sg_table->sgl)) -
		((unsigned long)sg_phys(right->sg_table->sgl));
}

void ion_contig_heap_show_buffers(struct seq_file *s, struct ion_heap *heap,
				  phys_addr_t base, size_t pool_size)
{
	size_t total_size = 0;
	struct rb_node *n;
	struct ion_buffer **sorted = NULL;
	int i, count = 64, ptr = 0;

	if (heap->type != ION_HEAP_TYPE_CARVEOUT &&
	    heap->type != ION_HEAP_TYPE_DMA)
		return;

	ion_debug_print(s,
			"ION heap '%s' of type %u and id %u, size %zu bytes\n",
			heap->name, heap->type, heap->id, pool_size);

	mutex_lock(&heap->dev->buffer_lock);
	for (n = rb_first(&heap->dev->buffers); n; n = rb_next(n)) {
		struct ion_buffer *buffer = rb_entry(n, struct ion_buffer,
						     node);

		if (buffer->heap != heap)
			continue;

		if (!sorted)
			sorted = kmalloc_array(count, sizeof(*sorted),
					       GFP_KERNEL);

		if (ptr == count) {
			struct ion_buffer **tmp;

			tmp = kmalloc_array(2 * count, sizeof(*sorted),
					    GFP_KERNEL);
			memcpy(tmp, sorted, sizeof(*sorted) * count);
			count *= 2;
			kfree(sorted);

			sorted = tmp;
		}

		sorted[ptr++] = buffer;
	}

	if (!sorted) {
		mutex_unlock(&heap->dev->buffer_lock);
		return;
	}

	sort(sorted, ptr, sizeof(*sorted), contig_heap_cmp, NULL);

	for (i = 0; i < ptr; i++) {
		unsigned long cur, next;

		cur = (unsigned long)(sg_phys(sorted[i]->sg_table->sgl) - base);
		next = (i == ptr - 1) ?
			pool_size : (unsigned long)
			(sg_phys(sorted[i + 1]->sg_table->sgl) - base);

		ion_debug_print(s,
				"offset %#010lx size %10zu (free %10zu)\n",
				cur, sorted[i]->size,
				next - cur - sorted[i]->size);

		total_size += sorted[i]->size;
	}

	ion_debug_print(s, "Total allocated size: %zu bytes --------------\n",
			total_size);

	mutex_unlock(&heap->dev->buffer_lock);
	kfree(sorted);
}

const static char *heap_type_name[] = {
	"system",
	"syscntig",
	"carveout",
	"chunk",
	"cma",
	"hpa",
};

static void ion_debug_freed_buffer(struct seq_file *s, struct ion_device *dev)
{
	struct ion_heap *heap;
	struct ion_buffer *buffer;

	down_read(&dev->lock);
	plist_for_each_entry(heap, &dev->heaps, node) {
		if (!(heap->flags & ION_HEAP_FLAG_DEFER_FREE))
			continue;

		spin_lock(&heap->free_lock);
		/* buffer lock is not required because the buffer is freed */
		list_for_each_entry(buffer, &heap->free_list, list) {
			unsigned int heaptype = (buffer->heap->type <
				ARRAY_SIZE(heap_type_name)) ?
				buffer->heap->type : 0;

			ion_debug_print(s,
					"[%4d] %15s %8s %#5lx %8zu : freed\n",
					buffer->id, buffer->heap->name,
					heap_type_name[heaptype], buffer->flags,
					buffer->size / SZ_1K);
		}
		spin_unlock(&heap->free_lock);
	}
	up_read(&dev->lock);
}

static void ion_debug_buffer_for_heap(struct seq_file *s,
				      struct ion_device *dev,
				      struct ion_heap *heap)
{
	struct rb_node *n;
	struct ion_buffer *buffer;
	size_t total = 0, peak = 0;

	ion_debug_print(s, "[  id] %15s %8s %5s %8s : %s\n",
			"heap", "heaptype", "flags", "size(kb)",
			"iommu_mapped...");

	mutex_lock(&dev->buffer_lock);
	for (n = rb_first(&dev->buffers); n; n = rb_next(n)) {
		buffer = rb_entry(n, struct ion_buffer, node);

		if (!heap || heap == buffer->heap) {
			unsigned int heaptype = (buffer->heap->type <
				ARRAY_SIZE(heap_type_name)) ?
				buffer->heap->type : 0;

			ion_debug_print(s, "[%4d] %15s %8s %#5lx %8zu %16s %16u (%16s %16u)",
					buffer->id, buffer->heap->name,
					heap_type_name[heaptype], buffer->flags,
					buffer->size / SZ_1K,
					buffer->task_comm, buffer->pid,
					buffer->thread_comm, buffer->tid);

			ion_debug_print(s, "\n");

			total += buffer->size;
		}
	}
	mutex_unlock(&dev->buffer_lock);

	total /= SZ_1K;

	ion_debug_print(s, "TOTAL: %zu kb\n", total);

	if (heap) {
		peak = atomic_long_read(&heap->total_allocated_peak) / SZ_1K;
		ion_debug_print(s, "PEAK : %zu kb\n", peak);
	}
}

#define ion_debug_buffer_for_all_heap(s, dev) \
	ion_debug_buffer_for_heap(s, dev, NULL)

static int ion_debug_heap_show(struct seq_file *s, void *unused)
{
	struct ion_heap *heap = s->private;

	seq_printf(s, "ION heap '%s' of type %u and id %u\n",
		   heap->name, heap->type, heap->id);

	ion_debug_buffer_for_heap(s, heap->dev, heap);

	if (heap->debug_show)
		heap->debug_show(heap, s, unused);

	return 0;
}

static int ion_debug_heap_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_heap_show, inode->i_private);
}

static const struct file_operations debug_heap_fops = {
	.open = ion_debug_heap_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void ion_debug_heap_init(struct ion_heap *heap)
{
	char debug_name[64];
	struct dentry *heap_file;

	if (!heap->dev->heaps_debug_root)
		return;

	snprintf(debug_name, 64, "%s", heap->name);

	heap_file = debugfs_create_file(
			debug_name, 0444, heap->dev->heaps_debug_root,
			heap, &debug_heap_fops);
	if (!heap_file) {
		char buf[256], *path;

		path = dentry_path(heap->dev->heaps_debug_root,
				   buf, 256);
		perrfn("failed to create %s/%s", path, heap_file);
	}
}

static int ion_debug_buffers_show(struct seq_file *s, void *unused)
{
	struct ion_device *idev = s->private;

	ion_debug_buffer_for_all_heap(s, idev);
	ion_debug_freed_buffer(s, idev);

	return 0;
}

static int ion_debug_buffers_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_buffers_show, inode->i_private);
}

static const struct file_operations debug_buffers_fops = {
	.open = ion_debug_buffers_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

struct ion_oom_notifier_struct {
	struct notifier_block nb;
	struct ion_device *idev;
};

static int ion_oom_notifier_fn(struct notifier_block *nb,
			       unsigned long action, void *data)
{
	struct ion_device *idev =
		container_of(nb, struct ion_oom_notifier_struct, nb)->idev;

	/*
	 * The oom notifier should be careful to use the lock because
	 * it makes recursive lock. If the ion get the lock and tries to
	 * allocate the memory and then oom notifier happens and oom notifier
	 * tries to get the same lock, it makes recursive lock.
	 */
	ion_debug_buffer_for_all_heap(NULL, idev);
	ion_debug_freed_buffer(NULL, idev);

	return 0;
}

static struct ion_oom_notifier_struct ion_oom_notifier = {
	.nb = { .notifier_call = ion_oom_notifier_fn}
};

void ion_debug_initialize(struct ion_device *idev)
{
	struct dentry *buffer_file, *event_file;

	buffer_file = debugfs_create_file("buffers", 0444, idev->debug_root,
					  idev, &debug_buffers_fops);
	if (!buffer_file)
		perrfn("failed to create debugfs/ion/buffers");

	event_file = debugfs_create_file("event", 0444, idev->debug_root,
					 idev, &debug_event_fops);
	if (!event_file)
		perrfn("failed to create debugfs/ion/event");

	idev->heaps_debug_root = debugfs_create_dir("heaps", idev->debug_root);
	if (!idev->heaps_debug_root)
		perrfn("failed to create debugfs/ion/heaps directory");

	ion_oom_notifier.idev = idev;
	register_oom_notifier(&ion_oom_notifier.nb);

	atomic_set(&eventid, -1);
}
