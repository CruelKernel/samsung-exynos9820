#include <linux/skbuff.h>

#include "mcps_device.h"
#include "mcps_sauron.h"
#include "mcps_buffer.h"
#include "mcps_debug.h"
#include "mcps_fl.h"

/* CONFIG_MCPS_GROBUF - Configuration of GRO Buffering
 * It is useful for buffering efficiently.
 * But We should change some code in dev_gro_receive, before gro buffering is on.
 * Because tcp_gro_receive is going to flush buffer directly , when it has too much of packets.
 * So we have to catch it, before it flush gro packet to netif_receive_skb.
 * */
#define CONFIG_MCPS_GROBUF 0

#define PRINT_QUEUE(q) {MCPS_DEBUG("BUF : Q[%u] FROM[%u] TO[%u] HASH[%u] on CPU[%u]\n" , q->idx , q->from , q->to , q->hash, smp_processor_id_safe());};
#define PRINT_MQTCP(s, i) {MCPS_DEBUG("BUF : BUF[%u] - %s on CPU[%u]\n" , i , s , smp_processor_id_safe());};
#define PRINT_MQTCP_HASH(s, i, h) {MCPS_DEBUG("BUF : BUF[%u] FLOW[%u] - %s on CPU[%u]\n" , i , h , s , smp_processor_id_safe());};
#define PRINT_MQTCP_HASH_UINT(s, i, h ,v) {MCPS_DEBUG("BUF : BUF[%u] FLOW[%u] - %s [%u] on CPU[%u]\n" , i , h , s , v, smp_processor_id_safe());};
#define PRINT_PINFO(s, pi) {MCPS_DEBUG("BUF(%u) : %s PINFO[%u : %u -> %u , %u]\n", smp_processor_id_safe(), s , pi->hash ,pi->from ,pi->to ,pi->threshold);};

void mcps_migration_napi_schedule(void * info)
{
    struct mcps_pantry * pantry = (struct mcps_pantry *) info;
    __napi_schedule_irqoff(&pantry->rx_napi_struct);
    pantry->received_arps++;
}

struct pending_info {
    u32 gro;
    unsigned int hash;
    unsigned int from, to;
    unsigned int threshold;

    unsigned int pqueue_idx;

    struct list_head node;
};

struct pending_info_queue {
    struct list_head pinfo_process_queue;
    struct list_head pinfo_input_queue;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    call_single_data_t csd ____cacheline_aligned_in_smp;
#else
    struct call_single_data csd ____cacheline_aligned_in_smp;
#endif
};

DEFINE_PER_CPU_ALIGNED(struct pending_info_queue , pqueues);
DEFINE_PER_CPU_ALIGNED(struct pending_info_queue , pqueues_gro);

static void init_pinfo_queue(struct mcps_pantry *pantry, struct pending_info_queue * q)
{
    INIT_LIST_HEAD(&q->pinfo_input_queue);
    INIT_LIST_HEAD(&q->pinfo_process_queue);
    q->csd.func = mcps_migration_napi_schedule;
    q->csd.info = pantry;
}

struct pending_info*
create_pending_info
(unsigned int hash , unsigned int from, unsigned int to , unsigned int pqidx, u32 gro) {
    struct pending_info * info = (struct pending_info *)kzalloc(sizeof(struct pending_info), GFP_ATOMIC);
    if(!info)
        return NULL;

    info->gro = gro;
    info->hash = hash;
    info->from = from;
    info->to = to;
    info->pqueue_idx = pqidx;

    return info;
}

static inline void init_threshold_info(struct pending_info * info , unsigned int thr)
{
    info->threshold = thr;
}

static inline void push_pending_info (struct pending_info_queue *q , struct pending_info * info)
{
    list_add_tail(&info->node , &q->pinfo_input_queue);
}

static inline void __splice_pending_info (struct list_head * from , struct list_head * to)
{
    if (list_empty(from))
        return;

    list_splice_tail_init(from, to);
}

void update_pending_info(struct list_head * q, unsigned int cpu, unsigned int enqueued)
{
    struct list_head *p;
    struct pending_info *info;

    if (list_empty(q))
        return;

    list_for_each(p, q)
    {
        info = list_entry(p, struct pending_info, node);
        info->from = cpu;
        info->threshold = enqueued;
        PRINT_PINFO("UPDATED PINFO BY HOTPLUG ", info);
    }
}

