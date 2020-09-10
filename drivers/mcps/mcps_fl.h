#ifndef _MCPS_FLOW_LEVEL_H
#define _MCPS_FLOW_LEVEL_H

#include <linux/printk.h>
#include <linux/netdevice.h>
#include <linux/cpu.h>

void mcps_napi_schedule(void *info);
void mcps_do_ipi_and_irq_enable(struct mcps_pantry *pantry);
bool mcps_on_ipi_waiting(struct mcps_pantry *pantry);
int schedule_ipi(struct napi_struct *napi, int quota);
int mcps_ipi_queued(struct mcps_pantry *pantry);

int mcps_gro_ipi_queued(struct mcps_pantry *pantry);
void mcps_gro_init(struct net_device *mcps_device);
void mcps_gro_exit(void);

int enqueue_to_pantry(struct sk_buff *skb, int cpu);
#if defined(CONFIG_MCPS_V2)
int mcps_try_skb_internal(struct sk_buff *skb);
#endif // #if defined(CONFIG_MCPS_V2)

#ifdef CONFIG_MCPS_DEBUG_PRINTK
#define PRINT_TRY_FAIL(hash, cpu) printk(KERN_DEBUG "MCPSD %s : Skip [%u]session - reason [%d]\n", __func__, hash, cpu)
#else
#define PRINT_TRY_FAIL(hash, cpu) do { } while (0)
#endif
#endif
