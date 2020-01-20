#ifndef __PMUCAL_COMMON_H__
#define __PMUCAL_COMMON_H__

#include "pwrcal-env.h"

#ifdef PWRCAL_TARGET_LINUX
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/debug-snapshot.h>
#include <linux/delay.h>
#endif

#define PMUCAL_PREFIX		"PMUCAL:  "

/* represents the value in access_type column in guide */
enum pmucal_seq_acctype {
	PMUCAL_READ = 0,
	PMUCAL_WRITE,
	PMUCAL_COND_READ,
	PMUCAL_COND_WRITE,
	PMUCAL_SAVE_RESTORE,
	PMUCAL_COND_SAVE_RESTORE,
	PMUCAL_WAIT,
	PMUCAL_WAIT_TWO,
	PMUCAL_CHECK_SKIP,
	PMUCAL_COND_CHECK_SKIP,
	PMUCAL_WRITE_WAIT,
	PMUCAL_WRITE_RETRY,
	PMUCAL_WRITE_RETRY_INV,
	PMUCAL_WRITE_RETURN,
	PMUCAL_SET_BIT_ATOMIC,
	PMUCAL_CLR_BIT_ATOMIC,
	PMUCAL_DELAY,
	PMUCAL_CLEAR_PEND,
};

/* represents each row in the PMU sequence guide */
struct pmucal_seq {
	u32 access_type;
	char *sfr_name;
	phys_addr_t base_pa;
	void __iomem *base_va;
	u32 offset;
	u32 mask;
	u32 value;
	phys_addr_t cond_base_pa;
	void __iomem *cond_base_va;
	u32 cond_offset;
	u32 cond_mask;
	u32 cond_value;
	bool need_restore;
	bool need_skip;
};

#define PMUCAL_SEQ_DESC(_access_type, _sfr_name, _base_pa, _offset,	\
			_mask, _value, _cond_base_pa, _cond_offset,	\
			_cond_mask, _cond_value) {			\
	.access_type = _access_type,					\
	.sfr_name = _sfr_name,						\
	.base_pa = _base_pa,						\
	.base_va = NULL,						\
	.offset = _offset,						\
	.mask = _mask,							\
	.value = _value,						\
	.cond_base_pa = _cond_base_pa,					\
	.cond_base_va = NULL,						\
	.cond_offset = _cond_offset,					\
	.cond_mask = _cond_mask,					\
	.cond_value = _cond_value,					\
	.need_restore = false,						\
	.need_skip = false,						\
}

#define PMUCAL_CPU_INFORM(_cpu_num, _base_pa, _offset) {		\
	.cpu_num = _cpu_num,						\
	.base_pa = _base_pa,						\
	.base_va = NULL,						\
	.offset = _offset,						\
}

#endif
