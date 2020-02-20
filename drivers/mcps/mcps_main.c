#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cpu.h>

#include "mcps_sauron.h"
#include "mcps_device.h"
#include "mcps_buffer.h"
#include "mcps_debug.h"
#include "mcps_fl.h"

int create_mcps(void);
void release_mcps(void);

int mcps_enable __read_mostly = 0;
module_param(mcps_enable , int , 0640);

int mcps_pantry_max_capability __read_mostly = 30000;
module_param(mcps_pantry_max_capability , int , 0640);
int mcps_pantry_quota __read_mostly = 300;
module_param(mcps_pantry_quota , int , 0640);

unsigned int num_mcps_dev __read_mostly = 1;
module_param(num_mcps_dev , int , 0640);

#ifdef CONFIG_MCTCP_DEBUG_PRINTK
int mcps_print_BBB __read_mostly = 0;
module_param(mcps_print_BBB , int , 0640);
#endif

DEFINE_PER_CPU_ALIGNED(struct mcps_pantry , mcps_pantries);
EXPORT_PER_CPU_SYMBOL(mcps_pantries);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
void mcps_napi_complete(struct napi_struct *n)
{
    BUG_ON(!test_bit(NAPI_STATE_SCHED, &n->state));

    list_del_init(&n->poll_list);
    smp_mb__before_atomic();

    clear_bit(NAPI_STATE_SCHED, &n->state);
}
#endif

/* Called from hardirq (IPI) context */
void mcps_napi_schedule(void *info)
{
    struct mcps_pantry * pantry = (struct mcps_pantry *)info;
    __napi_schedule_irqoff(&pantry->rx_napi_struct);
    pantry->received_arps++;
}

/* smp_call_function_single_async - Can deadlock when called with interrupts disabled.
* We allow cpu's that are not yet online though, as no one else can
* send smp call function interrupt to this cpu and as such deadlocks
* can't happen.
*/
void mcps_do_ipi_and_irq_enable(struct mcps_pantry *pantry)
{
    struct mcps_pantry *next = pantry->ipi_list;
    if(next) {
        pantry->ipi_list = NULL;

        local_irq_enable();

        while(next) {
            struct mcps_pantry *temp = next->ipi_next;
            if(mcps_cpu_online(next->cpu)) {
                PRINT_DO_IPI(next->cpu);

                smp_call_function_single_async(next->cpu , &next->csd);
            }
            next = temp;
        }
    }
}

bool mcps_on_ipi_waiting(struct mcps_pantry *pantry)
{
    return pantry->ipi_list != NULL;
}

static int process_pantry(struct napi_struct *napi , int quota)
{
    struct mcps_pantry *pantry = container_of(napi , struct mcps_pantry , rx_napi_struct);
    int work = 0;
    bool again = true;

    if(mcps_on_ipi_waiting(pantry)) {
        local_irq_disable();
        mcps_do_ipi_and_irq_enable(pantry);
    }

    while (again) {
        struct sk_buff *skb;

        while ((skb = __skb_dequeue(&pantry->process_queue))) {
            mcps_out_packet(skb);
            __this_cpu_inc(mcps_pantries.processed);
            netif_receive_skb(skb); //This function may only be called from softirq context and interrupts should be enabled. Ref.function description

            if (++work >= quota) {
                PRINT_WORKED(pantry->cpu , work , quota);
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
            PRINT_WORKED(pantry->cpu , work , quota);
            again = false;
        } else {
            skb_queue_splice_tail_init(&pantry->input_pkt_queue,
                                       &pantry->process_queue);
        }
        splice_pending_info_di(pantry->cpu, 0);

        pantry_unlock(pantry);
        local_irq_enable();

        check_pending_info(pantry->cpu , pantry->processed, 0);
    }

    check_pending_info(pantry->cpu , pantry->processed, 0);

end:
    return work;
}

int mcps_ipi_queued(struct mcps_pantry *pantry)
{
    struct mcps_pantry *this_pantry = this_cpu_ptr(&mcps_pantries);

    if(pantry != this_pantry) {
        pantry->ipi_next = this_pantry->ipi_list;
        this_pantry->ipi_list = pantry;

        pantry_lock(this_pantry);
        if (!__test_and_set_bit(NAPI_STATE_SCHED, &this_pantry->rx_napi_struct.state))
            __napi_schedule_irqoff(&this_pantry->rx_napi_struct);
        pantry_unlock(this_pantry);

        PRINT_IPIQ(this_pantry->cpu , pantry->cpu);
        return 1;
    }
    return 0;
}

static int enqueue_to_pantry(struct sk_buff *skb, int cpu)
{
    struct mcps_pantry *pantry;
    unsigned long flags;
    unsigned int qlen;

    pantry = &per_cpu(mcps_pantries, cpu);

    local_irq_save(flags); //save register information into flags and disable irq (local_irq_disabled)

    pantry_lock(pantry);

    // hp off.
    if(pantry->cpu == NR_CPUS) {
        int hdr_cpu = 0;
        pantry_unlock(pantry);
        local_irq_restore(flags);

        hdr_cpu = try_to_hqueue(skb->hash, cpu, skb, MCPS_NON_GRO_DEVICE);
        cpu = hdr_cpu;
        if(hdr_cpu < 0) {
            return NET_RX_SUCCESS;
        } else {
            pantry = &per_cpu(mcps_pantries, hdr_cpu);

            local_irq_save(flags);
            pantry_lock(pantry);
        }
    }

    qlen = skb_queue_len(&pantry->input_pkt_queue);
    if (qlen <= mcps_pantry_max_capability) {
        if (qlen) {
enqueue:
            __skb_queue_tail(&pantry->input_pkt_queue, skb);

            pantry->enqueued++;

            pantry_unlock(pantry);
            local_irq_restore(flags);

            return NET_RX_SUCCESS;
        }

        /* Schedule NAPI for backlog device
         * We can use non atomic operation since we own the queue lock
         */
        if (!__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state)) {
            if (!mcps_ipi_queued(pantry))
                __napi_schedule_irqoff(&pantry->rx_napi_struct);
        }
        goto enqueue;
    }

    MCPS_DEBUG("[%d] dropped \n" , cpu);

    pantry->dropped++;
    pantry_unlock(pantry);

    local_irq_restore(flags);

    return NET_RX_DROP;
}

