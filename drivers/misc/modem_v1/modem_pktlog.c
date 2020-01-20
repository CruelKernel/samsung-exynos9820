/*
 * Copyright (C) 2010 Samsung Electronics.
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

#include <linux/poll.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>

#include "modem_pktlog.h"

void pktlog_queue_skb(struct pktlog_data *pktlog, unsigned char dir,
		struct sk_buff *skb)
{
	struct sk_buff *pkt;

	if (!pktlog || !pktlog->qmax)
		return;

	pkt = skb_clone(skb, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	if (!pkt) {
		pr_err("%s: fail to skb_clone for pktlog buf\n", __func__);
		return;
	}
	pkt->tstamp = timespec_to_ktime(current_kernel_time());
	pktpriv(pkt)->dir = dir;

	skb_queue_tail(&pktlog->logq, pkt);
	if (pktlog->logq.qlen > pktlog->qmax) {
		struct sk_buff *drop = skb_dequeue(&pktlog->logq);
		if (drop)
			dev_kfree_skb_any(drop);
		printk_ratelimited(KERN_INFO "%s: over qmax(%d)\n", __func__,
			pktlog->logq.qlen);
	}
	wake_up(&pktlog->wq);
}

static int pktlog_open(struct inode *inode, struct file *filp)
{
	struct pktlog_data *pktlog = filp->private_data;

	if (!pktlog) {
		pr_err("%s: Invalid pktlog data\n", __func__);
		return -EINVAL;
	}

	if (atomic_inc_return(&pktlog->opened) >= 2) {
		pr_err("%s: Not support multi user oepn(%d)\n", __func__,
			atomic_read(&pktlog->opened));
		atomic_dec(&pktlog->opened);
		return -EINVAL;
	}
	pktlog->file_hdr.snaplen = pktlog->snaplen;
	pktlog->copy_file_header = true;

	pr_info("%s: qmax = %d open by %s\n", __func__, pktlog->qmax,
			current->comm);
	return 0;
}

static int pktlog_release(struct inode *inode, struct file *filp)
{
	struct pktlog_data *pktlog = filp->private_data;

	pr_info("%s: qmax = %d close by %s- %d\n", __func__, pktlog->qmax,
			current->comm, atomic_dec_return(&pktlog->opened));
	return 0;
}

static unsigned int pktlog_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct pktlog_data *pktlog = filp->private_data;

	if (!pktlog) {
		pr_err("%s: Invalid pktlog data\n", __func__);
		return POLLERR;
	}

	if (skb_queue_empty(&pktlog->logq))
		poll_wait(filp, &pktlog->wq, wait);

	return POLLIN;
}

static ssize_t pktlog_read(struct file *filp, char *buf, size_t count,
			loff_t *fpos)
{
	struct pktlog_data *pktlog = filp->private_data;
	struct sk_buff *pkt;
	char *p = buf;
	struct pktdump_hdr *hdr = &pktlog->hdr;
	struct timeval tv;
	int ret;
	unsigned cook_hdr_len = sizeof(struct pktdump_hdr)
			- sizeof(struct pcap_hdr);
	unsigned payload_len, cplen = 0;

	if (!pktlog) {
		pr_err("%s: Invalid pktlog data\n", __func__);
		return -EINVAL;
	}

	if (pktlog->copy_file_header) {
		pktlog->copy_file_header = false;
		ret = copy_to_user(p, &pktlog->file_hdr,
				sizeof(struct pcap_file_header));
		if (ret < 0)
			printk_ratelimited(KERN_ERR
			"%s: fileheader copy fail\n", __func__);
		cplen += sizeof(struct pcap_file_header);
		p += sizeof(struct pcap_file_header);
	}

	pkt = skb_dequeue(&pktlog->logq);
	if (unlikely(!pkt)) {
		printk_ratelimited(KERN_ERR "%s: no log pakcets\n", __func__);
		return 0;
	}

	tv = ktime_to_timeval(pkt->tstamp);
	hdr->pcap.tv_sec = tv.tv_sec;
	hdr->pcap.tv_usec = tv.tv_usec;
	hdr->pcap.len = cook_hdr_len + pkt->len;
	hdr->pcap.caplen = min(hdr->pcap.len, pktlog->snaplen);

	hdr->sd.dir = pktpriv(pkt)->dir;

	ret = copy_to_user(p, hdr, sizeof(struct pktdump_hdr));
	if (ret < 0) {
		printk_ratelimited(KERN_ERR "%s: pcap header copy fail\n",
			__func__);
		goto exit;
	}
	cplen += sizeof(struct pktdump_hdr);

	p += sizeof(struct pktdump_hdr);
	payload_len = min(pkt->len, pktlog->snaplen - cook_hdr_len);
	ret = copy_to_user(p, pkt->data, payload_len);
	if (ret < 0) {
		printk_ratelimited(KERN_ERR "%s: pcap data copy fail\n",
			__func__);
		goto exit;
	}
	cplen += payload_len;

exit:
	dev_kfree_skb_any(pkt);
	return cplen;
}

static ssize_t show_snaplen(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pktlog_data *pktlog = dev_get_drvdata(dev);
	char *p = buf;

	p += sprintf(buf, "packet log snaplen : %u\n", pktlog->snaplen);

	return p - buf;
}

static ssize_t store_snaplen(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pktlog_data *pktlog = dev_get_drvdata(dev);
	unsigned snaplen;
	int ret;

	ret = kstrtouint(buf, 10, &snaplen);
	if (ret)
		return count;

	pktlog->snaplen = snaplen;
	return count;
}

static struct device_attribute attr_snaplen =
	__ATTR(snaplen, S_IRUGO | S_IWUSR, show_snaplen, store_snaplen);

static ssize_t show_qmax(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pktlog_data *pktlog = dev_get_drvdata(dev);
	char *p = buf;

	p += sprintf(buf, "packet log queue max : %u\n", pktlog->qmax);

	return p - buf;
}

static ssize_t store_qmax(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pktlog_data *pktlog = dev_get_drvdata(dev);
	unsigned qmax;
	int ret;

	ret = kstrtouint(buf, 10, &qmax);
	if (ret)
		return count;

	pktlog->qmax = qmax;
	return count;
}

static struct device_attribute attr_qmax =
	__ATTR(qmax, S_IRUGO | S_IWUSR, show_qmax, store_qmax);

static const struct file_operations pktlog_fops = {
	.owner = THIS_MODULE,
	.open = pktlog_open,
	.release = pktlog_release,
	.poll = pktlog_poll,
	.read = pktlog_read,
};

static void init_pcap_fileheader(struct pktlog_data *pktlog)
{
	struct pcap_file_header *hdr = &pktlog->file_hdr;

	hdr->magic = TCPDUMP_MAGIC;
	hdr->version_major = PCAP_VERSION_MAJOR;
	hdr->version_minor = PCAP_VERSION_MINOR;
	hdr->thiszone = 0;
	hdr->sigflag = 0;
	hdr->snaplen = pktlog->snaplen;
	hdr->linktype = WTAP_ENCAP_USER0;
}

struct pktlog_data *create_pktlog(char *name)
{
	struct pktlog_data *pktlog;
	int ret;
	char node[64] = "pktlog_"; /* /dev/pktlog/{name} */

	pktlog = kzalloc(sizeof(struct pktlog_data), GFP_KERNEL);
	if (!pktlog) {
		pr_err("%s: pktlog data alloc fail\n", __func__);
		return NULL;
	}

	pktlog->misc.minor = MISC_DYNAMIC_MINOR;
	pktlog->misc.name = strncat(node, name, 50);
	pktlog->misc.fops = &pktlog_fops;
	pktlog->misc.parent = NULL;

	init_waitqueue_head(&pktlog->wq);
	skb_queue_head_init(&pktlog->logq);
	pktlog->qmax = 0;
	pktlog->snaplen = 256;
	atomic_set(&pktlog->opened, 0);

	ret = misc_register(&pktlog->misc);
	if (ret < 0) {
		pr_err("%s: fail to register misc device '%s'\n", __func__,
			pktlog->misc.name);
		goto free_exit;
	}

	ret = device_create_file(pktlog->misc.this_device, &attr_qmax);
	if (ret) {
		pr_err("%s: fail to create qmax sysfs file: %s\n", __func__,
				name);
		goto free_exit;
	}

	ret = device_create_file(pktlog->misc.this_device, &attr_snaplen);
	if (ret) {
		pr_err("%s: fail to create snaplen sysfs file: %s\n", __func__,
				name);
		goto free_exit;
	}
	init_pcap_fileheader(pktlog);
	pr_info("%s: probed - %s\n", __func__, name);

	return pktlog;

free_exit:
	kfree(pktlog);
	return NULL;
}

void remove_pktlog(struct pktlog_data *pktlog)
{
	if (!pktlog)
		return;

	device_remove_file(pktlog->misc.this_device, &attr_qmax);
	misc_deregister(&pktlog->misc);
	kfree(pktlog);
}
