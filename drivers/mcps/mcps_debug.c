/*
 * Copyright (C) 2019 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <net/ip.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/time.h>
#include <linux/timekeeping.h>

#include <linux/proc_fs.h>

#include "mcps_debug.h"
#include "mcps_device.h"
#include "mcps_sauron.h"

#define MCPS_PROC_DIR "mcps"
#define MCPS_PROC_LOG_FILE "mcps_dump"

#ifdef CONFIG_MCPS_DEBUG_OP_TIME
#define USEC 1000000
#define MINUTES 60

unsigned long tick_us(void)
{
	struct timespec time;
	getnstimeofday(&time);
	return (((unsigned long)time.tv_sec % MINUTES) * USEC + time.tv_nsec / 1000);
}
#endif

#define MCPS_DUMPSIZE 64

struct mcps_flow_dump {
	unsigned long   timestamp_sec;
	unsigned int	timestamp_ms;
	struct in6_addr dst_ipv6;
	unsigned int	dst_ipv4;
	unsigned int	dst_port;
	unsigned int	input_num;
	unsigned int	output_num;
	unsigned int	drop_num;
	unsigned int	ofo_num;
	unsigned int	mig_count;
	unsigned int	l2l_count;
	unsigned int	l2b_count;
	unsigned int	b2l_count;
	unsigned int	b2b_count;
	unsigned long   mig_1st_time_sec;
	unsigned int	mig_1st_time_ms;
	unsigned long   mig_last_time_sec;
	unsigned int	mig_last_time_ms;
};

struct mcps_debug_manager {
	struct mcps_flow_dump dump[MCPS_DUMPSIZE];
	int idx;
	int len;

	unsigned int miss_hit;

	spinlock_t lock;
};

static struct mcps_debug_manager *_manager;
static struct mcps_flow_dump _dump[MCPS_DUMPSIZE];
#define INTEGRITY(d, o, m) (d || o || m)

void mcps_drop_packet(unsigned int hash)
{
	struct eye *flow = NULL;
	struct sauron *sauron = &mcps->sauron;

	rcu_read_lock();
	flow = search_flow(sauron, hash);

	if (!flow) {
		rcu_read_unlock();
		//send error on mcps_debug_manager.
		_manager->miss_hit++;
		return;
	}
	atomic_inc_return_relaxed(&flow->drop_num);

	atomic_inc_return_relaxed(&flow->output_num);
	rcu_read_unlock();
}

void mcps_migrate_flow_history(struct eye *flow, int vec)
{
	int ret = atomic_inc_return_relaxed(&flow->mig_count);
	switch (vec) {
	case 0x5: // l2l 0101
		atomic_inc_return_relaxed(&flow->l2l_count);
		break;
	case 0x9: // l2b 1001
		atomic_inc_return_relaxed(&flow->l2b_count);
		break;
	case 0x6: // b2l 0110
		atomic_inc_return_relaxed(&flow->b2l_count);
		break;
	case 0xA: // b2b 1010
		atomic_inc_return_relaxed(&flow->b2b_count);
		break;
	}
	get_monotonic_boottime(&flow->mig_last_time);
	if (ret == 1) {
		get_monotonic_boottime(&flow->mig_1st_time);
	}
}

atomic_t g_dump_refcnt = ATOMIC_INIT(0);
void do_dump(struct seq_file *s)
{
	int len;
	int idx;
	int i = 0;

	int nref = atomic_inc_return(&g_dump_refcnt);
	if (nref > 1) {
		atomic_dec(&g_dump_refcnt);
		MCPS_INFO("ref cnt : %d -> skipped. \n", nref);
		return;
	}

	//lock
	local_bh_disable();
	spin_lock(&_manager->lock);
	len = _manager->len;
	idx = _manager->idx;
	memcpy(_dump, _manager, sizeof(struct mcps_flow_dump) * MCPS_DUMPSIZE);
	spin_unlock(&_manager->lock);
	local_bh_enable();
	//unlock

	for (i = 0; i < len; i++) {
		int pi = (idx - 1) - i;
		pi = pi < 0 ? len + pi : pi;

		if (!_dump[pi].dst_ipv4) {
			seq_printf(s, "%d] %lu.%u %pI6c:%u %u %u %u %u %u %u %u %u %u %lu.%u %lu.%u\n"
					  , i
					  , _dump[pi].timestamp_sec
					  , _dump[pi].timestamp_ms
					  , &_dump[pi].dst_ipv6
					  , _dump[pi].dst_port
					  , _dump[pi].input_num
					  , _dump[pi].output_num
					  , _dump[pi].drop_num
					  , _dump[pi].ofo_num
					  , _dump[pi].mig_count
					  , _dump[pi].l2l_count
					  , _dump[pi].l2b_count
					  , _dump[pi].b2l_count
					  , _dump[pi].b2b_count
					  , _dump[pi].mig_1st_time_sec
					  , _dump[pi].mig_1st_time_ms
					  , _dump[pi].mig_last_time_sec
					  , _dump[pi].mig_last_time_ms
					   );
		} else {
			seq_printf(s, "%d] %lu.%u %pI4:%u %u %u %u %u %u %u %u %u %u %lu.%u %lu.%u\n"
					  , i
					  , _dump[pi].timestamp_sec
					  , _dump[pi].timestamp_ms
					  , &_dump[pi].dst_ipv4
					  , _dump[pi].dst_port
					  , _dump[pi].input_num
					  , _dump[pi].output_num
					  , _dump[pi].drop_num
					  , _dump[pi].ofo_num
					  , _dump[pi].mig_count
					  , _dump[pi].l2l_count
					  , _dump[pi].l2b_count
					  , _dump[pi].b2l_count
					  , _dump[pi].b2b_count
					  , _dump[pi].mig_1st_time_sec
					  , _dump[pi].mig_1st_time_ms
					  , _dump[pi].mig_last_time_sec
					  , _dump[pi].mig_last_time_ms
					   );
		}
	}

	atomic_dec(&g_dump_refcnt);
}

void push(unsigned long ts_sec, unsigned int ts_ms,
struct in6_addr ipv6, unsigned int ipv4, unsigned int port,
unsigned int input_num, unsigned int output_num,
unsigned int drop_num, unsigned int ofo_num, unsigned int mig_count,
unsigned int l2l_count, unsigned int l2b_count, unsigned int b2l_count, unsigned int b2b_count,
unsigned long mig_1st_time_sec, unsigned int mig_1st_time_ms,
unsigned long mig_last_time_sec, unsigned int mig_last_time_ms)
{
	int idx;
#ifdef CONFIG_MCPS_DEBUG_OP_TIME
	unsigned long t0, t1;
#endif

#ifdef CONFIG_MCPS_DEBUG_OP_TIME
	t0 = tick_us();
#endif

	//lock
	spin_lock(&_manager->lock);
	idx = _manager->idx;
	_manager->dump[idx].timestamp_sec   = ts_sec;
	_manager->dump[idx].timestamp_ms	= ts_ms;
	memcpy(&_manager->dump[idx].dst_ipv6, &ipv6, sizeof(struct in6_addr));
	_manager->dump[idx].dst_ipv4		= ipv4;
	_manager->dump[idx].dst_port		= port;
	_manager->dump[idx].input_num	   = input_num;
	_manager->dump[idx].output_num	  = output_num;
	_manager->dump[idx].drop_num		= drop_num;
	_manager->dump[idx].ofo_num		 = ofo_num;
	_manager->dump[idx].mig_count	   = mig_count;
	_manager->dump[idx].l2l_count	   = l2l_count;
	_manager->dump[idx].l2b_count	   = l2b_count;
	_manager->dump[idx].b2l_count	   = b2l_count;
	_manager->dump[idx].b2b_count	   = b2b_count;
	_manager->dump[idx].mig_1st_time_sec	= mig_1st_time_sec;
	_manager->dump[idx].mig_1st_time_ms	 = mig_1st_time_ms;
	_manager->dump[idx].mig_last_time_sec   = mig_last_time_sec;
	_manager->dump[idx].mig_last_time_ms	= mig_last_time_ms;
	_manager->idx = (idx + 1)%MCPS_DUMPSIZE;
	_manager->len = _manager->len < MCPS_DUMPSIZE ? _manager->len + 1 : _manager->len;
	spin_unlock(&_manager->lock);
	//unlock

#ifdef CONFIG_MCPS_DEBUG_OP_TIME
	t1 = tick_us();
	MCPS_DEBUG("OP TIME : %lu [us]\n", (t1 - t0));
#endif
}

void dump(struct eye *flow)
{
	unsigned int	drop_num		  = atomic_read(&flow->drop_num);
	unsigned int	ofo_num		   = atomic_read(&flow->ofo_num);
	unsigned int	mig_count		 = atomic_read(&flow->mig_count);

	unsigned long   timestamp_sec	 ;
	unsigned int	timestamp_ms	  ;
	struct in6_addr ipv6			  ;
	unsigned int	ipv4			  ;
	unsigned int	port			  ;
	unsigned int	input_num		 ;
	unsigned int	output_num		;
	unsigned int	l2l_count		 ;
	unsigned int	l2b_count		 ;
	unsigned int	b2l_count		 ;
	unsigned int	b2b_count		 ;
	unsigned long   mig_1st_time_sec  ;
	unsigned int	mig_1st_time_ms   ;
	unsigned long   mig_last_time_sec ;
	unsigned int	mig_last_time_ms  ;

	if (!INTEGRITY(drop_num, ofo_num, mig_count)) {
		return;
	}

	timestamp_sec	 = (unsigned long) flow->timestamp.tv_sec;
	timestamp_ms	  = (unsigned int)(flow->timestamp.tv_nsec / 1000000);
	memcpy(&ipv6, &flow->dst_ipv6, sizeof(struct in6_addr));
	ipv4			  = flow->dst_ipv4;
	port			  = flow->dst_port;
	input_num		 = atomic_read(&flow->input_num);
	output_num		= atomic_read(&flow->output_num);
	l2l_count		 = atomic_read(&flow->l2l_count);
	l2b_count		 = atomic_read(&flow->l2b_count);
	b2l_count		 = atomic_read(&flow->b2l_count);
	b2b_count		 = atomic_read(&flow->b2b_count);
	mig_1st_time_sec  = (unsigned long) flow->mig_1st_time.tv_sec;
	mig_1st_time_ms   = (unsigned int)(flow->mig_1st_time.tv_nsec / 1000000);
	mig_last_time_sec = (unsigned long) flow->mig_last_time.tv_sec;
	mig_last_time_ms  = (unsigned int)(flow->mig_last_time.tv_nsec / 1000000);

	push(timestamp_sec, timestamp_ms,
			ipv6, ipv4, port,
			input_num, output_num,
			drop_num, ofo_num, mig_count,
			l2l_count, l2b_count, b2l_count, b2b_count,
			mig_1st_time_sec, mig_1st_time_ms,
			mig_last_time_sec, mig_last_time_ms);
}

#ifdef CONFIG_MCPS_DEBUG_SYSTRACE
void tracing_mark_writev(char sig, int pid, char *func, int value)
{
	char buf[40];
	snprintf(buf, 40, "%c|%d|%s|%d", sig, pid, func, value);
	trace_puts(buf);
}
#endif

static int mcps_single_show(struct seq_file *s, void *unsed)
{
	do_dump(s);
	return 0;
}

static int mcps_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mcps_single_show, NULL);
}

static struct proc_dir_entry *mcps_proc_dir;
static struct proc_dir_entry *mcps_proc_log_file;

static const struct file_operations mcps_proc_ops = {
		.owner		  = THIS_MODULE,
		.open		   = mcps_proc_open,
		.read		   = seq_read,
		.llseek		 = seq_lseek,
		.release		= seq_release,
};

int init_mcps_debug_manager(void)
{
	mcps_proc_dir = proc_mkdir(MCPS_PROC_DIR, NULL);
	if (mcps_proc_dir == NULL) {
		MCPS_DEBUG("Fail to create /proc/%s\n", MCPS_PROC_DIR);
		return -1;
	}

	mcps_proc_log_file = proc_create(MCPS_PROC_LOG_FILE, 0640, mcps_proc_dir, &mcps_proc_ops);
	if (mcps_proc_log_file == NULL) {
		MCPS_DEBUG("Fail to create /proc/%s/%s \n", MCPS_PROC_DIR, MCPS_PROC_LOG_FILE);
		remove_proc_entry(MCPS_PROC_DIR, NULL);
		return -1;
	}

	_manager = (struct mcps_debug_manager *)kzalloc(sizeof(struct mcps_debug_manager), GFP_KERNEL);
	if (!_manager) {
		MCPS_DEBUG("error");
		return -1;
	}
	_manager->lock = __SPIN_LOCK_UNLOCKED(lock);

	MCPS_DEBUG("good");

	return 0;
}

int release_mcps_debug_manager(void)
{
	if (!_manager) {
		return -1;
	}

	kfree(_manager);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
	remove_proc_entry(MCPS_PROC_LOG_FILE, NULL);
	remove_proc_entry(MCPS_PROC_DIR, NULL);
#else
	remove_proc_subtree(MCPS_PROC_DIR, NULL);
#endif

	proc_remove(mcps_proc_log_file);
	proc_remove(mcps_proc_dir);

	MCPS_DEBUG("freed");
	return 0;
}
