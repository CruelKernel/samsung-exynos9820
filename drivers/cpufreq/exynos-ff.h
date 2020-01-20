#ifndef	__EXYNOS_FF_H__
#define	__EXYNOS_FF_H__

struct exynos_ff_driver {
	bool			big_dvfs_done;

	unsigned int		boost_threshold;
	unsigned int		cal_id;

	struct mutex		lock;
	struct cpumask		cpus;
};

#ifdef CONFIG_EXYNOS_PSTATE_HAFM_TB
extern bool hwi_dvfs_req;
extern atomic_t boost_throttling;
#endif

#endif
