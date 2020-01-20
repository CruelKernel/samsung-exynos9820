#ifndef _IVA_RAMDUMP_H_
#define _IVA_RAMDUMP_H_

#include "iva_ctrl.h"

extern void	iva_ram_dump_copy_vcm_dbg(struct iva_dev_data *iva,
			void *dst_buf, uint32_t dst_buf_sz);
extern int32_t	iva_ram_dump_copy_mcu_sram(struct iva_dev_data *iva,
			void *dst_buf, uint32_t dst_buf_sz);
extern int32_t	iva_ram_dump_copy_iva_sfrs(struct iva_proc *proc, int shared_fd);

extern int32_t	iva_ram_dump_init(struct iva_dev_data *iva);
extern int32_t	iva_ram_dump_deinit(struct iva_dev_data *iva);

#endif /* _IVA_RAMDUMP_H_ */
