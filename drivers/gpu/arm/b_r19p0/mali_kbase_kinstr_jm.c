/*
 *
 * (C) COPYRIGHT 2019 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

/*
 * mali_kbase_kinstr_jm.c
 * Kernel driver public interface to job manager atom tracing
 */

#include "mali_kbase_kinstr_jm.h"
#include "mali_kbase_kinstr_jm_reader.h"

#include "mali_kbase.h"
#include "mali_kbase_linux.h"

#include <mali_kbase_jm_rb.h>

#include <asm-generic/barrier.h>
#include <linux/anon_inodes.h>
#include <linux/circ_buf.h>
#include <linux/mm.h>
#include <linux/rculist_bl.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#if KERNEL_VERSION(4, 16, 0) >= LINUX_VERSION_CODE
typedef unsigned int __poll_t;
#endif

/* The module printing prefix */
#define PR_ "mali_kbase_kinstr_jm: "

/* Allows use to perform ASM goto for the tracing
 * https://www.kernel.org/doc/Documentation/static-keys.txt
 */
DEFINE_STATIC_KEY_FALSE(basep_kinstr_jm_reader_static_key);

/**
 * struct kbase_kinstr_jm_atom_state_change - Represents an atom changing to a
 *                                            new state
 * @timestamp: Raw monotonic nanoseconds of the state change
 * @state:     The state that the atom has moved to
 * @atom:      The atom number that has changed state
 *
 * The KBASE_KINSTR_JM_READER_LAYOUT IO control call provides the layout of this
 * structure to user space. This allows it to understand how to be backwards and
 * forwards compatible.
 *
 * We can re-order fields and user code can still pull the data out because it
 * will know the layout of the fields.
 *
 * We can add new fields to the structure and old user code will gracefully
 * ignore the new fields.
 *
 * We can change the size of the structure and old user code will gracefully
 * skip over the new size from the layout data.
 *
 * If we remove fields, old user code will gracefully understand that the
 * necessary fields are not available and tell the user that the kernel API is
 * no longer supported.
 *
 * If we need to change the _meaning_ of one of the fields, i.e. the state
 * machine has had a incompatible change, we can keep the same members in the
 * structure and update the corresponding kbase_kinstr_jm_reader_layout_kind to
 * a new value. User code will no longer recognise that it has the supported
 * field and can gracefully explain to the user that the kernel API is no longer
 * supported.
 *
 * When making changes to this structure, make sure to update the layout
 * representation call in reader_ioctl_layout()
 */
struct kbase_kinstr_jm_atom_state_change {
	u64 timestamp;
	enum kbase_kinstr_jm_reader_atom_state state;
	u8 atom;
};

/**
 * struct reader_changes_buffer - The buffer of kernel atom state changes
 * @data: The allocated buffer. This is allocated when the user invokes mmap()
 *        on the reader file descriptor. It is released when the user munmaps()
 *        the memory. When accessing this, lock the producer spin lock to
 *        prevent races on the allocated memory.
 * @size: The number of changes that are allowed in @c data. Can be used with
 *        Linux @c CIRC_ helpers. Will always be a power of two.
 *
 * We use this so we can make it constant across mmap/munmap pairings
 */
struct reader_changes_buffer {
	struct kbase_kinstr_jm_atom_state_change *data;
	u16 size;
};

/**
 * struct reader_changes - The circular buffer of kernel atom state changes
 * @buffer:   The allocated buffer. This is allocated when the user invokes
 *            mmap() on the reader file descriptor. It is released when the user
 *            munmaps() the memory. When accessing this, lock the producer spin
 *            lock to prevent races on the allocated memory.
 * @producer: The producing spinlock which allows us to push changes into the
 *            buffer at the same time as a user view occurring
 * @consumer: The consuming spinlock which locks around the user view/advance
 * @overflow: Determines how many state changes overflowed the full buffer
 * @head:     The head of the circular buffer. Can be used with Linux @c CIRC_
 *            helpers
 * @tail:     The tail of the circular buffer. Can be used with Linux @c CIRC_
 *            helpers
 *
 * The circular buffer indexes are unsigned shorts as we should never need to
 * buffer more that 65535 state changes. If we *are* overflowing that amount,
 * then the reader isn't reading fast enough or we are doing an exceptional
 * amount of work on the GPU. The threshold maxes out at a page so flushing will
 * occur way before 65535 is reached.
 */
