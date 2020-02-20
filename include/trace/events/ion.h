#undef TRACE_SYSTEM
#define TRACE_SYSTEM ion

#if !defined(_TRACE_ION_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ION_H

#include <linux/types.h>
#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(ion_rbin,

	TP_PROTO(const char *heap_name,
		 void *buffer,
		 unsigned long size,
		 void *page),
	TP_ARGS(heap_name, buffer, size, page),

	TP_STRUCT__entry(
		__field(const char *, heap_name)
		__field(void *, buffer)
		__field(unsigned long, size)
		__field(void *, page)
	),

	TP_fast_assign(
		__entry->heap_name	= heap_name;
		__entry->buffer		= buffer;
		__entry->size		= size;
		__entry->page		= page;
	),

	TP_printk("heap_name=%s buffer=%p size=%lu page=%p",
		__entry->heap_name,
		__entry->buffer,
		__entry->size,
		__entry->page
	)
);

DEFINE_EVENT(ion_rbin, ion_rbin_alloc_start,
	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_alloc_end,
	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_free_start,
	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_free_end,
	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_partial_alloc_start,
	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_partial_alloc_end,
	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_pool_alloc_start,

	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);

DEFINE_EVENT(ion_rbin, ion_rbin_pool_alloc_end,

	TP_PROTO(const char *heap_name, void *buffer, unsigned long size,
		 void *page),

	TP_ARGS(heap_name, buffer, size, page)
);
#endif /* _TRACE_ION_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
