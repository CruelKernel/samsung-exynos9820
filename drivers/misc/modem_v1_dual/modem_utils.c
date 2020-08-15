/*
 * Copyright (C) 2011 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdarg.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/debug-snapshot.h>
#include <linux/bitops.h>
#include <linux/smc.h>
#include <soc/samsung/pmu-cp.h>
#include <soc/samsung/exynos-modem-ctrl.h>

#include <soc/samsung/acpm_ipc_ctrl.h>
#include "modem_prj.h"
#include "modem_utils.h"

#define CMD_SUSPEND	((u16)(0x00CA))
#define CMD_RESUME	((u16)(0x00CB))

#define TX_SEPARATOR	"mif: >>>>>>>>>> Outgoing packet "
#define RX_SEPARATOR	"mif: Incoming packet <<<<<<<<<<"
#define LINE_SEPARATOR	\
	"mif: ------------------------------------------------------------"
#define LINE_BUFF_SIZE	80

enum bit_debug_flags {
	DEBUG_FLAG_FMT,
	DEBUG_FLAG_MISC,
	DEBUG_FLAG_RFS,
	DEBUG_FLAG_PS,
	DEBUG_FLAG_BOOT,
	DEBUG_FLAG_DUMP,
	DEBUG_FLAG_CSVT,
	DEBUG_FLAG_LOG,
	DEBUG_FLAG_ALL,
};

int cp_crash_link;
module_param(cp_crash_link, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cp_crash_link, "save crash link");

char *cp_crash_info = "none";
module_param(cp_crash_info, charp, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cp_crash_info, "save crash info");

struct crash_reason *nr_crash_reason;

#define DEBUG_FLAG_DEFAULT    (1 << DEBUG_FLAG_FMT | 1 << DEBUG_FLAG_MISC)
#ifdef DEBUG_MODEM_IF_PS_DATA
static unsigned long dflags = (DEBUG_FLAG_DEFAULT | 1 << DEBUG_FLAG_RFS | 1 << DEBUG_FLAG_PS);
#else
static unsigned long dflags = (DEBUG_FLAG_DEFAULT);
#endif
module_param(dflags, ulong, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(dflags, "modem_v1 debug flags");

static unsigned long wakeup_dflags = (DEBUG_FLAG_DEFAULT | 1 << DEBUG_FLAG_RFS | 1 << DEBUG_FLAG_PS);
module_param(wakeup_dflags, ulong, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(wakeup_dflags, "modem_v1 wakeup debug flags");

static const char *hex = "0123456789abcdef";

static struct raw_notifier_head cp_crash_notifier;

struct mif_buff_mng *g_mif_buff_mng = NULL;

static inline void ts2utc(struct timespec *ts, struct utc_time *utc)
{
	struct tm tm;

	time_to_tm((ts->tv_sec - (sys_tz.tz_minuteswest * 60)), 0, &tm);
	utc->year = 1900 + (u32)tm.tm_year;
	utc->mon = 1 + tm.tm_mon;
	utc->day = tm.tm_mday;
	utc->hour = tm.tm_hour;
	utc->min = tm.tm_min;
	utc->sec = tm.tm_sec;
	utc->us = (u32)ns2us(ts->tv_nsec);
}

void get_utc_time(struct utc_time *utc)
{
	struct timespec ts;
	getnstimeofday(&ts);
	ts2utc(&ts, utc);
}

int mif_dump_log(struct modem_shared *msd, struct io_device *iod)
{
	unsigned long read_len = 0;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);
	while (read_len < MAX_MIF_BUFF_SIZE) {
		struct sk_buff *skb;

		skb = alloc_skb(MAX_IPC_SKB_SIZE, GFP_ATOMIC);
		if (!skb) {
			mif_err("ERR! alloc_skb fail\n");
			spin_unlock_irqrestore(&msd->lock, flags);
			return -ENOMEM;
		}
		memcpy(skb_put(skb, MAX_IPC_SKB_SIZE),
			msd->storage.addr + read_len, MAX_IPC_SKB_SIZE);
		skb_queue_tail(&iod->sk_rx_q, skb);
		read_len += MAX_IPC_SKB_SIZE;
		wake_up(&iod->wq);
	}
	spin_unlock_irqrestore(&msd->lock, flags);
	return 0;
}

static unsigned long long get_kernel_time(void)
{
	int this_cpu;
	unsigned long flags;
	unsigned long long time;

	preempt_disable();
	raw_local_irq_save(flags);

	this_cpu = smp_processor_id();
	time = cpu_clock(this_cpu);

	preempt_enable();
	raw_local_irq_restore(flags);

	return time;
}

void mif_ipc_log(enum mif_log_id id,
	struct modem_shared *msd, const char *data, size_t len)
{
	struct mif_ipc_block *block;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_ipc_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();
	block->len = (len > MAX_IPC_LOG_SIZE) ? MAX_IPC_LOG_SIZE : len;
	memcpy(block->buff, data, block->len);
}

void _mif_irq_log(enum mif_log_id id, struct modem_shared *msd,
	struct mif_irq_map map, const char *data, size_t len)
{
	struct mif_irq_block *block;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_irq_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();
	memcpy(&(block->map), &map, sizeof(struct mif_irq_map));
	if (data)
		memcpy(block->buff, data,
			(len > MAX_IRQ_LOG_SIZE) ? MAX_IRQ_LOG_SIZE : len);
}

void _mif_com_log(enum mif_log_id id,
	struct modem_shared *msd, const char *format, ...)
{
	struct mif_common_block *block;
	unsigned long int flags;
	va_list args;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_common_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();

	va_start(args, format);
	vsnprintf(block->buff, MAX_COM_LOG_SIZE, format, args);
	va_end(args);
}

void _mif_time_log(enum mif_log_id id, struct modem_shared *msd,
	struct timespec epoch, const char *data, size_t len)
{
	struct mif_time_block *block;
	unsigned long int flags;

	spin_lock_irqsave(&msd->lock, flags);

	block = (struct mif_time_block *)
		(msd->storage.addr + (MAX_LOG_SIZE * msd->storage.cnt));
	msd->storage.cnt = ((msd->storage.cnt + 1) < MAX_LOG_CNT) ?
		msd->storage.cnt + 1 : 0;

	spin_unlock_irqrestore(&msd->lock, flags);

	block->id = id;
	block->time = get_kernel_time();
	memcpy(&block->epoch, &epoch, sizeof(struct timespec));

	if (data)
		memcpy(block->buff, data,
			(len > MAX_IRQ_LOG_SIZE) ? MAX_IRQ_LOG_SIZE : len);
}

/* dump2hex
 * dump data to hex as fast as possible.
 * the length of @buff must be greater than "@len * 3"
 * it need 3 bytes per one data byte to print.
 */
