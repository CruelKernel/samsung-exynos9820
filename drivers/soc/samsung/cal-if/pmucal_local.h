#ifndef __PMUCAL_LOCAL_H__
#define __PMUCAL_LOCAL_H__
#include "pmucal_common.h"
#include "pmucal_dbg.h"

/* In Exynos, the number of MAX_POWER_DOMAIN is less than 20 */
#define PMUCAL_NUM_PDS	20

/* will be a member of struct exynos_pm_domain */
struct pmucal_pd {
	u32 id;
	char *name;
	struct pmucal_seq *on;
	struct pmucal_seq *save;
	struct pmucal_seq *off;
	struct pmucal_seq *status;
	u32 num_on;
	u32 num_save;
	u32 num_off;
	u32 num_status;
	u32 need_smc;
	struct pmucal_dbg_info *dbg;
};

#define PMUCAL_PD_DESC(_pd_id, _name, _on, _save, _off, _status)	\
	[_pd_id] = {							\
		.id = _pd_id,						\
		.name = _name,						\
		.on = _on,						\
		.save = _save,						\
		.off = _off,						\
		.status = _status,					\
		.num_on = ARRAY_SIZE(_on),				\
		.num_save = ARRAY_SIZE(_save),				\
		.num_off = ARRAY_SIZE(_off),				\
		.num_status = ARRAY_SIZE(_status),			\
		.need_smc = 0,						\
	}

/* APIs to be supported to PWRCAL interface */
extern int pmucal_local_enable(unsigned int pd_id);
extern int pmucal_local_disable(unsigned int pd_id);
extern int pmucal_local_is_enabled(unsigned int pd_id);
extern int pmucal_local_init(void);
extern void pmucal_local_set_smc_id(unsigned int pd_id, unsigned int need_smc);

extern struct pmucal_pd pmucal_pd_list[];
extern unsigned int pmucal_pd_list_size;
#endif
