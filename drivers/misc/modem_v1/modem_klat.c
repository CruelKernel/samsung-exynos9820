/*
 * Copyright (C) 2019 Samsung Electronics.
 *
 * This program has been developed as a CLAT module running at kernel,
 * and it is based on android-clat written by
 *
 * Copyright (c) 2010-2012, Daniel Drown
 *
 * This file is dual licensed. It may be redistributed and/or modified
 * under the terms of the Apache 2.0 License OR version 2 of the GNU
 * General Public License.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/device.h>
#include <linux/module.h>
#include <net/ip.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/time.h>
#include <linux/timer.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_klat.h"

#define KOBJ_CLAT "clat"

#define IP6F_OFF_MASK       0xf8ff  /* mask out offset from _offlg */
#define IP6F_RESERVED_MASK  0x0600  /* reserved bits in ip6f_offlg */
#define IP6F_MORE_FRAG      0x0100  /* more-fragments flag */

#define MAX_TCP_HDR (15 * 4)

#define IN6_ARE_ADDR_EQUAL(a,b) \
	((((const uint32_t *) (a))[0] == ((const uint32_t *) (b))[0])	      \
	 && (((const uint32_t *) (a))[1] == ((const uint32_t *) (b))[1])      \
	 && (((const uint32_t *) (a))[2] == ((const uint32_t *) (b))[2])      \
	 && (((const uint32_t *) (a))[3] == ((const uint32_t *) (b))[3]))

struct klat klat_obj;

static uint16_t ip_checksum_fold(uint32_t temp_sum)
{
	while(temp_sum > 0xFFFF)
		temp_sum = (temp_sum >> 16) + (temp_sum & 0xFFFF);

	return temp_sum;
}

static uint16_t ip_checksum_finish(uint32_t temp_sum)
{
	return ~ip_checksum_fold(temp_sum);
}

static uint32_t ip_checksum_add(uint32_t curr, const void *data, int len)
{
	uint32_t checksum = curr;
	int left = len;
	const uint16_t *data_16 = data;

	while (left > 1) {
		checksum += *data_16;
		data_16++;
		left -= 2;
	}

	if (left)
		checksum += *(uint8_t *)data_16;

	return checksum;
}

static uint16_t ip_checksum(const void *data, int len)
{
	uint32_t temp_sum;

	temp_sum = ip_checksum_add(0, data, len);
	return ip_checksum_finish(temp_sum);
}

static uint32_t ipv6_pseudo_header_checksum(struct ipv6hdr *ip6, uint16_t len,
						uint8_t protocol)
{
	uint32_t checksum_len, checksum_next;
	uint32_t curr = 0;

	checksum_len = htonl((uint32_t) len);
	checksum_next = htonl(protocol);

	curr = ip_checksum_add(curr, &(ip6->saddr), sizeof(struct in6_addr));
	curr = ip_checksum_add(curr, &(ip6->daddr), sizeof(struct in6_addr));
	curr = ip_checksum_add(curr, &checksum_len, sizeof(checksum_len));
	curr = ip_checksum_add(curr, &checksum_next, sizeof(checksum_next));

	return curr;
}

static uint32_t ipv4_pseudo_header_checksum(struct iphdr *ip, uint16_t len)
{
	uint16_t temp_protocol, temp_length;
	uint32_t curr = 0;

	temp_protocol = htons(ip->protocol);
	temp_length = htons(len);

	curr = ip_checksum_add(curr, &(ip->saddr), sizeof(uint32_t));
	curr = ip_checksum_add(curr, &(ip->daddr), sizeof(uint32_t));
	curr = ip_checksum_add(curr, &temp_protocol, sizeof(uint16_t));
	curr = ip_checksum_add(curr, &temp_length, sizeof(uint16_t));

	return curr;
}

static uint16_t ip_checksum_adjust(uint16_t checksum, uint32_t old_hdr_sum,
					uint32_t new_hdr_sum)
{
	uint16_t folded_sum, folded_old;

	/* Algorithm suggested in RFC 1624.
	 * http://tools.ietf.org/html/rfc1624#section-3
	 */
	checksum = ~checksum;

	folded_sum = ip_checksum_fold(checksum + new_hdr_sum);
	folded_old = ip_checksum_fold(old_hdr_sum);

	if (folded_sum > folded_old)
		return ~(folded_sum - folded_old);

	return ~(folded_sum - folded_old - 1);
}