void splice_pending_info_di (unsigned int cpu , u32 gro)
{
    struct pending_info_queue * pinfo_q;
    if(gro) {
        pinfo_q = &per_cpu(pqueues_gro, cpu);
    }else {
        pinfo_q = &per_cpu(pqueues, cpu);
    }

    __splice_pending_info(&pinfo_q->pinfo_input_queue , &pinfo_q->pinfo_process_queue);
}

void splice_pending_info_2q (unsigned int cpu ,struct list_head * queue , u32 gro)
{
    struct pending_info_queue * pinfo_q;
    if(gro) {
        pinfo_q = &per_cpu(pqueues_gro, cpu);
    }else {
        pinfo_q = &per_cpu(pqueues, cpu);
    }

    __splice_pending_info(&pinfo_q->pinfo_process_queue , queue);
    __splice_pending_info(&pinfo_q->pinfo_input_queue , queue);
}

void splice_pending_info_2cpu (struct list_head * queue , unsigned int cpu , u32 gro)
{
    struct pending_info_queue * pinfo_q;
    if(gro) {
        pinfo_q = &per_cpu(pqueues_gro, cpu);
    }else {
        pinfo_q = &per_cpu(pqueues, cpu);
    }

    __splice_pending_info(queue, &pinfo_q->pinfo_input_queue);
}

struct pending_queue {
    unsigned int hash;
    int state;
    atomic_t toggle;
    struct sk_buff_head rx_pending_queue;
};

#define CNT_BUCKETS 10

struct pending_queue pending_packet_bufs[CNT_BUCKETS];
struct pending_queue pending_packet_bufs_gro[CNT_BUCKETS];
#define BIDX(h) (h%CNT_BUCKETS)

static void init_pending_buf(struct pending_queue bufs[CNT_BUCKETS])
{
    int i;
    for(i = 0 ; i < CNT_BUCKETS; i++) {
        memset(&bufs[i], 0, sizeof(struct pending_queue));
        bufs[i].state = 0;
        atomic_set(&bufs[i].toggle, 0);
        skb_queue_head_init(&bufs[i].rx_pending_queue);
    }
}

static inline void lock_pending_buf(struct pending_queue * buf) {
    spin_lock(&buf->rx_pending_queue.lock);
}

static inline void unlock_pending_buf(struct pending_queue * buf) {
    spin_unlock(&buf->rx_pending_queue.lock);
}

/* flush_buffer - reset pending buffer
 * */
static void flush_buffer(struct pending_info * info)
{
    bool smp = false;
    const unsigned int cur  = info->from;
    unsigned int cpu        = 0;
    unsigned int qlen       = 0;
    unsigned long flag;

    struct mcps_pantry * pantry;
    struct pending_queue * buf;
    struct pending_info_queue * pinfo_q;

    tracing_mark_writev('B',1111,"flush_buffer", info->hash);
    if (!mcps_cpu_online(info->to)) {
        unsigned int lcpu = light_cpu(info->gro);
        info->to = lcpu;
        PRINT_PINFO("INFO->TO CORE OFFLINE SO UPDATED " , info);
    }

    cpu = info->to;
    if(cpu >= NR_CPUS) { //invalid
        cpu = 0;
        info->to = cpu;
    }

    if(info->gro) {
        pantry = &per_cpu(mcps_gro_pantries, cur);
        napi_gro_flush(&pantry->rx_napi_struct, false);
        getnstimeofday(&pantry->flush_time);

        pantry = &per_cpu(mcps_gro_pantries, cpu);
        pinfo_q = &per_cpu(pqueues_gro, cpu);
        buf = &pending_packet_bufs_gro[info->pqueue_idx];
    }else {
        pantry = &per_cpu(mcps_pantries, cpu);
        pinfo_q = &per_cpu(pqueues, cpu);
        buf = &pending_packet_bufs[info->pqueue_idx];
    }

    rcu_read_lock();
    _move_flow(info->hash, cpu);
    rcu_read_unlock();

    PRINT_PINFO("FLUSH START " , info);
    local_irq_save(flag);
    pantry_lock(pantry);
    lock_pending_buf(buf);

    qlen = skb_queue_len(&buf->rx_pending_queue);
    if(qlen) {
        skb_queue_splice_tail_init(&buf->rx_pending_queue,
                                                   &pantry->input_pkt_queue);
        pantry->enqueued += qlen;

        if (!__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state)) {
            smp = true;
        }
    }

    buf->state--;
    atomic_dec(&buf->toggle);

    unlock_pending_buf(buf);
    pantry_unlock(pantry);
    local_irq_restore(flag);

    tracing_mark_writev('E',1111,"flush_buffer", 0);

    if(smp) {
        smp_call_function_single_async(cpu , &pinfo_q->csd);
        PRINT_MQTCP_HASH_UINT("SMPCALLED FLUSHED CNT : " , info->pqueue_idx , info->hash, qlen);
    } else {
        PRINT_MQTCP_HASH_UINT("FLUSHED CNT : " , info->pqueue_idx , info->hash, qlen);
    }
}


