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
#include <linux/device.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/anon_inodes.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>

#include "dma-buf-trace.h"

struct dmabuf_trace_ref {
	struct list_head task_node;
	struct list_head buffer_node;

	struct dmabuf_trace_task *task;
	struct dmabuf_trace_buffer *buffer;
	int refcount;
};

struct dmabuf_trace_task {
	struct list_head node;
	struct list_head ref_list;

	struct task_struct *task;
	struct file *file;
	struct dentry *debug_task;
};

struct dmabuf_trace_buffer {
	struct list_head node;
	struct list_head ref_list;

	struct dma_buf *dmabuf;
	int shared_count;
};

static struct list_head buffer_list = LIST_HEAD_INIT(buffer_list);

/*
 * head_task.node is the head node of all other dmabuf_trace_task.node.
 * At the same time, head_task itself maintains the buffer information allocated
 * by the kernel threads.
 */
static struct dmabuf_trace_task head_task;
static DEFINE_MUTEX(trace_lock);

static struct dentry *debug_root;

static int dmabuf_trace_debug_show(struct seq_file *s, void *unused)
{
	struct dmabuf_trace_task *task = s->private;
	struct dmabuf_trace_ref *ref;

	mutex_lock(&trace_lock);
	seq_puts(s, "\nDma-buf-trace Objects:\n");
	seq_printf(s, "%10s %12s %12s %10s\n",
		   "exp_name", "size", "share", "refcount");

	list_for_each_entry(ref, &task->ref_list, task_node) {
		seq_printf(s, "%10s %12zu %12zu %10d\n",
			   ref->buffer->dmabuf->exp_name,
			   ref->buffer->dmabuf->size,
			   ref->buffer->dmabuf->size /
			   ref->buffer->shared_count,
			   ref->refcount);
	}
	mutex_unlock(&trace_lock);

	return 0;
}

static int dmabuf_trace_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, dmabuf_trace_debug_show, inode->i_private);
}

static const struct file_operations dmabuf_trace_debug_fops = {
	.open = dmabuf_trace_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void dmabuf_trace_free_ref_force(struct dmabuf_trace_ref *ref)
{
	ref->buffer->shared_count--;

	list_del(&ref->buffer_node);
	list_del(&ref->task_node);

	kfree(ref);
}

static int dmabuf_trace_free_ref(struct dmabuf_trace_ref *ref)
{
	/* The reference has never been registered */
	if (WARN_ON(ref->refcount == 0))
		return -EINVAL;

	if (--ref->refcount == 0)
		dmabuf_trace_free_ref_force(ref);

	return 0;
}

static int dmabuf_trace_task_release(struct inode *inode, struct file *file)
{
	struct dmabuf_trace_task *task = file->private_data;
	struct dmabuf_trace_ref *ref, *tmp;

	if (!(task->task->flags & PF_EXITING)) {
		pr_err("%s: Invalid to close '%d' on process '%s'(%x, %lx)\n",
		       __func__, task->task->pid, task->task->comm,
		       task->task->flags, task->task->state);

		dump_stack();
	}

	put_task_struct(task->task);

	mutex_lock(&trace_lock);

	list_for_each_entry_safe(ref, tmp, &task->ref_list, task_node)
		dmabuf_trace_free_ref_force(ref);

	list_del(&task->node);

	mutex_unlock(&trace_lock);

	debugfs_remove(task->debug_task);

	kfree(task);

	return 0;
}

static const struct file_operations dmabuf_trace_task_fops = {
	.release = dmabuf_trace_task_release,
};

static struct dmabuf_trace_buffer *dmabuf_trace_get_buffer(
		struct dma_buf *dmabuf)
{
	struct dmabuf_trace_buffer *buffer;

	list_for_each_entry(buffer, &buffer_list, node)
		if (buffer->dmabuf == dmabuf)
			return buffer;

	return NULL;
}

static struct dmabuf_trace_task *dmabuf_trace_get_task_noalloc(void)
{
	struct dmabuf_trace_task *task;

	if (!current->mm && (current->flags & PF_KTHREAD))
		return &head_task;

	list_for_each_entry(task, &head_task.node, node)
		if (task->task == current->group_leader)
			return task;

	return NULL;
}

static struct dmabuf_trace_task *dmabuf_trace_get_task(void)
{
	struct dmabuf_trace_task *task;
	unsigned char name[10];
	int ret, fd;

	task = dmabuf_trace_get_task_noalloc();
	if (task)
		return task;

	task = kzalloc(sizeof(*task), GFP_KERNEL);
	if (!task)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&task->node);
	INIT_LIST_HEAD(&task->ref_list);

	scnprintf(name, 10, "%d", current->group_leader->pid);

	get_task_struct(current->group_leader);

	task->task = current->group_leader;
	task->debug_task = debugfs_create_file(name, 0444,
					       debug_root, task,
					       &dmabuf_trace_debug_fops);
	if (IS_ERR(task->debug_task)) {
		ret = PTR_ERR(task->debug_task);
		goto err_debugfs;
	}

	ret = get_unused_fd_flags(O_RDONLY | O_CLOEXEC);
	if (ret < 0)
		goto err_fd;
	fd = ret;

	task->file = anon_inode_getfile(name, &dmabuf_trace_task_fops,
					task, O_RDWR);
	if (IS_ERR(task->file)) {
		ret = PTR_ERR(task->file);

		goto err_inode;
	}

	fd_install(fd, task->file);

	list_add_tail(&task->node, &head_task.node);

	return task;

err_inode:
	put_unused_fd(fd);
err_fd:
	debugfs_remove(task->debug_task);
err_debugfs:
	put_task_struct(current->group_leader);

	kfree(task);

	return ERR_PTR(ret);

}

