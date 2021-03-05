// SPDX-License-Identifier: GPL-2.0
/*
 * f2fs sysfs interface
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 * Copyright (c) 2017 Chao Yu <chao@kernel.org>
 */
#include <linux/compiler.h>
#include <linux/proc_fs.h>
#include <linux/f2fs_fs.h>
#include <linux/seq_file.h>
#include <linux/statfs.h>
#include <linux/nls.h>

#include "f2fs.h"
#include "segment.h"
#include "gc.h"

#define SEC_BIGDATA_VERSION	(2)

static struct proc_dir_entry *f2fs_proc_root;

/* Sysfs support for f2fs */
enum {
	GC_THREAD,	/* struct f2fs_gc_thread */
	SM_INFO,	/* struct f2fs_sm_info */
	DCC_INFO,	/* struct discard_cmd_control */
	NM_INFO,	/* struct f2fs_nm_info */
	F2FS_SBI,	/* struct f2fs_sb_info */
#ifdef CONFIG_F2FS_FAULT_INJECTION
	FAULT_INFO_RATE,	/* struct f2fs_fault_info */
	FAULT_INFO_TYPE,	/* struct f2fs_fault_info */
#endif
	RESERVED_BLOCKS,	/* struct f2fs_sb_info */
};

#ifdef CONFIG_F2FS_SEC_BLOCK_OPERATIONS_DEBUG
const char *sec_blkops_dbg_type_names[NR_F2FS_SEC_DBG_ENTRY] = {
	"DENTS",
	"IMETA",
	"NODES",
};
#endif

const char *sec_fua_mode_names[NR_F2FS_SEC_FUA_MODE] = {
	"NONE",
	"ROOT",
	"ALL",
};

struct f2fs_attr {
	struct attribute attr;
	ssize_t (*show)(struct f2fs_attr *, struct f2fs_sb_info *, char *);
	ssize_t (*store)(struct f2fs_attr *, struct f2fs_sb_info *,
			 const char *, size_t);
	int struct_type;
	int offset;
	int id;
};

static unsigned char *__struct_ptr(struct f2fs_sb_info *sbi, int struct_type)
{
	if (struct_type == GC_THREAD)
		return (unsigned char *)sbi->gc_thread;
	else if (struct_type == SM_INFO)
		return (unsigned char *)SM_I(sbi);
	else if (struct_type == DCC_INFO)
		return (unsigned char *)SM_I(sbi)->dcc_info;
	else if (struct_type == NM_INFO)
		return (unsigned char *)NM_I(sbi);
	else if (struct_type == F2FS_SBI || struct_type == RESERVED_BLOCKS)
		return (unsigned char *)sbi;
#ifdef CONFIG_F2FS_FAULT_INJECTION
	else if (struct_type == FAULT_INFO_RATE ||
					struct_type == FAULT_INFO_TYPE)
		return (unsigned char *)&F2FS_OPTION(sbi).fault_info;
#endif
	return NULL;
}

static ssize_t dirty_segments_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%llu\n",
		(unsigned long long)(dirty_segments(sbi)));
}

static ssize_t lifetime_write_kbytes_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	struct super_block *sb = sbi->sb;

	if (!sb->s_bdev->bd_part)
		return snprintf(buf, PAGE_SIZE, "0\n");

	return snprintf(buf, PAGE_SIZE, "%llu\n",
		(unsigned long long)(sbi->kbytes_written +
			BD_PART_WRITTEN(sbi)));
}

static ssize_t sec_fs_stat_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	struct dentry *root = sbi->sb->s_root;
	struct f2fs_checkpoint *ckpt = F2FS_CKPT(sbi);
	struct kstatfs statbuf;
	int ret;

	if (!root->d_sb->s_op->statfs)
		goto errout;

	ret = root->d_sb->s_op->statfs(root, &statbuf);
	if (ret)
		goto errout;

	return snprintf(buf, PAGE_SIZE, "\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%u\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%u\",\"%s\":\"%u\","
		"\"%s\":\"%d\"\n",
		"F_BLOCKS", statbuf.f_blocks,
		"F_BFREE", statbuf.f_bfree,
		"F_SFREE", free_sections(sbi),
		"F_FILES", statbuf.f_files,
		"F_FFREE", statbuf.f_ffree,
		"F_FUSED", ckpt->valid_inode_count,
		"F_NUSED", ckpt->valid_node_count,
		"F_VER", SEC_BIGDATA_VERSION);

errout:
	return snprintf(buf, PAGE_SIZE, "\"%s\":\"%d\",\"%s\":\"%d\",\"%s\":\"%d\","
		"\"%s\":\"%d\",\"%s\":\"%d\",\"%s\":\"%d\",\"%s\":\"%d\","
		"\"%s\":\"%d\"\n",
		"F_BLOCKS", 0, "F_BFREE", 0, "F_SFREE", 0, "F_FILES", 0,
		"F_FFREE", 0, "F_FUSED", 0, "F_NUSED", 0, "F_VER", SEC_BIGDATA_VERSION);
}