static int mcps_try_skb_internal(struct sk_buff *skb)
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

    cpu = get_arps_cpu(mcps , skb, hash, 0);

    if (cpu < 0 || cpu > NR_CPUS) {
        rcu_read_unlock();
        goto error;
    }
    if (cpu == NR_CPUS) {
        rcu_read_unlock();
        return NET_RX_SUCCESS;
    }

    ret = enqueue_to_pantry(skb, cpu);
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

int mcps_try_skb(struct sk_buff *skb)
{
    if(!mcps_enable)
        return NET_RX_DROP;

    return mcps_try_skb_internal(skb);
}
EXPORT_SYMBOL(mcps_try_skb);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int mcps_cpu_startup_callback(unsigned int ocpu)
{
    struct mcps_pantry *pantry = &per_cpu(mcps_pantries, ocpu);
    tracing_mark_writev('B',1111,"mcps_cpu_online", ocpu);

    local_irq_disable();
    pantry_lock(pantry);
    pantry->cpu = ocpu;
    pantry_unlock(pantry);
    local_irq_enable();

    hotplug_on(ocpu, 0 , MCPS_NON_GRO_DEVICE);

    tracing_mark_writev('E',1111,"mcps_cpu_online", ocpu);

    mcps_gro_cpu_startup_callback(ocpu);

    cpumask_set_cpu(ocpu, mcps_cpu_online_mask);
    return 0;
}

int mcps_cpu_teardown_callback(unsigned int ocpu)
{
    unsigned int cpu, oldcpu = ocpu;
    struct mcps_pantry *pantry , *oldpantry;
    unsigned int qlen = 0;

    struct sk_buff_head queue;
    bool needsched = false;
    struct list_head pinfo_queue;

    cpumask_clear_cpu(ocpu, mcps_cpu_online_mask);

    tracing_mark_writev('B',1111,"mcps_cpu_offline", ocpu);
    skb_queue_head_init(&queue);
    INIT_LIST_HEAD(&pinfo_queue);

    cpu = light_cpu(0);
    if(cpu == ocpu) {
        cpu = 0;
    }

    pantry = &per_cpu(mcps_pantries, cpu);
    oldpantry = &per_cpu(mcps_pantries, oldcpu);

    MCPS_DEBUG("[%u] : CPU[%u -> %u] - HOTPLUGGED OUT\n"
            , smp_processor_id_safe(), oldcpu, cpu);

    //detach first.
    local_irq_disable();
    pantry_lock(oldpantry);
    if(test_bit(NAPI_STATE_SCHED, &oldpantry->rx_napi_struct.state))
        mcps_napi_complete(&oldpantry->rx_napi_struct);

    if(!skb_queue_empty(&oldpantry->process_queue)) {
        qlen += skb_queue_len(&oldpantry->process_queue);
        skb_queue_splice_tail_init(&oldpantry->process_queue, &queue);
    }

    if(!skb_queue_empty(&oldpantry->input_pkt_queue)) {
        qlen += skb_queue_len(&oldpantry->input_pkt_queue);
        skb_queue_splice_tail_init(&oldpantry->input_pkt_queue, &queue);
    }

    oldpantry->ignored += qlen;
    oldpantry->processed += qlen;

    splice_pending_info_2q(oldcpu , &pinfo_queue , 0);

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

    qlen = pop_hqueue(cpu , oldcpu, &pantry->input_pkt_queue, 0);
    if(qlen) {
        pantry->enqueued += qlen;
        needsched = true;
    }

    if(!list_empty(&pinfo_queue)) {
        needsched = true;
        update_pending_info(&pinfo_queue, cpu, pantry->enqueued);
        splice_pending_info_2cpu(&pinfo_queue , cpu , 0);
    }

    needsched = (needsched && !__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state));

    pantry_unlock(pantry);
    local_irq_enable();

    if(needsched && mcps_cpu_online(cpu)) {
        smp_call_function_single_async(pantry->cpu , &pantry->csd);
    }
    tracing_mark_writev('E',1111,"mcps_cpu_offline", cpu);

    mcps_gro_cpu_teardown_callback(ocpu);
    return 0;
}
#else
/* it is guaranteed that cpu will not be turned on again while processing this function.
 * - Documentation/power/suspend-and-cpuhotplug.txt
 * */