struct reader_changes {
	const struct reader_changes_buffer buffer;
	spinlock_t producer;
	spinlock_t consumer;
	size_t overflow;
	u16 head;
	u16 tail;
};

/**
 * reader_changes_threshold() - Determines the threshold of state changes that,
 *                              when reached, we should wake polls on the file
 *                              descriptor
 * @changes: The state changes context
 * Return: the threshold
 */
static inline u16 reader_changes_threshold(
		const struct reader_changes *const changes) {
	const struct reader_changes_buffer *buffer = &changes->buffer;

	return min(((size_t)(buffer->size)) / 4,
		   ((size_t)(PAGE_SIZE)) / sizeof(*buffer->data));
}

/**
 * reader_changes_count() - Retrieves the count of state changes in the buffer
 * @changes: The state changes context
 *
 * Locks the changes consumer spinlock and uses the CIRC_CNT macro to determine
 * the count.
 *
 * Return: the number of changes in the circular buffer
 */
static u16 reader_changes_count(struct reader_changes *const changes)
{
	u16 head, tail, size, count;

	spin_lock(&changes->consumer);
	size = smp_load_acquire(&changes->buffer.size);
	head = smp_load_acquire(&changes->head);
	tail = changes->tail;
	count = CIRC_CNT(head, tail, size) >= 1;
	spin_unlock(&changes->consumer);

	return count;
}

/**
 * reader_changes_push() - Pushes a change into the reader circular buffer.
 * @changes:    The change to insert the change into
 * @change:     Kernel atom change to insert
 * @wait_queue: The queue to be kicked when changes should be read from
 *              userspace. Kicked when a threshold is reached or there is
 *              overflow.
 *
 * This function assumes that the reader list has been locked via the head of
 * the list.
 */
static void reader_changes_push(
	struct reader_changes *const changes,
	const struct kbase_kinstr_jm_atom_state_change *const change,
	struct wait_queue_head *const wait_queue)
{
	u16 head, tail, size, threshold;
	size_t overflow;

	if (unlikely(!changes->buffer.data))
		return;

	spin_lock(&changes->producer);

	head = changes->head;
	tail = READ_ONCE(changes->tail);
	size = changes->buffer.size;
	overflow = changes->overflow;

	if (CIRC_SPACE(head, tail, size) >= 1) {
		changes->buffer.data[head] = *change;

		smp_store_release(&changes->head, (head + 1) & (size - 1));

		threshold = reader_changes_threshold(changes);

		if (CIRC_CNT(head + 1, tail, size) >= threshold)
			wake_up_interruptible(wait_queue);
	} else if (overflow < SIZE_MAX) {
		if (unlikely(!overflow))
			pr_warn(PR_ "overflow of circular buffer\n");
		smp_store_release(&changes->overflow, (overflow + 1));

		wake_up_interruptible(wait_queue);
	}

	spin_unlock(&changes->producer);
}

struct reader;

/**
 *typedef reader_release_callback - Invoked when a reader is released due to the
 *                                  file descriptor being closed
 *
 * @count: The number of kernel atom state changes that were missed
 * @user:  Any user data that was provided
 */
typedef void (*reader_callback_release)(
	struct reader *const count, void *user);

/**
 *struct reader_binding_release - A callback binding that when a reader is
 *                                released due to the file descriptor being
 *                                closed
 * @callback: The function pointer that will be invoked
 * @user:     The user data that will be passed to the callback
 */
struct reader_binding_release {
	reader_callback_release callback;
	void *user;
};

/**
 * struct reader - Allows the kernel state changes to be read by user space.
 * @node: The node in the @c readers locked list
 * @changes: The circular buffer of user changes
 * @wait_queue: A wait queue for poll
 * @release: a callback that will be invoked when the reader is terminated
 *
 * The reader is a zero copy memory mapped circular buffer to user space. State
 * changes are pushed into the buffer. The flow from user space is:
 *
 *   * Request file descriptor with KBASE_IOCTL_KINSTR_JM_FD
 *   * On that file descriptor perform a memory mapping. This will allocate the
 *     kernel side circular buffer
 *   * The user will then poll the file descriptor for data
 *   * Upon receiving POLLIN
 *     * Perform a IO control on the file descriptor with
 *       KBASE_KINSTR_JM_READER_VIEW . This will return the index and count of
 *       state changes in the read only buffer.
 *     * Perform a IO control on the file descriptor with
 *       KBASE_KINSTR_JM_READER_ADVANCE  The user will then move the circular
 *       buffer tail forward
 *   * The memory will be unmapped and the file descriptor closed
 *
 * A novel thing about the implementation is that we map twice the virtual
 * memory so that the circular buffer always appears contiguous to the user.
 */