static ssize_t features_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	struct super_block *sb = sbi->sb;
	int len = 0;

	if (!sb->s_bdev->bd_part)
		return snprintf(buf, PAGE_SIZE, "0\n");

	if (f2fs_sb_has_encrypt(sb))
		len += snprintf(buf, PAGE_SIZE - len, "%s",
						"encryption");
	if (f2fs_sb_has_blkzoned(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "blkzoned");
	if (f2fs_sb_has_extra_attr(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "extra_attr");
	if (f2fs_sb_has_project_quota(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "projquota");
	if (f2fs_sb_has_inode_chksum(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "inode_checksum");
	if (f2fs_sb_has_flexible_inline_xattr(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "flexible_inline_xattr");
	if (f2fs_sb_has_quota_ino(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "quota_ino");
	if (f2fs_sb_has_inode_crtime(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "inode_crtime");
	if (f2fs_sb_has_lost_found(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "lost_found");
	if (f2fs_sb_has_sb_chksum(sb))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "sb_checksum");
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

static ssize_t current_reserved_blocks_show(struct f2fs_attr *a,
					struct f2fs_sb_info *sbi, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", sbi->current_reserved_blocks);
}

#ifdef CONFIG_F2FS_SEC_BLOCK_OPERATIONS_DEBUG
static int f2fs_sec_blockops_dbg(struct f2fs_sb_info *sbi, char *buf, int src_len) {
	int len = src_len;
	int i, j;

	len += snprintf(buf + len, PAGE_SIZE - len, "\nblock_operations() DBG : %u, max : %llu\n",
			sbi->s_sec_blkops_total,
			sbi->s_sec_blkops_max_elapsed);
	for (i = 0; i < F2FS_SEC_BLKOPS_ENTRIES; i++) {
		len += snprintf(buf + len, PAGE_SIZE - len, " - [%u - %s(%d)] S: %llu, E: %llu [%llu]",
				sbi->s_sec_dbg_entries[i].entry_idx,
				sec_blkops_dbg_type_names[sbi->s_sec_dbg_entries[i].step],
				sbi->s_sec_dbg_entries[i].ret_val,
				sbi->s_sec_dbg_entries[i].start_time,
				sbi->s_sec_dbg_entries[i].end_time,
				(sbi->s_sec_dbg_entries[i].end_time - sbi->s_sec_dbg_entries[i].start_time));

		for(j = 0; j < NR_F2FS_SEC_DBG_ENTRY; j++) {
			len += snprintf(buf + len, PAGE_SIZE - len, ", %s: [%u] [%llu]",
					sec_blkops_dbg_type_names[j],
					sbi->s_sec_dbg_entries[i].entry[j].nr_ops,
					sbi->s_sec_dbg_entries[i].entry[j].cumulative_jiffies);
		}
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "\n - [MAX - %s(%d)] S: %llu, E: %llu [%llu]",
				sec_blkops_dbg_type_names[sbi->s_sec_dbg_max_entry.step],
				sbi->s_sec_dbg_max_entry.ret_val,
				sbi->s_sec_dbg_max_entry.start_time,
				sbi->s_sec_dbg_max_entry.end_time,
				(sbi->s_sec_dbg_max_entry.end_time - sbi->s_sec_dbg_max_entry.start_time));
	for(j = 0; j < NR_F2FS_SEC_DBG_ENTRY; j++) {
		len += snprintf(buf + len, PAGE_SIZE - len, ", %s: [%u] [%llu]",
				sec_blkops_dbg_type_names[j],
				sbi->s_sec_dbg_max_entry.entry[j].nr_ops,
				sbi->s_sec_dbg_max_entry.entry[j].cumulative_jiffies);
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return (len - src_len);
}
#endif

/* Copy from debug.c stat_show */
static ssize_t f2fs_sec_stats_show(struct f2fs_sb_info *sbi, char *buf)
{
	struct f2fs_stat_info *si = sbi->stat_info;
	int i = 0, len = 0;
	int j;

	f2fs_update_sec_stats(sbi);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"\n=====[ partition info(%pg). #%d, %s, CP: %s]=====\n",
			si->sbi->sb->s_bdev, i++,
			f2fs_readonly(si->sbi->sb) ? "RO": "RW",
			is_set_ckpt_flags(si->sbi, CP_DISABLED_FLAG) ?
			"Disabled": (f2fs_cp_error(si->sbi) ? "Error": "Good"));
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[SB: 1] [CP: 2] [SIT: %d] [NAT: %d] ",
			si->sit_area_segs, si->nat_area_segs);
	len += snprintf(buf + len, PAGE_SIZE - len, "[SSA: %d] [MAIN: %d",
			si->ssa_area_segs, si->main_area_segs);
	len += snprintf(buf + len, PAGE_SIZE - len, "(OverProv:%d Resv:%d)]\n\n",
			si->overp_segs, si->rsvd_segs);
	if (test_opt(si->sbi, DISCARD))
		len += snprintf(buf + len, PAGE_SIZE - len, "Utilization: %u%% (%u valid blocks, %u discard blocks)\n",
				si->utilization, si->valid_count, si->discard_blks);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "Utilization: %u%% (%u valid blocks)\n",
				si->utilization, si->valid_count);

	len += snprintf(buf + len, PAGE_SIZE - len, "  - Node: %u (Inode: %u, ",
			si->valid_node_count, si->valid_inode_count);
	len += snprintf(buf + len, PAGE_SIZE - len, "Other: %u)\n  - Data: %u\n",
			si->valid_node_count - si->valid_inode_count,
			si->valid_count - si->valid_node_count);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Inline_xattr Inode: %u\n",
			si->inline_xattr);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Inline_data Inode: %u\n",
			si->inline_inode);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Inline_dentry Inode: %u\n",
			si->inline_dir);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Orphan/Append/Update Inode: %u, %u, %u\n",
			si->orphans, si->append, si->update);
	len += snprintf(buf + len, PAGE_SIZE - len, "\nMain area: %d segs, %d secs %d zones\n",
			si->main_area_segs, si->main_area_sections,
			si->main_area_zones);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - COLD  data: %d, %d, %d\n",
			si->curseg[CURSEG_COLD_DATA],
			si->cursec[CURSEG_COLD_DATA],
			si->curzone[CURSEG_COLD_DATA]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - WARM  data: %d, %d, %d\n",
			si->curseg[CURSEG_WARM_DATA],
			si->cursec[CURSEG_WARM_DATA],
			si->curzone[CURSEG_WARM_DATA]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - HOT   data: %d, %d, %d\n",
			si->curseg[CURSEG_HOT_DATA],
			si->cursec[CURSEG_HOT_DATA],
			si->curzone[CURSEG_HOT_DATA]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Dir   dnode: %d, %d, %d\n",
			si->curseg[CURSEG_HOT_NODE],
			si->cursec[CURSEG_HOT_NODE],
			si->curzone[CURSEG_HOT_NODE]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - File   dnode: %d, %d, %d\n",
			si->curseg[CURSEG_WARM_NODE],
			si->cursec[CURSEG_WARM_NODE],
			si->curzone[CURSEG_WARM_NODE]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Indir nodes: %d, %d, %d\n",
			si->curseg[CURSEG_COLD_NODE],
			si->cursec[CURSEG_COLD_NODE],
			si->curzone[CURSEG_COLD_NODE]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n  - Valid: %d\n  - Dirty: %d\n",
			si->main_area_segs - si->dirty_count -
			si->prefree_count - si->free_segs,
			si->dirty_count);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Prefree: %d\n  - Free: %d (%d)\n\n",
			si->prefree_count, si->free_segs, si->free_secs);
	len += snprintf(buf + len, PAGE_SIZE - len, "CP calls: %d (BG: %d)\n",
			si->cp_count, si->bg_cp_count);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - cp blocks : %u\n", si->meta_count[META_CP]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - sit blocks : %u\n",
			si->meta_count[META_SIT]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - nat blocks : %u\n",
			si->meta_count[META_NAT]);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - ssa blocks : %u\n",
			si->meta_count[META_SSA]);
	len += snprintf(buf + len, PAGE_SIZE - len, "GC calls: %d (BG: %d)\n",
			si->call_count, si->bg_gc);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - data segments : %d (%d)\n",
			si->data_segs, si->bg_data_segs);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - node segments : %d (%d)\n",
			si->node_segs, si->bg_node_segs);
	len += snprintf(buf + len, PAGE_SIZE - len, "Try to move %d blocks (BG: %d)\n", si->tot_blks,
			si->bg_data_blks + si->bg_node_blks);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - data blocks : %d (%d)\n", si->data_blks,
			si->bg_data_blks);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - node blocks : %d (%d)\n", si->node_blks,
			si->bg_node_blks);
	len += snprintf(buf + len, PAGE_SIZE - len, "Skipped : atomic write %llu (%llu)\n",
			si->skipped_atomic_files[BG_GC] +
			si->skipped_atomic_files[FG_GC],
			si->skipped_atomic_files[BG_GC]);
	len += snprintf(buf + len, PAGE_SIZE - len, "BG skip : IO: %u, Other: %u\n",
			si->io_skip_bggc, si->other_skip_bggc);
	len += snprintf(buf + len, PAGE_SIZE - len, "\nExtent Cache:\n");
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Hit Count: L1-1:%llu L1-2:%llu L2:%llu\n",
			si->hit_largest, si->hit_cached,
			si->hit_rbtree);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Hit Ratio: %llu%% (%llu / %llu)\n",
			!si->total_ext ? 0 :
			div64_u64(si->hit_total * 100, si->total_ext),
			si->hit_total, si->total_ext);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - Inner Struct Count: tree: %d(%d), node: %d\n",
			si->ext_tree, si->zombie_tree, si->ext_node);
	len += snprintf(buf + len, PAGE_SIZE - len, "\nBalancing F2FS Async:\n");
	len += snprintf(buf + len, PAGE_SIZE - len, "  - IO_R (Data: %4d, Node: %4d, Meta: %4d\n",
			si->nr_rd_data, si->nr_rd_node, si->nr_rd_meta);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - IO_W (CP: %4d, Data: %4d, Flush: (%4d %4d %4d), "
			"Discard: (%4d %4d)) cmd: %4d undiscard:%4u\n",
			si->nr_wb_cp_data, si->nr_wb_data,
			si->nr_flushing, si->nr_flushed,
			si->flush_list_empty,
			si->nr_discarding, si->nr_discarded,
			si->nr_discard_cmd, si->undiscard_blks);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - inmem: %4d, atomic IO: %4d (Max. %4d), "
			"volatile IO: %4d (Max. %4d)\n",
			si->inmem_pages, si->aw_cnt, si->max_aw_cnt,
			si->vw_cnt, si->max_vw_cnt);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - nodes: %4d in %4d\n",
			si->ndirty_node, si->node_pages);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - dents: %4d in dirs:%4d (%4d)\n",
			si->ndirty_dent, si->ndirty_dirs, si->ndirty_all);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - datas: %4d in files:%4d\n",
			si->ndirty_data, si->ndirty_files);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - quota datas: %4d in quota files:%4d\n",
			si->ndirty_qdata, si->nquota_files);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - meta: %4d in %4d\n",
			si->ndirty_meta, si->meta_pages);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - imeta: %4d\n",
			si->ndirty_imeta);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - NATs: %9d/%9d\n  - SITs: %9d/%9d\n",
			si->dirty_nats, si->nats, si->dirty_sits, si->sits);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - free_nids: %9d/%9d\n  - alloc_nids: %9d\n",
			si->free_nids, si->avail_nids, si->alloc_nids);
	len += snprintf(buf + len, PAGE_SIZE - len, "\nDistribution of User Blocks:");
	len += snprintf(buf + len, PAGE_SIZE - len, " [ valid | invalid | free ]\n");
	len += snprintf(buf + len, PAGE_SIZE - len, "  [");

	for (j = 0; j < si->util_valid; j++)
		len += snprintf(buf + len, PAGE_SIZE - len, "-");
	len += snprintf(buf + len, PAGE_SIZE - len, "|");

	for (j = 0; j < si->util_invalid; j++)
		len += snprintf(buf + len, PAGE_SIZE - len, "-");
	len += snprintf(buf + len, PAGE_SIZE - len, "|");

	for (j = 0; j < si->util_free; j++)
		len += snprintf(buf + len, PAGE_SIZE - len, "-");
	len += snprintf(buf + len, PAGE_SIZE - len, "]\n\n");
	len += snprintf(buf + len, PAGE_SIZE - len, "IPU: %u blocks\n", si->inplace_count);
	len += snprintf(buf + len, PAGE_SIZE - len, "SSR: %u blocks in %u segments\n",
			si->block_count[SSR], si->segment_count[SSR]);
	len += snprintf(buf + len, PAGE_SIZE - len, "LFS: %u blocks in %u segments\n",
			si->block_count[LFS], si->segment_count[LFS]);

	/* segment usage info */
	len += snprintf(buf + len, PAGE_SIZE - len, "\nBDF: %u, avg. vblocks: %u\n",
			si->bimodal, si->avg_vblocks);

	/* memory footprint */
	len += snprintf(buf + len, PAGE_SIZE - len, "\nMemory: %llu KB\n",
			(si->base_mem + si->cache_mem + si->page_mem) >> 10);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - static: %llu KB\n",
			si->base_mem >> 10);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - cached: %llu KB\n",
			si->cache_mem >> 10);
	len += snprintf(buf + len, PAGE_SIZE - len, "  - paged : %llu KB\n",
			si->page_mem >> 10);

