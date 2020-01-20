#ifndef __PMUCAL_CP_H__
#define __PMUCAL_CP_H__
#include "pmucal_common.h"

#if defined(CONFIG_SOC_EXYNOS9820)
#define PMU_CP_CTRL_NS_OFFSET	0x3010
#define PMU_CP_CTRL_S_OFFSET	0x3014
#define PMU_CP_STATUS_BIT	0x1
#else
#define PMU_CP_CTRL_NS_OFFSET	0x30
#define PMU_CP_CTRL_S_OFFSET	0x34
#define PMU_CP_STATUS_BIT	0x5
#endif

#define SMC_ID		0x82000700
#define READ_CTRL	0x3
#define WRITE_CTRL	0x4

enum cp_control {
	CP_CTRL_S,
	CP_CTRL_NS,
};

struct pmucal_cp {
	struct pmucal_seq *init;
	struct pmucal_seq *status;
	struct pmucal_seq *reset_assert;
	struct pmucal_seq *reset_release;
	struct pmucal_seq *cp_active_clear;
	struct pmucal_seq *cp_reset_req_clear;
	struct pmucal_seq *cp_enable_dump_pc_no_pg;
	struct pmucal_seq *cp_disable_dump_pc_no_pg;
	u32 num_init;
	u32 num_status;
	u32 num_reset_assert;
	u32 num_reset_release;
	u32 num_cp_active_clear;
	u32 num_cp_reset_req_clear;
	u32 num_cp_enable_dump_pc_no_pg;
	u32 num_cp_disable_dump_pc_no_pg;
};

/* APIs to be supported to PWRCAL interface */
extern int pmucal_cp_initialize(void);

extern int pmucal_cp_init(void);
extern int pmucal_cp_status(void);
extern int pmucal_cp_reset_assert(void);
extern int pmucal_cp_reset_release(void);
extern int pmucal_cp_active_clear(void);
extern int pmucal_cp_reset_req_clear(void);
extern int pmucal_cp_enable_dump_pc_no_pg(void);
extern int pmucal_cp_disable_dump_pc_no_pg(void);

extern struct pmucal_cp pmucal_cp_list;
extern unsigned int pmucal_cp_list_size;
#endif