struct reader {
	struct hlist_bl_node node;
	struct reader_changes changes;
	struct wait_queue_head wait_queue;
	struct reader_binding_release release;
};

/**
 * reader_init() - Initialise a instrumentation job manager reader context.
 * @out_ctx: Non-NULL pointer to where the pointer to the created context will
 *           be stored on success.
 *
 * Return: 0 on success, else error code.
 */
static int reader_init(struct reader **const out_ctx)
{
	struct reader *ctx = NULL;

	if (!out_ctx)
		return -EINVAL;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_HLIST_BL_NODE(&ctx->node);
	spin_lock_init(&ctx->changes.producer);
	spin_lock_init(&ctx->changes.consumer);
	init_waitqueue_head(&ctx->wait_queue);

	*out_ctx = ctx;

	return 0;
}

/**
 * reader_term() - Terminate a instrumentation job manager reader context.
 * @ctx: Pointer to context to be terminated.
 */
static void reader_term(struct reader *const ctx)
{
	reader_callback_release callback;

	if (!ctx)
		return;

	/* Barrier prevents race condition when the context is terminated */
	callback = smp_load_acquire(&ctx->release.callback);
	if (callback)
		callback(ctx, ctx->release.user);

	if (smp_load_acquire(&ctx->changes.buffer.data))
		pr_warn(PR_ "The reader was terminated before the circular buffer was unmapped\n");

	kfree(ctx);
}

/**
 * reader_release() - Invoked when the reader file descriptor is released
 * @node: The inode that the file descriptor that the file corresponds to. In
 *        our case our reader file descriptor is backed by and anonymous node so
 *        not much is in this.
 * @file: the file data. Our reader context is held in the private data
 * Return: zero on success
 */
static int reader_release(struct inode *const node, struct file *const file)
{
	struct reader *const reader = file->private_data;

	wake_up_interruptible(&reader->wait_queue);
	reader_term(reader);
	file->private_data = NULL;

	return 0;
}

/**
 * mmap_bytes() - Converts a requested memory mapped region size into the number
 *                of bytes that need to be physically allocated.
 * @size: The requested mapped memory size
 *
 * We allocate half the amount of requested memory so that we can wrap the
 * virtual memory around to the start of the buffer so that the circular buffer
 * is exposed contiguously to user space
 *
 * Return: the number of bytes to allocate with vmalloc_user()
 */
static inline unsigned long mmap_bytes(const unsigned long size)
{
	return size / 2;
}

/**
 * is_power_of_two() - Determines if a number is a power of two
 *
 * @value: the value to check
 * Return: true for 1, 2, 4, 8, etc. Zero is deemed not a power of two
 */
static inline bool is_power_of_two(const unsigned long value)
{
	return (value != 0) && ((value & (value - 1)) == 0);
}

/**
 * is_valid_mmap_size() - Determines if requested memory mapped size is valid.
 * @size: The requested mapped memory size
 *
 * As we have a circular buffer, we use the virtual memory to expose the buffer
 * to user space as contiguous memory. This requires that the size is page
 * aligned. It also means that we need to map double the amount of virtual
 * memory as physical memory.
 *
 * We have an extra constraint that the underlying physical buffer must be a
 * power of two so that we can use the efficient circular buffer helpers that
 * the kernel provides.
 *
 * Return:
 * * true  - the size is valid
 * * false - the size is invalid
 */
static inline bool is_valid_mmap_size(const unsigned long size)
{
	typedef struct reader_changes_buffer buffer_t;
	const size_t elem_size = sizeof(*((buffer_t *)0)->data);
	const size_t size_size = sizeof(((buffer_t *)0)->size);
	const size_t size_max = (1 << (size_size * 8)) - 1;

	return !(size & (PAGE_SIZE - 1)) && /* Page aligned */
	       !((size / PAGE_SIZE) & 1) && /* Double count of pages */
	       is_power_of_two(size / 2) && /* Is a power of two */
	       ((mmap_bytes(size) / elem_size) <= size_max); /* Small enough */
}

