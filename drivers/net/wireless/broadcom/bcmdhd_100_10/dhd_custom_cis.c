/*
 * Process CIS information from OTP for customer platform
 * (Handle the MAC address and module information)
 *
 * Copyright (C) 1999-2019, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: dhd_custom_cis.c 794072 2018-12-12 02:22:10Z $
 */

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>

#include <ethernet.h>
#include <dngl_stats.h>
#include <bcmutils.h>
#include <dhd.h>
#include <dhd_dbg.h>
#include <dhd_linux.h>
#include <bcmdevs.h>

#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/list.h>

#ifdef DHD_USE_CISINFO

/* File Location to keep each information */
#define MACINFO "/data/.mac.info"
#define MACINFO_EFS "/efs/wifi/.mac.info"
#define CIDINFO PLATFORM_PATH".cid.info"
#define CIDINFO_DATA "/data/.cid.info"

/* Definitions for MAC address */
#define MAC_BUF_SIZE 20
#define MAC_CUSTOM_FORMAT	"%02X:%02X:%02X:%02X:%02X:%02X"

/* Definitions for CIS information */
#if defined(BCM4359_CHIP)
#define CIS_BUF_SIZE            1280
#elif defined(BCM4361_CHIP) || defined(BCM4375_CHIP)
#define CIS_BUF_SIZE            1280
#else
#define CIS_BUF_SIZE            512
#endif /* BCM4359_CHIP */

#define CIS_TUPLE_TAG_START		0x80
#define CIS_TUPLE_TAG_VENDOR		0x81
#define CIS_TUPLE_TAG_MACADDR		0x19
#define CIS_TUPLE_TAG_BOARDTYPE		0x1b
#define CIS_TUPLE_LEN_MACADDR		7
#define CIS_DUMP_END                    0xff
#define CIS_TUPLE_NULL                  0X00

#ifdef CONFIG_BCMDHD_PCIE
#if defined(BCM4361_CHIP) || defined(BCM4375_CHIP)
#define OTP_OFFSET 208
#else
#define OTP_OFFSET 128
#endif /* BCM4361_CHIP || BCM4375_CHIP */
#else /* CONFIG_BCMDHD_PCIE */
#define OTP_OFFSET 12 /* SDIO */
#endif /* CONFIG_BCMDHD_PCIE */

unsigned char *g_cis_buf = NULL;
unsigned char g_have_cis_dump = FALSE;

/* Definitions for common interface */
typedef struct tuple_entry {
	struct list_head list;	/* head of the list */
	uint32 cis_idx;		/* index of each tuples */
} tuple_entry_t;

extern int _dhd_set_mac_address(struct dhd_info *dhd, int ifidx, struct ether_addr *addr);
#if defined(GET_MAC_FROM_OTP) || defined(USE_CID_CHECK)
static tuple_entry_t *dhd_alloc_tuple_entry(dhd_pub_t *dhdp, const int idx);
static void dhd_free_tuple_entry(dhd_pub_t *dhdp, struct list_head *head);
static int dhd_find_tuple_list_from_otp(dhd_pub_t *dhdp, int req_tup,
	unsigned char* req_tup_len, struct list_head *head);
#endif /* GET_MAC_FROM_OTP || USE_CID_CHECK */

/* Common Interface Functions */
void
dhd_clear_cis(dhd_pub_t *dhdp)
{
	if (g_cis_buf) {
		MFREE(dhdp->osh, g_cis_buf, CIS_BUF_SIZE);
		g_cis_buf = NULL;
		DHD_ERROR(("%s: Local CIS buffer is freed\n", __FUNCTION__));
	}

	g_have_cis_dump = FALSE;
}

int
dhd_read_cis(dhd_pub_t *dhdp)
{
	int ret = 0;
	cis_rw_t *cish;
	int buf_size = CIS_BUF_SIZE;
	int length = strlen("cisdump");

	if (length >= buf_size) {
		DHD_ERROR(("%s: check CIS_BUF_SIZE\n", __FUNCTION__));
		return BCME_BADLEN;
	}

	/* Try reading out from CIS */
	g_cis_buf = MALLOCZ(dhdp->osh, buf_size);
	if (!g_cis_buf) {
		DHD_ERROR(("%s: Failed to alloc buffer for CIS\n", __FUNCTION__));
		g_have_cis_dump = FALSE;
		return BCME_NOMEM;
	} else {
		DHD_ERROR(("%s: Local CIS buffer is alloced\n", __FUNCTION__));
		memset(g_cis_buf, 0, buf_size);
	}

	cish = (cis_rw_t *)(g_cis_buf + 8);
	cish->source = 0;
	cish->byteoff = 0;
	cish->nbytes = buf_size;
	strncpy(g_cis_buf, "cisdump", length);

	ret = dhd_wl_ioctl_cmd(dhdp, WLC_GET_VAR, g_cis_buf, buf_size, 0, 0);
	if (ret < 0) {
		/* free local buf */
		dhd_clear_cis(dhdp);
	} else {
		g_have_cis_dump = TRUE;
	}

	return ret;
}

#if defined(GET_MAC_FROM_OTP) || defined(USE_CID_CHECK)
static tuple_entry_t*
dhd_alloc_tuple_entry(dhd_pub_t *dhdp, const int idx)
{
	tuple_entry_t *entry;

	entry = MALLOCZ(dhdp->osh, sizeof(tuple_entry_t));
	if (!entry) {
		DHD_ERROR(("%s: failed to alloc entry\n", __FUNCTION__));
		return NULL;
	}

	entry->cis_idx = idx;

	return entry;
}

static void
dhd_free_tuple_entry(dhd_pub_t *dhdp, struct list_head *head)
{
	tuple_entry_t *entry;

	while (!list_empty(head)) {
		entry = list_entry(head->next, tuple_entry_t, list);
		list_del(&entry->list);

		MFREE(dhdp->osh, entry, sizeof(tuple_entry_t));
	}
}

