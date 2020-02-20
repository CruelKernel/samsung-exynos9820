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
 * mali_kbase_kinstr_jm_reader.h
 * Provides an ioctl API to read kernel atom state changes. The flow of the
 * API is:
 *    1. Obtain the file descriptor with ``KBASE_IOCTL_KINSTR_JM_FD``
 *    2. Retrieve the buffer structure layout with
 *       ``KBASE_KINSTR_JM_READER_LAYOUT``
 *    3. Memory map the file descriptor to get a read only view of the kernel
 *       state changes
 *    4. Poll the file descriptor for ``POLLIN``
 *    5. Get a view into the buffer with ``KBASE_KINSTR_JM_READER_VIEW``
 *    6. Use the structure layout to understand how to read the data from the
 *       buffer
 *    7. Advance the view with ``KBASE_KINSTR_JM_READER_ADVANCE``
 *    8. Repeat 4-7
 *    9. Unmap the buffer
 *   10. Close the file descriptor
 */

#ifndef _KBASE_KINSTR_JM_READER_H_
#define _KBASE_KINSTR_JM_READER_H_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/sort.h>
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#endif

/* The ids of ioctl commands. */
#define KBASE_KINSTR_JM_READER 0xCD
#define KBASE_KINSTR_JM_READER_VIEW \
	_IOR(KBASE_KINSTR_JM_READER, 0x00, \
	struct kbase_kinstr_jm_reader_view)
#define KBASE_KINSTR_JM_READER_ADVANCE \
	_IOW(KBASE_KINSTR_JM_READER, 0x01, size_t)
#define KBASE_KINSTR_JM_READER_LAYOUT \
	_IOR(KBASE_KINSTR_JM_READER, 0x02, \
	union kbase_kinstr_jm_reader_layout)

#ifndef KBASE_KINSTR_JM_READER_SORT
#ifdef __KERNEL__
#define KBASE_KINSTR_JM_READER_SORT(p, c, s, f) sort(p, c, s, f, NULL)
#else
#define KBASE_KINSTR_JM_READER_SORT(p, c, s, f) qsort(p, c, s, f)
#endif
#endif

/**
 * struct kbase_kinstr_jm_reader_view - Provides the view into the kernel job
 *                                      manager instrumentation atom state
 *                                      changes memory mapped buffer.
 * @index: the index into the memory mapped region to read the kernel state
 *         changes from
 * @count: the number of kernel state changes available
 */
struct kbase_kinstr_jm_reader_view {
	size_t index;
	size_t count;
};

/**
 * enum kbase_kinstr_jm_reader_atom_state - Determines the work state of an atom
 * @KBASE_KINSTR_JM_READER_ATOM_STATE_QUEUE:    Signifies that an atom has
 *                                              entered a hardware queue
 * @KBASE_KINSTR_JM_READER_ATOM_STATE_START:    Signifies that work has started
 *                                              on an atom
 * @KBASE_KINSTR_JM_READER_ATOM_STATE_STOP:     Signifies that work has stopped
 *                                              on an atom
 * @KBASE_KINSTR_JM_READER_ATOM_STATE_COMPLETE: Signifies that work has
 *                                              completed on an atom
 * @KBASE_KINSTR_JM_READER_ATOM_STATE_COUNT:    The number of state enumerations
 *
 * We can add new states to the end of this if they do not break the existing
 * state machine. Old user mode code can gracefully ignore states they do not
 * understand.
 *
 * If we need to make a breaking change to the state machine, we can do that by
 * changing the value of KBASE_KINSTR_JM_READER_LAYOUT_KIND_STATE. This will
 * mean that old user mode code will fail to understand the new state field in
 * the structure and gracefully not use the state change API.
 */
enum kbase_kinstr_jm_reader_atom_state {
	KBASE_KINSTR_JM_READER_ATOM_STATE_QUEUE,
	KBASE_KINSTR_JM_READER_ATOM_STATE_START,
	KBASE_KINSTR_JM_READER_ATOM_STATE_STOP,
	KBASE_KINSTR_JM_READER_ATOM_STATE_COMPLETE,
	KBASE_KINSTR_JM_READER_ATOM_STATE_COUNT
};