/**
 * reader_mmap_open() - Invoked when the reader memory is opened via @c mmap
 * @vma: The virtual memory area of the mapping
 *
 * Used to allocate the necessary state changes buffer that the user requested
 */
static void reader_mmap_open(struct vm_area_struct *const vma)
{
	struct file *const file = vma->vm_file;
	struct reader *const reader = file->private_data;
	struct reader_changes *const changes = &reader->changes;
	struct kbase_kinstr_jm_atom_state_change *data;
	const unsigned long size = vma->vm_end - vma->vm_start;
	const unsigned long bytes = mmap_bytes(size);
	struct reader_changes_buffer *const buffer =
		(struct reader_changes_buffer *)&changes->buffer;

	if (!is_valid_mmap_size(size))
		return;

	if (smp_load_acquire(&buffer->data)) {
		pr_warn(PR_ "circular buffer was already allocated\n");
		return;
	}

	data = vmalloc_user(bytes);
	if (!data)
		return;
	if (WARN_ON(smp_load_acquire(&buffer->data)))
		return;

	spin_lock(&changes->producer);
	spin_lock(&changes->consumer);
	smp_store_release(&buffer->data, data);
	smp_store_release(&buffer->size, bytes / sizeof(*buffer->data));
	changes->head = 0;
	changes->tail = 0;
	changes->overflow = 0;
	spin_unlock(&changes->consumer);
	spin_unlock(&changes->producer);

	static_branch_inc(&basep_kinstr_jm_reader_static_key);
}

/**
 * reader_mmap_close() - Invoked when the reader memory is closed
 * @vma: The virtual memory area of the mapping
 *
 * Releases the allocated state changes memory
 */
static void reader_mmap_close(struct vm_area_struct *const vma)
{
	struct file *const file = vma->vm_file;
	struct reader *const reader = file->private_data;
	struct reader_changes *const changes = &reader->changes;
	struct kbase_kinstr_jm_atom_state_change *data;
	struct reader_changes_buffer *const buffer =
		(struct reader_changes_buffer *)&changes->buffer;

	data = xchg(&buffer->data, NULL);
	if (!data)
		return;

	smp_store_release(&buffer->size, 0);

	vfree(data);

	static_branch_dec(&basep_kinstr_jm_reader_static_key);
}

/**
 * reader_mmap_mremap() - Invoked when the reader attempts to remap the memory
 * @vma: The virtual memory area of the mapping
 * Returns:
 * * -EPERM - we never allow remapping
 */
static int reader_mmap_mremap(struct vm_area_struct *const vma)
{
	return -EPERM;
}

/**
 * reader_mmap_fault() - Invoked when the reader accesses new memory pages
 * @vmf: The virtual memory fault information
 * Returns:
 * * 0 - successful mapping
 * * -EINVAL - a pointer was invalid
 * * -EBADF - the file descriptor did not have an attached reader
 * * VM_FAULT_NOPAGE - there was no memory available for the address
 */
static int reader_mmap_fault(struct vm_fault *const vmf)
{
	struct vm_area_struct *vma;
	struct file *file;
	struct reader *reader;
	struct reader_changes *changes;
	struct kbase_kinstr_jm_atom_state_change *buffer;
	unsigned long offset;
	unsigned long size;

	if (unlikely(!vmf))
		return -EINVAL;

	vma = vmf->vma;
	if (unlikely(!vma))
		return -EINVAL;

	file = vma->vm_file;
	if (unlikely(!file))
		return -EINVAL;

	reader = file->private_data;
	if (unlikely(!reader))
		return -EBADF;
	changes = &reader->changes;

	buffer = smp_load_acquire(&changes->buffer.data);
	size = smp_load_acquire(&changes->buffer.size) * sizeof(*buffer);

	if (unlikely(!buffer))
		return VM_FAULT_NOPAGE;

	offset = (vmf->address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);

	/* We have mapped double the amount of virtual memory so that the higher
	 * pages map back around to the start of the circular buffer. This
	 * allows user space to see a contiguous memory buffer
	 */
	if (offset >= (size * 2))
		return -VM_FAULT_NOPAGE;
	offset %= size;
	offset /= sizeof(*buffer);

	vmf->page = vmalloc_to_page(&buffer[offset]);
	get_page(vmf->page);

	return 0;
}

