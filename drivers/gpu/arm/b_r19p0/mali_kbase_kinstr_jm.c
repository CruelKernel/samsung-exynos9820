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
#include <linux/personality.h>
#include <linux/rculist_bl.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#if KERNEL_VERSION(4, 16, 0) >= LINUX_VERSION_CODE
typedef unsigned int __poll_t;
#endif

#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUPP
#endif

/* The module printing prefix */
#define PR_ "mali_kbase_kinstr_jm: "

/* Allows us to perform ASM goto for the tracing
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
 *        prevent races on the allocated memory. The consume lock does not need
 *        to be held because even though the view indexes are changed the
 *        access to the buffer is synchronised through the memory mapping fault
 *        handler.
 * @size: The number of changes that are allowed in @c data. Can be used with
 *        Linux @c CIRC_ helpers. Will always be a power of two. The producer
 *        lock should be held when updating this and stored with an SMP release
 *        memory barrier. This means that the consumer can do an SMP load.
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
 *            buffer at the same time as a user view occurring. This needs to
 *            be locked when saving/restoring the IRQ because we can receive an
 *            interrupt from the GPU when an atom completes. The CPU could have
 *            a task preempted that is holding this lock.
 * @consumer: The consuming spinlock which locks around the user view/advance.
 *            Must be held when updating the views of the circular buffer.
 * @overflow: Determines how many state changes overflowed the full buffer.
 *            updated by the producer with an atomic increment. The consumer
 *            should do an atomic exchange with 0 to read and reset the
 *            overflow.
 * @head:     The head of the circular buffer. Can be used with Linux @c CIRC_
 *            helpers. The producer should lock and update this with an SMP
 *            store when a new change lands. The consumer can read with an
 *            SMP load. This allows the producer to safely insert new changes
 *            into the circular buffer.
 * @tail:     The tail of the circular buffer. Can be used with Linux @c CIRC_
 *            helpers. The producer should do a READ_ONCE load and the consumer
 *            should SMP store. This allows the consumer to safely advance the
 *            view.
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
	atomic_t overflow;
	u16 head;
	u16 tail;
};

/**
 * reader_changes_bytes() - Converts a requested changes buffer size into the
 *                          number of bytes that need to be physically
 *                          allocated.
 * @size: The requested changes buffer size
 *
 * We allocate half the amount of requested memory so that we can wrap the
 * virtual memory around to the start of the buffer so that the circular buffer
 * is exposed contiguously to user space
 *
 * Return: the number of bytes to allocate with vmalloc_user()
 */
static inline size_t reader_changes_bytes(const size_t size)
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
 * reader_changes_is_valid_size() - Determines if requested changes buffer size
 *                                  is valid.
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
static inline bool reader_changes_is_valid_size(const size_t size)
{
	typedef struct reader_changes_buffer buffer_t;
	const size_t elem_size = sizeof(*((buffer_t *)0)->data);
	const size_t size_size = sizeof(((buffer_t *)0)->size);
	const size_t size_max = (1 << (size_size * 8)) - 1;
	const size_t bytes = reader_changes_bytes(size);

	return !(size & (PAGE_SIZE - 1)) && /* Page aligned */
	       !((size / PAGE_SIZE) & 1) && /* Double count of pages */
	       is_power_of_two(size / 2) && /* Is a power of two */
	       ((bytes / elem_size) <= size_max); /* Small enough */
}

/**
 * reader_changes_init() - Initializes the reader changes and allocates the
 *                         changes buffer
 * @out_ctx: The context pointer, must point to an allocated reader changes
 *           structure. We may support allocating the structure in the future.
 * @size: The requested changes buffer size
 *
 * We allocate half the amount of requested memory so that we can wrap the
 * virtual memory around to the start of the buffer so that the circular buffer
 * is exposed contiguously to user space
 *
 * * (0, U16_MAX] - the number of data elements allocated
 * * -EINVAL - a pointer was invalid
 * * -ENOTSUP - we do not support allocation of the context
 * * -ERANGE - the requested memory size was invalid
 * * -ENOMEM - could not allocate the memory
 * * -EADDRINUSE - the buffer memory was already allocated
 */