/**
 * enum kbase_kinstr_jm_reader_layout_kind - Determines the kind of member field
 * @KBASE_KINSTR_JM_READER_LAYOUT_KIND_TIMESTAMP: the timestamp is a uint64_t
 *                                                monotonic nanosecond
 *                                                duration from an unspecified
 *                                                point in time
 * @KBASE_KINSTR_JM_READER_LAYOUT_KIND_STATE:     is one of the reader atom
 *                                                state enumerations so will be
 *                                                represented as an signed
 *                                                integer
 * @KBASE_KINSTR_JM_READER_LAYOUT_KIND_ATOM:      an uint8_t identifier of the
 *                                                atom number
 *
 * These are used to describe the structure fields in the memory mapped buffer
 * that is provided to user space. In the first version the structure can be
 * represented as
 *
 *	struct kbase_kinstr_jm_reader_atom_state_change {
 *		u64 timestamp;
 *		enum kbase_kinstr_jm_reader_atom_state state;
 *		u8 atom;
 *	};
 *
 * However, in the future we may add new fields or change the shape of the
 * buffer. This enumeration will describe the fields in the buffer structure.
 * User space code should read the layout of the structure and determine a
 * correct way to read the data from the buffer.
 *
 * When new fields are added, a new kind enumeration will be added. This
 * represents a new member in the structure. If an old member is deprecated it
 * will no longer show up in the layout and old user code will have to
 * gracefully abort performing a read with the API. Every effort will be made to
 * keep fields relevant for as long as possible.
 */
enum kbase_kinstr_jm_reader_layout_kind {
	KBASE_KINSTR_JM_READER_LAYOUT_KIND_TIMESTAMP = 1,
	KBASE_KINSTR_JM_READER_LAYOUT_KIND_STATE = 2,
	KBASE_KINSTR_JM_READER_LAYOUT_KIND_ATOM = 3
};

/**
 * kbase_kinstr_jm_reader_layout_kind_size() - Determines byte size of a kind
 * @kind: the kind to get the size for
 * Returns: the number of bytes the kind takes up in memory
 */
static inline size_t kbase_kinstr_jm_reader_layout_kind_size(
	enum kbase_kinstr_jm_reader_layout_kind kind) {
	switch (kind) {
	case KBASE_KINSTR_JM_READER_LAYOUT_KIND_TIMESTAMP:
		return sizeof(uint64_t);
	case KBASE_KINSTR_JM_READER_LAYOUT_KIND_STATE:
		return sizeof(enum kbase_kinstr_jm_reader_atom_state);
	case KBASE_KINSTR_JM_READER_LAYOUT_KIND_ATOM:
		return sizeof(uint8_t);
	default:
		return SIZE_MAX;
	}
}

/**
 * struct kbase_kinstr_jm_reader_layout_member - Describes the layout of a
 *                                               member in the atom state change
 *                                               structure
 * @offset: determines the offset from the start of the structure
 * @kind:   the kind of the member which describes what type the member is
 *          represented by and what it represents
 *
 * This information can be used to pick and pull out relevant information from
 * a member even when working against older of newer kernel APIs
 */
struct kbase_kinstr_jm_reader_layout_member {
	u8 offset: 5;
	u8 kind: 3;
};

/**
 * struct kbase_kinstr_jm_reader_layout_info - Determines the layout of the
 *                                             reader buffer structure.
 * @size:    the total size of the buffer structure. Can be used to move the
 *           buffer pointer forward to the next structure
 * @members: information about the members of the structure. These can be used
 *           to determine where the relevant fields for determining atom states
 *           are in the structure.
 *
 * This layout information allows for a implementation of a slow path for
 * backwards and forwards compatibility against older and newer kernel APIs.
 *
 * Newer kernels might define extra fields but an older user space reader can
 * still work with it because it can pull out the relevant fields to itself.
 */