static uint16_t packet_checksum(uint32_t checksum, uint8_t *packet, size_t len)
{
	checksum = ip_checksum_add(checksum, packet, len);

	return ip_checksum_finish(checksum);
}

static int udp_translate(struct udphdr *udp, uint32_t old_sum, uint32_t new_sum,
				uint8_t *payload, size_t payload_size)
{
	if (udp->check) {
		udp->check = ip_checksum_adjust(udp->check, old_sum, new_sum);
	} else {
		udp->check = 0;
		udp->check = packet_checksum(new_sum, payload, payload_size);
	}

	/* RFC 768: "If the computed checksum is zero,
	 * it is transmitted as all ones (the equivalent in one's complement
	 * arithmetic)."
	 */
	if (!udp->check)
		udp->check = 0xFFFF;

	return 1;
}

static int tcp_translate(struct tcphdr *tcp,
			size_t header_size, uint32_t old_sum, uint32_t new_sum,
			const uint8_t *payload, size_t payload_size)
{
	if (header_size > MAX_TCP_HDR) {
		mif_err("header too long %d > %d, truncating", (int)header_size,
				MAX_TCP_HDR);
		header_size = MAX_TCP_HDR;
	}

	tcp->check = ip_checksum_adjust(tcp->check, old_sum, new_sum);

	return 1;
}

#if 0 // todo
static int is_icmp_error(uint8_t type)
{
	return type == 3 || type == 11 || type == 12;
}

static int is_icmp6_error(uint8_t type)
{
	return type < 128;
}

static uint8_t icmp6_to_icmp_type(uint8_t type, uint8_t code)
{
	switch (type) {
	case ICMPV6_ECHO_REQUEST:
		return ICMP_ECHO;

	case ICMPV6_ECHO_REPLY:
		return ICMP_ECHOREPLY;

	case ICMPV6_DEST_UNREACH:
		return ICMP_DEST_UNREACH;

	case ICMPV6_TIME_EXCEED:
		return ICMP_TIME_EXCEEDED;
	}

	mif_err("unhandled ICMP type/code %d/%d\n", type, code);
	return ICMP_PARAMETERPROB;
}

static uint8_t icmp6_to_icmp_code(uint8_t type, uint8_t code)
{
	switch (type) {
	case ICMPV6_ECHO_REQUEST:
	case ICMPV6_ECHO_REPLY:
	case ICMPV6_TIME_EXCEED:
		return code;

	case ICMPV6_DEST_UNREACH:
		switch (code) {
		case ICMPV6_NOROUTE:
			return ICMP_HOST_UNREACH;
		case ICMPV6_ADM_PROHIBITED:
			return ICMP_HOST_ANO;
		case ICMPV6_NOT_NEIGHBOUR:
			return ICMP_HOST_UNREACH;
		case ICMPV6_ADDR_UNREACH:
			return ICMP_HOST_UNREACH;
		case ICMPV6_PORT_UNREACH:
			return ICMP_PORT_UNREACH;
		}
	}

	mif_err("unhandled ICMP type/code %d/%d\n", type, code);
	return 0;
}

static uint8_t icmp_to_icmp6_type(uint8_t type, uint8_t code)
{
	switch (type) {
	case ICMP_ECHO:
		return ICMPV6_ECHO_REQUEST;

	case ICMP_ECHOREPLY:
		return ICMPV6_ECHO_REPLY;

	case ICMP_TIME_EXCEEDED:
		return ICMPV6_TIME_EXCEED;

	case ICMP_DEST_UNREACH:
		if (code != ICMP_PORT_UNREACH && code != ICMP_FRAG_NEEDED)
			return ICMPV6_DEST_UNREACH;
	}

	mif_err("unhandled ICMP type %d\n", type);
	return ICMPV6_PARAMPROB;
}

static uint8_t icmp_to_icmp6_code(uint8_t type, uint8_t code)
{
	switch (type) {
	case ICMP_ECHO:
	case ICMP_ECHOREPLY:
		return 0;

	case ICMP_TIME_EXCEEDED:
		return code;

	case ICMP_DEST_UNREACH:
		switch (code) {
		case ICMP_NET_UNREACH:
		case ICMP_HOST_UNREACH:
			return ICMPV6_NOROUTE;

		case ICMP_PROT_UNREACH:
			return ICMPV6_PORT_UNREACH;

		case ICMP_NET_ANO:
		case ICMP_HOST_ANO:
		case ICMP_PKT_FILTERED:
		case ICMP_PREC_CUTOFF:
			return ICMPV6_ADM_PROHIBITED;
		}
	}
	mif_err("unhandled ICMP type/code %d/%d\n", type, code);
	return 0;
}