static inline void dump2hex(char *buff, size_t buff_size,
			    const char *data, size_t data_len)
{
	char *dest = buff;
	size_t len;
	size_t i;

	if (buff_size < (data_len * 3))
		len = buff_size / 3;
	else
		len = data_len;

	for (i = 0; i < len; i++) {
		*dest++ = hex[(data[i] >> 4) & 0xf];
		*dest++ = hex[data[i] & 0xf];
		*dest++ = ' ';
	}

	/* The last character must be overwritten with NULL */
	if (likely(len > 0))
		dest--;

	*dest = 0;
}

static inline bool sipc_csd_ch(u8 ch)
{
	return (ch >= SIPC_CH_ID_CS_VT_DATA && ch <= SIPC_CH_ID_CS_VT_VIDEO) ?
		true : false;
}

static inline bool sipc_log_ch(u8 ch)
{
	return (ch >= SIPC_CH_ID_CPLOG1 && ch <= SIPC_CH_ID_CPLOG2) ?
		true : false;
}

static bool wakeup_log_enable = false;
inline void set_wakeup_packet_log(bool enable)
{
	wakeup_log_enable = enable;
}

inline unsigned long get_log_flags(void)
{
	return wakeup_log_enable ? wakeup_dflags : dflags;
}

void set_dflags(unsigned long flag)
{
	dflags = flag;
}

static inline bool log_enabled(u8 ch)
{
	unsigned long flags = get_log_flags();

	if (test_bit(DEBUG_FLAG_ALL, &flags))
		return 1;
	if (sipc_ps_ch(ch))
		return test_bit(DEBUG_FLAG_PS, &flags);
	if (sipc5_fmt_ch(ch))
		return test_bit(DEBUG_FLAG_FMT, &flags);
	if (sipc_log_ch(ch))
		return test_bit(DEBUG_FLAG_LOG, &flags);
	if (sipc5_rfs_ch(ch))
		return test_bit(DEBUG_FLAG_RFS, &flags);
	if (sipc_csd_ch(ch))
		return test_bit(DEBUG_FLAG_CSVT, &flags);
	if (sipc5_misc_ch(ch))
		return test_bit(DEBUG_FLAG_MISC, &flags);
	if (sipc5_boot_ch(ch))
		return test_bit(DEBUG_FLAG_BOOT, &flags);
	if (sipc5_dump_ch(ch))
		return test_bit(DEBUG_FLAG_DUMP, &flags);
	return 0;
}

/* print ipc packet */
void mif_pkt(u8 ch, const char *tag, struct sk_buff *skb)
{
	if (!log_enabled(ch))
		return;

	if (unlikely(!skb)) {
		mif_err("ERR! NO skb!!!\n");
		return;
	}

	pr_skb(tag, skb);
}

/* print buffer as hex string */
int pr_buffer(const char *tag, const char *data, size_t data_len,
							size_t max_len)
{
	size_t len = min(data_len, max_len);
	unsigned char str[len ? len * 3 : 1]; /* 1 <= sizeof <= max_len*3 */
	dump2hex(str, (len ? len * 3 : 1), data, len);

	/* don't change this printk to mif_debug for print this as level7 */
	return pr_info("%s: %s(%ld): %s%s\n", MIF_TAG, tag, (long)data_len,
			str, (len == data_len) ? "" : " ...");
}

/* flow control CM from CP, it use in serial devices */
int link_rx_flowctl_cmd(struct link_device *ld, const char *data, size_t len)
{
	struct modem_shared *msd = ld->msd;
	unsigned short *cmd, *end = (unsigned short *)(data + len);

	mif_debug("flow control cmd: size=%ld\n", (long)len);

	for (cmd = (unsigned short *)data; cmd < end; cmd++) {
		switch (*cmd) {
		case CMD_SUSPEND:
			iodevs_for_each(msd, iodev_netif_stop, 0);
			mif_info("flowctl CMD_SUSPEND(%04X)\n", *cmd);
			break;

		case CMD_RESUME:
			iodevs_for_each(msd, iodev_netif_wake, 0);
			mif_info("flowctl CMD_RESUME(%04X)\n", *cmd);
			break;

		default:
			mif_err("flowctl BAD CMD: %04X\n", *cmd);
			break;
		}
	}

	return 0;
}

struct io_device *get_iod_with_format(struct modem_shared *msd,
			enum dev_format format)
{
	struct rb_node *n = msd->iodevs_tree_fmt.rb_node;

	while (n) {
		struct io_device *iodev;

		iodev = rb_entry(n, struct io_device, node_fmt);
		if (format < iodev->format)
			n = n->rb_left;
		else if (format > iodev->format)
			n = n->rb_right;
		else
			return iodev;
	}

	return NULL;
}

void insert_iod_with_channel(struct modem_shared *msd, unsigned int channel,
			     struct io_device *iod)
{
	unsigned idx = msd->num_channels;

	msd->ch2iod[channel] = iod;
	msd->ch[idx] = channel;
	msd->num_channels++;
}

struct io_device *insert_iod_with_format(struct modem_shared *msd,
		enum dev_format format, struct io_device *iod)
{
	struct rb_node **p = &msd->iodevs_tree_fmt.rb_node;
	struct rb_node *parent = NULL;