static int reader_changes_init(struct reader_changes *const *const out_ctx,
			       const size_t size)
{
	struct kbase_kinstr_jm_atom_state_change *data = NULL;
	const size_t bytes = reader_changes_bytes(size);
	int status = 0;
	unsigned long irq;
	struct reader_changes *ctx;
	struct reader_changes_buffer *buffer;

	BUILD_BUG_ON((PAGE_SIZE % sizeof(*buffer->data)) != 0);

	if (unlikely(!out_ctx)) {
		status = -EINVAL;
		goto exit;
	}
	ctx = *out_ctx;

	if (unlikely(!ctx)) {
		status = -ENOTSUP;
		goto exit;
	}
	buffer = (struct reader_changes_buffer *)&ctx->buffer;

	if (!reader_changes_is_valid_size(size)) {
		status = -ERANGE;
		goto exit;
	}

	data = vmalloc_user(bytes);
	if (!data) {
		status = -ENOMEM;
		goto exit;
	}

	spin_lock_irqsave(&ctx->producer, irq);
	if (buffer->data) {
		status = -EADDRINUSE;
		goto unlock;
	}
	swap(buffer->data, data);
	status = bytes / sizeof(*buffer->data);
	smp_store_release(&buffer->size, status);
	smp_store_release(&ctx->head, 0);
	smp_store_release(&ctx->tail, 0);
	atomic_set(&ctx->overflow, 0);
unlock:
	spin_unlock_irqrestore(&ctx->producer, irq);
exit:
	if (unlikely(data))
		vfree(data);
	return status;
}

/**
 * reader_changes_term() - Cleans up a reader changes structure
 * @ctx: The context to clean up
 *
 * Releases the allocated state changes memory
 */
static void reader_changes_term(struct reader_changes *const ctx)
{
	struct reader_changes_buffer *buffer;
	struct kbase_kinstr_jm_atom_state_change *data = NULL;
	unsigned long irq;

	if (!ctx)
		return;
	buffer = (struct reader_changes_buffer *)&ctx->buffer;

	spin_lock_irqsave(&ctx->producer, irq);
	swap(buffer->data, data);
	smp_store_release(&buffer->size, 0);
	smp_store_release(&ctx->head, 0);
	smp_store_release(&ctx->tail, 0);
	atomic_set(&ctx->overflow, 0);
	spin_unlock_irqrestore(&ctx->producer, irq);

	if (likely(data))
		vfree(data);
}

/**
 * reader_changes_threshold() - Determines the threshold of state changes that,
 *                              when reached, we should wake polls on the file
 *                              descriptor
 * @changes: The state changes context
 * Return: the threshold
 */
static inline u16 reader_changes_threshold(
		struct reader_changes *const changes) {
	const struct reader_changes_buffer *buffer = &changes->buffer;

	return min(((size_t)(smp_load_acquire(&buffer->size))) / 4,
		   ((size_t)(PAGE_SIZE)) / sizeof(*buffer->data));
}

/**
 * reader_changes_count() - Retrieves the count of state changes in the buffer
 * @changes: The state changes context
 *
 * The consumer spinlock must be held. Uses the CIRC_CNT macro to determine
 * the count.
 *
 * Return: the number of changes in the circular buffer
 */
static u16 reader_changes_count(struct reader_changes *const changes)
{
	u16 head, tail, size, count;

	lockdep_assert_held(&changes->consumer);

	size = smp_load_acquire(&changes->buffer.size);
	head = smp_load_acquire(&changes->head);
	tail = changes->tail;

	/* We would usually use CIRC_CNT_TO_END here but we can use CIRC_CNT
	 * because we map enough virtual memory so that the top end pages wrap
	 * around to the start of the circular buffer
	 */
	count = CIRC_CNT(head, tail, size);

	return count;
}

/**
 * reader_changes_push() - Pushes a change into the reader circular buffer.
 * @changes:    The change to insert the change into
 * @change:     Kernel atom change to insert
 * @wait_queue: The queue to be kicked when changes should be read from
 *              userspace. Kicked when a threshold is reached or there is
 *              overflow.
 */
static void reader_changes_push(
	struct reader_changes *const changes,
	const struct kbase_kinstr_jm_atom_state_change *const change,
	struct wait_queue_head *const wait_queue)
{
	u16 head, tail, size, threshold;
	unsigned long irq;
	struct kbase_kinstr_jm_atom_state_change *data;

	spin_lock_irqsave(&changes->producer, irq);