static int icmp_to_icmp6(struct sk_buff *skb, struct icmphdr *icmp,
				uint32_t checksum, uint8_t *payload,
				size_t payload_size)
{
	/* todo */
	return 0;
}

static int icmp6_to_icmp(struct sk_buff *skb, struct icmp6hdr *icmp6,
				uint8_t *payload, size_t payload_size)
{
	/* todo */
	return 0;
}

int icmp_packet(struct sk_buff *skb, struct icmphdr *icmp, uint32_t checksum,
		size_t len)
{
	uint8_t *payload;
	size_t payload_size;

	if (len < sizeof(struct icmphdr)) {
		mif_err("icmp_packet/(too small)\n");
		return 0;
	}

	payload = (uint8_t *) (icmp + 1);
	payload_size = len - sizeof(struct icmphdr);

	return icmp_to_icmp6(skb, icmp, checksum, payload, payload_size);
}

static int icmp6_packet(struct sk_buff *skb, struct icmp6hdr *icmp6, size_t len)
{
	uint8_t *payload;
	size_t payload_size;

	if (len < sizeof(struct icmp6hdr)) {
		mif_err("too small\n");
		return 0;
	}

	payload = (uint8_t *) (icmp6 + 1);
	payload_size = len - sizeof(struct icmp6hdr);

	return icmp6_to_icmp(skb, icmp6, payload, payload_size);
}
#endif

static int udp_packet(struct udphdr *udp, uint32_t old_sum, uint32_t new_sum,
			size_t len)
{
	uint8_t *payload;
	size_t payload_size;

	if (len < sizeof(struct udphdr)) {
		mif_err("too small\n");
		return 0;
	}

	payload = (uint8_t *) (udp + 1);
	payload_size = len - sizeof(struct udphdr);

	return udp_translate(udp, old_sum, new_sum, payload, payload_size);
}

static int tcp_packet(struct tcphdr *tcp, uint32_t old_sum, uint32_t new_sum,
			size_t len)
{
	uint8_t *payload;
	size_t payload_size, header_size;

	if (len < sizeof(struct tcphdr)) {
		mif_err("too small\n");
		return 0;
	}

	if (tcp->doff < 5) {
		mif_err("tcp header length is less than 5: %x\n", tcp->doff);
		return 0;
	}

	if ((size_t)tcp->doff * 4 > len) {
		mif_err("tcp header length set too large: %x\n", tcp->doff);
		return 0;
	}

	header_size = tcp->doff * 4;
	payload = ((uint8_t *)tcp) + header_size;
	payload_size = len - header_size;

	return tcp_translate(tcp, header_size, old_sum, new_sum, payload,
				payload_size);
}

static uint8_t parse_frag_header(struct frag_hdr *frag_hdr,
					struct iphdr *ip_targ)
{
	uint16_t frag_off = (ntohs(frag_hdr->frag_off & IP6F_OFF_MASK) >> 3);

	if (frag_hdr->frag_off & IP6F_MORE_FRAG) {
		frag_off |= IP_MF;
	}

	ip_targ->frag_off = htons(frag_off);
	ip_targ->id = htons(ntohl(frag_hdr->identification) & 0xFFFF);
	ip_targ->protocol = frag_hdr->nexthdr;

	return frag_hdr->nexthdr;
}

static uint8_t icmp_guess_ttl(uint8_t ttl)
{
	if (ttl > 128) {
		return 255 - ttl;
	} else if (ttl > 64) {
		return 128 - ttl;
	} else if (ttl > 32) {
		return 64 - ttl;
	} else {
		return 32 - ttl;
	}
}

static bool is_in_plat_subnet(struct in6_addr *addr6)
{
	/* Assumes a /96 plat subnet. */
	return (addr6 != NULL) &&
		(memcmp(addr6->s6_addr, &klat_obj.plat_subnet, 12) == 0);
}

static struct in6_addr ipv4_daddr_to_ipv6_daddr(uint32_t addr4)
{
	struct in6_addr addr6;

