#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/rkp.h>
#include <linux/uh.h>
#include <linux/sched/signal.h>

/*
 * BIT[0:1]	TYPE	PXN BIT
 * 01		BLOCK	53	For LEVEL 0, 1, 2 //defined by L012_BLOCK_PXN
 * 11		TABLE	59	For LEVEL 0, 1, 2 //defined by L012_TABLE_PXN
 * 11		PAGE	53	For LEVEL 3       //defined by L3_PAGE_PXN
 */
#define L012_BLOCK_PXN (_AT(pmdval_t, 1) << 53)
#define L012_TABLE_PXN (_AT(pmdval_t, 1) << 59)
#define L3_PAGE_PXN    (_AT(pmdval_t, 1) << 53)

#define MEM_END		0xfffffffffffff000 /* 4K aligned */
#define DESC_MASK	0xFFFFFFFFF000

#define RKP_PA_READ	0
#define RKP_PA_WRITE	1

/**********************************************************
 *			FIMC defines
 **********************************************************/
#ifdef CONFIG_USE_DIRECT_IS_CONTROL //which means FIMC
#define FIMC_LIB_OFFSET_VA		(VMALLOC_START + 0xF6000000 - 0x8000000)
#define FIMC_LIB_START_VA		(FIMC_LIB_OFFSET_VA + 0x04000000)

#define VRA_START_VA	FIMC_LIB_START_VA
// #define VRA_CODE_SIZE	0x40000 /* for Great and Crown */
#define VRA_CODE_SIZE	0x80000
#define VRA_DATA_SIZE	0x40000

#define DDK_START_VA	(FIMC_LIB_START_VA + VRA_CODE_SIZE + VRA_DATA_SIZE)
// #define DDK_CODE_SIZE	0x300000 /* for Great and Crown */
#define DDK_CODE_SIZE	0x340000
#define DDK_DATA_SIZE	0x100000

#define RTA_START_VA	(DDK_START_VA + DDK_CODE_SIZE + DDK_DATA_SIZE)

/*#define RTA_CODE_SIZE	0x100000	*//* for Dream and Star */
/*#define RTA_DATA_SIZE	0x200000*/
//#define RTA_CODE_SIZE	0x180000	/* for Great and Crown*/
//#define RTA_DATA_SIZE	0x180000
#define RTA_CODE_SIZE	0x200000	/* for Beyond*/
#define RTA_DATA_SIZE	0x200000

#define FIMC_LIB_END_VA (RTA_START_VA+RTA_CODE_SIZE+RTA_DATA_SIZE)
#endif

struct test_data_struct{
	u64 iter;
	u64 pxn;
	u64 no_pxn;
	u64 read;
	u64 write;
	u64 cred_bkptr_match;
	u64 cred_bkptr_mismatch;
};

static DEFINE_RAW_SPINLOCK(par_lock);

#define RKP_BUF_SIZE	4096*2
#define RKP_LINE_MAX	80

char rkp_test_buf[RKP_BUF_SIZE];
unsigned long rkp_test_len = 0;
unsigned long prot_user_l2 = 1;

u64 *ha1;
u64 *ha2;

void buf_print(const char *fmt, ...)
{
	va_list aptr;

	if (rkp_test_len > RKP_BUF_SIZE-RKP_LINE_MAX)
		return;
	va_start(aptr, fmt);
	rkp_test_len += vsprintf(rkp_test_buf+rkp_test_len, fmt, aptr);
	va_end(aptr);
}

//if RO, return true; RW return false
bool hyp_check_page_ro(u64 va)
{
	unsigned long flags;
	u64 par = 0;

	raw_spin_lock_irqsave(&par_lock, flags);
	uh_call(UH_APP_RKP, CMD_ID_TEST_GET_PAR, (unsigned long)va, RKP_PA_WRITE, 0, 0);
	par = *ha1;
	raw_spin_unlock_irqrestore(&par_lock, flags);

	return (par & 0x1) ? true : false;
}

