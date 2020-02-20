#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <soc/samsung/ect_parser.h>
#include <soc/samsung/exynos-pmu.h>

#include "cmucal.h"
#include "ra.h"

static enum trans_opt ra_get_trans_opt(unsigned int to, unsigned int from)
{
	if (from == to)
		return TRANS_IGNORE;

	return to > from ? TRANS_HIGH : TRANS_LOW;
}

static int ra_wait_done(void __iomem *reg,
			unsigned char shift,
			unsigned int done,
			int usec)
{
	unsigned int result;

	do {
		result = get_bit(reg, shift);

		if (result == done)
			return 0;
		udelay(1);
	} while (--usec > 0);

	return -EVCLKTIMEOUT;
}

static unsigned int ra_get_fixed_rate(struct cmucal_clk *clk)
{
	struct cmucal_clk_fixed_rate *frate = to_fixed_rate_clk(clk);
	void __iomem *offset;
	unsigned int rate;

	if (!clk->enable)
		return frate->fixed_rate;

	offset = convert_pll_base(clk->enable);

	if (readl(offset) & PLL_MUX_SEL)
		rate = frate->fixed_rate;
	else
		rate = FIN_HZ_26M;

	return rate;
}

static unsigned int ra_get_fixed_factor(struct cmucal_clk *clk)
{
	struct cmucal_clk_fixed_factor *ffacor = to_fixed_factor_clk(clk);

	return ffacor->ratio;
}

static struct cmucal_pll_table *get_pll_table(struct cmucal_pll *pll_clk,
					      unsigned long rate,
					      unsigned long rate_hz)
{
	struct cmucal_pll_table *prate_table = pll_clk->rate_table;
	int i;

	if (rate_hz) {
		unsigned long matching = rate_hz;

		/* Skip Hz unit matching. It is too ideal. */

		/* 10Hz unit */
		do_div(matching, 10);
		for (i = 0; i < pll_clk->rate_count; i++) {
			if (matching == prate_table[i].rate / 10)
				return &prate_table[i];
		}

		/* Fallback: 100Hz unit */
		do_div(matching, 10);
		for (i = 0; i < pll_clk->rate_count; i++) {
			if (matching == prate_table[i].rate / 100)
				return &prate_table[i];
		}

		/* Fallback: 1000Hz unit */
	}

	for (i = 0; i < pll_clk->rate_count; i++) {
		if (rate == prate_table[i].rate / 1000)
			return &prate_table[i];
	}

	return NULL;
}

static int ra_is_pll_enabled(struct cmucal_clk *clk)
{
	return get_bit(clk->pll_con0, clk->e_shift);
}

static int ra_enable_pll(struct cmucal_clk *clk, int enable)
{
	unsigned int reg;
	int ret = 0;

	reg = readl(clk->pll_con0);
	if (!enable) {
		reg &= ~(PLL_MUX_SEL);
		writel(reg, clk->pll_con0);

		ret = ra_wait_done(clk->pll_con0, PLL_MUX_BUSY_SHIFT, 0, 100);
		if (ret)
			pr_err("pll mux change time out, \'%s\'\n", clk->name);
	}

	if (enable)
		reg |= 1 << clk->e_shift;
	else
		reg &= ~(1 << clk->e_shift);

	writel(reg, clk->pll_con0);

	if (enable) {
		ret = ra_wait_done(clk->pll_con0, clk->s_shift, 1, 100);
		if (ret)
			pr_err("pll time out, \'%s\' %d\n", clk->name, enable);
	}

	return ret;
}

static int ra_pll_set_pmsk(struct cmucal_clk *clk,
			       struct cmucal_pll_table *rate_table)
{
	struct cmucal_pll *pll = to_pll_clk(clk);
	unsigned int mdiv, pdiv, sdiv, pll_con0, pll_con1;
	signed short kdiv;
	int ret = 0;

	pdiv = rate_table->pdiv;
	mdiv = rate_table->mdiv;
	sdiv = rate_table->sdiv;

	if (!clk->pll_con0)
		return -EVCLKNOENT;

	pll_con0 = readl(clk->pll_con0);
	pll_con0 &= ~(get_mask(pll->m_width, pll->m_shift)
			| get_mask(pll->p_width, pll->p_shift)
			| get_mask(pll->s_width, pll->s_shift));
	pll_con0 |=  (mdiv << pll->m_shift
			| pdiv << pll->p_shift
			| sdiv << pll->s_shift);

	pll_con0 |= PLL_MUX_SEL | (1 << clk->e_shift);