static int
dhd_find_tuple_list_from_otp(dhd_pub_t *dhdp, int req_tup,
	unsigned char* req_tup_len, struct list_head *head)
{
	int idx = OTP_OFFSET + sizeof(cis_rw_t);
	int tup, tup_len = 0;
	int buf_len = CIS_BUF_SIZE;
	int found = 0;

	if (!g_have_cis_dump) {
		DHD_ERROR(("%s: Couldn't find cis info from"
			" local buffer\n", __FUNCTION__));
		return BCME_ERROR;
	}

	do {
		tup = g_cis_buf[idx++];
		if (tup == CIS_TUPLE_NULL || tup == CIS_DUMP_END) {
			tup_len = 0;
		} else {
			tup_len = g_cis_buf[idx++];
			if ((idx + tup_len) > buf_len) {
				return BCME_ERROR;
			}

			if (tup == CIS_TUPLE_TAG_START &&
				tup_len != CIS_TUPLE_NULL &&
				g_cis_buf[idx] == req_tup) {
				idx++;
				if (head) {
					tuple_entry_t *entry;
					entry = dhd_alloc_tuple_entry(dhdp, idx);
					if (entry) {
						list_add_tail(&entry->list, head);
						found++;
					}
				}
				if (found == 1 && req_tup_len) {
					*req_tup_len = tup_len;
				}
			}
		}
		idx += tup_len;
	} while (tup != CIS_DUMP_END && (idx < buf_len));

	return (found > 0) ? found : BCME_ERROR;
}
#endif /* GET_MAC_FROM_OTP || USE_CID_CHECK */

#ifdef DUMP_CIS
static void
dhd_dump_cis_buf(int size)
{
	int i;

	if (size <= 0) {
		return;
	}

	if (size > CIS_BUF_SIZE) {
		size = CIS_BUF_SIZE;
	}

	DHD_ERROR(("========== START CIS DUMP ==========\n"));
	for (i = 0; i < size; i++) {
		int cis_offset = OTP_OFFSET + sizeof(cis_rw_t);
		DHD_ERROR(("%02X ", g_cis_buf[i + cis_offset]));
		if ((i % 16) == 15) {
			DHD_ERROR(("\n"));
		}
	}
	if ((i % 16) != 15) {
		DHD_ERROR(("\n"));
	}
	DHD_ERROR(("========== END CIS DUMP ==========\n"));
}
#endif /* DUMP_CIS */

/* MAC address mangement functions */
#ifdef READ_MACADDR
static void
dhd_create_random_mac(char *buf, unsigned int buf_len)
{
	char random_mac[3];

	memset(random_mac, 0, sizeof(random_mac));
	get_random_bytes(random_mac, 3);

	snprintf(buf, buf_len, MAC_CUSTOM_FORMAT, 0x00, 0x12, 0x34,
		(uint32)random_mac[0], (uint32)random_mac[1], (uint32)random_mac[2]);

	DHD_ERROR(("%s: The Random Generated MAC ID: %s\n",
		__FUNCTION__, random_mac));
}

#ifndef DHD_MAC_ADDR_EXPORT
int
dhd_set_macaddr_from_file(dhd_pub_t *dhdp)
{
	char mac_buf[MAC_BUF_SIZE];
	char *filepath_efs = MACINFO_EFS;
#ifdef PLATFORM_SLP
	char *filepath_mac = MACINFO;
#endif /* PLATFORM_SLP */
	int ret;
	struct dhd_info *dhd;
	struct ether_addr *mac;
	char *invalid_mac = "00:00:00:00:00:00";

	if (dhdp) {
		dhd = dhdp->info;
		mac = &dhdp->mac;
	} else {
		DHD_ERROR(("%s: dhd is NULL\n", __FUNCTION__));
		return BCME_ERROR;
	}

	memset(mac_buf, 0, sizeof(mac_buf));

	/* Read MAC address from the specified file */
	ret = dhd_read_file(filepath_efs, mac_buf, sizeof(mac_buf) - 1);

	/* Check if the file does not exist or contains invalid data */
	if (ret || (!ret && strstr(mac_buf, invalid_mac))) {
		/* Generate a new random MAC address */
		dhd_create_random_mac(mac_buf, sizeof(mac_buf));

		/* Write random MAC address to the file */
		if (dhd_write_file(filepath_efs, mac_buf, strlen(mac_buf)) < 0) {
			DHD_ERROR(("%s: MAC address [%s] Failed to write into File:"
				" %s\n", __FUNCTION__, mac_buf, filepath_efs));
			return BCME_ERROR;
		} else {
			DHD_ERROR(("%s: MAC address [%s] written into File: %s\n",
				__FUNCTION__, mac_buf, filepath_efs));
		}
	}
#ifdef PLATFORM_SLP
	/* Write random MAC address for framework */
	if (dhd_write_file(filepath_mac, mac_buf, strlen(mac_buf)) < 0) {
		DHD_ERROR(("%s: MAC address [%s] Failed to write into File:"
			" %s\n", __FUNCTION__, mac_buf, filepath_mac));
	} else {
		DHD_ERROR(("%s: MAC address [%s] written into File: %s\n",
			__FUNCTION__, mac_buf, filepath_mac));
	}
#endif /* PLATFORM_SLP */

	mac_buf[sizeof(mac_buf) - 1] = '\0';

	/* Write the MAC address to the Dongle */
	sscanf(mac_buf, MAC_CUSTOM_FORMAT,
		(uint32 *)&(mac->octet[0]), (uint32 *)&(mac->octet[1]),
		(uint32 *)&(mac->octet[2]), (uint32 *)&(mac->octet[3]),
		(uint32 *)&(mac->octet[4]), (uint32 *)&(mac->octet[5]));

	if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
		DHD_INFO(("%s: MAC Address is overwritten\n", __FUNCTION__));
	} else {
		DHD_ERROR(("%s: _dhd_set_mac_address() failed\n", __FUNCTION__));
	}

	return 0;
}
#else
int
dhd_set_macaddr_from_file(dhd_pub_t *dhdp)
{
	char mac_buf[MAC_BUF_SIZE];

	struct dhd_info *dhd;
	struct ether_addr *mac;

	if (dhdp) {
		dhd = dhdp->info;
		mac = &dhdp->mac;
	} else {
		DHD_ERROR(("%s: dhd is NULL\n", __FUNCTION__));
		return BCME_ERROR;
	}

	memset(mac_buf, 0, sizeof(mac_buf));
	if (ETHER_ISNULLADDR(&sysfs_mac_addr)) {
		/* Generate a new random MAC address */
		dhd_create_random_mac(mac_buf, sizeof(mac_buf));
		if (!bcm_ether_atoe(mac_buf, &sysfs_mac_addr)) {
			DHD_ERROR(("%s : mac parsing err\n", __FUNCTION__));
			return BCME_ERROR;
		}
	}

	/* Write the MAC address to the Dongle */
	memcpy(mac, &sysfs_mac_addr, sizeof(sysfs_mac_addr));

	if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
		DHD_INFO(("%s: MAC Address is overwritten\n", __FUNCTION__));
	} else {
		DHD_ERROR(("%s: _dhd_set_mac_address() failed\n", __FUNCTION__));
	}

	return 0;
}
#endif /* !DHD_MAC_ADDR_EXPORT */
#endif /* READ_MACADDR */