	data = changes->buffer.data;

	if (unlikely(!data))
		goto unlock;

	head = changes->head;
	tail = READ_ONCE(changes->tail);
	size = changes->buffer.size;

	if (CIRC_SPACE(head, tail, size) >= 1) {
		data[head] = *change;

		smp_store_release(&changes->head, (head + 1) & (size - 1));

		threshold = reader_changes_threshold(changes);

		if (CIRC_CNT(head + 1, tail, size) >= threshold)
			wake_up_interruptible(wait_queue);
	} else {
		if (1 == atomic_add_unless(&changes->overflow, 1, -1))
			pr_warn(PR_ "overflow of circular buffer\n");
		wake_up_interruptible(wait_queue);
	}
unlock:
	spin_unlock_irqrestore(&changes->producer, irq);
}

/**
 * struct reader - Allows the kernel state changes to be read by user space.
 * @node: The node in the @c readers locked list
 * @changes: The circular buffer of user changes
 * @wait_queue: A wait queue for poll
 * @context: a pointer to the parent context that created this reader. Can be
 *           used to remove the reader from the list of readers. Reference
 *           counted.
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
	struct kbase_kinstr_jm *context;
};

static struct kbase_kinstr_jm *
kbase_kinstr_jm_ref_get(struct kbase_kinstr_jm *const ctx);
static void kbase_kinstr_jm_ref_put(struct kbase_kinstr_jm *const ctx);
static void kbase_kinstr_jm_readers_add(struct kbase_kinstr_jm *const ctx,
					struct reader *const reader);
static void kbase_kinstr_jm_readers_del(struct kbase_kinstr_jm *const ctx,
					struct reader *const reader);

/**
 * reader_init() - Initialise a instrumentation job manager reader context.
 * @out_ctx: Non-NULL pointer to where the pointer to the created context will
 *           be stored on success.
 * @context: the pointer to the parent context. Reference count will be
 *           increased
 *
 * Return: 0 on success, else error code.
 */
static int reader_init(struct reader **const out_ctx,
		       struct kbase_kinstr_jm *const context)
{
	struct reader *ctx = NULL;

	if (!out_ctx || !context)
		return -EINVAL;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_HLIST_BL_NODE(&ctx->node);
	spin_lock_init(&ctx->changes.producer);
	spin_lock_init(&ctx->changes.consumer);
	init_waitqueue_head(&ctx->wait_queue);

	/* Reader changes are initialized when memory is mapped */

	ctx->context = kbase_kinstr_jm_ref_get(context);
	kbase_kinstr_jm_readers_add(context, ctx);

	*out_ctx = ctx;

	return 0;
}

/**
 * reader_term() - Terminate a instrumentation job manager reader context.
 * @ctx: Pointer to context to be terminated.
 */
static void reader_term(struct reader *const ctx)
{
	if (!ctx)
		return;

	reader_changes_term(&ctx->changes);

	if (ctx->context) {
		kbase_kinstr_jm_readers_del(ctx->context, ctx);
		kbase_kinstr_jm_ref_put(ctx->context);
	}

	kfree(ctx);
}