	if (is_frac_pll(pll)) {
		kdiv = rate_table->kdiv;
		if (kdiv)
			writel(pdiv * pll->flock_time, clk->lock);
		else
			writel(pdiv * pll->lock_time, clk->lock);

		if (clk->pll_con1) {
			pll_con1 = readl(clk->pll_con1);
			pll_con1 &= ~(get_mask(pll->k_width, pll->k_shift));
			pll_con1 |= (kdiv << pll->k_shift);
			writel(pll_con1, clk->pll_con1);
		}
	} else {
		writel(pdiv * pll->lock_time, clk->lock);
	}

	writel(pll_con0, clk->pll_con0);

	ret = ra_wait_done(clk->pll_con0, clk->s_shift, 1, 100);
	if (ret)
		pr_err("time out, \'%s\'", clk->name);

	return ret;
}

static int ra_get_div_mux(struct cmucal_clk *clk)
{
	if (!clk->offset)
		return 0;

	return get_value(clk->offset, clk->shift, clk->width);
}

static int ra_set_div_mux(struct cmucal_clk *clk, unsigned int params)
{
	unsigned int reg;
	int ret = 0;

	if (!clk->offset)
		return 0;

	reg = clear_value(clk->offset, clk->width, clk->shift);
	writel(reg | (params << clk->shift), clk->offset);

	if (clk->status == NULL)
		return 0;

	ret = ra_wait_done(clk->status, clk->s_shift, 0, 100);
	if (ret) {
		pr_err("time out, \'%s\' [%p]=%x [%p]=%x\n",
			clk->name,
			clk->offset, readl(clk->offset),
			clk->status, readl(clk->status));
	}

	return ret;
}

static int ra_set_mux_rate(struct cmucal_clk *clk, unsigned int rate)
{
	struct cmucal_mux *mux;
	unsigned int p_rate, sel = 0;
	unsigned int diff, min_diff = 0xFFFFFFFF;
	int i;
	int ret = -EVCLKINVAL;

	if (rate == 0)
		return ret;

	mux = to_mux_clk(clk);

	for (i = 0; i < mux->num_parents; i++) {
		p_rate = ra_recalc_rate(mux->pid[i]);

		if (p_rate == rate) {
			sel = i;
			break;
		}

		diff = abs(p_rate - rate);
		if (diff < min_diff) {
			min_diff = diff;
			sel = i;
		}
	}

	if (i == mux->num_parents)
		pr_debug("approximately select %s %u:%u:%u\n",
			 clk->name, rate, min_diff, sel);
	ret = ra_set_div_mux(clk, sel);

	return ret;
}

static int ra_set_div_rate(struct cmucal_clk *clk, unsigned int rate)
{
	unsigned int p_rate;
	unsigned int ratio, max_ratio;
	unsigned int diff1, diff2;
	int ret = -EVCLKINVAL;

	if (rate == 0)
		return ret;

	p_rate = ra_recalc_rate(clk->pid);

	if (p_rate == 0)
		return ret;

	max_ratio = width_to_mask(clk->width) + 1;

	ratio = p_rate / rate;

	if (ratio > 0 && ratio <= max_ratio) {
		if (p_rate % rate) {
			diff1 = p_rate - (ratio * rate);
			diff2 = ratio * rate + rate - p_rate;
			if (diff1 > diff2) {
				ret = ra_set_div_mux(clk, ratio);
				return ret;
			}
		}

		ret = ra_set_div_mux(clk, ratio - 1);
	} else if (ratio == 0) {
		ret = ra_set_div_mux(clk, ratio);
	} else {
		pr_err("failed div_rate %s %u:%u:%u:%u\n",
			clk->name, p_rate, rate, ratio, max_ratio);
	}

	return ret;
}

static int ra_set_pll(struct cmucal_clk *clk, unsigned int rate,
		      unsigned int rate_hz)
{
	struct cmucal_pll *pll;
	struct cmucal_pll_table *rate_table;
	struct cmucal_pll_table table;
	struct cmucal_clk *umux;
	unsigned int fin;
	int ret = 0;

	pll = to_pll_clk(clk);

	if (rate == 0) {
		if (pll->umux != EMPTY_CLK_ID) {
			umux = cmucal_get_node(pll->umux);
			if (umux)
				ra_set_div_mux(umux, 0);
		}
		ra_enable_pll(clk, 0);
	} else {
		rate_table = get_pll_table(pll, rate, rate_hz);
		if (!rate_table) {
			if (IS_FIXED_RATE(clk->pid))
				fin = ra_get_value(clk->pid);
			else
				fin = FIN_HZ_26M;

			ret = pll_find_table(pll, &table, fin, rate, rate_hz);
			if (ret) {
				pr_err("failed %s table %u\n", clk->name, rate);
				return ret;
			}
			rate_table = &table;
		}
		ra_enable_pll(clk, 0);
		ret = ra_pll_set_pmsk(clk, rate_table);

		if (pll->umux != EMPTY_CLK_ID) {
			umux = cmucal_get_node(pll->umux);
			if (umux)
				ra_set_div_mux(umux, 1);
		}
	}

	return ret;
}