/* The virtual mapping operations virtual function table */
static const struct vm_operations_struct vm_operations = {
	.open = reader_mmap_open,
	.close = reader_mmap_close,
	.mremap = reader_mmap_mremap,
	.fault = reader_mmap_fault
};

/**
 * reader_mmap() - Performs the mapping of the circular reader buffer
 * @file: The file that the mmap was performed on
 * @vma: The virtual memory area of the mapping requested
 * Return:
 * * 0 - successful mapping
 * * -EINVAL - a pointer was invalid
 * * -EBADF - the file descriptor did not have an attached reader
 * * -EADDRINUSE - the memory was already mapped for the reader file descriptor
 * * -ENXIO - the offset for the page was invalid, we only allow the whole
 *            buffer to be mapped
 * * -EACCES - the virtual memory flags were invalid, especially that writable
 * *           memory was asked for
 * * -ENOMEM - we could not allocate the backing memory for the mapping
 */
static int reader_mmap(struct file *const file,
		       struct vm_area_struct *const vma)
{
	struct reader *reader;
	const struct reader_changes_buffer *buffer;
	struct kbase_kinstr_jm_atom_state_change *data;
	unsigned long size;
	unsigned long offset;
	unsigned short flags;

	if (unlikely(!file || !vma))
		return -EINVAL;

	reader = file->private_data;
	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;
	flags = vma->vm_flags;

	if (unlikely(!reader))
		return -EBADF;

	buffer = &reader->changes.buffer;
	data = smp_load_acquire(&buffer->data);

	if (data) {
		pr_warn(PR_ "cannot have multiple reader mappings\n");
		return -EADDRINUSE;
	}

	if ((flags & VM_WRITE) || (flags & VM_SHARED) ||
	    (!(current->personality & READ_IMPLIES_EXEC) &&
	     (flags & VM_EXEC))) {
		pr_warn(PR_ "can only map private read only memory\n");
		return -EACCES;
	}

	vma->vm_flags &= ~(VM_MAYWRITE | VM_MAYEXEC);

	BUILD_BUG_ON((PAGE_SIZE % sizeof(*data)) != 0);
	if (!is_valid_mmap_size(size)) {
		pr_warn(PR_ "mapped bytes (%lu) must be a power of two multiple of page size, doubled. Try %lu [(PAGE_SIZE << 2) * 2]\n",
			size, ((PAGE_SIZE << 2) * 2));
		return -EINVAL;
	}

	if (offset & ~PAGE_MASK) {
		pr_warn(PR_ "offset not aligned: %lu\n", offset);
		return -ENXIO;
	} else if (offset != 0) {
		pr_warn(PR_ "can only map full buffer\n");
		return -ENXIO;
	}

	vma->vm_ops = &vm_operations;
	reader_mmap_open(vma);

	if (!smp_load_acquire(&buffer->data)) {
		pr_warn(PR_ "failed to allocate %lu bytes\n", mmap_bytes(size));
		reader_mmap_close(vma);
		return -ENOMEM;
	}

	pr_info(PR_ "mapped %lu bytes against %lu physical bytes which allows storage of %u states\n",
		size, mmap_bytes(size), smp_load_acquire(&buffer->size));

	return 0;
}

/**
 * reader_poll() - Handles a poll call on the reader file descriptor
 * @file: The file that the poll was performed on
 * @wait: The poll table
 * Return:
 * * 0 - no data ready
 * * POLLIN - state changes have been buffered
 * * -EBADF - the file descriptor did not have an attached reader
 * * -EINVAL - the IO control arguments were invalid
 */
static __poll_t reader_poll(struct file *const file,
			    struct poll_table_struct *const wait)
{
	struct reader *reader;
	u16 threshold;

	if (unlikely(!file || !wait))
		return -EINVAL;

	reader = file->private_data;
	if (unlikely(!reader))
		return -EBADF;

	threshold = reader_changes_threshold(&reader->changes);

	if (reader_changes_count(&reader->changes) >= threshold)
		return POLLIN;

