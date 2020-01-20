#ifndef __CMUCAL_H__
#define __CMUCAL_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include "vclk.h"

struct dentry;

#ifndef abs
#define abs(x) ({						\
		long ret;					\
		if (sizeof(x) == sizeof(long)) {		\
			long __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		} else {					\
			int __x = (x);				\
			ret = (__x < 0) ? -__x : __x;		\
		}						\
		ret;						\
	})
#endif

#ifndef do_div
#define do_div(a, b)	(a /= b)
#endif

#define EVCLKPERM	 1
#define EVCLKNOENT	 2
#define EVCLKAGAIN	11
#define EVCLKNOMEM	12
#define EVCLKFAULT	14	/* Bad address */
#define EVCLKBUSY	16
#define EVCLKINVAL	22
#define EVCLKTIMEOUT	110

#define	MASK_OF_TYPE		0x0F000000
#define	MASK_OF_SUBTYPE		0x00FF0000
#define	MASK_OF_ID		0x0000FFFF
#define	FIXED_RATE_TYPE		0x01000000
#define	FIXED_FACTOR_TYPE	0x02000000
#define	PLL_TYPE		0x03000000
#define	MUX_TYPE		0x04000000
#define	USER_MUX_TYPE		0x04010000
#define	CONST_MUX_TYPE		0x04020000
#define	DIV_TYPE		0x05000000
#define	CONST_DIV_TYPE		0x05020000
#define	GATE_TYPE		0x06000000
#define	GATE_ROOT_TYPE		0x06010000
#define	QCH_TYPE		0x07000000
#define	SFR_BLOCK_TYPE		0x08000000
#define	SFR_TYPE		0x09000000
#define	SFR_ACCESS_TYPE		0x0A000000
#define	VCLK_TYPE		0x0B000000
#define	COMMON_VCLK_TYPE	0x0B010000
#define	DFS_VCLK_TYPE		0x0B020000
#define	GATE_VCLK_TYPE		0x0B030000
#define	ACPM_VCLK_TYPE		0x0B040000
#define	OPTION_TYPE		0x0C000000
#define	CLKOUT_TYPE		0x0D000000
#define	INVALID_CLK_ID		MASK_OF_ID
#define	EMPTY_CLK_ID		MASK_OF_ID
#define	EMPTY_CAL_ID		MASK_OF_ID

/**
 * struct vclk_lut - Virtual clock Look-Up-Table
 * @rate: virtual clock rate
 * @params: clks setting parameters, number of params is num_clks
 */
struct vclk_lut {
	unsigned int	rate;
	unsigned int	*params;
};

/**
 * struct vclk_seq - Virtual clock sequence
 * @idx: index of struct vclk_clks
 * @opt: option : TRANS_HIGH, TRANS_LOW, TRANS_FORCE
 */
struct vclk_seq {
	unsigned int		idx;
	unsigned int		opt;
};

/**
 * struct vclk_switch - switching lut info
 * @switch_rate: switch_rate
 * @mux_value: mux value about source
 * @div_value: div value about mout
 */
struct switch_lut {
	unsigned int		rate;
	unsigned int		mux_value;
	unsigned int		div_value;
};

/**
 * struct vclk_switch - Virtual clock switching PLL info
 * @switch_mux: switching MUX id
 * @src_mux: switching PLL source MUX id
 * @src_div: switching PLL source Divider id
 * @lut: switch PLL Look-Up-Table pointer
 */
struct vclk_switch {
	unsigned int		switch_mux;
	unsigned int		src_mux;
	unsigned int		src_div;
	unsigned int		src_gate;
	unsigned int		src_umux;
	struct switch_lut	*lut;
	unsigned int		num_switches;
};

/**
 * struct vclk - Virtual clock
 * @id: vclk id
 * @name: vclk name
 * @vrate: vclk virtual clock frequency
 * @lut: Look-Up-Table pointer
 * @seq: clock setting sequnce pointer
 * @clk_list: clock list pointer
 * @num_rates: number of lut rates
 * @num_clks: number of clks and seq
 */
