#ifndef _MCTCP_BUFFER_H
#define _MCTCP_BUFFER_H

#include <linux/skbuff.h>

int try_to_hqueue(unsigned int hash, unsigned int old , struct sk_buff * skb, u32 gro);
int pop_hqueue(unsigned int cur, unsigned int cpu , struct sk_buff_head* queue, u32 gro);

bool ON_PENDING(unsigned int hash, struct sk_buff* skb, u32 gro);
void hotplug_on(unsigned int off , unsigned int cur , u32 gro);
void hotplug_off(unsigned int off , unsigned int cur , u32 gro);
int FLUSH(unsigned int cpu, unsigned int cnt , u32 gro);

void update_pending_info (struct list_head * q, unsigned int cpu, unsigned int enqueued);
void splice_pending_info_di (unsigned int cpu , u32 gro);
void splice_pending_info_2q (unsigned int cpu ,struct list_head * queue , u32 gro);
void splice_pending_info_2cpu (struct list_head * queue , unsigned int cpu , u32 gro);
void check_pending_info(unsigned int cpu, const unsigned int processed , u32 gro);
int pending_migration (unsigned int hash, unsigned int from , unsigned int to , u32 gro);

void init_mcps_buffer(void);
void release_mcps_buffer(void);

#endif
