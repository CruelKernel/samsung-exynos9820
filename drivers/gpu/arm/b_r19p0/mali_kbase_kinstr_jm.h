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
 * mali_kbase_kinstr_jm.h
 * Kernel driver public interface to job manager atom tracing. This API provides
 * a method to get the atom state changes into user space. For efficiency, it is
 * implemented as a zero copy read-only memory mapped circular buffer from the
 * kernel to userspace.
 *
 * The flow of operation is:
 *
 * | kernel                              | user                                |
 * | ----------------------------------- | ----------------------------------- |
 * | Initialize API with                 |                                     |
 * | kbase_kinstr_jm_init()              |                                     |
 * |                                     |                                     |
 * | Kernel code injects states with     |                                     |
 * | kbase_kinstr_jm_atom_state_*() APIs |                                     |
 * |                                     | Call ioctl() to get file descriptor |
 * |                                     | via KBASE_IOCTL_KINSTR_JM_FD        |
 * | Allocates a reader attached to FD   |                                     |
 * |                                     | Map memory with mmap() on the FD    |
 * | Allocates circular buffer and       |                                     |
 * | patches, via ASM goto, the          |                                     |
 * | kbase_kinstr_jm_atom_state_*()      |                                     |
 * |                                     | loop:                               |
 * |                                     |   Call poll() on FD for POLLIN      |
 * |   When threshold of changes is hit, |                                     |
 * |   the poll is interrupted with      |                                     |
 * |   POLLIN. If circular buffer is     |                                     |
 * |   full then store the missed count  |                                     |
 * |   and interrupt poll                |   Call ioctl() to get view into     |
 * |                                     |   circular buffer via               |
 * |                                     |   KBASE_KINSTR_JM_READER_VIEW       |
 * |                                     |                                     |
 * |                                     |   Call ioctl() to advance the       |
 * |                                     |   circular buffer tail via          |
 * |                                     |   KBASE_KINSTR_JM_READER_VIEW       |
 * |   Kernel advances tail of circular  |                                     |
 * |   buffer                            |                                     |
 * |                                     | Unmap memory with munmap            |
 * | Deallocates circular buffer         |                                     |
 * |                                     | Close file descriptor               |
 * | Terminate API with                  |                                     |
 * | kbase_kinstr_jm_term()              |                                     |
 *
 * The circular buffer is mapped into user space with wrapped pages so that the
 * buffer always seems contiguous:
 *
 *   User Read                         |------------|
 *   Virtual   |-----------------------rrrrrrrrrrrrrr---------------------|
 *              'o.                    'r..x'o.   .r'                   .x'
 *                 'o.                 .x''r.  :r:                 .x'
 *                    'o.           .x'      :r:  'o.           .x'
 *                       'o.     .x'      .r'   'r.  'o.     .x'
 *                          'o.x'      .r'         'r.  'o.x'
 *   Physical                 |rrrrrrrrr-------------rrrrr|
 *
 * All tracepoints are guarded on a static key. The static key is activated when
 * a user space reader gets created. This means that there is negligible cost
 * inserting the tracepoints into code when there are no readers.
 */

#ifndef _KBASE_KINSTR_JM_H_
#define _KBASE_KINSTR_JM_H_

#include <linux/static_key.h>

#include "mali_kbase_kinstr_jm_reader.h"

/* Forward declarations */
struct kbase_kinstr_jm;
struct kbase_jd_atom;

/**
 * kbase_kinstr_jm_init() - Initialise an instrumentation job manager context.
 * @out_ctx: Non-NULL pointer to where the pointer to the created context will
 *           be stored on success.
 *
 * Return: 0 on success, else error code.
 */
int kbase_kinstr_jm_init(struct kbase_kinstr_jm **const out_ctx);

/**
 * kbase_kinstr_jm_term() - Terminate an instrumentation job manager context.
 * @ctx: Pointer to context to be terminated.
 */
void kbase_kinstr_jm_term(struct kbase_kinstr_jm *const ctx);

/**
 * kbase_kinstr_jm_fd() - Retrieves a file descriptor that can be used to read
 *                        the atom state changes from userspace
 *
 * @ctx: Pointer to the initialized context
 * Return: -1 on failure, valid file descriptor on success
 */
int kbase_kinstr_jm_fd(struct kbase_kinstr_jm *const ctx);

/**
 * kbase_kinstr_jm_atom_state_() - Signifies that an atom has changed state
 * @atom: The atom that has changed state
 * @state: The new state of the atom
 *
 * This performs the actual storage of the state ready for user space to
 * read the data. It is only called when the static key is enabled from
 * kbase_kinstr_jm_atom_state(). There is almost never a need to invoke this
 * function directly.
 */
