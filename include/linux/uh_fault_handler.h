#ifndef __UH_FAULT_HANDLER_H__
#define __UH_FAULT_HANDLER_H__

#include <linux/threads.h>

typedef enum esr_il {
	esr_il_16_bit_instruction_trapped
		= 0x0,
	esr_il_32_bit_instruction_trapped
		= 0x1
} esr_il;

typedef enum esr_ec {
	esr_ec_unknown_reason
		= 0x0,
	esr_ec_wfi_or_wfe_instruction_execution
		= 0x1,
	esr_ec_mcr_or_mrc_access_to_cp15
		= 0x3,
	esr_ec_mcrr_or_mrrc_access_to_cp15
		= 0x4,
	esr_ec_mcr_or_mrc_access_to_cp14
		= 0x5,
	esr_ec_ldc_or_stc_access_to_cp14
		= 0x6,
	esr_ec_access_to_simd_or_floating_point_registers
		= 0x7,
	esr_ec_trapped_mrc_or_mrs_access_to_cp10_for_id_group_traps
		= 0x8,
	esr_ec_trapped_mrrc_or_mcrr_access_to_cp14
		= 0xC,
	esr_ec_illegal_execution_state
		= 0xE,
	esr_ec_aarch32_svc_instruction_execution
		= 0x11,
	esr_ec_aarch32_hvc_instruction_execution
		= 0x12,
	esr_ec_aarch32_smc_instruction_execution
		= 0x13,
	esr_ec_aarch64_svc_instruction_execution
		= 0x15,
	esr_ec_aarch64_hvc_instruction_execution
		= 0x16,
	esr_ec_aarch64_smc_instruction_execution
		= 0x17,
	esr_ec_other_msr_mrs_or_system_instruction_execution
		= 0x18,
	esr_ec_prefetch_abort_from_a_lower_exception_level
		= 0x20,
	esr_ec_prefetch_abort_taken_without_a_change_in_exception_level
		= 0x21,
	esr_ec_misaligned_pc_exception
		= 0x22,
	esr_ec_data_abort_from_a_lower_exception_level
		= 0x24,
	esr_ec_data_abort_taken_without_a_change_in_exception_level
		= 0x25,
	esr_ec_stack_pointer_alignment_exception
		= 0x26,
	esr_ec_aarch32_floating_point_exception
		= 0x28,
	esr_ec_aarch64_floating_point_exception
		= 0x2C,
	esr_ec_serror_interrupt
		= 0x2F,
	esr_ec_breakpoint_exception_from_a_lower_exception_level
		= 0x30,
	esr_ec_breakpoint_exception_taken_without_a_change_in_exception_level
		= 0x31,
	esr_ec_software_step_exception_taken_from_a_lower_exception_level
		= 0x32,
	esr_ec_software_step_exception_taken_without_a_change_in_exception_level
		= 0x33,
	esr_ec_watchpoint_exception_from_a_lower_exception_level
		= 0x34,
	esr_ec_watchpoint_exception_taken_without_a_change_in_exception_level
		= 0x35,
	esr_ec_bkpt_instruction_execution
		= 0x38,
	esr_ec_vector_catch_exception_from_aach32_state
		= 0x3A,
	esr_ec_brk_instruction_execution
		= 0x3C
} esr_ec;

typedef union esr {
	struct {
		u32 iss   :25;
		esr_il il :1;
		esr_ec ec :6;
	}__attribute__((packed));
	u32 bits;
} esr_t;

typedef struct uh_registers {
	u64 regs[31];
	u64 sp;
	u64 pc;
	u64 pstate;
} uh_registers_t;

typedef struct uh_handler_data{
	esr_t esr_el2;
	u64 elr_el2;
	u64 hcr_el2;
	u64 far_el2;
	u64 hpfar_el2;
	uh_registers_t regs;
} uh_handler_data_t;

typedef struct uh_handler_list{
	u64 uh_handler;
	uh_handler_data_t uh_handler_data[NR_CPUS];
} uh_handler_list_t;

u64 uh_get_fault_handler(void);

void do_mem_abort(unsigned long addr, unsigned int esr, struct pt_regs *regs);
extern int do_not_show_extra;

#endif //__UH_FAULT_HANDLER_H__