struct vclk {
	unsigned int		id;
	char			*name;
	unsigned int		vrate;
	struct vclk_lut		*lut;
	unsigned int		*list;
	struct vclk_seq		*seq;
	unsigned int		num_rates;
	unsigned int		num_list;
	unsigned int		max_freq;
	unsigned int		min_freq;
	unsigned int		boot_freq;
	unsigned int		resume_freq;
	int			margin_id;
	struct vclk_switch	*switch_info;
	struct vclk_trans_ops	*ops;
#ifdef CONFIG_DEBUG_FS
	struct dentry		*dentry;
#endif
};

enum clk_pll_type {
	PLL_0831X = 8310,
	PLL_0817X = 8170,
	PLL_0818X = 8180,
	PLL_0820X = 8200,
	PLL_0821X = 8210,
	PLL_0822X = 8220,
	PLL_1416X = 14160,
	PLL_1417X = 14170,
	PLL_1418X = 14180,
	PLL_1419X = 14190,
	PLL_1431X = 14310,
	PLL_1450X = 14500,
	PLL_1451X = 14510,
	PLL_1452X = 14520,
	PLL_1460X = 14600,

	PLL_1050X = 10500,
	PLL_1051X = 10510,
	PLL_1052X = 10520,
	PLL_1061X = 10610,

	PLL_1016X = 10160,
	PLL_1017X = 10170,
	PLL_1018X = 10180,
	PLL_1019X = 10190,
	PLL_1031X = 10310,

	DPL_L0817X= 138170,
};

enum margin_id {
	MARGIN_MIF,
	MARGIN_INT,
	MARGIN_BIG,
	MARGIN_LIT,
	MARGIN_G3D,
	MARGIN_INTCAM,
	MARGIN_CAM,
	MARGIN_DISP,
	MARGIN_G3DM,
	MARGIN_CP,
	MARGIN_FSYS0,
	MARGIN_AUD,
	MARGIN_IVA,
	MARGIN_SCORE,
	MARGIN_MFC,
	MARGIN_NPU,
	MARGIN_MID,
	MAX_MARGIN_ID,
};

#define IS_FIXED_RATE(_id)	((_id & MASK_OF_TYPE) == FIXED_RATE_TYPE)
#define IS_FIXED_FACTOR(_id)	((_id & MASK_OF_TYPE) == FIXED_FACTOR_TYPE)
#define IS_PLL(_id)		((_id & MASK_OF_TYPE) == PLL_TYPE)
#define IS_MUX(_id)		((_id & MASK_OF_TYPE) == MUX_TYPE)
#define IS_USER_MUX(_id)	((_id & (MASK_OF_TYPE | MASK_OF_SUBTYPE)) == USER_MUX_TYPE)
#define IS_CONST_MUX(_id)	((_id & (MASK_OF_TYPE | MASK_OF_SUBTYPE)) == CONST_MUX_TYPE)
#define IS_DIV(_id)		((_id & MASK_OF_TYPE) == DIV_TYPE)
#define IS_CONST_DIV(_id)	((_id & MASK_OF_TYPE | MASK_OF_SUBTYPE) == CONST_DIV_TYPE)
#define IS_GATE(_id)		((_id & MASK_OF_TYPE) == GATE_TYPE)
#define IS_QCH(_id)		((_id & MASK_OF_TYPE) == QCH_TYPE)
#define IS_OPTION(_id)		((_id & MASK_OF_TYPE) == OPTION_TYPE)

#define IS_VCLK(_id)		((_id & MASK_OF_TYPE) == VCLK_TYPE)
#define IS_DFS_VCLK(_id)	((_id & (MASK_OF_TYPE | MASK_OF_SUBTYPE)) == DFS_VCLK_TYPE)
#define IS_COMMON_VCLK(_id)	((_id & (MASK_OF_TYPE | MASK_OF_SUBTYPE)) == COMMON_VCLK_TYPE)
#define IS_GATE_VCLK(_id)	((_id & (MASK_OF_TYPE | MASK_OF_SUBTYPE)) == GATE_VCLK_TYPE)
#define IS_ACPM_VCLK(_id)	((_id & (MASK_OF_TYPE | MASK_OF_SUBTYPE)) == ACPM_VCLK_TYPE)