	while (*p) {
		struct io_device *iodev;

		parent = *p;
		iodev = rb_entry(parent, struct io_device, node_fmt);
		if (format < iodev->format)
			p = &(*p)->rb_left;
		else if (format > iodev->format)
			p = &(*p)->rb_right;
		else
			return iodev;
	}

	rb_link_node(&iod->node_fmt, parent, p);
	rb_insert_color(&iod->node_fmt, &msd->iodevs_tree_fmt);
	return NULL;
}

void iodevs_for_each(struct modem_shared *msd, action_fn action, void *args)
{
	int i;

	for (i = 0; i < msd->num_channels; i++) {
		u8 ch = msd->ch[i];
		struct io_device *iod = msd->ch2iod[ch];
		action(iod, args);
	}
}

void iodev_netif_wake(struct io_device *iod, void *args)
{
	if (iod->io_typ == IODEV_NET && iod->ndev) {
		netif_wake_queue(iod->ndev);
		mif_info("%s\n", iod->name);
	}
}

void iodev_netif_stop(struct io_device *iod, void *args)
{
	if (iod->io_typ == IODEV_NET && iod->ndev) {
		netif_stop_queue(iod->ndev);
		mif_info("%s\n", iod->name);
	}
}

void netif_tx_flowctl(struct modem_shared *msd, bool tx_stop)
{
	struct io_device *iod;

	spin_lock(&msd->active_list_lock);
	list_for_each_entry(iod, &msd->activated_ndev_list, node_ndev) {
		if (tx_stop) {
			netif_stop_subqueue(iod->ndev, 0);
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
			mif_err("tx_stop:%s, iod->ndev->name:%s\n",
				tx_stop ? "suspend" : "resume",
				iod->ndev->name);
#endif
		} else {
			netif_wake_subqueue(iod->ndev, 0);
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
			mif_err("tx_stop:%s, iod->ndev->name:%s\n",
				tx_stop ? "suspend" : "resume",
				iod->ndev->name);
#endif
		}
	}
	spin_unlock(&msd->active_list_lock);

	return;
}

static void iodev_set_tx_link(struct io_device *iod, void *args)
{
	struct link_device *ld = (struct link_device *)args;
	if (iod->format == IPC_RAW && IS_CONNECTED(iod, ld)) {
		set_current_link(iod, ld);
		mif_err("%s -> %s\n", iod->name, ld->name);
	}
}

void rawdevs_set_tx_link(struct modem_shared *msd, enum modem_link link_type)
{
	struct link_device *ld = find_linkdev(msd, link_type);
	if (ld)
		iodevs_for_each(msd, iodev_set_tx_link, ld);
}

void stop_net_iface(struct link_device *ld, unsigned int channel)
{
	struct io_device *iod;
	unsigned long flags;

	spin_lock_irqsave(&ld->netif_lock, flags);

	if (test_bit(channel, &ld->netif_stop_mask)) {
		mif_err("channel %d was already stopped!\n", channel);
		goto exit;
	}

	iod = link_get_iod_with_channel(ld, channel);
	iodev_netif_stop(iod, 0);
	set_bit(channel, &ld->netif_stop_mask);

exit:
	spin_unlock_irqrestore(&ld->netif_lock, flags);
}

void stop_net_ifaces(struct link_device *ld)
{
	unsigned long flags;
	spin_lock_irqsave(&ld->netif_lock, flags);

	if (!atomic_read(&ld->netif_stopped)) {
		if (ld->msd)
			netif_tx_flowctl(ld->msd, true);

		atomic_set(&ld->netif_stopped, 1);
	}

	spin_unlock_irqrestore(&ld->netif_lock, flags);
}

void resume_net_iface(struct link_device *ld, unsigned int channel)
{
	struct io_device *iod;
	unsigned long flags;

	spin_lock_irqsave(&ld->netif_lock, flags);

	if (!test_bit(channel, &ld->netif_stop_mask)) {
		mif_err("channel %d was already resumed!\n", channel);
		goto exit;
	}

	iod = link_get_iod_with_channel(ld, channel);
	iodev_netif_wake(iod, 0);
	clear_bit(channel, &ld->netif_stop_mask);

exit:
	spin_unlock_irqrestore(&ld->netif_lock, flags);
}

void resume_net_ifaces(struct link_device *ld)
{
	unsigned long flags;

	spin_lock_irqsave(&ld->netif_lock, flags);

	if (atomic_read(&ld->netif_stopped) != 0) {
		if (ld->msd)
			netif_tx_flowctl(ld->msd, false);

		complete_all(&ld->raw_tx_resumed);
		atomic_set(&ld->netif_stopped, 0);
	}

	spin_unlock_irqrestore(&ld->netif_lock, flags);
}

/**
@brief		ipv4 string to be32 (big endian 32bits integer)
@return		zero when errors occurred
*/
__be32 ipv4str_to_be32(const char *ipv4str, size_t count)
{
	unsigned char ip[4];
	char ipstr[16]; /* == strlen("xxx.xxx.xxx.xxx") + 1 */
	char *next = ipstr;
	int i;

	strlcpy(ipstr, ipv4str, ARRAY_SIZE(ipstr));

	for (i = 0; i < 4; i++) {
		char *p;

		p = strsep(&next, ".");
		if (p && kstrtou8(p, 10, &ip[i]) < 0)
			return 0; /* == 0.0.0.0 */
	}

	return *((__be32 *)ip);
}

void mif_add_timer(struct timer_list *timer, unsigned long expire,
		   void (*function)(unsigned long), unsigned long data)
{
	if (timer_pending(timer))
		return;

	init_timer(timer);
	timer->expires = jiffies + expire;
	timer->function = function;
	timer->data = data;
	add_timer(timer);
}