#ifdef GET_MAC_FROM_OTP
static int
dhd_set_default_macaddr(dhd_pub_t *dhdp)
{
	char iovbuf[WLC_IOCTL_SMLEN];
	struct ether_addr *mac;
	int ret;

	if (!dhdp) {
		DHD_ERROR(("%s: dhdp is NULL\n", __FUNCTION__));
		return BCME_BADARG;
	}

	mac = &dhdp->mac;

	/* Read the default MAC address */
	ret = dhd_iovar(dhdp, 0, "cur_etheraddr", NULL, 0, iovbuf, sizeof(iovbuf),
			FALSE);
	if (ret < 0) {
		DHD_ERROR(("%s: Can't get the default MAC address\n", __FUNCTION__));
		return BCME_NOTUP;
	}

	/* Update the default MAC address */
	memcpy(mac, iovbuf, ETHER_ADDR_LEN);
#ifdef DHD_MAC_ADDR_EXPORT
	memcpy(&sysfs_mac_addr, mac, sizeof(sysfs_mac_addr));
#endif /* DHD_MAC_ADDR_EXPORT */

	return 0;
}

static int
dhd_verify_macaddr(dhd_pub_t *dhdp, struct list_head *head)
{
	tuple_entry_t *cur, *next;
	int idx = -1; /* Invalid index */

	list_for_each_entry(cur, head, list) {
		list_for_each_entry(next, &cur->list, list) {
			if (!memcmp(&g_cis_buf[cur->cis_idx],
				&g_cis_buf[next->cis_idx], ETHER_ADDR_LEN)) {
				idx = cur->cis_idx;
				break;
			}

			if (next->list.next == head) {
				break;
			}
		}
	}

	return idx;
}

