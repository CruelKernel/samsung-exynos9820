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

#include <linux/skbuff.h>

#include "mcps_device.h"
#include "mcps_sauron.h"
#include "mcps_buffer.h"
#include "mcps_debug.h"
#include "mcps_fl.h"

#define PRINT_MQTCP_HASH(s, i, h) MCPS_DEBUG("BUF : BUF[%u] FLOW[%u] - %s on CPU[%u]\n", i, h, s, smp_processor_id_safe())
#define PRINT_MQTCP_HASH_UINT(s, i, h, v) MCPS_DEBUG("BUF : BUF[%u] FLOW[%u] - %s [%u] on CPU[%u]\n", i, h, s, v, smp_processor_id_safe())
#define PRINT_PINFO(pi, fmt, ...) MCPS_DEBUG("PINFO[%10u:%2u->%2u|%u] : "fmt, pi->hash, pi->from, pi->to, pi->threshold, ##__VA_ARGS__)

struct pending_info {
	unsigned int hash;
	unsigned int from, to;
	unsigned int threshold;

	struct list_head node;
};

struct pending_info_queue {
	spinlock_t lock;
	struct list_head pinfo_process_queue;
	struct list_head pinfo_input_queue;

	struct napi_struct napi;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	call_single_data_t csd ____cacheline_aligned_in_smp;
#else
	struct call_single_data csd ____cacheline_aligned_in_smp;
#endif
};

DEFINE_PER_CPU_ALIGNED(struct pending_info_queue, pqueues);

void mcps_migration_napi_schedule(void *info)
{
	struct pending_info_queue *q = (struct pending_info_queue *) info;

	__napi_schedule_irqoff(&q->napi);
}

static inline void pinfo_lock(struct pending_info_queue *p)
{
	spin_lock(&p->lock);
}

static inline void pinfo_unlock(struct pending_info_queue *p)
{
	spin_unlock(&p->lock);
}
static int process_migration(struct napi_struct *napi, int quota);

static void init_pinfo_queue(struct net_device *dev, struct pending_info_queue *q)
{
	q->lock = __SPIN_LOCK_UNLOCKED(lock);
	INIT_LIST_HEAD(&q->pinfo_input_queue);
	INIT_LIST_HEAD(&q->pinfo_process_queue);

	q->csd.func = mcps_migration_napi_schedule;
	q->csd.info = q;

	memset(&q->napi, 0, sizeof(struct napi_struct));
	netif_napi_add(dev, &q->napi, process_migration, 1);
	napi_enable(&q->napi);
}

struct pending_info*
create_pending_info(unsigned int hash, unsigned int from, unsigned int to)
{
	struct pending_info *info = (struct pending_info *)kzalloc(sizeof(struct pending_info), GFP_ATOMIC);
	if (!info)
		return NULL;

	info->hash = hash;
	info->from = from;
	info->to = to;

	return info;
}

static inline void push_pending_info(struct pending_info_queue *q, struct pending_info *info)
{
	list_add_tail(&info->node, &q->pinfo_input_queue);
}

static inline void __splice_pending_info(struct list_head *from, struct list_head *to)
{
	if (list_empty(from))
		return;

	list_splice_tail_init(from, to);
}

void update_pending_info(struct list_head *q, unsigned int cpu, unsigned int enqueued)
{
	struct list_head *p;
	struct pending_info *info;

	if (list_empty(q))
		return;

	list_for_each(p, q)
	{
		info = list_entry(p, struct pending_info, node);
		PRINT_PINFO(info, "updated(hotplug) : ->%2u|%u\n", cpu, enqueued);
		info->from = cpu;
		info->threshold = enqueued;
	}
}

void splice_pending_info_2q(unsigned int cpu, struct list_head *queue)
{
	struct pending_info_queue *pinfo_q = &per_cpu(pqueues, cpu);
	__splice_pending_info(&pinfo_q->pinfo_process_queue, queue);
	__splice_pending_info(&pinfo_q->pinfo_input_queue, queue);
}

void splice_pending_info_2cpu(struct list_head *queue, unsigned int cpu)
{
	struct pending_info_queue *pinfo_q = &per_cpu(pqueues, cpu);
	__splice_pending_info(queue, &pinfo_q->pinfo_input_queue);
}

void discard_buffer(struct pending_queue *q)
{
	struct sk_buff *skb = NULL;
	int count = 0;
	while ((skb = __skb_dequeue(&q->rx_pending_queue))) {
		kfree_skb(skb);
		count++;
	}
	if (count > 0)
		MCPS_DEBUG("discard packets : %d\n", count);
}

/* flush_buffer - reset pending buffer
 */
