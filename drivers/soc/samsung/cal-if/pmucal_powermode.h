#ifndef __PMUCAL_POWERMODE_H___
#define __PMUCAL_POWERMODE_H___

#ifdef CONFIG_FLEXPMU
#ifdef PWRCAL_TARGET_LINUX
#include <linux/io.h>
#endif

extern void pmucal_powermode_hint(unsigned int mode);
extern void pmucal_powermode_hint_clear(void);
extern int pmucal_cpuinform_init(void);
extern u32 pmucal_is_lastcore_detecting(unsigned int cpu);
#else

#define pmucal_powermode_hint(a) do {} while (0)
#define pmucal_powermode_hint_clear() do {} while (0)
static inline int pmucal_cpuinform_init(void)
{
	return 0;
}

#endif

struct cpu_inform {
	unsigned int cpu_num;
	phys_addr_t base_pa;
	void __iomem *base_va;
	u32 offset;
};

extern struct cpu_inform pmucal_cpuinform_list[];
extern unsigned int cpu_inform_list_size;

#endif
