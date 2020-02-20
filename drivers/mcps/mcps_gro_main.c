//#ifndef CONFIG_MODEM_IF_NET_GRO
//#define CONFIG_MODEM_IF_NET_GRO

#include <linux/kernel.h>

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cpu.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
////++ beyond
//#ifdef CONFIG_LINK_FORWARD
//#include <linux/linkforward.h>
//#endif
////-- beyond

#include "mcps_sauron.h"
#include "mcps_device.h"
#include "mcps_buffer.h"
#include "mcps_debug.h"
#include "mcps_fl.h"

DEFINE_PER_CPU_ALIGNED(struct mcps_pantry , mcps_gro_pantries);
EXPORT_PER_CPU_SYMBOL(mcps_gro_pantries);

int mcps_gro_pantry_max_capability __read_mostly = 30000;
module_param(mcps_gro_pantry_max_capability , int , 0640);
int mcps_gro_pantry_quota __read_mostly = 128;
module_param(mcps_gro_pantry_quota , int , 0640);

static long mcps_gro_flush_time = 5000L;
module_param(mcps_gro_flush_time, long, 0640);

static void mcps_gro_flush_timer(struct mcps_pantry *pantry)
{
    struct timespec curr, diff;

    if (!mcps_gro_flush_time) {
        tracing_mark_writev('B',1111,"gro_flush_bytimer", 0);
        napi_gro_flush(&pantry->rx_napi_struct, false);
        tracing_mark_writev('E',1111,"gro_flush_bytimer", 0);
        return;
    }

    if (unlikely(pantry->flush_time.tv_sec == 0)) {
        getnstimeofday(&pantry->flush_time);
    } else {
        getnstimeofday(&(curr));
        diff = timespec_sub(curr, pantry->flush_time);
        if ((diff.tv_sec > 0) || (diff.tv_nsec > mcps_gro_flush_time)) {
            tracing_mark_writev('B',1111,"gro_flush_bytimer", 0);
            napi_gro_flush(&pantry->rx_napi_struct, false);
            tracing_mark_writev('E',1111,"gro_flush_bytimer", 0);
            getnstimeofday(&pantry->flush_time);
        }
    }
}

static inline int mcps_check_skb_can_gro(struct sk_buff* skb)
{
//++ bx
    switch (skb->data[0] & 0xF0) {
    case 0x40:
        return (ip_hdr(skb)->protocol == IPPROTO_TCP);

    case 0x60:
        return (ipv6_hdr(skb)->nexthdr == IPPROTO_TCP);
    }
    return 0;
//-- bx
}

static int process_gro_pantry(struct napi_struct *napi , int quota)
{
    struct mcps_pantry *pantry = container_of(napi , struct mcps_pantry , rx_napi_struct);
    int work = 0;
    bool again = true;
    gro_result_t ret;

    tracing_mark_writev('B',1111,"process_gro_pantry", 0);
    if(mcps_on_ipi_waiting(pantry)) {
        local_irq_disable();
        mcps_do_ipi_and_irq_enable(pantry);
    }

    getnstimeofday(&pantry->flush_time);

    while (again) {
        struct sk_buff *skb;

        while ((skb = __skb_dequeue(&pantry->process_queue))) {
            mcps_out_packet(skb);

            if (mcps_check_skb_can_gro(skb)/*&& (flag & FLAG_GRO)*/) {
                __this_cpu_inc(mcps_gro_pantries.gro_processed);
                ret = napi_gro_receive(napi, skb);

                if(ret == GRO_NORMAL) {
                    getnstimeofday(&pantry->flush_time);
                }
            } else {
                __this_cpu_inc(mcps_gro_pantries.processed);
                netif_receive_skb(skb); //This function may only be called from softirq context and interrupts should be enabled. Ref.function description
            }

            mcps_gro_flush_timer(pantry);

            if (++work >= quota) {
                PRINT_GRO_WORKED(pantry->cpu , work , quota);
                goto end;
            }
        }

        local_irq_disable();
        pantry_lock(pantry);
        if (skb_queue_empty(&pantry->input_pkt_queue)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
            mcps_napi_complete(&pantry->rx_napi_struct);
#else
            __napi_complete(&pantry->rx_napi_struct);
#endif
            PRINT_GRO_WORKED(pantry->cpu , work , quota);
            again = false;
        } else {
            skb_queue_splice_tail_init(&pantry->input_pkt_queue,
                                       &pantry->process_queue);
        }
        splice_pending_info_di(pantry->cpu, 1);

        pantry_unlock(pantry);
        local_irq_enable();

        check_pending_info(pantry->cpu , (pantry->processed + pantry->gro_processed), 1);
    }

    tracing_mark_writev('B',1111,"napi_gro_flush", 0);
    napi_gro_flush(&pantry->rx_napi_struct, false);
    tracing_mark_writev('E',1111,"napi_gro_flush", 0);

    check_pending_info(pantry->cpu , (pantry->processed + pantry->gro_processed), 1);

end:
    tracing_mark_writev('E',1111,"process_gro_pantry", work);
    return work;
}