void mif_print_data(const u8 *data, int len)
{
	int words = len >> 4;
	int residue = len - (words << 4);
	int i;
	char *b;
	char last[80];

	/* Make the last line, if ((len % 16) > 0) */
	if (residue > 0) {
		char tb[8];

		sprintf(last, "%04X: ", (words << 4));
		b = (char *)data + (words << 4);

		for (i = 0; i < residue; i++) {
			sprintf(tb, "%02x ", b[i]);
			strcat(last, tb);
			if ((i & 0x3) == 0x3) {
				sprintf(tb, " ");
				strcat(last, tb);
			}
		}
	}

	for (i = 0; i < words; i++) {
		b = (char *)data + (i << 4);
		mif_err("%04X: "
			"%02x %02x %02x %02x  %02x %02x %02x %02x  "
			"%02x %02x %02x %02x  %02x %02x %02x %02x\n",
			(i << 4),
			b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
			b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
	}

	/* Print the last line */
	if (residue > 0)
		mif_err("%s\n", last);
}

void mif_dump2format16(const u8 *data, int len, char *buff, char *tag)
{
	char *d;
	int i;
	int words = len >> 4;
	int residue = len - (words << 4);
	char line[LINE_BUFF_SIZE];

	for (i = 0; i < words; i++) {
		memset(line, 0, LINE_BUFF_SIZE);
		d = (char *)data + (i << 4);

		if (tag)
			sprintf(line, "%s%04X| "
				"%02x %02x %02x %02x  "
				"%02x %02x %02x %02x  "
				"%02x %02x %02x %02x  "
				"%02x %02x %02x %02x\n",
				tag, (i << 4),
				d[0], d[1], d[2], d[3],
				d[4], d[5], d[6], d[7],
				d[8], d[9], d[10], d[11],
				d[12], d[13], d[14], d[15]);
		else
			sprintf(line, "%04X| "
				"%02x %02x %02x %02x  "
				"%02x %02x %02x %02x  "
				"%02x %02x %02x %02x  "
				"%02x %02x %02x %02x\n",
				(i << 4),
				d[0], d[1], d[2], d[3],
				d[4], d[5], d[6], d[7],
				d[8], d[9], d[10], d[11],
				d[12], d[13], d[14], d[15]);

		strcat(buff, line);
	}

	/* Make the last line, if (len % 16) > 0 */
	if (residue > 0) {
		char tb[8];

		memset(line, 0, LINE_BUFF_SIZE);
		memset(tb, 0, sizeof(tb));
		d = (char *)data + (words << 4);

		if (tag)
			sprintf(line, "%s%04X|", tag, (words << 4));
		else
			sprintf(line, "%04X|", (words << 4));

		for (i = 0; i < residue; i++) {
			sprintf(tb, " %02x", d[i]);
			strcat(line, tb);
			if ((i & 0x3) == 0x3) {
				sprintf(tb, " ");
				strcat(line, tb);
			}
		}
		strcat(line, "\n");

		strcat(buff, line);
	}
}

void mif_dump2format4(const u8 *data, int len, char *buff, char *tag)
{
	char *d;
	int i;
	int words = len >> 2;
	int residue = len - (words << 2);
	char line[LINE_BUFF_SIZE];

	for (i = 0; i < words; i++) {
		memset(line, 0, LINE_BUFF_SIZE);
		d = (char *)data + (i << 2);

		if (tag)
			sprintf(line, "%s%04X| %02x %02x %02x %02x\n",
				tag, (i << 2), d[0], d[1], d[2], d[3]);
		else
			sprintf(line, "%04X| %02x %02x %02x %02x\n",
				(i << 2), d[0], d[1], d[2], d[3]);

		strcat(buff, line);
	}

	/* Make the last line, if (len % 4) > 0 */
	if (residue > 0) {
		char tb[8];

		memset(line, 0, LINE_BUFF_SIZE);
		memset(tb, 0, sizeof(tb));
		d = (char *)data + (words << 2);

		if (tag)
			sprintf(line, "%s%04X|", tag, (words << 2));
		else
			sprintf(line, "%04X|", (words << 2));

		for (i = 0; i < residue; i++) {
			sprintf(tb, " %02x", d[i]);
			strcat(line, tb);
		}
		strcat(line, "\n");

		strcat(buff, line);
	}
}

void mif_print_dump(const u8 *data, int len, int width)
{
	char *buff;

	buff = kzalloc(len << 3, GFP_ATOMIC);
	if (!buff) {
		mif_err("ERR! kzalloc fail\n");
		return;
	}

	if (width == 16)
		mif_dump2format16(data, len, buff, LOG_TAG);
	else
		mif_dump2format4(data, len, buff, LOG_TAG);

	pr_info("%s", buff);

	kfree(buff);
}

static void strcat_tcp_header(char *buff, u8 *pkt)
{
	struct tcphdr *tcph = (struct tcphdr *)pkt;
	int eol;
	char line[LINE_BUFF_SIZE] = {0, };
	char flag_str[32] = {0, };

/*-------------------------------------------------------------------------

				TCP Header Format

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Source Port          |       Destination Port        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Sequence Number                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Acknowledgment Number                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  Data |       |C|E|U|A|P|R|S|F|                               |
	| Offset| Rsvd  |W|C|R|C|S|S|Y|I|            Window             |
	|       |       |R|E|G|K|H|T|N|N|                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |         Urgent Pointer        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Options                    |    Padding    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             data                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

-------------------------------------------------------------------------*/

	snprintf(line, LINE_BUFF_SIZE,
		"%s: TCP:: Src.Port %u, Dst.Port %u\n",
		MIF_TAG, ntohs(tcph->source), ntohs(tcph->dest));
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE,
		"%s: TCP:: SEQ 0x%08X(%u), ACK 0x%08X(%u)\n",
		MIF_TAG, ntohs(tcph->seq), ntohs(tcph->seq),
		ntohs(tcph->ack_seq), ntohs(tcph->ack_seq));
	strcat(buff, line);

	if (tcph->cwr)
		strcat(flag_str, "CWR ");
	if (tcph->ece)
		strcat(flag_str, "ECE");
	if (tcph->urg)
		strcat(flag_str, "URG ");
	if (tcph->ack)
		strcat(flag_str, "ACK ");
	if (tcph->psh)
		strcat(flag_str, "PSH ");
	if (tcph->rst)
		strcat(flag_str, "RST ");
	if (tcph->syn)
		strcat(flag_str, "SYN ");
	if (tcph->fin)
		strcat(flag_str, "FIN ");
	eol = strlen(flag_str) - 1;
	if (eol > 0)
		flag_str[eol] = 0;
	snprintf(line, LINE_BUFF_SIZE, "%s: TCP:: Flags {%s}\n",
		MIF_TAG, flag_str);
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE,
		"%s: TCP:: Window %u, Checksum 0x%04X, Urgent %u\n", MIF_TAG,
		ntohs(tcph->window), ntohs(tcph->check), ntohs(tcph->urg_ptr));
	strcat(buff, line);
}