#ifdef CONFIG_F2FS_SEC_BLOCK_OPERATIONS_DEBUG
	/* block_operations debug node */
	len += f2fs_sec_blockops_dbg(sbi, buf, len);
#endif
	return len;
}

static void __sec_bigdata_init_value(struct f2fs_sb_info *sbi,
		const char *attr_name)
{
	unsigned int i = 0;

	if (!strcmp(attr_name, "sec_gc_stat")) {
		sbi->sec_stat.gc_count[BG_GC] = 0;
		sbi->sec_stat.gc_count[FG_GC] = 0;
		sbi->sec_stat.gc_node_seg_count[BG_GC] = 0;
		sbi->sec_stat.gc_node_seg_count[FG_GC] = 0;
		sbi->sec_stat.gc_data_seg_count[BG_GC] = 0;
		sbi->sec_stat.gc_data_seg_count[FG_GC] = 0;
		sbi->sec_stat.gc_node_blk_count[BG_GC] = 0;
		sbi->sec_stat.gc_node_blk_count[FG_GC] = 0;
		sbi->sec_stat.gc_data_blk_count[BG_GC] = 0;
		sbi->sec_stat.gc_data_blk_count[FG_GC] = 0;
		sbi->sec_stat.gc_ttime[BG_GC] = 0;
		sbi->sec_stat.gc_ttime[FG_GC] = 0;
	} else if (!strcmp(attr_name, "sec_io_stat")) {
		sbi->sec_stat.cp_cnt[STAT_CP_ALL] = 0;
		sbi->sec_stat.cp_cnt[STAT_CP_BG] = 0;
		sbi->sec_stat.cp_cnt[STAT_CP_FSYNC] = 0;
		for (i = 0; i < NR_CP_REASON; i++)
			sbi->sec_stat.cpr_cnt[i] = 0;
		sbi->sec_stat.cp_max_interval = 0;
		sbi->sec_stat.alloc_seg_type[LFS] = 0;
		sbi->sec_stat.alloc_seg_type[SSR] = 0;
		sbi->sec_stat.alloc_blk_count[LFS] = 0;
		sbi->sec_stat.alloc_blk_count[SSR] = 0;
		atomic64_set(&sbi->sec_stat.inplace_count, 0);
		sbi->sec_stat.fsync_count = 0;
		sbi->sec_stat.fsync_dirty_pages = 0;
		sbi->sec_stat.hot_file_written_blocks = 0;
		sbi->sec_stat.cold_file_written_blocks = 0;
		sbi->sec_stat.warm_file_written_blocks = 0;
		sbi->sec_stat.max_inmem_pages = 0;
		sbi->sec_stat.drop_inmem_all = 0;
		sbi->sec_stat.drop_inmem_files = 0;
		if (sbi->sb->s_bdev->bd_part)
			sbi->sec_stat.kwritten_byte = BD_PART_WRITTEN(sbi);
		sbi->sec_stat.fs_por_error = 0;
		sbi->sec_stat.fs_error = 0;
		sbi->sec_stat.max_undiscard_blks = 0;
	} else if (!strcmp(attr_name, "sec_fsck_stat")) {
		sbi->sec_fsck_stat.fsck_read_bytes = 0;
		sbi->sec_fsck_stat.fsck_written_bytes = 0;
		sbi->sec_fsck_stat.fsck_elapsed_time = 0;
		sbi->sec_fsck_stat.fsck_exit_code = 0;
		sbi->sec_fsck_stat.valid_node_count = 0;
		sbi->sec_fsck_stat.valid_inode_count = 0;
	} else if (!strcmp(attr_name, "sec_defrag_stat")) {
		sbi->s_sec_part_best_extents = 0;
		sbi->s_sec_part_current_extents = 0;
		sbi->s_sec_part_score = 0;
		sbi->s_sec_defrag_writes_kb = 0;
		sbi->s_sec_num_apps = 0;
		sbi->s_sec_capacity_apps_kb = 0;
	}
}