int
dhd_check_module_mac(dhd_pub_t *dhdp)
{
#ifndef DHD_MAC_ADDR_EXPORT
	char *filepath_efs = MACINFO_EFS;
#endif /* !DHD_MAC_ADDR_EXPORT */
	unsigned char otp_mac_buf[MAC_BUF_SIZE];
	struct ether_addr *mac;
	struct dhd_info *dhd;

	if (!dhdp) {
		DHD_ERROR(("%s: dhdp is NULL\n", __FUNCTION__));
		return BCME_BADARG;
	}

	dhd = dhdp->info;
	if (!dhd) {
		DHD_ERROR(("%s: dhd is NULL\n", __FUNCTION__));
		return BCME_BADARG;
	}

	mac = &dhdp->mac;
	memset(otp_mac_buf, 0, sizeof(otp_mac_buf));

	if (!g_have_cis_dump) {
#ifndef DHD_MAC_ADDR_EXPORT
		char eabuf[ETHER_ADDR_STR_LEN];
		DHD_INFO(("%s: Couldn't read CIS information\n", __FUNCTION__));

		/* Read the MAC address from the specified file */
		if (dhd_read_file(filepath_efs, otp_mac_buf, sizeof(otp_mac_buf) - 1) < 0) {
			DHD_ERROR(("%s: Couldn't read the file, "
				"use the default MAC Address\n", __FUNCTION__));
			if (dhd_set_default_macaddr(dhdp) < 0) {
				return BCME_BADARG;
			}
		} else {
			bzero((char *)eabuf, ETHER_ADDR_STR_LEN);
			strncpy(eabuf, otp_mac_buf, ETHER_ADDR_STR_LEN - 1);
			if (!bcm_ether_atoe(eabuf, mac)) {
				DHD_ERROR(("%s : mac parsing err\n", __FUNCTION__));
				if (dhd_set_default_macaddr(dhdp) < 0) {
					return BCME_BADARG;
				}
			}
		}
#else
		DHD_INFO(("%s: Couldn't read CIS information\n", __FUNCTION__));

		/* Read the MAC address from the specified file */
		if (ETHER_ISNULLADDR(&sysfs_mac_addr)) {
			DHD_ERROR(("%s: Couldn't read the file, "
				"use the default MAC Address\n", __FUNCTION__));
			if (dhd_set_default_macaddr(dhdp) < 0) {
				return BCME_BADARG;
			}
		} else {
			/* sysfs mac addr is confirmed with valid format in set_mac_addr */
			memcpy(mac, &sysfs_mac_addr, sizeof(sysfs_mac_addr));
		}
#endif /* !DHD_MAC_ADDR_EXPORT */
	} else {
		struct list_head mac_list;
		unsigned char tuple_len = 0;
		int found = 0;
		int idx = -1; /* Invalid index */

#ifdef DUMP_CIS
		dhd_dump_cis_buf(48);
#endif /* DUMP_CIS */

		/* Find a new tuple tag */
		INIT_LIST_HEAD(&mac_list);
		found = dhd_find_tuple_list_from_otp(dhdp, CIS_TUPLE_TAG_MACADDR,
			&tuple_len, &mac_list);
		if ((found > 0) && tuple_len == CIS_TUPLE_LEN_MACADDR) {
			if (found == 1) {
				tuple_entry_t *cur = list_entry((&mac_list)->next,
					tuple_entry_t, list);
				idx = cur->cis_idx;
			} else {
				/* Find the start index of MAC address */
				idx = dhd_verify_macaddr(dhdp, &mac_list);
			}
		}

		/* Find the MAC address */
		if (idx > 0) {
			/* update MAC address */
			snprintf(otp_mac_buf, sizeof(otp_mac_buf), MAC_CUSTOM_FORMAT,
				(uint32)g_cis_buf[idx], (uint32)g_cis_buf[idx + 1],
				(uint32)g_cis_buf[idx + 2], (uint32)g_cis_buf[idx + 3],
				(uint32)g_cis_buf[idx + 4], (uint32)g_cis_buf[idx + 5]);
			DHD_ERROR(("%s: MAC address is taken from OTP: " MACDBG "\n",
				__FUNCTION__, MAC2STRDBG(&g_cis_buf[idx])));
		} else {
			/* Not found MAC address info from the OTP, use the default value */
			if (dhd_set_default_macaddr(dhdp) < 0) {
				dhd_free_tuple_entry(dhdp, &mac_list);
				return BCME_BADARG;
			}
			snprintf(otp_mac_buf, sizeof(otp_mac_buf), MAC_CUSTOM_FORMAT,
				(uint32)mac->octet[0], (uint32)mac->octet[1],
				(uint32)mac->octet[2], (uint32)mac->octet[3],
				(uint32)mac->octet[4], (uint32)mac->octet[5]);
			DHD_ERROR(("%s: Cannot find MAC address info from OTP,"
				" Check module mac by initial value: " MACDBG "\n",
				__FUNCTION__, MAC2STRDBG(mac->octet)));
		}

		dhd_free_tuple_entry(dhdp, &mac_list);
#ifndef DHD_MAC_ADDR_EXPORT
		dhd_write_file(filepath_efs, otp_mac_buf, strlen(otp_mac_buf));
#else
		/* Export otp_mac_buf to the sys/mac_addr */
		if (!bcm_ether_atoe(otp_mac_buf, &sysfs_mac_addr)) {
			DHD_ERROR(("%s : mac parsing err\n", __FUNCTION__));
			if (dhd_set_default_macaddr(dhdp) < 0) {
				return BCME_BADARG;
			}
		} else {
			DHD_INFO(("%s : set mac address properly\n", __FUNCTION__));
			/* set otp mac to sysfs */
			memcpy(mac, &sysfs_mac_addr, sizeof(sysfs_mac_addr));
		}
#endif /* !DHD_MAC_ADDR_EXPORT */
	}

	if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
		DHD_INFO(("%s: MAC Address is set\n", __FUNCTION__));
	} else {
		DHD_ERROR(("%s: Failed to set MAC address\n", __FUNCTION__));
	}

	return 0;
}
#endif /* GET_MAC_FROM_OTP */

#ifndef DHD_MAC_ADDR_EXPORT
#ifdef WRITE_MACADDR
int
dhd_write_macaddr(struct ether_addr *mac)
{
	char *filepath_data = MACINFO;
	char *filepath_efs = MACINFO_EFS;
	char mac_buf[MAC_BUF_SIZE];
	int ret = 0;
	int retry_cnt = 0;

	memset(mac_buf, 0, sizeof(mac_buf));
	snprintf(mac_buf, sizeof(mac_buf), MAC_CUSTOM_FORMAT,
		(uint32)mac->octet[0], (uint32)mac->octet[1],
		(uint32)mac->octet[2], (uint32)mac->octet[3],
		(uint32)mac->octet[4], (uint32)mac->octet[5]);

	if (filepath_data) {
		for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
			/* Write MAC information into /data/.mac.info */
			ret = dhd_write_file_and_check(filepath_data, mac_buf, strlen(mac_buf));
			if (!ret) {
				break;
			}
		}

		if (ret < 0) {
			DHD_ERROR(("%s: MAC address [%s] Failed to write into"
				" File: %s\n", __FUNCTION__, mac_buf, filepath_data));
			return BCME_ERROR;
		}
	} else {
		DHD_ERROR(("%s: filepath_data doesn't exist\n", __FUNCTION__));
	}

	if (filepath_efs) {
		for (retry_cnt = 0; retry_cnt < 3; retry_cnt++) {
			/* Write MAC information into /efs/wifi/.mac.info */
			ret = dhd_write_file_and_check(filepath_efs, mac_buf, strlen(mac_buf));
			if (!ret) {
				break;
			}
		}

		if (ret < 0) {
			DHD_ERROR(("%s: MAC address [%s] Failed to write into"
				" File: %s\n", __FUNCTION__, mac_buf, filepath_efs));
			return BCME_ERROR;
		}
	} else {
		DHD_ERROR(("%s: filepath_efs doesn't exist\n", __FUNCTION__));
	}

	return ret;
}
#endif /* WRITE_MACADDR */
#endif /* !DHD_MAC_ADDR_EXPORT */

#ifdef USE_CID_CHECK
/* Definitions for module information */
#define MAX_VID_LEN		8

#ifdef	SUPPORT_MULTIPLE_BOARDTYPE
#define MAX_BNAME_LEN		6

typedef struct {
	uint8 b_len;
	unsigned char btype[MAX_VID_LEN];
	char bname[MAX_BNAME_LEN];
} board_info_t;

