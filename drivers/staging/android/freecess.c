/*
 * Copyright (c) Samsung Technologies Co., Ltd. 2001-2017. All rights reserved.
 *
 * File name: freecess.c
 * Description: Use to thaw process from state 'D'
 * Author: chao.gu@samsung.com
 * Version: 0.2
 * Date:  2017/07/17
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/freecess.h>
#include <linux/freezer.h>
#include <net/sock.h>
#include <linux/hrtimer.h>
#include <linux/proc_fs.h>


#define RET_OK   0
#define RET_ERR  1
static struct sock *kfreecess_mod_sock = NULL;
static atomic_t bind_port[MOD_END];
static atomic_t kfreecess_init_suc;
static int last_kill_pid = -1;
static struct proc_dir_entry *freecess_rootdir = NULL;

struct report_stat_s
{
	spinlock_t lock;
	struct {
		u64 report_suc_count;
		u64 report_fail_count;
		u64 total_runtime;

		u64 report_suc_from_windowstart;
		u64 report_fail_from_windowstart;
		u64 runtime_from_windowstart;
	}data;
	char name[32];
};

struct freecess_info_s
{
	struct report_stat_s mod_reportstat[MOD_END];
} freecess_info;

static char* mod_name[MOD_END] =
{
	"NULL",
	"MOD_BINDER",
	"MOD_SIG",
	"MOD_PKG",
	"MOD_CFB"
};

struct priv_data
{
	int caller_pid;
	int target_uid;
	int flag;		    //MOD_SIG,MOD_BINDER
	pkg_info_t pkg_info;	//MOD_PKG
};

static freecess_hook mod_recv_handler[MOD_END];

static int check_msg_type(int type)
{
	return (type < MSG_TYPE_END) && (type > 0);
}

static int check_mod_type(int mod)
{
	return (mod < MOD_END) && (mod > 0);
}

static int thread_group_is_frozen(struct task_struct* task)
{
	struct task_struct *leader = task->group_leader;

	return (freezing(leader) || frozen(leader));
}

static void dump_kfreecess_msg(struct kfreecess_msg_data *msg)
{
	printk(KERN_ERR "-----kfreecess msg dump-----\n");
	if (!msg) {
		printk(KERN_ERR "msg is NULL");
		return;
	}

	printk(KERN_ERR "type: %d\n", msg->type);
	printk(KERN_ERR "mode: %d\n", msg->mod);
	printk(KERN_ERR "src_portid: %d\n", msg->src_portid);
	printk(KERN_ERR "dest_portid: %d\n", msg->dst_portid);
	printk(KERN_ERR "caller_pid: %d\n", msg->caller_pid);
	printk(KERN_ERR "target_uid: %d\n", msg->target_uid);
}


int mod_sendmsg(int type, int mod, struct priv_data* data)
{
	int ret, msg_len = 0;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct kfreecess_msg_data *payload = NULL;

	if (!atomic_read(&kfreecess_init_suc))
		return RET_ERR;

	if (!check_msg_type(type)) {
		pr_err("%s: msg type is invalid! %d\n", __func__, type);
		return RET_ERR;
	}

	if (!check_mod_type(mod)) {
		pr_err("%s: mod type is invalid! %d\n", __func__, mod);
		return RET_ERR;
	}

	msg_len = sizeof(struct	kfreecess_msg_data);
	skb = nlmsg_new(msg_len, GFP_ATOMIC);
	if (!skb) {
		pr_err("%s alloc_skb failed! %d\n", __func__, mod);
		return RET_ERR;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, msg_len, 0);
	if (!nlh) {
		kfree_skb(skb);
		return RET_ERR;
	}

	payload = nlmsg_data(nlh);
	payload->type = type;
	payload->mod = mod;
	payload->src_portid = KERNEL_ID_NETLINK;
	payload->dst_portid = atomic_read(&bind_port[mod]);

	if (data) {
		payload->caller_pid = data->caller_pid;
		payload->target_uid = data->target_uid;
		if (payload->mod == MOD_PKG)
			memcpy(&payload->pkg_info, &data->pkg_info, sizeof(pkg_info_t));
		else
			payload->flag = data->flag;
	}
	//dump_kfreecess_msg(payload);
	if ((ret = nlmsg_unicast(kfreecess_mod_sock, skb, payload->dst_portid)) < 0) {
		pr_err("nlmsg_unicast failed! %s errno %d\n", __func__ , ret);
		return RET_ERR;
	} else
		pr_info("nlmsg_unicast snd msg success\n");

	return RET_OK;
}

int sig_report(struct task_struct *caller, struct task_struct *p)
{
	int ret = RET_OK;
	struct priv_data data;
	int target_pid = task_tgid_nr(p);
	u64 walltime, timecost;
	unsigned long flags;
	struct report_stat_s *stat;

	walltime = ktime_to_us(ktime_get());
	memset(&data, 0, sizeof(struct priv_data));
	data.caller_pid = task_tgid_nr(caller);
	data.target_uid = task_uid(p).val;
	data.flag = 0;

	if (thread_group_is_frozen(p) && (target_pid != last_kill_pid)) {
		last_kill_pid = target_pid;
		stat = &freecess_info.mod_reportstat[MOD_SIG];
		ret = mod_sendmsg(MSG_TO_USER, MOD_SIG, &data);

		spin_lock_irqsave(&stat->lock, flags);
		if (ret < 0) {
			stat->data.report_fail_count++;
			stat->data.report_fail_from_windowstart++;
			pr_err("sig_report error\n");
		} else {
			stat->data.report_suc_count++;
			stat->data.report_suc_from_windowstart++;
		}

		timecost = ktime_to_us(ktime_get()) - walltime;
		stat->data.total_runtime += timecost;
		stat->data.runtime_from_windowstart += timecost;
		spin_unlock_irqrestore(&stat->lock, flags);
	}

	return ret;
}

int binder_report(struct task_struct *caller, struct task_struct *p, int flag)
{
	int ret = RET_OK;
	struct priv_data data;
	u64 walltime, timecost;
	unsigned long flags;
	struct report_stat_s *stat;

	memset(&data, 0, sizeof(struct priv_data));
	data.target_uid = -1;
	data.caller_pid = -1;
        data.flag = flag;

	if(p)
		data.target_uid = task_uid(p).val;
	if(caller)
		data.caller_pid = task_tgid_nr(caller);

	walltime = ktime_to_us(ktime_get());
	if (p && thread_group_is_frozen(p)) {
		ret = mod_sendmsg(MSG_TO_USER, MOD_BINDER, &data);
		stat = &freecess_info.mod_reportstat[MOD_BINDER];
		spin_lock_irqsave(&stat->lock, flags);
		if (ret < 0) {
			stat->data.report_fail_count++;
			stat->data.report_fail_from_windowstart++;
			pr_err("binder_report error\n");
		} else {
			stat->data.report_suc_count++;
			stat->data.report_suc_from_windowstart++;
		}

		timecost = ktime_to_us(ktime_get()) - walltime;
		stat->data.total_runtime += timecost;
		stat->data.runtime_from_windowstart += timecost;
		spin_unlock_irqrestore(&stat->lock, flags);
	}

	return ret;
}

int pkg_report(int target_uid)
{
	int ret = RET_OK;
	struct priv_data data;
	u64 walltime, timecost;
	unsigned long flags;
	struct report_stat_s *stat;

	memset(&data, 0, sizeof(struct priv_data));
	data.target_uid = target_uid;
	data.pkg_info.uid = (uid_t)target_uid;
	walltime = ktime_to_us(ktime_get());
	ret = mod_sendmsg(MSG_TO_USER, MOD_PKG, &data);
	stat = &freecess_info.mod_reportstat[MOD_PKG];
	spin_lock_irqsave(&stat->lock, flags);
	if (ret < 0) {
		stat->data.report_fail_count++;
		stat->data.report_fail_from_windowstart++;
	} else {
		stat->data.report_suc_count++;
		stat->data.report_suc_from_windowstart++;
	}

	timecost = ktime_to_us(ktime_get()) - walltime;
	stat->data.total_runtime += timecost;
	stat->data.runtime_from_windowstart += timecost;
	spin_unlock_irqrestore(&stat->lock, flags);

	return ret;
}

int cfb_report(int target_uid, const char *reason)
{
	int ret = RET_OK;
	struct priv_data data;

	printk(KERN_INFO "cfb_report: uid %d frozen, reason: %s\n", target_uid, reason);
	memset(&data, 0, sizeof(struct priv_data));
	data.target_uid = target_uid;
	ret = mod_sendmsg(MSG_TO_USER, MOD_CFB, &data);

	return ret;
}

static void recv_handler(struct sk_buff *skb)
{
	struct kfreecess_msg_data *payload = NULL;
	struct nlmsghdr *nlh = NULL;
	unsigned int msglen  = 0;
	uid_t uid = 0;

	if (!skb) {
		pr_err("recv_handler %s: skb is	NULL!\n", __func__);
		return;
	}
	
	uid = (*NETLINK_CREDS(skb)).uid.val;
	//only allow system user to communicate with Freecess kernel part.
	if (uid != 1000) {
		pr_err("freecess--uid: %d, permission denied\n", uid);
		return;
	}
	printk(KERN_ERR "kernel freecess receive msg now\n");
	if (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		msglen = NLMSG_PAYLOAD(nlh, 0);
		payload = (struct kfreecess_msg_data*)NLMSG_DATA(nlh);

		if (msglen >= (sizeof(struct kfreecess_msg_data))) {
			dump_kfreecess_msg(payload);
			if (payload->src_portid < 0) {
				pr_err("USER_HOOK_CALLBACK %s: src_portid %d is not valid!\n", __func__, payload->src_portid);
				return;
			}

			if (payload->dst_portid != KERNEL_ID_NETLINK) {
				pr_err("USER_HOOK_CALLBACK %s: dst_portid is %d not kernel!\n", __func__, payload->dst_portid);
				return;
			}
			
			if (!check_mod_type(payload->mod)) {
				pr_err("USER_HOOK_CALLBACK %s: mod %d is not valid!\n", __func__, payload->mod);
				return;
			}

			switch (payload->type) {
			case LOOPBACK_MSG:
				atomic_set(&bind_port[payload->mod], payload->src_portid);
				dump_kfreecess_msg(payload);
				mod_sendmsg(LOOPBACK_MSG, payload->mod, NULL);
				break;
			case MSG_TO_KERN:
				if (mod_recv_handler[payload->mod])
					mod_recv_handler[payload->mod](payload, sizeof(struct kfreecess_msg_data));
				else
					dump_kfreecess_msg(payload);
				break;

			default:
				pr_err("msg type is valid %d\n", payload->type);
				break;
			}
		} else {
			pr_err("%s: length err msglen %d  struct kfreecess_msg_data %lu!\n",  __func__, msglen, sizeof(struct kfreecess_msg_data));
		}
	}
}

/*proc and sysctl interface*/
static int freecess_window_stat_show(struct seq_file *m, void *v)
{
	int i;
	unsigned long flags;
	struct report_stat_s *stat;
	struct freecess_info_s tmp_freecess_info;
	struct report_stat_s total_stat;

	memset(&tmp_freecess_info, 0, sizeof(struct freecess_info_s));
	memset(&total_stat, 0, sizeof(struct report_stat_s));
	for(i = 1; i < MOD_END; i++) {
		stat = &freecess_info.mod_reportstat[i];
		spin_lock_irqsave(&stat->lock, flags);
		tmp_freecess_info.mod_reportstat[i].data = stat->data;
		spin_unlock_irqrestore(&stat->lock, flags);
		strlcpy(tmp_freecess_info.mod_reportstat[i].name, stat->name, 32);
	}

	seq_printf(m, "freecess window stat show\n");
	seq_printf(m, "-----------------------------\n\n");
	for(i = 1; i < MOD_END; i++) {
		stat = &tmp_freecess_info.mod_reportstat[i];
		total_stat.data.report_suc_from_windowstart  += stat->data.report_suc_from_windowstart;
		total_stat.data.report_fail_from_windowstart += stat->data.report_fail_from_windowstart;
		total_stat.data.runtime_from_windowstart     += stat->data.runtime_from_windowstart;

		seq_printf(m, "mod name: %s mod number %d:\n", stat->name, i);
		seq_printf(m, "suc_count: %llu\n", stat->data.report_suc_from_windowstart);
		seq_printf(m, "fail_count: %llu\n", stat->data.report_fail_from_windowstart);
		seq_printf(m, "total_runtime: %llu us\n\n", stat->data.runtime_from_windowstart);

	}

	stat = &total_stat;
	seq_printf(m, "info of all mod\n");
	seq_printf(m, "-----------------------------\n");
	seq_printf(m, "suc_count: %llu\n", stat->data.report_suc_from_windowstart);
	seq_printf(m, "fail_count: %llu\n", stat->data.report_fail_from_windowstart);
	seq_printf(m, "total_runtime: %llu us\n", stat->data.runtime_from_windowstart);
	return 0;
}