static ssize_t f2fs_sbi_show(struct f2fs_attr *a,
			struct f2fs_sb_info *sbi, char *buf)
{
	unsigned char *ptr = NULL;
	unsigned int *ui;

	ptr = __struct_ptr(sbi, a->struct_type);
	if (!ptr)
		return -EINVAL;

	if (!strcmp(a->attr.name, "extension_list")) {
		__u8 (*extlist)[F2FS_EXTENSION_LEN] =
					sbi->raw_super->extension_list;
		int cold_count = le32_to_cpu(sbi->raw_super->extension_count);
		int hot_count = sbi->raw_super->hot_ext_count;
		int len = 0, i;

		len += snprintf(buf + len, PAGE_SIZE - len,
						"cold file extension:\n");
		for (i = 0; i < cold_count; i++)
			len += snprintf(buf + len, PAGE_SIZE - len, "%s\n",
								extlist[i]);

		len += snprintf(buf + len, PAGE_SIZE - len,
						"hot file extension:\n");
		for (i = cold_count; i < cold_count + hot_count; i++)
			len += snprintf(buf + len, PAGE_SIZE - len, "%s\n",
								extlist[i]);
		return len;
	} else if (!strcmp(a->attr.name, "sec_gc_stat")) {
		int len = 0;

		len = snprintf(buf, PAGE_SIZE, "\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\"\n",
			"FGGC", sbi->sec_stat.gc_count[FG_GC],
			"FGGC_NSEG", sbi->sec_stat.gc_node_seg_count[FG_GC],
			"FGGC_NBLK", sbi->sec_stat.gc_node_blk_count[FG_GC],
			"FGGC_DSEG", sbi->sec_stat.gc_data_seg_count[FG_GC],
			"FGGC_DBLK", sbi->sec_stat.gc_data_blk_count[FG_GC],
			"FGGC_TTIME", sbi->sec_stat.gc_ttime[FG_GC],
			"BGGC", sbi->sec_stat.gc_count[BG_GC],
			"BGGC_NSEG", sbi->sec_stat.gc_node_seg_count[BG_GC],
			"BGGC_NBLK", sbi->sec_stat.gc_node_blk_count[BG_GC],
			"BGGC_DSEG", sbi->sec_stat.gc_data_seg_count[BG_GC],
			"BGGC_DBLK", sbi->sec_stat.gc_data_blk_count[BG_GC],
			"BGGC_TTIME", sbi->sec_stat.gc_ttime[BG_GC]);

		if (!sbi->sec_hqm_preserve)
			__sec_bigdata_init_value(sbi, a->attr.name);

		return len;
	} else if (!strcmp(a->attr.name, "sec_io_stat")) {
		u64 kbytes_written = 0;
		int len = 0;

		if (sbi->sb->s_bdev->bd_part)
			kbytes_written = BD_PART_WRITTEN(sbi) -
					 sbi->sec_stat.kwritten_byte;

		len = snprintf(buf, PAGE_SIZE, "\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\","
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%u\","
		"\"%s\":\"%u\",\"%s\":\"%u\"\n",
			"CP",		sbi->sec_stat.cp_cnt[STAT_CP_ALL],
			"CPBG",		sbi->sec_stat.cp_cnt[STAT_CP_BG],
			"CPSYNC",	sbi->sec_stat.cp_cnt[STAT_CP_FSYNC],
			"CPNONRE",	sbi->sec_stat.cpr_cnt[CP_NON_REGULAR],
			"CPSBNEED",	sbi->sec_stat.cpr_cnt[CP_SB_NEED_CP],
			"CPWPINO",	sbi->sec_stat.cpr_cnt[CP_WRONG_PINO],
			"CP_MAX_INT",	sbi->sec_stat.cp_max_interval,
			"LFSSEG",	sbi->sec_stat.alloc_seg_type[LFS],
			"SSRSEG",	sbi->sec_stat.alloc_seg_type[SSR],
			"LFSBLK",	sbi->sec_stat.alloc_blk_count[LFS],
			"SSRBLK",	sbi->sec_stat.alloc_blk_count[SSR],
			"IPU",		(u64)atomic64_read(&sbi->sec_stat.inplace_count),
			"FSYNC",	sbi->sec_stat.fsync_count,
			"FSYNC_MB",	sbi->sec_stat.fsync_dirty_pages >> 8,
			"HOT_DATA",	sbi->sec_stat.hot_file_written_blocks >> 8,
			"COLD_DATA",	sbi->sec_stat.cold_file_written_blocks >> 8,
			"WARM_DATA",	sbi->sec_stat.warm_file_written_blocks >> 8,
			"MAX_INMEM",	sbi->sec_stat.max_inmem_pages,
			"DROP_INMEM",	sbi->sec_stat.drop_inmem_all,
			"DROP_INMEMF",	sbi->sec_stat.drop_inmem_files,
			"WRITE_MB",	(u64)(kbytes_written >> 10),
			"FS_PERROR",	sbi->sec_stat.fs_por_error,
			"FS_ERROR",	sbi->sec_stat.fs_error,
			"MAX_UNDSCD",	sbi->sec_stat.max_undiscard_blks);

		if (!sbi->sec_hqm_preserve)
			__sec_bigdata_init_value(sbi, a->attr.name);

		return len;
	} else if (!strcmp(a->attr.name, "sec_fsck_stat")) {
		int len = 0;

		len = snprintf(buf, PAGE_SIZE,
		"\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%llu\",\"%s\":\"%u\","
		"\"%s\":\"%u\",\"%s\":\"%u\"\n",
			"FSCK_RBYTES",	sbi->sec_fsck_stat.fsck_read_bytes,
			"FSCK_WBYTES",	sbi->sec_fsck_stat.fsck_written_bytes,
			"FSCK_TIME_MS",	sbi->sec_fsck_stat.fsck_elapsed_time,
			"FSCK_EXIT",	sbi->sec_fsck_stat.fsck_exit_code,
			"FSCK_VNODES",	sbi->sec_fsck_stat.valid_node_count,
			"FSCK_VINODES",	sbi->sec_fsck_stat.valid_inode_count);

		if (!sbi->sec_hqm_preserve)
			__sec_bigdata_init_value(sbi, a->attr.name);

		return len;
	} else if (!strcmp(a->attr.name, "sec_defrag_stat")) {
		int len = 0;

		len = snprintf(buf, PAGE_SIZE,
		"\"%s\":\"%u\",\"%s\":\"%u\",\"%s\":\"%u\",\"%s\":\"%u\",\"%s\":\"%u\",\"%s\":\"%u\"\n",
			"BESTEXT",  sbi->s_sec_part_best_extents,
			"CUREXT",   sbi->s_sec_part_current_extents,
			"DEFSCORE", sbi->s_sec_part_score,
			"DEFWRITE", sbi->s_sec_defrag_writes_kb,
			"NUMAPP",   sbi->s_sec_num_apps,
			"CAPAPP",   sbi->s_sec_capacity_apps_kb);

		if (!sbi->sec_hqm_preserve)
			__sec_bigdata_init_value(sbi, a->attr.name);

		return len;
	} else if (!strcmp(a->attr.name, "sec_fua_mode")) {
		int len = 0, i;

		for (i = 0; i < NR_F2FS_SEC_FUA_MODE; i++) {
			if (i == sbi->s_sec_cond_fua_mode)
				len += snprintf(buf, PAGE_SIZE, "[%s] ", 
						sec_fua_mode_names[i]);
			else
				len += snprintf(buf, PAGE_SIZE, "%s ", 
						sec_fua_mode_names[i]);
		}

		return len;
	} else if (!strcmp(a->attr.name, "sec_stats")) {
		return f2fs_sec_stats_show(sbi, buf);
	}

