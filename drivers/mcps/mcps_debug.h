#ifndef __MCPS_DEBUG_H__
#define __MCPS_DEBUG_H__

#include <linux/skbuff.h>
#include <linux/printk.h>

#include "mcps_sauron.h"

#ifdef CONFIG_MCTCP_DEBUG_PRINTK
#define MCPS_DEBUG(fmt, ...) printk(KERN_DEBUG "MCPSD %s : "fmt , __FUNCTION__, ##__VA_ARGS__);
#else
#define MCPS_DEBUG(fmt, ...) {};
#endif
#define MCPS_INFO(fmt, ...) printk(KERN_INFO "MCPSI %s : "fmt , __FUNCTION__, ##__VA_ARGS__);

/* This struct use char cb[48] in struct sk_buff.
*/
struct mcps_flow_debug_info {
    unsigned int index;
};
#define MCPSCB(skb) ((struct mcps_flow_debug_info*)(skb->cb))

#ifdef CONFIG_MCPS_DEBUG_OP_TIME
unsigned long tick_us(void);
#endif

#ifdef CONFIG_MCPS_DEBUG
void update_protocol_info(struct sk_buff *skb , struct eye *flow);
void mcps_index(struct sk_buff * skb);
void mcps_out_packet(struct sk_buff * skb);
void mcps_drop_packet(unsigned int hash);
void mcps_in_packet(struct eye* flow , struct sk_buff * skb);
void mcps_migrate_flow_history(struct eye *flow , int vec);
int init_mcps_debug_manager(void);
int release_mcps_debug_manager(void);
void dump(struct eye * flow);
#else
void update_protocol_info(struct sk_buff *skb , struct eye *flow){}
void mcps_index(struct sk_buff * skb){}
void mcps_out_packet(struct sk_buff * skb){}
void mcps_drop_packet(unsigned int hash){}
void mcps_in_packet(struct eye* flow , struct sk_buff * skb){}
void mcps_migrate_flow_history(struct eye *flow , int vec){}
int init_mcps_debug_manager(void) {}
int release_mcps_debug_manager(void) {}
void dump(struct eye * flow) {}
#endif

#ifdef CONFIG_MCPS_DEBUG_SYSTRACE
extern void tracing_mark_writev(char sig, int pid, char *func, int value);
#else
#define tracing_mark_writev(sig, pid, func, value){}
#endif
#endif //__MCPS_DEBUG_H__