static void hyp_check_range_rw(u64 va, u64 count, u64 *ro, u64 *rw)
{
	unsigned long  flags;

	raw_spin_lock_irqsave(&par_lock, flags);
	uh_call(UH_APP_RKP, CMD_ID_TEST_GET_RO, (unsigned long)va, (unsigned long)count, 0, 0);
	*ro = *ha1;
	*rw = *ha2;
	raw_spin_unlock_irqrestore(&par_lock, flags);
}

static void hyp_check_l23pgt_rw(u64 *pg_l, unsigned int level, struct test_data_struct *test)
{
	unsigned int i;

	// Level is 0 1 2
	if (level >= 3) 
		return;

	for (i = 0; i < 512; i++) {
		if ((pg_l[i] & 3) == 3) {
			test[level].iter++;
			if (hyp_check_page_ro((u64)phys_to_virt(pg_l[i] & DESC_MASK))) {
				test[level].read++;
			} else {
				test[level].write++;
			}
			hyp_check_l23pgt_rw((u64 *) (phys_to_virt(pg_l[i] & DESC_MASK)), level + 1, test);
		}
	}
}

static pmd_t * get_addr_pmd(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;

	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd))
		return NULL;

	pud = pud_offset(pgd, addr);
	if (pud_none(*pud))
		return NULL;

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd))
		return NULL;
	
	return pmd; 
}

static int test_case_user_pgtable_ro(void)
{
	struct task_struct *task;
	struct test_data_struct test[3] = {{0}, {0}, {0} };
	int i;
	struct mm_struct *mm = NULL;

	for_each_process(task) {
		mm = task->active_mm;
		if (!(mm) || !(mm->context.id.counter)) {
			continue;
		}
		if (!(mm->pgd))
			continue;
		if (hyp_check_page_ro((u64)(mm->pgd))) {
			test[0].read++;
		} else {
			test[0].write++;
		}
		test[0].iter++;
		hyp_check_l23pgt_rw(((u64 *) (mm->pgd)), 1, test);
	}
	for (i = 0; i < 3; i++)	{
		buf_print("\t\tL%d TOTAL PAGES %6llu | READ ONLY %6llu | WRITABLE %6llu\n",
			i+1, test[i].iter, test[i].read, test[i].write);
	}

	//L1 and L2 pgtable should be RO
	if ((!prot_user_l2) && (0 == test[0].write))
		return 0;
	if ((0 == test[0].write) && (0 == test[1].write))
		return 0; //pass
	else
		return 1; //fail
}

static int test_case_kernel_pgtable_ro(void)
{
	struct test_data_struct test[3] = {{0}, {0}, {0} };
	int i = 0;

	// Check for swapper_pg_dir
	test[0].iter++;
	if (hyp_check_page_ro((u64)swapper_pg_dir)) {
		test[0].read++;
	} else {
		test[0].write++;
	}
	hyp_check_l23pgt_rw((u64 *)swapper_pg_dir, 1, test);

	for (i = 0; i < 3; i++)
		buf_print("\t\tL%d TOTAL PAGE TABLES %6llu | READ ONLY %6llu |WRITABLE %6llu\n",
			i+1, test[i].iter, test[i].read, test[i].write);

	if ((0 == test[0].write) && (0 == test[1].write))
		return 0;
	else
		return 1;
}

static int test_case_kernel_l3pgt_ro(void)
{
	int rw = 0, ro = 0, i = 0;
	u64 addrs[] = {
		(u64)_text,
		(u64)_etext
	};
	int len = sizeof(addrs)/sizeof(u64);

	pmd_t * pmd;
	u64 pgt_addr;

	for (i=0; i<len; i++) {
		pmd = get_addr_pmd(&init_mm, addrs[i]);

		pgt_addr = (u64)phys_to_virt(((u64)(pmd_val(*pmd))) & DESC_MASK);
		if (hyp_check_page_ro(pgt_addr))
			ro ++;
		else
			rw ++;
	}

	buf_print("\t\tKERNEL TEXT HEAD TAIL L3PGT | RO %6u | RW %6u\n", ro, rw);
	return (rw == 0) ? 0 : 1;
}

