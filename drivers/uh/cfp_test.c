#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/slab.h>

#include <linux/vmalloc.h>
#include <linux/sched/signal.h>

unsigned long x17 = 0x7777;
unsigned long ret_addr = 0x8888;

/*
 * For storing callsite and calltarget
 * Give enough space about around 20k 
 * callsites or calltargets
 */

#define MAXPAIR 20*1024
unsigned long jopp_array[MAXPAIR*2];

#ifdef CONFIG_RKP_CFP_ROPP

#ifdef CONFIG_RKP_CFP_ROPP_SYSREGKEY
static void ropp_readbvr(void)
{
	unsigned long bcr_val = 0xcccc;
	unsigned long bvr_val = 0xbbbb;

	asm volatile("mrs %0, DBGBCR5_EL1" : "=r" (bcr_val));
	asm volatile("mrs %0, DBGBVR5_EL1" : "=r" (bvr_val));
	printk(KERN_INFO "CFP_TEST SYSREG BCR5 %lx BVR5 %lx\n", bcr_val, bvr_val);
	printk(KERN_INFO "CFP_TEST ropp master key =%lx\n", ropp_master_key);
}

static void ropp_writebvr(void)
{
	unsigned long bvr_val = 0x5555;

	printk("SYSREG set BVR val\n");
	asm volatile("msr DBGBVR5_EL1, %0" :: "r" (bvr_val));

	ropp_readbvr();
}
#else 

static void ropp_readbvr(void) { return;}
static void ropp_writebvr(void) { return;}

#endif

static void traversal_thread_group(struct task_struct *tsk)
{
	struct task_struct *curr_tsk = NULL;
	struct thread_info *thread = NULL;
	unsigned long tg_offset = offsetof(struct task_struct, thread_group);

	curr_tsk = (struct task_struct *) (((unsigned long)tsk->thread_group.next) - tg_offset);
	/*if (curr_tsk == tsk){*/
	/*printk("Thread Group is empty!");*/
	/*return;*/
	/*}*/
	while (curr_tsk != tsk) {
		thread = task_thread_info(curr_tsk);
		printk(KERN_INFO "    CFP_TEST rrk=0x%16lx, pid=%ld comm=%s\n", thread->rrk, (unsigned long)curr_tsk->pid, curr_tsk->comm);
		curr_tsk = (struct task_struct *) (((unsigned long)curr_tsk->thread_group.next) - tg_offset);
	}
}

static void ropp_print_all_rrk(void)
{
	struct task_struct *tsk;
	struct thread_info *thread;
	int cpu_num;

	ropp_readbvr();
	printk(KERN_INFO "CFP_TEST: %s:\n", __func__);

	// print all swappers
	for (cpu_num = 0; cpu_num < NR_CPUS; cpu_num++) {
		tsk = idle_task(cpu_num);
		thread = task_thread_info(tsk);
		printk(KERN_INFO "CFP_TEST rrk=0x%16lx, pid=%ld comm=%s\n", thread->rrk, (unsigned long)tsk->pid, tsk->comm);
	}

	for_each_process(tsk) {
		thread = task_thread_info(tsk);
		printk(KERN_INFO "CFP_TEST rrk=0x%16lx, pid=%ld comm=%s\n", thread->rrk, (unsigned long)tsk->pid, tsk->comm);
		traversal_thread_group(tsk);
	}
}

static void test_smc(void)
{
	unsigned long x17 = 0xdeadc0de;

	asm volatile("mov %0, x17" : "=r" (x17));
	printk(KERN_INFO "Before SMC, x17=%lx\n", x17);

	asm volatile("smc 0x0");

	asm volatile("mov %0, x17" : "=r" (x17));
	printk(KERN_INFO "After SMC, x17=%lx\n", x17);
}

static void print_ra(void)
{
	printk(KERN_INFO "x17=%lx, ret_addr=%lx\n", x17, ret_addr);
}
#endif

void cfp_record_jopp(unsigned long calltarget, unsigned long callsite){
	unsigned int i = 0;
	/*
	 * Need to confirm
	 * 1) one callsite, call many calltargets
	 * 2) one calltarget, reached by many callsites
	 */

	for (i=0; i<MAXPAIR; i++){
		if ((jopp_array[i*2] == callsite) && (jopp_array[i*2+1] == calltarget)){
			break;
		}

		if (jopp_array[i*2] == 0){
			jopp_array[i*2] = callsite;
			jopp_array[i*2+1] = calltarget;
			break;
		}
	}
}

static int cfp_is_prefix(const char *prefix, const char *string)
{
	return strncmp(prefix, string, strlen(prefix)) == 0;
}

static ssize_t cfp_write(struct file *file, const char __user *buf,
				size_t datalen, loff_t *ppos)
{
	char *data = NULL;
	ssize_t result = 0;
	unsigned int i = 0;

	if (datalen >= PAGE_SIZE)
		datalen = PAGE_SIZE - 1;

	/* No partial writes. */
	result = -EINVAL;
	if (*ppos != 0)
		goto out;

	result = -ENOMEM;
	data = kmalloc(datalen + 1, GFP_KERNEL);
	if (!data)
		goto out;

	*(data + datalen) = '\0';

	result = -EFAULT;
	if (copy_from_user(data, buf, datalen))
		goto out;

#ifdef CONFIG_RKP_CFP_ROPP
	if (cfp_is_prefix("print_rrk", data)) {
		ropp_print_all_rrk();
	} else if (cfp_is_prefix("print_ra", data)) {
		print_ra();
	} else if (cfp_is_prefix("test_smc", data)) {
		test_smc();
	} else if (cfp_is_prefix("readbvr", data)) {
		ropp_readbvr();
	} else if (cfp_is_prefix("writebvr", data)) {
		ropp_writebvr();
	} else {
		result = -EINVAL;
	}
#endif

#ifdef CONFIG_RKP_CFP_JOPP
	if (cfp_is_prefix("print_fp", data)) {
		//extern unsigned long *__boot_kernel_offset; 
		//unsigned long *kernel_addr = (unsigned long *) &__boot_kernel_offset;
		//unsigned long offset = kernel_addr[1]- kernel_addr[2]; 

		for (i=0; i<MAXPAIR; i++){
			if (jopp_array[i*2] == 0)
				break;
			printk("callsite target %p %p\n", (void *)(jopp_array[i*2]), (void *)(jopp_array[i*2+1]));
		}
	} else {
		result = -EINVAL;
	}
#endif
out:
	kfree(data);
	return result;
}

ssize_t	cfp_read(struct file *filep, char __user *buf, size_t size, loff_t *offset)
{
	printk("echo print_rrk      > /proc/cfp_test  --> will print all RRK\n");
	return 0;
}

static const struct file_operations cfp_proc_fops = {
	.read		= cfp_read,
	.write		= cfp_write,
};

static int __init cfp_test_read_init(void)
{
	if (proc_create("cfp_test", 0644, NULL, &cfp_proc_fops) == NULL) {
		printk(KERN_ERR "%s: Error creating proc entry\n", __func__);
		goto error_return;
	}
	return 0;

error_return:
	return -1;
}

static void __exit cfp_test_read_exit(void)
{
	remove_proc_entry("cfp_test", NULL);
	printk(KERN_INFO"Deregistering /proc/cfp_test Interface\n");
}

module_init(cfp_test_read_init);
module_exit(cfp_test_read_exit);