int mcps_gro_ipi_queued(struct mcps_pantry *pantry)
{
    struct mcps_pantry *this_pantry = this_cpu_ptr(&mcps_gro_pantries);

    if(pantry != this_pantry) {
        pantry->ipi_next = this_pantry->ipi_list;
        this_pantry->ipi_list = pantry;

        pantry_lock(this_pantry);
        if (!__test_and_set_bit(NAPI_STATE_SCHED, &this_pantry->rx_napi_struct.state))
            __napi_schedule_irqoff(&this_pantry->rx_napi_struct);
        pantry_unlock(this_pantry);

        PRINT_GRO_IPIQ(this_pantry->cpu , pantry->cpu);
        return 1;
    }
    return 0;
}

static int enqueue_to_gro_pantry(struct sk_buff *skb, int cpu)
{
    struct mcps_pantry *pantry;
    unsigned long flags;
    unsigned int qlen;

    tracing_mark_writev('B',1111,"enqueue_to_gro_pantry", cpu);
    pantry = &per_cpu(mcps_gro_pantries, cpu);

    local_irq_save(flags); //save register information into flags and disable irq (local_irq_disabled)

    pantry_lock(pantry);

    // hp off.
    if(pantry->cpu == NR_CPUS) {
        int hdr_cpu = 0;
        pantry_unlock(pantry);
        local_irq_restore(flags);

        hdr_cpu = try_to_hqueue(skb->hash, cpu, skb, MCPS_GRO_DEVICE);
        cpu = hdr_cpu;
        if(hdr_cpu < 0) {
            tracing_mark_writev('E',1111,"enqueue_to_gro_pantry", 88);
            return NET_RX_SUCCESS;
        } else {
            pantry = &per_cpu(mcps_gro_pantries, hdr_cpu);

            local_irq_save(flags);
            pantry_lock(pantry);
        }
    }

    qlen = skb_queue_len(&pantry->input_pkt_queue);
    if (qlen <= mcps_gro_pantry_max_capability) {
        if (qlen) {
enqueue:
            __skb_queue_tail(&pantry->input_pkt_queue, skb);

            pantry->enqueued++;

            pantry_unlock(pantry);
            local_irq_restore(flags);
            tracing_mark_writev('E',1111,"enqueue_to_gro_pantry", cpu);
            return NET_RX_SUCCESS;
        }

        /* Schedule NAPI for backlog device
         * We can use non atomic operation since we own the queue lock
         */
        if (!__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state)) {
            if (!mcps_gro_ipi_queued(pantry))
                __napi_schedule_irqoff(&pantry->rx_napi_struct);
        }
        goto enqueue;
    }

    MCPS_DEBUG("[%d] dropped \n" ,cpu);

    pantry->dropped++;
    pantry_unlock(pantry);

    local_irq_restore(flags);
    tracing_mark_writev('E',1111,"enqueue_to_gro_pantry", 99);
//    kfree_skb(skb);
    return NET_RX_DROP;
}

static int
mcps_try_gro_internal (struct sk_buff *skb)
{
    int cpu = -1;
    int ret = -1;
    u32 hash;

    flush_flows(0);

    rcu_read_lock();

    skb_reset_network_header(skb);
    hash = skb_get_hash(skb);

    if(!hash) {
        rcu_read_unlock();
        goto error;
    }

    cpu = get_arps_cpu(mcps , skb, hash , 1);

    if (cpu < 0 || cpu > NR_CPUS) {
        rcu_read_unlock();
        goto error;
    }

    if (cpu == NR_CPUS) {
        rcu_read_unlock();
        return NET_RX_SUCCESS;
    }
    ret = enqueue_to_gro_pantry(skb, cpu);
    rcu_read_unlock();

    if (ret == NET_RX_DROP) {
        mcps_drop_packet(hash);
        goto error;
    }

    return ret;
error:
    PRINT_TRY_FAIL(hash , cpu);
    return NET_RX_DROP;
}

