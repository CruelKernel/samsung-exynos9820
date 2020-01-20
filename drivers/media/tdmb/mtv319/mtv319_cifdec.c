/*
 *
 * File name: mtv319_cifdec.c
 *
 * Description : MTV319 CIF decoder source file for T-DMB and DAB.
 *
 * Copyright (C) (2014, RAONTECH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/******************************************************************************
 * REVISION HISTORY
 *
 *    DATE         NAME          REMARKS
 * ----------  -------------    ------------------------------------------------
 * 07/12/2012  Ko, Kevin        Created.
 ******************************************************************************/

#include "mtv319_cifdec.h"


/*============================================================================
 * Defines the number of CIF buffer pool.
 */
#define MAX_NUM_INPUT_TSP	(16)

/*============================================================================
 * Select the debug options.
 */
#define _DEBUG_MSG_ENABLE
#define _DEBUG_ASSERT_ENABLE
/* #define _DEBUG_CIFDEC_ERR_STATIC */
#define _DEBUG_HEADER_DECODING

#define MAX_NUM_SUBCH		64

/* Used sub channel ID bits. [0]: 0 ~ 31, [1]: 32 ~ 63 */
static U32 g_aAddedSubChIdBits[2];

#ifdef _DEBUG_MSG_ENABLE
	#define CIF_DBGMSG0(fmt)		printk(fmt)
	#define CIF_DBGMSG1(fmt, arg1)		printk(fmt, arg1)
	#define CIF_DBGMSG2(fmt, arg1, arg2)	printk(fmt, arg1, arg2)
	#define CIF_DBGMSG3(fmt, arg1, arg2, arg3) printk(fmt, arg1, arg2, arg3)
	#define CIF_DBGMSG4(fmt, arg1, arg2, arg3, arg4)\
					printk(fmt, arg1, arg2, arg3, arg4)
#else
	/* To eliminates the debug messages. */
	#define CIF_DBGMSG0(fmt)			((void)0)
	#define CIF_DBGMSG1(fmt, arg1)			((void)0)
	#define CIF_DBGMSG2(fmt, arg1, arg2)		((void)0)
	#define CIF_DBGMSG3(fmt, arg1, arg2, arg3)	((void)0)
	#define CIF_DBGMSG4(fmt, arg1, arg2, arg3, arg4)	((void)0)
#endif