	ui = (unsigned int *)(ptr + a->offset);

	return snprintf(buf, PAGE_SIZE, "%u\n", *ui);
}

static ssize_t __sbi_store(struct f2fs_attr *a,
			struct f2fs_sb_info *sbi,
			const char *buf, size_t count)
{
	unsigned char *ptr;
	unsigned long t;
	unsigned int *ui;
	ssize_t ret;

	ptr = __struct_ptr(sbi, a->struct_type);
	if (!ptr)
		return -EINVAL;

	if (!strcmp(a->attr.name, "extension_list")) {
		const char *name = strim((char *)buf);
		bool set = true, hot;

		if (!strncmp(name, "[h]", 3))
			hot = true;
		else if (!strncmp(name, "[c]", 3))
			hot = false;
		else
			return -EINVAL;

		name += 3;

		if (*name == '!') {
			name++;
			set = false;
		}

		if (strlen(name) >= F2FS_EXTENSION_LEN)
			return -EINVAL;

		down_write(&sbi->sb_lock);

		ret = f2fs_update_extension_list(sbi, name, hot, set);
		if (ret)
			goto out;

		ret = f2fs_commit_super(sbi, false);
		if (ret)
			f2fs_update_extension_list(sbi, name, hot, !set);
out:
		up_write(&sbi->sb_lock);
		return ret ? ret : count;
	} else if(!strcmp(a->attr.name, "sec_gc_stat")) {
		__sec_bigdata_init_value(sbi, a->attr.name);
		return count;
	} else if (!strcmp(a->attr.name, "sec_io_stat")) {
		__sec_bigdata_init_value(sbi, a->attr.name);
		return count;
	} else if (!strcmp(a->attr.name, "sec_fsck_stat")) {
		__sec_bigdata_init_value(sbi, a->attr.name);
		return count;
	} else if (!strcmp(a->attr.name, "sec_defrag_stat")) {
		__sec_bigdata_init_value(sbi, a->attr.name);
		return count;
	} else if (!strcmp(a->attr.name, "sec_fua_mode")) {
		const char *mode= strim((char *)buf);
		int idx;

		for (idx = 0; idx < NR_F2FS_SEC_FUA_MODE; idx++) {
			if(!strcmp(mode, sec_fua_mode_names[idx]))
				sbi->s_sec_cond_fua_mode = idx;
		}

		return count;
	}

	ui = (unsigned int *)(ptr + a->offset);

	ret = kstrtoul(skip_spaces(buf), 0, &t);
	if (ret < 0)
		return ret;
#ifdef CONFIG_F2FS_FAULT_INJECTION
	if (a->struct_type == FAULT_INFO_TYPE && t >= (1 << FAULT_MAX))
		return -EINVAL;
#endif
	if (a->struct_type == RESERVED_BLOCKS) {
		spin_lock(&sbi->stat_lock);
		if (t > (unsigned long)(sbi->user_block_count -
				F2FS_OPTION(sbi).root_reserved_blocks)) {
			spin_unlock(&sbi->stat_lock);
			return -EINVAL;
		}
		*ui = t;
		sbi->current_reserved_blocks = min(sbi->reserved_blocks,
				sbi->user_block_count - valid_user_blocks(sbi));
		spin_unlock(&sbi->stat_lock);
		return count;
	}

	if (!strcmp(a->attr.name, "discard_granularity")) {
		if (t == 0 || t > MAX_PLIST_NUM)
			return -EINVAL;
		if (t == *ui)
			return count;
		*ui = t;
		return count;
	}

	if (!strcmp(a->attr.name, "trim_sections"))
		return -EINVAL;

	if (!strcmp(a->attr.name, "gc_urgent")) {
		if (t >= 1) {
			sbi->gc_mode = GC_URGENT;
			if (sbi->gc_thread) {
				sbi->gc_thread->gc_wake = 1;
				wake_up_interruptible_all(
					&sbi->gc_thread->gc_wait_queue_head);
				wake_up_discard_thread(sbi, true);
			}
		} else {
			sbi->gc_mode = GC_NORMAL;
		}
		return count;
	}
	if (!strcmp(a->attr.name, "gc_idle")) {
		if (t == GC_IDLE_CB)
			sbi->gc_mode = GC_IDLE_CB;
		else if (t == GC_IDLE_GREEDY)
			sbi->gc_mode = GC_IDLE_GREEDY;
		else
			sbi->gc_mode = GC_NORMAL;
		return count;
	}

	*ui = t;

	if (!strcmp(a->attr.name, "iostat_enable") && *ui == 0)
		f2fs_reset_iostat(sbi);
	return count;
}