struct kbase_kinstr_jm_reader_layout_info {
	u8 size;
	struct kbase_kinstr_jm_reader_layout_member members[7];
};

/**
 * union kbase_kinstr_jm_reader_layout - Represents the layout of the buffer
 *                                       structure that is exposed to user code
 *                                       in the read only memory mapped buffer.
 * @identifier: a single value that can quickly identify that a structure
 *              representation can be cast to
 * @info:       the detailed information about the structure which can be used
 *              to pull out relevant information from future structures when
 *              the user code has not been updated.
 *
 * The returned identifier from the kernel can be used to understand if the
 * readonly buffer can be quickly cast to a corresponding user structure.
 *
 *	struct atom_state_change {
 *		u64 timestamp;
 *		enum kbase_kinstr_jm_reader_atom_state state;
 *		u8 atom;
 *	};
 *
 *	const uint64_t user = KBASE_KINSTR_JM_READER_LAYOUT_IDENTIFIER(
 *		struct atom_state_change,
 *		timestamp, KBASE_KINSTR_JM_READER_LAYOUT_KIND_TIMESTAMP,
 *		state, KBASE_KINSTR_JM_READER_LAYOUT_KIND_STATE,
 *		atom, KBASE_KINSTR_JM_READER_LAYOUT_KIND_ATOM);
 *
 *	const kernel = ioctl(ctx->fd, KBASE_KINSTR_JM_READER_LAYOUT);
 *
 *	assert(kernel == user.identifier);
 *
 * When the kernel identifier matches the user generated identifier we can walk
 * through the memory mapped buffer easily as we can cast it directly to our
 * user structure.
 *
 * When the kernel identifier *does not* match we will be in one of a few
 * states:
 *
 *	- our fields still match the kernel structure but there are new fields
 *	  and/or the size of the structure has changed
 *		- We can cast to our structure safely, we just have to make sure
 *		  we increment the buffer read pointer by the kernel structure
 *		  size
 *	- our fields are still present in the kernel structure
 *		- use the structure layout to correctly find and pull out the
 *		  information in each kernel structure exposed in the buffer
 *	- some of our fields are not present in the kernel structure
 *		- breaking change, we are no longer compatible
 */
union kbase_kinstr_jm_reader_layout {
	u64 identifier;
	struct kbase_kinstr_jm_reader_layout_info info;
};

/** kbase_kinstr_jm_reader_layout_member_compare() - Compares two layout members
 *                                                   for sorting
 * @a: the first member to compare
 * @b: the second member to compare
 *
 * The sorting is performed on the member offset. Any members with a kind of
 * zero will be sorted to the end.
 *
 * Returns:
 * * -1: ``a < b``
 * * 1: ``a > b``
 * * 0: ``a == b``
 */
static inline int kbase_kinstr_jm_reader_layout_member_compare(
	const void *const a, const void *const b)
{
	const struct kbase_kinstr_jm_reader_layout_member *const x =
			(const struct kbase_kinstr_jm_reader_layout_member *)a;
	const struct kbase_kinstr_jm_reader_layout_member *const y =
			(const struct kbase_kinstr_jm_reader_layout_member *)b;
	const u8 l = x->offset + (!x->kind << 5);
	const u8 r = y->offset + (!y->kind << 5);

	return (l > r) - (l < r);
}

/** kbase_kinstr_jm_reader_layout_sort() - Sorts the members in the layout
 * @layout: the layout to sort
 *
 * After the function completes the ``layout->members`` will be sorted as
 * specified by kbase_kinstr_jm_reader_layout_member_compare()
 */
static inline void kbase_kinstr_jm_reader_layout_sort(
	union kbase_kinstr_jm_reader_layout *const layout)
{
	struct kbase_kinstr_jm_reader_layout_member *const members =
		layout->info.members;
	const size_t size = sizeof(layout->info.members) / sizeof(*members);

	KBASE_KINSTR_JM_READER_SORT(
		members, size, sizeof(*members),
		kbase_kinstr_jm_reader_layout_member_compare);
}

