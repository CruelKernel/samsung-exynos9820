#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/rkp.h>
#include <linux/uh.h>
#include <linux/nsproxy.h>
#include <linux/security.h>
#include <../../fs/mount.h>
#include <linux/sched/signal.h>

struct task_security_struct {
	u32 osid;               /* SID prior to last execve */
	u32 sid;                /* current SID */
	u32 exec_sid;           /* exec SID */
	u32 create_sid;         /* fscreate SID */
	u32 keycreate_sid;      /* keycreate SID */
	u32 sockcreate_sid;     /* fscreate SID */
	void *bp_cred;
};

enum kdp_test{
	CMD_ID_CRED = 0,
	CMD_ID_SEC_CONTEXT,
	CMD_ID_NS,
};

#define KDP_BUF_SIZE	4096*2
#define KDP_LINE_MAX	80

char kdp_test_buf[KDP_BUF_SIZE];
unsigned long kdp_test_len = 0;

void kdp_print(const char *fmt, ...)
{
	va_list aptr;

	if (kdp_test_len > KDP_BUF_SIZE-KDP_LINE_MAX)
		return;
	va_start(aptr, fmt);
	kdp_test_len += vsprintf(kdp_test_buf+kdp_test_len, fmt, aptr);
	va_end(aptr);
}

#ifdef CONFIG_RKP_NS_PROT
struct vfsmount *get_vfsmnt(struct task_struct *p)
{
	if(!p || !(p->nsproxy) 
		||!(p->nsproxy->mnt_ns) 
		||!(p->nsproxy->mnt_ns->root))
		return NULL;

	return p->nsproxy->mnt_ns->root->mnt;
}
#endif /* CONFIG_RKP_NS_PROT */ 

extern bool hyp_check_page_ro(u64 va);
static int test_case_kdp_ro(int cmd_id)
{
	struct task_struct *p = NULL;
	u64 ro = 0, rw = 0,dst;

	for_each_process(p) {
		switch(cmd_id) {
		case CMD_ID_CRED: 
			/*Here dst points to struct cred*/
			dst =(u64)__task_cred(p);
			break;
		case CMD_ID_SEC_CONTEXT: 
			/*Here dst points to process security context*/
			dst =(u64)__task_cred(p)->security;
			break;
#ifdef CONFIG_RKP_NS_PROT
		case CMD_ID_NS: 
			/*Here dst points to process security context*/
			dst = (u64)get_vfsmnt(p);
			break;
#endif /* CONFIG_RKP_NS_PROT */ 
		}
		if(!dst)
		    continue;

		if (hyp_check_page_ro((u64)dst)) {
			ro++;
		} else {
			rw++;
		}
	}
	kdp_print("ro =%llu, rw =%llu\n", ro, rw);
	return rw ? 1 : 0;
}


static int test_case_cred_ro(void)
{
	kdp_print("CRED PROTECTION ");
	return test_case_kdp_ro(CMD_ID_CRED);
}
static int test_case_sec_context_ro(void)
{
	kdp_print("SECURITY CONTEXT PROTECTION ");
	return test_case_kdp_ro(CMD_ID_SEC_CONTEXT);
}


static int cred_match(struct task_struct *p,const struct cred *cred) 
{
	struct mm_struct *mm = p->mm;

	if(cred->bp_task != p)
		return 0;

	if(mm) {
		if(cred->bp_pgd != mm->pgd)
			return 0;
		}else {
		if(cred->bp_pgd != init_mm.pgd) 
			return 0;
	}
	return 1;
}

static int sec_context_match(const struct cred *cred) 
{
	struct task_security_struct *tsec = (struct task_security_struct *)cred->security;
    
	if((u64)tsec->bp_cred != (u64)cred) {
		return 0;
	}
	return 1;
}
static int test_case_match_bp(int cmd_id)
{
	struct task_struct *p = NULL;
	u64  match = 0, mismatch = 0 , ret  = 0;

	for_each_process(p) {
		switch(cmd_id) {
		case CMD_ID_CRED: 
			/*Here dst points to struct cred*/
			ret = cred_match(p,__task_cred(p));
			break;
		case CMD_ID_SEC_CONTEXT: 
			/*Here dst points to process security context*/
			ret = sec_context_match(__task_cred(p));
			break;
		}
		ret ? match++ : mismatch++;
	}
	kdp_print("match =%llu, mismatch =%llu\n", match, mismatch);
	return mismatch ? 1 : 0;
}
static int test_case_cred_match_bp(void)
{
	kdp_print("CRED Back Poiner check");
	return test_case_match_bp(CMD_ID_CRED);
}

static int test_case_sec_context_match_bp(void)
{
	kdp_print("Security Context Back Poiner check");
	return test_case_match_bp(CMD_ID_SEC_CONTEXT);
}


#ifdef CONFIG_RKP_NS_PROT
static int test_case_ns_ro(void)
{
	kdp_print("NAMESPACE PROTECTION ");
	return test_case_kdp_ro(CMD_ID_NS);
}
#endif /* CONFIG_RKP_NS_PROT */ 



ssize_t	kdp_read(struct file *filep, char __user *buffer, size_t count, loff_t *ppos)
{
	int ret = 0, temp_ret = 0, i = 0;
	struct test_case_struct kdp_test_case[] = {
		{test_case_cred_ro,		"TEST TASK_CRED_RO"},
		{test_case_sec_context_ro,	"TEST TASK_SECURITY_CONTEXT_RO"},
		{test_case_cred_match_bp,	"TEST CRED_MATCH_BACKPOINTERS"},
		{test_case_sec_context_match_bp,"TEST TASK_SEC_CONTEXT_BACKPOINTER"},
#ifdef CONFIG_RKP_NS_PROT
		{test_case_ns_ro,	"TEST NAMESPACE_RO"},
#endif /* CONFIG_RKP_NS_PROT */ 
	};
	int tc_num = sizeof(kdp_test_case)/sizeof(struct test_case_struct);

	static bool done = false;
	if (done)
		return 0;
	done = true;


	for (i = 0; i < tc_num; i++) {
		kdp_print( "KDP_TEST_CASE %d ===========> RUNNING %s\n", i, kdp_test_case[i].describe);
		temp_ret = kdp_test_case[i].fn();

		if (temp_ret) {
			kdp_print("KDP_TEST_CASE %d ===========> %s FAILED WITH %d ERRORS\n", 
				i, kdp_test_case[i].describe, temp_ret);
		} else {
		kdp_print("KDP_TEST_CASE %d ===========> %s PASSED\n", i, kdp_test_case[i].describe);
		}

		ret += temp_ret;
	}

	if (ret) {
		kdp_print("KDP_TEST SUMMARY: FAILED WITH %d ERRORS\n", ret);
	} else {
		kdp_print("KDP_TEST SUMMARY: PASSED\n");
	}

	return simple_read_from_buffer(buffer, count, ppos, kdp_test_buf, kdp_test_len);
}

static const struct file_operations kdp_proc_fops = {
	.read		= kdp_read,
};

static int __init kdp_test_init(void)
{
	if (proc_create("kdp_test", 0444, NULL, &kdp_proc_fops) == NULL) {
		printk(KERN_ERR "KDP_TEST: Error creating proc entry");
		return -1;
	}

	return 0;
}

static void __exit kdp_test_exit(void)
{
	remove_proc_entry("kdp_test", NULL);
}

module_init(kdp_test_init);
module_exit(kdp_test_exit);
