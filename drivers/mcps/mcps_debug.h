#ifndef __MCPS_DEBUG_H__
#define __MCPS_DEBUG_H__

#include <linux/skbuff.h>
#include <linux/printk.h>

#include "mcps_sauron.h"

#define DEBUG_LEVEL_TOP 1
#define DEBUG_LEVEL_MID 2
#define DEBUG_LEVEL_LOW 3
#define DEBUG_LEVEL_ERR 4

#if defined(CONFIG_MCPS_DEBUG_PRINTK) && (CONFIG_MCPS_DEBUG_PRINTK <= DEBUG_LEVEL_TOP)
#define MCPS_VERBOSE(fmt, ...) printk(KERN_DEBUG "MCPSV %s : "fmt, __func__, ##__VA_ARGS__)
#else
#define MCPS_VERBOSE(fmt, ...) do { } while (0)
#endif

#if defined(CONFIG_MCPS_DEBUG_PRINTK) && (CONFIG_MCPS_DEBUG_PRINTK <= DEBUG_LEVEL_MID)
#define MCPS_DEBUG(fmt, ...) printk(KERN_DEBUG "MCPSD %s : "fmt, __func__, ##__VA_ARGS__)
#else
#define MCPS_DEBUG(fmt, ...) do { } while (0)
#endif

#if defined(CONFIG_MCPS_DEBUG_PRINTK) && (CONFIG_MCPS_DEBUG_PRINTK <= DEBUG_LEVEL_LOW)
#define MCPS_INFO(fmt, ...) printk(KERN_INFO "MCPSI %s : "fmt, __func__, ##__VA_ARGS__)
#else
#define MCPS_INFO(fmt, ...) do { } while (0)
#endif

#if defined(CONFIG_MCPS_DEBUG_PRINTK) && (CONFIG_MCPS_DEBUG_PRINTK <= DEBUG_LEVEL_ERR)
#define MCPS_ERR(fmt, ...) printk(KERN_ERR "MCPSE %s : "fmt, __func__, ##__VA_ARGS__)
#else
#define MCPS_ERR(fmt, ...) do { } while (0)
#endif

/* This struct use char cb[48] in struct sk_buff.
*/
struct mcps_flow_debug_info {
    unsigned int index;
};
#define MCPSCB(skb) ((struct mcps_flow_debug_info *)(skb->cb))

#ifdef CONFIG_MCPS_DEBUG_OP_TIME
unsigned long tick_us(void);
#endif

#ifdef CONFIG_MCPS_DEBUG
void mcps_out_packet(struct sk_buff *skb);
void mcps_drop_packet(unsigned int hash);
void mcps_in_packet(struct eye *flow, struct sk_buff *skb);
void mcps_migrate_flow_history(struct eye *flow, int vec);
int init_mcps_debug_manager(void);
int release_mcps_debug_manager(void);
void dump(struct eye *flow);
#else
#define mcps_out_packet(skb)                 do { } while (0)
#define mcps_drop_packet(hash)               do { } while (0)
#define mcps_in_packet(flow, skb)            do { } while (0)
#define mcps_migrate_flow_history(flow, vec) do { } while (0)
#define init_mcps_debug_manager()            do { } while (0)
#define release_mcps_debug_manager()         do { } while (0)
#define dump(flow)                           do { } while (0)
#endif

#ifdef CONFIG_MCPS_DEBUG_SYSTRACE
extern void tracing_mark_writev(char sig, int pid, char *func, int value);
#else
#define tracing_mark_writev(sig, pid, func, value) do { } while (0)
#endif
#endif //__MCPS_DEBUG_H__
