#include "dm-verity-debug.h"

#include <linux/module.h>
#include <linux/reboot.h>

#include <linux/ctype.h>

#define DM_MSG_PREFIX			"verity"

#define DM_VERITY_ENV_LENGTH		42
#define DM_VERITY_ENV_VAR_NAME		"DM_VERITY_ERR_BLOCK_NR"

#define DM_VERITY_DEFAULT_PREFETCH_SIZE	262144

#define DM_VERITY_MAX_CORRUPTED_ERRS	100

extern int ignore_fs_panic;

static void print_block_data(unsigned long long blocknr, unsigned char *data_to_dump
			, int start, int len)
{
	int i, j;
	int bh_offset = (start / 16) * 16;
	char row_data[17] = { 0, };
	char row_hex[50] = { 0, };
	char ch;

	if (blocknr == 0) {
		pr_err("printing Hash dump %dbyte, hash_to_dump 0x%p\n", len, (void *)data_to_dump);
	} else {
		pr_err("dm-verity corrupted, printing data in hex\n");
		pr_err(" dump block# : %llu, start offset(byte) : %d\n"
				, blocknr, start);
		pr_err(" length(byte) : %d, data_to_dump 0x%p\n"
				, len, (void *)data_to_dump);
		pr_err("-------------------------------------------------\n");
	}
	for (i = 0; i < (len + 15) / 16; i++) {
		for (j = 0; j < 16; j++) {
			ch = *(data_to_dump + bh_offset + j);
			if (start <= bh_offset + j
				&& start + len > bh_offset + j) {

				if (isascii(ch) && isprint(ch))
					sprintf(row_data + j, "%c", ch);
				else
					sprintf(row_data + j, ".");

				sprintf(row_hex + (j * 3), "%2.2x ", ch);
			} else {
				sprintf(row_data + j, " ");
				sprintf(row_hex + (j * 3), "-- ");
			}
		}

		pr_err("0x%4.4x : %s | %s\n"
				, bh_offset, row_hex, row_data);
		bh_offset += 16;
	}
	pr_err("---------------------------------------------------\n");
}

int verity_handle_err_hex_debug(struct dm_verity *v, enum verity_block_type type,
			     unsigned long long block, struct dm_verity_io *io, struct bvec_iter *iter)
{
	char verity_env[DM_VERITY_ENV_LENGTH];
	char *envp[] = { verity_env, NULL };
	const char *type_str = "";
	struct dm_buffer *buf;
	struct mapped_device *md = dm_table_get_md(v->ti->table);
	struct bio *bio = dm_bio_from_per_bio_data(io, v->ti->per_io_data_size);
	struct bio_vec bv;
	u8 *data;
	u8 *page;

	int i;
	char hex_str[65] = {0, };
	/* Corruption should be visible in device status in all modes */
	v->hash_failed = 1;

	if (ignore_fs_panic) {
		DMERR("%s: Don't trigger a panic during cleanup for shutdown. Skipping %s",
				v->data_dev->name, __func__);
		return 0;
	}

	if (v->corrupted_errs >= DM_VERITY_MAX_CORRUPTED_ERRS)
		goto out;

	v->corrupted_errs++;

	switch (type) {
	case DM_VERITY_BLOCK_TYPE_DATA:
		type_str = "data";
		break;
	case DM_VERITY_BLOCK_TYPE_METADATA:
		type_str = "metadata";
		break;
	default:
		BUG();
	}

	DMERR("%s: %s block %llu is corrupted", v->data_dev->name, type_str, block);
	
	for(i=0 ; i < v->salt_size; i++){
		sprintf(hex_str + (i * 2), "%02x", *(v->salt + i)); 	
	}
	DMERR("dm-verity salt: %s", hex_str);

	if (!(strcmp(type_str, "metadata"))) {
		data = dm_bufio_read(v->bufio, block, &buf);
		print_block_data(0, (unsigned char *)(verity_io_real_digest(v, io)), 0, v->digest_size);
		print_block_data(0, (unsigned char *)(verity_io_want_digest(v, io)), 0, v->digest_size);
		print_block_data((unsigned long long)block, (unsigned char *)data, 0, PAGE_SIZE);
	} else if (!(strcmp(type_str, "data"))) {
		bv = bio_iter_iovec(bio, *iter);
		page = kmap_atomic(bv.bv_page);
		print_block_data(0, (unsigned char *)(verity_io_real_digest(v, io)), 0, v->digest_size);
		print_block_data(0, (unsigned char *)(verity_io_want_digest(v, io)), 0, v->digest_size);
		print_block_data((unsigned long long)block, (unsigned char *)page, 0, PAGE_SIZE);
		kunmap_atomic(page);
	} else {
		DMERR("%s: %s block : Unknown block type", v->data_dev->name, type_str);
	}
	
	panic("dmv corrupt");

	if (v->corrupted_errs == DM_VERITY_MAX_CORRUPTED_ERRS)
		DMERR("%s: reached maximum errors", v->data_dev->name);

	snprintf(verity_env, DM_VERITY_ENV_LENGTH, "%s=%d,%llu",
		DM_VERITY_ENV_VAR_NAME, type, block);

	kobject_uevent_env(&disk_to_dev(dm_disk(md))->kobj, KOBJ_CHANGE, envp);

out:
	if (v->mode == DM_VERITY_MODE_LOGGING)
		return 0;

	if (v->mode == DM_VERITY_MODE_RESTART)
		kernel_restart("dm-verity device corrupted");

	return 1;
}
