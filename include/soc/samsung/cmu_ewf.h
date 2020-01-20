#ifndef __CMU_EWF_H__
#define __CMU_EWF_H__

#define EARLY_WAKEUP_FORCED_ENABLE		(0x0870)
#define EWF_MAX_INDEX				(32)

extern int (*wa_set_cmuewf)(unsigned int index, unsigned int en, void *cmu_cmu, int *ewf_refcnt);
#ifdef CONFIG_CMU_EWF
extern int get_cmuewf_index(struct device_node *np, unsigned int *index);
extern int set_cmuewf(unsigned int index, unsigned int en);

#else
static inline int get_cmuewf_index(struct device_node *np, unsigned int *index)
{
	return 0;
}

static inline int set_cmuewf(unsigned int index, unsigned int en)
{
	return 0;
}
#endif
#endif