#if defined(__KERNEL__) /* Linux kernel */
  #include <linux/mutex.h>
	static struct mutex cif_mutex;
	#define CIF_MUTEX_INIT		mutex_init(&cif_mutex)
	#define CIF_MUTEX_LOCK		mutex_lock(&cif_mutex)
	#define CIF_MUTEX_UNLOCK	mutex_unlock(&cif_mutex)
	#define CIF_MUTEX_DEINIT	((void)0)

	#ifdef _DEBUG_ASSERT_ENABLE
		#define CIF_ASSERT(expr)	\
		do {				\
			if (!(expr)) {		\
				printk("assert failed %s: %d: %s\n",\
				__FILE__, __LINE__, #expr);	\
				BUG();				\
			}					\
		} while (0)
	#else
		#define CIF_ASSERT(expr)	do {} while (0)
	#endif

#elif !defined(__KERNEL__) && defined(__linux__) /* Linux application */
	#include <pthread.h>
	static pthread_mutex_t cif_mutex;
	#define CIF_MUTEX_INIT		pthread_mutex_init(&cif_mutex)
	#define CIF_MUTEX_LOCK		pthread_mutex_lock(&cif_mutex)
	#define CIF_MUTEX_UNLOCK	pthread_mutex_unlock(&cif_mutex)
	#define CIF_MUTEX_DEINIT	((void)0)

	#define READ_ONCE(x)	x

	#ifdef _DEBUG_ASSERT_ENABLE
		#include <assert.h>
		#define CIF_ASSERT(expr)	assert(expr)
	#else
		#define CIF_ASSERT(expr)	do {} while (0)
	#endif

#elif defined(WINCE) || defined(WINDOWS) || defined(WIN32)
	static CRITICAL_SECTION cif_mutex;
	#define CIF_MUTEX_INIT		InitializeCriticalSection(&cif_mutex)
	#define CIF_MUTEX_LOCK		EnterCriticalSection(&cif_mutex)
	#define CIF_MUTEX_UNLOCK	LeaveCriticalSection(&cif_mutex)
	#define CIF_MUTEX_DEINIT	DeleteCriticalSection(&cif_mutex)

	#define READ_ONCE(x)	x

	#ifdef _DEBUG_ASSERT_ENABLE
		#ifdef WINCE
			#include <Dbgapi.h>
			#define CIF_ASSERT(expr)	ASSERT(expr)
		#else
			#include <crtdbg.h>
			#define CIF_ASSERT(expr)	_ASSERT(expr)
		#endif
	#else
		#define CIF_ASSERT(expr)	do {} while (0)
	#endif
#else
	#error "Code not present"

	#define READ_ONCE(x)

	/* temp: TODO */
	#define CIF_MUTEX_INIT		((void)0)
	#define CIF_MUTEX_LOCK		((void)0)
	#define CIF_MUTEX_UNLOCK	((void)0)
	#define CIF_MUTEX_DEINIT	((void)0)

	#ifdef _DEBUG_ASSERT_ENABLE
		#include <assert.h>
		#define CIF_ASSERT(expr)	assert(expr)
	#else
		#define CIF_ASSERT(expr)	do {} while (0)
	#endif
#endif

#if defined(__KERNEL__) && defined(RTV_MCHDEC_DIRECT_COPY_USER_SPACE)\
/* Linux kernel */

	/* File dump enable */
	#if 0
	#define _MSC_KERNEL_FILE_DUMP_ENABLE
	#endif

	#ifdef _MSC_KERNEL_FILE_DUMP_ENABLE
		#include <linux/fs.h>

		/* MSC filename: /data/dmb_msc_SUBCH_ID.ts */
		#define MSC_DEC_FNAME_PREFIX	"/data/dmb_msc"

		static struct file *g_ptDecMscFilp[MAX_NUM_SUBCH];

		static UINT g_nKfileDecSubchID;

		static void cif_kfile_write(const char __user *buf, size_t len)
		{
			mm_segment_t oldfs;
			struct file *fp_msc;
			int ret;

			if (g_nKfileDecSubchID != 0xFFFF) {
				fp_msc = g_ptDecMscFilp[g_nKfileDecSubchID];
				oldfs = get_fs();
				set_fs(KERNEL_DS);
				ret = fp_msc->f_op->write(fp_msc, buf,
							len, &fp_msc->f_pos);
				set_fs(oldfs);
			}
		}

		static INLINE void __cif_kfile_close(UINT subch_id)
		{
			if (g_ptDecMscFilp[subch_id]) {
				filp_close(g_ptDecMscFilp[subch_id], NULL);
				g_ptDecMscFilp[subch_id] = NULL;
			}
		}

		static void cif_kfile_all_close(void)
		{
			unsigned int subch_id;
			U32 reged_subch_id_bits;

			subch_id = 0;
			reged_subch_id_bits = g_aAddedSubChIdBits[0];
			while (reged_subch_id_bits) {
				if (reged_subch_id_bits & 0x01)
					__cif_kfile_close(subch_id);

				reged_subch_id_bits >>= 1;
				subch_id++;
			}

			subch_id = 32;
			reged_subch_id_bits = g_aAddedSubChIdBits[1];
			while (reged_subch_id_bits) {
				if (reged_subch_id_bits & 0x01)
					__cif_kfile_close(subch_id);

				reged_subch_id_bits >>= 1;
				subch_id++;
			}
		}

		static void cif_kfile_close(UINT subch_id)
		{
			if (subch_id != 0xFFFF)
				__cif_kfile_close(subch_id);
			else
				cif_kfile_all_close();
		}

		static int cif_kfile_open(UINT subch_id)
		{
			char fname[MAX_NUM_SUBCH];
			struct file *fp_msc;

			if (g_ptDecMscFilp[subch_id] == NULL) {
				sprintf(fname, "%s_%u.ts",
						MSC_DEC_FNAME_PREFIX, subch_id);
				fp_msc = filp_open(fname, O_WRONLY, 0400);
				if (fp_msc == NULL) {
					RTV_DBGMSG("f_err: %s!\n", fname);
					return -1;
				}

				g_ptDecMscFilp[subch_id] = fp_msc;
				RTV_DBGMSG("File opened: %s\n", fname);
			} else
				RTV_DBGMSG("Already opened!\n");

			return 0;
		}

		#define CIF_KFILE_CLOSE(subch_id) cif_kfile_close(subch_id)
		#define CIF_KFILE_OPEN(subch_id)	cif_kfile_open(subch_id)
		#define CIF_KFILE_WRITE(buf, size) cif_kfile_write(buf, size)
		#define CIF_MSC_KFILE_WRITE_DIS { g_nKfileDecSubchID = 0xFFFF; }
		#define CIF_MSC_KFILE_WRITE_EN(subch_id) {g_nKfileDecSubchID = subch_id; }
	#else
		#define CIF_KFILE_CLOSE(subch_id)		((void)0)
		#define CIF_KFILE_OPEN(subch_id)		((void)0)
		#define CIF_KFILE_WRITE(buf, size)		((void)0)
		#define CIF_MSC_KFILE_WRITE_DIS			((void)0)
		#define CIF_MSC_KFILE_WRITE_EN(subch_id)	((void)0)
	#endif

	#include <linux/uaccess.h>

	#define CIF_DATA_COPY(dst_ptr, src_ptr, size)	\
		do {	\
			CIF_KFILE_WRITE(src_ptr, size);\
			if (copy_to_user(dst_ptr, src_ptr, size))\
				RTV_DBGMSG("copy_to_user error!\n");\
		} while (0)
#else
	#define CIF_KFILE_CLOSE(subch_id)		((void)0)
	#define CIF_KFILE_OPEN(subch_id)		((void)0)
	#define CIF_KFILE_WRITE(buf, size)		((void)0)
	#define CIF_MSC_KFILE_WRITE_DIS			((void)0)
	#define CIF_MSC_KFILE_WRITE_EN(subch_id)	((void)0)

	#define CIF_DATA_COPY(dst_ptr, src_ptr, size)	\
		memcpy(dst_ptr, src_ptr, size)
#endif


#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
static enum RTV_MCH_HDR_ID_FLAG_T g_eSingleSvcFlagID;
#endif

#define CIF_DISCARDED_TSP_MARK_0	0x48
#define CIF_DISCARDED_TSP_MARK_1	0xFF

#define CIF_INVALID_SUBCH_ID		0xFF

static UINT g_nOutDecBufIdxBits; /* Decoded out buffer index bits */

static enum E_RTV_SERVICE_TYPE g_eaSvcType[MAX_NUM_SUBCH];

#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	/* Decoded out buffer index for sub ch */
	static U8 g_abOutDecBufIdx[MAX_NUM_SUBCH];
#endif

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	static UINT g_nSingleSubchID;
#endif

static BOOL g_fCifInited = FALSE;

static BOOL g_fForceDecodeStop;


enum CIFDEC_STATUS_TYPE {
	CIFDEC_S__OK = 0,
	CIFDEC_S__INVALID_SYNC_BYTE = 1,
	CIFDEC_S__INVALID_SUBCH_SIZE = 2,
	CIFDEC_S__INVALID_SUBCH_ID = 3,

	CIFDEC_S__NOT_EN_FIC_DECODER = 4,
	CIFDEC_S__CHANGED_SUBCH_ID = 5,
	CIFDEC_S__SMALL_DEC_BUF = 6,
	CIFDEC_S__CIFB_ALLOC_ERR = 7,
	CIFDEC_S__DISCARDED_TSP = 8,

	CIFDEC_S__NOT_ENOUGH_DECODE_MSC_SIZE = 9,
	CIFDEC_S__INVALID_HEADER_BYTE = 10,
	CIFDEC_S__INVALID_SVC_TYPE = 11,
	CIFDEC_S__MISSING_LAST_DMB_TS = 12,

	CIFDEC_S__TRUNCATED_FIC = 13,
	CIFDEC_S__NOT_ENOUGH_DECODE_FIC_SIZE = 14,

#ifdef _DEBUG_HEADER_DECODING
	CIFDEC_S__ZERO_PAYLOAD_SIZE = 15,
	CIFDEC_S__SHORT_PAYLOAD_SIZE = 16,

	CIFDEC_S__INVALID_FIC_SIZE = 17,
#endif
	MAX_NUM_DEC_ERR_TYPE
};


#ifdef _DEBUG_CIFDEC_ERR_STATIC
	/* CIF decoder error statistics */
	static unsigned long g_aErrStat[MAX_NUM_DEC_ERR_TYPE];
	#define CIF_ERR_STAT_INC(err_type)\
		{ g_aErrStat[err_type]++; }
	#define CIF_ERR_STAT_RST(err_type)\
		{ g_aErrStat[err_type] = 0; }
	#define CIF_ERR_STAT_RST_ALL\
		memset(g_aErrStat, 0, sizeof(g_aErrStat))
	#define CIF_ERR_STAT_SHOW\
		do {\
			INT i, mod = (MAX_NUM_DEC_ERR_TYPE-1) % 2;	\
			CIF_DBGMSG0("|----- CIF decoding Statistics -----\n");\
			for (i = 1; i < MAX_NUM_DEC_ERR_TYPE-1; i += 2) {\
				CIF_DBGMSG4("| S_%d(%ld), S_%d(%ld)\n",\
				i, g_aErrStat[i], i+1, g_aErrStat[i+1]);\
			}	\
			if (mod)	\
				CIF_DBGMSG2("| S_%d(%ld)\n", i, g_aErrStat[i]);\
			CIF_DBGMSG0("|------------------------------------\n");\
		} while (0)
#else
	#define CIF_ERR_STAT_INC(err_type)	((void)0)
	#define CIF_ERR_STAT_RST(err_type)	((void)0)
	#define CIF_ERR_STAT_RST_ALL		((void)0)
	#define CIF_ERR_STAT_SHOW			((void)0)
#endif

/* CIF decoder Buffer */
struct CIFB_INFO {
	struct CIFB_INFO *ptNext;

	U8 id_flag;
	U8 subch_id;
	U8 cont;
	U8 ts_size; /* size of data in the current TSP. */

	U16 data_len; /* Total data size */

	U8 header_on;

	const U8 *ts_ptr; /* The pointer to real TS data */

	BOOL frag_buf_used;
};

struct CIFB_HEAD_INFO {
	struct CIFB_INFO *ptFirst;
	struct CIFB_INFO *ptLast;
	UINT cnt; /* Item count in the list */
	UINT tts_len; /* Total ts len in the chain. */
};


#define NUM_CIF_BUF_POOL\
	(MAX_NUM_INPUT_TSP + 2/*FIC frag*/\
	+ (10*RTV_MAX_NUM_USE_SUBCHANNEL)/*for short frag cifb*/)


/* FIC TSP Queue */
static struct CIFB_HEAD_INFO cifdec_fic_tspq;

/* Buffer for fragment. Up to 2 fragments. */
static U8 fic_frag_buf[2][188];

/* MSC TSP Queue */
static struct CIFB_HEAD_INFO cifdec_msc_tspq[RTV_MAX_NUM_USE_SUBCHANNEL];

/* Buffer for fragment */
static U8 msc_frag_buf[RTV_MAX_NUM_USE_SUBCHANNEL][188*2];

static struct CIFB_INFO cif_buf_pool[NUM_CIF_BUF_POOL];
static struct CIFB_INFO *g_aCifBufPoolList[NUM_CIF_BUF_POOL];
static UINT g_nCifBufPoolFreeCnt;


static INLINE void cifb_free_buffer(struct CIFB_INFO *cifb)
{
	CIF_ASSERT(g_nCifBufPoolFreeCnt < NUM_CIF_BUF_POOL);
	g_aCifBufPoolList[g_nCifBufPoolFreeCnt++] = cifb;
}

static INLINE struct CIFB_INFO *cifb_alloc_buffer(const U8 *ts_ptr)
{
	if (g_nCifBufPoolFreeCnt) {
		struct CIFB_INFO *cifb;

		cifb = g_aCifBufPoolList[--g_nCifBufPoolFreeCnt];
		cifb->ts_ptr = ts_ptr;
		cifb->ts_size = 0;
		return cifb;
	} else
		return NULL;
}

static INLINE void cifb_queue_unlink(struct CIFB_HEAD_INFO *q,
					struct CIFB_INFO *cifb)
{
	if (q->ptFirst != NULL) {
		q->ptFirst = cifb->ptNext;

		if (q->ptFirst == NULL)
			q->ptLast = NULL;

		cifb->ptNext = NULL;
	}

	q->tts_len -= cifb->ts_size;
	q->cnt--;
}

static INLINE void cifb_queue_free(struct CIFB_HEAD_INFO *q,
						struct CIFB_INFO *cifb)
{
	CIF_ASSERT(q->cnt != 0);
	cifb_queue_unlink(q, cifb);
	cifb_free_buffer(cifb);
}

static INLINE struct CIFB_INFO *cifb_peek_frist(struct CIFB_HEAD_INFO *q)
{
	return q->ptFirst;
}

static void cifb_queue_release_all(struct CIFB_HEAD_INFO *q)
{
	struct CIFB_INFO *cifb;

	while ((cifb = cifb_peek_frist(q)) != NULL)
		cifb_queue_free(q, cifb);

	CIF_ASSERT(g_nCifBufPoolFreeCnt == NUM_CIF_BUF_POOL);
}

static INLINE void cifb_enqueue(struct CIFB_HEAD_INFO *q,
				struct CIFB_INFO *new)
{
	new->ptNext = NULL;

	if (q->ptLast != NULL)
		q->ptLast->ptNext = new;
	else
		q->ptFirst = new;

	q->ptLast = new;

	q->cnt++;
	q->tts_len += new->ts_size;
}

static INLINE void cifb_queue_init(struct CIFB_HEAD_INFO *q)
{
	q->ptFirst = q->ptLast = NULL;

	q->cnt = 0;
	q->tts_len = 0;
}

/**
 * Copy TS from cifb to decoded output buffer.
 * return: Copying node in next time or Head.
 */
static INLINE struct CIFB_INFO *cifb_copydec(U8 *dec_buf,
			struct CIFB_INFO *cifb,
			struct CIFB_HEAD_INFO *head, UINT len)
{
	UINT copied;
	struct CIFB_INFO *next;

	CIF_ASSERT(cifb != NULL);
	CIF_ASSERT(len && (head->tts_len >= len));

	while ((cifb = cifb_peek_frist(head)) != NULL) {
		copied = MIN(cifb->ts_size, len);
		if (copied)
			CIF_DATA_COPY(dec_buf, cifb->ts_ptr, copied);

		if (cifb->ts_size == copied) {
			next = cifb->ptNext;
			cifb_queue_free(head, cifb);
			cifb = next;
		} else {
			cifb->ts_ptr += copied;
			cifb->ts_size -= copied;
			head->tts_len -= copied;
		}

		dec_buf += copied;
		len -= copied;
		if (len == 0)
			break;
	}

	return cifb;
}

/* Copy the Audio/Data MSC into user-buffer with 188-bytes align. */
static UINT copy_dab_188(struct RTV_CIF_DEC_INFO *dec, UINT idx,
						struct CIFB_HEAD_INFO *tspq,
						enum CIFDEC_STATUS_TYPE *status)
{
	UINT mod, copied;
	UINT subch_size = dec->subch_size[idx];
	U8 *subch_ptr = &dec->subch_buf_ptr[idx][subch_size];
	struct CIFB_INFO *cifb = cifb_peek_frist(tspq); /* The first entry */

	*status = CIFDEC_S__OK;

	if (tspq->tts_len >= 188) {
		mod = tspq->tts_len % 188;
		copied = tspq->tts_len - mod;

		cifb = cifb_copydec(subch_ptr, cifb, tspq, copied);
		dec->subch_size[idx] += copied;
	}

	if (tspq->tts_len) {
		UINT len; /* Length, in bytes of fragment data */
		struct CIFB_INFO *frist_frag_cifb = cifb;

		if (cifb->frag_buf_used == FALSE)
			len = 0;
		else {
			len = cifb->ts_size;
			memmove(msc_frag_buf[idx], cifb->ts_ptr, len);
			cifb->ts_size = 0;
		}

		do {
			/* Copy ts data into fragment-buffer. */
			if (cifb->ts_size) {
				memcpy(&msc_frag_buf[idx][len],
						cifb->ts_ptr, cifb->ts_size);

				len += cifb->ts_size;
				cifb->ts_size = 0; /* Reset the size */
			}

			cifb = cifb->ptNext;
		} while (cifb);

		/* Adjust ts pointer */
		frist_frag_cifb->ts_ptr = (const U8 *)msc_frag_buf[idx];
		frist_frag_cifb->ts_size = len;
		frist_frag_cifb->frag_buf_used = TRUE;

		*status = CIFDEC_S__NOT_ENOUGH_DECODE_MSC_SIZE;
	}

	return dec->subch_size[idx] - subch_size;
}

static void proc_msc_dab(struct RTV_CIF_DEC_INFO *dec,
			struct CIFB_HEAD_INFO *tspq)
{
	enum CIFDEC_STATUS_TYPE status = CIFDEC_S__OK;
	U8 dec_idx;
	UINT i, copy_size;
	struct CIFB_INFO *cifb = cifb_peek_frist(tspq); /* The first entry */

	/* Get the index of output buffer determined when add sub channel. */
#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	dec_idx = 0;
#else
	dec_idx = g_abOutDecBufIdx[cifb->subch_id];
#endif
	if (dec->subch_id[dec_idx] == CIF_INVALID_SUBCH_ID)
		dec->subch_id[dec_idx] = cifb->subch_id; /* The first */

	/* Check if subch ID was changed in the same out buf index ? */
	if (dec->subch_id[dec_idx] == cifb->subch_id) {
		/*Check if the size is greater than the size of output buffer.*/
		if (tspq->tts_len <= dec->subch_buf_size[dec_idx]) {
			CIF_MSC_KFILE_WRITE_EN(cifb->subch_id);
			copy_size = copy_dab_188(dec, dec_idx, tspq, &status);
			dec->subch_buf_size[dec_idx] -= copy_size;
			CIF_MSC_KFILE_WRITE_DIS;
		} else
			status = CIFDEC_S__SMALL_DEC_BUF;
	} else
		status = CIFDEC_S__CHANGED_SUBCH_ID;

	if (status != CIFDEC_S__OK) {
		if (status == CIFDEC_S__NOT_ENOUGH_DECODE_MSC_SIZE) {
			CIF_ERR_STAT_INC(status);
		} else {
			for (i = 0; i < tspq->cnt; i++) /* must first */
				CIF_ERR_STAT_INC(status);

			cifb_queue_release_all(tspq);
		}
	}
}

static int copy_dmb_last(struct RTV_CIF_DEC_INFO *dec,
		struct CIFB_HEAD_INFO *tspq, struct CIFB_INFO *cifb,
		UINT idx, enum CIFDEC_STATUS_TYPE *status,
		U8 **subch_ptr__)
{
	U8 *subch_ptr = *subch_ptr__;

	if (cifb->ts_ptr[0] == 0x47) {
		if (tspq->cnt > 1) {
			if (cifb->ptNext->cont == RTV_MCH_HDR_CONT_LAST) {
				cifb_copydec(subch_ptr,
						cifb, tspq, 188);
				subch_ptr += 188;
				dec->subch_size[idx] += 188;
			} else {
				CIF_ERR_STAT_INC(CIFDEC_S__MISSING_LAST_DMB_TS);
				cifb_queue_free(tspq, cifb); /* Discard TSP */
			}
		} else {
			/* Copy ts data into buffer. NOT input buffer! */
			memcpy(&msc_frag_buf[idx],
					cifb->ts_ptr, cifb->ts_size);

			/* Adjust ts pointer */
			cifb->ts_ptr = (const U8 *)&msc_frag_buf[idx];
			cifb->frag_buf_used = TRUE;

			*status = CIFDEC_S__NOT_ENOUGH_DECODE_MSC_SIZE;
			return -1;
		}
	} else {
		CIF_ERR_STAT_INC(CIFDEC_S__INVALID_SYNC_BYTE);
		cifb_queue_free(tspq, cifb); /* Discard TSP */
	}

	return 0;
}

/* Copy the Video MSC into user-buffer with 188-bytes align. */
static UINT copy_dmb_188(struct RTV_CIF_DEC_INFO *dec, UINT idx,
				struct CIFB_HEAD_INFO *tspq,
				enum CIFDEC_STATUS_TYPE *status)
{
	struct CIFB_INFO *cifb;
	UINT subch_size = dec->subch_size[idx];
	U8 *subch_ptr = &dec->subch_buf_ptr[idx][subch_size];

	*status = CIFDEC_S__OK;

	while ((cifb = cifb_peek_frist(tspq)) != NULL) {
		if (g_fForceDecodeStop == TRUE)
			break;

		if (cifb->header_on == 0) {
			CIF_DATA_COPY(subch_ptr, cifb->ts_ptr, 188);
			subch_ptr += 188;
			dec->subch_size[idx] += 188;
			cifb_queue_free(tspq, cifb);
		} else {
			if (cifb->cont == RTV_MCH_HDR_CONT_FIRST) {
				if (copy_dmb_last(dec, tspq, cifb, idx,
						status, &subch_ptr) != 0)
					goto exit_dmb_copy;
			} else
				/* Discard the TSP. */
				cifb_queue_free(tspq, cifb);
		}
	}

exit_dmb_copy:
	return dec->subch_size[idx] - subch_size;
}

static void proc_msc_dmb_dabp(struct RTV_CIF_DEC_INFO *dec,
				struct CIFB_HEAD_INFO *tspq)
{
	enum CIFDEC_STATUS_TYPE status = CIFDEC_S__OK;
	U8 dec_idx;
	UINT i, copy_size;
	struct CIFB_INFO *cifb = cifb_peek_frist(tspq); /* The first entry */

	/* Get the index of output buffer determined when add sub channel. */
#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	dec_idx = 0;
#else
	dec_idx = g_abOutDecBufIdx[cifb->subch_id];
#endif
	if (dec->subch_id[dec_idx] == CIF_INVALID_SUBCH_ID)
		dec->subch_id[dec_idx] = cifb->subch_id; /* The 1st time */

	/* Check if subch ID was changed in the same out buf index ? */
	if (dec->subch_id[dec_idx] == cifb->subch_id) {
		/*Check if the size is greater than the size of output buffer.*/
		if (tspq->tts_len <= dec->subch_buf_size[dec_idx]) {
			CIF_MSC_KFILE_WRITE_EN(cifb->subch_id);

			if (g_eaSvcType[cifb->subch_id]
						== RTV_SERVICE_DMB)
				copy_size = copy_dmb_188(dec, dec_idx,
							tspq, &status);
			else
				copy_size = copy_dab_188(dec, dec_idx,
							tspq, &status);

			dec->subch_buf_size[dec_idx] -= copy_size;
			CIF_MSC_KFILE_WRITE_DIS;
		} else
			status = CIFDEC_S__SMALL_DEC_BUF;
	} else
		status = CIFDEC_S__CHANGED_SUBCH_ID;

	if (status != CIFDEC_S__OK) {
		if (status == CIFDEC_S__NOT_ENOUGH_DECODE_MSC_SIZE) {
			CIF_ERR_STAT_INC(status);
		} else {
			for (i = 0; i < tspq->cnt; i++) /* must first */
				CIF_ERR_STAT_INC(status);

			cifb_queue_release_all(tspq);
		}
	}
}

#if 0
static void proc_fidc(struct RTV_CIF_DEC_INFO *dec,
					struct CIFB_HEAD_INFO *tspq)
{
	struct CIFB_INFO *cifb = cifb_peek_frist(tspq); /* The first entry */

	CIF_DBGMSG0("[proc_fidc] EWS data. Not yet processed...\n");

	while ((cifb = cifb_peek_frist(tspq)) != NULL)
		cifb = cifb_queue_free(tspq, cifb);
}
#endif

static struct CIFB_INFO *copy_fic_192(struct RTV_CIF_DEC_INFO *dec,
		struct CIFB_HEAD_INFO *tspq,
		struct CIFB_INFO *cifb, enum CIFDEC_STATUS_TYPE *status)
{
	UINT copied = 0; /* Copy length */

	if (tspq->cnt >= 2) {
		if (cifb->ptNext->cont == RTV_MCH_HDR_CONT_LAST) {
			dec->fic_size = cifb->data_len; /* FINISHED */

			copied = cifb->ts_size/* First */
				+ cifb->ptNext->ts_size/* Last */;
			cifb = cifb_copydec(dec->fic_buf_ptr,
						cifb, tspq, copied);
		} else
			*status = CIFDEC_S__TRUNCATED_FIC;
	} else
		*status = CIFDEC_S__NOT_ENOUGH_DECODE_FIC_SIZE;

	return cifb;
}

static struct CIFB_INFO *copy_fic_96_128(struct RTV_CIF_DEC_INFO *dec,
		struct CIFB_HEAD_INFO *tspq,
		struct CIFB_INFO *cifb, enum CIFDEC_STATUS_TYPE *status)
{
	dec->fic_size = cifb->data_len; /* FINISHED */
	cifb = cifb_copydec(dec->fic_buf_ptr, cifb, tspq, cifb->data_len);

	return cifb;
}

static INLINE struct CIFB_INFO *copy_fic_384(struct RTV_CIF_DEC_INFO *dec,
				struct CIFB_HEAD_INFO *tspq,
				struct CIFB_INFO *cifb,
				enum CIFDEC_STATUS_TYPE *status)
{
	UINT copied = 0; /* Copy length */

	if (tspq->cnt >= 3) {
		if ((cifb->ptNext->cont == RTV_MCH_HDR_CONT_MED)
		&& (cifb->ptNext->ptNext->cont == RTV_MCH_HDR_CONT_LAST)) {
			dec->fic_size = cifb->data_len; /* FINISHED */

			copied = cifb->ts_size/* First */
					+ cifb->ptNext->ts_size/* Med */
					+ cifb->ptNext->ptNext->ts_size/*Last*/;
			cifb = cifb_copydec(dec->fic_buf_ptr,
						cifb, tspq, copied);
		} else
			*status = CIFDEC_S__TRUNCATED_FIC;
	} else
		*status = CIFDEC_S__NOT_ENOUGH_DECODE_FIC_SIZE;

	return cifb;
}

static void copy_fic_fragment(struct CIFB_INFO *cifb)
{
	UINT cnt = 0;

	do {
		/* Copy ts data into buffer. NOT input buffer! */
		memcpy(&fic_frag_buf[cnt],
				cifb->ts_ptr, cifb->ts_size);

		/* Adjust ts pointer */
		cifb->ts_ptr = (const U8 *)&fic_frag_buf[cnt];
		cnt++;

		cifb = cifb->ptNext;
	} while (cifb);
}

static void proc_fic(struct RTV_CIF_DEC_INFO *dec, struct CIFB_HEAD_INFO *tspq)
{
	enum CIFDEC_STATUS_TYPE status;
	struct CIFB_INFO *cifb;

	while ((cifb = cifb_peek_frist(tspq)) != NULL) {
		if (g_fForceDecodeStop == TRUE)
			break;

		status = CIFDEC_S__OK;

		if (!dec->fic_size) { /* Decoding was not finished ? */
			if (cifb->cont == RTV_MCH_HDR_CONT_FIRST) {
				switch (cifb->data_len) {
				case 384:
					cifb = copy_fic_384(dec, tspq,
							cifb, &status);
					break;
				case 96:
				case 128:
					cifb = copy_fic_96_128(dec, tspq,
								cifb, &status);
					break;
				case 192:
					cifb = copy_fic_192(dec, tspq,
							cifb, &status);
					break;
				default:
					break;
				}

				if (status != CIFDEC_S__OK) {
					CIF_ERR_STAT_INC(status);

					if (status
				== CIFDEC_S__NOT_ENOUGH_DECODE_FIC_SIZE) {
						copy_fic_fragment(cifb);
						goto stop_fic_decode;
					}

					cifb_queue_free(tspq, cifb);
				}
			} else if (cifb->cont == RTV_MCH_HDR_CONT_ALONE) {
				dec->fic_size = cifb->data_len; /* FINISHED */
				cifb_copydec(dec->fic_buf_ptr, cifb,
							tspq, cifb->data_len);
			} else
				/* Discard the TSP. */
				cifb_queue_free(tspq, cifb);
		} else { /* Already decoded ? */
			/* Discard the TSP. */
			cifb_queue_free(tspq, cifb);
			/*CIF_DBGMSG0("[proc_fic] Already decoded.\n");*/
		}
	}

stop_fic_decode:
	return;
}

static INLINE enum CIFDEC_STATUS_TYPE
	decode_header(struct CIFB_INFO *cifb, const U8 *tsp)
{
	UINT array_idx, last_size;
	U32 bit_val;

	if (tsp[0] == RTV_MCH_HEADER_SYNC_BYTE) { /* sync byte ok? */
		cifb->id_flag = tsp[1] >> 6;
		cifb->subch_id = tsp[1] & 0x3F;
		cifb->cont = tsp[2] >> 6;
		cifb->data_len = ((tsp[2] & 0x3F)<<8) | tsp[3];

		cifb->header_on = 1;
		cifb->frag_buf_used = FALSE;

#ifdef _DEBUG_HEADER_DECODING
		if (cifb->data_len == 0)
			return CIFDEC_S__ZERO_PAYLOAD_SIZE;
#endif

		last_size = cifb->data_len % MAX_MCH_PAYLOAD_SIZE;
		if (!last_size)
			last_size = MAX_MCH_PAYLOAD_SIZE;

		switch (cifb->cont) {
		case RTV_MCH_HDR_CONT_FIRST:
		case RTV_MCH_HDR_CONT_MED:
#ifdef _DEBUG_HEADER_DECODING
			if (cifb->data_len < MAX_MCH_PAYLOAD_SIZE)
				return CIFDEC_S__SHORT_PAYLOAD_SIZE;
#endif
			cifb->ts_size = MAX_MCH_PAYLOAD_SIZE;
			break;
		case RTV_MCH_HDR_CONT_LAST:
			cifb->ts_size = last_size;
			break;
		default: /* Alone */
			cifb->ts_size = (U8)cifb->data_len;
			break;
		}

		switch (cifb->id_flag) {
		case RTV_MCH_HDR_ID_DMB:
		case RTV_MCH_HDR_ID_DAB:
			if (cifb->id_flag == RTV_MCH_HDR_ID_DMB) {
				if (cifb->data_len != 188)
					return CIFDEC_S__INVALID_SUBCH_SIZE;
			} else {
				if (cifb->data_len > 6912/*1 CIF size*/)
					return CIFDEC_S__INVALID_SUBCH_SIZE;
			}

			array_idx = cifb->subch_id >> 5; /* Divide by 32 */
			bit_val = 1 << (cifb->subch_id & 31);
			if (!(g_aAddedSubChIdBits[array_idx] & bit_val))
				/* Closed(or Not opened) or Changed ID */
				return CIFDEC_S__INVALID_SUBCH_ID;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
			if (g_eSingleSvcFlagID != cifb->id_flag)
				return CIFDEC_S__INVALID_SVC_TYPE;
#endif
			break;

		case RTV_MCH_HDR_ID_FIC:
#ifdef _DEBUG_HEADER_DECODING
			switch (cifb->data_len) {
			case 384: case 96: case 128: case 192:
				break;
			default:
				return CIFDEC_S__INVALID_FIC_SIZE;
			}
#endif
			break;

		default: /* RTV_MCH_HDR_ID_FIDC */
			break;
		}
	} else { /* Not sync byte */
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if !defined(RTV_MSC_HDR_ENABLED)
		if (g_eSingleSvcFlagID == RTV_MCH_HDR_ID_DMB) {
			if (tsp[0] == 0x47) {
				cifb->header_on = 0;
				cifb->subch_id = g_nSingleSubchID;
				cifb->ts_size = 188;
				cifb->id_flag = RTV_MCH_HDR_ID_DMB;
				cifb->ts_ptr -= RTV_MCH_HDR_SIZE;
				return CIFDEC_S__OK;
			}
			CIF_DBGMSG1("[decode_headers] ERR tsp[%d]\n", tsp[0]);
		} else
			return CIFDEC_S__INVALID_SVC_TYPE;
	#endif
#endif
		/* tsp[2]: alone tsp && MSB of data_len. */
		if (tsp[2] == CIF_DISCARDED_TSP_MARK_1)
			return CIFDEC_S__DISCARDED_TSP;
		else
			return CIFDEC_S__INVALID_HEADER_BYTE;
	}

	return CIFDEC_S__OK;
}

int rtvCIFDEC_Decode(struct RTV_CIF_DEC_INFO *ptDecInfo,
			const U8 *pbTsBuf, UINT nTsLen)
{
	enum CIFDEC_STATUS_TYPE status;
	struct CIFB_INFO *cifb;
	struct CIFB_HEAD_INFO *tspq;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	ptDecInfo->subch_size[0] = 0;
	ptDecInfo->subch_id[0] = CIF_INVALID_SUBCH_ID; /* Default invalid */
#else
	UINT i, q_idx;

	for (i = 0; i < RTV_MAX_NUM_USE_SUBCHANNEL; i++) {
		ptDecInfo->subch_size[i] = 0;
		ptDecInfo->subch_id[i] = CIF_INVALID_SUBCH_ID; /* Default */
	}
#endif

	ptDecInfo->fic_size = 0;

	if (nTsLen == 0) {
		CIF_DBGMSG0("[rtvCIFDEC_Decode] Sourct TS size is zero\n");
		return 0;
	}

#if 0 /* for debug */
	if (nTsLen % RTV_TSP_XFER_SIZE) {
		CIF_DBGMSG1("[rtvCIFDEC_Decode] Not aligned: %u\n", nTsLen);
		return;
	}
#endif

	if (g_fCifInited == FALSE) {
		CIF_DBGMSG0("[rtvCIFDEC_Decode] Not yet init\n");
		return 0;
	}

	CIF_MUTEX_LOCK;

	do {
		if (g_fForceDecodeStop == TRUE)
			break;

cifb_alloc_retry:
		/* Get a current TSP header buffer to be decoded. */
		cifb = cifb_alloc_buffer(pbTsBuf + RTV_MCH_HDR_SIZE);
		if (!cifb) {
			/* Free the first entry. */
			if (cifdec_msc_tspq[0].cnt) /* Not empty? */
				cifb_queue_free(&cifdec_msc_tspq[0],
						cifdec_msc_tspq[0].ptFirst);

			CIF_DBGMSG0("No more cif buffer!\n");
			CIF_ERR_STAT_INC(CIFDEC_S__CIFB_ALLOC_ERR);
			goto cifb_alloc_retry;
		}

		status = decode_header(cifb, pbTsBuf);
		if (status == CIFDEC_S__OK) {
			switch (cifb->id_flag) {
			case RTV_MCH_HDR_ID_DMB:
			case RTV_MCH_HDR_ID_DAB:
#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
				cifb_enqueue(&cifdec_msc_tspq[0], cifb);
#else
				q_idx = g_abOutDecBufIdx[cifb->subch_id];
				cifb_enqueue(&cifdec_msc_tspq[q_idx], cifb);
#endif
				break;

			case RTV_MCH_HDR_ID_FIC:
				cifb_enqueue(&cifdec_fic_tspq, cifb);
				break;

			default: /* FIDC */
				CIF_DBGMSG1("Invalid ID flag(%d)\n",
						cifb->id_flag);
				cifb_free_buffer(cifb);
				break;
			}
		} else {
			cifb_free_buffer(cifb);
			CIF_ERR_STAT_INC(status);
		}

		pbTsBuf += RTV_TSP_XFER_SIZE;
		nTsLen -= RTV_TSP_XFER_SIZE;
	} while (nTsLen);

	if (cifdec_fic_tspq.cnt) /* Not empty? */
		proc_fic(ptDecInfo, &cifdec_fic_tspq);

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	if (cifdec_msc_tspq[0].cnt) { /* Not empty? */
		tspq = &cifdec_msc_tspq[0];
		cifb = cifb_peek_frist(tspq);

		if (cifb->id_flag == RTV_MCH_HDR_ID_DMB) /* or DAB+ */
			proc_msc_dmb_dabp(ptDecInfo, tspq);
		else
			proc_msc_dab(ptDecInfo, tspq);
	}
#else
	for (i = 0; i < RTV_MAX_NUM_USE_SUBCHANNEL; i++) {
		if (cifdec_msc_tspq[i].cnt) { /* Not empty? */
			tspq = &cifdec_msc_tspq[i];
			cifb = cifb_peek_frist(tspq);
			if (cifb->id_flag == RTV_MCH_HDR_ID_DMB)
				proc_msc_dmb_dabp(ptDecInfo, tspq);
			else
				proc_msc_dab(ptDecInfo, tspq);
		}
	}
#endif

	CIF_MUTEX_UNLOCK;

	return 2;
}

/* nFicMscType: 0(FIC), 1(MSC) */
UINT rtvCIFDEC_SetDiscardTS(int nFicMscType, U8 *pbTsBuf, UINT nTsLen)
{
	UINT i, nTsCnt;
	enum RTV_MCH_HDR_ID_FLAG_T nIdFlag;
	UINT nDiscardSize = 0;

	nTsCnt = nTsLen / RTV_TSP_XFER_SIZE;

	for (i = 0; i < nTsCnt; i++, pbTsBuf += RTV_TSP_XFER_SIZE) {
		if (pbTsBuf[0] == RTV_MCH_HEADER_SYNC_BYTE) {
			/* NOT discarded byte */
			nIdFlag = pbTsBuf[1] >> 6;

			if (nFicMscType == 1) {
				if ((nIdFlag == RTV_MCH_HDR_ID_DMB)
				|| (nIdFlag == RTV_MCH_HDR_ID_DAB)) {
					/* sync byte => discarded byte */
					pbTsBuf[0] = CIF_DISCARDED_TSP_MARK_0;
					pbTsBuf[2] = CIF_DISCARDED_TSP_MARK_1;
					nDiscardSize += RTV_TSP_XFER_SIZE;
				}
			} else {
				if (nIdFlag == RTV_MCH_HDR_ID_FIC) {
					pbTsBuf[0] = CIF_DISCARDED_TSP_MARK_0;
					pbTsBuf[2] = CIF_DISCARDED_TSP_MARK_1;
					nDiscardSize += RTV_TSP_XFER_SIZE;
				}
			}
		}
	}

	return nDiscardSize;
}

UINT rtvCIFDEC_GetDecBufIndex(UINT nSubChID)
{
	UINT nDecIdx;

	if (g_fCifInited == FALSE) {
		RTV_DBGMSG("Not yet init\n");
		return RTV_CIFDEC_INVALID_BUF_IDX;
	}

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	nDecIdx = 0;
#else
	CIF_MUTEX_LOCK; /* To wait for add/delete operation. */
	nDecIdx = g_abOutDecBufIdx[nSubChID];
	CIF_MUTEX_UNLOCK;
#endif

	return nDecIdx;
}

/**
 * This function delete a sub channel ID from the CIF decoder.
 * This function should called after Sub Channel Close.
 */
void rtvCIFDEC_DeleteSubChannelID(UINT nSubChID)
{
	U8 nIdx;
	UINT nArrayIdx = nSubChID >> 5; /* Divide by 32 */
	U32 dwBitVal = 1 << (nSubChID & 31); /* Modular and Shift */

	if (g_fCifInited == FALSE) {
		RTV_DBGMSG("Not yet init\n");
		return;
	}

	CIF_MUTEX_LOCK;

	if (g_aAddedSubChIdBits[nArrayIdx] & dwBitVal) {
		CIF_KFILE_CLOSE(nSubChID);

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
		nIdx = 0;
		cifb_queue_release_all(&cifdec_msc_tspq[0]);
		g_nSingleSubchID = CIF_INVALID_SUBCH_ID;
#else
		nIdx = g_abOutDecBufIdx[nSubChID];
		cifb_queue_release_all(&cifdec_msc_tspq[nIdx]);
		g_abOutDecBufIdx[nSubChID] = RTV_CIFDEC_INVALID_BUF_IDX;
#endif
		g_nOutDecBufIdxBits &= ~(1 << nIdx);
		g_aAddedSubChIdBits[nArrayIdx] &= ~dwBitVal; /* after kfile */
	}

	CIF_MUTEX_UNLOCK;
}

/**
 * This function add a sub channel ID to the CIF decoder to verify CIF header.
 * This function should called before Sub Channel Open.
 */
BOOL rtvCIFDEC_AddSubChannelID(UINT nSubChID,
				enum E_RTV_SERVICE_TYPE eServiceType)
{
	BOOL fRet = FALSE;
	U8 nIdx;
	UINT nArrayIdx = nSubChID >> 5; /* Divide by 32 */
	U32 dwBitVal = 1 << (nSubChID & 31); /* Modular and Shift */

	if (g_fCifInited == FALSE) {
		RTV_DBGMSG("Not yet init\n");
		return FALSE;
	}

	CIF_MUTEX_LOCK;

	/* Check if already registerd ? */
	if ((g_aAddedSubChIdBits[nArrayIdx] & dwBitVal) == 0) {
		g_aAddedSubChIdBits[nArrayIdx] |= dwBitVal;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
		nIdx = 0;
		g_nSingleSubchID = nSubChID;
#else
		for (nIdx = 0; nIdx < RTV_MAX_NUM_USE_SUBCHANNEL; nIdx++) {
#endif
			if ((g_nOutDecBufIdxBits & (1<<nIdx)) == 0) {
				g_nOutDecBufIdxBits |= (1<<nIdx);
				g_eaSvcType[nSubChID] = eServiceType;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
				cifb_queue_init(&cifdec_msc_tspq[0]);
#else
				g_abOutDecBufIdx[nSubChID] = nIdx;
				cifb_queue_init(&cifdec_msc_tspq[nIdx]);
#endif

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
				switch (eServiceType) {
				case RTV_SERVICE_DMB:
					g_eSingleSvcFlagID = RTV_MCH_HDR_ID_DMB;
					break;
				case RTV_SERVICE_DAB:
					g_eSingleSvcFlagID = RTV_MCH_HDR_ID_DAB;
					break;
				case RTV_SERVICE_DABPLUS:
					g_eSingleSvcFlagID = RTV_MCH_HDR_ID_DAB;
					break;
				default:
					break;
				}
#endif
				fRet = TRUE;

				CIF_KFILE_OPEN(nSubChID);
		#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
				break;
		#endif
			}
#if (RTV_MAX_NUM_USE_SUBCHANNEL != 1)
		}
#endif
	}

	CIF_MUTEX_UNLOCK;

	return fRet;
}

/* This function deinitialize the CIF decoder.*/
void rtvCIFDEC_Deinit(void)
{
	if (g_fCifInited == FALSE)
		return;

	g_fCifInited = FALSE;
	g_fForceDecodeStop = TRUE;

	CIF_DBGMSG0("CIF decode Exit\n");

	CIF_MUTEX_LOCK;

	CIF_ERR_STAT_SHOW;

	CIF_KFILE_CLOSE(0xFFFF);

	CIF_MUTEX_UNLOCK;

	CIF_MUTEX_DEINIT;
}


/* This function initialize the CIF decoder. */
void rtvCIFDEC_Init(void)
{
	UINT i;

	if (g_fCifInited == TRUE) {
		CIF_DBGMSG0("Already inited!\n");
		return;
	}

	g_nCifBufPoolFreeCnt = NUM_CIF_BUF_POOL;
	for (i = 0; i < NUM_CIF_BUF_POOL; i++)
		g_aCifBufPoolList[i] = &cif_buf_pool[(NUM_CIF_BUF_POOL-1) - i];

	CIF_ERR_STAT_RST_ALL;

	g_nOutDecBufIdxBits = 0x00;

	g_aAddedSubChIdBits[0] = 0;
	g_aAddedSubChIdBits[1] = 0;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	g_nSingleSubchID = CIF_INVALID_SUBCH_ID;
#endif
#ifdef _MSC_KERNEL_FILE_DUMP_ENABLE
	memset(g_ptDecMscFilp, 0, sizeof(g_ptDecMscFilp));
	CIF_MSC_KFILE_WRITE_DIS;
#endif

#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	memset(g_abOutDecBufIdx, RTV_CIFDEC_INVALID_BUF_IDX,
		sizeof(g_abOutDecBufIdx));
#endif

	CIF_MUTEX_INIT;

	cifb_queue_init(&cifdec_fic_tspq);

	g_fCifInited = TRUE;
	g_fForceDecodeStop = FALSE;
}