static void page_pxn_set(struct mm_struct *mm, unsigned long addr, u64 *xn, u64 *x)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd))
		return;

	pud = pud_offset(pgd, addr);
	if (pud_none(*pud))
		return;

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd))
		return;

	if (pmd_sect(*pmd)) {
		if ((pmd_val(*pmd) & L012_BLOCK_PXN) > 0)
			*xn +=1;
		else 
			*x +=1;
		return;
	}
	else {
		if ((pmd_val(*pmd) & L012_TABLE_PXN) > 0) {
			*xn +=1;
			return;
		}
	}

	// If pmd is table, such as kernel text head and tail, need to check L3
	pte = pte_offset_map(pmd, addr);
	if (!pte_none(*pte)) {
		if ((pte_val(*pte) & L3_PAGE_PXN) > 0)
			*xn +=1;
		else 
			*x +=1;
	}

	pte_unmap(pte);
}

static void count_pxn(unsigned long pxn, int level, struct test_data_struct *test){
	test[level].iter ++;	
	if (pxn)
		test[level].pxn ++;	
	else
		test[level].no_pxn ++;	
}

static void walk_pte(pmd_t *pmd, int level, struct test_data_struct *test)
{
	pte_t *pte = pte_offset_kernel(pmd, 0UL);
	unsigned i;
	unsigned long prot;

	for (i = 0; i < PTRS_PER_PTE; i++, pte++) {
		if (pte_none(*pte)){
			continue;
		} else {
			prot = pte_val(*pte) & L3_PAGE_PXN;
			count_pxn(prot, level, test);
		}
	}
}

static void walk_pmd(pud_t *pud, int level, struct test_data_struct *test)
{
	pmd_t *pmd = pmd_offset(pud, 0UL);
	unsigned i;
	unsigned long prot;

	for (i = 0; i < PTRS_PER_PMD; i++, pmd++) {
		if (pmd_none(*pmd)){
			continue;
		} else if (pmd_sect(*pmd)) {
			prot = pmd_val(*pmd) & L012_BLOCK_PXN;
			count_pxn(prot, level, test);
		} else {
		/*
		 * For user space, all L2 should have PXN, including block and
		 * table. Only kernel text head and tail L2 table can have no
		 * pxn, and kernel text middle L2 blocks can have no pxn
		 */
			BUG_ON(pmd_bad(*pmd));
			prot = pmd_val(*pmd) & L012_TABLE_PXN;
			count_pxn(prot, level, test);
			walk_pte(pmd, level+1, test);
		}
	}
}

static void walk_pud(pgd_t *pgd, int level, struct test_data_struct *test)
{
	pud_t *pud = pud_offset(pgd, 0UL);
	unsigned i;

	for (i = 0; i < PTRS_PER_PUD; i++, pud++) {
		if (pud_none(*pud) || pud_sect(*pud)) {
			continue;
		} else {
			BUG_ON(pud_bad(*pud));
			walk_pmd(pud, level, test);
		}
	}
}

#define rkp_pgd_table		(_AT(pgdval_t, 1) << 1)
#define rkp_pgd_bad(pgd)		(!(pgd_val(pgd) & rkp_pgd_table))
static void walk_pgd(struct mm_struct *mm, int level, struct test_data_struct *test)
{
	pgd_t *pgd = pgd_offset(mm, 0UL);
	unsigned i;
	unsigned long prot;

	for (i = 0; i < PTRS_PER_PGD; i++, pgd++) {
		if (rkp_pgd_bad(*pgd)) {
			continue;
		} else { //table
			prot = pgd_val(*pgd) & L012_TABLE_PXN;
			count_pxn(prot, level, test);

			walk_pud(pgd, level+1, test);
		}
	}
}