static unsigned int ra_get_pll(struct cmucal_clk *clk)
{
	struct cmucal_pll *pll;
	unsigned int mdiv, pdiv, sdiv, pll_con0;
	short kdiv;
	unsigned long long fout;

	if (!ra_is_pll_enabled(clk)) {
		pll_con0 = readl(clk->pll_con0);
		if (pll_con0 & PLL_MUX_SEL)
			return 0;
		else
			return FIN_HZ_26M;
	}

	pll = to_pll_clk(clk);

	pll_con0 = readl(clk->pll_con0);
	mdiv = (pll_con0 >> pll->m_shift) & width_to_mask(pll->m_width);
	pdiv = (pll_con0 >> pll->p_shift) & width_to_mask(pll->p_width);
	sdiv = (pll_con0 >> pll->s_shift) & width_to_mask(pll->s_width);

	if (IS_FIXED_RATE(clk->pid))
		fout = ra_get_value(clk->pid);
	else
		fout = FIN_HZ_26M;

	if (is_normal_pll(pll)) {
		fout *= mdiv;
		do_div(fout, (pdiv << sdiv));
	} else if (is_frac_pll(pll) && clk->pll_con1) {
		kdiv = get_value(clk->pll_con1, pll->k_shift, pll->k_width);
		fout *= (mdiv << 16) + kdiv;
		do_div(fout, (pdiv << sdiv));
		fout >>= 16;
	} else {
		pr_err("Un-support PLL type\n");
		fout = 0;
	}

	return fout;
}

static unsigned int ra_get_pll_idx(struct cmucal_clk *clk)
{
	struct cmucal_pll *pll;
	struct cmucal_pll_table *prate_table;
	unsigned int mdiv, pdiv, sdiv, pll_con0;
	int i;

	pll = to_pll_clk(clk);
	prate_table = pll->rate_table;

	pll_con0 = readl(clk->pll_con0);
	mdiv = (pll_con0 >> pll->m_shift) & width_to_mask(pll->m_width);
	pdiv = (pll_con0 >> pll->p_shift) & width_to_mask(pll->p_width);
	sdiv = (pll_con0 >> pll->s_shift) & width_to_mask(pll->s_width);

	for (i = 0; i < pll->rate_count; i++) {
		if (mdiv != prate_table[i].mdiv)
			continue;
		if (pdiv != prate_table[i].pdiv)
			continue;
		if (sdiv != prate_table[i].sdiv)
			continue;

		return i;
	}

	return -1;
}

static int ra_set_gate(struct cmucal_clk *clk, unsigned int pass)
{
	unsigned int reg;

	/*
	 * MANUAL(status) 1 : CG_VALUE(offset) control
	 *                0 : ENABLE_AUTOMATIC_CLKGATING(enable) control
	 */
	if (!clk->status || get_bit(clk->status, clk->s_shift)) {
		reg = readl(clk->offset);
		reg &= ~(get_mask(clk->width, clk->shift));
		if (pass)
			reg |= get_mask(clk->width, clk->shift);

		writel(reg, clk->offset);
	} else {
		reg = readl(clk->enable);
		reg &= ~(get_mask(clk->e_width, clk->e_shift));
		if (!pass)
			reg |= get_mask(clk->e_width, clk->e_shift);

		writel(reg, clk->offset);
	}
	return 0;
}

static unsigned int ra_get_gate(struct cmucal_clk *clk)
{
	unsigned int pass;

	/*
	 * MANUAL(status) 1 : CG_VALUE(offset) control
	 *                0 : ENABLE_AUTOMATIC_CLKGATING(enable) control
	 */
	if (!clk->status || get_bit(clk->status, clk->s_shift))
		pass = get_value(clk->offset, clk->shift, clk->width);
	else
		pass = !(get_value(clk->enable, clk->e_shift, clk->e_width));

	return pass;
}

/*
 * en : qch enable bit
 * req : qch request bit
 * expire == 0 => default value
 * expire != 0 => change value
 */