void check_pending_info(unsigned int cpu, const unsigned int processed , u32 gro)
{
    struct list_head *p, *n;
    struct pending_info *info;
    struct pending_info_queue *pinfo_q;

    if(gro) {
        pinfo_q = &per_cpu(pqueues_gro, cpu);
    }else {
        pinfo_q = &per_cpu(pqueues, cpu);
    }

    if(list_empty(&pinfo_q->pinfo_process_queue))
        return;

    list_for_each_safe(p,n,&pinfo_q->pinfo_process_queue) {
        info = list_entry(p, struct pending_info, node);

        if(info->threshold > processed)
            return;

        list_del(&info->node);
        flush_buffer(info);
        PRINT_PINFO("TRIGGER DONE ", info);
        kfree(info);
    }
}

struct hotplug_queue {
    int state;  // must be used with lock ..

    spinlock_t lock;

    struct sk_buff_head queue;
};

struct hotplug_queue * hqueue[NR_CPUS];
struct hotplug_queue * gro_hqueue[NR_CPUS];

static inline void update_state(unsigned long from , int to , u32 gro)
{
    unsigned long flag;

    struct hotplug_queue * q;
    if(gro) q = gro_hqueue[from];
    else q = hqueue[from];

    local_irq_save(flag);
    spin_lock(&q->lock);
    q->state = to;
    spin_unlock(&q->lock);
    local_irq_restore(flag);
}

static int enqueue_to_hqueue(unsigned int old , struct sk_buff *skb, u32 gro)
{
    int ret = -1;
    unsigned long flag;

    struct hotplug_queue * q;

    if (gro)
        q = gro_hqueue[old];
    else
        q = hqueue[old];

    local_irq_save(flag);
    spin_lock(&q->lock);

    if(q->state == -1) {
        __skb_queue_tail(&q->queue, skb);
    } else {
        ret = q->state;
    }

    spin_unlock(&q->lock);
    local_irq_restore(flag);

    PRINT_MQTCP_HASH_UINT("result " , old , 0, (unsigned int)ret);
    return ret;
}

