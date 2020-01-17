/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <asm/cacheflush.h>
#include "ncp_header.h"
#include "npu-golden.h"
#include "npu-log.h"
#include "npu-debug.h"
#include "npu-util-comparator.h"
#include "npu-common.h"
#include "npu-session.h"

#define GOLDEN_FILENAME_LEN	2048

typedef enum {
	NPU_GOLDEN_STATE_NOT_INITIALIZED,
	NPU_GOLDEN_STATE_OPENED,
	NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES,
	NPU_GOLDEN_STATE_CONFIGURED,
	NPU_GOLDEN_STATE_COMPARING,
	NPU_GOLDEN_STATE_INVALID,
} npu_golden_state_e;

static const char *state_names[NPU_GOLDEN_STATE_INVALID + 1]
	= {"NOT INITIALZED", "OPENED", "ADDING_GOLDEN_FILES", "CONFIGURED", "COMPARING"};

static const u8 transition_map[][STATE_KEEPER_MAX_STATE] = {
/* From - To			NOT INITIALZED	OPENED	ADDING_GOLDEN_FILES	CONFIGURED	COMPARING	INVALID */
/* NOT INITIALZED      */  {	1,		1,	0,			0,		0,		0},
/* OPENED              */  {	1,		0,	1,			0,		0,		0},
/* ADDING_GOLDEN_FILES */  {	1,		0,	1,			1,		0,		0},
/* CONFIGURED          */  {	1,		0,	0,			0,		1,		0},
/* COMPARING           */  {	1,		0,	0,			1,		1,		0},
/* INVALID             */  {	0,		0,	0,			0,		0,		0},
};

/*
 * Trailing message to notify the result is truncated
 * due to the limitation of buffer size
 */
static const char *TRUNCATE_MSG = "\nWarning:Result is truncated.\n";

struct golden_file {
	u32	av_index;
	size_t	size;
	struct  list_head	list;
	u8 *img_vbuf;
	char	file_name[GOLDEN_FILENAME_LEN+1];
};

struct parse_buf {
	size_t	idx;
	int	(*value_setter)(struct parse_buf *parse_buf);
	char	token_char[GOLDEN_FILENAME_LEN+1];
};

typedef int (*action_func)(const char ch, struct parse_buf *parse_buf);

typedef enum {
	TOKEN_ALPHA,	/* Alphabet and some special character */
	TOKEN_NUMBER,	/* Number */
	TOKEN_WS,	/* White space */
	TOKEN_NL,	/* New line */
	TOKEN_SHARP,	/* '#' sign */
	TOKEN_INVALID,
} npu_golden_set_token;

typedef enum {
	PS_BEGIN = 0,	/* Initial */
	PS_HDR,		/* Header directive */
	PS_HDR_SEP,
	PS_HDR_PARAM,
	PS_COMMENT,
	PS_GOLDEN_IDX,
	PS_GOLDEN_SEP,
	PS_GOLDEN_FILE,
	PS_INVALID,
} npu_golden_set_parsing_state;

struct npu_golden_data {
	struct {
		u8	frame_id_matching;
		u8	net_id_matching;
		u8	valid;
		u8	updated;
	} flags;
	struct {
		u32	frame_id;
		u32	net_id;
	} match;
	golden_comparator		comparator;
	golden_is_av_valid		is_av_valid;

	npu_golden_set_parsing_state	parse_state;
	struct parse_buf		parse_buf;
	struct {
		u32			line;
		u32			col;
	} parse_pos;

	struct list_head	file_list;
};

struct npu_golden_ctx {
	/* Flag to check golden is available or not, without acquiring lock */
	atomic_t				loaded;

	struct npu_statekeeper			statekeeper;
	struct mutex				lock;
	struct npu_golden_data			data;
	struct npu_golden_compare_result	result;
	struct device *dev;
};

struct parsing_action {
	npu_golden_set_parsing_state	next_state;
	action_func			func;
};

/* Comparator function configuration map */
struct {
	const char *name;
	golden_comparator	comparator;
	golden_is_av_valid	is_av_valid;
} COMPARATOR_TABLE[] = {
	{	/* SIMPLE: Default */
		.name = "SIMPLE",
		.comparator = simple_compare,
		.is_av_valid = is_av_valid_simple,
	},
	{	/* REORDERED */
		.name = "REORDERED",
		.comparator = reordered_compare,
		.is_av_valid = is_av_valid_reordered,
	},
	{	/* Terminator */
		.name = NULL,
		.comparator = NULL,
		.is_av_valid = NULL,
	}
};

/* Global storage for golden reference data */
static struct npu_golden_ctx	npu_golden_ctx;

/*********************************************************
 * Golden file structure manipulation
 */
static struct golden_file *new_golden_file(void)
{
	struct golden_file *obj;

	BUG_ON(!npu_golden_ctx.dev);

	obj = devm_kzalloc(npu_golden_ctx.dev, sizeof(*obj), GFP_KERNEL);
	if (obj == NULL) {
		npu_err("fail in golden_file memory allocattion");
		return NULL;
	}
	/* new golden_file object is zero-cleared, no memset required */
	return obj;
}