/** kbase_kinstr_jm_reader_layout_sorted() - Sorts the members in the layout
 * @layout: the layout to sort
 *
 * After the function completes the ``layout->members`` will be sorted as
 * specified by kbase_kinstr_jm_reader_layout_member_compare()
 *
 * Returns: the sorted layout
 */
static inline union kbase_kinstr_jm_reader_layout
kbase_kinstr_jm_reader_layout_sorted(
	union kbase_kinstr_jm_reader_layout layout)
{
	kbase_kinstr_jm_reader_layout_sort(&layout);
	return layout;
}

/** kbase_kinstr_jm_reader_layout_members_includes() - ``haystack`` will include
 *                                                     all of ``needles``
 * @haystack: the layout members to search
 * @needles: the layout members to check for in the haystack
 * @compare: the comparison function, return false to break loop
 *
 * Will walk through needles and provide each of haystack to the comparison
 * callback. If any of the haystack and needle pairs match up, we continue to
 * the next needle.
 *
 * Returns: result of comparison
 */
static inline bool kbase_kinstr_jm_reader_layout_members_includes(
	union kbase_kinstr_jm_reader_layout haystack,
	union kbase_kinstr_jm_reader_layout needles,
	bool (*compare)(
		struct kbase_kinstr_jm_reader_layout_member h,
		struct kbase_kinstr_jm_reader_layout_member n))
{
	const size_t size =
		sizeof(needles.info.members) / sizeof(*needles.info.members);
	const struct kbase_kinstr_jm_reader_layout_member
		*const nb = needles.info.members,
		*const ne = nb + size,
		*const hb = haystack.info.members,
		*const he = hb + size,
		*n, *h;
	bool ok = true;

	kbase_kinstr_jm_reader_layout_sort(&needles);
	kbase_kinstr_jm_reader_layout_sort(&haystack);
	for (n = nb; n < ne && n->kind && ok; ++n)
		for (ok = false, h = hb; h < he && h->kind && !ok; ++h)
			if (compare(*n, *h))
				ok = true;
	return ok;
}

/** kbase_kinstr_jm_reader_layout_member_kind_equal() - Layout member kinds are
 *                                                      equal
 * @a: the first member
 * @b: the second member
 * Returns: ``true`` when the member kinds are equal
 */
static inline bool kbase_kinstr_jm_reader_layout_member_kind_equal(
		struct kbase_kinstr_jm_reader_layout_member a,
		struct kbase_kinstr_jm_reader_layout_member b)
{
	return a.kind == b.kind;
}

/** kbase_kinstr_jm_reader_layout_member_equal() - Layout members are equal
 * @a: the first member
 * @b: the second member
 * Returns: ``true`` when equal
 */
static inline bool kbase_kinstr_jm_reader_layout_member_equal(
		struct kbase_kinstr_jm_reader_layout_member a,
		struct kbase_kinstr_jm_reader_layout_member b)
{
	return a.kind == b.kind && a.offset == b.offset;
}

/** kbase_kinstr_jm_reader_layout_members_compatible() - Determines if a layout
 *                                                       contains all of the
 *                                                       members of another
 * @from: the source layout
 * @to:   the target layout
 *
 * This can be used to understand if a layout can provide all the necessary
 * fields in subset.
 *
 * If ``from`` is compatible with ``to``, then the ``from`` layout can be used
 * to find all the data needed to fulfil ``to``.
 *
 * Returns: layout contains all the same members kinds as subset and is
 * therefore compatible
 */
static inline bool kbase_kinstr_jm_reader_layout_members_compatible(
	union kbase_kinstr_jm_reader_layout from,
	union kbase_kinstr_jm_reader_layout to)
{
	return kbase_kinstr_jm_reader_layout_members_includes(
		from, to,
		kbase_kinstr_jm_reader_layout_member_kind_equal);
}

/** kbase_kinstr_jm_reader_layout_members_convertible() - Determines if a layout
 *                                                        members are compatible
 *                                                        with the members of
 *                                                        another layout
 * @from: the layout to convert from
 * @to:   the layout to convert to
 *
 * Determines that ``to`` has the same members as ``from`` with the same
 * offsets which means that the ``from`` structure can be cast to the
 * ``to`` structure safely.
 *
 * Returns: ``derived`` members are compatible with the members of ``base``
 */