static int freecess_window_stat_open(struct inode *inode, struct file *file)
{
	 return single_open(file, freecess_window_stat_show, NULL);
}

static void reset_window(void)
{
	int i;
	unsigned long flags;
	struct report_stat_s *stat;

	for(i = 1; i < MOD_END; i++) {
		stat = &freecess_info.mod_reportstat[i];
		spin_lock_irqsave(&stat->lock, flags);
		stat->data.report_suc_from_windowstart = 0;
		stat->data.report_fail_from_windowstart = 0;
		stat->data.runtime_from_windowstart = 0;
		spin_unlock_irqrestore(&stat->lock, flags);
	}
}

static ssize_t freecess_window_stat_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	unsigned char tmp = 0;
	int value = 0;

	get_user(tmp, buf);
	value = simple_strtol(&tmp, NULL, 10);
	printk(KERN_WARNING "input value number: %d, if value == 1: window will reset ....\n", value);
	if (value == 1)
		reset_window();

	printk(KERN_WARNING "window reset now\n");
	return count;

}

static const struct file_operations window_stat_proc_fops = {
	.open   = freecess_window_stat_open,
	.read   = seq_read,
	.write   = freecess_window_stat_write,
	.llseek   = seq_lseek,
	.release   = single_release,
	.owner   = THIS_MODULE,
};

