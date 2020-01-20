/*
 * Copyright (C) 2013 Google, Inc.
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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mfc

#if !defined(_TRACE_MFC_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MFC_H

#include <linux/types.h>
#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(mfc_node,

	TP_PROTO(int ctx_num,
			int num_inst,
			unsigned int type,
			int is_drm),

	TP_ARGS(ctx_num, num_inst, type, is_drm),

	TP_STRUCT__entry(
		__field(	int,		ctx_num		)
		__field(	int,		num_inst	)
		__field(	unsigned int,   type		)
		__field(	int,		is_drm		)
	),

	TP_fast_assign(
		__entry->ctx_num	= ctx_num;
		__entry->num_inst	= num_inst;
		__entry->type		= type;
		__entry->is_drm		= is_drm;
	),

	TP_printk("ctx[%d] total inst=%d, type=%d, %s",
			__entry->ctx_num,
			__entry->num_inst,
			__entry->type,
			__entry->is_drm ? "drm" : "normal"
	)
);

DEFINE_EVENT(mfc_node, mfc_node_open,

	TP_PROTO(int ctx_num,
			int num_inst,
			unsigned int type,
			int is_drm),

	TP_ARGS(ctx_num, num_inst, type, is_drm)
);

DEFINE_EVENT(mfc_node, mfc_node_close,

	TP_PROTO(int ctx_num,
			int num_inst,
			unsigned int type,
			int is_drm),

	TP_ARGS(ctx_num, num_inst, type, is_drm)
);

DECLARE_EVENT_CLASS(mfc_loadfw,

	TP_PROTO(size_t fw_region_size,
			size_t fw_size),

	TP_ARGS(fw_region_size, fw_size),

	TP_STRUCT__entry(
		__field(size_t, fw_region_size)
		__field(size_t, fw_size)
	),

	TP_fast_assign(
		__entry->fw_region_size	= fw_region_size;
		__entry->fw_size	= fw_size;
	),

	TP_printk("FW region: %ld, size: %ld",
			__entry->fw_region_size,
			__entry->fw_size
	)
);

DEFINE_EVENT(mfc_loadfw, mfc_loadfw_start,

	TP_PROTO(size_t fw_region_size,
			size_t fw_size),

	TP_ARGS(fw_region_size, fw_size)
);

DEFINE_EVENT(mfc_loadfw, mfc_loadfw_end,

	TP_PROTO(size_t fw_region_size,
			size_t fw_size),

	TP_ARGS(fw_region_size, fw_size)
);

DECLARE_EVENT_CLASS(mfc_dcpp,

	TP_PROTO(int ctx_num,
			int is_support_smc,
			int drm_fw_status),

	TP_ARGS(ctx_num, is_support_smc, drm_fw_status),

	TP_STRUCT__entry(
		__field(	int,		ctx_num		)
		__field(	int,		is_support_smc	)
		__field(	int,		drm_fw_status	)
	),

	TP_fast_assign(
		__entry->ctx_num	= ctx_num;
		__entry->is_support_smc	= is_support_smc;
		__entry->drm_fw_status	= drm_fw_status;
	),

	TP_printk("ctx[%d] support drm=%d, drm fw %s",
			__entry->ctx_num,
			__entry->is_support_smc,
			__entry->drm_fw_status ? "loaded" : "not-loaded"
	)
);

DEFINE_EVENT(mfc_dcpp, mfc_dcpp_start,

	TP_PROTO(int ctx_num,
			int is_support_smc,
			int drm_fw_status),

	TP_ARGS(ctx_num, is_support_smc, drm_fw_status)
);

DEFINE_EVENT(mfc_dcpp, mfc_dcpp_end,

	TP_PROTO(int ctx_num,
			int is_support_smc,
			int drm_fw_status),

	TP_ARGS(ctx_num, is_support_smc, drm_fw_status)
);

DECLARE_EVENT_CLASS(mfc_frame,

	TP_PROTO(int ctx_num,
			int reason,
			int type,
			int is_drm),

	TP_ARGS(ctx_num, reason, type, is_drm),

	TP_STRUCT__entry(
		__field(	int,		ctx_num )
		__field(	int,		reason	)
		__field(	int,		type	)
		__field(	int,		is_drm	)
	),

	TP_fast_assign(
		__entry->ctx_num	= ctx_num;
		__entry->reason		= reason;
		__entry->type		= type;
		__entry->is_drm		= is_drm;
	),

	TP_printk("ctx[%d] reason=%d, type=%d, %s",
			__entry->ctx_num,
			__entry->reason,
			__entry->type,
			__entry->is_drm ? "drm" : "normal"
	)
);

DEFINE_EVENT(mfc_frame, mfc_frame_start,

	TP_PROTO(int ctx_num,
			int reason,
			int type,
			int is_drm),

	TP_ARGS(ctx_num, reason, type, is_drm)
);

DEFINE_EVENT(mfc_frame, mfc_frame_top,

	TP_PROTO(int ctx_num,
			int reason,
			int type,
			int is_drm),

	TP_ARGS(ctx_num, reason, type, is_drm)
);

DEFINE_EVENT(mfc_frame, mfc_frame_bottom,

	TP_PROTO(int ctx_num,
			int reason,
			int type,
			int is_drm),

	TP_ARGS(ctx_num, reason, type, is_drm)
);

#endif /* _TRACE_MFC_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