static void flush_buffer(struct pending_info *info)
{
	unsigned int cpu		= 0;
	unsigned int qlen	   = 0;
	unsigned long flag;

	struct pending_queue *buf;

	tracing_mark_writev('B', 1111, "flush_buffer", info->hash);
	if (!mcps_cpu_online(info->to)) {
		unsigned int lcpu = light_cpu();
		PRINT_PINFO(info, "updated(offline) :->%2u\n", lcpu);
		info->to = lcpu;
	}

	cpu = info->to;
	if (cpu >= NR_CPUS) { //invalid
		cpu = 0;
		info->to = cpu;
	}

#if CONFIG_MCPS_GRO_ENABLE
	mcps_current_processor_gro_flush();
#endif

	if (_move_flow(info->hash, info->to)) {
		PRINT_PINFO(info, "WARN : fail to move flow \n");
		tracing_mark_writev('M', 1111, "flush_buffer", EINVAL);
	}

	local_irq_save(flag);
	rcu_read_lock();
	buf = find_pendings((unsigned long)info->hash);
	if (!buf) {
		rcu_read_unlock();
		local_irq_restore(flag);
		PRINT_PINFO(info, "flush completed : no pendings\n");
		tracing_mark_writev('E', 1111, "flush_buffer", EINVAL);
		return;
	}

	PRINT_PINFO(info, "flush starts\n");
	lock_pendings(buf);
	qlen = skb_queue_len(&buf->rx_pending_queue);
	if (qlen > 0) {
		splice_to_gro_pantry(&buf->rx_pending_queue, qlen, info->to);
	}
	buf->state = NO_PENDING;
	unlock_pendings(buf);
	rcu_read_unlock();
	local_irq_restore(flag);
	PRINT_PINFO(info, "flush completed : qlen %u\n", qlen);
	tracing_mark_writev('E', 1111, "flush_buffer", qlen);
}

static int process_migration(struct napi_struct *napi, int quota)
{
	struct pending_info_queue *q = container_of(napi, struct pending_info_queue, napi);
	int left = 0;
	bool again = true;

	unsigned int proc = this_cpu_read(mcps_gro_pantries.processed) + this_cpu_read(mcps_gro_pantries.gro_processed);

	while (again) {
		if (!list_empty(&q->pinfo_process_queue)) {
			struct list_head *p, *n;
			struct pending_info *info;

			list_for_each_safe(p, n, &q->pinfo_process_queue) {
				info = list_entry(p, struct pending_info, node);

				if (info->threshold <= proc) {
					list_del(&info->node);
					flush_buffer(info);
					kfree(info);
				} else {
					left++;
				}
			}
		}

		local_irq_disable();
		pinfo_lock(q);
		if (list_empty(&q->pinfo_input_queue)) {
			if (left == 0) {
				mcps_napi_complete(&q->napi);
			}
			again = false;
		} else {
			list_splice_tail_init(&q->pinfo_input_queue, &q->pinfo_process_queue);
		}
		pinfo_unlock(q);
		local_irq_enable();
	}
	return (!!left);
}

struct hotplug_queue {
	int state;  // must be used with lock ..

	spinlock_t lock;

	struct sk_buff_head queue;
};

struct hotplug_queue *hqueue[NR_CPUS];
struct hotplug_queue *gro_hqueue[NR_CPUS];

static inline void update_state(unsigned long from, int to, u32 gro)
{
	unsigned long flag;

	struct hotplug_queue *q;
	if (gro)
		q = gro_hqueue[from];
	else
		q = hqueue[from];

	local_irq_save(flag);
	spin_lock(&q->lock);
	q->state = to;
	spin_unlock(&q->lock);
	local_irq_restore(flag);
}

static int enqueue_to_hqueue(unsigned int old, struct sk_buff *skb, u32 gro)
{
	int ret = -1;
	unsigned long flag;

	struct hotplug_queue *q;

	if (gro)
		q = gro_hqueue[old];
	else
		q = hqueue[old];

	local_irq_save(flag);
	spin_lock(&q->lock);

	if (q->state == -1) {
		__skb_queue_tail(&q->queue, skb);
	} else {
		ret = q->state;
	}

	spin_unlock(&q->lock);
	local_irq_restore(flag);

	PRINT_MQTCP_HASH_UINT("result ", old, 0, (unsigned int)ret);
	return ret;
}

// current off-line state
int try_to_hqueue(unsigned int hash, unsigned int old, struct sk_buff *skb, u32 layer)
{
	int hdrcpu = enqueue_to_hqueue(old, skb, layer);

	if (hdrcpu < 0) {
		PRINT_MQTCP_HASH("PUSHED INTO HOTQUEUE", old, hash);
		return hdrcpu;
	}
	// cpu off-line callback was expired.
	// all packets on off cpu were already moved into hdrcpu.
	// So we can change core number of the @flow.
	// by this way we can keep order.
	_move_flow(hash, hdrcpu);
	PRINT_MQTCP_HASH_UINT("MOVED TO ", old, hash, (unsigned int)hdrcpu);
	return hdrcpu;

}