/* mcps_try_gro - Main entry point of mcps gro
 * return NET_RX_DROP(1) if mcps fail or disabled , otherwise NET_RX_SUCCESS(0)
 * */
int mcps_try_gro(struct sk_buff *skb)
{
    if(!mcps_enable)
        return NET_RX_DROP;

    return mcps_try_gro_internal(skb);
}
EXPORT_SYMBOL(mcps_try_gro);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int mcps_gro_cpu_startup_callback(unsigned int ocpu)
{
    struct mcps_pantry *pantry = &per_cpu(mcps_gro_pantries, ocpu);
    tracing_mark_writev('B',1111,"mcps_gro_online", ocpu);

    local_irq_disable();
    pantry_lock(pantry);
    pantry->cpu = ocpu;
    pantry_unlock(pantry);
    local_irq_enable();

    hotplug_on(ocpu, 0 , MCPS_GRO_DEVICE);

    tracing_mark_writev('E',1111,"mcps_gro_online", ocpu);
    return 0;
}

int mcps_gro_cpu_teardown_callback(unsigned int ocpu)
{
    unsigned int cpu, oldcpu = ocpu;
    struct mcps_pantry *pantry , *oldpantry;
    unsigned int qlen = 0;

    struct sk_buff_head queue;
    bool needsched = false;
    struct list_head pinfo_queue;

    tracing_mark_writev('B',1111,"mcps_gro_offline", ocpu);
    skb_queue_head_init(&queue);
    INIT_LIST_HEAD(&pinfo_queue);

    cpu = light_cpu(1);
    if(cpu == ocpu) {
        cpu = 0;
    }

    pantry = &per_cpu(mcps_gro_pantries, cpu);
    oldpantry = &per_cpu(mcps_gro_pantries, oldcpu);

    MCPS_DEBUG("[%u] : CPU[%u -> %u] - HOTPLUGGED OUT\n"
            , smp_processor_id_safe(), oldcpu, cpu);

    napi_gro_flush(&oldpantry->rx_napi_struct, false);
    getnstimeofday(&oldpantry->flush_time);

    //detach first.
    local_irq_disable();
    pantry_lock(oldpantry);
    if (test_bit(NAPI_STATE_SCHED, &oldpantry->rx_napi_struct.state))
        mcps_napi_complete(&oldpantry->rx_napi_struct);

    if (!skb_queue_empty(&oldpantry->process_queue)) {
        qlen += skb_queue_len(&oldpantry->process_queue);
        skb_queue_splice_tail_init(&oldpantry->process_queue, &queue);
    }

    if (!skb_queue_empty(&oldpantry->input_pkt_queue)) {
        qlen += skb_queue_len(&oldpantry->input_pkt_queue);
        skb_queue_splice_tail_init(&oldpantry->input_pkt_queue, &queue);
    }

    oldpantry->ignored += qlen;
    oldpantry->processed += qlen;

    splice_pending_info_2q(oldcpu , &pinfo_queue , 1);

    pantry_unlock(oldpantry);
    local_irq_enable();

    // attach second.
    local_irq_disable();
    pantry_lock(pantry);

    if (!skb_queue_empty(&queue)) {
        skb_queue_splice_tail_init(&queue, &pantry->input_pkt_queue);
        pantry->enqueued += qlen;
        needsched = true;
    }

    qlen = pop_hqueue(cpu, oldcpu, &pantry->input_pkt_queue, 1);
    if(qlen) {
        pantry->enqueued += qlen;
        needsched = true;
    }

    if(!list_empty(&pinfo_queue)) {
        needsched = true;
        update_pending_info(&pinfo_queue, cpu, pantry->enqueued);
        splice_pending_info_2cpu(&pinfo_queue , cpu , 1);
    }

    needsched = (needsched && !__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state));

    pantry_unlock(pantry);
    local_irq_enable();

    if(needsched && mcps_cpu_online(cpu)) {
        smp_call_function_single_async(pantry->cpu , &pantry->csd);
    }

    tracing_mark_writev('E',1111,"mcps_gro_offline", cpu);
    return 0;
}
#else
int mcps_gro_cpu_callback(struct notifier_block *notifier , unsigned long action, void* ocpu)
{
    unsigned int cpu, oldcpu = (unsigned long)ocpu;
    struct mcps_pantry *pantry , *oldpantry;
    unsigned int qlen = 0;

    struct sk_buff_head queue;
    bool needsched = false;
    struct list_head pinfo_queue;

    skb_queue_head_init(&queue);
    INIT_LIST_HEAD(&pinfo_queue);

    if (action == CPU_ONLINE)
        goto online;

    if (action != CPU_DEAD && action != CPU_DEAD_FROZEN)
        return NOTIFY_OK;

    cpu = light_cpu(1);
    cpu = cpu < NR_CPUS ? cpu : 0;
    pantry = &per_cpu(mcps_gro_pantries, cpu);
    oldpantry = &per_cpu(mcps_gro_pantries, oldcpu);

    MCPS_DEBUG("[%u] : CPU[%u -> %u] - HOTPLUGGED OUT\n"
            , smp_processor_id_safe(), oldcpu, cpu);

    //detach first.
    local_irq_disable();
    pantry_lock(oldpantry);
    if (test_bit(NAPI_STATE_SCHED, &oldpantry->rx_napi_struct.state))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
        mcps_napi_complete(&oldpantry->rx_napi_struct);
#else
        __napi_complete(&oldpantry->rx_napi_struct);
#endif
    if (!skb_queue_empty(&oldpantry->process_queue)) {
        qlen += skb_queue_len(&oldpantry->process_queue);
        skb_queue_splice_tail_init(&oldpantry->process_queue, &queue);
    }

    if (!skb_queue_empty(&oldpantry->input_pkt_queue)) {
        qlen += skb_queue_len(&oldpantry->input_pkt_queue);
        skb_queue_splice_tail_init(&oldpantry->input_pkt_queue, &queue);
    }

    oldpantry->ignored += qlen;
    oldpantry->processed += qlen;

    splice_pending_info_2q(oldcpu , &pinfo_queue , 1);

    pantry_unlock(oldpantry);
    local_irq_enable();

    napi_gro_flush(&oldpantry->rx_napi_struct, false);
    getnstimeofday(&oldpantry->flush_time);

    // attach second.
    local_irq_disable();
    pantry_lock(pantry);

    if (!skb_queue_empty(&queue)) {
        skb_queue_splice_tail_init(&queue, &pantry->input_pkt_queue);
        pantry->enqueued += qlen;
        needsched = true;
    }

    qlen = pop_hqueue(cpu, oldcpu, &pantry->input_pkt_queue, 1);
    if(qlen) {
        pantry->enqueued += qlen;
        needsched = true;
    }

    if(!list_empty(&pinfo_queue)) {
        needsched = true;
        update_pending_info(&pinfo_queue, cpu, pantry->enqueued);
        splice_pending_info_2cpu(&pinfo_queue , cpu , 1);
    }

    if (needsched && !__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state)) {
        if (!mcps_gro_ipi_queued(pantry)) // changed
            __napi_schedule_irqoff(&pantry->rx_napi_struct);
    }

    pantry_unlock(pantry);
    local_irq_enable();

    return NOTIFY_OK;