int ra_set_qch(unsigned int id, unsigned int en,
		unsigned int req, unsigned int expire)
{
	struct cmucal_qch *qch;
	struct cmucal_clk *clk;
	unsigned int reg;

	clk = cmucal_get_node(id);
	if (!clk) {
		pr_err("%s:[%x]\n", __func__, id);
		return -EVCLKINVAL;
	}

	if (!IS_QCH(clk->id)) {
		if (IS_GATE(clk->id)) {
			reg = readl(clk->status);
			reg &= ~(get_mask(clk->s_width, clk->s_shift));
			if (!en)
				reg |= get_mask(clk->s_width, clk->s_shift);

			writel(reg, clk->status);
			return 0;
		}

		pr_err("%s:cannot find qch [%x]\n", __func__, id);
		return -EVCLKINVAL;
	}

	if (expire) {
		reg = ((en & width_to_mask(clk->width)) << clk->shift) |
		      ((req & width_to_mask(clk->s_width)) << clk->s_shift) |
		      ((expire & width_to_mask(clk->e_width)) << clk->e_shift);
	} else {
		reg = readl(clk->offset);
		reg &= ~(get_mask(clk->width, clk->shift) |
			get_mask(clk->s_width, clk->s_shift));
		reg |= (en << clk->shift) | (req << clk->s_shift);
	}

	if (IS_ENABLED(CONFIG_CMUCAL_QCH_IGNORE_SUPPORT)) {
		qch = to_qch(clk);

		if (en)
			reg &= ~(0x1 << qch->ig_shift);
		else
			reg |= (0x1 << qch->ig_shift);
	}

	writel(reg, clk->offset);

	return 0;
}

static int ra_req_enable_qch(struct cmucal_clk *clk, unsigned int req)
{
	unsigned int reg;
	/*
	 * QH ENABLE(offset) 1 : Skip
	 *		     0 : REQ(status) control
	 */
	if (!get_bit(clk->offset, clk->shift)) {
		reg = readl(clk->status);
		reg &= ~(get_mask(clk->s_width, clk->s_shift));
		if (req)
			reg |= get_mask(clk->s_width, clk->s_shift);

		writel(reg, clk->status);
	}

	return 0;
}

int ra_enable_qch(struct cmucal_clk *clk, unsigned int en)
{
	unsigned int reg;
	/*
	 * QH ENABLE(offset)
	 */
	reg = readl(clk->offset);
	reg &= ~(get_mask(clk->width, clk->shift));
	if (en)
		reg |= get_mask(clk->width, clk->shift);

	writel(reg, clk->offset);

	return 0;
}

int ra_set_enable_hwacg(struct cmucal_clk *clk, unsigned int en)
{
	unsigned int reg;

	/*
	 * Automatic clkgating enable(enable)
	 */
	if (!clk->enable)
		return 0;

	reg = readl(clk->enable);
	reg &= ~(get_mask(clk->e_width, clk->e_shift));
	if (en)
		reg |= get_mask(clk->s_width, clk->s_shift);

	writel(reg, clk->enable);

	return 0;
}

static int ra_enable_fixed_rate(struct cmucal_clk *clk, unsigned int params)
{
	unsigned int reg;
	void __iomem *offset;
	int ret;

	if (!clk->enable)
		return 0;

	offset = convert_pll_base(clk->enable);
	reg = readl(offset);
	if (params) {
		reg |= (PLL_ENABLE | PLL_MUX_SEL);
		writel(reg, offset);

		ret = ra_wait_done(offset, PLL_STABLE_SHIFT, 1, 400);
		if (ret)
			pr_err("fixed pll enable time out, \'%s\'\n", clk->name);
	} else {
		reg &= ~(PLL_MUX_SEL);
		writel(reg, offset);
		ret = ra_wait_done(offset, PLL_MUX_BUSY_SHIFT, 0, 100);
		if (ret)
			pr_err("fixed pll mux change time out, \'%s\'\n", clk->name);

		reg &= ~(PLL_ENABLE);
		writel(reg, offset);
	}

	return 0;
}

int ra_enable_clkout(struct cmucal_clk *clk, bool enable)
{
	struct cmucal_clkout *clkout = to_clkout(clk);

	if (enable) {
		exynos_pmu_update(clk->offset_idx, get_mask(clk->width, clk->shift),
				clkout->sel << clk->shift);
		exynos_pmu_update(clk->offset_idx, get_mask(clk->e_width, clk->e_shift),
				0x0 << clk->e_shift);
	} else {
		exynos_pmu_update(clk->offset_idx, get_mask(clk->e_width, clk->e_shift),
				0x1 << clk->e_shift);
	}

	return 0;
}

