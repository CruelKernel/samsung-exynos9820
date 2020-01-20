#include <linux/string.h>
#include "cmucal.h"

#define get_node(list, idx)		(GET_IDX(list[idx].id) == idx ? \
					&list[idx] : NULL)
#define get_clk_node(list, idx)		(GET_IDX(list[idx].clk.id) == idx ? \
					&list[idx] : NULL)

extern struct cmucal_clk_fixed_rate cmucal_fixed_rate_list[];
extern struct cmucal_clk_fixed_factor cmucal_fixed_factor_list[];
extern struct cmucal_pll cmucal_pll_list[];
extern struct cmucal_mux cmucal_mux_list[];
extern struct cmucal_div cmucal_div_list[];
extern struct cmucal_gate cmucal_gate_list[];
extern struct cmucal_qch cmucal_qch_list[];
extern struct cmucal_option cmucal_option_list[];
extern struct sfr_block cmucal_sfr_block_list[];
extern struct sfr cmucal_sfr_list[];
extern struct sfr_access cmucal_sfr_access_list[];
extern struct vclk cmucal_vclk_list[];
extern struct vclk acpm_vclk_list[];
extern struct cmucal_clkout cmucal_clkout_list[];

extern unsigned int cmucal_fixed_rate_size;
extern unsigned int cmucal_fixed_factor_size;
extern unsigned int cmucal_pll_size;
extern unsigned int cmucal_mux_size;
extern unsigned int cmucal_div_size;
extern unsigned int cmucal_gate_size;
extern unsigned int cmucal_qch_size;
extern unsigned int cmucal_option_size;
extern unsigned int cmucal_sfr_block_size;
extern unsigned int cmucal_sfr_size;
extern unsigned int cmucal_sfr_access_size;
extern unsigned int cmucal_vclk_size;
extern unsigned int acpm_vclk_size;
extern unsigned int cmucal_clkout_size;