	/* Assumes a /96 plat subnet. */
	addr6 = klat_obj.plat_subnet;
	addr6.s6_addr32[3] = addr4;
	return addr6;
}

static uint32_t ipv6_saddr_to_ipv4_saddr(struct in6_addr *addr6)
{
	if (is_in_plat_subnet(addr6)) {
		/* Assumes a /96 plat subnet. */
		return addr6->s6_addr32[3];
	}

	return INADDR_NONE;
}

static void fill_ip_header(struct iphdr *ip, uint16_t payload_len,
				uint8_t protocol, struct ipv6hdr *old_header,
				int ndev_index)
{
	int ttl_guess;

	memset(ip, 0, sizeof(struct iphdr));

	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = htons(sizeof(struct iphdr) + payload_len);
	ip->id = 0;
	ip->frag_off = htons(IP_DF);
	ip->ttl = old_header->hop_limit;
	ip->protocol = protocol;
	ip->check = 0;

	ip->saddr = ipv6_saddr_to_ipv4_saddr(&old_header->saddr);
	ip->daddr = klat_obj.xlat_v4_addrs[ndev_index].s_addr;

	if ((uint32_t)ip->saddr == INADDR_NONE) {
		ttl_guess = icmp_guess_ttl(old_header->hop_limit);
		ip->saddr = htonl((0xff << 24) + ttl_guess);
	}
}

static void fill_ip6_header(struct ipv6hdr *ip6, uint16_t payload_len,
				uint8_t protocol,
				const struct iphdr *old_header, int ndev_index)
{
	memset(ip6, 0, sizeof(struct ipv6hdr));

	ip6->version = 6;
	ip6->payload_len = htons(payload_len);
	ip6->nexthdr = protocol;
	ip6->hop_limit = old_header->ttl;

	ip6->saddr = klat_obj.xlat_addrs[ndev_index];
	ip6->daddr = ipv4_daddr_to_ipv6_daddr(old_header->daddr);
}

static size_t maybe_fill_frag_header(struct frag_hdr *frag_hdr,
					struct ipv6hdr *ip6_targ,
					const struct iphdr *old_header)
{
	uint16_t frag_flags = ntohs(old_header->frag_off);
	uint16_t frag_off = frag_flags & IP_OFFSET;

	if (frag_off == 0 && (frag_flags & IP_MF) == 0) {
		// Not a fragment.
		return 0;
	}

	frag_hdr->nexthdr = ip6_targ->nexthdr;
	frag_hdr->reserved = 0;

	frag_hdr->frag_off = htons(frag_off << 3);
	if (frag_flags & IP_MF)
		frag_hdr->frag_off |= IP6F_MORE_FRAG;

	frag_hdr->identification = htonl(ntohs(old_header->id));
	ip6_targ->nexthdr = IPPROTO_FRAGMENT;

	return sizeof(*frag_hdr);
}

static int ipv4_packet(struct sk_buff *skb, int ndev_index)
{
	struct iphdr *header = (struct iphdr *)skb->data;
	struct ipv6hdr ip6_targ;
	struct frag_hdr frag_hdr;
	size_t frag_hdr_len;
	uint8_t nxthdr;
	uint8_t *next_header;
	uint8_t *p_curr;
	size_t len_left;
	uint32_t old_sum, new_sum;
	int iov_len = 0;
	int len = skb->len;

	if (len < sizeof(struct iphdr)) {
		mif_err("too short for an ip header\n");
		return 0;
	}

	if (header->ihl < 5) {
		mif_err("ip header length is less than 5: %x\n", header->ihl);
		return 0;
	}

	if (header->ihl * 4 > len) {
		mif_err("ip header length set too large: %x\n", header->ihl);
		return 0;
	}

	if (header->version != 4) {
		mif_err("ip header version not 4: %x\n", header->version);
		return 0;
	}

	next_header = skb->data + header->ihl*4;
	len_left = len - header->ihl * 4;

	nxthdr = header->protocol;
	if (nxthdr == IPPROTO_ICMP)
		nxthdr = IPPROTO_ICMPV6;

	fill_ip6_header(&ip6_targ, 0, nxthdr, header, ndev_index);

	old_sum = ipv4_pseudo_header_checksum(header, len_left);
	new_sum = ipv6_pseudo_header_checksum(&ip6_targ, len_left, nxthdr);

	frag_hdr_len = maybe_fill_frag_header(&frag_hdr, &ip6_targ, header);

	if (frag_hdr_len && frag_hdr.frag_off & IP6F_OFF_MASK) {
		//iov_len = 1;
	} else if (nxthdr == IPPROTO_ICMPV6) {
		// todo, iov_len = icmp_packet(skb, (struct icmphdr *)next_header, new_sum, len_left);
	} else if (nxthdr == IPPROTO_TCP) {
		iov_len = tcp_packet((struct tcphdr *)next_header, old_sum,
					new_sum, len_left);
	} else if (nxthdr == IPPROTO_UDP) {
		iov_len = udp_packet((struct udphdr *)next_header, old_sum,
					new_sum, len_left);
	} else {
#ifdef KLAT_DEBUG
		mif_err("unknown protocol: %x\n", header->protocol);
		mif_err("protocol: %*ph\n", skb->data, min(skb->len, 48));
#endif
		return 0;
	}

	if (iov_len) {
		/* Set the length.*/
		ip6_targ.payload_len = htons(len_left);

		/* copy ip_targ to skb */
		p_curr = next_header - (frag_hdr_len + sizeof(struct ipv6hdr));
		memcpy(p_curr, &ip6_targ, sizeof(struct ipv6hdr));
		if (frag_hdr_len) {
			p_curr += sizeof(struct ipv6hdr);
			memcpy(p_curr, &frag_hdr, frag_hdr_len);
		}

		skb_push(skb, frag_hdr_len + sizeof(struct ipv6hdr)
				- sizeof(struct iphdr));
	}
	return iov_len;
}