/**
 * reader_release() - Invoked when the reader file descriptor is released
 * @node: The inode that the file descriptor that the file corresponds to. In
 *        our case our reader file descriptor is backed by an anonymous node so
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
	const unsigned long size = vma->vm_end - vma->vm_start;

	const int status = reader_changes_init(&changes, size);

	if (status < 0)
		pr_warn(PR_ "failed to initialize reader changes: %i\n",
			status);
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

	reader_changes_term(changes);
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
	struct kbase_kinstr_jm_atom_state_change *buffer;
	struct reader_changes *changes;
	unsigned long offset, size, irq;

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
	spin_lock_irqsave(&changes->producer, irq);
	buffer = changes->buffer.data;
	size = changes->buffer.size * sizeof(*buffer);
	spin_unlock_irqrestore(&changes->producer, irq);

	if (unlikely(!buffer))
		return VM_FAULT_NOPAGE;

	offset = (vmf->address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);

	/* We have mapped double the amount of virtual memory so that the higher
	 * pages map back around to the start of the circular buffer. This
	 * allows user space to see a contiguous memory buffer
	 */
	if (offset >= (size * 2))
		return VM_FAULT_NOPAGE;
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
 * * -ERANGE - the requested memory size was invalid
 * * -EOVERFLOW - the buffer allocated was too large
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
	struct reader_changes *changes;
	unsigned long size, offset;
	unsigned short flags;
	int status = 0;
	u16 count;

	if (unlikely(!file || !vma)) {
		status = -EINVAL;
		goto exit;
	}

	reader = file->private_data;
	if (unlikely(!reader)) {
		status = -EBADF;
		goto exit;
	}
	changes = &reader->changes;

	flags = vma->vm_flags;
	if ((flags & VM_WRITE) || (flags & VM_SHARED) ||
	    (!(current->personality & READ_IMPLIES_EXEC) &&
	     (flags & VM_EXEC))) {
		pr_warn(PR_ "can only map private read only memory\n");
		status = -EACCES;
		goto exit;
	}
	vma->vm_flags &= ~(VM_MAYWRITE | VM_MAYEXEC);

	offset = vma->vm_pgoff << PAGE_SHIFT;
	if (offset & ~PAGE_MASK) {
		pr_warn(PR_ "offset not aligned: %lu\n", offset);
		status = -ENXIO;
		goto exit;
	} else if (offset != 0) {
		pr_warn(PR_ "can only map full buffer\n");
		status = -ENXIO;
		goto exit;
	}

	vma->vm_ops = &vm_operations;

	size = vma->vm_end - vma->vm_start;
	status = reader_changes_init(&changes, size);
	if (status > U16_MAX) {
		status = -EOVERFLOW;
		goto term;
	} else if (status == -ERANGE) {
		pr_warn(PR_ "failed to allocate %lu virtual bytes\n", size);
		goto exit;
	} else if (status == -ENOMEM) {
		pr_warn(PR_ "mapped bytes (%lu) must be a power of two multiple of page size, doubled. Try %lu [(PAGE_SIZE << 2) * 2]\n",
			size, ((PAGE_SIZE << 2) * 2));
		goto exit;
	} else if (status == -EADDRINUSE) {
		pr_warn(PR_ "cannot have multiple reader mappings\n");
		goto exit;
	} else if (status < 0) {
		pr_warn(PR_ "failed to initialize reader changes: %i\n",
			status);
		goto exit;
	}

	count = (u16)status;
	pr_info(PR_ "mapped %zu virtual bytes against %zu physical bytes which allows storage of %u states\n",
		2 * count * sizeof(*changes->buffer.data),
		count * sizeof(*changes->buffer.data),
		count);
	return 0;
term:
	reader_changes_term(changes);
exit:
	return status;
}

/**
 * reader_poll() - Handles a poll call on the reader file descriptor
 * @file: The file that the poll was performed on
 * @wait: The poll table
 *
 * The results of the poll will be unreliable if there is no mapped memory as
 * there is no circular buffer to push atom state changes into.
 *
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
	struct reader_changes *changes;
	u16 threshold, count;

	if (unlikely(!file || !wait))
		return -EINVAL;

	reader = file->private_data;
	if (unlikely(!reader))
		return -EBADF;
	changes = &reader->changes;

	spin_lock(&changes->consumer);
	threshold = reader_changes_threshold(changes);
	count = reader_changes_count(&reader->changes);
	spin_unlock(&changes->consumer);

	if (count >= threshold)
		return POLLIN;

	poll_wait(file, &reader->wait_queue, wait);

	spin_lock(&changes->consumer);
	count = reader_changes_count(&reader->changes);
	spin_unlock(&changes->consumer);

	return count >= 1 ? POLLIN : 0;
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
 * This function call is only valid whilst the user has mapped the reader
 * memory. If the memory has not been mapped the indexes are non-sensical
 * because they cannot be used to index into a valid kernel read-only buffer.
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

	if (unlikely(!reader || !buffer))
		return -EINVAL;

	if (sizeof(view) != buffer_size)
		return -EINVAL;

	spin_lock(&changes->consumer);
	view.count = reader_changes_count(changes);
	view.index = changes->tail;
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
 * This function call is only valid whilst the user has mapped the reader
 * memory. If the memory has not been mapped, advancing the index will not
 * perform an useful operation because the advancement will not relate to any
 * valid kernel read-only memory.
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
	size_t allowed, overflow;
	u16 size;

	if (unlikely(!reader))
		return -EINVAL;

	changes = &reader->changes;

	spin_lock(&changes->consumer);

	allowed = reader_changes_count(changes);
	if (advance > allowed) {
		spin_unlock(&changes->consumer);
		pr_warn(PR_ "cannot advance %zu elements as there are only %zu unread elements\n",
			advance, allowed);
		return -EINVAL;
	}

	size = smp_load_acquire(&changes->buffer.size);
	smp_store_release(&changes->tail,
			  (changes->tail + advance) & (size - 1));

	/* While atomic_t, atomic_long_t and atomic64_t use int, long and s64
	 * respectively (for hysterical raisins), the kernel uses
	 *  -fno-strict-overflow (which implies -fwrapv) and defines signed
	 * overflow to behave like 2s-complement.
	 *
	 * Therefore, an explicitly unsigned variant of the atomic ops is
	 * strictly unnecessary and we can simply cast, there is no UB.
	 */
	overflow = (size_t)(atomic_xchg(&changes->overflow, 0));

	spin_unlock(&changes->consumer);

	if (unlikely(overflow > LONG_MAX))
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
 * @refcount: reference count for the context. Any reader will have a link
 *            back to the context so that they can remove themselves from the
 *            list.
 *
 * This is opaque outside this compilation unit
 */