static void destroy_golden_file(struct golden_file *obj)
{
	BUG_ON(!obj);
	BUG_ON(!npu_golden_ctx.dev);

	if (obj->img_vbuf) {
		vfree(obj->img_vbuf);
	}
	devm_kfree(npu_golden_ctx.dev, obj);
}

static int load_golden_file(struct golden_file *obj)
{

	int ret;
	mm_segment_t old_fs = 0;
	struct file *fp = NULL;
	struct kstat st;
	int nread;

	loff_t file_offset = 0;

	BUG_ON(!obj);
	BUG_ON(!npu_golden_ctx.dev);

	obj->img_vbuf = NULL;

	if (obj->file_name[0] == '\0') {
		npu_err("no file name\n");
		ret = -EINVAL;
		goto err_exit;
	}

	npu_info("start in load_golden_file %s\n", obj->file_name);

	/* Switch to Kernel data segmenet
	   Ref: https://wiki.kldp.org/wiki.php/%C4%BF%B3%CE%B3%BB%BF%A1%BC%ADReadWrite%BB%E7%BF%EB%C7%CF%B1%E2
	 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* Step 1. Determine file size */
	fp = filp_open(obj->file_name, O_RDONLY, 0);
	if (!fp) {
		npu_err("fail in filp_open %s.\n", obj->file_name);
		ret = -EBADFD;
		goto err_exit;
	} else if (IS_ERR(fp)) {
		npu_err("fail in filp_open(%s): errno = %ld\n",
			obj->file_name, PTR_ERR(fp));
		fp = NULL;	/* No need to close fp on err_exit */
		ret = -ENOENT;
		goto err_exit;
	}

	/* Get file attributes */
	//[MOD]
	ret = vfs_getattr(&fp->f_path, &st, STATX_MODE | STATX_TYPE, AT_STATX_SYNC_AS_STAT);
	if (ret) {
		npu_err("error(%d) in vfs_getattr\n", ret);
		goto err_exit;
	}
	/* Checke whether it is regular file */
	if (!S_ISREG(st.mode))	{
		npu_err("fail in regular file.\n");
		ret = -ENOENT;
		goto err_exit;
	}

	obj->size = st.size;
	if (obj->size <= 0) {
		npu_err("fail in size check: %s, size=%lu.\n",
			obj->file_name, obj->size);
		ret = -ENOENT;
		goto err_exit;
	}

	/* Step 2. Allocate memory */
	obj->img_vbuf = vzalloc(obj->size);
	if (!obj->img_vbuf) {
		npu_err("fail in memory allocation: %s, size=%lu.\n",
			obj->file_name, obj->size);
		ret = -ENOMEM;
		goto err_exit;
	}

	/* Step 3. Read file contents and save to the allocated buffer */
	nread = kernel_read(fp, obj->img_vbuf, obj->size, &file_offset);
	if (nread != obj->size) {
		npu_err("fail(%d) in kernel_read", nread);
		ret = -EFAULT;
		goto err_exit;
	}

	ret = 0;
	goto ok_exit;

err_exit:
	/* Step 4. Clean-up */

	/* if an error occurs, image_buf buffer should be de-allocated */
	if (obj->img_vbuf)
		vfree(obj->img_vbuf);

	obj->img_vbuf = NULL;
	obj->size = 0;

ok_exit:
	if (fp)
		filp_close(fp, NULL);

	/* Second parameter of filp_close is NULL
	   Ref: https://stackoverflow.com/questions/32442521/
		why-can-the-posix-thread-id-be-null-in-linux-kernel-function-filp-close
	*/
	set_fs(old_fs);

	switch (ret) {
	case 0:
		npu_info("success in load_golden_file: (%s, %luB length)\n",
			 obj->file_name, obj->size);
		npu_dbg("loaded file on (%pK)\n", obj->img_vbuf);
		break;
	default:
		npu_err("fail(%d) in load_golden_file %s\n",
			ret, obj->file_name);
		break;
	}
	return ret;
}


static void destroy_npu_golden(void)
{
	struct golden_file *cur;
	struct golden_file *tmp;
	int cnt = 0;

	npu_statekeeper_transition(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_NOT_INITIALIZED);

	if (npu_golden_ctx.data.flags.valid == 0) {
		npu_warn("not initialized in npu_golden.data\n");
		return;
	}
	list_for_each_entry_safe(cur, tmp, &npu_golden_ctx.data.file_list, list) {
		list_del_init(&cur->list);
		destroy_golden_file(cur);
		cnt++;
	}
	npu_dbg("%s: removed %d file entries.\n", __func__, cnt);

	atomic_set(&npu_golden_ctx.loaded, 0);
	npu_golden_ctx.data.flags.valid = 0;
}
/*
 * Open :
 *  Create a new golden data
 */
