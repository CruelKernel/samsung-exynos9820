#ifndef __CMUCAL_RA_H__
#define __CMUCAL_RA_H__

enum trans_opt {
	TRANS_IGNORE,
	TRANS_HIGH,
	TRANS_LOW,
	TRANS_FORCE,
};

#define SWITCH_TRANS		(0x1 << 0)
#define RESTORE_TRANS		(0x1 << 1)
#define ONESHOT_TRANS		(SWITCH_TRANS | RESTORE_TRANS)
#define is_switch_trans(cmd)	(cmd & SWITCH_TRANS)
#define is_restore_trans(cmd)	(cmd & RESTORE_TRANS)
#define is_oneshot_trans(cmd)	(cmd == ONESHOT_TRANS)

#ifndef MHZ
#define MHZ			((unsigned long long)1000000)
#endif
#ifndef KHZ
#define KHZ			((unsigned int)1000)
#endif

#define PLL_CON_ADDR_MASK	(0xF)
#define PLL_CON0_OFFSET		(0x0)
#define PLL_CON1_OFFSET		(0x4)
#define PLL_CON2_OFFSET		(0x8)
#define PLL_CON3_OFFSET		(0xC)

#define convert_pll_base(_addr) (void __iomem *)((unsigned long)_addr & ~PLL_CON_ADDR_MASK)

#define PLL_MUX_SEL_SHIFT	(4)
#define PLL_MUX_SEL		(0x1 << PLL_MUX_SEL_SHIFT)
#define PLL_MUX_BUSY_SHIFT	(7)
#define PLL_MUX_BUSY		(0x1 << PLL_MUX_BUSY_SHIFT)
#define PLL_STABLE_SHIFT	(29)
#define PLL_STABLE		(0x1 << PLL_STABLE_SHIFT)
#define PLL_ENABLE_SHIFT	(31)
#define PLL_ENABLE		(0x1 << PLL_ENABLE_SHIFT)

#define FIN_HZ_26M		(26*MHZ)
#define CLK_WAIT_CNT		1000
#define RECALC_MAX		32

#define khz_to_hz(rate)		(rate * 1000)
#define hz_to_khz(rate)		(rate / 1000)

#define width_to_mask(_w)	((1U << (_w))-1)
#define get_bit(addr, bitp)	((readl(addr) >> bitp) & 1)
#define get_value(addr, _s, _w)	((readl(addr) >> _s) & width_to_mask(_w))
#define get_mask(_w, _s)	(width_to_mask(_w) << _s)
#define clear_value(_a, _w, _s)	(readl(_a) & ~(get_mask(_w, _s)))

static inline int is_normal_pll(struct cmucal_pll *pll)
{
	if (pll->flock_time)
		return 0;

	return 1;
}

static inline int is_frac_pll(struct cmucal_pll *pll)
{
	if (pll->flock_time)
		return 1;

	return 0;
}

extern struct pll_spec *pll_get_spec(struct cmucal_pll *pll);
extern int pll_get_locktime(struct cmucal_pll *pll);
extern int pll_find_table(struct cmucal_pll *pll,
			  struct cmucal_pll_table *table,
			  unsigned long long fin,
			  unsigned long long rate);
extern void ra_set_clk_by_type(unsigned int *list,
			    struct vclk_lut *lut,
			    unsigned int num_list,
			    unsigned int type,
			    enum trans_opt opt);
extern void ra_set_clk_by_seq(unsigned int *list,
			   struct vclk_lut *lut,
			   struct vclk_seq *seq,
			   unsigned int num_list);
extern int ra_compare_clk_list(unsigned int *params,
			    unsigned int *list,
			    unsigned int num_list);
extern void ra_set_pll_ops(unsigned int *list,
			   struct vclk_lut *lut,
			   unsigned int num_list,
			   struct vclk_trans_ops *ops);
extern unsigned int ra_set_rate_switch(struct vclk_switch *info,
				       unsigned int max);
extern void ra_select_switch_pll(struct vclk_switch *info, unsigned int value);
extern int ra_set_list_enable(unsigned int *list, unsigned int num_list);
extern int ra_set_list_disable(unsigned int *list, unsigned int num_list);
extern struct cmucal_clk *ra_get_parent(unsigned int id);
extern unsigned int ra_recalc_rate(unsigned int id);
extern unsigned int ra_get_value(unsigned int id);
extern int ra_set_value(unsigned int id, unsigned int params);
extern int ra_set_rate(unsigned int id, unsigned int rate);
extern int ra_set_enable(unsigned int id, unsigned int params);
extern int ra_set_qch(unsigned int id, unsigned int en,
		unsigned int req, unsigned int expire);
extern int ra_set_enable_hwacg(struct cmucal_clk *clk, unsigned int en);

extern int ra_init(void);

#endif
