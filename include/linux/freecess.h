#ifndef KFRECESS_H
#define KFRECESS_H

#include <linux/sched.h>

#define KERNEL_ID_NETLINK      0x12341234
#define UID_MIN_VALUE          10000
#define MSG_NOOP               0
#define LOOPBACK_MSG           1
#define MSG_TO_KERN            2
#define MSG_TO_USER            3
#define MSG_TYPE_END           4

#define MOD_NOOP               0
#define MOD_BINDER             1
#define MOD_SIG                2
#define MOD_PKG                3
#define MOD_CFB                4
#define MOD_END                5

typedef enum {
	ADD_UID,
	DEL_UID,
	CLEAR_ALL_UID,
} pkg_cmd_t;

typedef struct {
	pkg_cmd_t cmd;
	uid_t uid;
}pkg_info_t;


struct kfreecess_msg_data
{
	int type;
	int mod;
	int src_portid;
	int dst_portid;
	int caller_pid;
	int target_uid;
	int flag;		//MOD_SIG,MOD_BINDER
	pkg_info_t pkg_info;	//MOD_PKG
};

typedef void (*freecess_hook)(void* data, unsigned int len);

int sig_report(struct task_struct *caller, struct task_struct *p);
int binder_report(struct task_struct *caller, struct task_struct *p, int flag);
int pkg_report(int target_uid);
int cfb_report(int target_uid, const char *reason);
int register_kfreecess_hook(int mod, freecess_hook hook);
int unregister_kfreecess_hook(int mod);
int pkg_stat_show(struct seq_file *m, void *v);
#endif