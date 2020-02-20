#ifndef _MCPS_FLOW_LEVEL_H
#define _MCPS_FLOW_LEVEL_H

#include <linux/printk.h>
#include <linux/netdevice.h>
#include <linux/cpu.h>

void mcps_napi_schedule(void *info);
void mcps_do_ipi_and_irq_enable(struct mcps_pantry *pantry);
bool mcps_on_ipi_waiting(struct mcps_pantry *pantry);
int mcps_ipi_queued(struct mcps_pantry *pantry);

int mcps_gro_ipi_queued(struct mcps_pantry *pantry);
void mcps_gro_init(struct net_device * mcps_device);
void mcps_gro_exit(void);

#ifdef CONFIG_MCTCP_DEBUG_PRINTK
#define PRINT_TRY_FAIL(hash , cpu) { \
    printk(KERN_DEBUG "MCPSD %s : Skip [%u]session - reason [%d]\n", __FUNCTION__, hash , cpu); \
};
#else
#define PRINT_TRY_FAIL(hash , cpu) {};
#endif
#endif