void kbasep_kinstr_jm_atom_state(
	struct kbase_jd_atom *const atom,
	const enum kbase_kinstr_jm_reader_atom_state state);

/* Allows ASM goto patching to reduce tracing overhead. This is
 * incremented/decremented when readers are created and terminated. This really
 * shouldn't be changed externally, but if you do, make sure you use
 * a static_key_inc()/static_key_dec() pair.
 */
extern struct static_key_false basep_kinstr_jm_reader_static_key;

/**
 * kbase_kinstr_jm_atom_state() - Signifies that an atom has changed state
 * @atom: The atom that has changed state
 * @state: The new state of the atom
 *
 * This uses a static key to reduce overhead when tracing is disabled
 */
static inline void kbase_kinstr_jm_atom_state(
	struct kbase_jd_atom *const atom,
	const enum kbase_kinstr_jm_reader_atom_state state)
{
	if (static_branch_unlikely(&basep_kinstr_jm_reader_static_key))
		kbasep_kinstr_jm_atom_state(atom, state);
}

/**
 * kbase_kinstr_jm_atom_state_queue() - Signifies that an atom has entered a
 *                                      hardware or software queue.
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_state_queue(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state(
		atom, KBASE_KINSTR_JM_READER_ATOM_STATE_QUEUE);
}

/**
 * kbase_kinstr_jm_atom_state_start() - Signifies that work has started on an
 *                                      atom
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_state_start(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state(
		atom, KBASE_KINSTR_JM_READER_ATOM_STATE_START);
}

/**
 * kbase_kinstr_jm_atom_state_stop() - Signifies that work has stopped on an
 *                                     atom
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_state_stop(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state(
		atom, KBASE_KINSTR_JM_READER_ATOM_STATE_STOP);
}

/**
 * kbase_kinstr_jm_atom_state_complete() - Signifies that all work has completed
 *                                         on an atom
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_state_complete(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state(
		atom, KBASE_KINSTR_JM_READER_ATOM_STATE_COMPLETE);
}

/**
 * kbase_kinstr_jm_atom_queue() - A software *or* hardware atom is queued for
 *                                execution
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_queue(struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state_queue(atom);
}

/**
 * kbase_kinstr_jm_atom_complete() - A software *or* hardware atom is fully
 *                                   completed
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_complete(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state_complete(atom);
}

/**
 * kbase_kinstr_jm_atom_sw_start() - A software atom has started work
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_sw_start(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state_start(atom);
}

/**
 * kbase_kinstr_jm_atom_sw_stop() - A software atom has stopped work
 * @atom: The atom that has changed state
 */
static inline void kbase_kinstr_jm_atom_sw_stop(
	struct kbase_jd_atom *const atom)
{
	kbase_kinstr_jm_atom_state_stop(atom);
}

/**
 * kbasep_kinstr_jm_atom_hw_submit() - A hardware atom has been submitted
 * @atom: The atom that has been submitted
 *
 * This private implementation should not be called directly, it is protected
 * by a static key in kbase_kinstr_jm_atom_hw_submit(). Use that instead.
 */
void kbasep_kinstr_jm_atom_hw_submit(struct kbase_jd_atom *const atom);

/**
 * kbase_kinstr_jm_atom_hw_submit() - A hardware atom has been submitted
 * @atom: The atom that has been submitted
 */
static inline void kbase_kinstr_jm_atom_hw_submit(
	struct kbase_jd_atom *const atom)
{
	if (static_branch_unlikely(&basep_kinstr_jm_reader_static_key))
		kbasep_kinstr_jm_atom_hw_submit(atom);
}

/**
 * kbasep_kinstr_jm_atom_hw_submit() - A hardware atom has been released
 * @atom: The atom that has been submitted
 *
 * This private implementation should not be called directly, it is protected
 * by a static key in kbase_kinstr_jm_atom_hw_release(). Use that instead.
 */
void kbasep_kinstr_jm_atom_hw_release(struct kbase_jd_atom *const atom);

/**
 * kbase_kinstr_jm_atom_hw_submit() - A hardware atom has been released
 * @atom: The atom that has been submitted
 */
static inline void kbase_kinstr_jm_atom_hw_release(
	struct kbase_jd_atom *const atom)
{
	if (static_branch_unlikely(&basep_kinstr_jm_reader_static_key))
		kbasep_kinstr_jm_atom_hw_release(atom);
}

#endif /* _KBASE_KINSTR_JM_H_ */