static void strcat_udp_header(char *buff, u8 *pkt)
{
	struct udphdr *udph = (struct udphdr *)pkt;
	char line[LINE_BUFF_SIZE] = {0, };

/*-------------------------------------------------------------------------

				UDP Header Format

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|          Source Port          |       Destination Port        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            Length             |           Checksum            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                             data                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

-------------------------------------------------------------------------*/

	snprintf(line, LINE_BUFF_SIZE,
		"%s: UDP:: Src.Port %u, Dst.Port %u\n",
		MIF_TAG, ntohs(udph->source), ntohs(udph->dest));
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE,
		"%s: UDP:: Length %u, Checksum 0x%04X\n",
		MIF_TAG, ntohs(udph->len), ntohs(udph->check));
	strcat(buff, line);

	if (ntohs(udph->dest) == 53) {
		snprintf(line, LINE_BUFF_SIZE, "%s: UDP:: DNS query!!!\n",
			MIF_TAG);
		strcat(buff, line);
	}

	if (ntohs(udph->source) == 53) {
		snprintf(line, LINE_BUFF_SIZE, "%s: UDP:: DNS response!!!\n",
			MIF_TAG);
		strcat(buff, line);
	}
}

void print_ipv4_packet(const u8 *ip_pkt, enum direction dir)
{
	char *buff;
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	char *pkt = (char *)ip_pkt + (iph->ihl << 2);
	u16 flags = (ntohs(iph->frag_off) & 0xE000);
	u16 frag_off = (ntohs(iph->frag_off) & 0x1FFF);
	int eol;
	char line[LINE_BUFF_SIZE] = {0, };
	char flag_str[16] = {0, };

/*---------------------------------------------------------------------------
				IPv4 Header Format

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|Version|  IHL  |Type of Service|          Total Length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         Identification        |C|D|M|     Fragment Offset     |
	|                               |E|F|F|                         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  Time to Live |    Protocol   |         Header Checksum       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Source Address                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Destination Address                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Options                    |    Padding    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	IHL - Header Length
	Flags - Consist of 3 bits
		The 1st bit is "Congestion" bit.
		The 2nd bit is "Dont Fragment" bit.
		The 3rd bit is "More Fragments" bit.

---------------------------------------------------------------------------*/

	if (iph->version != 4)
		return;

	buff = kzalloc(4096, GFP_ATOMIC);
	if (!buff)
		return;

	if (dir == TX)
		snprintf(line, LINE_BUFF_SIZE, "%s\n", TX_SEPARATOR);
	else
		snprintf(line, LINE_BUFF_SIZE, "%s\n", RX_SEPARATOR);
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE, "%s\n", LINE_SEPARATOR);
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE,
		"%s: IP4:: Version %u, Header Length %u, TOS %u, Length %u\n",
		MIF_TAG, iph->version, (iph->ihl << 2), iph->tos,
		ntohs(iph->tot_len));
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE, "%s: IP4:: ID %u, Fragment Offset %u\n",
		MIF_TAG, ntohs(iph->id), frag_off);
	strcat(buff, line);

	if (flags & IP_CE)
		strcat(flag_str, "CE ");
	if (flags & IP_DF)
		strcat(flag_str, "DF ");
	if (flags & IP_MF)
		strcat(flag_str, "MF ");
	eol = strlen(flag_str) - 1;
	if (eol > 0)
		flag_str[eol] = 0;
	snprintf(line, LINE_BUFF_SIZE, "%s: IP4:: Flags {%s}\n",
		MIF_TAG, flag_str);
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE,
		"%s: IP4:: TTL %u, Protocol %u, Header Checksum 0x%04X\n",
		MIF_TAG, iph->ttl, iph->protocol, ntohs(iph->check));
	strcat(buff, line);

	snprintf(line, LINE_BUFF_SIZE,
		"%s: IP4:: Src.IP %u.%u.%u.%u, Dst.IP %u.%u.%u.%u\n",
		MIF_TAG, ip_pkt[12], ip_pkt[13], ip_pkt[14], ip_pkt[15],
		ip_pkt[16], ip_pkt[17], ip_pkt[18], ip_pkt[19]);
	strcat(buff, line);

	switch (iph->protocol) {
	case 6: /* TCP */
		strcat_tcp_header(buff, pkt);
		break;

	case 17: /* UDP */
		strcat_udp_header(buff, pkt);
		break;

	default:
		break;
	}

	snprintf(line, LINE_BUFF_SIZE, "%s\n", LINE_SEPARATOR);
	strcat(buff, line);

	pr_err("%s\n", buff);

	kfree(buff);
}

bool is_dns_packet(const u8 *ip_pkt)
{
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	struct udphdr *udph = (struct udphdr *)(ip_pkt + (iph->ihl << 2));

	/* If this packet is not a UDP packet, return here. */
	if (iph->protocol != 17)
		return false;

	if (ntohs(udph->dest) == 53 || ntohs(udph->source) == 53)
		return true;
	else
		return false;
}