#define GET_TYPE(_id)		(_id & MASK_OF_TYPE)
#define GET_IDX(_id)		(_id & MASK_OF_ID)
/*
 * struct sfr_block - SFR block
 * @id: id of sfr_block
 * @name: name of sfr_block
 * @pa: physical address
 * @va: virtual address
 * @size: sfr block size, 2 ^ n
 */
struct sfr_block {
	unsigned int		id;
	char			*name;
	phys_addr_t		pa;
	void __iomem		*va;
	unsigned int		size;
};

/*
 * struct sfr - SFR
 * @id: id of sfr
 * @name: sfr name
 * @offset: offset from block
 * @block: index of block
 */
struct sfr {
	unsigned int		id;
	char			*name;
	unsigned int		offset;
	unsigned int		block;
};

/*
 * struct sfr_access - SFR field
 * @id: id of sfr
 * @name: sfr field name
 * @shift: shift value of field
 * @width: width value of field
 * @sfr: index of sfr
 */
struct sfr_access {
	unsigned int		id;
	char			*name;
	unsigned char		shift;
	unsigned char		width;
	unsigned int		sfr;
};

/*
 * struct cmucal_clk - CMUCAL Clock node
 * @id: id of clock node
 * @paddr: physical addr of clock node
 * @pid: parent id of clock node
 * @name: name of clock node
 * @offset_idx: index of offset
 * @status_idx: index of status
 * @enable_idx: index of enable
 * @offset: offset address
 * @status: status address, or lock_offset of pll
 * @enable: enable address
 * @shift: shift of offset
 * @width: width of offset
 * @s_shift: shift of status
 * @s_width: width of status
 * @e_shift: shift of enable
 * @e_width: width of enable
 */
struct cmucal_clk {
	unsigned int		id;
	unsigned int		paddr;
	unsigned int		pid;
	char			*name;
	union {
		void __iomem	*offset;
		void __iomem	*lock;
		unsigned long	offset_idx;
	};
	union {
		void __iomem	*enable;
		void __iomem	*pll_con0;
		unsigned long	enable_idx;
	};
	union {
		void __iomem	*status;
		void __iomem	*pll_con1;
		unsigned long	status_idx;
	};
	unsigned char		shift;
	unsigned char		width;
	unsigned char		s_shift;
	unsigned char		s_width;
	unsigned char		e_shift;
	unsigned char		e_width;
};

/*
 * struct pll_spec
 * @p,m,s,k min/max value
 * @fref, fvco, fout min/max value
 */
struct pll_spec {
	unsigned int pdiv_min;
	unsigned int pdiv_max;
	unsigned int mdiv_min;
	unsigned int mdiv_max;
	unsigned int sdiv_min;
	unsigned int sdiv_max;
	signed short kdiv_min;
	signed short kdiv_max;
	unsigned long long fref_min;
	unsigned long long fref_max;
	unsigned long long fvco_min;
	unsigned long long fvco_max;
	unsigned long long fout_min;
	unsigned long long fout_max;
	unsigned int lock_time;
	unsigned int flock_time;
};

/*
 * struct cmucal_pll_table
 * @rate: pll rate
 * @pdiv, @mdiv, @sdiv, @kdiv: for rate
 */
struct cmucal_pll_table {
	unsigned int		rate;
	unsigned short		pdiv;
	unsigned short		mdiv;
	unsigned short		sdiv;
	signed short		kdiv;
};

/*
 * struct cmucal_pll
 * @clk: cmucal_rate structure
 * @type: pll type
 * @rate_table: rate table
 * @rate_count: number of rate_table
 * @lock_time: integer PLL locktime
 * @flock_time: fractional PLL locktime
 */
struct cmucal_pll {
	struct cmucal_clk	clk;
	unsigned int		umux;
	unsigned int		type;
	struct cmucal_pll_table	*rate_table;
	unsigned int		rate_count;
	unsigned int		lock_time;
	unsigned int		flock_time;
	unsigned int		p_idx, m_idx, s_idx, k_idx;
	unsigned char		p_shift, m_shift, s_shift, k_shift;
	unsigned char		p_width, m_width, s_width, k_width;
};

