#ifndef __VCLK_H__
#define __VCLK_H__

struct vclk_trans_ops {
	void (*trans_pre)(unsigned int from, unsigned int to);
	void (*trans_post)(unsigned int from, unsigned int to);
	void (*switch_pre)(unsigned int from, unsigned int to);
	void (*switch_post)(unsigned int from, unsigned int to);
	void (*switch_trans)(unsigned int from, unsigned int to);
	void (*restore_trans)(unsigned int from, unsigned int to);
	void (*set_pll)(unsigned int id, unsigned int to);
	unsigned int (*get_pll)(unsigned int id);
};

#ifdef CONFIG_CMUCAL
#define get_vclk(_id)		cmucal_get_node(_id | VCLK_TYPE)
#define get_clk_id(_name)	cmucal_get_id(_name)

extern int vclk_set_rate(unsigned int id, unsigned long rate);
extern int vclk_set_rate_switch(unsigned int id, unsigned long rate);
extern int vclk_set_rate_restore(unsigned int id, unsigned long rate);
extern unsigned long vclk_recalc_rate(unsigned int id);
extern unsigned long vclk_get_rate(unsigned int id);
extern int vclk_set_enable(unsigned int id);
extern int vclk_set_disable(unsigned int id);
extern int vclk_initialize(void);
extern unsigned int vclk_get_max_freq(unsigned int id);
extern unsigned int vclk_get_min_freq(unsigned int id);
extern unsigned int vclk_get_boot_freq(unsigned int id);
extern unsigned int vclk_get_resume_freq(unsigned int id);
extern unsigned int vclk_get_lv_num(unsigned int id);
extern int vclk_get_rate_table(unsigned int id, unsigned long *table);
extern int vclk_register_ops(unsigned int id, struct vclk_trans_ops *ops);
extern int vclk_get_bigturbo_table(unsigned int *table);
#else
static inline void *get_vclk(unsigned int id)
{
	return NULL;
}

static inline unsigned int get_clk_id(char *name)
{
	return 0UL;
}

static inline int vclk_set_rate(unsigned int id, unsigned long rate)
{
	return 0;
}

static inline int vclk_set_rate_switch(unsigned int id, unsigned long rate)
{
	return 0;
}

static inline int vclk_set_rate_restore(unsigned int id, unsigned long rate)
{
	return 0;
}

static inline unsigned long vclk_recalc_rate(unsigned int id)
{
	return 0UL;
}

static inline unsigned long vclk_get_rate(unsigned int id)
{
	return 0UL;
}

static inline int vclk_set_enable(unsigned int id)
{
	return 0;
}

static inline int vclk_set_disable(unsigned int id)
{
	return 0;
}

static inline unsigned int vclk_get_max_freq(unsigned int id)
{
	return 0;
}

static inline unsigned int vclk_get_min_freq(unsigned int id)
{
	return 0;
}

static inline unsigned int vclk_get_boot_freq(unsigned int id)
{
	return 0;
}

static inline unsigned int vclk_get_resume_freq(unsigned int id)
{
	return 0;
}

static inline unsigned int vclk_get_lv_num(unsigned int id)
{
	return 0;
}

static inline int vclk_initialize(void)
{
	return 0;
}

static inline int vclk_get_rate_table(unsigned int id, unsigned long *table)
{
	return 0;
}

static inline int vclk_register_ops(unsigned int id, struct vclk_trans_ops *ops)
{
	return 0;
}

static inline int vclk_get_bigturbo_table(unsigned int *table)
{
	return -EVCLKINVAL;
}
#endif

#ifdef CONFIG_CMUCAL_DEBUG
extern unsigned int vclk_debug_clk_get_rate(unsigned int id);
extern int vclk_debug_clk_set_value(unsigned int id, unsigned int params);
extern unsigned int vclk_debug_clk_get_value(unsigned int id);
#else
static inline unsigned int vclk_debug_clk_get_rate(unsigned int id)
{
	return 0;
}

static inline int vclk_debug_clk_set_value(unsigned int id, unsigned int params)
{
	return 0;
}

static inline unsigned int vclk_debug_clk_get_value(unsigned int id)
{
	return 0;
}
#endif
#endif