bool is_syn_packet(const u8 *ip_pkt)
{
	struct iphdr *iph = (struct iphdr *)ip_pkt;
	struct tcphdr *tcph = (struct tcphdr *)(ip_pkt + (iph->ihl << 2));

	/* If this packet is not a TCP packet, return here. */
	if (iph->protocol != 6)
		return false;

	if (tcph->syn || tcph->fin)
		return true;
	else
		return false;
}

void mif_init_irq(struct modem_irq *irq, unsigned int num, const char *name,
		  unsigned long flags)
{
	spin_lock_init(&irq->lock);
	irq->num = num;
	strncpy(irq->name, name, (MAX_NAME_LEN - 1));
	irq->flags = flags;
	mif_info("name:%s num:%d flags:0x%08lX\n", name, num, flags);
}

int mif_request_irq(struct modem_irq *irq, irq_handler_t isr, void *data)
{
	int ret;

	ret = request_irq(irq->num, isr, irq->flags, irq->name, data);
	if (ret) {
		mif_err("%s: ERR! request_irq fail (%d)\n", irq->name, ret);
		return ret;
	}

	enable_irq_wake(irq->num);
	irq->active = true;
	irq->registered = true;

	mif_info("%s(#%d) handler registered (flags:0x%08lX)\n",
		irq->name, irq->num, irq->flags);

	return 0;
}

void mif_enable_irq(struct modem_irq *irq)
{
	unsigned long flags;

	if (irq->registered == false)
		return;

	spin_lock_irqsave(&irq->lock, flags);

	if (irq->active) {
		mif_err("%s(#%d) is already active <%pf>\n",
			irq->name, irq->num, CALLER);
		goto exit;
	}

	enable_irq(irq->num);
	enable_irq_wake(irq->num);

	irq->active = true;

	mif_info("%s(#%d) is enabled <%pf>\n",
		irq->name, irq->num, CALLER);

exit:
	spin_unlock_irqrestore(&irq->lock, flags);
}

void mif_disable_irq(struct modem_irq *irq)
{
	unsigned long flags;

	if (irq->registered == false)
		return;

	spin_lock_irqsave(&irq->lock, flags);

	if (!irq->active) {
		mif_err("%s(#%d) is not active <%pf>\n",
			irq->name, irq->num, CALLER);
		goto exit;
	}

	disable_irq_nosync(irq->num);
	disable_irq_wake(irq->num);

	irq->active = false;

	mif_info("%s(#%d) is disabled <%pf>\n",
		irq->name, irq->num, CALLER);

exit:
	spin_unlock_irqrestore(&irq->lock, flags);
}

void mif_disable_irq_sync(struct modem_irq *irq)
{
	if (irq->registered == false)
		return;

	spin_lock(&irq->lock);

	if (!irq->active) {
		spin_unlock(&irq->lock);
		mif_err("%s(#%d) is not active <%pf>\n",
				irq->name, irq->num, CALLER);
		return;
	}

	spin_unlock(&irq->lock);

	disable_irq(irq->num);
	enable_irq_wake(irq->num);

	spin_lock(&irq->lock);
	irq->active = false;
	spin_unlock(&irq->lock);

	mif_info("%s(#%d) is disabled <%pf>\n",
		irq->name, irq->num, CALLER);
}

struct file *mif_open_file(const char *path)
{
	struct file *fp;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	fp = filp_open(path, O_RDWR|O_CREAT|O_APPEND, 0666);

	set_fs(old_fs);

	if (IS_ERR(fp))
		return NULL;

	return fp;
}

void mif_save_file(struct file *fp, const char *buff, size_t size)
{
	int ret;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	ret = fp->f_op->write(fp, buff, size, &fp->f_pos);
	if (ret < 0)
		mif_err("ERR! write fail\n");

	set_fs(old_fs);
}

void mif_close_file(struct file *fp)
{
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(get_ds());

	filp_close(fp, NULL);

	set_fs(old_fs);
}

int board_gpio_export(struct device *dev,
		unsigned gpio, bool dir, const char *name)
{
	int ret = 0;

	if (!gpio_is_valid(gpio)) {
		mif_err("invalid gpio pins - %s\n", name);
		return -EINVAL;
	}

	ret = gpio_export(gpio, dir);
	if (ret) {
		mif_err("%s: failed to export gpio (%d)\n", name, ret);
		return ret;
	}

	ret = gpio_export_link(dev, name, gpio);
	if (ret) {
		mif_err("%s: failed to export link_gpio (%d)\n", name, ret);
		return ret;
	}

	mif_info("%s exported\n", name);

	return 0;
}

void make_gpio_floating(unsigned int gpio, bool floating)
{
	if (floating)
		gpio_direction_input(gpio);
	else
		gpio_direction_output(gpio, 0);
}

int __ref register_cp_crash_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&cp_crash_notifier, nb);
}

void __ref modemctl_notify_event(enum modemctl_event evt)
{
	raw_notifier_call_chain(&cp_crash_notifier, evt, NULL);
}

void mif_set_snapshot(bool enable)
{
	if (!enable)
		acpm_stop_log();
	dbg_snapshot_set_enable("log_kevents", enable);
}