static int npu_golden_set_open(struct inode *inode, struct file *file)
{
	BUG_ON(!file);
	mutex_lock(&npu_golden_ctx.lock);

	npu_info("start in npu_golden_set_open, npu_golden_ctx = @%pK\n", &npu_golden_ctx);

	/* Destroy last golden setting */
	if (npu_golden_ctx.data.flags.valid) {
		npu_dbg("open for npu_golden_set: Purge existing golden data....\n");
		destroy_npu_golden();
	}
	/* Clear old data */
	memset(&npu_golden_ctx.data, 0, sizeof(npu_golden_ctx.data));

	/* Set initial value for non-zero value */
	npu_golden_ctx.data.parse_state = PS_BEGIN;
	npu_golden_ctx.data.parse_pos.line = 1;
	npu_golden_ctx.data.parse_pos.col = 1;
	npu_golden_ctx.data.flags.valid = 1;
	npu_golden_ctx.data.flags.updated = 0;
	npu_golden_ctx.data.comparator = COMPARATOR_TABLE[0].comparator;	/* Default comparator */
	npu_golden_ctx.data.is_av_valid = COMPARATOR_TABLE[0].is_av_valid;	/* Default comparator */
	INIT_LIST_HEAD(&npu_golden_ctx.data.file_list);

	npu_statekeeper_transition(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_OPENED);

	npu_info("complete in npu_golden_set_open\n");
	mutex_unlock(&npu_golden_ctx.lock);
	return 0;
}

/*
 * Close : Wrap up the golden data and transition to
 *         comparing ready state
 */
static int npu_golden_set_close(struct inode *inode, struct file *file)
{
	int ret;
	struct golden_file *f;

	mutex_lock(&npu_golden_ctx.lock);

	npu_info("start in npu_golden_set_close\n");
	BUG_ON(!file);
	if (npu_golden_ctx.data.flags.valid == 0) {
		npu_err("not initialized in npu_golden_ctx.data\n");
		ret = -EBADFD;
		goto err_exit;
	}
	if (npu_golden_ctx.data.flags.updated == 0) {
		// No update after open
		npu_err("no golden description\n");
		ret = 0;
		goto err_exit;
	}
	if (npu_golden_ctx.data.parse_state != PS_BEGIN) {
		/* Improper parsing */
		npu_err("unexpected EOF in description.\n");
		ret = -EINVAL;
		goto err_exit;
	}

	/* Load golden files on memory */
	list_for_each_entry(f, &npu_golden_ctx.data.file_list, list) {
		ret = load_golden_file(f);
		if (ret) {
			npu_err("fail in load_golden_file on entry(%pK).\n", f);
			ret = -EFAULT;
			goto err_exit;
		}
	}

	/* Set flag to mark the golden is valid */
	atomic_set(&npu_golden_ctx.loaded, 1);

	/* Test data is all prepared */
	npu_statekeeper_transition(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_CONFIGURED);

	npu_info("complete in npu_golden_set_close\n");
	ret = 0;
	goto ok_exit;

err_exit:
	npu_err("error(%d) in npu_golden_set_close]\n", ret);

	destroy_npu_golden();
	npu_golden_ctx.data.flags.valid = 0;
	npu_statekeeper_transition(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_NOT_INITIALIZED);

ok_exit:
	mutex_unlock(&npu_golden_ctx.lock);
	return ret;
}

static npu_golden_set_token get_token_type(const char ch)
{
	switch (ch) {
	case '\n':
		return TOKEN_NL;
	case ' ':
	case '\t':
	case '\r':
		return TOKEN_WS;
	case '#':
		return TOKEN_SHARP;
	case '_':
	case '-':
	case '/':
	case '.':
		/* Special character allowed for alphabet */
		return TOKEN_ALPHA;
	}

	/* Range compare */
	if (('a' <= ch && ch <= 'z')
	    || ('A' <= ch && ch <= 'Z')) {
		return TOKEN_ALPHA;
	}

	if ('0' <= ch && ch <= '9')	{
		return TOKEN_NUMBER;
	}

	return TOKEN_INVALID;
}

/* Remove trailing whitespcae from parse_buf */
static void trim_and_term_parse_buf(struct parse_buf *parse_buf)
{
	while (parse_buf->idx > 0) {
		if (get_token_type(parse_buf->token_char[parse_buf->idx - 1]) != TOKEN_WS) {
			break;
		}
		parse_buf->idx--;
	}
	parse_buf->token_char[parse_buf->idx] = '\0';
}

static void to_upper_parse_buf(struct parse_buf *parse_buf)
{
	size_t i;

	for (i = 0; i < parse_buf->idx; ++i) {
		parse_buf->token_char[i] = toupper(parse_buf->token_char[i]);
	}
}

static inline void reset_parse_buf(struct parse_buf *parse_buf)
{
	parse_buf->idx = 0;
	parse_buf->token_char[0] = '\0';
}

/***************************************************************************
 * Value setter funtion
 */
static int set_frame_id(struct parse_buf *parse_buf)
{
	int ret;
	long long frame_id;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);
	if (parse_buf->idx <= 0)	{
		return -1;
	}

	ret = kstrtoll(parse_buf->token_char, 10, &frame_id);
	if (ret) {
		npu_err("invalid frame id (%s) specified on line (%d)\n",
			parse_buf->token_char,
			npu_golden_ctx.data.parse_pos.line);
		return ret;
	}

	npu_golden_ctx.data.flags.frame_id_matching = 1;
	npu_golden_ctx.data.match.frame_id = frame_id;

	npu_info("frame ID matching set: Frame ID = (%u)\n", npu_golden_ctx.data.match.frame_id);
	return 0;
}
static int set_net_id(struct parse_buf *parse_buf)
{
	int ret;
	long long net_id;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);
	if (parse_buf->idx <= 0)	{
		return -1;
	}

	ret = kstrtoll(parse_buf->token_char, 10, &net_id);
	if (ret) {
		npu_err("invalid net id [%s] specified on line %d\n",
			parse_buf->token_char,
			npu_golden_ctx.data.parse_pos.line);
		return ret;
	}

	npu_golden_ctx.data.flags.net_id_matching = 1;
	npu_golden_ctx.data.match.net_id = net_id;

	npu_info("net ID matching set: Net ID = (%u)\n", npu_golden_ctx.data.match.net_id);
	return 0;
}