static int pkg_stat_open(struct inode *inode, struct file *file)
{
	 return single_open(file, pkg_stat_show, NULL);
}

static const struct file_operations pkg_stat_proc_fops = {
	.open     = pkg_stat_open,
	.read     = seq_read,
	.llseek   = seq_lseek,
	.release  = single_release,
};

static int freecess_modstat_show(struct seq_file *m, void *v)
{
	int i;
	unsigned long flags;
	struct report_stat_s *stat;
	static struct freecess_info_s tmp_freecess_info;
	struct report_stat_s total_stat;

	memset(&tmp_freecess_info, 0, sizeof(struct freecess_info_s));
	memset(&total_stat, 0, sizeof(struct report_stat_s));
	for(i = 1; i < MOD_END; i++) {
		stat = &freecess_info.mod_reportstat[i];
		spin_lock_irqsave(&stat->lock, flags);
		tmp_freecess_info.mod_reportstat[i].data = stat->data;
		spin_unlock_irqrestore(&stat->lock, flags);
		strlcpy(tmp_freecess_info.mod_reportstat[i].name, stat->name, 32);
	}

	seq_printf(m, "freecess mod stat show\n");
	seq_printf(m, "-----------------------------\n\n");
	for(i = 1; i < MOD_END; i++) {
		stat = &tmp_freecess_info.mod_reportstat[i];
		total_stat.data.report_suc_count    += stat->data.report_suc_count;
		total_stat.data.report_fail_count   += stat->data.report_fail_count;
		total_stat.data.total_runtime       += stat->data.total_runtime;

		seq_printf(m, "mod name: %s mod number %d:\n", stat->name, i);
		seq_printf(m, "suc_count: %llu\n", stat->data.report_suc_count);
		seq_printf(m, "fail_count: %llu\n", stat->data.report_fail_count);
		seq_printf(m, "total_runtime: %llu us\n\n", stat->data.total_runtime);
	}

	stat = &total_stat;
	seq_printf(m, "info of all mod:\n");
	seq_printf(m, "suc_count: %llu\n", stat->data.report_suc_count);
	seq_printf(m, "fail_count: %llu\n", stat->data.report_fail_count);
	seq_printf(m, "total_runtime: %llu us\n", stat->data.total_runtime);
	return 0;
}