	poll_wait(file, &reader->wait_queue, wait);

	return reader_changes_count(&reader->changes) >= 1 ? POLLIN : 0;
}

/**
 * reader_ioctl_view() - Handles a viewing IO control on the reader
 * @reader:      the allocated reader that is attached to the file descriptor
 * @buffer:      pointer to the user structure which is expecting the index and
 *               count
 * @buffer_size: the size of the user structure which we check to make sure it
 *               can receive the data
 *
 * This will check if there are changes in the circular buffer and return the
 * starting index and count of changes. The user can then use these to
 * dereference the mapped memory to get the changes with zero copy.
 *
 * Return:
 * * 0 - successfully returned the readable view into the circular buffer
 * * -EINVAL - the IO control arguments were invalid
 * * -EFAULT - failed to copy view data to the user
 */
static long reader_ioctl_view(struct reader *const reader, void __user *buffer,
			      size_t buffer_size)
{
	struct reader_changes *const changes = &reader->changes;
	struct kbase_kinstr_jm_reader_view view = { .index = 0, .count = 0 };
	u16 head, tail, size;

	if (unlikely(!reader || !buffer))
		return -EINVAL;

	if (sizeof(view) != buffer_size)
		return -EINVAL;

	spin_lock(&changes->consumer);

	size = smp_load_acquire(&changes->buffer.size);
	head = smp_load_acquire(&changes->head);
	tail = changes->tail;

	/* We would usually use CIRC_CNT_TO_END here but we can use CIRC_CNT
	 * because we map enough virtual memory so that the top end pages wrap
	 * around to the start of the circular buffer
	 */
	view.count = CIRC_CNT(head, tail, size);
	view.index = tail;

	spin_unlock(&changes->consumer);

	if (copy_to_user(buffer, &view, buffer_size))
		return -EFAULT;

	return 0;
}

/**
 * reader_ioctl_advance() - Handles advancing the tail index of the buffer
 * @reader:  the allocated reader that is attached to the file descriptor
 * @advance: the amount of elements to advance the circular buffer tail forward
 *
 * This is called with the count provided back from the view IO control call.
 *
 * Return:
 * * [1, LONG_MAX] - the number of overflowed state changes
 * * 0 - a successful advancement of the reading view
 * * -EINVAL - the advancement value was larger than the viewable amount
 */
static long reader_ioctl_advance(struct reader *const reader,
				 const size_t advance)
{
	struct reader_changes *changes;
	u16 head, tail, size;
	size_t allowed, overflow;

	if (unlikely(!reader))
		return -EINVAL;

	changes = &reader->changes;

	spin_lock(&changes->consumer);

	size = smp_load_acquire(&changes->buffer.size);
	head = smp_load_acquire(&changes->head);
	tail = changes->tail;
	allowed = CIRC_CNT(head, tail, size);

	if (advance > allowed) {
		spin_unlock(&changes->consumer);
		pr_warn(PR_ "cannot advance %zu elements as there are only %zu unread elements\n",
			advance, allowed);
		return -EINVAL;
	}

	smp_store_release(&changes->tail, (tail + advance) & (size - 1));

	overflow = xchg(&changes->overflow, 0);

	spin_unlock(&changes->consumer);

	if (overflow > LONG_MAX)
		return LONG_MAX;

	return overflow;
}

/**
 * reader_ioctl_layout() - Handles a layout IO control on the reader
 * @reader:      the allocated reader that is attached to the file descriptor
 * @buffer:      pointer to the user structure which is expecting the index and
 *               count
 * @buffer_size: the size of the user structure which we check to make sure it
 *               can receive the data
 *
 * This creates the layout identifier for the internal state changes buffer.
 * When user space has that information it can be proactive against forward and
 * backwards compatibility
 *
 * Return:
 * * 0 - successfully returned the buffer structure layout
 * * -EINVAL - the IO control arguments were invalid
 * * -EFAULT - failed to copy view data to the user
 */