#if defined(BCM4361_CHIP)
board_info_t semco_PA_info[] = {
	{ 3, { 0x0f, 0x08, }, { "_ePA" } },     /* semco All ePA */
	{ 3, { 0x27, 0x08, }, { "_iPA" } },     /* semco 2g iPA, 5g ePA */
	{ 3, { 0x1a, 0x08, }, { "_iPA" } },		/* semco 2g iPA, 5g ePA old */
	{ 0, { 0x00, }, { "" } }   /* Default: Not specified yet */
};
#else
board_info_t semco_board_info[] = {
	{ 3, { 0x51, 0x07, }, { "_b90b" } },     /* semco three antenna */
	{ 3, { 0x61, 0x07, }, { "_b90b" } },     /* semco two antenna */
	{ 0, { 0x00, }, { "" } }   /* Default: Not specified yet */
};
board_info_t murata_board_info[] = {
	{ 3, { 0xa5, 0x07, }, { "_b90" } },      /* murata three antenna */
	{ 3, { 0xb0, 0x07, }, { "_b90b" } },     /* murata two antenna */
	{ 3, { 0xb1, 0x07, }, { "_es5" } },     /* murata two antenna */
	{ 0, { 0x00, }, { "" } }   /* Default: Not specified yet */
};
#endif /* BCM4361_CHIP */
#endif /* SUPPORT_MULTIPLE_BOARDTYPE */

typedef struct {
	uint8 vid_length;
	unsigned char vid[MAX_VID_LEN];
	char cid_info[MAX_VNAME_LEN];
} vid_info_t;

#if defined(BCM4335_CHIP)
vid_info_t vid_info[] = {
	{ 3, { 0x33, 0x66, }, { "semcosh" } },		/* B0 Sharp 5G-FEM */
	{ 3, { 0x33, 0x33, }, { "semco" } },		/* B0 Skyworks 5G-FEM and A0 chip */
	{ 3, { 0x33, 0x88, }, { "semco3rd" } },		/* B0 Syri 5G-FEM */
	{ 3, { 0x00, 0x11, }, { "muratafem1" } },	/* B0 ANADIGICS 5G-FEM */
	{ 3, { 0x00, 0x22, }, { "muratafem2" } },	/* B0 TriQuint 5G-FEM */
	{ 3, { 0x00, 0x33, }, { "muratafem3" } },	/* 3rd FEM: Reserved */
	{ 0, { 0x00, }, { "murata" } }	/* Default: for Murata A0 module */
};
#elif defined(BCM4339_CHIP) || defined(BCM4354_CHIP) || \
	defined(BCM4356_CHIP)