int pop_hqueue(unsigned int to, unsigned int from, struct sk_buff_head *queue, u32 gro)
{
	int qlen = 0;
	struct hotplug_queue *q;
	if (gro)
		q = gro_hqueue[from];
	else
		q = hqueue[from];

	spin_lock(&q->lock);

	q->state = to;
	qlen = skb_queue_len(&q->queue);
	if (qlen)
		skb_queue_splice_tail_init(&q->queue, queue);

	spin_unlock(&q->lock);

	if (qlen > 0)
		PRINT_MQTCP_HASH_UINT("SPLICE AND FLUSH FROM ", from, to, (unsigned int)qlen);

	return qlen;
}

// argument cur was deprecated...
// must remove.
void hotplug_on(unsigned int off, unsigned int cur, u32 gro)
{
	update_state(off, -1, gro);
}

bool check_pending(struct pending_queue *q, struct sk_buff *skb)
{
	bool pushed = false;
	unsigned long flag;

	local_irq_save(flag);
	lock_pendings(q);
	if (on_pending(q)) {
		__skb_queue_tail(&q->rx_pending_queue, skb);
		pushed = true;
	}
	unlock_pendings(q);
	local_irq_restore(flag);

	return pushed;
}

/* pending_migration - pend migration of session.
 * Temporarily pending the session migration btw two cores.
 * This is because if a session move it immediately,
 * there is a Out Of Order(ofo) problem.
 * Allow a pending buffer based on the id(hash) in the session.
 *
 * Cancel the session migration if the pending buffer is already in use.
 *
 * @hash - session id
 * @from - ex-core
 * @to   - a core id to be
 */
int pending_migration
(struct pending_queue *buf, unsigned int hash, unsigned int from, unsigned int to)
{
	struct pending_info *info;
	struct pending_info_queue *pinfo_q;

	int smp = 0;

	tracing_mark_writev('B', 1111, "pending_migration", hash);

	pinfo_q = &per_cpu(pqueues, from);

	info = create_pending_info(hash, from, to);
	if (!info) {
		tracing_mark_writev('E', 1111, "pending_migration", ENOMEM);
		return -ENOMEM;
	}
	PRINT_PINFO(info, "pending setup\n");

	local_irq_disable();
	lock_pendings(buf);
	if (!pending_available(buf)) {
		unlock_pendings(buf);
		local_irq_enable();
		tracing_mark_writev('E', 1111, "pending_migration", 999);
		PRINT_PINFO(info, "pending fail : queue on pending\n");
		kfree(info);
		return -999;
	}

	info->threshold = pantry_read_enqueued(&per_cpu(mcps_gro_pantries, from));
	buf->state = ON_PENDING;
	unlock_pendings(buf);

	pinfo_lock(pinfo_q);
	push_pending_info(pinfo_q, info);

	// check schedule
	if (!__test_and_set_bit(NAPI_STATE_SCHED, &pinfo_q->napi.state)) {
		smp = 1;
	}
	pinfo_unlock(pinfo_q);
	local_irq_enable();

	// schedule
	if (smp && mcps_cpu_online(from)) {
		smp_call_function_single_async(from, &pinfo_q->csd);
	}
	tracing_mark_writev('E', 1111, "pending_migration", hash);
	return 0;
}

void init_mcps_buffer(struct net_device *dev)
{
	int i;

	for_each_possible_cpu(i) {
		if (VALID_CPU(i)) {
			init_pinfo_queue(dev, &per_cpu(pqueues, i));
		}
	}

	for (i = 0 ; i < NR_CPUS; i++) {
		hqueue[i] = (struct hotplug_queue *)kzalloc(sizeof(struct hotplug_queue), GFP_KERNEL);
		hqueue[i]->state = -1;
		skb_queue_head_init(&hqueue[i]->queue);

		hqueue[i]->lock = __SPIN_LOCK_UNLOCKED(lock);
	}

	for (i = 0 ; i < NR_CPUS; i++) {
		gro_hqueue[i] = (struct hotplug_queue *)kzalloc(sizeof(struct hotplug_queue), GFP_KERNEL);
		gro_hqueue[i]->state = -1;
		skb_queue_head_init(&gro_hqueue[i]->queue);

		gro_hqueue[i]->lock = __SPIN_LOCK_UNLOCKED(lock);
	}
}

void release_mcps_buffer(void)
{
	int i;
	for (i = 0; i < NR_CPUS; i++) {
		kfree(hqueue[i]);
		kfree(gro_hqueue[i]);
	}
}