static ssize_t f2fs_sbi_store(struct f2fs_attr *a,
			struct f2fs_sb_info *sbi,
			const char *buf, size_t count)
{
	ssize_t ret;
	bool gc_entry = (!strcmp(a->attr.name, "gc_urgent") ||
					a->struct_type == GC_THREAD);

	if (gc_entry) {
		if (!down_read_trylock(&sbi->sb->s_umount))
			return -EAGAIN;
	}
	ret = __sbi_store(a, sbi, buf, count);
	if (gc_entry)
		up_read(&sbi->sb->s_umount);

	return ret;
}

static ssize_t f2fs_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->show ? a->show(a, sbi, buf) : 0;
}

static ssize_t f2fs_attr_store(struct kobject *kobj, struct attribute *attr,
						const char *buf, size_t len)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
									s_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->store ? a->store(a, sbi, buf, len) : 0;
}

static void f2fs_sb_release(struct kobject *kobj)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_kobj);
	complete(&sbi->s_kobj_unregister);
}

enum feat_id {
	FEAT_CRYPTO = 0,
	FEAT_BLKZONED,
	FEAT_ATOMIC_WRITE,
	FEAT_EXTRA_ATTR,
	FEAT_PROJECT_QUOTA,
	FEAT_INODE_CHECKSUM,
	FEAT_FLEXIBLE_INLINE_XATTR,
	FEAT_QUOTA_INO,
	FEAT_INODE_CRTIME,
	FEAT_LOST_FOUND,
	FEAT_SB_CHECKSUM,
};

static ssize_t f2fs_feature_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	switch (a->id) {
	case FEAT_CRYPTO:
	case FEAT_BLKZONED:
	case FEAT_ATOMIC_WRITE:
	case FEAT_EXTRA_ATTR:
	case FEAT_PROJECT_QUOTA:
	case FEAT_INODE_CHECKSUM:
	case FEAT_FLEXIBLE_INLINE_XATTR:
	case FEAT_QUOTA_INO:
	case FEAT_INODE_CRTIME:
	case FEAT_LOST_FOUND:
	case FEAT_SB_CHECKSUM:
		return snprintf(buf, PAGE_SIZE, "supported\n");
	}
	return 0;
}

#define F2FS_ATTR_OFFSET(_struct_type, _name, _mode, _show, _store, _offset) \
static struct f2fs_attr f2fs_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.show	= _show,					\
	.store	= _store,					\
	.struct_type = _struct_type,				\
	.offset = _offset					\
}

#define F2FS_RW_ATTR(struct_type, struct_name, name, elname)	\
	F2FS_ATTR_OFFSET(struct_type, name, 0644,		\
		f2fs_sbi_show, f2fs_sbi_store,			\
		offsetof(struct struct_name, elname))

#define F2FS_RW_ATTR_640(struct_type, struct_name, name, elname)	\
	F2FS_ATTR_OFFSET(struct_type, name, 0640,		\
		f2fs_sbi_show, f2fs_sbi_store,			\
		offsetof(struct struct_name, elname))

#define F2FS_GENERAL_RO_ATTR(name) \
static struct f2fs_attr f2fs_attr_##name = __ATTR(name, 0444, name##_show, NULL)

#define F2FS_FEATURE_RO_ATTR(_name, _id)			\
static struct f2fs_attr f2fs_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = 0444 },	\
	.show	= f2fs_feature_show,				\
	.id	= _id,						\
}

F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_urgent_sleep_time,
							urgent_sleep_time);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_min_sleep_time, min_sleep_time);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_max_sleep_time, max_sleep_time);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_no_gc_sleep_time, no_gc_sleep_time);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_idle, gc_mode);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_urgent, gc_mode);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, reclaim_segments, rec_prefree_segments);
F2FS_RW_ATTR(DCC_INFO, discard_cmd_control, max_small_discards, max_discards);
F2FS_RW_ATTR(DCC_INFO, discard_cmd_control, discard_granularity, discard_granularity);
F2FS_RW_ATTR(RESERVED_BLOCKS, f2fs_sb_info, reserved_blocks, reserved_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, batched_trim_sections, trim_sections);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, ipu_policy, ipu_policy);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_ipu_util, min_ipu_util);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_fsync_blocks, min_fsync_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_seq_blocks, min_seq_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_hot_blocks, min_hot_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_ssr_sections, min_ssr_sections);
F2FS_RW_ATTR(NM_INFO, f2fs_nm_info, ram_thresh, ram_thresh);
F2FS_RW_ATTR(NM_INFO, f2fs_nm_info, ra_nid_pages, ra_nid_pages);
F2FS_RW_ATTR(NM_INFO, f2fs_nm_info, dirty_nats_ratio, dirty_nats_ratio);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, max_victim_search, max_victim_search);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, dir_level, dir_level);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, cp_interval, interval_time[CP_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, idle_interval, interval_time[REQ_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, discard_idle_interval,
					interval_time[DISCARD_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_idle_interval, interval_time[GC_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info,
		umount_discard_timeout, interval_time[UMOUNT_DISCARD_TIMEOUT]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, iostat_enable, iostat_enable);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, readdir_ra, readdir_ra);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_pin_file_thresh, gc_pin_file_threshold);
F2FS_RW_ATTR(F2FS_SBI, f2fs_super_block, extension_list, extension_list);
#ifdef CONFIG_F2FS_FAULT_INJECTION
F2FS_RW_ATTR(FAULT_INFO_RATE, f2fs_fault_info, inject_rate, inject_rate);
F2FS_RW_ATTR(FAULT_INFO_TYPE, f2fs_fault_info, inject_type, inject_type);
#endif
F2FS_RW_ATTR_640(F2FS_SBI, f2fs_sb_info, sec_gc_stat, sec_stat);
F2FS_RW_ATTR_640(F2FS_SBI, f2fs_sb_info, sec_io_stat, sec_stat);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_stats, stat_info);
F2FS_RW_ATTR_640(F2FS_SBI, f2fs_sb_info, sec_fsck_stat, sec_fsck_stat);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_part_best_extents, s_sec_part_best_extents);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_part_current_extents, s_sec_part_current_extents);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_part_score, s_sec_part_score);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_defrag_writes_kb, s_sec_defrag_writes_kb);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_num_apps, s_sec_num_apps);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_capacity_apps_kb, s_sec_capacity_apps_kb);
F2FS_RW_ATTR_640(F2FS_SBI, f2fs_sb_info, sec_defrag_stat, s_sec_part_best_extents);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_fua_mode, s_sec_cond_fua_mode);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, sec_hqm_preserve, sec_hqm_preserve);
F2FS_GENERAL_RO_ATTR(dirty_segments);
F2FS_GENERAL_RO_ATTR(lifetime_write_kbytes);
F2FS_GENERAL_RO_ATTR(sec_fs_stat);
F2FS_GENERAL_RO_ATTR(features);
F2FS_GENERAL_RO_ATTR(current_reserved_blocks);