vid_info_t vid_info[] = {			  /* 4339:2G FEM+5G FEM ,4354: 2G FEM+5G FEM */
	{ 3, { 0x33, 0x33, }, { "semco" } },      /* 4339:Skyworks+sharp,4354:Panasonic+Panasonic */
	{ 3, { 0x33, 0x66, }, { "semco" } },      /* 4339:  , 4354:Panasonic+SEMCO */
	{ 3, { 0x33, 0x88, }, { "semco3rd" } },   /* 4339:  , 4354:SEMCO+SEMCO */
	{ 3, { 0x90, 0x01, }, { "wisol" } },      /* 4339:  , 4354:Microsemi+Panasonic */
	{ 3, { 0x90, 0x02, }, { "wisolfem1" } },  /* 4339:  , 4354:Panasonic+Panasonic */
	{ 3, { 0x90, 0x03, }, { "wisolfem2" } },  /* 4354:Murata+Panasonic */
	{ 3, { 0x00, 0x11, }, { "muratafem1" } }, /* 4339:  , 4354:Murata+Anadigics */
	{ 3, { 0x00, 0x22, }, { "muratafem2"} },  /* 4339:  , 4354:Murata+Triquint */
	{ 0, { 0x00, }, { "samsung" } }           /* Default: Not specified yet */
};
#elif defined(BCM4358_CHIP)
vid_info_t vid_info[] = {
	{ 3, { 0x33, 0x33, }, { "semco_b85" } },
	{ 3, { 0x33, 0x66, }, { "semco_b85" } },
	{ 3, { 0x33, 0x88, }, { "semco3rd_b85" } },
	{ 3, { 0x90, 0x01, }, { "wisol_b85" } },
	{ 3, { 0x90, 0x02, }, { "wisolfem1_b85" } },
	{ 3, { 0x90, 0x03, }, { "wisolfem2_b85" } },
	{ 3, { 0x31, 0x90, }, { "wisol_b85b" } },
	{ 3, { 0x00, 0x11, }, { "murata_b85" } },
	{ 3, { 0x00, 0x22, }, { "murata_b85"} },
	{ 6, { 0x00, 0xFF, 0xFF, 0x00, 0x00, }, { "murata_b85"} },
	{ 3, { 0x10, 0x33, }, { "semco_b85a" } },
	{ 3, { 0x30, 0x33, }, { "semco_b85b" } },
	{ 3, { 0x31, 0x33, }, { "semco_b85b" } },
	{ 3, { 0x10, 0x22, }, { "murata_b85a" } },
	{ 3, { 0x20, 0x22, }, { "murata_b85a" } },
	{ 3, { 0x21, 0x22, }, { "murata_b85a" } },
	{ 3, { 0x23, 0x22, }, { "murata_b85a" } },
	{ 3, { 0x31, 0x22, }, { "murata_b85b" } },
	{ 0, { 0x00, }, { "samsung" } }           /* Default: Not specified yet */
};
#elif defined(BCM4359_CHIP)
vid_info_t vid_info[] = {
#if defined(SUPPORT_BCM4359_MIXED_MODULES)
	{ 3, { 0x34, 0x33, }, { "semco_b90b" } },
	{ 3, { 0x40, 0x33, }, { "semco_b90b" } },
	{ 3, { 0x41, 0x33, }, { "semco_b90b" } },
	{ 3, { 0x11, 0x33, }, { "semco_b90b" } },
	{ 3, { 0x33, 0x66, }, { "semco_b90b" } },
	{ 3, { 0x23, 0x22, }, { "murata_b90b" } },
	{ 3, { 0x40, 0x22, }, { "murata_b90b" } },
	{ 3, { 0x10, 0x90, }, { "wisol_b90b" } },
	{ 3, { 0x33, 0x33, }, { "semco_b90s_b1" } },
	{ 3, { 0x66, 0x33, }, { "semco_b90s_c0" } },
	{ 3, { 0x60, 0x22, }, { "murata_b90s_b1" } },
	{ 3, { 0x61, 0x22, }, { "murata_b90s_b1" } },
	{ 3, { 0x62, 0x22, }, { "murata_b90s_b1" } },
	{ 3, { 0x63, 0x22, }, { "murata_b90s_b1" } },
	{ 3, { 0x70, 0x22, }, { "murata_b90s_c0" } },
	{ 3, { 0x71, 0x22, }, { "murata_b90s_c0" } },
	{ 3, { 0x72, 0x22, }, { "murata_b90s_c0" } },
	{ 3, { 0x73, 0x22, }, { "murata_b90s_c0" } },
	{ 0, { 0x00, }, { "samsung" } }           /* Default: Not specified yet */
#else /* SUPPORT_BCM4359_MIXED_MODULES */
	{ 3, { 0x34, 0x33, }, { "semco" } },
	{ 3, { 0x40, 0x33, }, { "semco" } },
	{ 3, { 0x41, 0x33, }, { "semco" } },
	{ 3, { 0x11, 0x33, }, { "semco" } },
	{ 3, { 0x33, 0x66, }, { "semco" } },
	{ 3, { 0x23, 0x22, }, { "murata" } },
	{ 3, { 0x40, 0x22, }, { "murata" } },
	{ 3, { 0x51, 0x22, }, { "murata" } },
	{ 3, { 0x52, 0x22, }, { "murata" } },
	{ 3, { 0x10, 0x90, }, { "wisol" } },
	{ 0, { 0x00, }, { "samsung" } }           /* Default: Not specified yet */
#endif /* SUPPORT_BCM4359_MIXED_MODULES */
};
#elif defined(BCM4361_CHIP)
vid_info_t vid_info[] = {
#if defined(SUPPORT_BCM4361_MIXED_MODULES)
	{ 3, { 0x66, 0x33, }, { "semco_sky_r00a_e000_a0" } },
	{ 3, { 0x30, 0x33, }, { "semco_sky_r01a_e30a_a1" } },
	{ 3, { 0x31, 0x33, }, { "semco_sky_r02a_e30a_a1" } },
	{ 3, { 0x32, 0x33, }, { "semco_sky_r02a_e30a_a1" } },
	{ 3, { 0x51, 0x33, }, { "semco_sky_r01d_e31_b0" } },
	{ 3, { 0x61, 0x33, }, { "semco_sem_r01f_e31_b0" } },
	{ 3, { 0x62, 0x33, }, { "semco_sem_r02g_e31_b0" } },
	{ 3, { 0x71, 0x33, }, { "semco_sky_r01h_e32_b0" } },
	{ 3, { 0x81, 0x33, }, { "semco_sem_r01i_e32_b0" } },
	{ 3, { 0x82, 0x33, }, { "semco_sem_r02j_e32_b0" } },
	{ 3, { 0x91, 0x33, }, { "semco_sem_r02a_e32a_b2" } },
	{ 3, { 0xa1, 0x33, }, { "semco_sem_r02b_e32a_b2" } },
	{ 3, { 0x12, 0x22, }, { "murata_nxp_r012_1kl_a1" } },
	{ 3, { 0x13, 0x22, }, { "murata_mur_r013_1kl_b0" } },
	{ 3, { 0x14, 0x22, }, { "murata_mur_r014_1kl_b0" } },
	{ 3, { 0x15, 0x22, }, { "murata_mur_r015_1kl_b0" } },
	{ 3, { 0x20, 0x22, }, { "murata_mur_r020_1kl_b0" } },
	{ 3, { 0x21, 0x22, }, { "murata_mur_r021_1kl_b0" } },
	{ 3, { 0x22, 0x22, }, { "murata_mur_r022_1kl_b0" } },
	{ 3, { 0x23, 0x22, }, { "murata_mur_r023_1kl_b0" } },
	{ 3, { 0x24, 0x22, }, { "murata_mur_r024_1kl_b0" } },
	{ 3, { 0x30, 0x22, }, { "murata_mur_r030_1kl_b0" } },
	{ 3, { 0x31, 0x22, }, { "murata_mur_r031_1kl_b0" } },
	{ 3, { 0x32, 0x22, }, { "murata_mur_r032_1kl_b0" } },
	{ 3, { 0x33, 0x22, }, { "murata_mur_r033_1kl_b0" } },
	{ 3, { 0x34, 0x22, }, { "murata_mur_r034_1kl_b0" } },
	{ 3, { 0x50, 0x22, }, { "murata_mur_r020_1qw_b2" } },
	{ 3, { 0x51, 0x22, }, { "murata_mur_r021_1qw_b2" } },
	{ 3, { 0x52, 0x22, }, { "murata_mur_r022_1qw_b2" } },
	{ 3, { 0x61, 0x22, }, { "murata_mur_r031_1qw_b2" } },
	{ 3, { 0x62, 0x22, }, { "murata_mur_r032_1qw_b2" } },
	{ 3, { 0x71, 0x22, }, { "murata_mur_r041_1qw_b2" } },
	{ 0, { 0x00, }, { "samsung" } }           /* Default: Not specified yet */
#endif /* SUPPORT_BCM4359_MIXED_MODULES */
};
#elif defined(BCM4375_CHIP)
vid_info_t vid_info[] = {
#if defined(SUPPORT_BCM4375_MIXED_MODULES)
	{ 3, { 0x11, 0x33, }, { "semco_sky_e41_es11" } },
	{ 3, { 0x33, 0x33, }, { "semco_sem_e43_es33" } },
	{ 3, { 0x34, 0x33, }, { "semco_sem_e43_es34" } },
	{ 3, { 0x35, 0x33, }, { "semco_sem_e43_es35" } },
	{ 3, { 0x36, 0x33, }, { "semco_sem_e43_es36" } },
	{ 3, { 0x41, 0x33, }, { "semco_sem_e43_cs41" } },
	{ 3, { 0x51, 0x33, }, { "semco_sem_e43_cs51" } },
	{ 3, { 0x53, 0x33, }, { "semco_sem_e43_cs53" } },
	{ 3, { 0x61, 0x33, }, { "semco_sky_e43_cs61" } },
	{ 3, { 0x10, 0x22, }, { "murata_mur_1rh_es10" } },
	{ 3, { 0x11, 0x22, }, { "murata_mur_1rh_es11" } },
	{ 3, { 0x12, 0x22, }, { "murata_mur_1rh_es12" } },
	{ 3, { 0x13, 0x22, }, { "murata_mur_1rh_es13" } },
	{ 3, { 0x20, 0x22, }, { "murata_mur_1rh_es20" } },
	{ 3, { 0x32, 0x22, }, { "murata_mur_1rh_es32" } },
	{ 3, { 0x41, 0x22, }, { "murata_mur_1rh_es41" } },
	{ 3, { 0x42, 0x22, }, { "murata_mur_1rh_es42" } },
	{ 3, { 0x43, 0x22, }, { "murata_mur_1rh_es43" } },
	{ 3, { 0x44, 0x22, }, { "murata_mur_1rh_es44" } }
#endif /* SUPPORT_BCM4375_MIXED_MODULES */
};
#else
vid_info_t vid_info[] = {
	{ 0, { 0x00, }, { "samsung" } }			/* Default: Not specified yet */
};
#endif /* BCM_CHIP_ID */

