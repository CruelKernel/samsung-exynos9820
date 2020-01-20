#ifndef __PWRCAL_RAE_H__
#define __PWRCAL_RAE_H__

/* enumeration for register access operations */
enum pwrcal_rae_opid {
	RA_GETB = 0,
	RA_SETB,
	RA_GETF,
	RA_SETF,
	RA_RD,
	RA_WR,
};

/* structure for register access operations description (recommended) */
/*
	unsigned int p[4];
	0: register address
	1: access operation id
	2: position (bit/field)
	3: mask (field);
*/
struct pwrcal_rae_op {
	int	id;
	unsigned int	p[4];
};

/* structrue for v2p map */
struct v2p_sfr {
	unsigned int pa;
	void *ma;	/* mapped address */
};

extern struct v2p_sfr v2psfrmap[];
extern int num_of_v2psfrmap;
#define DEFINE_V2P(v, p)	\
	[v >> 16] = {	.pa = p,	.ma = (void *)p,	}
#define MAP_IDX(va)	(((unsigned long)va & 0x0FFF0000) >> 16)

extern void *spinlock_enable_offset;

/* signatures for the interface to the register access engine  */
extern unsigned int pwrcal_readl(void *rega);
extern void pwrcal_writel(void *rega, unsigned int regv);
extern void pwrcal_setbit(void *rega, unsigned int bitp, unsigned int bitv);
extern unsigned int pwrcal_getf(void *rega,
				unsigned int fieldp,
				unsigned int fieldm);
extern void pwrcal_setf(void *rega,
			unsigned int fieldp,
			unsigned int fieldm,
			unsigned int fieldv);
extern unsigned int pwrcal_getbit(void *rega, unsigned int bitp);
extern unsigned int pwrcal_get_pa(void *pa);
extern void cal_rae_init(void);

#endif