struct mif_buff_mng *init_mif_buff_mng(unsigned char *buffer_start,
	unsigned int buffer_size, unsigned int cell_size)
{
	struct mif_buff_mng *bm;

	if (buffer_start == NULL || buffer_size == 0 || cell_size == 0) {
		mif_err("init_mif_buff_mng parameter ERR!\n");
		return NULL;
	}

	mif_info("Init mif_buffer management - buffer:%pK, size:%u, cell_size:%u\n",
		buffer_start, buffer_size, cell_size);

	bm = kzalloc(sizeof(struct mif_buff_mng), GFP_KERNEL);
	if (bm == NULL)
		return NULL;

	bm->buffer_start = buffer_start;
	bm->buffer_end = buffer_start + buffer_size;
	bm->buffer_size = buffer_size;
	bm->cell_size = cell_size;
	bm->cell_count = buffer_size / cell_size;
	bm->free_cell_count = bm->cell_count;
	bm->used_cell_count = 0;

	bm->current_map_index = 0;
	bm->buffer_map_size = (unsigned int)(bm->cell_count /
			MIF_BITS_FOR_MAP_CELL) + 1;
	bm->buffer_map = kzalloc((MIF_BUFF_MAP_CELL_SIZE * bm->buffer_map_size),
		GFP_KERNEL);
	if (bm->buffer_map == NULL) {
		kfree(bm);
		return NULL;
	}

	mif_info("cell_count:%u, map_size:%u, map_size_byte:%lu  buff_map:%pK\n"
		, bm->cell_count, bm->buffer_map_size,
		(sizeof(unsigned int) * bm->buffer_map_size), bm->buffer_map);

#ifdef MIF_BUFF_DEBUG
	mif_info("MIF_BUFF_MAP_CELL_SIZE:%lu\n", MIF_BUFF_MAP_CELL_SIZE);
	mif_info("MIF_BITS_FOR_BYTE:%u\n", MIF_BITS_FOR_BYTE);
	mif_info("MIF_BITS_FOR_MAP_CELL:%lu\n", MIF_BITS_FOR_MAP_CELL);
#endif

	spin_lock_init(&bm->lock);

	return bm;
}

void exit_mif_buff_mng(struct mif_buff_mng *bm)
{
	if (bm) {
		kfree(bm->buffer_map);
		kfree(bm);
	}
}

void *alloc_mif_buff(struct mif_buff_mng *bm)
{
	unsigned char *buff_allocated;
	int i, j;
	unsigned int location;
	int find_flag = false;
	unsigned long flags;
	uint64_t test_map;
	int last_bit_set;

	if (bm == NULL || bm->buffer_map == NULL)
		return NULL;

	if (bm->free_cell_count == 0) {
#ifdef MIF_BUFF_DEBUG
		mif_info("ERR allocation fail\n");
#endif
		return NULL;
	}

	spin_lock_irqsave(&bm->lock, flags);

	for (i = bm->current_map_index ; i < bm->buffer_map_size; i++) {
		test_map = (uint64_t) bm->buffer_map[i];
		test_map = ~test_map;
		last_bit_set = fls64(test_map);

		if (last_bit_set == 0)
			continue;
		else
			j = MIF_BITS_FOR_MAP_CELL - last_bit_set;

		location = (i * MIF_BITS_FOR_MAP_CELL) + j;

		if (location >= bm->cell_count)
			break;

		find_flag = true;
		bm->buffer_map[i] |= (uint64_t)(MIF_64BIT_FIRST_BIT >> j);
		bm->current_map_index = i;

#ifdef MIF_BUFF_DEBUG
		mif_info("map: %016llx\n", bm->buffer_map[i]);
		mif_info("i:%d j:%d location:%d\n", i, j, location);
#endif
		break;
	}

	if (find_flag == false) {
		for (i = 0 ; i < bm->current_map_index; i++) {
			test_map = (uint64_t) bm->buffer_map[i];
			test_map = ~test_map;
			last_bit_set = fls64(test_map);

			if (last_bit_set == 0)
				continue;
			else
				j = MIF_BITS_FOR_MAP_CELL - last_bit_set;

			location = (i * MIF_BITS_FOR_MAP_CELL) + j;

			if (location >= bm->cell_count)
				break;

			find_flag = true;
			bm->buffer_map[i] |= (uint64_t)(MIF_64BIT_FIRST_BIT >> j);
			bm->current_map_index = i;

#ifdef MIF_BUFF_DEBUG
			mif_info("map: %016llx\n", bm->buffer_map[i]);
			mif_info("i:%d j:%d location:%d\n", i, j, location);
#endif
			break;
		}

	}

	if (find_flag == false) {
#ifdef MIF_BUFF_DEBUG
		mif_info("ERR allocation fail\n");
#endif
		spin_unlock_irqrestore(&bm->lock, flags);
		return NULL;
	}

	buff_allocated = bm->buffer_start;
	buff_allocated += (location * bm->cell_size);

	bm->free_cell_count--;
	bm->used_cell_count++;

	spin_unlock_irqrestore(&bm->lock, flags);

#ifdef MIF_BUFF_DEBUG
	mif_info("location:%d cell_size:%u\n", location, bm->cell_size);
	mif_info("buffer_allocated:%pK\n", buff_allocated);
	mif_info("used/free: %u/%u\n", get_mif_buff_used_count(bm),
			get_mif_buff_free_count(bm));
#endif

	return (void *)buff_allocated;
}

int free_mif_buff(struct mif_buff_mng *bm, void *buffer)
{
	unsigned char *uc_buffer = (unsigned char *)buffer;
	unsigned int addr_diff;
	int i, j, location;
	unsigned long flags;

	if (bm == NULL)
		return -1;

	if (buffer == NULL)
		return 0;

	addr_diff = (unsigned int)(uc_buffer - bm->buffer_start);

	if (addr_diff > bm->buffer_size) {
		mif_err("ERR Buffer:%pK is not my pool one.\n", uc_buffer);
		return -1;
	}

	location = addr_diff / bm->cell_size;
	i = location / MIF_BITS_FOR_MAP_CELL;
	j = location % MIF_BITS_FOR_MAP_CELL;

#ifdef MIF_BUFF_DEBUG
	mif_info("uc_buff:%pK diff:%u\n", uc_buffer, addr_diff);
	mif_info("location:%d i:%d j:%d\n", location, i, j);
#endif

	if ((bm->buffer_map[i] & (MIF_64BIT_FIRST_BIT >> j)) == 0) {
		mif_err("ERR Buffer:%pK is allready freed\n", uc_buffer);
		return -1;
	}

	spin_lock_irqsave(&bm->lock, flags);

	bm->buffer_map[i] &= ~(MIF_64BIT_FIRST_BIT >> j);
	bm->free_cell_count++;
	bm->used_cell_count--;

	spin_unlock_irqrestore(&bm->lock, flags);

#ifdef MIF_BUFF_DEBUG
	mif_info("used/free: %u/%u\n", get_mif_buff_used_count(bm),
			get_mif_buff_free_count(bm));
#endif
	return 0;
}

