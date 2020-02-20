#ifndef	__EXYNOS_HIU_H__
#define	__EXYNOS_HIU_H__

#include <linux/cpufreq.h>
#include <linux/interrupt.h>

/* Function Id to Enable HIU in EL3 */
#define GCU_BASE		(0x1E4C0000)

/* SR1 write monitoring operation mode */
#define	POLLING_MODE		(0)
#define	INTERRUPT_MODE		(1)

/* GCU Control Register */
#define	GCUCTL			(0x22C)
#define	HIUINTR_EN_MASK		(0x1)
#define	HIUINTR_EN_SHIFT	(3)
#define	HIUERR_EN_MASK		(0x1)
#define	HIUERR_EN_SHIFT		(2)

/* GCU ERROR Register */
#define	GCUERR			(0x26C)
#define	HIUINTR_MASK		(0x1)
#define	HIUINTR_SHIFT		(3)
#define	HIUERR_MASK		(0x1)
#define	HIUERR_SHIFT		(2)

/* HIU Top Level Control 1 Register */
#define	HIUTOPCTL1		(0xE00)
#define	ACTDVFS_MASK		(0x3F)
#define	ACTDVFS_SHIFT		(24)
#define	HIU_MBOX_RESPONSE_MASK	(0x1)
#define	SR1INTR_SHIFT		(15)
#define	SR1UXPERR_MASK		(1 << 3)
#define	SR1SNERR_MASK		(1 << 2)
#define	SR1TIMEOUT_MASK		(1 << 1)
#define	SR0RDERR_MASK		(1 << 0)
#define	HIU_MBOX_ERR_MASK	(0xF)
#define	HIU_MBOX_ERR_SHIFT	(11)
#define	ENB_SR1INTR_MASK	(0x1)
#define ENB_SR1INTR_SHIFT	(5)
#define ENB_ERR_INTERRUPTS_MASK	(0x1E)
#define	ENB_SR1UXPERR_MASK	(0x1)
#define ENB_SR1UXPERR_SHIFT	(4)
#define	ENB_SR1SNERR_MASK	(0x1)
#define ENB_SR1SNERR_SHIFT	(3)
#define	ENB_SR1TIMEOUT_MASK	(0x1)
#define ENB_SR1TIMEOUT_SHIFT	(2)
#define	ENB_SR0RDERR_MASK	(0x1)
#define ENB_SR0RDERR_SHIFT	(1)
#define	ENB_ACPM_COMM_MASK	(0x1)
#define	ENB_ACPM_COMM_SHIFT	(0)

/* HIU Top Level Control 2 Register */
#define HIUTOPCTL2		(0xE04)
#define	SEQNUM_MASK		(0x7)
#define	SEQNUM_SHIFT		(26)
#define LIMITDVFS_MASK		(0x3F)
#define LIMITDVFS_SHIFT		(14)
#define	REQDVFS_MASK		(0x3F)
#define	REQDVFS_SHIFT		(8)
#define	REQPBL_MASK		(0xFF)
#define	REQPBL_SHIFT		(0)

/* HIU Turbo Boost Control Register */
#define	HIUTBCTL		(0xE10)
#define	BOOSTED_MASK		(0x1)
#define	BOOSTED_SHIFT		(31)
#define	B3_INC_MASK		(0x3)
#define	B3_INC_SHIFT		(20)
#define	B2_INC_MASK		(0x3)
#define	B2_INC_SHIFT		(18)
#define	B1_INC_MASK		(0x3)
#define	B1_INC_SHIFT		(16)
#define	TBDVFS_MASK		(0x3F)
#define TBDVFS_SHIFT		(8)
#define	PC_DISABLE_MASK		(0x1)
#define	PC_DISABLE_SHIFT	(1)
#define	TB_ENB_MASK		(0x1)
#define	TB_ENB_SHIFT		(0)

/* HIU Turbo Boost Power State Config Registers */
#define	HIUTBPSCFG_BASE		(0xE14)
#define	HIUTBPSCFG_OFFSET	(0x4)
#define	HIUTBPSCFG_MASK		(0x1 << 31 | 0x7 << 28 | 0xFF << 20 | 0xFFFF << 0x0)
#define	PB_MASK			(0x1)
#define	PB_SHIFT		(31)
#define	BL_MASK			(0x7)
#define	BL_SHIFT		(28)
#define	PBL_MASK		(0xFF)
#define	PBL_SHIFT		(20)
#define	TBPWRTHRESH_INC_MASK	(0xFFFF)
#define	TBPWRTHRESH_INC_SHIFT	(0x0)

/* HIU Turbo Boost Power Threshold Register */
#define	HIUTBPWRTHRESH_NUM	(0x6)
#define	HIUTBPWRTHRESH_FIELDS	(0x4)
#define	HIUTBPWRTHRESH_BASE	(0xE34)
#define	HIUTBPWRTHRESH_OFFSET	(0x4)
#define	HIUTBPWRTHRESH_MASK	(0x3F << 24 | 0xF << 20 | 0xF << 16 | 0xFFFF << 0)
#define	R_MASK			(0x3F)
#define	R_SHIFT			(24)
#define	MONINTERVAL_MASK	(0xF)
#define	MONINTERVAL_SHIFT	(20)
#define	TBPWRTHRESH_EXP_MASK	(0xF)
#define	TBPWRTHRESH_EXP_SHIFT	(16)
#define	TBPWRTHRESH_FRAC_MASK	(0xFFFF)
#define	TBPWRTHRESH_FRAC_SHIFT	(0)

struct hiu_stats {
	unsigned int		last_level;
	unsigned int		*freq_table;
	unsigned long long	*time_in_state;
};

struct hiu_tasklet_data {
	unsigned int		index;
};

struct hiu_cfg {
	unsigned int		power_borrowed;
	unsigned int		boost_level;
	unsigned int		power_budget_limit;
	unsigned int		power_threshold_inc;
};

struct exynos_hiu_data {
	bool			enabled;

	bool			normdvfs_req;
	bool			normdvfs_done;
	bool			hwidvfs_done;
	bool			boosting_activated;
	bool			cpd_blocked;

	int			operation_mode;

	int			irq;
	struct work_struct	work;
	struct work_struct	hwi_work;
	struct mutex		lock;
	wait_queue_head_t	normdvfs_wait;
	wait_queue_head_t	hwidvfs_wait;
	wait_queue_head_t	polling_wait;

	struct cpumask		cpus;
	unsigned int		cpu;

	unsigned int		cal_id;

	unsigned int		cur_freq;
	unsigned int		norm_target_freq;
	unsigned int		clipped_freq;
	unsigned int		boost_threshold;
	unsigned int		boost_max;
	unsigned int		level_offset;
	unsigned int		sw_pbl;
	unsigned int		pc_enabled;
	unsigned int		tb_enabled;

	unsigned int		last_req_level;
	unsigned int		last_req_freq;

	void __iomem *		base;

	struct device_node *	dn;
	struct hiu_stats *	stats;
};

#if defined(CONFIG_EXYNOS_PSTATE_HAFM) || defined(CONFIG_EXYNOS_PSTATE_HAFM_TB)
extern int exynos_hiu_set_freq(unsigned int id, unsigned int req_freq);
extern int exynos_hiu_get_freq(unsigned int id);
extern int exynos_hiu_get_max_freq(void);
#else
inline int exynos_hiu_set_freq(unsigned int id, unsigned int req_freq) { return 0; }
inline int exynos_hiu_get_freq(unsigned int id) { return 0; }
inline int exynos_hiu_get_max_freq(void) { return 0; }
#endif

#endif