int ra_set_enable(unsigned int id, unsigned int params)
{
	struct cmucal_clk *clk;
	unsigned type = GET_TYPE(id);
	int ret = 0;

	clk = cmucal_get_node(id);
	if (!clk) {
		pr_err("%s:[%x]type : %x, params : %x\n",
			__func__, id, type, params);
		return -EVCLKINVAL;
	}

	switch (type) {
	case FIXED_RATE_TYPE:
		ret = ra_enable_fixed_rate(clk, params);
		break;
	case PLL_TYPE:
		ret = ra_enable_pll(clk, params);
		break;
	case MUX_TYPE:
		if (IS_USER_MUX(clk->id))
			ret = ra_set_div_mux(clk, params);
		break;
	case GATE_TYPE:
		ret = ra_set_gate(clk, params);
		break;
	case QCH_TYPE:
		ret = ra_req_enable_qch(clk, params);
		break;
	case DIV_TYPE:
		break;
	case CLKOUT_TYPE:
		ret = ra_enable_clkout(clk, params);
		break;
	default:
		pr_err("Un-support clk type %x\n", id);
		ret = -EVCLKINVAL;
	}

	return ret;
}

int ra_set_value(unsigned int id, unsigned int params)
{
	struct cmucal_clk *clk;
	unsigned type = GET_TYPE(id);
	int ret;

	clk = cmucal_get_node(id);
	if (!clk) {
		pr_err("%s:[%x]type : %x, params : %x\n",
			__func__, id, type, params);
		return -EVCLKINVAL;
	}

	pr_debug("%s:[%s:%x]type : %x, params : %x\n",
		__func__, clk->name, id, type, params);

	switch (type) {
	case DIV_TYPE:
		ret = ra_set_div_mux(clk, params);
		break;
	case MUX_TYPE:
		ret = ra_set_div_mux(clk, params);
		break;
	case PLL_TYPE:
		ret = ra_set_pll(clk, params, 0);
		break;
	case GATE_TYPE:
		ret = ra_set_gate(clk, params);
		break;
	default:
		pr_err("Un-support clk type %x\n", id);
		ret = -EVCLKINVAL;
	}

	return ret;
}

unsigned int ra_get_value(unsigned int id)
{
	struct cmucal_clk *clk;
	unsigned type = GET_TYPE(id);
	unsigned int val;

	clk = cmucal_get_node(id);
	if (!clk) {
		pr_err("%s:[%x]type : %x\n", __func__, id, type);
		return 0;
	}
	switch (type) {
	case DIV_TYPE:
		val = ra_get_div_mux(clk);
		break;
	case MUX_TYPE:
		val = ra_get_div_mux(clk);
		break;
	case PLL_TYPE:
		val = ra_get_pll(clk);
		break;
	case GATE_TYPE:
		val = ra_get_gate(clk);
		break;
	case FIXED_RATE_TYPE:
		val = ra_get_fixed_rate(clk);
		break;
	case FIXED_FACTOR_TYPE:
		val = ra_get_fixed_factor(clk);
		break;
	default:
		pr_err("Un-support clk type %x\n", id);
		val = 0;
	}

	return val;
}

static unsigned int __init ra_get_sfr_address(unsigned short idx,
			       void __iomem **addr,
			       unsigned char *shift,
			       unsigned char *width)
{
	struct sfr_access *field;
	struct sfr *reg;
	struct sfr_block *block;

	field = cmucal_get_sfr_node(idx | SFR_ACCESS_TYPE);
	if (!field) {
		pr_info("%s:failed idx:%x\n", __func__, idx);
		return 0;
	}
	*shift = field->shift;
	*width = field->width;

	reg = cmucal_get_sfr_node(field->sfr | SFR_TYPE);
	if (!reg) {
		pr_info("%s:failed idx:%x sfr:%x\n", __func__, idx, field->sfr);
		return 0;
	}

	block = cmucal_get_sfr_node(reg->block | SFR_BLOCK_TYPE);
	if (!reg || !block) {
		pr_info("%s:failed idx:%x reg:%x\n", __func__, idx, reg->block);
		return 0;
	}
	*addr = block->va + reg->offset;

	return block->pa + reg->offset;
}

static void ra_get_pll_address(struct cmucal_clk *clk)
{
	struct cmucal_pll *pll = to_pll_clk(clk);

	/* lock_div */
	ra_get_sfr_address(clk->offset_idx,
			   &clk->lock,
			   &clk->shift,
			   &clk->width);
	/* enable_div */
	clk->paddr = ra_get_sfr_address(clk->enable_idx,
					&clk->pll_con0,
					&clk->e_shift,
					&clk->e_width);

	/* status_div */
	ra_get_sfr_address(clk->status_idx,
			   &clk->pll_con0,
			   &clk->s_shift,
			   &clk->s_width);

	/* m_div */
	ra_get_sfr_address(pll->m_idx,
			   &clk->pll_con0,
			   &pll->m_shift,
			   &pll->m_width);

	/* p_div */
	ra_get_sfr_address(pll->p_idx,
			   &clk->pll_con0,
			   &pll->p_shift,
			   &pll->p_width);

	/* s_div */
	ra_get_sfr_address(pll->s_idx,
			   &clk->pll_con0,
			   &pll->s_shift,
			   &pll->s_width);

	/* k_div */
	if (pll->k_idx != EMPTY_CAL_ID)
		ra_get_sfr_address(pll->k_idx,
				   &clk->pll_con1,
				   &pll->k_shift,
				   &pll->k_width);
	else
		clk->pll_con1 = NULL;
}