/* CID managment functions */
static int
dhd_find_tuple_idx_from_otp(dhd_pub_t *dhdp, int req_tup, unsigned char *req_tup_len)
{
	struct list_head head;
	int start_idx;
	int entry_num;

	if (!g_have_cis_dump) {
		DHD_ERROR(("%s: Couldn't find cis info from"
			" local buffer\n", __FUNCTION__));
		return BCME_ERROR;
	}

	INIT_LIST_HEAD(&head);
	entry_num = dhd_find_tuple_list_from_otp(dhdp, req_tup, req_tup_len, &head);
	/* find the first cis index from the tuple list */
	if (entry_num > 0) {
		tuple_entry_t *cur = list_entry((&head)->next, tuple_entry_t, list);
		start_idx = cur->cis_idx;
	} else {
		start_idx = -1; /* Invalid index */
	}

	dhd_free_tuple_entry(dhdp, &head);

	return start_idx;
}

char *
dhd_get_cid_info(unsigned char *vid, int vid_length)
{
	int i;

	for (i = 0; i < ARRAYSIZE(vid_info); i++) {
		if (vid_info[i].vid_length-1 == vid_length &&
				!memcmp(vid_info[i].vid, vid, vid_length)) {
			return vid_info[i].cid_info;
		}
	}

	return NULL;
}

int
dhd_check_module_cid(dhd_pub_t *dhdp)
{
	int ret = -1;
#ifndef DHD_EXPORT_CNTL_FILE
	const char *cidfilepath = CIDINFO;
#endif /* DHD_EXPORT_CNTL_FILE */
	int idx, max;
	vid_info_t *cur_info;
	unsigned char *tuple_start = NULL;
	unsigned char tuple_length = 0;
	unsigned char cid_info[MAX_VNAME_LEN];
	int found = FALSE;
#ifdef SUPPORT_MULTIPLE_BOARDTYPE
	board_info_t *cur_b_info = NULL;
	board_info_t *vendor_b_info = NULL;
	unsigned char *btype_start;
	unsigned char boardtype_len = 0;
#endif /* SUPPORT_MULTIPLE_BOARDTYPE */

	/* Try reading out from CIS */
	if (!g_have_cis_dump) {
		DHD_INFO(("%s: Couldn't read CIS info\n", __FUNCTION__));
		return BCME_ERROR;
	}

	DHD_INFO(("%s: Reading CIS from local buffer\n", __FUNCTION__));
#ifdef DUMP_CIS
	dhd_dump_cis_buf(48);
#endif /* DUMP_CIS */

	idx = dhd_find_tuple_idx_from_otp(dhdp, CIS_TUPLE_TAG_VENDOR, &tuple_length);
	if (idx > 0) {
		found = TRUE;
		tuple_start = &g_cis_buf[idx];
	}

	if (found) {
		max = sizeof(vid_info) / sizeof(vid_info_t);
		for (idx = 0; idx < max; idx++) {
			cur_info = &vid_info[idx];
#ifdef BCM4358_CHIP
			if (cur_info->vid_length  == 6 && tuple_length == 6) {
				if (cur_info->vid[0] == tuple_start[0] &&
					cur_info->vid[3] == tuple_start[3] &&
					cur_info->vid[4] == tuple_start[4]) {
					goto check_board_type;
				}
			}
#endif /* BCM4358_CHIP */
			if ((cur_info->vid_length == tuple_length) &&
				(cur_info->vid_length != 0) &&
				(memcmp(cur_info->vid, tuple_start,
					cur_info->vid_length - 1) == 0)) {
				goto check_board_type;
			}
		}
	}

	/* find default nvram, if exist */
	DHD_ERROR(("%s: cannot find CIS TUPLE set as default\n", __FUNCTION__));
	max = sizeof(vid_info) / sizeof(vid_info_t);
	for (idx = 0; idx < max; idx++) {
		cur_info = &vid_info[idx];
		if (cur_info->vid_length == 0) {
			goto write_cid;
		}
	}
	DHD_ERROR(("%s: cannot find default CID\n", __FUNCTION__));
	return BCME_ERROR;

check_board_type:
#ifdef SUPPORT_MULTIPLE_BOARDTYPE
	idx = dhd_find_tuple_idx_from_otp(dhdp, CIS_TUPLE_TAG_BOARDTYPE, &tuple_length);
	if (idx > 0) {
		btype_start = &g_cis_buf[idx];
		boardtype_len = tuple_length;
		DHD_INFO(("%s: board type found.\n", __FUNCTION__));
	} else {
		boardtype_len = 0;
	}
#if defined(BCM4361_CHIP)
	vendor_b_info = semco_PA_info;
	max = sizeof(semco_PA_info) / sizeof(board_info_t);
#else
	if (strcmp(cur_info->cid_info, "semco") == 0) {
		vendor_b_info = semco_board_info;
		max = sizeof(semco_board_info) / sizeof(board_info_t);
	} else if (strcmp(cur_info->cid_info, "murata") == 0) {
		vendor_b_info = murata_board_info;
		max = sizeof(murata_board_info) / sizeof(board_info_t);
	} else {
		max = 0;
	}
#endif /* BCM4361_CHIP */
	if (boardtype_len) {
		for (idx = 0; idx < max; idx++) {
			cur_b_info = vendor_b_info;
			if ((cur_b_info->b_len == boardtype_len) &&
				(cur_b_info->b_len != 0) &&
				(memcmp(cur_b_info->btype, btype_start,
					cur_b_info->b_len - 1) == 0)) {
				DHD_INFO(("%s : board type name : %s\n",
					__FUNCTION__, cur_b_info->bname));
				break;
			}
			cur_b_info = NULL;
			vendor_b_info++;
		}
	}
#endif /* SUPPORT_MULTIPLE_BOARDTYPE */

write_cid:
#ifdef SUPPORT_MULTIPLE_BOARDTYPE
	if (cur_b_info && cur_b_info->b_len > 0) {
		strcpy(cid_info, cur_info->cid_info);
		strcpy(cid_info + strlen(cur_info->cid_info), cur_b_info->bname);
	} else
#endif /* SUPPORT_MULTIPLE_BOARDTYPE */
		strcpy(cid_info, cur_info->cid_info);

	DHD_INFO(("%s: CIS MATCH FOUND : %s\n", __FUNCTION__, cid_info));
#ifndef DHD_EXPORT_CNTL_FILE
	dhd_write_file(cidfilepath, cid_info, strlen(cid_info) + 1);
#else
	strlcpy(cidinfostr, cid_info, MAX_VNAME_LEN);
#endif /* DHD_EXPORT_CNTL_FILE */

	return ret;
}

