#ifndef __CAL_IF_H__
#define __CAL_IF_H__

#ifdef CONFIG_PWRCAL
#include "../../../drivers/soc/samsung/pwrcal/pwrcal.h"

static inline int cal_qch_init(unsigned int vclkid, unsigned int use_qch)
{
	return 0;
}

static inline int cal_if_init(void)
{
	return 0;
}
#else
#include "../../../drivers/soc/samsung/cal-if/pmucal_system.h"

#define BLKPWR_MAGIC	0xB1380000

extern unsigned int cal_clk_get(char *name);
extern unsigned int cal_clk_is_enabled(unsigned int vclkid);
extern int cal_clk_setrate(unsigned int vclkid, unsigned long rate);
extern unsigned long cal_clk_getrate(unsigned int vclkid);
extern int cal_clk_enable(unsigned int vclkid);
extern int cal_clk_disable(unsigned int vclkid);
extern int cal_qch_init(unsigned int vclkid, unsigned int use_qch);

extern int cal_pd_control(unsigned int id, int on);
extern int cal_pd_status(unsigned int id);
extern int cal_pd_set_smc_id(unsigned int id, int need_smc);


extern int cal_pm_enter(int mode);
extern int cal_pm_exit(int mode);
extern int cal_pm_earlywakeup(int mode);

extern int cal_cpu_enable(unsigned int cpu);
extern int cal_cpu_disable(unsigned int cpu);
extern int cal_cpu_status(unsigned int cpu);
extern int cal_cluster_enable(unsigned int cluster);
extern int cal_cluster_disable(unsigned int cluster);
extern int cal_cluster_status(unsigned int cluster);
extern int cal_cluster_req_emulation(unsigned int cluster, bool en);
extern int cal_is_lastcore_detecting(unsigned int cpu);

extern unsigned int cal_dfs_get(char *name);
extern unsigned long cal_dfs_get_max_freq(unsigned int id);
extern unsigned long cal_dfs_get_min_freq(unsigned int id);
extern int cal_dfs_set_rate(unsigned int id, unsigned long rate);
extern int cal_dfs_set_rate_switch(unsigned int id, unsigned long switch_rate);
extern unsigned long cal_dfs_cached_get_rate(unsigned int id);
extern unsigned long cal_dfs_get_rate(unsigned int id);
extern int cal_dfs_get_rate_table(unsigned int id, unsigned long *table);
extern int cal_dfs_get_asv_table(unsigned int id, unsigned int *table);
extern int cal_dfs_get_bigturbo_max_freq(unsigned int *table);
extern unsigned int cal_dfs_get_boot_freq(unsigned int id);
extern unsigned int cal_dfs_get_resume_freq(unsigned int id);
extern unsigned int cal_dfs_get_lv_num(unsigned int id);
extern unsigned int cal_asv_pmic_info(void);

struct dvfs_rate_volt {
	unsigned long rate;
	unsigned int volt;
};
int cal_dfs_get_rate_asv_table(unsigned int id,
					struct dvfs_rate_volt *table);
extern void cal_dfs_set_volt_margin(unsigned int id, int volt);
extern unsigned long cal_dfs_get_rate_by_member(unsigned int id,
							char *member,
							unsigned long rate);
extern int cal_dfs_set_ema(unsigned int id, unsigned int volt);

enum cal_dfs_ext_ops {
	cal_dfs_initsmpl		= 0,
	cal_dfs_setsmpl		= 1,
	cal_dfs_get_smplstatus	= 2,
	cal_dfs_deinitsmpl	= 3,

	cal_dfs_dvs = 30,

	/* Add new ops at below */
	cal_dfs_mif_is_dll_on	= 50,

	cal_dfs_cpu_idle_clock_down = 60,

	cal_dfs_ctrl_clk_gate	= 70,
};

extern int cal_dfs_ext_ctrl(unsigned int id,
				enum cal_dfs_ext_ops ops,
				int para);

extern int cal_asv_get_ids_info(unsigned int id);
extern int cal_asv_get_grp(unsigned int id);
extern int cal_asv_get_tablever(void);

extern int cal_cp_init(void);
extern int cal_cp_status(void);
extern int cal_cp_reset_assert(void);
extern int cal_cp_reset_release(void);
extern void cal_cp_active_clear(void);
extern void cal_cp_reset_req_clear(void);
extern void cal_cp_enable_dump_pc_no_pg(void);
extern void cal_cp_disable_dump_pc_no_pg(void);

extern int cal_init(void);
extern int cal_if_init(void *);

/* It is for debugging. */
#define cal_vclk_dbg_info(a)	do{} while(0);
//extern void cal_vclk_dbg_info(unsigned int id);
#endif
#endif