struct cmucal_clk_fixed_rate {
	struct cmucal_clk	clk;
	unsigned int		fixed_rate;
};

struct cmucal_clk_fixed_factor {
	struct cmucal_clk	clk;
	unsigned short	ratio;
};

struct cmucal_mux {
	struct cmucal_clk	clk;
	unsigned int		*pid;
	unsigned char		num_parents;
};

struct cmucal_div {
	struct cmucal_clk	clk;
};

struct cmucal_gate {
	struct cmucal_clk	clk;
};

struct cmucal_qch {
	struct cmucal_clk	clk;
	union {
		void __iomem    *ignore;
		unsigned long   ignore_idx;
	};
	unsigned char ig_shift;
	unsigned char ig_width;
};

struct cmucal_option {
	struct cmucal_clk	clk;
};

struct cmucal_clkout {
	struct cmucal_clk	clk;
	unsigned int		sel;
};

#define CMUCAL_VCLK(_id, _lut, _list, _seq, _switch) \
[_id & MASK_OF_ID] = {	\
	.id		= _id,						\
	.name		= #_id,						\
	.lut		= _lut,						\
	.list		= _list,					\
	.seq		= _seq,						\
	.num_rates	= (sizeof(_lut) / sizeof((_lut)[0])),		\
	.num_list	= (sizeof(_list) / sizeof((_list)[0])),		\
	.switch_info	= _switch,					\
	.ops		= NULL,						\
}

#define CMUCAL_ACPM_VCLK(_id, _lut, _list, _seq, _switch, _margin_id)	\
[_id & MASK_OF_ID] = {	\
	.id		= _id,						\
	.name		= #_id,						\
	.lut		= _lut,						\
	.list		= _list,					\
	.seq		= _seq,						\
	.num_rates	= (sizeof(_lut) / sizeof((_lut)[0])),		\
	.num_list	= (sizeof(_list) / sizeof((_list)[0])),		\
	.switch_info	= _switch,					\
	.ops		= NULL,						\
	.margin_id	= _margin_id,					\
}

#define SFR_BLOCK(_id, _pa, _size) \
[_id & MASK_OF_ID] = {	\
	.id		= _id,						\
	.pa		= _pa,						\
	.size		= _size,					\
}

#define SFR(_id, _offset, _block) \
[_id & MASK_OF_ID] = {	\
	.id		= _id,						\
	.offset		= _offset,					\
	.block		= _block,					\
}

#define SFR_ACCESS(_id, _shift, _width, _sfr) \
[_id & MASK_OF_ID] = {	\
	.id		= _id,						\
	.shift		= _shift,					\
	.width		= _width,					\
	.sfr		= _sfr,						\
}

#define CLK_PLL(_typ, _id, _pid, _lock, _enable, _stable,		\
		_p, _m, _s, _k,						\
		_rtable, _time, _ftime)					\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,						\
	.clk.pid	= EMPTY_CLK_ID,					\
	.clk.name	= #_id,						\
	.clk.offset_idx	= _lock,					\
	.clk.enable_idx	= _enable,					\
	.clk.status_idx	= _stable,					\
	.p_idx		= _p,						\
	.m_idx		= _m,						\
	.s_idx		= _s,						\
	.k_idx		= _k,						\
	.type		= _typ,						\
	.umux		= _pid,						\
	.rate_table	= _rtable,					\
	.rate_count	= (sizeof(_rtable) / sizeof((_rtable)[0])),	\
	.lock_time	= _time,					\
	.flock_time	= _ftime,					\
}

#define CLK_MUX(_id, _pids, _o, _so, _eo)		\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.offset_idx	= _o,				\
	.clk.status_idx	= _so,				\
	.clk.enable_idx	= _eo,				\
	.pid		= _pids,			\
	.num_parents	= (sizeof(_pids) / sizeof((_pids)[0])), \
}

