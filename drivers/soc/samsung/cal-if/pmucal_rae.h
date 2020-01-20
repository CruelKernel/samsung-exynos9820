#ifndef __PMUCAL_RAE_H__
#define __PMUCAL_RAE_H__

#ifdef PWRCAL_TARGET_LINUX
#include <linux/io.h>
#endif

/* for phy2virt address mapping */
struct p2v_map {
	phys_addr_t pa;
	void __iomem *va;
};

#define DEFINE_PHY(p) { .pa = p }

/* APIs to be supported to PMUCAL common logics */
extern int pmucal_rae_init(void);
extern int pmucal_rae_handle_seq(struct pmucal_seq *seq, unsigned int seq_size);
extern int pmucal_rae_handle_cp_seq(struct pmucal_seq *seq, unsigned int seq_size);
extern void pmucal_rae_save_seq(struct pmucal_seq *seq, unsigned int seq_size);
extern int pmucal_rae_restore_seq(struct pmucal_seq *seq, unsigned int seq_size);
extern int pmucal_rae_phy2virt(struct pmucal_seq *seq, unsigned int seq_size);

extern struct p2v_map pmucal_p2v_list[];
extern unsigned int pmucal_p2v_list_size;

/* for cp */
extern int pmucal_is_cp_smc_regs(struct pmucal_seq *seq);
extern void pmucal_smc_read(struct pmucal_seq *seq, int update);
extern void pmucal_smc_write(struct pmucal_seq *seq);
#endif