#ifdef SUPPORT_MULTIPLE_MODULE_CIS
#ifndef DHD_EXPORT_CNTL_FILE
static bool
dhd_check_module(char *module_name)
{
	char vname[MAX_VNAME_LEN];
	const char *cidfilepath = CIDINFO;
	int ret;

	memset(vname, 0, sizeof(vname));
	ret = dhd_read_file(cidfilepath, vname, sizeof(vname) - 1);
	if (ret < 0) {
		return FALSE;
	}
	DHD_INFO(("%s: This module is %s \n", __FUNCTION__, vname));
	return strstr(vname, module_name) ? TRUE : FALSE;
}
#else
bool
dhd_check_module(char *module_name)
{
	return strstr(cidinfostr, module_name) ? TRUE : FALSE;
}
#endif /* !DHD_EXPORT_CNTL_FILE */

int
dhd_check_module_b85a(void)
{
	int ret;
	char *vname_b85a = "_b85a";

	if (dhd_check_module(vname_b85a)) {
		DHD_INFO(("%s: It's a b85a module\n", __FUNCTION__));
		ret = 1;
	} else {
		DHD_INFO(("%s: It is not a b85a module\n", __FUNCTION__));
		ret = -1;
	}

	return ret;
}

int
dhd_check_module_b90(void)
{
	int ret = 0;
	char *vname_b90b = "_b90b";
	char *vname_b90s = "_b90s";

	if (dhd_check_module(vname_b90b)) {
		DHD_INFO(("%s: It's a b90b module \n", __FUNCTION__));
		ret = BCM4359_MODULE_TYPE_B90B;
	} else if (dhd_check_module(vname_b90s)) {
		DHD_INFO(("%s: It's a b90s module\n", __FUNCTION__));
		ret = BCM4359_MODULE_TYPE_B90S;
	} else {
		DHD_ERROR(("%s: It's neither b90b nor b90s\n", __FUNCTION__));
		ret = BCME_ERROR;
	}

	return ret;
}
#endif /* SUPPORT_MULTIPLE_MODULE_CIS */

#define CID_FEM_MURATA	"_mur_"
/* extract module type from cid information */
int
dhd_check_module_bcm(char *module_type, int index, bool *is_murata_fem)
{
	int ret = 0, i;
	char vname[MAX_VNAME_LEN];
	char *ptr = NULL;
#ifndef DHD_EXPORT_CNTL_FILE
	const char *cidfilepath = CIDINFO;
#endif /* DHD_EXPORT_CNTL_FILE */

	memset(vname, 0, sizeof(vname));

#ifndef DHD_EXPORT_CNTL_FILE
	ret = dhd_read_file(cidfilepath, vname, sizeof(vname) - 1);
	if (ret < 0) {
		DHD_ERROR(("%s: failed to get module infomaion from .cid.info\n",
			__FUNCTION__));
		return ret;
	}
#else
	strlcpy(vname, cidinfostr, MAX_VNAME_LEN);
#endif /* DHD_EXPORT_CNTL_FILE */

	for (i = 1, ptr = vname; i < index && ptr; i++) {
		ptr = bcmstrstr(ptr, "_");
		if (ptr) {
			ptr++;
		}
	}

	if (bcmstrnstr(vname, MAX_VNAME_LEN, CID_FEM_MURATA, 5)) {
		*is_murata_fem = TRUE;
	}

	if (ptr) {
		memcpy(module_type, ptr, strlen(ptr));
	} else {
		DHD_ERROR(("%s: failed to get module infomaion\n", __FUNCTION__));
		return BCME_ERROR;
	}

	DHD_INFO(("%s: module type = %s \n", __FUNCTION__, module_type));

	return ret;
}
#endif /* USE_CID_CHECK */
#endif /* DHD_USE_CISINFO */