static int ipv6_packet(struct sk_buff *skb, int ndev_index)
{
	struct ipv6hdr *ip6 = (struct ipv6hdr *)skb->data;
	struct iphdr ip_targ;
	struct frag_hdr *frag_hdr = NULL;
	uint8_t protocol;
	unsigned char *next_header;
	size_t len_left;
	uint32_t old_sum, new_sum;
	struct in6_addr	*xlat_addr = &klat_obj.xlat_addrs[ndev_index];
	int iov_len = 0;

	if (skb->len < sizeof(struct ipv6hdr)) {
		mif_info("too short for an ip6 header: %d\n", skb->len);
		return 0;
	}

	if (ipv6_addr_is_multicast(&ip6->daddr)) {
		mif_err("multicast %pI6->%pI6\n", &ip6->saddr, &ip6->daddr);
		return 0;
	}

	if (!(is_in_plat_subnet(&ip6->saddr) &&
		IN6_ARE_ADDR_EQUAL(&ip6->daddr, xlat_addr)) &&
		!(is_in_plat_subnet(&ip6->daddr) &&
		IN6_ARE_ADDR_EQUAL(&ip6->saddr, xlat_addr)) &&
		ip6->nexthdr != NEXTHDR_ICMP) {
		mif_err("wrong saddr: %pI6->%pI6\n", &ip6->saddr, &ip6->daddr);
		return 0;
	}

	next_header = skb->data + sizeof(struct ipv6hdr);
	len_left = skb->len - sizeof(struct ipv6hdr);

	protocol = ip6->nexthdr;

	fill_ip_header(&ip_targ, 0, protocol, ip6, ndev_index);

	if (protocol == IPPROTO_FRAGMENT) {
		frag_hdr = (struct frag_hdr *)next_header;
		if (len_left < sizeof(*frag_hdr)) {
			mif_err("short for fragment header: %d\n", skb->len);
			return 0;
		}

		next_header += sizeof(*frag_hdr);
		len_left -= sizeof(*frag_hdr);

		protocol = parse_frag_header(frag_hdr, &ip_targ);
	}

	if (protocol == IPPROTO_ICMPV6) {
		protocol = IPPROTO_ICMP;
		ip_targ.protocol = IPPROTO_ICMP;
	}

	old_sum = ipv6_pseudo_header_checksum(ip6, len_left, protocol);
	new_sum = ipv4_pseudo_header_checksum(&ip_targ, len_left);

	/* Does not support IPv6 extension headers except Fragment. */
	if (frag_hdr && (frag_hdr->frag_off & IP6F_OFF_MASK)) {
		iov_len = 1;
	} else if (protocol == IPPROTO_ICMP) {
		// todo, iov_len = icmp6_packet(skb, (struct icmp6hdr *)next_header, len_left);
	} else if (protocol == IPPROTO_TCP) {
		iov_len = tcp_packet((struct tcphdr *)next_header, old_sum,
					new_sum, len_left);
	} else if (protocol == IPPROTO_UDP) {
		iov_len = udp_packet((struct udphdr *)next_header, old_sum,
					new_sum, len_left);
	} else {
#ifdef KLAT_DEBUG
		mif_err("unknown next header type: %x\n", ip6->nexthdr);
		mif_err("nxthdr: %*ph\n", skb->data, min(skb->len, 48));
#endif
		return 0;
	}

	if (iov_len) {
		/* Set the length and calculate the checksum. */
		ip_targ.tot_len = htons(len_left + sizeof(struct iphdr));
		ip_targ.check = ip_checksum(&ip_targ, sizeof(struct iphdr));

		/* copy ip_targ to skb */
		memcpy(next_header - sizeof(struct iphdr), &ip_targ,
			sizeof(struct iphdr));
		skb_pull(skb, next_header - skb->data - sizeof(struct iphdr));
	}
	return iov_len;
}

