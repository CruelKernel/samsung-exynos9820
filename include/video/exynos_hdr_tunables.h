#ifndef _LINUX_EXYNOS_HDR_TUNABLES_H
#define _LINUX_EXYNOS_HDR_TUNABLES_H

#include <linux/types.h>

#define NR_TM_LUT_VALUES 33

#ifdef CONFIG_EXYNOS_HDR_TUNABLE_TONEMAPPING
int exynos_hdr_get_tm_lut_xy(u32 lut_x[], u32 lut_y[]);
int exynos_hdr_get_tm_lut(u32 lut[]);
#else
static inline int exynos_hdr_get_tm_lut_xy(u32 lut_x[], u32 lut_y[])
{
	return 0;
}
static inline int exynos_hdr_get_tm_lut(u32 lut[])
{
	return 0;
}
#endif

#endif /* _LINUX_EXYNOS_HDR_TUNABLES_H */
