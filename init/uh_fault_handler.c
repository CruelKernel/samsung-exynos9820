#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/cacheflush.h>
#include <asm/irqflags.h>
#include <linux/fs.h>
#include <asm/tlbflush.h>
#include <linux/init.h>
#include <linux/io.h>
#include <asm/topology.h>
#include <asm/memory.h>
#include <asm/system_misc.h>

#include <linux/uh.h>
#include <linux/uh_fault_handler.h>

DEFINE_SPINLOCK(uh_fault_lock);

static const char *exception_class_string[] = {
	[esr_ec_unknown_reason]
	    = "[EL2 EXCEPTION] unknown reason",
	[esr_ec_wfi_or_wfe_instruction_execution]
	    = "[EL2 EXCEPTION] wfi or wfe instruction execution",
	[esr_ec_mcr_or_mrc_access_to_cp15]
	    = "[EL2 EXCEPTION] mcr or mrc access to cp15",
	[esr_ec_mcrr_or_mrrc_access_to_cp15]
	    = "[EL2 EXCEPTION] mcrr or mrrc access to cp15",
	[esr_ec_mcr_or_mrc_access_to_cp14]
	    = "[EL2 EXCEPTION] mcr or mrc access to cp14",
	[esr_ec_ldc_or_stc_access_to_cp14]
	    = "[EL2 EXCEPTION] ldc or stc access to cp14",
	[esr_ec_access_to_simd_or_floating_point_registers]
	    = "[EL2 EXCEPTION] access to simd or floating point registers",
	[esr_ec_trapped_mrc_or_mrs_access_to_cp10_for_id_group_traps]
	    = "[EL2 EXCEPTION] trapped mrc or mrs access to cp10 for id group traps",
	[esr_ec_trapped_mrrc_or_mcrr_access_to_cp14]
	    = "[EL2 EXCEPTION] trapped mrrc or mcrr access to cp14",
	[esr_ec_illegal_execution_state]
	    = "[EL2 EXCEPTION] illegal execution state",
	[esr_ec_aarch32_svc_instruction_execution]
	    = "[EL2 EXCEPTION] aarch32 svc instruction execution",
	[esr_ec_aarch32_hvc_instruction_execution]
	    = "[EL2 EXCEPTION] aarch32 hvc instruction execution",
	[esr_ec_aarch32_smc_instruction_execution]
	    = "[EL2 EXCEPTION] aarch32 smc instruction execution",
	[esr_ec_aarch64_svc_instruction_execution]
	    = "[EL2 EXCEPTION] aarch64 svc instruction execution",
	[esr_ec_aarch64_hvc_instruction_execution]
	    = "[EL2 EXCEPTION] aarch64 hvc instruction execution",
	[esr_ec_aarch64_smc_instruction_execution]
	    = "[EL2 EXCEPTION] aarch64 smc instruction execution",
	[esr_ec_other_msr_mrs_or_system_instruction_execution]
	    = "[EL2 EXCEPTION] other msr mrs or system instruction execution",
	[esr_ec_prefetch_abort_from_a_lower_exception_level]
	    = "[EL2 EXCEPTION] prefetch abort from a lower exception level",
	[esr_ec_prefetch_abort_taken_without_a_change_in_exception_level]
	    = "[EL2 EXCEPTION] prefetch abort taken without a change in exception level",
	[esr_ec_misaligned_pc_exception]
	    = "[EL2 EXCEPTION] misaligned pc exception",
	[esr_ec_data_abort_from_a_lower_exception_level]
	    = "[EL2 EXCEPTION] data abort from a lower exception level",
	[esr_ec_data_abort_taken_without_a_change_in_exception_level]
	    = "[EL2 EXCEPTION] data abort taken without a change in exception level",
	[esr_ec_stack_pointer_alignment_exception]
	    = "[EL2 EXCEPTION] stack pointer alignment exception",
	[esr_ec_aarch32_floating_point_exception]
	    = "[EL2 EXCEPTION] aarch32 floating point exception",
	[esr_ec_aarch64_floating_point_exception]
	    = "[EL2 EXCEPTION] aarch64 floating point exception",
	[esr_ec_serror_interrupt]
	    = "[EL2 EXCEPTION] serror interrupt",
	[esr_ec_breakpoint_exception_from_a_lower_exception_level]
	    = "[EL2 EXCEPTION] breakpoint exception from a lower exception level",
	[esr_ec_breakpoint_exception_taken_without_a_change_in_exception_level]
	    = "[EL2 EXCEPTION] breakpoint exception taken without a change in exception level",
	[esr_ec_software_step_exception_taken_from_a_lower_exception_level]
	    = "[EL2 EXCEPTION] software step exception taken from a lower exception level",
	[esr_ec_software_step_exception_taken_without_a_change_in_exception_level]
	    = "[EL2 EXCEPTION] software step exception taken without a change in exception level",
	[esr_ec_watchpoint_exception_from_a_lower_exception_level]
	    = "[EL2 EXCEPTION] watchpoint exception from a lower exception level",
	[esr_ec_watchpoint_exception_taken_without_a_change_in_exception_level]
	    = "[EL2 EXCEPTION] watchpoint exception taken without a change in exception level",
	[esr_ec_bkpt_instruction_execution]
	    = "[EL2 EXCEPTION] bkpt instruction execution",
	[esr_ec_vector_catch_exception_from_aach32_state]
	    = "[EL2 EXCEPTION] vector catch exception from aach32 state",
	[esr_ec_brk_instruction_execution]
	    = "[EL2 EXCEPTION] brk instruction execution",
};

static uh_handler_list_t uh_handler_list;

void uh_fault_handler(void)
{
	unsigned int cpu;
	uh_handler_data_t *uh_handler_data;
	u32 exception_class;
	unsigned long flags;
	struct pt_regs regs;

	spin_lock_irqsave(&uh_fault_lock, flags);

	cpu = smp_processor_id();
	uh_handler_data = &uh_handler_list.uh_handler_data[cpu];
	exception_class = uh_handler_data->esr_el2.ec;

	if (!exception_class_string[exception_class]
	    || exception_class > esr_ec_brk_instruction_execution)
		exception_class = esr_ec_unknown_reason;
	pr_alert("=============uH fault handler logging=============\n");
	pr_alert("%s",exception_class_string[exception_class]);
	pr_alert("[System registers]\n", cpu);
	pr_alert("ESR_EL2: %x\tHCR_EL2: %llx\tHPFAR_EL2: %llx\n",
		 uh_handler_data->esr_el2.bits,
		 uh_handler_data->hcr_el2, uh_handler_data->hpfar_el2);
	pr_alert("FAR_EL2: %llx\tELR_EL2: %llx\n", uh_handler_data->far_el2,
		 uh_handler_data->elr_el2);

	memset(&regs, 0, sizeof(regs));
	memcpy(&regs, &uh_handler_data->regs, sizeof(uh_handler_data->regs));

	do_mem_abort(uh_handler_data->far_el2, (u32)uh_handler_data->esr_el2.bits, &regs);
	panic("uH Fault handler : %s",exception_class_string[exception_class]);
}

u64 uh_get_fault_handler(void)
{
	uh_handler_list.uh_handler = (u64) & uh_fault_handler;
	return (u64) & uh_handler_list;
}
