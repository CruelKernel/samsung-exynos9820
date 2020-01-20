
#ifndef _SH_MEM_H_
#define _SH_MEM_H_

#include "iva_ipc_param_ll.h"
#include "iva_ctrl.h"
#include "regs/iva_base_addr.h"

/*
 * please use standard type in the structure if possible.
 * and if you want to use a pointer, it MUST not point to
 * other memory region outside ".shared" section.
 * Furthermore, MUST synchronize to AP-side data structure
 */
#define SHMEM_MAGIC1		((uint32_t) sizeof(struct sh_mem_info))
#define SHMEM_MAGIC2_INIT	(0x12131415)	/* during initialization */
#define SHMEM_MAGIC2_NORMAL	(0x12345678)	/* can accept request */
#define SHMEM_MAGIC2_NORM_EXIT	(0x87654321)	/* exiting */
#define SHMEM_MAGIC2_FAULT	(0xDEADDEAD)	/* fault occurred */
#define SHMEM_MAGIC2_FATAL	(0xDEADFFFF)	/* fatal occurred */
#define SHMEM_MAGIC3		(0xA1100ED0)
#define BUILD_INFO_SIZE		(32)
#define GEN_PURPOSE_COUNT	(4)
#define BUF_POS_SET_COUNT	(2)

/* actually cm0p registers */
#define CP_REGISTER_CNT		(20)
struct cp_regs {
	volatile unsigned int uregs[CP_REGISTER_CNT];
};


/* mcu log buffer information*/
struct buf_pos {
	volatile int	head;
	volatile int	tail;
};

/* SHARED MEMORY STRUCTURE */
struct sh_mem_info {
	volatile unsigned int	magic1;
	volatile unsigned int	shmem_size;
	char			build_info[BUILD_INFO_SIZE];

	volatile unsigned int	gen_purpose[GEN_PURPOSE_COUNT];

	/* for mcu status(error, etc) info */
	volatile unsigned int	magic2;
	struct cp_regs	cp_reg;

	/* for parameter communication - check alignment for AArch64*/
	volatile unsigned int	clk_rate;
	struct ipc_cmd_queue	ap_cmd_q;
	struct ipc_res_queue	ap_rsp_q;

	/* always the last items */
	struct buf_pos	log_pos;
	int		log_buf_size;
	unsigned int	magic3;
	char		log_buf[0];	/*circular log buf start address*/
};


static inline struct sh_mem_info *sh_mem_get_sm_pointer(struct iva_dev_data *iva)
{
	return (struct sh_mem_info *) iva->shmem_va;
}

static inline void sh_mem_set_iva_clk_rate(struct iva_dev_data *iva, uint32_t clk_rate)
{
	struct sh_mem_info *sh_mem = sh_mem_get_sm_pointer(iva);
	sh_mem->clk_rate = clk_rate;
}

extern int	sh_mem_wait_mcu_ready(struct iva_dev_data *iva, uint32_t to_us);
extern int	sh_mem_wait_stop(struct iva_dev_data *iva, uint32_t to_us);
extern void	sh_mem_dump_info(struct iva_dev_data *iva);
extern int	sh_mem_init(struct iva_dev_data *iva);
extern int	sh_mem_deinit(struct iva_dev_data *iva);
#endif