static int set_comparator(struct parse_buf *parse_buf)
{
	int i;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);
	to_upper_parse_buf(parse_buf);
	if (parse_buf->idx <= 0)	{
		return -1;
	}

	/* Find header value */
	for (i = 0; COMPARATOR_TABLE[i].name != NULL; i++) {
		if (strncmp(parse_buf->token_char, COMPARATOR_TABLE[i].name, parse_buf->idx) == 0) {
			npu_info("setting comparator to (%s)\n", COMPARATOR_TABLE[i].name);
			npu_golden_ctx.data.comparator = COMPARATOR_TABLE[i].comparator;
			npu_golden_ctx.data.is_av_valid = COMPARATOR_TABLE[i].is_av_valid;
			goto find_header;
		}
	}
	/* Could not find appropirate header */
	npu_err("invalid comparator (%s) found on line (%d)\n", parse_buf->token_char, npu_golden_ctx.data.parse_pos.line);
	return -1;
find_header:
	return 0;
}

struct {
	const char *header;
	int (*value_setter)(struct parse_buf *);
} HDR_PARSE_TABLE[] = {
	{.header = "FRAME_ID",		.value_setter = set_frame_id},
	{.header = "NET_ID",		.value_setter = set_net_id},
	{.header = "COMPARATOR",	.value_setter = set_comparator},
	{.header = NULL,		.value_setter = NULL},
};

/***************************************************************************
 * Parse action funtion
 */
/* Do nothing but consume the input character */
static int skip(const char ch, struct parse_buf *parse_buf)
{
	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	return 0;
}
/* Keep the character in parse_buf */
static int append(const char ch, struct parse_buf *parse_buf)
{
	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	if (parse_buf->idx + 1 >= GOLDEN_FILENAME_LEN) {
		return -1;
	} else {
		parse_buf->token_char[parse_buf->idx] = ch;
		parse_buf->idx++;
	}
	return 0;
}
static int set_hdr_name(const char ch, struct parse_buf *parse_buf)
{
	int i;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);
	to_upper_parse_buf(parse_buf);
	if (parse_buf->idx <= 0)	{
		return -1;
	}

	/* Find header value */
	for (i = 0; HDR_PARSE_TABLE[i].header != NULL; i++) {
		if (strncmp(parse_buf->token_char, HDR_PARSE_TABLE[i].header, parse_buf->idx) == 0) {
			parse_buf->value_setter = HDR_PARSE_TABLE[i].value_setter;
			goto find_header;
		}
	}
	/* Could not find appropirate header */
	npu_err("invalid header (%s) found on line (%d)\n", parse_buf->token_char, npu_golden_ctx.data.parse_pos.line);
	return -1;
find_header:
	/* reset parse_buf */
	reset_parse_buf(parse_buf);

	return 0;
}
static int set_hdr_param(const char ch, struct parse_buf *parse_buf)
{
	int ret;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);

	if (parse_buf->value_setter == NULL) {
		npu_err("no value setter on line (%d)\n", npu_golden_ctx.data.parse_pos.line);
		return -1;
	}
	ret = parse_buf->value_setter(parse_buf);
	if (ret) {
		npu_err("value setter failed on line (%d)\n", npu_golden_ctx.data.parse_pos.line);
		return ret;
	}
	parse_buf->value_setter = NULL;

	/* reset parse_buf */
	reset_parse_buf(parse_buf);

	return 0;
}
static int set_golden_idx(const char ch, struct parse_buf *parse_buf)
{
	int ret;
	long long golden_idx;
	struct golden_file *golden_file;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);
	if (parse_buf->idx <= 0)	{
		return -1;
	}

	ret = kstrtoll(parse_buf->token_char, 10, &golden_idx);
	if (ret) {
		npu_err("invalid vector index (%s) specified on line %d\n",
			parse_buf->token_char,
			npu_golden_ctx.data.parse_pos.line);
		return ret;
	}

	/* Create a new file entry */
	golden_file = new_golden_file();
	if (!golden_file) {
		npu_err("fail in new_golden_file\n");
		return -ENOMEM;
	}
	golden_file->av_index = golden_idx;
	list_add_tail(&golden_file->list, &npu_golden_ctx.data.file_list);

	npu_dbg("adding new_golden_file entry: IDX(%d), Ptr(%pK)\n",
		golden_file->av_index, golden_file);

	/* reset parse_buf */
	reset_parse_buf(parse_buf);

	return 0;
}
static int set_golden_file(const char ch, struct parse_buf *parse_buf)
{
	struct golden_file *golden_file;

	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	trim_and_term_parse_buf(parse_buf);

	/* Retrive last entry from the list -> Added on last set_golden_idx() */
	if (list_empty(&npu_golden_ctx.data.file_list)) {
		/* Somethings wrong */
		npu_err("no file list entry on line (%d)\n", npu_golden_ctx.data.parse_pos.line);
		return -1;
	}
	golden_file = list_last_entry(&npu_golden_ctx.data.file_list, struct golden_file, list);
	BUG_ON(!golden_file);

	strncpy(golden_file->file_name, parse_buf->token_char, parse_buf->idx);
	golden_file->file_name[parse_buf->idx] = '\0';

	npu_dbg("golden file entry set: IDX(%d), filename(%s), Ptr(%pK)\n",
		golden_file->av_index, golden_file->file_name, golden_file);

	/* reset parse_buf */
	reset_parse_buf(parse_buf);

	return 0;
}
static int reset(const char ch, struct parse_buf *parse_buf)
{
	BUG_ON(!parse_buf);
	SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);
	/* reset parse_buf */
	reset_parse_buf(parse_buf);

	return 0;
}