#ifdef CONFIG_F2FS_FS_ENCRYPTION
F2FS_FEATURE_RO_ATTR(encryption, FEAT_CRYPTO);
#endif
#ifdef CONFIG_BLK_DEV_ZONED
F2FS_FEATURE_RO_ATTR(block_zoned, FEAT_BLKZONED);
#endif
F2FS_FEATURE_RO_ATTR(atomic_write, FEAT_ATOMIC_WRITE);
F2FS_FEATURE_RO_ATTR(extra_attr, FEAT_EXTRA_ATTR);
F2FS_FEATURE_RO_ATTR(project_quota, FEAT_PROJECT_QUOTA);
F2FS_FEATURE_RO_ATTR(inode_checksum, FEAT_INODE_CHECKSUM);
F2FS_FEATURE_RO_ATTR(flexible_inline_xattr, FEAT_FLEXIBLE_INLINE_XATTR);
F2FS_FEATURE_RO_ATTR(quota_ino, FEAT_QUOTA_INO);
F2FS_FEATURE_RO_ATTR(inode_crtime, FEAT_INODE_CRTIME);
F2FS_FEATURE_RO_ATTR(lost_found, FEAT_LOST_FOUND);
F2FS_FEATURE_RO_ATTR(sb_checksum, FEAT_SB_CHECKSUM);

#define ATTR_LIST(name) (&f2fs_attr_##name.attr)
static struct attribute *f2fs_attrs[] = {
	ATTR_LIST(gc_urgent_sleep_time),
	ATTR_LIST(gc_min_sleep_time),
	ATTR_LIST(gc_max_sleep_time),
	ATTR_LIST(gc_no_gc_sleep_time),
	ATTR_LIST(gc_idle),
	ATTR_LIST(gc_urgent),
	ATTR_LIST(reclaim_segments),
	ATTR_LIST(max_small_discards),
	ATTR_LIST(discard_granularity),
	ATTR_LIST(batched_trim_sections),
	ATTR_LIST(ipu_policy),
	ATTR_LIST(min_ipu_util),
	ATTR_LIST(min_fsync_blocks),
	ATTR_LIST(min_seq_blocks),
	ATTR_LIST(min_hot_blocks),
	ATTR_LIST(min_ssr_sections),
	ATTR_LIST(max_victim_search),
	ATTR_LIST(dir_level),
	ATTR_LIST(ram_thresh),
	ATTR_LIST(ra_nid_pages),
	ATTR_LIST(dirty_nats_ratio),
	ATTR_LIST(cp_interval),
	ATTR_LIST(idle_interval),
	ATTR_LIST(discard_idle_interval),
	ATTR_LIST(gc_idle_interval),
	ATTR_LIST(umount_discard_timeout),
	ATTR_LIST(iostat_enable),
	ATTR_LIST(readdir_ra),
	ATTR_LIST(gc_pin_file_thresh),
	ATTR_LIST(extension_list),
	ATTR_LIST(sec_gc_stat),
	ATTR_LIST(sec_io_stat),
	ATTR_LIST(sec_stats),
	ATTR_LIST(sec_fsck_stat),
	ATTR_LIST(sec_part_best_extents),
	ATTR_LIST(sec_part_current_extents),
	ATTR_LIST(sec_part_score),
	ATTR_LIST(sec_defrag_writes_kb),
	ATTR_LIST(sec_num_apps),
	ATTR_LIST(sec_capacity_apps_kb),
	ATTR_LIST(sec_defrag_stat),
	ATTR_LIST(sec_fua_mode),
	ATTR_LIST(sec_hqm_preserve),
#ifdef CONFIG_F2FS_FAULT_INJECTION
	ATTR_LIST(inject_rate),
	ATTR_LIST(inject_type),
#endif
	ATTR_LIST(dirty_segments),
	ATTR_LIST(lifetime_write_kbytes),
	ATTR_LIST(sec_fs_stat),
	ATTR_LIST(features),
	ATTR_LIST(reserved_blocks),
	ATTR_LIST(current_reserved_blocks),
	NULL,
};

static struct attribute *f2fs_feat_attrs[] = {
#ifdef CONFIG_F2FS_FS_ENCRYPTION
	ATTR_LIST(encryption),
#endif
#ifdef CONFIG_BLK_DEV_ZONED
	ATTR_LIST(block_zoned),
#endif
	ATTR_LIST(atomic_write),
	ATTR_LIST(extra_attr),
	ATTR_LIST(project_quota),
	ATTR_LIST(inode_checksum),
	ATTR_LIST(flexible_inline_xattr),
	ATTR_LIST(quota_ino),
	ATTR_LIST(inode_crtime),
	ATTR_LIST(lost_found),
	ATTR_LIST(sb_checksum),
	NULL,
};

static const struct sysfs_ops f2fs_attr_ops = {
	.show	= f2fs_attr_show,
	.store	= f2fs_attr_store,
};

static struct kobj_type f2fs_sb_ktype = {
	.default_attrs	= f2fs_attrs,
	.sysfs_ops	= &f2fs_attr_ops,
	.release	= f2fs_sb_release,
};

static struct kobj_type f2fs_ktype = {
	.sysfs_ops	= &f2fs_attr_ops,
};

static struct kset f2fs_kset = {
	.kobj   = {.ktype = &f2fs_ktype},
};

static struct kobj_type f2fs_feat_ktype = {
	.default_attrs	= f2fs_feat_attrs,
	.sysfs_ops	= &f2fs_attr_ops,
};

static struct kobject f2fs_feat = {
	.kset	= &f2fs_kset,
};

static int __maybe_unused segment_info_seq_show(struct seq_file *seq,
						void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	unsigned int total_segs =
			le32_to_cpu(sbi->raw_super->segment_count_main);
	int i;

	seq_puts(seq, "format: segment_type|valid_blocks\n"
		"segment_type(0:HD, 1:WD, 2:CD, 3:HN, 4:WN, 5:CN)\n");

	for (i = 0; i < total_segs; i++) {
		struct seg_entry *se = get_seg_entry(sbi, i);

		if ((i % 10) == 0)
			seq_printf(seq, "%-10d", i);
		seq_printf(seq, "%d|%-3u", se->type,
					get_valid_blocks(sbi, i, false));
		if ((i % 10) == 9 || i == (total_segs - 1))
			seq_putc(seq, '\n');
		else
			seq_putc(seq, ' ');
	}

	return 0;
}

static int __maybe_unused segment_bits_seq_show(struct seq_file *seq,
						void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	unsigned int total_segs =
			le32_to_cpu(sbi->raw_super->segment_count_main);
	int i, j;

	seq_puts(seq, "format: segment_type|valid_blocks|bitmaps\n"
		"segment_type(0:HD, 1:WD, 2:CD, 3:HN, 4:WN, 5:CN)\n");

	for (i = 0; i < total_segs; i++) {
		struct seg_entry *se = get_seg_entry(sbi, i);

		seq_printf(seq, "%-10d", i);
		seq_printf(seq, "%d|%-3u|", se->type,
					get_valid_blocks(sbi, i, false));
		for (j = 0; j < SIT_VBLOCK_MAP_SIZE; j++)
			seq_printf(seq, " %.2x", se->cur_valid_map[j]);
		seq_putc(seq, '\n');
	}
	return 0;
}