static int test_case_user_pxn(void)
{
	struct task_struct *task = NULL;
	struct mm_struct *mm = NULL;
	struct test_data_struct test[3] = {{0}, {0}, {0} };
	int i = 0;

	for_each_process(task) {
		mm = task->active_mm;
		if (!(mm) || !(mm->context.id.counter)) {
			continue;
		}
		if (!(mm->pgd))
			continue;

		/* Check if PXN bit is set */
		walk_pgd(mm, 0, test);
	}

	for (i = 0; i < 3; i++)	{
		buf_print("\t\tL%d TOTAL ENTRIES %6llu | PXN %6llu | NO_PXN %6llu\n",
			i+1, test[i].iter, test[i].pxn, test[i].no_pxn);
	}

	//all 2nd level entries should be PXN
	if (0 == test[0].no_pxn) {
		prot_user_l2 = 0;
		return 0;
	}
	else if (0 == test[1].no_pxn) {
		prot_user_l2 = 1;
		return 0;
	}
	else
		return 1;
}

static void range_pxn_set(unsigned long va_start, unsigned long count, u64 *xn, u64 *x)
{
	unsigned long i;
	for (i = 0; i < count; i++) {
		 page_pxn_set(&init_mm, (va_start + i*PAGE_SIZE), xn, x);
	}
}

struct mem_range_struct {
	u64 start_va;
	u64 size; //in bytes
	char *info;
	bool no_rw;
	bool no_x;
};

static int test_case_kernel_range_rwx(void)
{
	int ret = 0;
	u64 ro = 0, rw = 0;
	u64 xn = 0, x = 0;
	int i;
	u64 fixmap_va = __fix_to_virt(FIX_ENTRY_TRAMP_TEXT1);

	struct mem_range_struct test_ranges[] = {
		{(u64)VMALLOC_START,		((u64)_text) - ((u64)VMALLOC_START),	"VMALLOC -  STEXT", false, true},
		{((u64)_text),			((u64)_etext) - ((u64)_text),		"STEXT - ETEXT   ", true, false},

		// For STAR, two bit maps are between etext and srodata
		{((u64)_etext),			((u64) __end_rodata) - ((u64)_etext),	"ETEXT -  ERODATA", true, true},
		// For STAR, FIMC is after erodata
		{((u64) __end_rodata),		VRA_START_VA-((u64) __end_rodata),	"ERODATA - S_FIMC", false, true},
		{VRA_START_VA,			VRA_CODE_SIZE,				"     VRA CODE   ", true, false},
		{VRA_START_VA+VRA_CODE_SIZE,	VRA_DATA_SIZE,				"     VRA DATA   ", false, true},
		{DDK_START_VA,			DDK_CODE_SIZE,				"     DDK CODE   ", true, false},
		{DDK_START_VA+DDK_CODE_SIZE,	DDK_DATA_SIZE,				"     DDK_DATA   ", false, true},
		{RTA_START_VA,			RTA_CODE_SIZE,				"     RTA CODE   ", true, false},
		{RTA_START_VA+RTA_CODE_SIZE,	RTA_DATA_SIZE,				"     RTA DATA   ", false, true},
		{((u64)FIMC_LIB_END_VA),	fixmap_va - ((u64)FIMC_LIB_END_VA),	"FIMC_END- FIXMAP", false, true},
		{((u64)fixmap_va),		((u64) PAGE_SIZE),			"     FIXMAP     ", true, false},
		{((u64)fixmap_va+PAGE_SIZE),	((u64) MEM_END-(fixmap_va+PAGE_SIZE)),	"FIXMAP - MEM_END", false, true},
	};

	int len = sizeof(test_ranges)/sizeof(struct mem_range_struct);

	buf_print("\t\t| MEMORY RANGES  | %16s - %16s | %8s %8s %8s %8s\n",
		"START", "END", "RO", "RW", "PXN", "PX");
	for (i = 0; i < len; i++) {
		hyp_check_range_rw(test_ranges[i].start_va, test_ranges[i].size/PAGE_SIZE, &ro, &rw);
		range_pxn_set(test_ranges[i].start_va, test_ranges[i].size/PAGE_SIZE, &xn, &x);

		buf_print("\t\t|%s| %016llx - %016llx | %8llu %8llu %8llu %8llu\n",
			test_ranges[i].info, test_ranges[i].start_va,
			test_ranges[i].start_va + test_ranges[i].size,
			ro, rw, xn, x);

		if (test_ranges[i].no_rw && (0 != rw)) {
			buf_print("RKP_TEST FAILED, NO RW PAGE ALLOWED, rw=%llu\n", rw);
			ret++;
		}

		if (test_ranges[i].no_x && (0 != x)) {
			buf_print("RKP_TEST FAILED, NO X PAGE ALLOWED, x=%llu\n", x);
			ret++;
		}

		if ((0 != rw) && (0 != x)) {
			buf_print("RKP_TEST FAILED, NO RWX PAGE ALLOWED, rw=%llu, x=%llu\n", rw, x);
			ret++;
		}

		ro = 0; rw = 0;	
		xn = 0; x = 0;
	}

	return ret;
}