static const struct parsing_action PARSE_TABLE[][TOKEN_INVALID+1] = {
/*PS_BEGIN*/ {
	/*TOKEN_ALPHA,*/	{PS_HDR, append},
	/*TOKEN_NUMBER,*/	{PS_GOLDEN_IDX, append},
	/*TOKEN_WS,*/		{PS_BEGIN, skip},
	/*TOKEN_NL,*/		{PS_BEGIN, skip},
	/*TOKEN_SHARP,*/	{PS_COMMENT, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
/*PS_HDR,*/ {
	/*TOKEN_ALPHA,*/	{PS_HDR, append},
	/*TOKEN_NUMBER,*/	{PS_HDR, append},
	/*TOKEN_WS,*/		{PS_HDR_SEP, set_hdr_name},
	/*TOKEN_NL,*/		{PS_INVALID, skip},
	/*TOKEN_SHARP,*/	{PS_INVALID, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
/*PS_HDR_SEP,*/ {
	/*TOKEN_ALPHA,*/	{PS_HDR_PARAM, append},
	/*TOKEN_NUMBER,*/	{PS_HDR_PARAM, append},
	/*TOKEN_WS,*/		{PS_HDR_SEP, skip},
	/*TOKEN_NL,*/		{PS_INVALID, skip},
	/*TOKEN_SHARP,*/	{PS_INVALID, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
/*PS_HDR_PARAM,*/ {
	/*TOKEN_ALPHA,*/	{PS_HDR_PARAM, append},
	/*TOKEN_NUMBER,*/	{PS_HDR_PARAM, append},
	/*TOKEN_WS,*/		{PS_HDR_PARAM, append},
	/*TOKEN_NL,*/		{PS_BEGIN, set_hdr_param},
	/*TOKEN_SHARP,*/	{PS_INVALID, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
/*PS_COMMENT,*/ {
	/*TOKEN_ALPHA,*/	{PS_COMMENT, skip},
	/*TOKEN_NUMBER,*/	{PS_COMMENT, skip},
	/*TOKEN_WS,*/		{PS_COMMENT, skip},
	/*TOKEN_NL,*/		{PS_BEGIN, reset},
	/*TOKEN_SHARP,*/	{PS_COMMENT, skip},
	/*TOKEN_INVALID,*/	{PS_COMMENT, skip},
},
/*PS_GOLDEN_IDX,*/ {
	/*TOKEN_ALPHA,*/	{PS_INVALID, skip},
	/*TOKEN_NUMBER,*/	{PS_GOLDEN_IDX, append},
	/*TOKEN_WS,*/		{PS_GOLDEN_SEP, set_golden_idx},
	/*TOKEN_NL,*/		{PS_INVALID, skip},
	/*TOKEN_SHARP,*/	{PS_INVALID, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
/*PS_GOLDEN_SEP,*/  {
	/*TOKEN_ALPHA,*/	{PS_GOLDEN_FILE, append},
	/*TOKEN_NUMBER,*/	{PS_GOLDEN_FILE, append},
	/*TOKEN_WS,*/		{PS_GOLDEN_SEP, skip},
	/*TOKEN_NL,*/		{PS_INVALID, skip},
	/*TOKEN_SHARP,*/	{PS_INVALID, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
/*PS_GOLDEN_FILE*/  {
	/*TOKEN_ALPHA,*/	{PS_GOLDEN_FILE, append},
	/*TOKEN_NUMBER,*/	{PS_GOLDEN_FILE, append},
	/*TOKEN_WS,*/		{PS_GOLDEN_FILE, append},
	/*TOKEN_NL,*/		{PS_BEGIN, set_golden_file},
	/*TOKEN_SHARP,*/	{PS_INVALID, skip},
	/*TOKEN_INVALID,*/	{PS_INVALID, skip},
},
};

int parse_char(const char ch)
{
	npu_golden_set_token token_type;
	npu_golden_set_parsing_state cur_state = npu_golden_ctx.data.parse_state;
	const struct parsing_action *cur_action = NULL;
	struct parse_buf *parse_buf = &npu_golden_ctx.data.parse_buf;

	token_type = get_token_type(ch);
	if (token_type == TOKEN_INVALID) {
		/* Invalid character */
		npu_err("invalid token at line:%u, col:%u\n",
			npu_golden_ctx.data.parse_pos.line,
			npu_golden_ctx.data.parse_pos.col);
		return -EINVAL;
	}
	cur_action = &(PARSE_TABLE[cur_state][token_type]);

	/* Do action and state transition */
	if (cur_action->func) {
		if (cur_action->func(ch, parse_buf) != 0) {
			/* Invalid character */
			npu_err("parsing error at line:%u, col:%u\n",
				npu_golden_ctx.data.parse_pos.line,
				npu_golden_ctx.data.parse_pos.col);
			return -EINVAL;
		}
	}
	npu_golden_ctx.data.parse_state = cur_action->next_state;
	if (cur_action->next_state >= PS_INVALID) {
		npu_err("parsing error at line:%u, col:%u\n",
			npu_golden_ctx.data.parse_pos.line,
			npu_golden_ctx.data.parse_pos.col);
		return -EINVAL;
	}

	/* Adjust col and line */
	if (token_type == TOKEN_NL) {
		npu_golden_ctx.data.parse_pos.line++;
		npu_golden_ctx.data.parse_pos.col = 0;
	} else {
		npu_golden_ctx.data.parse_pos.col++;
	}
	return 0;
}

/*
 * Write : Set golden definition
 */
static ssize_t npu_golden_set_write(struct file *file, const char * __user userbuf, size_t count, loff_t *offp)
{
	char ch;
	size_t i;
	int ret;

	mutex_lock(&npu_golden_ctx.lock);
	npu_statekeeper_transition(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);

	for (i = 0; i < count; i++) {
		get_user(ch, userbuf+i);
		ret = parse_char(ch);
		if (ret) {
			goto err_exit;
		}
	}

	ret = i;
	npu_golden_ctx.data.flags.updated = 1;

err_exit:
	mutex_unlock(&npu_golden_ctx.lock);
	return ret;
}

/* Same implementation but buf is kernel buffer - For IDIOT testing */
__attribute__((unused)) static ssize_t npu_golden_set_write_kern(struct file *file, const char *buf, size_t count, loff_t *offp)
{
	char ch;
	size_t i;
	int ret;

	mutex_lock(&npu_golden_ctx.lock);
	npu_statekeeper_transition(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_ADDING_GOLDEN_FILES);

	for (i = 0; i < count; i++) {
		ch = buf[i];
		ret = parse_char(ch);
		if (ret) {
			goto err_exit;
		}
	}

	ret = i;
	npu_golden_ctx.data.flags.updated = 1;

err_exit:
	mutex_unlock(&npu_golden_ctx.lock);
	return ret;
}

static const struct file_operations npu_golden_set_debug_fops = {
	.owner = THIS_MODULE,
	.open = npu_golden_set_open,
	.release = npu_golden_set_close,
	.write = npu_golden_set_write,
};

/*
 * Open: Check readiness of compare
 */
static int npu_golden_match_open(struct inode *inode, struct file *file)
{
	int	ret;
	BUG_ON(!file);
	BUG_ON(!npu_golden_ctx.dev);

	mutex_lock(&npu_golden_ctx.lock);
	npu_info("start in npu_golden_match_open\n");
	if (!SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_CONFIGURED)
	   && !SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_COMPARING)) {
		npu_err("not configured in npu_golden_ctx\n");
		ret = -EBADFD;
		goto err_exit;
	}

	if (!atomic_read(&npu_golden_ctx.loaded)) {
		npu_err("not loaded in npu_golden_ctx\n");
		ret = -EBADFD;
		goto err_exit;
	}
	ret = 0;

err_exit:
	mutex_unlock(&npu_golden_ctx.lock);
	npu_info("complete in npu_golden_match_open\n");
	return ret;
}

/*
 * Close : Reset the copied_len to get same result on next read.
 */
static int npu_golden_match_close(struct inode *inode, struct file *file)
{
	mutex_lock(&npu_golden_ctx.lock);
	npu_golden_ctx.result.copied_len = 0;
	mutex_unlock(&npu_golden_ctx.lock);
	return 0;
}

/*
 * Provide contents of golden matching
 */
static ssize_t npu_golden_match_read(struct file *file, char __user *outbuf, size_t outlen, loff_t *loff)
{
	struct npu_golden_compare_result *result = &npu_golden_ctx.result;
	size_t copy_len;
	int ret;

	mutex_lock(&npu_golden_ctx.lock);
	if (result->out_buf_cur == 0) {
		/* No data available */
		npu_dbg("No golden matching result available.");
		copy_len = 0;
		goto err_exit;
	}
	if (result->copied_len >= result->out_buf_cur) {
		/* Otherwise, EOF */
		copy_len = 0;
		goto err_exit;
	}

	copy_len = min(outlen, result->out_buf_cur - result->copied_len);
	ret = copy_to_user(outbuf, result->out_buf + result->copied_len, copy_len);
	if (ret) {
		npu_err("fail in copy_to_user: %d bytes are not copied\n", ret);
	}
	copy_len -= ret;
	result->copied_len += copy_len;

err_exit:
	mutex_unlock(&npu_golden_ctx.lock);
	return copy_len;
}

static void reset_compare_result(struct npu_golden_compare_result *result)
{
	BUG_ON(!result);
	result->copied_len = 0;
	result->out_buf_cur = 0;
	result->out_buf_len = NPU_GOLDEN_RESULT_BUF_LEN;
	result->out_buf[0] = '\0';
}



/*
 * Compare Golden data with address vector
 */
static int __npu_golden_compare(const struct npu_frame *frame)
{
	struct npu_golden_compare_result *result;
	struct npu_session *session;
	struct golden_file *golden_entry;
	struct addr_info *target_entry;
	struct ncp_header *ncp_header;
	struct memory_vector *memory_vector;

	struct npu_fmap_dim	dim;
	u32			av_len;
	int			compare_result = 0;
	int			total_result = 0;
	int			ret;
	int			tr_msg_len;
	size_t			byte_size, dim_size;

	BUG_ON(!frame);
	BUG_ON(!frame->session);

	if (!SKEEPER_EXPECT_STATE(&npu_golden_ctx.statekeeper, NPU_GOLDEN_STATE_CONFIGURED)) {
		npu_dbg("invalid state.");
		return -EINVAL;
	}

	result = &npu_golden_ctx.result;
	session = frame->session;
	ncp_header = (struct ncp_header *)(session->ncp_info.ncp_addr.vaddr);

	npu_dbg("golden compare: starting with frame @ %pK, result at %pK\n [cur=%zu len=%zu cop=%zu]",
		 frame, result->out_buf, result->out_buf_cur, result->out_buf_len, result->copied_len);

	if (!ncp_header) {
		npu_uferr("not initialized in NCP header\n", frame);
		return -EINVAL;
	}
	if (ncp_header->magic_number1 != NCP_MAGIC1) {
		npu_uferr("NCP header magic mismatch: expected(0x%08x), actual(0x%08x).\n",
			frame, NCP_MAGIC1, ncp_header->magic_number1);
		return -EINVAL;
	}

	/* Check comparing condition */
	if (npu_golden_ctx.data.flags.frame_id_matching) {
		if (frame->frame_id != npu_golden_ctx.data.match.frame_id) {
			npu_ufdbg("frame ID [frame:%u != Golden:%u] mismatch. skipping.\n",
				frame, frame->frame_id, npu_golden_ctx.data.match.frame_id);
			return 0;
		}
	}
	if (npu_golden_ctx.data.flags.net_id_matching) {
		if (ncp_header->net_id != npu_golden_ctx.data.match.net_id) {
			npu_ufdbg("net ID [fFrame:%u != golden:%u] mismatch. skipping.\n",
				frame, ncp_header->net_id, npu_golden_ctx.data.match.net_id);
			return 0;
		}
	}

	/* Reset comparing buffer */
	reset_compare_result(&npu_golden_ctx.result);

	/* Retrieve address vector information */
	av_len = session->ncp_info.address_vector_cnt;

	if (av_len <= 0) {
		npu_uferr("Address vector table length [%u] is not properly initialized.\n", frame, av_len);
		return -EINVAL;
	}

	/* Compare each vector */
	list_for_each_entry(golden_entry, &npu_golden_ctx.data.file_list, list) {
		compare_result = 0;

		if (golden_entry->av_index >= av_len) {
			add_compare_msg(&npu_golden_ctx.result,
					"Golden idx[%u] exceedes total number of address vector[%u]. ",
					golden_entry->av_index, av_len);
			compare_result++;
			goto end_compare;
		}

		target_entry = find_addr_info(session, golden_entry->av_index, OFM_TYPE, frame->input->index);
		if (!target_entry) {
			target_entry = find_addr_info(session, golden_entry->av_index, IMB_TYPE, frame->input->index);
		}
		if (!target_entry) {
			add_compare_msg(&npu_golden_ctx.result,
					"No matching address vector found for idx[%u]. ",
					golden_entry->av_index);
			compare_result++;
			goto end_compare;
		}

		/* Size compare */
		if (!npu_golden_ctx.data.is_av_valid(golden_entry->size, target_entry->size, result)) {
			compare_result++;
			goto end_compare;
		}

		/* Setting featuremap dimension */
		memory_vector = (struct memory_vector *)((char *) ncp_header + ncp_header->memory_vector_offset);

		dim.axis1 = (size_t)((memory_vector + golden_entry->av_index)->channels);
		dim.axis2 = (size_t)((memory_vector + golden_entry->av_index)->height);
		dim.axis3 = (size_t)((memory_vector + golden_entry->av_index)->width);
		byte_size = (golden_entry->size < target_entry->size) ? golden_entry->size : target_entry->size;
		dim_size = dim.axis1 * dim.axis2 * dim.axis3;

		if (dim_size == 0 || dim_size != byte_size) {
			npu_ufwarn("Axis on MV(%lu x %lu x %lu) does not match with size[%lu] -> Use 1D comparison\n",
				frame, dim.axis1, dim.axis2, dim.axis3, byte_size);
			dim.axis1 = dim.axis2 = 1;
			dim.axis3 = byte_size;
		}

		npu_ufdbg("av_index(%u), dim.axis1(%zu), dim.axis2(%zu), dim.axis3(%zu)\n",
			frame, golden_entry->av_index, dim.axis1, dim.axis2, dim.axis3);

		add_compare_msg(&npu_golden_ctx.result, "%u: [%lu x %lu x %lu]:",
				golden_entry->av_index,
				dim.axis1, dim.axis2, dim.axis3);

		/* Buffer pointer validation */
		if (!target_entry->vaddr) {
			add_compare_msg(&npu_golden_ctx.result,
					"KV addr of address vector golden idx[%u] is not valid. ",
					golden_entry->av_index);
			compare_result++;
			goto end_compare;
		}
		if (!golden_entry->img_vbuf) {
			add_compare_msg(&npu_golden_ctx.result,
					"Image data of golden idx[%u] is not valid. ",
					golden_entry->av_index);
			compare_result++;
			goto end_compare;
		}
		/* Cache invalidation */
		__dma_map_area(target_entry->vaddr, target_entry->size, DMA_FROM_DEVICE);

		npu_ufdbg("golden compare Golden[%pK:%zuB] / Target[%pK:%zuB]\n",
			frame, golden_entry->img_vbuf, golden_entry->size,
			target_entry->vaddr, target_entry->size);
		/* Finally, comparing contents */
		if (npu_golden_ctx.data.comparator) {
			ret = npu_golden_ctx.data.comparator(0,
					golden_entry->img_vbuf, target_entry->vaddr,
					dim, result);
			compare_result += ret;
			npu_ufdbg("result of comparator for golden idx(%u): (%d)\n",
				frame, golden_entry->av_index, ret);

		} else {
			npu_ufdbg("no comparator defined for golden idx(%u). skipping data match.\n",
				frame, golden_entry->av_index);
		}

end_compare:
		if (compare_result > 0) {
			add_compare_msg(&npu_golden_ctx.result,
					" MISMATCH(%d)\n", compare_result);
		} else {
			add_compare_msg(&npu_golden_ctx.result, " OK\n");
		}

		total_result += compare_result;
	}

	/* If the output buffer is full, left an message at the end of the buffer */
	if (result->out_buf_cur == (result->out_buf_len - 1)) {
		npu_ufdbg("full in output buffer, truncate the result.\n", frame);
		/* Notify that the buffer is truncated */
		tr_msg_len = strlen(TRUNCATE_MSG);
		strncpy(&result->out_buf[NPU_GOLDEN_RESULT_BUF_LEN - tr_msg_len],
			TRUNCATE_MSG, tr_msg_len);
	}
	return total_result;
}

/*
 * Wrapper to npu_golden_compare for locking
 */
int npu_golden_compare(const struct npu_frame *frame)
{
	int ret;

	if (!atomic_read(&npu_golden_ctx.loaded)) {
		npu_uftrace("Golden is not configured.\n", frame);
		return 0;
	}

	mutex_lock(&npu_golden_ctx.lock);
	ret = __npu_golden_compare(frame);
	mutex_unlock(&npu_golden_ctx.lock);

	return ret;
}

static const struct file_operations npu_golden_match_debug_fops = {
	.owner = THIS_MODULE,
	.open = npu_golden_match_open,
	.release = npu_golden_match_close,
	.read = npu_golden_match_read,
};

#define SET_GOLDEN_DESC_DEBUGFS_NAME		"set-golden-desc"
#define RESULT_GOLDEN_MATCH_DEBUGFS_NAME	"result-golden-match"

int register_golden_matcher(struct device *dev)
{
	int ret;

	npu_info("start in register_golden_matcher\n");

	BUG_ON(!dev);

	npu_dbg("initialize statekeeper in register_golden_matcher\n");
	npu_statekeeper_initialize(&npu_golden_ctx.statekeeper,
				   NPU_GOLDEN_STATE_INVALID,
				   state_names, transition_map);

	npu_info("Registeration of golden matcher : Register " SET_GOLDEN_DESC_DEBUGFS_NAME " to debugfs.\n");
	ret = npu_debug_register(SET_GOLDEN_DESC_DEBUGFS_NAME, &npu_golden_set_debug_fops);
	if (ret) {
		npu_err("fail(%d) in npu_debug_register for " SET_GOLDEN_DESC_DEBUGFS_NAME "\n", ret);
		goto err_exit;
	}

	npu_info("Registeration of golden matcher: Register " RESULT_GOLDEN_MATCH_DEBUGFS_NAME " to debugfs.\n");
	ret = npu_debug_register(RESULT_GOLDEN_MATCH_DEBUGFS_NAME, &npu_golden_match_debug_fops);
	if (ret) {
		npu_err("fail(%d) in npu_debug_register for " RESULT_GOLDEN_MATCH_DEBUGFS_NAME "\n", ret);
		goto err_exit;
	}

	/* Initialize lock */
	atomic_set(&npu_golden_ctx.loaded, 0);
	mutex_init(&npu_golden_ctx.lock);

	npu_golden_ctx.dev = dev;
	npu_info("complete in register_golden_matcher\n");
	return 0;

err_exit:
	npu_err("fail(%d) in register_golden_matcher\n", ret);
	return ret;
}

/* Unit test */
#ifdef CONFIG_VISION_UNITTEST
#define IDIOT_TESTCASE_IMPL "npu-golden.idiot"
#include "idiot-def.h"
#endif
