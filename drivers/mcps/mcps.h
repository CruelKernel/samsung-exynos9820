#ifndef _MCTCP_H
#define _MCTCP_H

#include <linux/skbuff.h>

extern int mcps_try_skb(struct sk_buff *skb);

#ifdef CONFIG_MCPS_GRO_ENABLE
extern int mcps_try_gro(struct sk_buff *skb);
#else
#define mcps_try_gro(skb) mcps_try_skb(skb)
#endif

#endif