ssize_t	rkp_read(struct file *filep, char __user *buffer, size_t count, loff_t *ppos)
{
	int ret = 0, temp_ret = 0, i = 0;
	struct test_case_struct test_cases[] = {
		{test_case_user_pxn,		"TEST USER_PXN"},
		{test_case_user_pgtable_ro,	"TEST USER_PGTABLE_RO"},
		{test_case_kernel_pgtable_ro,	"TEST KERNEL_PGTABLE_RO"},
		{test_case_kernel_l3pgt_ro,	"TEST KERNEL TEXT HEAD TAIL L3PGT RO"},
		{test_case_kernel_range_rwx,	"TEST KERNEL_RANGE_RWX"},
	};
	int tc_num = sizeof(test_cases)/sizeof(struct test_case_struct);

	static bool done = false;
	if (done)
		return 0;
	done = true;

	if ((!ha1) || (!ha2)) {
		buf_print("ERROR RKP_TEST ha1 is NULL\n");
		goto error;
	}

	for (i = 0; i < tc_num; i++) {
		buf_print( "RKP_TEST_CASE %d ===========> RUNNING %s\n", i, test_cases[i].describe);
		temp_ret = test_cases[i].fn();

		if (temp_ret) {
			buf_print("RKP_TEST_CASE %d ===========> %s FAILED WITH %d ERRORS\n", 
				i, test_cases[i].describe, temp_ret);
		} else {
			buf_print("RKP_TEST_CASE %d ===========> %s PASSED\n", i, test_cases[i].describe);
		}

		ret += temp_ret;
	}

	if (ret) {
		buf_print("RKP_TEST SUMMARY: FAILED WITH %d ERRORS\n", ret);
	} else {
		buf_print("RKP_TEST SUMMARY: PASSED\n");
	}

error:
	return simple_read_from_buffer(buffer, count, ppos, rkp_test_buf, rkp_test_len);
}

static const struct file_operations rkp_proc_fops = {
	.read		= rkp_read,
};

static int __init rkp_test_init(void)
{
	phys_addr_t ret = 0;

	if (proc_create("rkp_test", 0444, NULL, &rkp_proc_fops) == NULL) {
		printk(KERN_ERR "RKP_TEST: Error creating proc entry");
		return -1;
	}

	ret = uh_call(UH_APP_RKP, RKP_RKP_ROBUFFER_ALLOC, 1, 0, 0, 0);
	ha1 = (u64 *)(__va(ret));
	ha2 = (u64 *)(__va(ret) + 8);

	/*
	ha1 = (u64 *)(__va(RKP_ROBUF_START)+RKP_ROBUF_SIZE-16);
	ha2 = (u64 *)(__va(RKP_ROBUF_START)+RKP_ROBUF_SIZE-8);
	*/

	return 0;
}

static void __exit rkp_test_exit(void)
{
	remove_proc_entry("rkp_test", NULL);
}

module_init(rkp_test_init);
module_exit(rkp_test_exit);