static struct dmabuf_trace_ref *dmabuf_trace_get_ref_noalloc(
		struct dmabuf_trace_buffer *buffer,
		struct dmabuf_trace_task *task)
{
	struct dmabuf_trace_ref *ref;

	list_for_each_entry(ref, &task->ref_list, task_node)
		if (ref->buffer == buffer)
			return ref;

	return NULL;
}

static struct dmabuf_trace_ref *dmabuf_trace_get_ref(
		struct dmabuf_trace_buffer *buffer,
		struct dmabuf_trace_task *task)
{
	struct dmabuf_trace_ref *ref;

	ref = dmabuf_trace_get_ref_noalloc(buffer, task);
	if (ref) {
		ref->refcount++;
		return ref;
	}

	ref = kzalloc(sizeof(*ref), GFP_KERNEL);
	if (!ref)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&ref->buffer_node);
	INIT_LIST_HEAD(&ref->task_node);

	ref->task = task;
	ref->buffer = buffer;
	ref->refcount = 1;

	list_add_tail(&ref->task_node, &task->ref_list);
	list_add_tail(&ref->buffer_node, &buffer->ref_list);

	buffer->shared_count++;

	return ref;
}

/**
 * dmabuf_trace_alloc - get reference after creating dmabuf.
 * @dmabuf : buffer to register reference.
 *
 * This create a ref that has relationship between dmabuf
 * and process that requested allocation, and also create
 * the buffer object to trace.
 */
int dmabuf_trace_alloc(struct dma_buf *dmabuf)
{
	struct dmabuf_trace_buffer *buffer;
	struct dmabuf_trace_task *task;
	struct dmabuf_trace_ref *ref;
	int ret = -ENOMEM;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ret;

	INIT_LIST_HEAD(&buffer->ref_list);

	buffer->dmabuf = dmabuf;
	buffer->shared_count = 1;

	ref = kzalloc(sizeof(*ref), GFP_KERNEL);
	if (!ref)
		goto err_ref;

	ref->buffer = buffer;

	/*
	 * The interim ref created in dmabuf_trace_alloc is just for counting
	 * the buffer references correctly. The interim ref has zero refcount.
	 * If user explicitly registers a ref, it is handled in
	 * dmabuf_trace_track_buffer(). If first dmabuf_trace_track_buffer() is
	 * called in the same task that allocated the buffer, the interim ref
	 * becomes the regular ref that has larget refcount than 0. If the first
	 * call to dmabuf_trace_track_buffer() made in other tasks, the interim
	 * ref is removed and a new regular ref is created and registered
	 * instead of the interim ref.
	 */
	ref->refcount = 0;

	mutex_lock(&trace_lock);

	task = dmabuf_trace_get_task();
	if (IS_ERR(task)) {
		mutex_unlock(&trace_lock);
		ret = PTR_ERR(task);

		goto err_task;
	}

	ref->task = task;

	list_add_tail(&buffer->node, &buffer_list);

	list_add_tail(&ref->task_node, &task->ref_list);
	list_add_tail(&ref->buffer_node, &buffer->ref_list);

	mutex_unlock(&trace_lock);

	return 0;
err_task:
	kfree(ref);
err_ref:
	kfree(buffer);
	pr_err("%s: Failed to trace dmabuf after to export(err %d)\n",
	       __func__, ret);

	return ret;
}