int klat_rx(struct sk_buff *skb, int ndev_index)
{
	if (klat_obj.use[ndev_index] && skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6hdr = (struct ipv6hdr *)skb->data;

		if (ipv6_addr_equal(&ip6hdr->daddr,
					&klat_obj.xlat_addrs[ndev_index])) {
			if (ipv6_packet(skb, ndev_index) > 0) {
				skb->protocol = htons(ETH_P_IP);
				skb->dev = klat_obj.tun_device[ndev_index];
				return 1;
			}
		}
	}

	return 0;
}

int klat_tx(struct sk_buff *skb, int ndev_index)
{
	if (klat_obj.use[ndev_index] && skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iphdr = (struct iphdr *)skb->data;

		if (iphdr->saddr == klat_obj.xlat_v4_addrs[ndev_index].s_addr) {
			if (ipv4_packet(skb, ndev_index) > 0) {
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				skb->protocol = htons(ETH_P_IPV6);

				return 1;
			}
		}
	}

	return 0;
}

static struct net_device *klat_dev_get_by_name(const char *devname)
{
	struct net_device *dev = NULL;

	rcu_read_lock();
	dev = dev_get_by_name_rcu(&init_net, devname);
	rcu_read_unlock();

	return dev;
}

static int get_rmnet_index(const char *buf)
{
	if (buf && *buf != '\0' && strncmp(buf, "rmnet", 5) == 0 &&
		*(buf + 5) != '\0') {
		int rmnet_idx = *(buf + 5) - '0';

		if (rmnet_idx < KLAT_MAX_NDEV)
			return rmnet_idx;
	}

	return -1;
}

static int klat_netdev_event(struct notifier_block *this,
					  unsigned long event, void *ptr)
{
	struct net_device *net = netdev_notifier_info_to_dev(ptr);
	char *buf = strstr(net->name, "v4-rmnet");
	int ndev_idx;

	if (!buf)
		return NOTIFY_DONE;

	ndev_idx = get_rmnet_index(buf + 3);
	if (ndev_idx < 0)
		return NOTIFY_DONE;

	switch (event) {
	case NETDEV_DOWN:
	case NETDEV_UNREGISTER:
		klat_obj.use[ndev_idx] = 0;
		mif_info("klat disabled(%s)\n", net->name);
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block klat_netdev_notifier = {
	.notifier_call	= klat_netdev_event,
};

ssize_t klat_plat_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	struct in6_addr val;
	char *ptr = NULL;
	int rmnet_idx = 0;

	mif_err("plat prefix: %s\n", buf);

	ptr = strstr(buf, "@");
	if (!ptr)
		return -EINVAL;
	*ptr++ = '\0';

	if (in6_pton(buf, strlen(buf), val.s6_addr, '\0', NULL) == 0)
		return -EINVAL;

	rmnet_idx = get_rmnet_index(ptr);
	if (rmnet_idx >= 0) {
		klat_obj.plat_subnet = val;
		klat_obj.use[rmnet_idx] = 1;

		mif_err("plat prefix: %pI6, klat(%d) enabled\n",
				&klat_obj.plat_subnet,
				rmnet_idx);
	} else {
		mif_err("plat prefix: %pI6, failed to enable klat\n",
				&klat_obj.plat_subnet);
	}

	return count;
}

ssize_t klat_addrs_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	struct in6_addr val;
	char *ptr = NULL;

	mif_err("-- v6 addr: %s\n", buf);

	ptr = strstr(buf, "@");
	if (!ptr)
		return -EINVAL;
	*ptr++ = '\0';

	if (in6_pton(buf, strlen(buf), val.s6_addr, '\0', NULL) == 0)
		return -EINVAL;

	if (strstr(ptr, "rmnet0")) {
		klat_obj.xlat_addrs[0] = val;
		klat_obj.tun_device[0] = klat_dev_get_by_name("v4-rmnet0");
		mif_err("rmnet0: %pI6\n", &klat_obj.xlat_addrs[0]);
	} else if (strstr(ptr, "rmnet1")) {
		klat_obj.xlat_addrs[1] = val;
		klat_obj.tun_device[1] = klat_dev_get_by_name("v4-rmnet1");
		mif_err("rmnet1: %pI6\n", &klat_obj.xlat_addrs[1]);
	} else {
		mif_err("unhandled clat addr for device %s\n", ptr);
	}

	return count;
}