static inline bool kbase_kinstr_jm_reader_layout_members_convertible(
	union kbase_kinstr_jm_reader_layout from,
	union kbase_kinstr_jm_reader_layout to)
{
	return from.info.size >= to.info.size &&
		kbase_kinstr_jm_reader_layout_members_includes(
			from, to,
			kbase_kinstr_jm_reader_layout_member_equal);
}

/** kbase_kinstr_jm_reader_layout_valid() - Determines if the layout is valid
 * @layout: the layout to check
 *
 * Makes sure that there are no duplicate fields and that the size is word
 * aligned.
 *
 * Returns: the result of the check
 */
static inline bool kbase_kinstr_jm_reader_layout_valid(
	union kbase_kinstr_jm_reader_layout layout)
{
	const size_t size =
		sizeof(layout.info.members) / sizeof(*layout.info.members);
	const struct kbase_kinstr_jm_reader_layout_member
		*const mb = layout.info.members,
		*const me = mb + size,
		*i, *j;
	bool found = true;

	if ((layout.info.size % sizeof(unsigned int)) != 0)
		return false;

	kbase_kinstr_jm_reader_layout_sort(&layout);

#define kind_size kbase_kinstr_jm_reader_layout_kind_size
	for (i = mb; i < me && i->kind; ++i)
		if ((i->offset + kind_size(i->kind)) > layout.info.size)
			return false;
#undef kind_size

	for (i = mb; i < me && i->kind && found; ++i)
		for (found = false, j = mb; j < me && j->kind && !found; ++j)
			if (i != j && i->kind == j->kind)
				found = true;
	return true;
}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(structure, member, _kind) \
	{ \
		.offset = offsetof(structure, member), \
		.kind = _kind \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_1(\
		s, m1, k1) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_2(\
		s, m1, k1, m2, k2) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m2, k2) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_3(\
		s, m1, k1, m2, k2, m3, k3) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m2, k2), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m3, k3) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_4(\
		s, m1, k1, m2, k2, m3, k3, m4, k4) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m2, k2), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m3, k3), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m4, k4) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_5(\
		s, m1, k1, m2, k2, m3, k3, m4, k4, m5, k5) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m2, k2), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m3, k3), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m4, k4), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m5, k5) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_6(\
		s, m1, k1, m2, k2, m3, k3, m4, k4, m5, k5, m6, k6) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m2, k2), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m3, k3), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m4, k4), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m5, k5), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m6, k6) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_7(\
		s, m1, k1, m2, k2, m3, k3, m4, k4, m5, k5, m6, k6, m7, k7) \
	{ \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m1, k1), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m2, k2), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m3, k3), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m4, k4), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m5, k5), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m6, k6), \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBER_INIT(s, m7, k7) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_SELECT( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, \
	name, ...) \
	name

#define KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT(s, ...) \
	KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_SELECT(__VA_ARGS__, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_7, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_6, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_5, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_4, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_3, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_2, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_1, \
		KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT_INVALID) \
	(s, __VA_ARGS__)

#define KBASE_KINSTR_JM_READER_LAYOUT_INFO_INIT(s, ...) \
	{ \
		.size = sizeof(s), \
		.members = \
		    KBASE_KINSTR_JM_READER_LAYOUT_MEMBERS_INIT(s, __VA_ARGS__) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_INIT(s, ...) \
	{ \
		.info = \
		       KBASE_KINSTR_JM_READER_LAYOUT_INFO_INIT(s, __VA_ARGS__) \
	}

#define KBASE_KINSTR_JM_READER_LAYOUT_IDENTIFIER(s, ...) \
	(((union kbase_kinstr_jm_reader_layout) \
	KBASE_KINSTR_JM_READER_LAYOUT_INIT(s, __VA_ARGS__)).identifier)

#endif /* _KBASE_KINSTR_JM_READER_H_ */