// current off-line state
int try_to_hqueue(unsigned int hash, unsigned int old , struct sk_buff * skb, u32 gro)
{
    int hdrcpu = enqueue_to_hqueue(old , skb, gro);

    if(hdrcpu < 0) {
        PRINT_MQTCP_HASH("PUSHED INTO HOTQUEUE" , old , hash);
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

int pop_hqueue(unsigned int to, unsigned int from , struct sk_buff_head* queue, u32 gro)
{
    int qlen = 0;
    struct hotplug_queue * q;
    if (gro)
        q = gro_hqueue[from];
    else
        q = hqueue[from];

    spin_lock(&q->lock);

    q->state = to;
    qlen = skb_queue_len(&q->queue);
    if(qlen)
        skb_queue_splice_tail_init(&q->queue, queue);

    spin_unlock(&q->lock);

    if(qlen > 0)
        PRINT_MQTCP_HASH_UINT("SPLICE AND FLUSH FROM " , from , to , (unsigned int)qlen);

    return qlen;
}

// argument cur was deprecated...
// must remove.
void hotplug_on(unsigned int off, unsigned int cur, u32 gro) {
    update_state(off , -1 , gro);
}

bool ON_PENDING(unsigned int hash , struct sk_buff * skb, u32 gro)
{
    bool pushed = false;
    unsigned int i = BIDX(hash);
    unsigned long flag;
    int val = 0;

    struct pending_queue * buf;
    if(gro) {
        buf = &pending_packet_bufs_gro[i];
    } else {
        buf = &pending_packet_bufs[i];
    }

    val = atomic_inc_return(&buf->toggle);
    if(val == 2) {
        local_irq_save(flag);
        lock_pending_buf(buf);
        if(buf->hash == hash && buf->state) { // > 0
            __skb_queue_tail(&buf->rx_pending_queue , skb);
            pushed = true;
        }
        unlock_pending_buf(buf);
        local_irq_restore(flag);
    }

    val = atomic_dec_return(&buf->toggle);
    if(val < 0 || val > 1)
        panic("TOGGLE PANIC %d \n" , val);

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
 * */
int pending_migration
(unsigned int hash, unsigned int from, unsigned int to, const u32 gro) {
    unsigned int idx = BIDX(hash);
    struct pending_info *info;
    struct mcps_pantry *pantry;
    struct pending_queue * buf;
    struct pending_info_queue *pinfo_q;

    int state;
    int smp = 0;

    tracing_mark_writev('B',1111,"pending_migration", hash);

    if (gro) {
        pantry = &per_cpu(mcps_gro_pantries, from);
        pinfo_q = &per_cpu(pqueues_gro, from);
        buf = &pending_packet_bufs_gro[idx];
    } else {
        pantry = &per_cpu(mcps_pantries, from);
        pinfo_q = &per_cpu(pqueues, from);
        buf = &pending_packet_bufs[idx];
    }

    local_irq_disable();
    lock_pending_buf(buf);
    state = buf->state;
    unlock_pending_buf(buf);
    local_irq_enable();

    if(state != 0) {
        tracing_mark_writev('E',1111,"pending_migration", 999);
        return -999;
    }

    info = create_pending_info(hash, from , to, idx, gro);
    if(!info) {
        tracing_mark_writev('E',1111,"pending_migration", ENOMEM);
        return -ENOMEM;
    }
    PRINT_PINFO("PENDING MIGRATION START" ,info);

    local_irq_disable();
    pantry_lock(pantry);
    lock_pending_buf(buf);
    buf->hash = hash;
    atomic_inc(&buf->toggle); // can be leak
    buf->state++;

    init_threshold_info(info, pantry->enqueued); // gro check
    push_pending_info(pinfo_q , info);

    if (!__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state)) {
        smp = 1;
    }

    unlock_pending_buf(buf);
    pantry_unlock(pantry);
    local_irq_enable();

    if(smp && mcps_cpu_online(from)) {
        smp_call_function_single_async(pantry->cpu , &pantry->csd);
    }

    PRINT_PINFO("PENDING MIGRATION END" ,info);
    tracing_mark_writev('E',1111,"pending_migration", hash);
    return 0;
}

void init_mcps_buffer(void)
{
    int i;

    init_pending_buf(pending_packet_bufs);
    init_pending_buf(pending_packet_bufs_gro);

    for_each_possible_cpu(i)
    {
        if(VALID_CPU(i)) {
            struct mcps_pantry *pantry;
            pantry = &per_cpu(mcps_pantries, i);
            init_pinfo_queue(pantry , &per_cpu(pqueues, i));

            pantry = &per_cpu(mcps_gro_pantries, i);
            init_pinfo_queue(pantry , &per_cpu(pqueues_gro, i));
        }
    }

    for(i = 0 ; i < NR_CPUS; i++) {
        hqueue[i] = (struct hotplug_queue *)kzalloc(sizeof(struct hotplug_queue), GFP_KERNEL);
        hqueue[i]->state = -1;
        skb_queue_head_init(&hqueue[i]->queue);

        hqueue[i]->lock = __SPIN_LOCK_UNLOCKED(lock);
    }

    for(i = 0 ; i < NR_CPUS; i++) {
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