static int __maybe_unused iostat_info_seq_show(struct seq_file *seq,
					       void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	time64_t now = ktime_get_real_seconds();

	if (!sbi->iostat_enable)
		return 0;

	seq_printf(seq, "time:		%-16llu\n", now);

	/* print app IOs */
	seq_printf(seq, "app buffered:	%-16llu\n",
				sbi->write_iostat[APP_BUFFERED_IO]);
	seq_printf(seq, "app direct:	%-16llu\n",
				sbi->write_iostat[APP_DIRECT_IO]);
	seq_printf(seq, "app mapped:	%-16llu\n",
				sbi->write_iostat[APP_MAPPED_IO]);

	/* print fs IOs */
	seq_printf(seq, "fs data:	%-16llu\n",
				sbi->write_iostat[FS_DATA_IO]);
	seq_printf(seq, "fs node:	%-16llu\n",
				sbi->write_iostat[FS_NODE_IO]);
	seq_printf(seq, "fs meta:	%-16llu\n",
				sbi->write_iostat[FS_META_IO]);
	seq_printf(seq, "fs gc data:	%-16llu\n",
				sbi->write_iostat[FS_GC_DATA_IO]);
	seq_printf(seq, "fs gc node:	%-16llu\n",
				sbi->write_iostat[FS_GC_NODE_IO]);
	seq_printf(seq, "fs cp data:	%-16llu\n",
				sbi->write_iostat[FS_CP_DATA_IO]);
	seq_printf(seq, "fs cp node:	%-16llu\n",
				sbi->write_iostat[FS_CP_NODE_IO]);
	seq_printf(seq, "fs cp meta:	%-16llu\n",
				sbi->write_iostat[FS_CP_META_IO]);
	seq_printf(seq, "fs discard:	%-16llu\n",
				sbi->write_iostat[FS_DISCARD]);

	return 0;
}

static int __maybe_unused victim_bits_seq_show(struct seq_file *seq,
						void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct dirty_seglist_info *dirty_i = DIRTY_I(sbi);
	int i;

	seq_puts(seq, "format: victim_secmap bitmaps\n");

	for (i = 0; i < MAIN_SECS(sbi); i++) {
		if ((i % 10) == 0)
			seq_printf(seq, "%-10d", i);
		seq_printf(seq, "%d", test_bit(i, dirty_i->victim_secmap) ? 1 : 0);
		if ((i % 10) == 9 || i == (MAIN_SECS(sbi) - 1))
			seq_putc(seq, '\n');
		else
			seq_putc(seq, ' ');
	}
	return 0;
}

#define F2FS_PROC_FILE_DEF(_name)					\
static int _name##_open_fs(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, _name##_seq_show, PDE_DATA(inode));	\
}									\
									\
static const struct file_operations f2fs_seq_##_name##_fops = {		\
	.open = _name##_open_fs,					\
	.read = seq_read,						\
	.llseek = seq_lseek,						\
	.release = single_release,					\
};

F2FS_PROC_FILE_DEF(segment_info);
F2FS_PROC_FILE_DEF(segment_bits);
F2FS_PROC_FILE_DEF(iostat_info);
F2FS_PROC_FILE_DEF(victim_bits);

int __init f2fs_init_sysfs(void)
{
	int ret;

	kobject_set_name(&f2fs_kset.kobj, "f2fs");
	f2fs_kset.kobj.parent = fs_kobj;
	ret = kset_register(&f2fs_kset);
	if (ret)
		return ret;

	ret = kobject_init_and_add(&f2fs_feat, &f2fs_feat_ktype,
				   NULL, "features");
	if (ret)
		kset_unregister(&f2fs_kset);
	else
		f2fs_proc_root = proc_mkdir("fs/f2fs", NULL);
	return ret;
}

void f2fs_exit_sysfs(void)
{
	kobject_put(&f2fs_feat);
	kset_unregister(&f2fs_kset);
	remove_proc_entry("fs/f2fs", NULL);
	f2fs_proc_root = NULL;
}

#define SEC_MAX_VOLUME_NAME	16
static bool __volume_is_userdata(struct f2fs_sb_info *sbi)
{
	char volume_name[SEC_MAX_VOLUME_NAME] = {0, };

	utf16s_to_utf8s(sbi->raw_super->volume_name, SEC_MAX_VOLUME_NAME,
			UTF16_LITTLE_ENDIAN, volume_name, SEC_MAX_VOLUME_NAME);
	volume_name[SEC_MAX_VOLUME_NAME - 1] = '\0';

	if (!strcmp(volume_name, "data"))
		return true;

	return false;
}

int f2fs_register_sysfs(struct f2fs_sb_info *sbi)
{
	struct super_block *sb = sbi->sb;
	int err;

	sbi->s_kobj.kset = &f2fs_kset;
	init_completion(&sbi->s_kobj_unregister);
	err = kobject_init_and_add(&sbi->s_kobj, &f2fs_sb_ktype, NULL,
				"%s", sb->s_id);
	if (err)
		return err;

	if (__volume_is_userdata(sbi)) {
		err = sysfs_create_link(&f2fs_kset.kobj, &sbi->s_kobj,
				"userdata");
		if (err)
			pr_err("Can not create sysfs link for userdata(%d)\n",
					err);
	}

	if (f2fs_proc_root)
		sbi->s_proc = proc_mkdir(sb->s_id, f2fs_proc_root);

	if (sbi->s_proc) {
		proc_create_data("segment_info", S_IRUGO, sbi->s_proc,
				 &f2fs_seq_segment_info_fops, sb);
		proc_create_data("segment_bits", S_IRUGO, sbi->s_proc,
				 &f2fs_seq_segment_bits_fops, sb);
		proc_create_data("iostat_info", S_IRUGO, sbi->s_proc,
				&f2fs_seq_iostat_info_fops, sb);
		proc_create_data("victim_bits", S_IRUGO, sbi->s_proc,
				&f2fs_seq_victim_bits_fops, sb);
	}
	return 0;
}

void f2fs_unregister_sysfs(struct f2fs_sb_info *sbi)
{
	if (sbi->s_proc) {
		remove_proc_entry("iostat_info", sbi->s_proc);
		remove_proc_entry("segment_info", sbi->s_proc);
		remove_proc_entry("segment_bits", sbi->s_proc);
		remove_proc_entry("victim_bits", sbi->s_proc);
		remove_proc_entry(sbi->sb->s_id, f2fs_proc_root);
	}

	if (__volume_is_userdata(sbi))
		sysfs_delete_link(&f2fs_kset.kobj, &sbi->s_kobj, "userdata");

	kobject_del(&sbi->s_kobj);
}