static int freecess_mod_stat_open(struct inode *inode, struct file *file)
{
	 return single_open(file, freecess_modstat_show, NULL);
}

static const struct file_operations mod_stat_proc_fops = {
	.open     = freecess_mod_stat_open,
	.read     = seq_read,
	.llseek   = seq_lseek,
	.release  = single_release,
};

static void freecess_runinfo_init(struct freecess_info_s *f)
{
	int i;
	struct report_stat_s *stat;

	for(i = 1; i < MOD_END; i++) {
		stat = &f->mod_reportstat[i];
		spin_lock_init(&stat->lock);
		stat->data.report_suc_count = 0;
		stat->data.report_fail_count = 0;
		stat->data.total_runtime = 0;
		stat->data.report_suc_from_windowstart = 0;
		stat->data.report_fail_from_windowstart = 0;
		stat->data.runtime_from_windowstart = 0;
		strlcpy(f->mod_reportstat[i].name, mod_name[i], 32);
	}

}

int register_kfreecess_hook(int mod, freecess_hook hook)
{

	if (!check_mod_type(mod)) {
		pr_err("%s: mod type is invalid! %d\n", __func__, mod);
		return RET_ERR;
	}

	if (hook)
		mod_recv_handler[mod] = hook;
	return RET_OK;
}