ssize_t klat_v4_addrs_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	struct in_addr val;
	char *ptr = NULL;

	mif_err("v4 addr: %s\n", buf);

	ptr = strstr(buf, "@");
	if (!ptr)
		return -EINVAL;
	*ptr++ = '\0';

	if (in4_pton(buf, strlen(buf), (u8 *)&val.s_addr, '\0', NULL) == 0)
		return -EINVAL;

	if (strstr(ptr, "rmnet0")) {
		klat_obj.xlat_v4_addrs[0].s_addr = val.s_addr;
		mif_err("v4_rmnet0: %pI4\n", &klat_obj.xlat_v4_addrs[0].s_addr);
	} else if (strstr(ptr, "rmnet1")) {
		klat_obj.xlat_v4_addrs[1].s_addr = val.s_addr;
		mif_err("v4_rmnet1: %pI4\n", &klat_obj.xlat_v4_addrs[1].s_addr);

	} else {
		mif_err("unhandled clat v4 addr for device %s\n", ptr);
	}

	return count;
}

#ifndef CONFIG_CP_DIT
static ssize_t klat_plat_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "plat prefix: %pI6\n", &klat_obj.plat_subnet);
}

static ssize_t klat_addrs_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%pI6\n%pI6\n",
			&klat_obj.xlat_addrs[0],
			&klat_obj.xlat_addrs[1]);
};

static ssize_t klat_v4_addrs_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%pI4\n%pI4\n",
			&klat_obj.xlat_v4_addrs[0],
			&klat_obj.xlat_v4_addrs[1]);
}

static struct kobject *clat_kobject;
static struct kobj_attribute xlat_plat_attribute = {
	.attr = {.name = "xlat_plat", .mode = 0660},
	.show = klat_plat_show,
	.store = klat_plat_store,
};
static struct kobj_attribute xlat_addrs_attribute = {
	.attr = {.name = "xlat_addrs", .mode = 0660},
	.show = klat_addrs_show,
	.store = klat_addrs_store,
};
static struct kobj_attribute xlat_v4_addrs_attribute = {
	.attr = {.name = "xlat_v4_addrs", .mode = 0660},
	.show = klat_v4_addrs_show,
	.store = klat_v4_addrs_store,
};
static struct attribute *clat_attrs[] = {
	&xlat_plat_attribute.attr,
	&xlat_addrs_attribute.attr,
	&xlat_v4_addrs_attribute.attr,
	NULL,
};
ATTRIBUTE_GROUPS(clat);
#endif

static int __init klat_init(void)
{
#ifndef CONFIG_CP_DIT
	clat_kobject = kobject_create_and_add(KOBJ_CLAT, kernel_kobj);
	if (!clat_kobject)
		mif_err("%s: done ---\n", KOBJ_CLAT);

	if (sysfs_create_groups(clat_kobject, clat_groups))
		mif_err("failed to create clat groups node\n");
#endif
	register_netdevice_notifier(&klat_netdev_notifier);

	return 0;
}

static void klat_exit(void)
{
	unregister_netdevice_notifier(&klat_netdev_notifier);

#ifndef CONFIG_CP_DIT
	if (clat_kobject)
		kobject_put(clat_kobject);
#endif
}

module_init(klat_init);
module_exit(klat_exit);

MODULE_LICENSE("GPL and additional rights");
MODULE_DESCRIPTION("SAMSUNG klat module");