unsigned int cmucal_get_list_size(unsigned int type)
{
	switch (type) {
	case FIXED_RATE_TYPE:
		return cmucal_fixed_rate_size;
	case FIXED_FACTOR_TYPE:
		return cmucal_fixed_factor_size;
	case PLL_TYPE:
		return cmucal_pll_size;
	case MUX_TYPE:
		return cmucal_mux_size;
	case DIV_TYPE:
		return cmucal_div_size;
	case GATE_TYPE:
		return cmucal_gate_size;
	case QCH_TYPE:
		return cmucal_qch_size;
	case OPTION_TYPE:
		return cmucal_option_size;
	case SFR_BLOCK_TYPE:
		return cmucal_sfr_block_size;
	case SFR_TYPE:
		return cmucal_sfr_size;
	case SFR_ACCESS_TYPE:
		return cmucal_sfr_access_size;
	case VCLK_TYPE:
		return cmucal_vclk_size;
	case CLKOUT_TYPE:
		return cmucal_clkout_size;
	case ACPM_VCLK_TYPE:
		return acpm_vclk_size;
	default:
		pr_info("unsupport cmucal type %x\n", type);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(cmucal_get_list_size);

void *cmucal_get_node(unsigned int id)
{
	unsigned int type = GET_TYPE(id);
	unsigned short idx = GET_IDX(id);
	void *node = NULL;

	switch (type) {
	case FIXED_RATE_TYPE:
		node = get_clk_node(cmucal_fixed_rate_list, idx);
		break;
	case FIXED_FACTOR_TYPE:
		node = get_clk_node(cmucal_fixed_factor_list, idx);
		break;
	case PLL_TYPE:
		node = get_clk_node(cmucal_pll_list, idx);
		break;
	case MUX_TYPE:
		node = get_clk_node(cmucal_mux_list, idx);
		break;
	case DIV_TYPE:
		node = get_clk_node(cmucal_div_list, idx);
		break;
	case GATE_TYPE:
		node = get_clk_node(cmucal_gate_list, idx);
		break;
	case QCH_TYPE:
		node = get_clk_node(cmucal_qch_list, idx);
		break;
	case OPTION_TYPE:
		node = get_clk_node(cmucal_option_list, idx);
		break;
	case CLKOUT_TYPE:
		node = get_clk_node(cmucal_clkout_list, idx);
		break;
	case VCLK_TYPE:
		if (IS_ACPM_VCLK(id))
			node = get_node(acpm_vclk_list, idx);
		else
			node = get_node(cmucal_vclk_list, idx);
		break;
	default:
		pr_info("unsupport cmucal node %x\n", id);
	}

	return node;
}
EXPORT_SYMBOL_GPL(cmucal_get_node);

void * __init cmucal_get_sfr_node(unsigned int id)
{
	unsigned int type = GET_TYPE(id);
	unsigned short idx = GET_IDX(id);
	void *node = NULL;

	switch (type) {
	case SFR_BLOCK_TYPE:
		node = get_node(cmucal_sfr_block_list, idx);
		break;
	case SFR_TYPE:
		node = get_node(cmucal_sfr_list, idx);
		break;
	case SFR_ACCESS_TYPE:
		node = get_node(cmucal_sfr_access_list, idx);
		break;
	default:
		pr_info("unsupport cmucal sfr node %x\n", id);
	}

	return node;
}
EXPORT_SYMBOL_GPL(cmucal_get_sfr_node);

unsigned int cmucal_get_id(char *name)
{
	unsigned int id = INVALID_CLK_ID;
	int i;

	if (strstr(name, "MUX") || strstr(name, "MOUT")) {
		for (i = 0; i < cmucal_mux_size; i++)
			if (!strcmp(name, cmucal_mux_list[i].clk.name)) {
				id = cmucal_mux_list[i].clk.id;
				return id;
			}
	}

	if (strstr(name, "GATE") || strstr(name, "GOUT") || strstr(name, "CLK_BLK")) {
		for (i = 0; i < cmucal_gate_size; i++)
			if (!strcmp(name, cmucal_gate_list[i].clk.name)) {
				id = cmucal_gate_list[i].clk.id;
				return id;
			}
	}

	if (strstr(name, "DIV") || strstr(name, "DOUT") || strstr(name, "CLKCMU")) {
		for (i = 0; i < cmucal_div_size; i++)
			if (!strcmp(name, cmucal_div_list[i].clk.name)) {
				id = cmucal_div_list[i].clk.id;
				return id;
			}
		for (i = 0; i < cmucal_fixed_factor_size; i++)
			if (!strcmp(name, cmucal_fixed_factor_list[i].clk.name)) {
				id = cmucal_fixed_factor_list[i].clk.id;
				return id;
			}
	}

	if (strstr(name, "VCLK") || strstr(name, "dvfs")) {
		for (i = 0; i < cmucal_vclk_size; i++)
			if (!strcmp(name, cmucal_vclk_list[i].name)) {
				id = cmucal_vclk_list[i].id;
				return id;
			}
	}

	if (strstr(name, "PLL")) {
		for (i = 0; i < cmucal_pll_size; i++)
			if (!strcmp(name, cmucal_pll_list[i].clk.name)) {
				id = cmucal_pll_list[i].clk.id;
				return id;
			}
	}

	if (strstr(name, "IO") || strstr(name, "OSC")) {
		for (i = 0; i < cmucal_pll_size; i++)
			if (!strcmp(name, cmucal_fixed_rate_list[i].clk.name)) {
				id = cmucal_fixed_rate_list[i].clk.id;
				return id;
			}
	}

	return id;
}
EXPORT_SYMBOL_GPL(cmucal_get_id);

unsigned int cmucal_get_id_by_addr(unsigned int addr)
{
	unsigned int id = INVALID_CLK_ID;
	unsigned int type;
	int i;

	type = addr & 0x3F00;

	if (type == 0x1800 || type == 0x1900) {
		/* DIV */
		for (i = 0; i < cmucal_div_size; i++)
			if (addr == cmucal_div_list[i].clk.paddr) {
				id = cmucal_div_list[i].clk.id;
				return id;
			}
	} else if (type == 0x1000 || type == 0x1100) {
		/* MUX */
		for (i = 0; i < cmucal_mux_size; i++)
			if (addr == cmucal_mux_list[i].clk.paddr) {
				id = cmucal_mux_list[i].clk.id;
				return id;
			}
	} else if (type == 0x2000 || type == 0x2100) {
		/* GATE */
		for (i = 0; i < cmucal_gate_size; i++)
			if (addr == cmucal_gate_list[i].clk.paddr) {
				id = cmucal_gate_list[i].clk.id;
				return id;
			}
	} else {
		/* PLL */
		for (i = 0; i < cmucal_pll_size; i++)
			if (addr == cmucal_pll_list[i].clk.paddr) {
				id = cmucal_pll_list[i].clk.id;
				return id;
			}

		/* USER_MUX */
		for (i = 0; i < cmucal_mux_size; i++)
			if (addr == cmucal_mux_list[i].clk.paddr) {
				id = cmucal_mux_list[i].clk.id;
				return id;
			}
	}

	return id;
}
EXPORT_SYMBOL_GPL(cmucal_get_id_by_addr);
