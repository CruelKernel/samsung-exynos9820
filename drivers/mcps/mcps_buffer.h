#ifndef _MCTCP_BUFFER_H
#define _MCTCP_BUFFER_H

#include <linux/skbuff.h>

enum {
	NO_PENDING = 0,
	ON_PENDING,
};

struct pending_queue {
	int state;
	struct sk_buff_head rx_pending_queue;
};

#define pending_available(q) ((q)->state == NO_PENDING)
#define on_pending(q) ((q)->state == ON_PENDING)

static inline void init_pendings(struct pending_queue *q)
{
	q->state = NO_PENDING;
	skb_queue_head_init(&q->rx_pending_queue);
}

static inline void lock_pendings(struct pending_queue *q)
{
	spin_lock(&q->rx_pending_queue.lock);
}

static inline void unlock_pendings(struct pending_queue *q)
{
	spin_unlock(&q->rx_pending_queue.lock);
}

void discard_buffer(struct pending_queue *q);
struct pending_queue *find_pendings(unsigned long data);
void mcps_current_processor_gro_flush(void);
int splice_to_gro_pantry(struct sk_buff_head *skb, int len, int cpu);

int try_to_hqueue(unsigned int hash, unsigned int old, struct sk_buff *skb, u32 gro);
int pop_hqueue(unsigned int cur, unsigned int cpu, struct sk_buff_head *queue, u32 gro);

bool check_pending(struct pending_queue *q, struct sk_buff *skb);
void hotplug_on(unsigned int off, unsigned int cur, u32 gro);
void hotplug_off(unsigned int off, unsigned int cur, u32 gro);

void update_pending_info(struct list_head *q, unsigned int cpu, unsigned int enqueued);
void splice_pending_info_2q(unsigned int cpu, struct list_head *queue);
void splice_pending_info_2cpu(struct list_head *queue, unsigned int cpu);
int pending_migration(struct pending_queue *buf, unsigned int hash, unsigned int from, unsigned int to);

void init_mcps_buffer(struct net_device *dev);
void release_mcps_buffer(void);

#endif
