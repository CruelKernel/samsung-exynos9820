/* iva_rt_table.c
 *
 * Copyright (C) 2017 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>

#include "iva_rt_table.h"

/* Global ID structure */
typedef struct _global_id {
	uint16_t	local_entry_id	: 12;
	uint16_t	target_type	: 4;
	uint16_t	global_entry_id : 16;
} global_id;

typedef struct _global_id_field {
	union {
		uint32_t    gid_num;
		global_id   gid;
	};
} global_id_field;

typedef struct _rt_tile_config_entry_t {
	uint16_t	tile_width;
	uint16_t	tile_height;
} rt_tile_config_entry_t;

typedef struct _rt_iva_entry_t {
	global_id		guid;
	rt_tile_config_entry_t	tile_config;
	uint16_t	input_dep_offset;
	uint16_t	output_trigger_offset;
	uint16_t	node_offset;
	uint8_t		processor_id;
	uint8_t		flags;
} rt_iva_entry_t, *rt_iva_entry;

/* IVA RT Table Header: NOTICE - do not use the pointer inside */
typedef struct _rt_iva_table_t {
	uint32_t	entries;
	uint32_t	table_id;
	uint16_t	num_entries;
	uint8_t 	is_updated;
	uint8_t		empty;
	uint32_t	reserved;
} rt_iva_table_t, *rt_iva_table;

static inline rt_iva_entry_t *rt_iva_get_first_iva_entry(const rt_iva_table_t *iva_table)
{
	return (rt_iva_entry_t *)((uint8_t *) iva_table + sizeof(rt_iva_table_t));
}

#define DEBUG_PRINT_LINE(dev)		dev_info(dev,			\
		"+----------------------------------------------"	\
		"------------------------------------------+\n");

void iva_rt_print_iva_entries(struct iva_dev_data *iva,
		void *iva_tbl_ptr, int8_t *num_deps_list)
{
	struct device	*dev = iva->dev;
	rt_iva_table_t	*iva_table = iva_tbl_ptr;
	rt_iva_entry_t	*iva_fst_entry;
	rt_iva_entry_t	*iva_entry;
	uint32_t	n;
	size_t		start_addr;
	uint32_t	ofs;
	global_id_field	tmp_gid;
	char		log_buf[512];
	int32_t		nod, ref_nod;

	if (!iva_table)
		return;

	DEBUG_PRINT_LINE(dev);
	snprintf(log_buf, sizeof(log_buf),
			"|  ofs |     GID  TGT | in ofs | out ofs | cfg ofs "
			"|   Tile Conf   | PID | Flg | Depends |\n");
	dev_info(dev, "%s", log_buf);
	DEBUG_PRINT_LINE(dev);

	iva_fst_entry = rt_iva_get_first_iva_entry(iva_table);
	start_addr = (size_t) iva_fst_entry;
	for (n = 0; n < iva_table->num_entries; n++) {
		iva_entry = iva_fst_entry + n;

		ofs = (uint32_t)((size_t)iva_entry - start_addr);
		tmp_gid.gid = iva_entry->guid;
		ref_nod = *(int32_t *)((int8_t *) iva_table +
						iva_entry->input_dep_offset);
		if (num_deps_list)
			nod = num_deps_list[n];
		else
			nod = ref_nod;

		snprintf(log_buf, sizeof(log_buf),
				"| %4x |"		/* offset */
				" %8x %s |"		/* GID  TARGET */
				" %6x |"		/* Input Dep Offset */
				"  %6x |"		/* Output Trig Offset */
				"  %6x |"		/* Config Offset */
				" %7d x %-3d |"		/* Tile Config */
				" %3d |"		/* Proc ID */
				" %3x |"		/* Flags */
				" %3d(%2d) |\n",	/* num of (remaning) dependencies */
				ofs,
				tmp_gid.gid_num,
				iva_entry->guid.target_type == 1 ? "IVA":
					iva_entry->guid.target_type == 2 ?
						"SCR" : "NON",
				iva_entry->input_dep_offset,
				iva_entry->output_trigger_offset,
				iva_entry->node_offset,
				iva_entry->tile_config.tile_width,
				iva_entry->tile_config.tile_height,
				iva_entry->processor_id,
				iva_entry->flags,
				nod, ref_nod);


		dev_info(dev, "%s", log_buf);
	}
	DEBUG_PRINT_LINE(dev);
}


#define NUM_LINE	(80)
#define NUM_DATA	(3)
#define BIN_LOG_MAGIC	(0xeeeeee00)
#define BIN_LOG_MAGIC_M	(0xffffff00)
#define BIN_LOG_MAGIC_S	(8)

typedef struct _rt_bin_log_ {
	uint32_t	type;
	uint32_t	data[NUM_DATA];
} rt_bin_log_t;

typedef struct _rt_bin_dbg_ {
	uint32_t	bin_log_idx;
	rt_bin_log_t	rt_bin_log[NUM_LINE];
} rt_bin_dbg_t;

void iva_ram_dump_print_iva_bin_log(struct iva_dev_data *iva, void *bin_dbg_ptr)
{
#define IVA_BIN_LOG_LINE	"=============================================\n"
#define IVA_BIN_LOG_LINE_M	"---------------------------------------------\n"
	struct device	*dev = iva->dev;
	int32_t		i;
	uint32_t	idx;
	rt_bin_dbg_t	*iva_bin_dbg = bin_dbg_ptr;
	rt_bin_log_t	*iva_bin_log;

	if (!iva_bin_dbg)
		return;

	idx = iva_bin_dbg->bin_log_idx;

	dev_info(dev, IVA_BIN_LOG_LINE);
	dev_info(dev, "  IVA BIN LOG DUMP   max(%3d) cur_idx(%3d)\n",
			NUM_LINE, idx);
	dev_info(dev, IVA_BIN_LOG_LINE_M);
	dev_info(dev, " INDEX TYPE   DATA0      DATA1      DATA2\n");


	for (i = 0; i < NUM_LINE; i++) {
		iva_bin_log = &iva_bin_dbg->rt_bin_log[idx];
		if ((iva_bin_log->type >> BIN_LOG_MAGIC_S) ==
				(BIN_LOG_MAGIC >> BIN_LOG_MAGIC_S)) {
			dev_info(dev, " %5d %4d 0x%08X 0x%08X 0x%08X\n",
					idx,
					iva_bin_log->type & (~BIN_LOG_MAGIC_M),
					iva_bin_log->data[0],
					iva_bin_log->data[1],
					iva_bin_log->data[2]);
		}

		if (++idx >= NUM_LINE)
			idx = 0;
	}

	dev_info(dev, IVA_BIN_LOG_LINE);
}
