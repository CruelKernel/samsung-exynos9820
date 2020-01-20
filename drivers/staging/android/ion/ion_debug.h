/*
 * drivers/staging/android/ion/ion_debug.h
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

#ifndef _ION_DEBUG_H_
#define _ION_DEBUG_H_

#ifdef CONFIG_ION_EXYNOS
void ion_contig_heap_show_buffers(struct seq_file *s, struct ion_heap *heap,
				  phys_addr_t base, size_t pool_size);
#else
#define ion_contig_heap_show_buffers do { } while (0)
#endif

enum ion_event_type {
	ION_EVENT_TYPE_ALLOC = 0,
	ION_EVENT_TYPE_FREE,
	ION_EVENT_TYPE_MMAP,
	ION_EVENT_TYPE_KMAP,
	ION_EVENT_TYPE_MAP_DMA_BUF,
	ION_EVENT_TYPE_UNMAP_DMA_BUF,
	ION_EVENT_TYPE_BEGIN_CPU_ACCESS,
	ION_EVENT_TYPE_END_CPU_ACCESS,
	ION_EVENT_TYPE_IOVMM_MAP,
};

#ifdef CONFIG_ION_DEBUG_EVENT_RECORD
void ion_event_record(enum ion_event_type type,
		      struct ion_buffer *buffer, ktime_t begin);
#define ion_event_begin()	ktime_t begin = ktime_get()
#define ion_event_end(type, buffer)	ion_event_record(type, buffer, begin)
#else
#define ion_event_record(type, buffer, begin)	do { } while (0)
#define ion_event_begin()	do { } while (0)
#define ion_event_end(type, buffer)	do { } while (0)
#endif
#endif /* _ION_DEBUG_H_ */