static inline bool is_zerocopy_buffer(struct sk_buff *skb)
{
	if (g_mif_buff_mng == NULL)
		return false;

	if (((g_mif_buff_mng->buffer_start <= skb->head) &&
			(skb->head < g_mif_buff_mng->buffer_end)))
		return true;
	else
		return false;
}

bool __skb_free_head_cp_zerocopy(struct sk_buff *skb)
{
	if (!is_zerocopy_buffer(skb))
		return false;

	free_mif_buff(g_mif_buff_mng, skb->head);
	return true;
}

int security_request_cp_ram_logging(void)
{
	int err;

	mif_err("SMC: CP_BOOT_REQ_CP_RAM_LOGGING\n");

	exynos_smc(SMC_ID_CLK, SSS_CLK_ENABLE, 0, 0);
	err = exynos_smc(SMC_ID, CP_BOOT_REQ_CP_RAM_LOGGING, 0, 0);
	exynos_smc(SMC_ID_CLK, SSS_CLK_DISABLE, 0, 0);

	mif_err("SMC: return_value=%d\n", err);

	return err;
}

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
struct rmnet_iod_storage rmnet_iod = {{RMNET_LINK_4G,}, };
static atomic_t cp_upload_cnt;

void insert_rmnet_iod_with_channel(struct io_device *iod, u8 type)
{
	u8 ch;
	u8 id;

	if (!iod)
		return;

	if (sipc_ps_ch(iod->id) &&
		((type >= 0) && (type < MAX_RMNET_LINK))) {

		ch = iod->id;
		id = ch - SIPC_CH_ID_PDP_0;

		rmnet_iod.iod_pot[type].rmnet_ch2id[ch] = id;
		rmnet_iod.iod_pot[type].rmnet[id] = iod;

		mif_info("rmnet_iod saved (ch:%d, iod:%s - addr:%p)\n",
				ch, iod->name, iod);
	}
}

struct io_device *get_static_rmnet_iod_with_channel(u8 ch)
{
	struct io_device *iod;
	u8 id;

	if (sipc_ps_ch(ch)) {
		id = rmnet_iod.iod_pot[RMNET_LINK_4G].rmnet_ch2id[ch];
		iod = rmnet_iod.iod_pot[RMNET_LINK_4G].rmnet[id];

		return iod;
	}

	return NULL;
}

struct io_device *get_current_rmnet_tx_iod(u8 ch)
{
	struct io_device *iod = get_static_rmnet_iod_with_channel(ch);
	struct link_device *ld = iod->__current_link;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	u32 path = mld->ulpath.ctl[(ch - SIPC_CH_ID_PDP_0)].path;
	struct io_device *tx_iod = NULL;
	u8 id;

	if (sipc_ps_ch(iod->id)) {
		id = rmnet_iod.iod_pot[path].rmnet_ch2id[iod->id];
		tx_iod = rmnet_iod.iod_pot[path].rmnet[id];
	} else
		mif_err("iod id is not PS channel\n");

	return tx_iod;
}

void print_ulpath_table(struct mem_link_device *mld)
{
	struct shmem_4mb_phys_map *map =
		(struct shmem_4mb_phys_map *)mld->base;
	int i;

	for (i = 0; i < RMNET_COUNT; i++)
		mif_info("rmnet%d: status/path - %d/%d\n",
			i,
			map->ulpath.ctl[i].status,
			map->ulpath.ctl[i].path);
}

int update_ul_path_table(struct mem_link_device *mld)
{
	struct shmem_4mb_phys_map *map =
		(struct shmem_4mb_phys_map *)mld->base;
	int i;
	bool suspend_req = false;
	bool resume_req = false;

	for (i = 0; i < RMNET_COUNT; i++) {
		if ((map->ulpath.ctl[i].status != mld->ulpath.ctl[i].status) ||
			(map->ulpath.ctl[i].path != mld->ulpath.ctl[i].path)) {

			mif_info("[%d] status/path - Old:%d/%d, New:%d/%d\n",
				i,
				mld->ulpath.ctl[i].status,
				mld->ulpath.ctl[i].path,
				map->ulpath.ctl[i].status,
				map->ulpath.ctl[i].path);

			mld->ulpath.ctl[i].status = map->ulpath.ctl[i].status;
			mld->ulpath.ctl[i].path = map->ulpath.ctl[i].path;

			if (mld->ulpath.ctl[i].status == 1)
				resume_req = true;
			else
				suspend_req = true;
		}
	}

	print_ulpath_table(mld);

	if (suspend_req && resume_req) {
		mif_err("ERR both suspend & resume were requested\n");
		return -EINVAL;
	}

	if (suspend_req) {
		mif_info("Suspend TX flowctrl\n");
		tx_flowctrl_suspend(mld);
	}

	if (resume_req) {
		mif_info("Resume TX flowctrl\n");
		tx_flowctrl_resume(mld);
	}

	return 0;
}

int update_rmnet_status(struct io_device *iod, bool open)
{
	struct link_device *ld = iod->__current_link;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct shmem_4mb_phys_map *map =
		(struct shmem_4mb_phys_map *)mld->base;
	u8 ch = iod->id;
	u8 id = ch - SIPC_CH_ID_PDP_0;

	if (sipc_ps_ch(iod->id)) {
		mif_info("Update rmnet state: %s to %d\n", iod->name, open);
		mld->ulpath.ctl[id].status = open;
		map->ulpath.ctl[id].status = open;
	}

	print_ulpath_table(mld);

	return 0;
}

void reset_cp_upload_cnt(void)
{
	atomic_set(&cp_upload_cnt, 0);
}

bool check_cp_upload_cnt(void)
{
	atomic_inc(&cp_upload_cnt);
	if (atomic_read(&cp_upload_cnt) == 2)
		return true;
	else
		return false;
}
#endif /* CONFIG_SEC_SIPC_DUAL_MODEM_IF */
