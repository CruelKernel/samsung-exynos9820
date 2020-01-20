#ifndef __MODEM_PKTLOG_H__
#define __MODEM_PKTLOG_H__

#ifdef CONFIG_DEBUG_PKTLOG

#include <linux/time.h>

struct pktbuf_private {
	unsigned char dir;
} __packed;
#define pktpriv(pkt) ((struct pktbuf_private *)&pkt->cb)

#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4
#define TCPDUMP_MAGIC 0xa1b2c3d4

#define DLT_LINUX_SLL 113
#define WTAP_ENCAP_USER0 147 /* sipc5 header */

struct pcap_file_header {
	unsigned magic;
	unsigned short version_major;
	unsigned short version_minor;
	int thiszone;
	unsigned sigflag;
	unsigned snaplen;
	unsigned linktype;
} __packed;

struct pcap_hdr {
	unsigned tv_sec;
	unsigned tv_usec;
	unsigned caplen;
	unsigned len;
} __packed;

#define SLL_ADDRLEN 8
struct sll_hdr {
	unsigned short pkttype;
	unsigned short hatype;
	unsigned short halen;
	unsigned char addr[SLL_ADDRLEN];
	unsigned short protocol;
} __packed;

struct sipc_debug {
	unsigned char dir;
} __packed;

struct pktdump_hdr {
	struct pcap_hdr pcap;
	struct sipc_debug sd;
} __packed;

struct pktlog_data {
	struct miscdevice misc;
	atomic_t opened;
	wait_queue_head_t wq;

	unsigned qmax;
	unsigned snaplen;

	struct sk_buff_head logq;

	bool copy_file_header;
	struct pcap_file_header file_hdr;
	struct pktdump_hdr hdr;
};

enum {
	PKTLOG_BOTTOM,	/* to/from phy layer */
	PKTLOG_TOP,	/* to/from network/ril layer */
	PKTLOG_TEXT,
};

#define PKT_TX 0x10
#define PKT_RX 0x20
#define PKT_IP 0x40

#define PKT_BOT 0x0
#define PKT_TOP 0xF


extern struct pktlog_data *create_pktlog(char *name);
extern void remove_pktlog(struct pktlog_data *pktlog);
extern void pktlog_queue_skb(struct pktlog_data *pktlog, unsigned char dir,
		struct sk_buff *skb);

#define pktlog_tx_bottom_skb(mld, skb)	pktlog_queue_skb((mld)->pktlog, (PKT_TX | PKT_BOT) , skb)
#define pktlog_tx_top_skb(mld, skb)	pktlog_queue_skb((mld)->pktlog, (PKT_TX | PKT_TOP) , skb)
#define pktlog_rx_bottom_skb(mld, skb)	pktlog_queue_skb((mld)->pktlog, (PKT_RX | PKT_BOT) , skb)
#define pktlog_rx_top_skb(mld, skb)	pktlog_queue_skb((mld)->pktlog, (PKT_RX | PKT_TOP) , skb)
#define pktlog_text_buf(mld, buf)	do {} while (0)

#else /* else of DEBUG_PKTLOG */
static inline struct pktlog_data *create_pktlog(char *name) { return NULL; }
#define remove_pktlog(pkt)		do {} while (0)
#define pktlog_tx_bottom_skb(mld, skb)	do {} while (0)
#define pktlog_tx_top_skb(mld, skb)	do {} while (0)
#define pktlog_rx_bottom_skb(mld, skb)	do {} while (0)
#define pktlog_rx_top_skb(mld, skb)	do {} while (0)
#define pktlog_text_buf(mld, buf)	do {} while (0)
#endif /* end of DEBUG_PKTLOG */

#endif