#define CLK_DIV(_id, _pid, _o, _so, _eo)		\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.pid	= _pid,				\
	.clk.offset_idx	= _o,				\
	.clk.status_idx	= _so,				\
	.clk.enable_idx = _eo,				\
}

#define CLK_GATE(_id, _pid, _o, _so, _eo)		\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.pid	= _pid,				\
	.clk.offset_idx	= _o,				\
	.clk.status_idx	= _so,				\
	.clk.enable_idx	= _eo,				\
}
#ifdef CONFIG_CMUCAL_QCH_IGNORE_SUPPORT
#define CLK_QCH(_id, _o, _so, _eo, _ig)			\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.pid	= EMPTY_CLK_ID,			\
	.clk.offset_idx	= _o,				\
	.clk.status_idx	= _so,				\
	.clk.enable_idx	= _eo,				\
	.ignore_idx     = _ig,                          \
}
#else
#define CLK_QCH(_id, _o, _so, _eo)			\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.pid	= EMPTY_CLK_ID,			\
	.clk.offset_idx	= _o,				\
	.clk.status_idx	= _so,				\
	.clk.enable_idx	= _eo,				\
}
#endif
#define CLK_OPTION(_id, _o, _eo)			\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.pid	= EMPTY_CLK_ID,			\
	.clk.offset_idx	= _o,				\
	.clk.enable_idx	= _eo,				\
}

#define FIXEDRATE(_id, _frate, _eo)			\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.pid	= EMPTY_CLK_ID,			\
	.clk.name	= #_id,				\
	.clk.enable_idx = _eo,				\
	.fixed_rate	= _frate,			\
}

#define FIXEDFACTOR(_id, _pid, _ratio, _eo)		\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.pid	= _pid,				\
	.clk.name	= #_id,				\
	.clk.enable_idx = _eo,				\
	.ratio		= _ratio,			\
}

#define CLKOUT(_id, _o, _s, _w, _sel, _es, _ew)		\
[_id & MASK_OF_ID] = {	\
	.clk.id		= _id,				\
	.clk.name	= #_id,				\
	.clk.offset_idx	= _o,				\
	.clk.shift	= _s,				\
	.clk.width	= _w,				\
	.clk.e_shift	= _es,				\
	.clk.e_width	= _ew,				\
	.sel		= _sel,				\
}

#define PLL_RATE_MPS(_rate, _m, _p, _s)			\
	{						\
		.rate	=	(_rate),		\
		.mdiv	=	(_m),			\
		.pdiv	=	(_p),			\
		.sdiv	=	(_s),			\
		.kdiv	=	(0),			\
	}

#define PLL_RATE_MPSK(_rate, _m, _p, _s, _k)		\
	{						\
		.rate	=	(_rate),		\
		.mdiv	=	(_m),			\
		.pdiv	=	(_p),			\
		.sdiv	=	(_s),			\
		.kdiv	=	(_k),			\
	}

#define to_fixed_rate_clk(_clk)		container_of(_clk, struct cmucal_clk_fixed_rate, clk)
#define to_fixed_factor_clk(_clk)	container_of(_clk, struct cmucal_clk_fixed_factor, clk)
#define to_pll_clk(_clk)		container_of(_clk, struct cmucal_pll, clk)
#define to_mux_clk(_clk)		container_of(_clk, struct cmucal_mux, clk)
#define to_div_clk(_clk)		container_of(_clk, struct cmucal_div, clk)
#define to_gate_clk(_clk)		container_of(_clk, struct cmucal_gate, clk)
#define to_clkout(_clk)			container_of(_clk, struct cmucal_clkout, clk)
#define to_qch(_clk)			container_of(_clk, struct cmucal_qch, clk)

extern unsigned int cmucal_get_list_size(unsigned int type);
extern void *cmucal_get_node(unsigned int id);
extern void *cmucal_get_sfr_node(unsigned int id);
extern unsigned int cmucal_get_id(char *name);
extern unsigned int cmucal_get_id_by_addr(unsigned int addr);
extern void (*cal_data_init)(void);
extern int (*cal_check_hiu_dvfs_id)(u32 id);
extern void (*cal_set_cmu_smpl_warn)(void);
#endif