int mcps_cpu_callback(struct notifier_block *notifier , unsigned long action, void* ocpu)
{
    unsigned int cpu, oldcpu = (unsigned long)ocpu;
    struct mcps_pantry *pantry , *oldpantry;
    unsigned int qlen = 0;

    struct sk_buff_head queue;
    bool needsched = false;
    struct list_head pinfo_queue;

    mcps_gro_cpu_callback(notifier, action, ocpu);

    skb_queue_head_init(&queue);
    INIT_LIST_HEAD(&pinfo_queue);

    if (action == CPU_ONLINE)
        goto online;

    if (action != CPU_DEAD && action != CPU_DEAD_FROZEN)
        return NOTIFY_OK;

    cpu = light_cpu(0);
    if(cpu == ocpu) {
        cpu = 0;
    }

    pantry = &per_cpu(mcps_pantries, cpu);
    oldpantry = &per_cpu(mcps_pantries, oldcpu);

    MCPS_DEBUG("[%u] : CPU[%u -> %u] - HOTPLUGGED OUT\n"
            , smp_processor_id_safe(), oldcpu, cpu);

    //detach first.
    local_irq_disable();
    pantry_lock(oldpantry);
    if(test_bit(NAPI_STATE_SCHED, &oldpantry->rx_napi_struct.state))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
        mcps_napi_complete(&oldpantry->rx_napi_struct);
#else
        __napi_complete(&oldpantry->rx_napi_struct);
#endif

    if(!skb_queue_empty(&oldpantry->process_queue)) {
        qlen += skb_queue_len(&oldpantry->process_queue);
        skb_queue_splice_tail_init(&oldpantry->process_queue, &queue);
    }

    if(!skb_queue_empty(&oldpantry->input_pkt_queue)) {
        qlen += skb_queue_len(&oldpantry->input_pkt_queue);
        skb_queue_splice_tail_init(&oldpantry->input_pkt_queue, &queue);
    }

    oldpantry->ignored += qlen;
    oldpantry->processed += qlen;

    splice_pending_info_2q(oldcpu , &pinfo_queue , 0);

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

    qlen = pop_hqueue(cpu , oldcpu, &pantry->input_pkt_queue, 0);
    if(qlen) {
        pantry->enqueued += qlen;
        needsched = true;
    }

    if(!list_empty(&pinfo_queue)) {
        needsched = true;
        update_pending_info(&pinfo_queue, cpu, pantry->enqueued);
        splice_pending_info_2cpu(&pinfo_queue , cpu , 0);
    }

    if (needsched && !__test_and_set_bit(NAPI_STATE_SCHED, &pantry->rx_napi_struct.state)) {
        if (!mcps_ipi_queued(pantry))
            __napi_schedule_irqoff(&pantry->rx_napi_struct);
    }

    pantry_unlock(pantry);
    local_irq_enable();

    return NOTIFY_OK;
online:
    hotplug_on(oldcpu, cpu , 0);

    return NOTIFY_OK;
}
#endif

/* struct mcps_config * mcps - MCPS Module core structure.
 * The structure must be considered as singleton and immutable structure.
 * So I(youngwook) removed rcu.
 * */
struct mcps_config * mcps __read_mostly;
EXPORT_SYMBOL(mcps);

int create_mcps()
{
    int ret;
    struct mcps_config *config;

    config = (struct mcps_config*)kzalloc(sizeof(struct mcps_config) , GFP_KERNEL);
    if(!config)
        return -ENOMEM;

    ret = init_mcps(config);
    if(ret < 0) {
        kfree(config);
        return -EINVAL;
    }

    mcps = config;
    return 0;
}

void release_mcps()
{
    if(mcps) {
        del_mcps(mcps);
        kfree(mcps);
    }
}


struct net_device *mcps_device;

static int mcps_dev_init(struct net_device *dev)
{
    MCPS_DEBUG("called \n");

    return 0;
}