struct kbase_kinstr_jm {
	struct hlist_bl_head readers;
	struct kref refcount;
};

/**
 * kbasep_kinstr_jm_release() - Invoked when the reference count is dropped
 * @ref: the context reference count
 */
static void kbase_kinstr_jm_release(struct kref *const ref)
{
	struct kbase_kinstr_jm *const ctx =
		container_of(ref, struct kbase_kinstr_jm, refcount);
	kfree(ctx);
}

/**
 * kbase_kinstr_jm_ref_get() - Reference counts the instrumentation context
 * @ctx: the context to reference count
 * Return: the reference counted context
 */
static struct kbase_kinstr_jm *
kbase_kinstr_jm_ref_get(struct kbase_kinstr_jm *const ctx)
{
	if (likely(ctx))
		kref_get(&ctx->refcount);
	return ctx;
}

/**
 * kbase_kinstr_jm_ref_put() - Dereferences the instrumentation context
 * @ctx: the context to lower the reference count on
 */
static void kbase_kinstr_jm_ref_put(struct kbase_kinstr_jm *const ctx)
{
	if (likely(ctx))
		kref_put(&ctx->refcount, kbase_kinstr_jm_release);
}

/**
 * kbase_kinstr_jm_readers_add() - Adds a reader to the list of readers
 * @ctx: the instrumentation context
 * @reader: the reader to add
 */
static void kbase_kinstr_jm_readers_add(struct kbase_kinstr_jm *const ctx,
					struct reader *const reader)
{
	struct hlist_bl_head *const readers = &ctx->readers;

	hlist_bl_lock(readers);
	hlist_bl_add_head_rcu(&reader->node, readers);
	hlist_bl_unlock(readers);

	static_branch_inc(&basep_kinstr_jm_reader_static_key);
}

/**
 * readers_del() - Deletes a reader from the list of readers
 * @ctx: the instrumentation context
 * @reader: the reader to delete
 */
static void kbase_kinstr_jm_readers_del(struct kbase_kinstr_jm *const ctx,
					struct reader *const reader)
{
	struct hlist_bl_head *const readers = &ctx->readers;

	hlist_bl_lock(readers);
	hlist_bl_del_rcu(&reader->node);
	hlist_bl_unlock(readers);

	static_branch_dec(&basep_kinstr_jm_reader_static_key);
}

int kbase_kinstr_jm_fd(struct kbase_kinstr_jm *const ctx)
{
	struct reader *reader = NULL;
	int status;
	int fd;

	if (!ctx)
		return -EINVAL;

	status = reader_init(&reader, ctx);
	if (status < 0)
		return status;

	fd = anon_inode_getfd("[mali_kinstr_jm]", &file_operations, reader,
			      O_CLOEXEC);

	if (fd < 0) {
		reader_term(reader);
		return fd;
	}

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
	kref_init(&ctx->refcount);

	*out_ctx = ctx;

	return 0;
}

void kbase_kinstr_jm_term(struct kbase_kinstr_jm *const ctx)
{
	kbase_kinstr_jm_ref_put(ctx);
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