static long reader_ioctl_layout(struct reader *const reader,
				void __user *buffer,
				size_t buffer_size)
{
	const union kbase_kinstr_jm_reader_layout layout =
		KBASE_KINSTR_JM_READER_LAYOUT_INIT(
			struct kbase_kinstr_jm_atom_state_change,
			timestamp, KBASE_KINSTR_JM_READER_LAYOUT_KIND_TIMESTAMP,
			state, KBASE_KINSTR_JM_READER_LAYOUT_KIND_STATE,
			atom, KBASE_KINSTR_JM_READER_LAYOUT_KIND_ATOM);

	BUILD_BUG_ON(
		sizeof(struct kbase_kinstr_jm_atom_state_change) >
		((1 << (sizeof(layout.info.size) * 8)) - 1));

	if (unlikely(!reader || !buffer))
		return -EINVAL;

	if (sizeof(layout) != buffer_size)
		return -EINVAL;

	if (copy_to_user(buffer, &layout, buffer_size))
		return -EFAULT;

	return 0;
}

/**
 * reader_ioctl() - Handles an IOCTL call on the reader file descriptor
 * @file: The file that the poll was performed on
 * @cmd: The IO control command
 * @arg: The IO control argument
 * Return:
 * * -EINVAL - invalid IO control command
 * * -EBADF - the file descriptor did not have an attached reader
 */