/**
 * dmabuf_trace_free - release references after removing buffer.
 * @dmabuf : buffer to release reference.
 *
 * This remove refs that connected with released dmabuf.
 */
void dmabuf_trace_free(struct dma_buf *dmabuf)
{
	struct dmabuf_trace_buffer *buffer;
	struct dmabuf_trace_ref *ref, *tmp;

	mutex_lock(&trace_lock);

	buffer = dmabuf_trace_get_buffer(dmabuf);

	list_for_each_entry_safe(ref, tmp, &buffer->ref_list, buffer_node)
		dmabuf_trace_free_ref_force(ref);

	list_del(&buffer->node);

	mutex_unlock(&trace_lock);

	kfree(buffer);
}

/**
 * dmabuf_trace_register - create ref between task and buffer.
 * @dmabuf : buffer to register reference.
 *
 * This create ref between current task and buffer.
 */
int dmabuf_trace_track_buffer(struct dma_buf *dmabuf)
{
	struct dmabuf_trace_buffer *buffer;
	struct dmabuf_trace_task *task;
	struct dmabuf_trace_ref *ref;
	int ret = 0;

	mutex_lock(&trace_lock);
	task = dmabuf_trace_get_task();
	if (IS_ERR(task)) {
		ret = PTR_ERR(task);
		goto err;
	}

	buffer = dmabuf_trace_get_buffer(dmabuf);
	ref = dmabuf_trace_get_ref(buffer, task);
	if (IS_ERR(ref)) {
		/*
		 * task allocated by dmabuf_trace_get_task() is not freed here.
		 * It is deallocated when the process exits.
		 */
		ret = PTR_ERR(ref);
		goto err;
	}

	ref = list_first_entry(&buffer->ref_list, struct dmabuf_trace_ref,
			       buffer_node);
	/*
	 * If first ref of buffer has zero refcount, it is an interim ref
	 * created in dmabuf_trace_alloc() called by other tasks than current.
	 * The interim ref should be removed after the regular ref created here
	 * is registered.
	 */
	if (ref->refcount == 0)
		dmabuf_trace_free_ref_force(ref);
err:
	mutex_unlock(&trace_lock);

	if (ret)
		pr_err("%s: Failed to trace dmabuf (err %d)\n", __func__, ret);
	return ret;
}

/**
 * dmabuf_trace_unregister - remove ref between task and buffer.
 * @dmabuf : buffer to unregister reference.
 *
 * This remove ref between current task and buffer.
 */
int dmabuf_trace_untrack_buffer(struct dma_buf *dmabuf)
{
	struct dmabuf_trace_buffer *buffer;
	struct dmabuf_trace_task *task;
	struct dmabuf_trace_ref *ref;
	int ret;

	mutex_lock(&trace_lock);
	task = dmabuf_trace_get_task_noalloc();
	if (!task) {
		ret = -ESRCH;
		goto err_unregister;
	}

	buffer = dmabuf_trace_get_buffer(dmabuf);
	ref = dmabuf_trace_get_ref_noalloc(buffer, task);
	if (!ref) {
		ret = -ENOENT;
		goto err_unregister;
	}

	ret = dmabuf_trace_free_ref(ref);

err_unregister:
	if (ret)
		pr_err("%s: Failed to untrace dmabuf(err %d)", __func__, ret);

	mutex_unlock(&trace_lock);

	return ret;
}

static int __init dmabuf_trace_create(void)
{
	debug_root = debugfs_create_dir("footprint", dma_buf_debugfs_dir);
	if (IS_ERR(debug_root)) {
		pr_err("%s : Failed to create directory\n", __func__);

		return PTR_ERR(debug_root);
	}

	/*
	 * PID 1 is actually the pid of the init process. dma-buf trace borrows
	 * pid 1 for the buffers allocated by the kernel threads to provide
	 * full buffer information to Android Memory Tracker.
	 */
	head_task.debug_task = debugfs_create_file("0", 0444,
						   debug_root, &head_task,
						   &dmabuf_trace_debug_fops);
	if (IS_ERR(head_task.debug_task)) {
		debugfs_remove(debug_root);
		pr_err("%s: Failed to create task for kernel thread\n",
		       __func__);
		return PTR_ERR(head_task.debug_task);
	}

	INIT_LIST_HEAD(&head_task.node);
	INIT_LIST_HEAD(&head_task.ref_list);

	pr_info("Initialized dma-buf trace successfully.\n");

	return 0;
}
fs_initcall_sync(dmabuf_trace_create);