online:
    hotplug_on(oldcpu, cpu , 1);

    return NOTIFY_OK;
}
#endif

void mcps_gro_init(struct net_device * mcps_device)
{
    int i = 0;

    for_each_possible_cpu(i) {
        if(VALID_CPU(i)) {
            struct mcps_pantry *pantry = &per_cpu(mcps_gro_pantries , i);
            skb_queue_head_init(&pantry->input_pkt_queue);
            skb_queue_head_init(&pantry->process_queue);

            pantry->received_arps = 0;
            pantry->processed = 0;
            pantry->gro_processed = 0;
            pantry->enqueued = 0;
            pantry->dropped = 0;
            pantry->ignored = 0;

            pantry->csd.func = mcps_napi_schedule;
            pantry->csd.info = pantry;
            pantry->cpu = i;

            memset(&pantry->rx_napi_struct, 0, sizeof(struct napi_struct));
            netif_napi_add(mcps_device , &pantry->rx_napi_struct ,
                    process_gro_pantry , mcps_gro_pantry_quota);
            napi_enable(&pantry->rx_napi_struct);

            getnstimeofday(&pantry->flush_time);
        }
    }

    MCPS_DEBUG("COMPLETED \n");
}

void mcps_gro_exit(void)
{
    int i = 0;

    for_each_possible_cpu(i) {
        if(VALID_CPU(i)) {
            struct mcps_pantry *pantry = &per_cpu(mcps_gro_pantries, i);
            napi_disable(&pantry->rx_napi_struct);
            netif_napi_del(&pantry->rx_napi_struct);
        }
    }
}
//#endif