static long reader_ioctl(struct file *const file, const unsigned int cmd,
			 const unsigned long arg)
{
	struct reader *reader;
	long ret = -EPERM;

	if (!file || (_IOC_TYPE(cmd) != KBASE_KINSTR_JM_READER))
		return -EINVAL;

	reader = file->private_data;
	if (unlikely(!reader))
		return -EBADF;

	switch (cmd) {
	case KBASE_KINSTR_JM_READER_VIEW:
		ret = reader_ioctl_view(
				reader, (void __user *)(arg), _IOC_SIZE(cmd));
		break;
	case KBASE_KINSTR_JM_READER_ADVANCE:
		if ((sizeof(arg) != sizeof(size_t)) && arg > SIZE_MAX)
			return -EINVAL;
		ret = reader_ioctl_advance(reader, arg);
		break;
	case KBASE_KINSTR_JM_READER_LAYOUT:
		ret = reader_ioctl_layout(
				reader, (void __user *)(arg), _IOC_SIZE(cmd));
		break;
	default:
		pr_warn(PR_ "invalid ioctl (%u)\n", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* The file operations virtual function table */
static const struct file_operations file_operations = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.poll = reader_poll,
	.unlocked_ioctl = reader_ioctl,
	.compat_ioctl = reader_ioctl,
	.mmap = reader_mmap,
	.release = reader_release
};

/**
 * struct kbase_kinstr_jm - The context for the kernel job manager atom tracing
 * @readers: a bitlocked list of opened readers. Readers are attached to the
 *           private data of a file descriptor that the user opens with the
 *           KBASE_IOCTL_KINSTR_JM_FD IO control call.
 *
 * This is opaque outside this compilation unit
 */
struct kbase_kinstr_jm {
	struct hlist_bl_head readers;
};

/**
 * kbasep_kinstr_jm_reader_release() - Invoked when a reader is released
 * @reader: the reader that is being released
 * @user:   the kinstr_jm context
 *
 * We use this callback to remove the reader from the list of readers.
 */
static void kbasep_kinstr_jm_reader_release(struct reader *const reader,
					    void *user)
{
	struct kbase_kinstr_jm *const ctx = user;
	struct hlist_bl_head readers;

	/* If we get the head before the termination of the context happens
	 * we can safely delete the reader in the list
	 */
	readers.first = smp_load_acquire(&ctx->readers.first);
	hlist_bl_lock(&readers);
	hlist_bl_del_rcu(&reader->node);
	hlist_bl_unlock(&readers);
}

int kbase_kinstr_jm_fd(struct kbase_kinstr_jm *const ctx)
{
	struct reader *reader = NULL;
	int fd;

	if (!ctx)
		return -EINVAL;

	fd = reader_init(&reader);
	if (fd)
		goto failure;

	fd = anon_inode_getfd("[mali_kinstr_jm]", &file_operations, reader,
			      O_CLOEXEC);

	if (fd < 0)
		goto failure;

	hlist_bl_lock(&ctx->readers);
	hlist_bl_add_head_rcu(&reader->node, &ctx->readers);
	reader->release.callback = kbasep_kinstr_jm_reader_release;
	reader->release.user = ctx;
	hlist_bl_unlock(&ctx->readers);

	return fd;
failure:
	reader_term(reader);
	return fd;
}

int kbase_kinstr_jm_init(struct kbase_kinstr_jm **const out_ctx)
{
	struct kbase_kinstr_jm *ctx = NULL;

	if (!out_ctx)
		return -EINVAL;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_HLIST_BL_HEAD(&ctx->readers);

	*out_ctx = ctx;

	return 0;
}

void kbase_kinstr_jm_term(struct kbase_kinstr_jm *const ctx)
{
	struct reader *reader;
	struct hlist_bl_node *node;
	struct hlist_bl_head readers;

	if (!ctx)
		return;

	/* There is a race condition here where a file descriptor can be
	 * released when we are killing the callbacks. We synchronise on the
	 * list head pointer.
	 */
	readers.first = xchg(&ctx->readers.first, NULL);
	rcu_read_lock();
	hlist_bl_for_each_entry_rcu(reader, node, &readers, node)
		smp_store_release(&reader->release.callback, NULL);
	rcu_read_unlock();

	kfree(ctx);
}

/**
 * timestamp() - Retrieves the current monotonic nanoseconds
 * Return: monotonic nanoseconds timestamp.
 */
static u64 timestamp(void)
{
	struct timespec ts;
	long ns;

	getrawmonotonic(&ts);
	ns = ((long)(ts.tv_sec) * NSEC_PER_SEC) + ts.tv_nsec;
	if (unlikely(ns < 0))
		return 0;
	return ((u64)(ns));
}

void kbasep_kinstr_jm_atom_state(
	struct kbase_jd_atom *const atom,
	const enum kbase_kinstr_jm_reader_atom_state state)
{
	struct kbase_context *const base = atom->kctx;
	struct kbase_kinstr_jm *const ctx = base->kinstr_jm;
	const u8 id = kbase_jd_atom_id(base, atom);
	struct kbase_kinstr_jm_atom_state_change change = {
		.timestamp = timestamp(), .atom = id, .state = state
	};
	struct reader *reader;
	struct hlist_bl_node *node;

	WARN(KBASE_KINSTR_JM_READER_ATOM_STATE_COUNT < state || 0 > state,
	     PR_ "unsupported atom (%u) state (%i)", id, state);

	rcu_read_lock();
	hlist_bl_for_each_entry_rcu(reader, node, &ctx->readers, node)
		reader_changes_push(
			&reader->changes, &change, &reader->wait_queue);
	rcu_read_unlock();
}

KBASE_EXPORT_TEST_API(kbase_kinstr_jm_atom_state);

void kbasep_kinstr_jm_atom_hw_submit(struct kbase_jd_atom *const atom)
{
	struct kbase_context *const base = atom->kctx;
	struct kbase_device *const dev = base->kbdev;
	const int slot = atom->slot_nr;
	struct kbase_jd_atom *const submitted = kbase_gpu_inspect(dev, slot, 0);
	struct kbase_jd_atom *const queued = kbase_gpu_inspect(dev, slot, 1);

	BUILD_BUG_ON(SLOT_RB_SIZE != 2);

	lockdep_assert_held(&dev->hwaccess_lock);

	if (WARN_ON(slot < 0 || slot >= GPU_MAX_JOB_SLOTS))
		return;
	if (WARN_ON(!submitted && queued))
		return;

	if (submitted == atom)
		kbase_kinstr_jm_atom_state_start(atom);
}

void kbasep_kinstr_jm_atom_hw_release(struct kbase_jd_atom *const atom)
{
	struct kbase_context *const base = atom->kctx;
	struct kbase_device *const dev = base->kbdev;
	const int slot = atom->slot_nr;
	struct kbase_jd_atom *const submitted = kbase_gpu_inspect(dev, slot, 0);
	struct kbase_jd_atom *const queued = kbase_gpu_inspect(dev, slot, 1);

	BUILD_BUG_ON(SLOT_RB_SIZE != 2);

	lockdep_assert_held(&dev->hwaccess_lock);

	if (WARN_ON(slot < 0 || slot >= GPU_MAX_JOB_SLOTS))
		return;
	if (WARN_ON(!submitted && queued))
		return;
	if (WARN_ON(submitted != atom))
		return;

	kbase_kinstr_jm_atom_state_stop(atom);
	if (queued)
		kbase_kinstr_jm_atom_state_start(queued);
}