static void ra_get_pll_rate_table(struct cmucal_clk *clk)
{
	struct cmucal_pll *pll = to_pll_clk(clk);
	void *pll_block;
	struct cmucal_pll_table *table;
	struct ect_pll *pll_unit;
	struct ect_pll_frequency *pll_frequency;
	int i;

	pll_block = ect_get_block(BLOCK_PLL);
	if (!pll_block)
		return;

	pll_unit = ect_pll_get_pll(pll_block, clk->name);
	if (!pll_unit)
		return;

	table = kzalloc(sizeof(struct cmucal_pll_table) * pll_unit->num_of_frequency,
			GFP_KERNEL);
	if (!table)
		return;

	for (i = 0; i < pll_unit->num_of_frequency; ++i) {
		pll_frequency = &pll_unit->frequency_list[i];

		table[i].rate = pll_frequency->frequency;
		table[i].pdiv = pll_frequency->p;
		table[i].mdiv = pll_frequency->m;
		table[i].sdiv = pll_frequency->s;
		table[i].kdiv = pll_frequency->k;
	}

	pll->rate_table = table;
	pll->rate_count = pll_unit->num_of_frequency;
}

int ra_set_list_enable(unsigned int *list, unsigned int num_list)
{
	unsigned int id;
	int i;

	for (i = 0; i < num_list; i++) {
		id = list[i];
		if (IS_USER_MUX(id) || IS_GATE(id))
			ra_set_value(id, 1);
		else if (IS_PLL(id))
			ra_set_enable(id, 1);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ra_set_list_enable);

int ra_set_list_disable(unsigned int *list, unsigned int num_list)
{
	unsigned int id;
	int i;

	for (i = num_list ; i > 0; i--) {
		id = list[i-1];
		if (IS_USER_MUX(id) || IS_GATE(id))
			ra_set_value(id, 0);
		else if (IS_PLL(id))
			ra_set_enable(id, 0);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ra_set_list_disable);

void ra_set_pll_ops(unsigned int *list,
		struct vclk_lut *lut,
		unsigned int num_list,
		struct vclk_trans_ops *ops)
{
	unsigned int from, to;
	int i;
	bool trans;

	for (i = 0; i < num_list; i++) {
		if (GET_TYPE(list[i]) != PLL_TYPE)
			continue;

		to = lut->params[i];
		if (ops && ops->get_pll)
			from = ops->get_pll(list[i]);
		else
			from = ra_get_value(list[i]);

		trans = ra_get_trans_opt(to, from);
		if (trans == TRANS_IGNORE)
			continue;

		if (ops && ops->set_pll)
			ops->set_pll(list[i], to);
		else
			ra_set_value(list[i], to);
	}
}
EXPORT_SYMBOL_GPL(ra_set_pll_ops);

void ra_set_clk_by_type(unsigned int *list,
		     struct vclk_lut *lut,
		     unsigned int num_list,
		     unsigned int type,
		     enum trans_opt opt)
{
	unsigned int from, to;
	int i;
	bool trans;

	for (i = 0; i < num_list; i++) {
		if (GET_TYPE(list[i]) != type)
			continue;

		to = lut->params[i];
		from = ra_get_value(list[i]);
		trans = ra_get_trans_opt(to, from);
		if (trans == TRANS_IGNORE)
			continue;
		if (opt != TRANS_FORCE && trans != opt)
			continue;

		ra_set_value(list[i], to);
	}
}
EXPORT_SYMBOL_GPL(ra_set_clk_by_type);

void ra_set_clk_by_seq(unsigned int *list,
		    struct vclk_lut *lut,
		    struct vclk_seq *seq,
		    unsigned int num_list)
{
	unsigned int from, to;
	unsigned int i, idx;
	bool trans;

	for (i = 0; i < num_list; i++) {
		from = lut->params[i];
		to = ra_get_value(list[i]);
		trans = ra_get_trans_opt(to, from);
		if (seq[i].opt & trans) {
			idx = seq[i].idx;
			ra_set_value(list[idx], to);
		}
	}
}
EXPORT_SYMBOL_GPL(ra_set_clk_by_seq);

int ra_compare_clk_list(unsigned int *params,
			unsigned int *list,
			unsigned int num_list)
{
	struct cmucal_clk *clk;
	unsigned int i, type;

	for (i = 0; i < num_list; i++) {
		type = GET_TYPE(list[i]);
		clk = cmucal_get_node(list[i]);
		if (!clk) {
			pr_err("%s:[%x]type : %x\n", __func__, list[i], type);
			return -EVCLKINVAL;
		}
		switch (type) {
		case DIV_TYPE:
			if (params[i] != ra_get_div_mux(clk))
				goto mismatch;
			break;
		case MUX_TYPE:
			if (params[i] != ra_get_div_mux(clk))
				goto mismatch;
			break;
		case PLL_TYPE:
			if (params[i] != ra_get_pll_idx(clk))
				goto mismatch;
			break;
		default:
			pr_err("Un-support clk type %x\n", list[i]);
			return -EVCLKINVAL;
		}
	}

	return 0;
mismatch:
	pr_debug("mis-match %s <%u %u> \n",
		 clk->name, params[i], ra_get_value(list[i]));
	return -EVCLKNOENT;
}
EXPORT_SYMBOL_GPL(ra_compare_clk_list);

unsigned int ra_set_rate_switch(struct vclk_switch *info, unsigned int rate_max)
{
	struct switch_lut *lut;
	unsigned int switch_rate = rate_max;
	int i;

	for (i = 0; i < info->num_switches; i++) {
		lut = &info->lut[i];
		if (rate_max >= lut->rate) {
			if (info->src_div != EMPTY_CLK_ID)
				ra_set_value(info->src_div, lut->div_value);
			if (info->src_mux != EMPTY_CLK_ID)
				ra_set_value(info->src_mux, lut->mux_value);

			switch_rate = lut->rate;
			break;
		}
	}

	if (i == info->num_switches)
		switch_rate = rate_max;

	return switch_rate;
}
EXPORT_SYMBOL_GPL(ra_set_rate_switch);

void ra_select_switch_pll(struct vclk_switch *info, unsigned int value)
{
	if (value) {
		if (info->src_gate != EMPTY_CLK_ID)
			ra_set_value(info->src_gate, value);
		if (info->src_umux != EMPTY_CLK_ID)
			ra_set_value(info->src_umux, value);
	}

	ra_set_value(info->switch_mux, value);

	if (!value) {
		if (info->src_umux != EMPTY_CLK_ID)
			ra_set_value(info->src_umux, value);
		if (info->src_gate != EMPTY_CLK_ID)
			ra_set_value(info->src_gate, value);
	}
}
EXPORT_SYMBOL_GPL(ra_select_switch_pll);

struct cmucal_clk *ra_get_parent(unsigned int id)
{
	struct cmucal_clk *clk, *parent;
	struct cmucal_mux *mux;
	unsigned int val;

	clk = cmucal_get_node(id);
	if (!clk)
		return NULL;

	switch (GET_TYPE(clk->id)) {
	case FIXED_RATE_TYPE:
	case FIXED_FACTOR_TYPE:
	case PLL_TYPE:
	case DIV_TYPE:
	case GATE_TYPE:
		if (clk->pid == EMPTY_CLK_ID)
			parent = NULL;
		else
			parent = cmucal_get_node(clk->pid);
		break;
	case MUX_TYPE:
		mux = to_mux_clk(clk);
		val = ra_get_div_mux(clk);
		parent = cmucal_get_node(mux->pid[val]);
		break;
	default:
		parent = NULL;
		break;
	}

	return parent;
}
EXPORT_SYMBOL_GPL(ra_get_parent);

int ra_set_rate(unsigned int id, unsigned int rate)
{
	struct cmucal_clk *clk;
	int ret = 0;

	clk = cmucal_get_node(id);
	if (!clk)
		return -EVCLKINVAL;

	switch (GET_TYPE(clk->id)) {
	case PLL_TYPE:
		ret = ra_set_pll(clk, rate / 1000, rate);
		break;
	case DIV_TYPE:
		ret = ra_set_div_rate(clk, rate);
		break;
	case MUX_TYPE:
		ret = ra_set_mux_rate(clk, rate);
		break;
	default:
		pr_err("Un-support clk type %x, rate = %u\n", id, rate);
		ret = -EVCLKINVAL;
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(ra_set_rate);

unsigned int ra_recalc_rate(unsigned int id)
{
	struct cmucal_clk *clk;
	unsigned int cur;
	unsigned int clk_path[RECALC_MAX];
	unsigned int depth, ratio;
	unsigned long rate;

	if (GET_TYPE(id) > GATE_TYPE)
		return 0;

	cur = id;

	for (depth = 0; depth < RECALC_MAX; depth++) {
		clk_path[depth] = cur;
		clk = ra_get_parent(cur);
		if (!clk)
			break;
		cur = clk->id;
	}

	if (depth == RECALC_MAX) {
		pr_err("recalc_rate overflow id:%x\n", id);
		return 0;
	}

	/* get root clock rate */
	if (depth > 0 && IS_PLL(clk_path[depth-1]))
		rate = ra_get_value(clk_path[--depth]);
	else
		rate = ra_get_value(clk_path[depth]);

	if (!rate)
		return 0;

	/* calc request clock node rate */
	for (; depth > 0; --depth) {
		cur = clk_path[depth-1];

		if (IS_FIXED_FACTOR(cur) || IS_DIV(cur))
			ratio = ra_get_value(cur) + 1;
		else
			continue;

		do_div(rate, ratio);
	}

	return rate;
}
EXPORT_SYMBOL_GPL(ra_recalc_rate);

int __init ra_init(void)
{
	struct cmucal_clk *clk;
	struct sfr_block *block;
	int i;
	int size;
	struct cmucal_qch *qch;

	/* convert physical address to virtual address */
	size = cmucal_get_list_size(SFR_BLOCK_TYPE);
	for (i = 0; i < size; i++) {
		block = cmucal_get_sfr_node(i | SFR_BLOCK_TYPE);
		if (block && block->pa)
			block->va = ioremap(block->pa, block->size);
	}

	size = cmucal_get_list_size(PLL_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | PLL_TYPE);
		if (!clk)
			continue;

		ra_get_pll_address(clk);

		ra_get_pll_rate_table(clk);

		pll_get_locktime(to_pll_clk(clk));
	}

	size = cmucal_get_list_size(MUX_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | MUX_TYPE);
		if (!clk)
			continue;

		if (GET_IDX(clk->offset_idx) != EMPTY_CAL_ID)
			clk->paddr = ra_get_sfr_address(clk->offset_idx,
							&clk->offset,
							&clk->shift,
							&clk->width);
		else
			clk->offset = NULL;

		if (GET_IDX(clk->status_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->status_idx,
					   &clk->status,
					   &clk->s_shift,
					   &clk->s_width);
		else
			clk->status = NULL;

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;
	}

	size = cmucal_get_list_size(DIV_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | DIV_TYPE);
		if (!clk)
			continue;

		if (GET_IDX(clk->offset_idx) != EMPTY_CAL_ID)
			clk->paddr = ra_get_sfr_address(clk->offset_idx,
							&clk->offset,
							&clk->shift,
							&clk->width);
		else
			clk->offset = NULL;

		if (GET_IDX(clk->status_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->status_idx,
					   &clk->status,
					   &clk->s_shift,
					   &clk->s_width);
		else
			clk->status = NULL;

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;
	}

	size = cmucal_get_list_size(GATE_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | GATE_TYPE);
		if (!clk)
			continue;

		if (GET_IDX(clk->offset_idx) != EMPTY_CAL_ID)
			clk->paddr = ra_get_sfr_address(clk->offset_idx,
							&clk->offset,
							&clk->shift,
							&clk->width);
		else
			clk->offset = NULL;

		if (GET_IDX(clk->status_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->status_idx,
					   &clk->status,
					   &clk->s_shift,
					   &clk->s_width);
		else
			clk->status = NULL;

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;
	}

	size = cmucal_get_list_size(FIXED_RATE_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | FIXED_RATE_TYPE);
		if (!clk)
			continue;

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;
	}

	size = cmucal_get_list_size(FIXED_FACTOR_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | FIXED_FACTOR_TYPE);
		if (!clk)
			continue;

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;
	}

	size = cmucal_get_list_size(QCH_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | QCH_TYPE);
		if (!clk)
			continue;

		clk->paddr = ra_get_sfr_address(clk->offset_idx,
						&clk->offset,
						&clk->shift,
						&clk->width);

		if (GET_IDX(clk->status_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->status_idx,
					   &clk->status,
					   &clk->s_shift,
					   &clk->s_width);
		else
			clk->status = NULL;

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;

		qch = to_qch(clk);
		if (GET_IDX(qch->ignore_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(qch->ignore_idx,
					&qch->ignore,
					&qch->ig_shift,
					&qch->ig_width);
		else
			qch->ignore= NULL;
	}

	size = cmucal_get_list_size(OPTION_TYPE);
	for (i = 0; i < size; i++) {
		clk = cmucal_get_node(i | OPTION_TYPE);
		if (!clk)
			continue;

		ra_get_sfr_address(clk->offset_idx,
				   &clk->offset,
				   &clk->shift,
				   &clk->width);

		if (GET_IDX(clk->enable_idx) != EMPTY_CAL_ID)
			ra_get_sfr_address(clk->enable_idx,
					   &clk->enable,
					   &clk->e_shift,
					   &clk->e_width);
		else
			clk->enable = NULL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(ra_init);