static void mcps_dev_uninit(struct net_device *dev)
{
    MCPS_DEBUG("called \n");
}

static const struct net_device_ops mcps_netdev_ops = {
    .ndo_init       = mcps_dev_init,
    .ndo_uninit     = mcps_dev_uninit,
};

static void mcps_dev_setup(struct net_device *dev)
{
    /* Initialize the device structure. */
    dev->netdev_ops = &mcps_netdev_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    dev->needs_free_netdev = true;
#else
    dev->destructor = free_netdev;
#endif
}

static int create_mcps_virtual_dev(void)
{
    int err;

    mcps_device = alloc_netdev(0, "mcps%d", NET_NAME_UNKNOWN, mcps_dev_setup);
    if(!mcps_device)
        return -ENOMEM;

    //must change to int register_netdev(struct net_device *dev) for lock.
    err = register_netdev(mcps_device);
    if (err < 0)
        goto err;

    MCPS_DEBUG("Success to create mcps netdevice \n");
    return 0;

err:
    free_netdev(mcps_device);
    MCPS_DEBUG("Fail to create mcps netdevice \n");
    return err;
}

static void release_mcps_virtual_dev(void)
{
    if(!mcps_device)
        return;

    unregister_netdev(mcps_device);
}

static int init_mcps_notifys(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    int ret = 0;
    ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "net/mcps:online",
            mcps_cpu_startup_callback , mcps_cpu_teardown_callback);
    if(ret < 0) { // < 0 return If fail to setup. ref) http://heim.ifi.uio.no/~knuto/kernel/4.14/core-api/cpu_hotplug.html
        return -ENOMEM;
    }
#else
    struct notifier_block* cpu_notifier = (struct notifier_block*)kzalloc(sizeof(struct notifier_block), GFP_KERNEL);
    if(!cpu_notifier)
        return -ENOMEM;

    if(!mcps)
        return -EINVAL;

    cpu_notifier->notifier_call = mcps_cpu_callback;
    mcps->mcps_cpu_notifier = cpu_notifier;

    register_cpu_notifier(mcps->mcps_cpu_notifier);
#endif
    return 0;
}

static int release_mcps_notifys(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    cpuhp_remove_state_nocalls(CPUHP_AP_ONLINE_DYN);
#else
    if(!mcps || !mcps->mcps_cpu_notifier)
        return -EINVAL;

    unregister_cpu_notifier(mcps->mcps_cpu_notifier);
    kfree(mcps->mcps_cpu_notifier);
#endif
    return 0;
}

static int mcps_init(void)
{
    int i = 0;
    int ret = 0;

    ret = create_mcps();
    if(ret) {
        MCPS_DEBUG("Fail to create_mcps : reason [%d]\n" , ret);
        return -ENOMEM;
    }

    if (create_mcps_virtual_dev()) {
        MCPS_DEBUG("create_mcps_virtual_dev - fail\n");
    }

    for_each_possible_cpu(i) {
        if(VALID_CPU(i)) {
            struct mcps_pantry *pantry = &per_cpu(mcps_pantries, i);
            skb_queue_head_init(&pantry->input_pkt_queue);
            skb_queue_head_init(&pantry->process_queue);

            pantry->received_arps = 0;
            pantry->processed = 0;
            pantry->enqueued = 0;
            pantry->dropped = 0;
            pantry->ignored = 0;

            pantry->csd.func = mcps_napi_schedule;
            pantry->csd.info = pantry;
            pantry->cpu = i;

            memset(&pantry->rx_napi_struct, 0, sizeof(struct napi_struct));
            netif_napi_add(mcps_device, &pantry->rx_napi_struct ,
                    process_pantry, mcps_pantry_quota);
            napi_enable(&pantry->rx_napi_struct);
        }
    }

    init_mcps_core();
#ifdef CONFIG_MCPS_GRO_ENABLE
    mcps_gro_init(mcps_device);
#endif
    init_mcps_notifys();

	return 0;
}

static void mcps_exit(void)
{
    int i = 0;

    release_mcps_notifys();
    for_each_possible_cpu(i) {
        if(VALID_CPU(i)) {
            struct mcps_pantry *pantry = &per_cpu(mcps_pantries, i);
            napi_disable(&pantry->rx_napi_struct);
            netif_napi_del(&pantry->rx_napi_struct);
        }
    }

#ifdef CONFIG_MCPS_GRO_ENABLE
    mcps_gro_exit();
#endif
    release_mcps_core();
    release_mcps();
    release_mcps_virtual_dev();
}

module_init(mcps_init);
module_exit(mcps_exit);

MODULE_AUTHOR("yw8738.kim@samsung.com");
MODULE_VERSION("0.4.0");
MODULE_DESCRIPTION("Multi-Core Packet Scheduler");