int unregister_kfreecess_hook(int mod)
{
	if (!check_mod_type(mod)) {
		pr_err("%s: mod type is invalid! %d\n", __func__, mod);
		return RET_ERR;
	}
	mod_recv_handler[mod] = NULL;
	return RET_OK;
}

static int __init kfreecess_init(void)
{
	int ret = RET_ERR;
	struct proc_dir_entry *freecess_modstat_entry = NULL;
	struct proc_dir_entry *freecess_window_stat_entry = NULL;
	struct proc_dir_entry *pkg_modstat_entry = NULL;
	int i;

	struct netlink_kernel_cfg cfg = {
		.input = recv_handler,
	};
	kfreecess_mod_sock = netlink_kernel_create(&init_net, NETLINK_KFREECESS, &cfg);
	if (!kfreecess_mod_sock) {
		pr_err("%s: create kfreecess_mod_sock socket error!\n", __func__);
		return ret;
	}

	for(i = 1; i<MOD_END; i++)
		atomic_set(&bind_port[i], 0);


	freecess_rootdir = proc_mkdir("freecess", NULL);
	if (!freecess_rootdir)
		pr_err("create /proc/freecess failed\n");
	else {
		freecess_window_stat_entry = proc_create("windowstat", 0644, freecess_rootdir, &window_stat_proc_fops);
		if (!freecess_window_stat_entry) {
			pr_err("create /proc/freecess/windowstat failed, remove /proc/freecess dir\n");
			remove_proc_entry("freecess", NULL);
			freecess_rootdir = NULL;
		} else {
			freecess_modstat_entry = proc_create("modstat", 0, freecess_rootdir, &mod_stat_proc_fops);
			if (!freecess_modstat_entry) {
				pr_err("create /proc/freecess/modstat failed, remove /proc/freecess	dir\n");
				remove_proc_entry("freecess/windowstat", NULL);
				remove_proc_entry("freecess", NULL);
				freecess_rootdir = NULL;
			} else {
				pkg_modstat_entry = proc_create("pkgstat", 0, freecess_rootdir, &pkg_stat_proc_fops);
				if (!pkg_modstat_entry) {
					pr_err("create /proc/freecess/pkgstat failed, remove /proc/freecess	dir\n");
					remove_proc_entry("freecess/windowstat", NULL);
					remove_proc_entry("freecess/modstat", NULL);
					remove_proc_entry("freecess", NULL);
					freecess_rootdir = NULL;
				}
			}
		}
	}

	freecess_runinfo_init(&freecess_info);
	atomic_set(&kfreecess_init_suc, 1);
	return RET_OK;
}

static void __exit kfreecess_exit(void)
{
	if (kfreecess_mod_sock)
		netlink_kernel_release(kfreecess_mod_sock);

	if (freecess_rootdir) {
		remove_proc_entry("windowstat", freecess_rootdir);
		remove_proc_entry("modstat", freecess_rootdir);
		remove_proc_entry("pkgstat", freecess_rootdir);
		remove_proc_entry("freecess", NULL);
	}
}

module_init(kfreecess_init);
module_exit(kfreecess_exit);

MODULE_LICENSE("GPL");
