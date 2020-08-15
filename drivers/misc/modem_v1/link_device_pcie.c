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

#include <linux/irq.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/suspend.h>
#include <linux/pm_qos.h>
#include <linux/reboot.h>
#if defined(CONFIG_SOC_EXYNOS3475)
#include <mach/smc.h>
#else
#include <linux/smc.h>
#endif
#include <linux/exynos-pci-ctrl.h>
#include <linux/modem_notifier.h>

#if defined(CONFIG_ECT)
#include <soc/samsung/ect_parser.h>
#endif
#include <linux/pci.h>
#include <linux/shm_ipc.h>
#include <linux/version.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_dump.h"
#include "link_device_memory.h"
#include "link_ctrlmsg_iosm.h"
#include "s5100_pcie.h"

extern int s5100_force_crash_exit_ext(void);

#if !defined(CONFIG_CP_SECURE_BOOT)
#define CRC32_XINIT 0xFFFFFFFFL		/* initial value */
#define CRC32_XOROT 0xFFFFFFFFL		/* final xor value */

static const unsigned long CRC32_TABLE[256] =
{
	0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL, 0x076DC419L,
	0x706AF48FL, 0xE963A535L, 0x9E6495A3L, 0x0EDB8832L, 0x79DCB8A4L,
	0xE0D5E91EL, 0x97D2D988L, 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L,
	0x90BF1D91L, 0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
	0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L, 0x136C9856L,
	0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL, 0x14015C4FL, 0x63066CD9L,
	0xFA0F3D63L, 0x8D080DF5L, 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L,
	0xA2677172L, 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
	0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L, 0x32D86CE3L,
	0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L, 0x26D930ACL, 0x51DE003AL,
	0xC8D75180L, 0xBFD06116L, 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L,
	0xB8BDA50FL, 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
	0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL, 0x76DC4190L,
	0x01DB7106L, 0x98D220BCL, 0xEFD5102AL, 0x71B18589L, 0x06B6B51FL,
	0x9FBFE4A5L, 0xE8B8D433L, 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL,
	0xE10E9818L, 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
	0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL, 0x6C0695EDL,
	0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L, 0x65B0D9C6L, 0x12B7E950L,
	0x8BBEB8EAL, 0xFCB9887CL, 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L,
	0xFBD44C65L, 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
	0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL, 0x4369E96AL,
	0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L, 0x44042D73L, 0x33031DE5L,
	0xAA0A4C5FL, 0xDD0D7CC9L, 0x5005713CL, 0x270241AAL, 0xBE0B1010L,
	0xC90C2086L, 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
	0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L, 0x59B33D17L,
	0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL, 0xEDB88320L, 0x9ABFB3B6L,
	0x03B6E20CL, 0x74B1D29AL, 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L,
	0x73DC1683L, 0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
	0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L, 0xF00F9344L,
	0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL, 0xF762575DL, 0x806567CBL,
	0x196C3671L, 0x6E6B06E7L, 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL,
	0x67DD4ACCL, 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
	0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L, 0xD1BB67F1L,
	0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL, 0xD80D2BDAL, 0xAF0A1B4CL,
	0x36034AF6L, 0x41047A60L, 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL,
	0x4669BE79L, 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
	0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL, 0xC5BA3BBEL,
	0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L, 0xC2D7FFA7L, 0xB5D0CF31L,
	0x2CD99E8BL, 0x5BDEAE1DL, 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL,
	0x026D930AL, 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
	0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L, 0x92D28E9BL,
	0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L, 0x86D3D2D4L, 0xF1D4E242L,
	0x68DDB3F8L, 0x1FDA836EL, 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L,
	0x18B74777L, 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
	0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L, 0xA00AE278L,
	0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L, 0xA7672661L, 0xD06016F7L,
	0x4969474DL, 0x3E6E77DBL, 0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L,
	0x37D83BF0L, 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
	0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L, 0xBAD03605L,
	0xCDD70693L, 0x54DE5729L, 0x23D967BFL, 0xB3667A2EL, 0xC4614AB8L,
	0x5D681B02L, 0x2A6F2B94L, 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL,
	0x2D02EF8DL
};
#endif

enum smc_error_flag {
	CP_NO_ERROR = 0,
	CP_NOT_ALIGN_64KB,
	CP_MEM_TOO_BIG,
	CP_FLAG_OUT_RANGE,
	CP_WRONG_TZASC_REGION_NUM,
	CP_WRONG_BL_SIZE = 5,
	CP_MEM_OUT_OF_RANGE,
	CP_NOT_ALIGN_16B,
	CP_MEM_IN_SECURE_DRAM,
	CP_ASP_ENABLE_FAIL,
	CP_ASP_DISABLE_FAIL = 10,
	CP_NOT_WORKING,
	CP_ALREADY_WORKING,
	CP_ALREADY_DUMP_MODE,
	CP_NOT_VALID_MAGIC,
	CP_SHOULD_BE_DISABLE = 15,
	CP_ALREADY_ENABLE_CPMEM_ON,
	CP_ALREADY_SET_WND,
	CP_FAIL_TO_SET_WND,
	CP_INVALID_CP_BASE,
	CP_CORRUPTED_CP_MEM_INFO = 20
};

char *smc_err_string[32] = {
	"CP_NO_ERROR",
	"CP_NOT_ALIGN_64KB",
	"CP_MEM_TOO_BIG",
	"CP_FLAG_OUT_RANGE",
	"CP_WRONG_TZASC_REGION_NUM",
	"CP_WRONG_BL_SIZE",
	"CP_MEM_OUT_OF_RANGE",
	"CP_NOT_ALIGN_16B",
	"CP_MEM_IN_SECURE_DRAM",
	"CP_ASP_ENABLE_FAIL",
	"CP_ASP_DISABLE_FAIL",
	"CP_NOT_WORKING",
	"CP_ALREADY_WORKING",
	"CP_ALREADY_DUMP_MODE",
	"CP_NOT_VALID_MAGIC",
	"CP_SHOULD_BE_DISABLE",
	"CP_ALREADY_ENABLE_CPMEM_ON",
	"CP_ALREADY_SET_WND",
	"CP_FAIL_TO_SET_WND",
	"CP_INVALID_CP_BASE",
	"CP_CORRUPTED_CP_MEM_INFO",
};

#ifdef GROUP_MEM_LINK_COMMAND

static inline void reset_ipc_map(struct mem_link_device *mld)
{
	int i;

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];

		set_txq_head(dev, 0);
		set_txq_tail(dev, 0);
		set_rxq_head(dev, 0);
		set_rxq_tail(dev, 0);
	}
}

static int shmem_reset_ipc_link(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int magic;
	unsigned int access;
	int i;

	set_access(mld, 0);
	set_magic(mld, 0);

	reset_ipc_map(mld);

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];

		skb_queue_purge(dev->skb_txq);
		atomic_set(&dev->txq.busy, 0);
		dev->req_ack_cnt[TX] = 0;

		skb_queue_purge(dev->skb_rxq);
		atomic_set(&dev->rxq.busy, 0);
		dev->req_ack_cnt[RX] = 0;
	}

	atomic_set(&ld->netif_stopped, 0);

	set_magic(mld, MEM_IPC_MAGIC);
	set_access(mld, 1);

	magic = get_magic(mld);
	access = get_access(mld);
	if (magic != MEM_IPC_MAGIC || access != 1)
		return -EACCES;

	return 0;
}

#endif

#ifdef GROUP_MEM_LINK_DEVICE

static inline bool ipc_active(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (unlikely(!cp_online(mc))) {
		mif_err("%s<->%s: %s.state %s != ONLINE <%pf>\n",
			ld->name, mc->name, mc->name, mc_state(mc), CALLER);
		return false;
	}

	if (mld->dpram_magic) {
		unsigned int magic = get_magic(mld);
		unsigned int access = get_access(mld);
		if (magic != MEM_IPC_MAGIC || access != 1) {
			mif_err("%s<->%s: ERR! magic:0x%X access:%d <%pf>\n",
				ld->name, mc->name, magic, access, CALLER);
			return false;
		}
	}

	if (atomic_read(&mld->forced_cp_crash)) {
		mif_err("%s<->%s: ERR! forced_cp_crash:%d <%pf>\n",
			ld->name, mc->name, atomic_read(&mld->forced_cp_crash),
			CALLER);
		return false;
	}

	return true;
}

static inline void purge_txq(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	int i;

	/* Purge the skb_txq in every rb */
	if (ld->sbd_ipc) {
		struct sbd_link_device *sl = &mld->sbd_link_dev;

		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, TX);
			skb_queue_purge(&rb->skb_q);
		}
	}

	/* Purge the skb_txq in every IPC device
	 * (IPC_MAP_FMT, IPC_MAP_NORM_RAW, etc.)
	 */
	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		skb_queue_purge(dev->skb_txq);
	}
}

#endif

#ifdef GROUP_MEM_CP_CRASH

static void set_modem_state(struct mem_link_device *mld, enum modem_state state)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;
	struct io_device *iod;

	spin_lock_irqsave(&mc->lock, flags);
	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, state);
	}
	spin_unlock_irqrestore(&mc->lock, flags);
}

static void shmem_handle_cp_crash(struct mem_link_device *mld,
		enum modem_state state)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->stop_pm)
		mld->stop_pm(mld);
#endif

	/* Disable normal IPC */
	set_magic(mld, MEM_CRASH_MAGIC);
	set_access(mld, 0);

	stop_net_ifaces(ld);
	purge_txq(mld);

	if (cp_online(mc)) {
		switch (state) {
		case STATE_CRASH_RESET:
			modem_notify_event(MODEM_EVENT_RESET);
			break;
		case STATE_CRASH_EXIT:
			modem_notify_event(MODEM_EVENT_EXIT);
			break;
		case STATE_CRASH_WATCHDOG:
			modem_notify_event(MODEM_EVENT_WATCHDOG);
			break;
		default:
			mif_err("Invalid state to notify\n");
			break;
		}
	}

	if (cp_online(mc) || cp_booting(mc))
		set_modem_state(mld, state);

	atomic_set(&mld->forced_cp_crash, 0);
}

static void handle_no_cp_crash_ack(unsigned long arg)
{
	struct mem_link_device *mld = (struct mem_link_device *)arg;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (gpio_get_value(mc->s5100_gpio_phone_active) == 0) {
		mif_info("Phone_active GPIO is LOW - Response of CP\n");
		mif_info("Set s5100_cp_reset_required to FALSE\n");
		mc->s5100_cp_reset_required = false;
	} else {
		mif_info("Phone_active GPIO is HIGH - NO response of CP\n");
		mif_info("Set s5100_cp_reset_required to TRUE\n");
		mc->s5100_cp_reset_required = true;
	}

	if (cp_crashed(mc)) {
		mif_debug("%s: STATE_CRASH_EXIT without CRASH_ACK\n",
			ld->name);
	} else {
		mif_err("%s: ERR! No CRASH_ACK from CP\n", ld->name);
		shmem_handle_cp_crash(mld, STATE_CRASH_EXIT);
	}
}

static void shmem_forced_cp_crash(struct mem_link_device *mld,
	u32 crash_reason_owner, char *crash_reason_string)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	/* Disable normal IPC */
	set_magic(mld, MEM_CRASH_MAGIC);
	set_access(mld, 0);

	if (!cp_online(mc) && !cp_booting(mc)) {
		mif_err("%s: %s.state %s != ONLINE <%pf>\n",
			ld->name, mc->name, mc_state(mc), CALLER);
		return;
	}

	if (atomic_inc_return(&mld->forced_cp_crash) > 1) {
		mif_err("%s: ALREADY in progress <%pf>\n",
			ld->name, CALLER);
		return;
	}

	/* Disable debug Snapshot */
	mif_set_snapshot(false);

	mld->crash_reason.owner = crash_reason_owner;
	strlcpy(mld->crash_reason.string, crash_reason_string,
		CRASH_REASON_SIZE);

	stop_net_ifaces(ld);

	if (mld->debug_info)
		mld->debug_info();

	/**
	 * If there is no CRASH_ACK from CP in a timeout,
	 * handle_no_cp_crash_ack() will be executed.
	 */
	mif_add_timer(&mld->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
		      handle_no_cp_crash_ack, (unsigned long)mld);

	/* Raise DUMP_NOTI GPIO to CP */
	s5100_force_crash_exit_ext();

	mif_err("%s->%s: CP_CRASH_REQ by %d, %s <%pf>\n",
				ld->name, mc->name,
				crash_reason_owner, crash_reason_string,
				CALLER);
}
#endif

static bool rild_ready(struct link_device *ld)
{
	struct io_device *fmt_iod;
	struct io_device *rfs_iod;
	int fmt_opened;
	int rfs_opened;

	fmt_iod = link_get_iod_with_channel(ld, SIPC5_CH_ID_FMT_0);
	if (!fmt_iod) {
		mif_err("%s: No FMT io_device\n", ld->name);
		return false;
	}

	rfs_iod = link_get_iod_with_channel(ld, SIPC5_CH_ID_RFS_0);
	if (!rfs_iod) {
		mif_err("%s: No RFS io_device\n", ld->name);
		return false;
	}

	fmt_opened = atomic_read(&fmt_iod->opened);
	rfs_opened = atomic_read(&rfs_iod->opened);
	mif_err_limited("%s: %s.opened=%d, %s.opened=%d\n", ld->name,
		fmt_iod->name, fmt_opened, rfs_iod->name, rfs_opened);
	if (fmt_opened > 0 && rfs_opened > 0)
		return true;

	return false;
}

static void cmd_init_start_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	int err;

	mif_err("%s: INIT_START <- %s (%s.state:%s cp_boot_done:%d)\n",
		ld->name, mc->name, mc->name, mc_state(mc),
		atomic_read(&mld->cp_boot_done));

	if (!ld->sbd_ipc) {
		mif_err("%s: LINK_ATTR_SBD_IPC is NOT set\n", ld->name);
		return;
	}

	err = init_sbd_link(&mld->sbd_link_dev);
	if (err < 0) {
		mif_err("%s: init_sbd_link fail(%d)\n", ld->name, err);
		return;
	}

	if (mld->attrs & LINK_ATTR(LINK_ATTR_IPC_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	sbd_activate(&mld->sbd_link_dev);
	send_ipc_irq(mld, cmd2int(CMD_PIF_INIT_DONE));

	mif_err("%s: PIF_INIT_DONE -> %s\n", ld->name, mc->name);
}

static void write_clk_table_to_shmem(struct mem_link_device *mld)
{
	struct clock_table *clk_tb;
	u32 *clk_data;
	int i, j;

	if (mld->clk_table == NULL) {
		mif_err("clk_table is not defined\n");
		return;
	}

	clk_tb = (struct clock_table *)mld->clk_table;

	strcpy(clk_tb->parser_version, "CT0");
	clk_tb->total_table_count = 4;

	strcpy(clk_tb->table_info[0].table_name, "CL0");
	clk_tb->table_info[0].table_count = mld->cl0_table.num_of_table;

	strcpy(clk_tb->table_info[1].table_name, "CL1");
	clk_tb->table_info[1].table_count = mld->cl1_table.num_of_table;

	strcpy(clk_tb->table_info[2].table_name, "MIF");
	clk_tb->table_info[2].table_count = mld->mif_table.num_of_table;

	strcpy(clk_tb->table_info[3].table_name, "INT");
	clk_tb->table_info[3].table_count = mld->int_table.num_of_table;

	clk_data = (u32 *)&(clk_tb->table_info[clk_tb->total_table_count]);

	/* CL0 */
	for (i = 0; i < mld->cl0_table.num_of_table; i++) {
		*clk_data = mld->cl0_table.freq[i];
		clk_data++;
	}

	/* CL1 */
	for (i = 0; i < mld->cl1_table.num_of_table; i++) {
		*clk_data = mld->cl1_table.freq[i];
		clk_data++;
	}

	/* MIF */
	for (i = 0; i < mld->mif_table.num_of_table; i++) {
		*clk_data = mld->mif_table.freq[i];
		clk_data++;
	}

	/* INT */
	for (i = 0; i < mld->int_table.num_of_table; i++) {
		*clk_data = mld->int_table.freq[i];
		clk_data++;
	}

	mif_info("PARSER_VERSION: %s\n", clk_tb->parser_version);
	mif_info("TOTAL_TABLE_COUNT: %d\n", clk_tb->total_table_count);

	for (i = 0; i < clk_tb->total_table_count; i++) {
		mif_info("TABLE_NAME[%d] : %s\n", i+1,
			clk_tb->table_info[i].table_name);
		mif_info("TABLE_COUNT[%d]: %d\n", i+1,
			clk_tb->table_info[i].table_count);
	}

	clk_data = (u32 *)&(clk_tb->table_info[clk_tb->total_table_count]);

	for (i = 0; i < clk_tb->total_table_count; i++) {
		for (j = 0; j < clk_tb->table_info[i].table_count; j++)
			mif_info("CLOCK_TABLE[%d][%d] : %d\n",
				i+1, j+1, *clk_data++);
	}
}

static void cmd_phone_start_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;
	int err;
	static int phone_start_count;

	mif_err_limited("%s: CP_START <- %s (%s.state:%s cp_boot_done:%d)\n",
		ld->name, mc->name, mc->name, mc_state(mc),
		atomic_read(&mld->cp_boot_done));

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->start_pm)
		mld->start_pm(mld);
#endif

	if (mld->state == LINK_STATE_OFFLINE)
		phone_start_count = 0;

	if (atomic_read(&mld->cp_boot_done)) {
		mif_err_limited("Abnormal CP_START from CP\n");

		if (phone_start_count < 100) {
			if (phone_start_count++ > 3) {
				phone_start_count = 101;
				send_ipc_irq(mld,
					cmd2int(phone_start_count - 100));
				return;
			}
		} else {
			if (phone_start_count++ < 105) {
				mif_err("%s: CMD(0x%x) -> %s\n", ld->name,
					cmd2int(phone_start_count - 100),
					mc->name);
				send_ipc_irq(mld,
					cmd2int(phone_start_count - 100));
			} else {
				shmem_forced_cp_crash(mld,
					CRASH_REASON_CP_RSV_0,
					"Abnormal CP_START from CP");
			}
			return;
		}
	}

	spin_lock_irqsave(&mld->state_lock, flags);

	if (mld->state == LINK_STATE_IPC) {
		/*
		If there is no INIT_END command from AP, CP sends a CP_START
		command to AP periodically until it receives INIT_END from AP
		even though it has already been in ONLINE state.
		*/
		if (rild_ready(ld)) {
			mif_err("%s: INIT_END -> %s\n", ld->name, mc->name);
			send_ipc_irq(mld, cmd2int(CMD_INIT_END));
		}
		goto exit;
	}

	err = shmem_reset_ipc_link(mld);
	if (err) {
		mif_err("%s: shmem_reset_ipc_link fail(%d)\n", ld->name, err);
		goto exit;
	}

	if (rild_ready(ld)) {
		mif_err("%s: INIT_END -> %s\n", ld->name, mc->name);
		send_ipc_irq(mld, cmd2int(CMD_INIT_END));
		atomic_set(&mld->cp_boot_done, 1);
	}

	mld->state = LINK_STATE_IPC;
	complete_all(&mc->init_cmpl);
	modem_notify_event(MODEM_EVENT_ONLINE);

exit:
	spin_unlock_irqrestore(&mld->state_lock, flags);
}

static void cmd_crash_reset_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	mif_err("%s<-%s: ERR! CP_CRASH_RESET\n", ld->name, mc->name);

	shmem_handle_cp_crash(mld, STATE_CRASH_RESET);
}

static void cmd_crash_exit_handler(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	/* Disable debug Snapshot */
	mif_set_snapshot(false);

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_CP_CRASH;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);

	if (atomic_read(&mld->forced_cp_crash))
		mif_err("%s<-%s: CP_CRASH_ACK\n", ld->name, mc->name);
	else
		mif_err("%s<-%s: ERR! CP_CRASH_EXIT\n", ld->name, mc->name);

	if (gpio_get_value(mc->s5100_gpio_phone_active) == 0) {
		mif_info("Phone_active GPIO is LOW - Response of CP\n");
		mif_info("Set s5100_cp_reset_required to FALSE\n");
		mc->s5100_cp_reset_required = false;
	} else {
		mif_info("Phone_active GPIO is HIGH - NO response of CP\n");
		mif_info("Set s5100_cp_reset_required to TRUE\n");
		mc->s5100_cp_reset_required = true;
	}

	shmem_handle_cp_crash(mld, STATE_CRASH_EXIT);
}

static void shmem_cmd_handler(struct mem_link_device *mld, u16 cmd)
{
	struct link_device *ld = &mld->link_dev;

	switch (cmd) {
	case CMD_INIT_START:
		cmd_init_start_handler(mld);
		break;

	case CMD_PHONE_START:
		cmd_phone_start_handler(mld);
		break;

	case CMD_CRASH_RESET:
		cmd_crash_reset_handler(mld);
		break;

	case CMD_CRASH_EXIT:
		cmd_crash_exit_handler(mld);
		break;

	default:
		mif_err("%s: Unknown command 0x%04X\n", ld->name, cmd);
		break;
	}
}

#ifdef GROUP_MEM_IPC_TX

static inline int check_txq_space(struct mem_link_device *mld,
				  struct mem_ipc_device *dev,
				  unsigned int qsize, unsigned int in,
				  unsigned int out, unsigned int count)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned int space;

	if (!circ_valid(qsize, in, out)) {
		mif_err("%s: ERR! Invalid %s_TXQ{qsize:%d in:%d out:%d}\n",
			ld->name, dev->name, qsize, in, out);
		return -EIO;
	}

	space = circ_get_space(qsize, in, out);
	if (unlikely(space < count)) {
		if (cp_online(mc)) {
			mif_err("%s: NOSPC %s_TX{qsize:%d in:%d out:%d space:%d len:%d}\n",
				ld->name, dev->name, qsize,
				in, out, space, count);
		}
		return -ENOSPC;
	}

	return space;
}

static int txq_write(struct mem_link_device *mld, struct mem_ipc_device *dev,
		     struct sk_buff *skb)
{
	char *src = skb->data;
	unsigned int count = skb->len;
	char *dst = get_txq_buff(dev);
	unsigned int qsize = get_txq_buff_size(dev);
	unsigned int in = get_txq_head(dev);
	unsigned int out = get_txq_tail(dev);
	int space;

	space = check_txq_space(mld, dev, qsize, in, out, count);
	if (unlikely(space < 0))
		return space;

	barrier();

	circ_write(dst, src, qsize, in, count);

	barrier();

	set_txq_head(dev, circ_new_ptr(qsize, in, count));

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

static int tx_frames_to_dev(struct mem_link_device *mld,
			    struct mem_ipc_device *dev)
{
	struct sk_buff_head *skb_txq = dev->skb_txq;
	int tx_bytes = 0;
	int ret = 0;

	while (1) {
		struct sk_buff *skb;

		skb = skb_dequeue(skb_txq);
		if (unlikely(!skb))
			break;

		ret = txq_write(mld, dev, skb);
		if (unlikely(ret < 0)) {
			/* Take the skb back to the skb_txq */
			skb_queue_head(skb_txq, skb);
			break;
		}
		pktlog_tx_bottom_skb(mld, skb);

		tx_bytes += ret;

#ifdef DEBUG_MODEM_IF_LINK_TX
		mif_pkt(skbpriv(skb)->sipc_ch, "LNK-TX", skb);
#endif

		dev_kfree_skb_any(skb);
	}

	return (ret < 0) ? ret : tx_bytes;
}

static enum hrtimer_restart tx_timer_func(struct hrtimer *timer)
{
	struct mem_link_device *mld;
	struct link_device *ld;
	struct modem_ctl *mc;
	int i;
	bool need_schedule;
	u16 mask;
	unsigned long flags;

	mld = container_of(timer, struct mem_link_device, tx_timer);
	ld = &mld->link_dev;
	mc = ld->mc;

	need_schedule = false;
	mask = 0;

	cp_runtime_link(mc, LINK_TX_TIMER, 0);
	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(!ipc_active(mld)))
		goto exit;

#ifdef CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM
	if (mld->link_active) {
		if (!mld->link_active(mld)) {
			need_schedule = true;
			goto exit;
		}
	}
#endif

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		int ret;

		if (unlikely(under_tx_flow_ctrl(mld, dev))) {
			ret = check_tx_flow_ctrl(mld, dev);
			if (ret < 0) {
				if (ret == -EBUSY || ret == -ETIME) {
					need_schedule = true;
					continue;
				} else {
					shmem_forced_cp_crash(mld, CRASH_REASON_MIF_TX_ERR,
						"check_tx_flow_ctrl error");
					need_schedule = false;
					goto exit;
				}
			}
		}

		ret = tx_frames_to_dev(mld, dev);
		if (unlikely(ret < 0)) {
			if (ret == -EBUSY || ret == -ENOSPC) {
				need_schedule = true;
				txq_stop(mld, dev);
				/* If txq has 2 or more packet and 2nd packet
				  has -ENOSPC return, It request irq to consume
				  the TX ring-buffer from CP */
				mask |= msg_mask(dev);
				continue;
			} else {
				shmem_forced_cp_crash(mld, CRASH_REASON_MIF_TX_ERR,
					"tx_frames_to_dev error");
				need_schedule = false;
				goto exit;
			}
		}

		if (ret > 0)
			mask |= msg_mask(dev);

		if (!skb_queue_empty(dev->skb_txq))
			need_schedule = true;
	}

	if (!need_schedule) {
		for (i = 0; i < MAX_SIPC_MAP; i++) {
			if (!txq_empty(mld->dev[i])) {
				need_schedule = true;
				break;
			}
		}
	}

	if (mask)
		send_ipc_irq(mld, mask2int(mask));

exit:
	if (need_schedule) {
		ktime_t ktime = ktime_set(0, mld->tx_period_ms * NSEC_PER_MSEC);
		if (!wake_lock_active(&mld->tx_timer_wlock))
			wake_lock(&mld->tx_timer_wlock);
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	} else {
		if (wake_lock_active(&mld->tx_timer_wlock))
			wake_unlock(&mld->tx_timer_wlock);
	}

	spin_unlock_irqrestore(&mc->lock, flags);
	cp_runtime_dislink(mc, LINK_TX_TIMER, 0);

	return HRTIMER_NORESTART;
}

static inline void start_tx_timer(struct mem_link_device *mld,
				  struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(cp_offline(mc)))
		goto exit;

	if (!hrtimer_is_queued(timer)) {
		ktime_t ktime = ktime_set(0, mld->tx_period_ms * NSEC_PER_MSEC);
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

exit:
	spin_unlock_irqrestore(&mc->lock, flags);
}

static inline void cancel_tx_timer(struct mem_link_device *mld,
				   struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (hrtimer_active(timer))
		hrtimer_cancel(timer);

	spin_unlock_irqrestore(&mc->lock, flags);
}

static inline void start_datalloc_timer(struct mem_link_device *mld,
				  struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(cp_offline(mc)))
		goto exit;

	if (!hrtimer_is_queued(timer)) {
		ktime_t ktime = ktime_set(0, ms2ns(DATALLOC_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

exit:
	spin_unlock_irqrestore(&mc->lock, flags);
}

static inline void __cancel_datalloc_timer(struct mem_link_device *mld,
				   struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (hrtimer_active(timer))
		hrtimer_cancel(timer);

	spin_unlock_irqrestore(&mc->lock, flags);
}

static inline void cancel_datalloc_timer(struct mem_link_device *mld)
{
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	struct sbd_ipc_device *ipc_dev = sl->ipc_dev;
	int i;

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = &ipc_dev[i].rb[DL];
		if (rb->zerocopy) {
			struct zerocopy_adaptor *zdptr = rb->zdptr;
			__cancel_datalloc_timer(mld, &zdptr->datalloc_timer);
		}
	}

}

static int tx_frames_to_rb(struct sbd_ring_buffer *rb)
{
	struct sk_buff_head *skb_txq = &rb->skb_q;
	int tx_bytes = 0;
	int ret = 0;

	while (1) {
		struct sk_buff *skb;

		skb = skb_dequeue(skb_txq);
		if (unlikely(!skb))
			break;

		ret = sbd_pio_tx(rb, skb);
		if (unlikely(ret < 0)) {
			/* Take the skb back to the skb_txq */
			skb_queue_head(skb_txq, skb);
			break;
		}

		tx_bytes += ret;
#ifdef DEBUG_MODEM_IF_LINK_TX
		mif_pkt(rb->ch, "LNK-TX", skb);
#endif
		dev_kfree_skb_any(skb);
	}

	return (ret < 0) ? ret : tx_bytes;
}

static enum hrtimer_restart sbd_tx_timer_func(struct hrtimer *timer)
{
	struct mem_link_device *mld =
		container_of(timer, struct mem_link_device, sbd_tx_timer);
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	int i;
	bool need_schedule = false;
	u16 mask = 0;
	unsigned long flags = 0;

	cp_runtime_link(mc, LINK_SBD_TX_TIMER, 0);

	spin_lock_irqsave(&mc->lock, flags);
	if (unlikely(!ipc_active(mld))) {
		spin_unlock_irqrestore(&mc->lock, flags);
		goto exit;
	}
	spin_unlock_irqrestore(&mc->lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->link_active) {
		if (!mld->link_active(mld)) {
			need_schedule = true;
			goto exit;
		}
	}
#endif

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, TX);
		int ret;

		if (unlikely(sbd_under_tx_flow_ctrl(rb))) {
			ret = sbd_check_tx_flow_ctrl(rb);
			if (ret < 0) {
				if (ret == -EBUSY || ret == -ETIME) {
					need_schedule = true;
					continue;
				} else {
					shmem_forced_cp_crash(mld,
						CRASH_REASON_MIF_TX_ERR,
						"check_sbd_tx_flow_ctrl error");
					need_schedule = false;
					goto exit;
				}
			}
		}

		ret = tx_frames_to_rb(rb);
		if (unlikely(ret < 0)) {
			if (ret == -EBUSY || ret == -ENOSPC) {
				need_schedule = true;
				sbd_txq_stop(rb);
				mask = MASK_SEND_DATA;
				continue;
			} else {
				shmem_forced_cp_crash(mld, CRASH_REASON_MIF_TX_ERR,
					"tx_frames_to_rb error");
				need_schedule = false;
				goto exit;
			}
		}

		if (ret > 0)
			mask = MASK_SEND_DATA;

		if (!skb_queue_empty(&rb->skb_q))
			need_schedule = true;
	}

	if (!need_schedule) {
		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb;

			rb = sbd_id2rb(sl, i, TX);
			if (!rb_empty(rb)) {
				need_schedule = true;
				break;
			}
		}
	}

	if (mask) {
		spin_lock_irqsave(&mc->lock, flags);
		if (unlikely(!ipc_active(mld))) {
			spin_unlock_irqrestore(&mc->lock, flags);
			need_schedule = false;
			goto exit;
		}
		send_ipc_irq(mld, mask2int(mask));
		spin_unlock_irqrestore(&mc->lock, flags);
	}
	

	if (mc->reserve_doorbell_int == true)
		wake_lock_timeout(&mld->sbd_tx_timer_wlock, msecs_to_jiffies(3000));

exit:
	if (need_schedule) {
		ktime_t ktime = ktime_set(0, mld->tx_period_ms * NSEC_PER_MSEC);
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

	cp_runtime_dislink(mc, LINK_SBD_TX_TIMER, 0);

	return HRTIMER_NORESTART;
}

enum hrtimer_restart datalloc_timer_func(struct hrtimer *timer)
{
	struct zerocopy_adaptor *zdptr =
		container_of(timer, struct zerocopy_adaptor, datalloc_timer);
	struct sbd_ring_buffer *rb = zdptr->rb;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	bool need_schedule = false;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);
	if (unlikely(!ipc_active(mld))) {
		spin_unlock_irqrestore(&mc->lock, flags);
		goto exit;
	}
	spin_unlock_irqrestore(&mc->lock, flags);

	if (-ENOMEM == allocate_data_in_advance(zdptr)) {
		need_schedule = true;
	}

	if (need_schedule) {
		ktime_t ktime = ktime_set(0, ms2ns(DATALLOC_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}
exit:

	return HRTIMER_NORESTART;
}

static int xmit_ipc_to_rb(struct mem_link_device *mld, enum sipc_ch_id ch,
			  struct sk_buff *skb)
{
	int ret;
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = ld->mc;
	struct sbd_ring_buffer *rb = sbd_ch2rb_with_skb(&mld->sbd_link_dev, ch, TX, skb);
	struct sk_buff_head *skb_txq;
	unsigned long flags;

	if (!rb) {
		mif_err("%s: %s->%s: ERR! NO SBD RB {ch:%d}\n",
			ld->name, iod->name, mc->name, ch);
		return -ENODEV;
}

	skb_txq = &rb->skb_q;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->forbid_cp_sleep)
		mld->forbid_cp_sleep(mld);
#endif

	spin_lock_irqsave(&rb->lock, flags);

	if (unlikely(skb_txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err_limited("%s: %s->%s: ERR! {ch:%d} "
				"skb_txq.len %d >= limit %d\n",
				ld->name, iod->name, mc->name, ch,
				skb_txq->qlen, MAX_SKB_TXQ_DEPTH);
		ret = -EBUSY;
	} else {
		skb->len = min_t(int, skb->len, rb->buff_size);

		ret = skb->len;
		skb_queue_tail(skb_txq, skb);
		if (!wake_lock_active(&mld->sbd_tx_timer_wlock))
			wake_lock_timeout(&mld->sbd_tx_timer_wlock, msecs_to_jiffies(3000));
		start_tx_timer(mld, &mld->sbd_tx_timer);
	}

	spin_unlock_irqrestore(&rb->lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->permit_cp_sleep)
		mld->permit_cp_sleep(mld);
#endif

	return ret;
}
#endif

static int xmit_ipc_to_dev(struct mem_link_device *mld, enum sipc_ch_id ch,
			   struct sk_buff *skb)
{
	int ret;
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = ld->mc;
	struct mem_ipc_device *dev = mld->dev[get_mmap_idx(ch, skb)];
	struct sk_buff_head *skb_txq;

	if (!dev) {
		mif_err("%s: %s->%s: ERR! NO IPC DEV {ch:%d}\n",
			ld->name, iod->name, mc->name, ch);
		return -ENODEV;
	}

	skb_txq = dev->skb_txq;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->forbid_cp_sleep)
		mld->forbid_cp_sleep(mld);
#endif
	if (unlikely(skb_txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err_limited("%s: %s->%s: ERR! %s TXQ.qlen %d >= limit %d\n",
				ld->name, iod->name, mc->name, dev->name,
				skb_txq->qlen, MAX_SKB_TXQ_DEPTH);
		ret = -EBUSY;
	} else {
		ret = skb->len;
		skb_queue_tail(dev->skb_txq, skb);
		if (!wake_lock_active(&mld->tx_timer_wlock))
			wake_lock(&mld->tx_timer_wlock);
		start_tx_timer(mld, &mld->tx_timer);
	}

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->permit_cp_sleep)
		mld->permit_cp_sleep(mld);
#endif

	return ret;
}

static int xmit_ipc(struct mem_link_device *mld, struct io_device *iod,
		    enum sipc_ch_id ch, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;

	if (unlikely(!ipc_active(mld)))
		return -EIO;

	if (ld->sbd_ipc && iod->sbd_ipc) {
		if (likely(sbd_active(&mld->sbd_link_dev)))
			return xmit_ipc_to_rb(mld, ch, skb);
		else
			return -ENODEV;
	} else {
		return xmit_ipc_to_dev(mld, ch, skb);
	}
}

static inline int check_udl_space(struct mem_link_device *mld,
				  struct mem_ipc_device *dev,
				  unsigned int qsize, unsigned int in,
				  unsigned int out, unsigned int count)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int space;

	if (!circ_valid(qsize, in, out)) {
		mif_err("%s: ERR! Invalid %s_TXQ{qsize:%d in:%d out:%d}\n",
			ld->name, dev->name, qsize, in, out);
		return -EIO;
	}

	space = circ_get_space(qsize, in, out);
	if (unlikely(space < count)) {
		mif_err("%s: NOSPC %s_TX{qsize:%d in:%d out:%d free:%d len:%d}\n",
			ld->name, dev->name, qsize, in, out, space, count);
		return -ENOSPC;
	}

	return 0;
}

static inline int udl_write(struct mem_link_device *mld,
			    struct mem_ipc_device *dev, struct sk_buff *skb)
{
	unsigned int count = skb->len;
	char *src = skb->data;
	char *dst = get_txq_buff(dev);
	unsigned int qsize = get_txq_buff_size(dev);
	unsigned int in = get_txq_head(dev);
	unsigned int out = get_txq_tail(dev);
	int space;

	space = check_udl_space(mld, dev, qsize, in, out, count);
	if (unlikely(space < 0))
		return space;

	barrier();

	circ_write(dst, src, qsize, in, count);

	barrier();

	set_txq_head(dev, circ_new_ptr(qsize, in, count));

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

static int xmit_udl(struct mem_link_device *mld, struct io_device *iod,
		    enum sipc_ch_id ch, struct sk_buff *skb)
{
	int ret;
	struct mem_ipc_device *dev = mld->dev[IPC_MAP_NORM_RAW];
	int count = skb->len;
	int tried = 0;

	while (1) {
		ret = udl_write(mld, dev, skb);
		if (ret == count)
			break;

		if (ret != -ENOSPC)
			goto exit;

		tried++;
		if (tried >= 20)
			goto exit;

		if (in_interrupt())
			mdelay(50);
		else
			msleep(50);
	}

#ifdef DEBUG_MODEM_IF_LINK_TX
	mif_pkt(ch, "LNK-TX", skb);
#endif

	dev_kfree_skb_any(skb);

exit:
	return ret;
}

/*============================================================================*/

#ifdef GROUP_MEM_IPC_RX

static void pass_skb_to_demux(struct mem_link_device *mld, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	int ret;
	u8 ch = skbpriv(skb)->sipc_ch;

	if (unlikely(!iod)) {
		mif_err("%s: ERR! No IOD for CH.%d\n", ld->name, ch);
		dev_kfree_skb_any(skb);
		shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RIL_BAD_CH,
			"ERR! No IOD for CH.XX in pass_skb_to_demux()");
		return;
	}

#ifdef DEBUG_MODEM_IF_LINK_RX
	mif_pkt(ch, "LNK-RX", skb);
#endif

	ret = iod->recv_skb_single(iod, ld, skb);
	if (unlikely(ret < 0)) {
		struct modem_ctl *mc = ld->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s->recv_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
		dev_kfree_skb_any(skb);
	}
}

static inline void link_to_demux(struct mem_link_device  *mld)
{
	int i;

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		struct sk_buff_head *skb_rxq = dev->skb_rxq;

		while (1) {
			struct sk_buff *skb;

			skb = skb_dequeue(skb_rxq);
			if (!skb)
				break;

			pass_skb_to_demux(mld, skb);
		}
	}
}

static void link_to_demux_work(struct work_struct *ws)
{
	struct link_device *ld;
	struct mem_link_device *mld;

	ld = container_of(ws, struct link_device, rx_delayed_work.work);
	mld = to_mem_link_device(ld);

	link_to_demux(mld);
}

static inline void schedule_link_to_demux(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct delayed_work *dwork = &ld->rx_delayed_work;

	/*queue_delayed_work(ld->rx_wq, dwork, 0);*/
	queue_work_on(7, ld->rx_wq, &dwork->work);
}

static struct sk_buff *rxq_read(struct mem_link_device *mld,
				struct mem_ipc_device *dev,
				unsigned int in)
{
	struct link_device *ld = &mld->link_dev;
	struct sk_buff *skb;
	char *src = get_rxq_buff(dev);
	unsigned int qsize = get_rxq_buff_size(dev);
	unsigned int out = get_rxq_tail(dev);
	unsigned int rest = circ_get_usage(qsize, in, out);
	unsigned int len;
	char hdr[SIPC5_MIN_HEADER_SIZE];

	/* Copy the header in a frame to the header buffer */
	circ_read(hdr, src, qsize, out, SIPC5_MIN_HEADER_SIZE);

	/* Check the config field in the header */
	if (unlikely(!sipc5_start_valid(hdr))) {
		mif_err("%s: ERR! %s BAD CFG 0x%02X (in:%d out:%d rest:%d)\n",
			ld->name, dev->name, hdr[SIPC5_CONFIG_OFFSET],
			in, out, rest);
		goto bad_msg;
	}

	/* Verify the length of the frame (data + padding) */
	len = sipc5_get_total_len(hdr);
	if (unlikely(len > rest)) {
		mif_err("%s: ERR! %s BAD LEN %d > rest %d\n",
			ld->name, dev->name, len, rest);
		goto bad_msg;
	}

	/* Allocate an skb */
	skb = mem_alloc_skb(len);
	if (!skb) {
		mif_err("%s: ERR! %s mem_alloc_skb(%d) fail\n",
			ld->name, dev->name, len);
		goto no_mem;
	}

	/* Read the frame from the RXQ */
	circ_read(skb_put(skb, len), src, qsize, out, len);

	/* Update tail (out) pointer to the frame to be read in the future */
	set_rxq_tail(dev, circ_new_ptr(qsize, out, len));

	/* Finish reading data before incrementing tail */
	smp_mb();

#ifdef DEBUG_MODEM_IF
	/* Record the time-stamp */
	getnstimeofday(&skbpriv(skb)->ts);
#endif

	return skb;

bad_msg:
	mif_err("%s%s%s: ERR! BAD MSG: %02x %02x %02x %02x\n",
		ld->name, arrow(RX), ld->mc->name,
		hdr[0], hdr[1], hdr[2], hdr[3]);
	set_rxq_tail(dev, in);	/* Reset tail (out) pointer */
	shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RX_BAD_DATA,
		"ERR! BAD MSG from CP in rxq_read()");

no_mem:
	return NULL;
}

static int rx_frames_from_dev(struct mem_link_device *mld,
			      struct mem_ipc_device *dev)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int qsize = get_rxq_buff_size(dev);
	unsigned int in = get_rxq_head(dev);
	unsigned int out = get_rxq_tail(dev);
	unsigned int size = circ_get_usage(qsize, in, out);
	int rcvd = 0;

	if (unlikely(circ_empty(in, out)))
		return 0;

	while (rcvd < size) {
		struct sk_buff *skb;
		u8 ch;
		struct io_device *iod;

		skb = rxq_read(mld, dev, in);
		if (!skb)
			return -ENOMEM;

		pktlog_rx_bottom_skb(mld, skb);

		ch = sipc5_get_ch(skb->data);
		iod = link_get_iod_with_channel(ld, ch);
		if (!iod) {
			mif_err("%s: ERR! [%s]No IOD for CH.%d(out:%u)\n",
				ld->name, dev->name, ch, get_rxq_tail(dev));
			pr_skb("CRASH", skb);
			dev_kfree_skb_any(skb);
			shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RX_BAD_DATA,
				"ERR! No IOD from CP in rx_frames_from_dev()");
			break;
		}

		/* Record the IO device and the link device into the &skb->cb */
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		skbpriv(skb)->lnk_hdr = iod->link_header;
		skbpriv(skb)->sipc_ch = ch;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd += skb->len;
		pass_skb_to_demux(mld, skb);
	}

	if (rcvd < size) {
		struct link_device *ld = &mld->link_dev;
		mif_err("%s: WARN! rcvd %d < size %d\n", ld->name, rcvd, size);
	}

	return rcvd;
}

static int recv_ipc_frames(struct mem_link_device *mld,
			    struct mem_snapshot *mst)
{
	int i;
	u16 intr = mst->int2ap;

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		int rcvd;

		if (req_ack_valid(dev, intr))
			recv_req_ack(mld, dev, mst);

		rcvd = rx_frames_from_dev(mld, dev);
		if (rcvd < 0)
			return rcvd;

		if (req_ack_valid(dev, intr))
			send_res_ack(mld, dev);

		if (res_ack_valid(dev, intr))
			recv_res_ack(mld, dev, mst);
	}
	return 0;
}

void pass_skb_to_net(struct mem_link_device *mld, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;
	struct skbuff_private *priv;
	struct io_device *iod;
	struct modem_ctl *mc = ld->mc;
	int ret;

	if (unlikely(!cp_online(mc))) {
		mif_err_limited("ERR! CP not online!, skb:%pK\n", skb);
		dev_kfree_skb_any(skb);
		return;
	}

	priv = skbpriv(skb);
	if (unlikely(!priv)) {
		mif_err("%s: ERR! No PRIV in skb@%pK\n", ld->name, skb);
		dev_kfree_skb_any(skb);
		shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RX_BAD_DATA,
			"ERR! No PRIV in pass_skb_to_net()");
		return;
	}

	iod = priv->iod;
	if (unlikely(!iod)) {
		mif_err("%s: ERR! No IOD in skb@%pK\n", ld->name, skb);
		dev_kfree_skb_any(skb);
		shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RX_BAD_DATA,
			"ERR! No IOD in pass_skb_to_net()");
		return;
	}

#ifdef DEBUG_MODEM_IF_LINK_RX
	mif_pkt(iod->id, "LNK-RX", skb);
#endif

	ret = iod->recv_net_skb(iod, ld, skb);
	if (unlikely(ret < 0)) {
		struct modem_ctl *mc = ld->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s->recv_net_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
		dev_kfree_skb_any(skb);
	}
}

#define FREE_RB_BUF_COUNT 200
static int rx_net_frames_from_zerocopy_adaptor(struct sbd_ring_buffer *rb,
		int budget, int *work_done)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int num_frames;
	int use_memcpy = 0;

#ifdef CONFIG_LINK_DEVICE_NAPI
	num_frames = min_t(unsigned int, rb_usage(rb), budget);
#else /* !CONFIG_LINK_DEVICE_NAPI */
	num_frames = rb_usage(rb);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	if (mld->force_use_memcpy || (num_frames > ld->mif_buff_mng->free_cell_count)
		|| (FREE_RB_BUF_COUNT > circ_get_space(zdptr->len, *(zdptr->rp), *(zdptr->wp)))) {
		use_memcpy = 1;
		mld->memcpy_packet_count++;
	} else {
		use_memcpy = 0;
		mld->zeromemcpy_packet_count++;
	}

	while (rcvd < num_frames) {
		struct sk_buff *skb;

		skb = sbd_pio_rx_zerocopy_adaptor(rb, use_memcpy);
		if (!skb)
			break;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_net(). */
		rcvd++;

		pass_skb_to_net(mld, skb);
	}

	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct link_device *ld = rb->ld;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}

#ifdef CONFIG_LINK_DEVICE_NAPI
	*work_done = rcvd;
	allocate_data_in_advance(zdptr);
#else /* !CONFIG_LINK_DEVICE_NAPI */
	start_datalloc_timer(mld, &zdptr->datalloc_timer);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	return rcvd;
}

static int rx_net_frames_from_rb(struct sbd_ring_buffer *rb, int budget,
		int *work_done)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned int num_frames;

#ifdef CONFIG_LINK_DEVICE_NAPI
	num_frames = min_t(unsigned int, rb_usage(rb), budget);
#else /* !CONFIG_LINK_DEVICE_NAPI */
	num_frames = rb_usage(rb);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	while (rcvd < num_frames) {
		struct sk_buff *skb;

		skb = sbd_pio_rx(rb);
		if (!skb)
			return -ENOMEM;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_net(). */
		rcvd++;

		pass_skb_to_net(mld, skb);
	}

	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct link_device *ld = rb->ld;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}

#ifdef CONFIG_LINK_DEVICE_NAPI
	*work_done = rcvd;
#endif /* CONFIG_LINK_DEVICE_NAPI */

	return rcvd;
}

static int rx_ipc_frames_from_zerocopy_adaptor(struct sbd_ring_buffer *rb)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int num_frames = zerocopy_adaptor_usage(zdptr);

	while (rcvd < num_frames) {
		struct sk_buff *skb;

		skb = sbd_pio_rx_zerocopy_adaptor(rb, 0);
		if (!skb) {
#ifdef DEBUG_MODEM_IF
			panic("skb alloc failed.");
#else
			shmem_forced_cp_crash(mld, CRASH_REASON_MIF_ZMC,
				"ERR! ZMC alloc failed");
#endif
			break;
		}

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd++;

		if (skbpriv(skb)->lnk_hdr) {
			u8 ch = rb->ch;
			u8 fch = sipc5_get_ch(skb->data);
			if (fch != ch) {
				mif_err("frm.ch:%d != rb.ch:%d\n", fch, ch);
				pr_skb("CRASH", skb);
				dev_kfree_skb_any(skb);
				shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RX_BAD_DATA,
					"frm.ch is not same with rb.ch");
				continue;
			}
		}

		pass_skb_to_demux(mld, skb);
	}

	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}

	start_datalloc_timer(mld, &zdptr->datalloc_timer);

	return rcvd;
}

static int rx_ipc_frames_from_rb(struct sbd_ring_buffer *rb)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned int qlen = rb->len;
	unsigned int in = *rb->wp;
	unsigned int out = *rb->rp;
	unsigned int num_frames = circ_get_usage(qlen, in, out);

	while (rcvd < num_frames) {
		struct sk_buff *skb;

		skb = sbd_pio_rx(rb);
		if (!skb)
			return -ENOMEM;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd++;

		if (skbpriv(skb)->lnk_hdr) {
			u8 ch = rb->ch;
			u8 fch = sipc5_get_ch(skb->data);
			if (fch != ch) {
				mif_err("frm.ch:%d != rb.ch:%d\n", fch, ch);
				pr_skb("CRASH", skb);
				dev_kfree_skb_any(skb);
				shmem_forced_cp_crash(mld, CRASH_REASON_MIF_RX_BAD_DATA,
					"frm.ch is not same with rb.ch");
				continue;
			}
		}

		pass_skb_to_demux(mld, skb);
	}

	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}
	return rcvd;
}

#ifdef CONFIG_LINK_DEVICE_NAPI
static int shmem_poll_recv_on_iod(struct link_device *ld, struct io_device *iod,
		int budget)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct sbd_ring_buffer *rb = sbd_ch2rb(&mld->sbd_link_dev, iod->id, RX);
	int rcvd;
	int ret;

	if (rb->zerocopy)
		ret = rx_net_frames_from_zerocopy_adaptor(rb, budget, &rcvd);
	else
		ret = rx_net_frames_from_rb(rb, budget, &rcvd);

	if (IS_ERR_VALUE((unsigned long)ret))
		mif_err("RX error (%d)\n", ret);

	return rcvd;
}

static ktime_t rx_int_enable_time;
static ktime_t rx_int_disable_time;

static int shmem_enable_rx_int(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;

	if (likely(cp_online(mc))) {
		mld->rx_int_enable = 1;
		if (rx_int_disable_time) {
			rx_int_enable_time = ktime_get();
			mld->rx_int_disabled_time += ktime_to_us(
					ktime_sub(rx_int_enable_time,
						rx_int_disable_time));
			rx_int_enable_time = 0;
			rx_int_disable_time = 0;
		}

		if (mld->msi_irq_base_enabled == 0) {
			/* If PCIE link is on, MSI interrupt cannot be disabled */
			/*enable_irq(mld->msi_irq_base);*/
			mld->msi_irq_base_enabled = 1;
		}
	}

	return 0;
}

static int shmem_disable_rx_int(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;

	if (likely(cp_online(mc))) {
		mld->rx_int_enable = 0;
		rx_int_disable_time = ktime_get();

		if (mld->msi_irq_base_enabled == 1) {
			/* If PCIE link is on, MSI interrupt cannot be disabled */
			/*disable_irq(mld->msi_irq_base);*/
			mld->msi_irq_base_enabled = 0;
		}
	}

	return 0;
}
#endif /* CONFIG_LINK_DEVICE_NAPI */

static int recv_sbd_ipc_frames(struct mem_link_device *mld,
				struct mem_snapshot *mst, int budget)
{
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	int i;
	int total_ps_rcvd = 0;
	int total_non_ps_rcvd = 0;

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, RX);
		int rcvd = 0;

		if (unlikely(rb_empty(rb)))
			continue;

		if (likely(sipc_ps_ch(rb->ch))) {
			if (rb->zerocopy)
				rcvd = rx_net_frames_from_zerocopy_adaptor(rb, budget, &rcvd);
			else
				rcvd = rx_net_frames_from_rb(rb, budget, &rcvd);
			budget -= rcvd;
			total_ps_rcvd += rcvd;
		} else {
			if (rb->zerocopy)
				rcvd = rx_ipc_frames_from_zerocopy_adaptor(rb);
			else
				rcvd = rx_ipc_frames_from_rb(rb);
			total_non_ps_rcvd += rcvd;
		}

		if (rcvd < 0)
			return rcvd;
	}
	return total_ps_rcvd;
}

static void shmem_oom_handler_work(struct work_struct *ws)
{
	struct mem_link_device *mld =
		container_of(ws, struct mem_link_device, page_reclaim_work);
	struct sk_buff *skb;

	/* try to page reclaim with GFP_KERNEL */
	skb = alloc_skb(PAGE_SIZE - 512, GFP_KERNEL);
	if (skb)
		dev_kfree_skb_any(skb);

	/* need to disable the RX irq ?? */
	msleep(200);

	mif_info("trigger the rx task again\n");
	tasklet_schedule(&mld->rx_tsk);
}

static int ipc_rx_func(struct mem_link_device *mld, int budget)
{
	u32 qlen = mld->msb_rxq.qlen;
	int total_ps_rcvd = 0;
	int ps_rcvd = 0;

	while (qlen-- > 0) {
		struct mst_buff *msb;
		u16 intr;
		int ret = 0;

		msb = msb_dequeue(&mld->msb_rxq);
		if (!msb)
			break;

		intr = msb->snapshot.int2ap;

		if (cmd_valid(intr))
			mld->cmd_handler(mld, int2cmd(intr));

		if (sbd_active(&mld->sbd_link_dev)) {
			ps_rcvd = recv_sbd_ipc_frames(mld, &msb->snapshot, budget);
			if (ps_rcvd >= 0)
				total_ps_rcvd += ps_rcvd;
			else
				ret = ps_rcvd;

		} else
			ret = recv_ipc_frames(mld, &msb->snapshot);

		if (ret == -ENOMEM) {
			msb_queue_head(&mld->msb_rxq, msb);
			if (!work_pending(&mld->page_reclaim_work)) {
				struct link_device *ld = &mld->link_dev;
				mif_err_limited("Rx ENOMEM, try reclaim work");
				queue_work(ld->rx_wq,
						&mld->page_reclaim_work);
			}
			return 0;
		}
		msb_free(msb);
	}

	return total_ps_rcvd;
}

static void udl_rx_work(struct work_struct *ws)
{
	struct mem_link_device *mld;

	mld = container_of(ws, struct mem_link_device, udl_rx_dwork.work);

	ipc_rx_func(mld, 0);
}

static void shmem_rx_task(unsigned long data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (likely(cp_online(mc)))
		ipc_rx_func(mld, 0);
	else
		queue_delayed_work(ld->rx_wq, &mld->udl_rx_dwork, 0);
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_LINK_METHOD

static int shmem_init_comm(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	struct io_device *check_iod;
	int id = iod->id;
	int fmt2rfs = (SIPC5_CH_ID_RFS_0 - SIPC5_CH_ID_FMT_0);
	int rfs2fmt = (SIPC5_CH_ID_FMT_0 - SIPC5_CH_ID_RFS_0);

	if (atomic_read(&mld->cp_boot_done))
		return 0;

#ifdef CONFIG_LINK_CONTROL_MSG_IOSM
	if (mld->iosm) {
		struct sbd_link_device *sl = &mld->sbd_link_dev;
		struct sbd_ipc_device *sid = sbd_ch2dev(sl, iod->id);

		if (atomic_read(&sid->config_done)) {
			tx_iosm_message(mld, IOSM_A2C_OPEN_CH, (u32 *)&id);
			return 0;
		} else {
			mif_err("%s isn't configured channel\n", iod->name);
			return -ENODEV;
		}
	}
#endif

	switch (id) {
	case SIPC5_CH_ID_FMT_0 ... SIPC5_CH_ID_FMT_9:
		check_iod = link_get_iod_with_channel(ld, (id + fmt2rfs));
		if (check_iod ? atomic_read(&check_iod->opened) : true) {
			write_clk_table_to_shmem(mld);
			mif_err("%s: %s->INIT_END->%s\n",
				ld->name, iod->name, mc->name);
			send_ipc_irq(mld, cmd2int(CMD_INIT_END));
			atomic_set(&mld->cp_boot_done, 1);
		} else {
			mif_err("%s is not opened yet\n", check_iod->name);
		}
		break;

	case SIPC5_CH_ID_RFS_0 ... SIPC5_CH_ID_RFS_9:
		check_iod = link_get_iod_with_channel(ld, (id + rfs2fmt));
		if (check_iod) {
			if (atomic_read(&check_iod->opened)) {
				write_clk_table_to_shmem(mld);
				mif_err("%s: %s->INIT_END->%s\n",
					ld->name, iod->name, mc->name);
				send_ipc_irq(mld, cmd2int(CMD_INIT_END));
				atomic_set(&mld->cp_boot_done, 1);
			} else {
				mif_err("%s not opened yet\n", check_iod->name);
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

static void shmem_terminate_comm(struct link_device *ld, struct io_device *iod)
{
#ifdef CONFIG_LINK_CONTROL_MSG_IOSM
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (mld->iosm)
		tx_iosm_message(mld, IOSM_A2C_CLOSE_CH, (u32 *)&iod->id);
#endif
}

static int shmem_send(struct link_device *ld, struct io_device *iod,
		    struct sk_buff *skb)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	enum dev_format id = iod->format;
	u8 ch = iod->id;
	int ret = -ENODEV;

	cp_runtime_link(mc, LINK_SEND, iod->id);

	switch (id) {
	case IPC_RAW:
		if (sipc_ps_ch(iod->id)) {
			if (unlikely(atomic_read(&ld->netif_stopped) > 0)) {
				if (skb->queue_mapping != 1) {
					if (in_interrupt()) {
						mif_err("raw tx is suspended, drop size=%d\n",
								skb->len);
						ret = -EBUSY;
						goto exit;
					}

					mif_err("wait TX RESUME CMD...\n");
					reinit_completion(&ld->raw_tx_resumed);
					wait_for_completion_timeout(&ld->raw_tx_resumed,
							msecs_to_jiffies(3000));
					mif_err("TX resumed done.\n");
				} else {
					mif_err_limited("Tx_flowctrl, but received ack from ch %d\n", ch);
				}
			}
		}

	case IPC_RFS:
	case IPC_FMT:
		if (likely(sipc5_ipc_ch(ch)))
			ret = xmit_ipc(mld, iod, ch, skb);
		else
			ret = xmit_udl(mld, iod, ch, skb);

	case IPC_BOOT:
	case IPC_DUMP:
		if (sipc5_udl_ch(ch))
			ret = xmit_udl(mld, iod, ch, skb);
		break;

	default:
		break;
	}

exit:
	cp_runtime_dislink(mc, LINK_SEND, iod->id);

	return ret;
}

static void shmem_boot_on(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	unsigned long flags;

	atomic_set(&mld->cp_boot_done, 0);

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	cancel_tx_timer(mld, &mld->tx_timer);
	if (wake_lock_active(&mld->tx_timer_wlock))
		wake_unlock(&mld->tx_timer_wlock);

	if (ld->sbd_ipc) {
#ifdef CONFIG_LTE_MODEM_XMM7260
		sbd_deactivate(&mld->sbd_link_dev);
#endif
		cancel_tx_timer(mld, &mld->sbd_tx_timer);
		cancel_datalloc_timer(mld);
		if (wake_lock_active(&mld->sbd_tx_timer_wlock))
			wake_unlock(&mld->sbd_tx_timer_wlock);

		if (mld->iosm) {
			memset(mld->base + CMD_RGN_OFFSET, 0, CMD_RGN_SIZE);
			mif_info("Control message region has been initialized\n");
		}
	}

	purge_txq(mld);
}

static int shmem_xmit_boot(struct link_device *ld, struct io_device *iod,
		     unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct resource *cpmem_info = mld->syscp_info;
	void __iomem *dst;
	void __user *src;
	int err;
	struct modem_firmware mf;
	void __iomem *v_base;
	size_t valid_space;

	/**
	 * Get the information about the boot image
	 */
	memset(&mf, 0, sizeof(struct modem_firmware));

	err = copy_from_user(&mf, (const void __user *)arg, sizeof(mf));
	if (err) {
		mif_err("%s: ERR! INFO copy_from_user fail\n", ld->name);
		return -EFAULT;
	}

	/* Calculate size of valid space which BL will download */
	valid_space = (mf.mode) ? mld->size : mld->boot_size;
	/* Calculate base address (0: BOOT_MODE, 1: DUMP_MODE) */
	v_base = (mf.mode) ? mld->base : mld->boot_base;

	/**
	 * Check the size of the boot image
	 * fix the integer overflow of "mf.m_offset + mf.len" from Jose Duart
	 */
	if (mf.size > valid_space || mf.len > valid_space
			|| mf.m_offset > valid_space - mf.len) {
		mif_err("%s: ERR! Invalid args: size %x, offset %x, len %x\n",
			ld->name, mf.size, mf.m_offset, mf.len);
		return -EINVAL;
	}

	dst = (void __iomem *)(v_base + mf.m_offset);
	src = (void __user *)((unsigned long)mf.binary);

	if (mf.m_offset == (u32)cpmem_info->start)
	{
		mld->cp_binary_size = mf.size;
	}

	err = copy_from_user(dst, src, mf.len);
	if (err) {
		mif_err("%s: ERR! BOOT copy_from_user fail\n", ld->name);
		return err;
	}

	return 0;
}

#if !defined(CONFIG_CP_SECURE_BOOT)
unsigned long shmem_calculate_CRC32(const unsigned char *buf, unsigned long len)
{
	unsigned long ul_crc;

	if (0 == buf) return 0L;

	ul_crc = CRC32_XINIT;
	while (len--)
	{
		ul_crc = CRC32_TABLE[(ul_crc ^ *buf++) & 0xFF] ^ (ul_crc >> 8);
	}

	ul_crc ^= CRC32_XOROT;

	return ul_crc;
}

void shmem_check_modem_binary_crc(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct resource *cpmem_info = mld->syscp_info;
	unsigned char * data;
	unsigned long CRC;

	data = (unsigned char *)mld->boot_base + (u32)cpmem_info->start;

	CRC = shmem_calculate_CRC32(data, mld->cp_binary_size);

	mif_info("Modem Main Binary CRC: %08X\n", (unsigned int)CRC);

}
#endif

int cp_init_done;
static int shmem_security_request(struct link_device *ld, struct io_device *iod,
				unsigned long arg)
{
	unsigned long param2, param3;
	int err = 0;
	struct modem_sec_req msr;

	err = copy_from_user(&msr, (const void __user *)arg, sizeof(msr));
	if (err) {
		mif_err("%s: ERR! copy_from_user fail\n", ld->name);
		err = -EFAULT;
		goto exit;
	}

	param2 = shm_get_security_param2(msr.mode, msr.param2);
	param3 = shm_get_security_param3(msr.mode, msr.param3);

#if !defined(CONFIG_CP_SECURE_BOOT)
	if (msr.mode == 0)
		shmem_check_modem_binary_crc(ld);
#else
	exynos_smc(SMC_ID_CLK, SSS_CLK_ENABLE, 0, 0);
	if (shm_get_use_cp_memory_map_flag() && msr.mode == 0)
		msr.mode |= (unsigned int)shm_get_phys_base();

	if (mc->ap2cp_cfg_ioaddr && !cp_init_done) {
		if (cal_cp_status() == 0) {
			mif_info("CP first Init!\n");
			cal_cp_init();
		}
		cp_init_done = 1;
	}

	mif_err("mode=%u, size=0x%lx, addr=0x%lx, cp_base_addr=0x%lx\n",
			msr.mode, param2, param3, shm_get_phys_base());
	err = exynos_smc(SMC_ID, msr.mode, param2, param3);
	exynos_smc(SMC_ID_CLK, SSS_CLK_DISABLE, 0, 0);
	mif_info("%s: return_value=0x%08x(%s)\n", ld->name, err,
			err < sizeof(smc_err_string) ? smc_err_string[err] : "NULL");
#endif

exit:
	return err;
}

#ifdef CONFIG_MODEM_IF_NET_GRO
static long gro_flush_time = 100000L;
module_param(gro_flush_time, long, 0644);

static void gro_flush_timer(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct timespec curr, diff;

	if (!gro_flush_time)
		return;

	if (unlikely(mld->flush_time.tv_sec == 0)) {
		getnstimeofday(&mld->flush_time);
	} else {
		getnstimeofday(&(curr));
		diff = timespec_sub(curr, mld->flush_time);
		if ((diff.tv_sec > 0) || (diff.tv_nsec > gro_flush_time)) {
			napi_gro_flush(napi_get_current(), false);
			getnstimeofday(&mld->flush_time);
		}
	}
}
#endif

#ifdef CONFIG_LINK_DEVICE_NAPI
static int shmem_enqueue_snapshot(struct mem_link_device *mld);

/*
 * mld_rx_int_poll
 *
 * This NAPI poll function does not handle reception of any network frames.
 * It is used for servicing CP2AP commands and FMT RX frames while the RX
 * mailbox interrupt is masked. When the mailbox interrupt is masked, CP can
 * set the interrupt but the AP will not react. However, the interrupt status
 * bit will still be set, so we can poll the status bit to handle new RX
 * interrupts.
 * If the RAW NAPI functions are no longer scheduled at the end of this poll
 * function, we can enable the mailbox interrupt and stop polling.
 */
static int mld_rx_int_poll(struct napi_struct *napi, int budget)
{
	struct mem_link_device *mld = container_of(napi, struct mem_link_device,
			mld_napi);
	struct link_device *ld = &mld->link_dev;
/*	struct modem_ctl *mc = ld->mc;
 *	struct sbd_link_device *sl = &mld->sbd_link_dev;
 */
	int total_ps_rcvd = 0;
/*	int ps_rcvd = 0;
 *	int i;
 *	int ret = 0;
 */
	int total_budget;

#if 1 /* Simple NAPI */
	if (shmem_enqueue_snapshot(mld))
		goto dummy_poll_complete;

	mld->rx_poll_count++;

	total_budget = budget;

	total_ps_rcvd = ipc_rx_func(mld, budget);

	if (total_ps_rcvd) {
		if (total_ps_rcvd < total_budget) {
			napi_complete_done(napi, total_ps_rcvd);
			ld->enable_rx_int(ld);
		}
	} else {
		napi_complete(napi);
		ld->enable_rx_int(ld);
	}

	return total_ps_rcvd;

#else
	ret = mbox_check_irq(MCU_CP, mld->irq_cp2ap_msg);
	if (IS_ERR_VALUE((unsigned long)ret))
		goto dummy_poll_complete;

	mld->rx_poll_count++;

	if (ret) {
		/* If there was a new interrupt, do what irq_handler would have done. */
		if (shmem_enqueue_snapshot(mld))
			goto dummy_poll_complete;

		if (likely(cp_online(mc))) {
			total_ps_rcvd = ipc_rx_func(mld, budget);
		} else
			queue_delayed_work(ld->rx_wq, &mld->udl_rx_dwork, 0);

		if (total_ps_rcvd) {
			if (total_ps_rcvd < budget) {
				napi_complete_done(napi, total_ps_rcvd);
				ld->enable_rx_int(ld);
			}
			return total_ps_rcvd;
		} else
			goto dummy_poll_complete;
	} else {
		/* Leave interrupt disabled and poll if NET polling is not finished. */
		total_budget = budget;
		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, RX);
			if (likely(sipc_ps_ch(rb->ch))) {
				ps_rcvd = shmem_poll_recv_on_iod(ld, rb->iod, budget);
				budget -= ps_rcvd;
				total_ps_rcvd += ps_rcvd;
			}
		}

		if (total_ps_rcvd) {
			if (total_ps_rcvd < total_budget) {
				napi_complete_done(napi, total_ps_rcvd);
				ld->enable_rx_int(ld);
			}
		} else {
			napi_complete(napi);
			ld->enable_rx_int(ld);
		}

		return total_ps_rcvd;
	}
#endif

dummy_poll_complete:
	napi_complete(napi);
	ld->enable_rx_int(ld);

	return 0;
}

static void sync_net_dev(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	napi_synchronize(&mld->mld_napi);
	mif_info("%s\n", netdev_name(&mld->dummy_net));
}
#endif /* CONFIG_LINK_DEVICE_NAPI */

static int shmem_start_download(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (ld->sbd_ipc && mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		sbd_deactivate(&mld->sbd_link_dev);

#ifdef CONFIG_LINK_DEVICE_NAPI
	sync_net_dev(ld);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	reset_ipc_map(mld);

	if (mld->attrs & LINK_ATTR(LINK_ATTR_BOOT_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	if (mld->dpram_magic) {
		unsigned int magic;

		set_magic(mld, MEM_BOOT_MAGIC);
		magic = get_magic(mld);
		if (magic != MEM_BOOT_MAGIC) {
			mif_err("%s: ERR! magic 0x%08X != BOOT_MAGIC 0x%08X\n",
				ld->name, magic, MEM_BOOT_MAGIC);
			return -EFAULT;
		}
		mif_err("%s: magic == 0x%08X\n", ld->name, magic);
	}

	return 0;
}

static int shmem_update_firm_info(struct link_device *ld, struct io_device *iod,
				unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret;

	ret = copy_from_user(&mld->img_info, (void __user *)arg,
			     sizeof(struct std_dload_info));
	if (ret) {
		mif_err("ERR! copy_from_user fail!\n");
		return -EFAULT;
	}

	return 0;
}

static int shmem_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	u32 crash_type = ld->crash_type;

	mif_err("+++\n");
	shmem_forced_cp_crash(mld, crash_type,
		"forced crash is called by ioctl command");
	mif_err("---\n");
	return 0;
}

static int shmem_start_upload(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (ld->sbd_ipc && mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		sbd_deactivate(&mld->sbd_link_dev);

#ifdef CONFIG_LINK_DEVICE_NAPI
	sync_net_dev(ld);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	reset_ipc_map(mld);

	if (mld->attrs & LINK_ATTR(LINK_ATTR_DUMP_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	if (mld->dpram_magic) {
		unsigned int magic;

		set_magic(mld, MEM_DUMP_MAGIC);
		magic = get_magic(mld);
		if (magic != MEM_DUMP_MAGIC) {
			mif_err("%s: ERR! magic 0x%08X != DUMP_MAGIC 0x%08X\n",
				ld->name, magic, MEM_DUMP_MAGIC);
			return -EFAULT;
		}
		mif_err("%s: magic == 0x%08X\n", ld->name, magic);
	}

	return 0;
}

static void shmem_close_tx(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	unsigned long flags;

	spin_lock_irqsave(&mld->state_lock, flags);
	mld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&mld->state_lock, flags);

	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);

	stop_net_ifaces(ld);
	purge_txq(mld);
}

static int shmem_crash_reason(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret;

	ret = copy_to_user((void __user *)arg, &mld->crash_reason,
		sizeof(struct crash_reason));
	if (ret) {
		mif_err("ERR! copy_to_user fail!\n");
		return -EFAULT;
	}

	return 0;
}

static int shmem_airplane_mode(struct link_device *ld, struct io_device *iod,
		unsigned long arg)
{
	struct modem_ctl *mc = ld->mc;
	struct mem_link_device *mld = to_mem_link_device(ld);

	mc->airplane_mode = (unsigned int)arg;
	mif_info("Set airplane mode: %d\n", mc->airplane_mode);

	if (mc->airplane_mode)
		shmem_unlock_mif_freq(mld);
	else
		shmem_restore_mif_freq(mld);

	return 0;
}
#endif

/*============================================================================*/


static u16 recv_cp2ap_irq(struct mem_link_device *mld)
{
	return (u16) ioread16(mld->cp2ap_msg);
}

static u16 recv_cp2ap_status(struct mem_link_device *mld)
{
	return (u16) ioread16(mld->cp2ap_united_status);
}

static void send_ap2cp_irq(struct mem_link_device *mld, u16 mask)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	int wait_counter = 0;

	if ((mc->phone_state != STATE_ONLINE) &&
			(mc->phone_state != STATE_CRASH_EXIT)) {
		mif_info("Send Doorbell in Unexpected state : ld(%d), mc(%d)\n",
				mc->phone_state, mc->phone_state);
		return;
	}

	if (in_interrupt() || irqs_disabled()) {
		/* If runtime PM state is not RPM_ACTIVE */
		while (mc->dev->power.runtime_status != RPM_ACTIVE) {
			udelay(100);
			if (++wait_counter == 10) {
				mif_err("Can't send interrupt in tasklet!!!\n");
				mif_err("Reserve doorbell interrupt\n");
				iowrite32(mask, mld->ap2cp_msg);
				mc->reserve_doorbell_int = true;
				return ;
			}
		}

	}

	iowrite32(mask, mld->ap2cp_msg);

	if (s5100pcie_send_doorbell_int(mld->intval_ap2cp_msg) != 0)
		s5100_force_crash_exit_ext();
}

static inline u16 read_ap2cp_irq(struct mem_link_device *mld)
{
	return ioread16(mld->ap2cp_msg);
}

#define SHMEM_SRINFO_OFFSET		0x800  /* 4KB - 2KB */
#define SHMEM_SRINFO_SBD_OFFSET	0xF800 /* 64KB - 2KB */
#define SHMEM_SRINFO_SIZE		0x800
#define SHMEM_SRINFO_DATA_STR	64
#define SHMEM_BOOTLOG_BASE		0xC00
#define SHMEM_BOOTLOG_BUFF		0x1FF
#define SHMEM_BOOTLOG_OFFSET	0x4

struct shmem_srinfo {
	unsigned int size;
	char buf[0];
};

static char *shmem_get_srinfo_address(struct link_device *ld)
{
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned offs = (mld->link_dev.sbd_ipc) ?
		SHMEM_SRINFO_SBD_OFFSET : SHMEM_SRINFO_OFFSET;
	char *base = mld->base + offs;

	return base;
}

/* not in use */
static int shmem_ioctl(struct link_device *ld, struct io_device *iod,
		       unsigned int cmd, unsigned long arg)
{
	mif_err("%s: cmd 0x%08X\n", ld->name, cmd);

	switch (cmd) {
	case IOCTL_MODEM_GET_SHMEM_SRINFO:
	{
		struct shmem_srinfo __user *sr_arg =
			(struct shmem_srinfo __user *)arg;
		unsigned count, size = SHMEM_SRINFO_SIZE;

		if (copy_from_user(&count, &sr_arg->size, sizeof(unsigned)))
			return -EFAULT;

		mif_info("get srinfo:%s, size = %d\n", iod->name, count);

		size = min(size, count);
		if (copy_to_user(&sr_arg->size, &size, sizeof(unsigned)))
			return -EFAULT;

		if (copy_to_user(sr_arg->buf, shmem_get_srinfo_address(ld),
			size))
			return -EFAULT;
		break;
	}

	case IOCTL_MODEM_SET_SHMEM_SRINFO:
	{
		struct shmem_srinfo __user *sr_arg =
			(struct shmem_srinfo __user *)arg;
		unsigned count, size = SHMEM_SRINFO_SIZE;

		if (copy_from_user(&count, &sr_arg->size, sizeof(unsigned)))
			return -EFAULT;

		mif_info("set srinfo:%s, size = %d\n", iod->name, count);

		if (copy_from_user(shmem_get_srinfo_address(ld), sr_arg->buf,
			min(count, size)))
			return -EFAULT;
		break;
	}

	case IOCTL_MODEM_GET_CP_BOOTLOG:
	{
		struct mem_link_device *mld = ld_to_mem_link_device(ld);
		u8 __iomem *base = mld->base + SHMEM_BOOTLOG_BASE;
		char str[SHMEM_BOOTLOG_BUFF];
		unsigned int size = base[0]        + (base[1] << 8)
				 + (base[2] << 16) + (base[3] << 24);

		if (size <= 0 || size > SHMEM_BOOTLOG_BUFF - SHMEM_BOOTLOG_OFFSET) {
			mif_info("Invalid CP boot log[%d]\n", size);
			return -EINVAL;
		}

		strncpy(str, base + SHMEM_BOOTLOG_OFFSET, size);
		mif_info("CP boot log[%d] : %s\n", size, str);
		break;
	}

	case IOCTL_MODEM_CLR_CP_BOOTLOG:
	{
		struct mem_link_device *mld = ld_to_mem_link_device(ld);
		u8 __iomem *base = mld->base + SHMEM_BOOTLOG_BASE;

		mif_info("Clear CP boot log\n");
		memset(base, 0, SHMEM_BOOTLOG_BUFF);

		break;
	}

	default:
		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		return -EINVAL;
	}

	return 0;
}

void shmem_tx_state_handler(void *data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	u16 int2ap_status;

	int2ap_status = mld->recv_cp2ap_status(mld);

	/* Change SHM_FLOWCTL to MASK_TX_FLOWCTRL */
	int2ap_status = (int2ap_status & SHM_FLOWCTL_BIT) << 2;

	switch (int2ap_status & (SHM_FLOWCTL_BIT << 2)) {
	case MASK_TX_FLOWCTL_SUSPEND:
		if (!chk_same_cmd(mld, int2ap_status))
			tx_flowctrl_suspend(mld);
		break;

	case MASK_TX_FLOWCTL_RESUME:
		if (!chk_same_cmd(mld, int2ap_status))
			tx_flowctrl_resume(mld);
		break;

	default:
		break;
	}

	if (unlikely(!rx_possible(mc))) {
		mif_err("%s: ERR! %s.state == %s\n", ld->name, mc->name,
			mc_state(mc));
		return;
	}
}

static int shmem_enqueue_snapshot(struct mem_link_device *mld)
{
	struct mst_buff *msb;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	msb = mem_take_snapshot(mld, RX);
	if (!msb)
		return -ENOMEM;

	if (unlikely(!int_valid(msb->snapshot.int2ap))) {
		mif_err("%s: ERR! invalid intr 0x%X\n",
				ld->name, msb->snapshot.int2ap);
		msb_free(msb);
		return -EINVAL;
	}

	if (unlikely(!rx_possible(mc))) {
		mif_err("%s: ERR! %s.state == %s\n", ld->name, mc->name,
			mc_state(mc));
		msb_free(msb);
		return -EINVAL;
	}

	msb_queue_tail(&mld->msb_rxq, msb);

	return 0;
}

static irqreturn_t shmem_irq_handler(int irq, void *data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;

#ifdef CONFIG_LINK_DEVICE_NAPI
	mld->rx_int_count++;
	if (napi_schedule_prep(&mld->mld_napi)) {
		struct link_device *ld = &mld->link_dev;

		ld->disable_rx_int(ld);
		__napi_schedule(&mld->mld_napi);
	}
#else /* !CONFIG_LINK_DEVICE_NAPI */
	if (shmem_enqueue_snapshot(mld))
		return IRQ_HANDLED;

	tasklet_schedule(&mld->rx_tsk);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	return IRQ_HANDLED;
}

static struct pm_qos_request pm_qos_req_mif;
static struct pm_qos_request pm_qos_req_cl0;
static struct pm_qos_request pm_qos_req_cl1;
static struct pm_qos_request pm_qos_req_int;

static void shmem_qos_work_cpu(struct work_struct *work)
{
	struct mem_link_device *mld =
		container_of(work, struct mem_link_device, pm_qos_work_cpu);
	int qos_val;
	int cpu_val;

	qos_val = ioread32(mld->cp2ap_dvfsreq_cpu);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	cpu_val = (qos_val & 0xff);
	if (qos_val & BIT(31)) {
		/* CL0 - Big */
		if (cpu_val > 0 && cpu_val <= mld->cl0_table.num_of_table) {
			mif_err("Lock CL0[%d] : %u\n", cpu_val,
					mld->cl0_table.freq[cpu_val - 1]);
			pm_qos_update_request(&pm_qos_req_cl0,
				mld->cl0_table.freq[cpu_val - 1]);
		} else {
			mif_err("Unlock CL0(req_val : %d)\n", qos_val);
			pm_qos_update_request(&pm_qos_req_cl0, 0);
		}
	} else {
		/* CL1 - Little */
		if (cpu_val > 0 && cpu_val <= mld->cl1_table.num_of_table) {
			mif_err("Lock CL1[%d] : %u\n", cpu_val,
					mld->cl1_table.freq[cpu_val - 1]);
			pm_qos_update_request(&pm_qos_req_cl1,
				mld->cl1_table.freq[cpu_val - 1]);
		} else {
			mif_err("Unlock CL1(req_val : %d)\n", qos_val);
			pm_qos_update_request(&pm_qos_req_cl1, 0);
		}
	}
}

static void shmem_qos_work_mif(struct work_struct *work)
{
	struct mem_link_device *mld =
		container_of(work, struct mem_link_device, pm_qos_work_mif);
	int qos_val;
	int mif_val;

	qos_val = ioread32(mld->cp2ap_dvfsreq_mif);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	mif_val = (qos_val & 0xff);
	if (mif_val > 0 && mif_val <= mld->mif_table.num_of_table) {
		mif_err("Lock MIF[%d] : %u\n", mif_val,
				mld->mif_table.freq[mif_val - 1]);
		pm_qos_update_request(&pm_qos_req_mif, mld->mif_table.freq[mif_val - 1]);
	} else {
		mif_err("Unlock MIF(req_val : %d)\n", qos_val);
		pm_qos_update_request(&pm_qos_req_mif, 0);
	}

	/* Save current mif_val */
	mld->current_mif_val = mif_val;
}

static void shmem_qos_work_int(struct work_struct *work)
{
	struct mem_link_device *mld =
		container_of(work, struct mem_link_device, pm_qos_work_int);
	int qos_val;
	int int_val;

	qos_val = ioread32(mld->cp2ap_dvfsreq_int);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	int_val = (qos_val & 0xff);
	if (int_val > 0 && int_val <= mld->int_table.num_of_table) {
		mif_err("Lock INT[%d] : %u\n", int_val,
				mld->int_table.freq[int_val - 1]);
		pm_qos_update_request(&pm_qos_req_int, mld->int_table.freq[int_val - 1]);
	} else {
		mif_err("Unlock INT(req_val : %d)\n", qos_val);
		pm_qos_update_request(&pm_qos_req_int, 0);
	}
}

void shmem_unlock_mif_freq(struct mem_link_device *mld)
{
	if (mld) {
		mif_err("Currnet MIF value: %d)\n", mld->current_mif_val);
		mif_err("Unlock MIF\n");
		pm_qos_update_request(&pm_qos_req_mif, 0);
	}
}

void shmem_restore_mif_freq(struct mem_link_device *mld)
{
	int mif_val;

	if (mld) {
		mif_val = mld->current_mif_val;
		mif_err("Currnet MIF value: %d)\n", mif_val);

		if (mif_val > 0 && mif_val <= mld->mif_table.num_of_table) {
			mif_err("Lock MIF[%d] : %u\n", mif_val,
					mld->mif_table.freq[mif_val - 1]);
			pm_qos_update_request(&pm_qos_req_mif, mld->mif_table.freq[mif_val - 1]);
		}
	}
}

#if defined(CONFIG_ECT)
static int exynos_devfreq_parse_ect(struct mem_link_device *mld, char *dvfs_domain_name)
{
	int i, counter = 0;
	void *dvfs_block;
	struct ect_dvfs_domain *dvfs_domain;

	dvfs_block = ect_get_block(BLOCK_DVFS);
	if (dvfs_block == NULL)
		return -ENODEV;

	dvfs_domain = ect_dvfs_get_domain(dvfs_block, (char *)dvfs_domain_name);
	if (dvfs_domain == NULL)
		return -ENODEV;

	if (!strcmp(dvfs_domain_name, "dvfs_mif")) {
		mld->mif_table.num_of_table = dvfs_domain->num_of_level;
		for (i = dvfs_domain->num_of_level - 1; i >= 0; i--) {
			mld->mif_table.freq[i] = dvfs_domain->list_level[counter++].level;
			mif_err("MIF_LEV[%d] : %u\n", i + 1, mld->mif_table.freq[i]);
		}
	} else if (!strcmp(dvfs_domain_name, "dvfs_cpucl0")) {
		mld->cl0_table.num_of_table = dvfs_domain->num_of_level;
		for (i = dvfs_domain->num_of_level - 1; i >= 0; i--) {
			mld->cl0_table.freq[i] = dvfs_domain->list_level[counter++].level;
			mif_err("CL0_LEV[%d] : %u\n", i + 1, mld->cl0_table.freq[i]);
		}
	} else if (!strcmp(dvfs_domain_name, "dvfs_cpucl1")) {
		mld->cl1_table.num_of_table = dvfs_domain->num_of_level;
		for (i = dvfs_domain->num_of_level - 1; i >= 0; i--) {
			mld->cl1_table.freq[i] = dvfs_domain->list_level[counter++].level;
			mif_err("CL1_LEV[%d] : %u\n", i + 1, mld->cl1_table.freq[i]);
		}
	} else if (!strcmp(dvfs_domain_name, "dvfs_int")) {
		mld->int_table.num_of_table = dvfs_domain->num_of_level;
		for (i = dvfs_domain->num_of_level - 1; i >= 0; i--) {
			mld->int_table.freq[i] = dvfs_domain->list_level[counter++].level;
			mif_err("INT_LEV[%d] : %u\n", i + 1, mld->int_table.freq[i]);
		}
	}

	return 0;
}
#else
static int exynos_devfreq_parse_ect(struct mem_link_device *mld, char *dvfs_domain_name)
{
	mif_err("ECT is not defined(%s)\n", __func__);

	mld->cl0_table.num_of_table = 0;
	mld->cl1_table.num_of_table = 0;
	mld->mif_table.num_of_table = 0;
	mld->int_table.num_of_table = 0;

	return 0;
}
#endif

static void remap_4mb_map_to_ipc_dev(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct shmem_4mb_phys_map *map;
	struct mem_ipc_device *dev;

	map = (struct shmem_4mb_phys_map *)mld->base;

	/* magic code and access enable fields */
	mld->magic = (u32 __iomem *)&map->magic;
	mld->access = (u32 __iomem *)&map->access;

	/* To share ECT clock table with CP */
	mld->clk_table = (u32 __iomem *)map->reserved_first;

	/* IPC_MAP_FMT */
	dev = &mld->ipc_dev[IPC_MAP_FMT];

	dev->id = IPC_MAP_FMT;
	strcpy(dev->name, "FMT");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->fmt_tx_head;
	dev->txq.tail = &map->fmt_tx_tail;
	dev->txq.buff = &map->fmt_tx_buff[0];
	dev->txq.size = SHM_4M_FMT_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->fmt_rx_head;
	dev->rxq.tail = &map->fmt_rx_tail;
	dev->rxq.buff = &map->fmt_rx_buff[0];
	dev->rxq.size = SHM_4M_FMT_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_FMT;
	dev->req_ack_mask = MASK_REQ_ACK_FMT;
	dev->res_ack_mask = MASK_RES_ACK_FMT;

	dev->skb_txq = &ld->skb_txq[IPC_MAP_FMT];
	dev->skb_rxq = &ld->skb_rxq[IPC_MAP_FMT];
	skb_queue_head_init(dev->skb_txq);
	skb_queue_head_init(dev->skb_rxq);

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_MAP_FMT] = dev;

#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	/* IPC_MAP_HPRIO_RAW */
	dev = &mld->ipc_dev[IPC_MAP_HPRIO_RAW];

	dev->id = IPC_MAP_HPRIO_RAW;
	strcpy(dev->name, "HPRIO_RAW");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->raw_hprio_tx_head;
	dev->txq.tail = &map->raw_hprio_tx_tail;
	dev->txq.buff = &map->raw_hprio_tx_buff[0];
	dev->txq.size = SHM_4M_RAW_HPRIO_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->raw_hprio_rx_head;
	dev->rxq.tail = &map->raw_hprio_rx_tail;
	dev->rxq.buff = &map->raw_hprio_rx_buff[0];
	dev->rxq.size = SHM_4M_RAW_HPRIO_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_RAW;
	dev->req_ack_mask = MASK_REQ_ACK_RAW;
	dev->res_ack_mask = MASK_RES_ACK_RAW;

	dev->skb_txq = &ld->skb_txq[IPC_MAP_HPRIO_RAW];
	dev->skb_rxq = &ld->skb_rxq[IPC_MAP_HPRIO_RAW];
	skb_queue_head_init(dev->skb_txq);
	skb_queue_head_init(dev->skb_rxq);

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_MAP_HPRIO_RAW] = dev;
#endif

	/* IPC_MAP_NORM_RAW */
	dev = &mld->ipc_dev[IPC_MAP_NORM_RAW];

	dev->id = IPC_MAP_NORM_RAW;
	strcpy(dev->name, "NORM_RAW");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->raw_tx_head;
	dev->txq.tail = &map->raw_tx_tail;
	dev->txq.buff = &map->raw_tx_buff[0];
	dev->txq.size = SHM_4M_RAW_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->raw_rx_head;
	dev->rxq.tail = &map->raw_rx_tail;
	dev->rxq.buff = &map->raw_rx_buff[0];
	dev->rxq.size = SHM_4M_RAW_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_RAW;
	dev->req_ack_mask = MASK_REQ_ACK_RAW;
	dev->res_ack_mask = MASK_RES_ACK_RAW;

	dev->skb_txq = &ld->skb_txq[IPC_MAP_NORM_RAW];
	dev->skb_rxq = &ld->skb_rxq[IPC_MAP_NORM_RAW];
	skb_queue_head_init(dev->skb_txq);
	skb_queue_head_init(dev->skb_rxq);

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_MAP_NORM_RAW] = dev;

	mld->ap2cp_msg = (u32 __iomem *)&map->ap2cp_msg;
	mld->cp2ap_msg = (u32 __iomem *)&map->cp2ap_msg;
	mld->ap2cp_united_status = (u32 __iomem *)&map->ap2cp_united_status;
	mld->cp2ap_united_status = (u32 __iomem *)&map->cp2ap_united_status;
	mld->ap2cp_mif_freq = (u32 __iomem *)&map->ap2cp_mif_freq;
	mld->cp2ap_dvfsreq_cpu = (u32 __iomem *)&map->cp2ap_dvfsreq_cpu;
	mld->cp2ap_dvfsreq_mif = (u32 __iomem *)&map->cp2ap_dvfsreq_mif;
	mld->cp2ap_dvfsreq_int = (u32 __iomem *)&map->cp2ap_dvfsreq_int;
	mld->ap2cp_kerneltime = (u32 __iomem *)&map->ap2cp_kerneltime;
	mld->cp2ap_int_state = (u32 __iomem *)&map->cp2ap_int_state;

}

static int shmem_rx_setup(struct link_device *ld)
{
	ld->rx_wq = alloc_workqueue(
			"mem_rx_work", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 1);
	if (!ld->rx_wq) {
		mif_err("%s: ERR! fail to create rx_wq\n", ld->name);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&ld->rx_delayed_work, link_to_demux_work);

	return 0;
}

static ssize_t tx_period_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;
	return sprintf(buf, "%d\n", modem->mld->tx_period_ms);
}

static ssize_t tx_period_ms_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;

	ret = sscanf(buf, "%u", &modem->mld->tx_period_ms);
	if (ret != 1)
		return -EINVAL;

	ret = count;
	return ret;
}

int request_pcie_msi_int(struct link_device *ld,
				struct platform_device *pdev)
{
	int ret, base_irq;
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct device *dev = &pdev->dev;

	pr_err("Request MSI interrups.\n");

        base_irq = s5100pcie_request_msi_int(4);

	pr_err("Request MSI interrups. : BASE_IRQ(%d)\n", base_irq);
	mld->msi_irq_base = base_irq;

	if (base_irq <= 0) {
		pr_err("Can't get MSI IRQ!!!\n");
		return -EFAULT;
	}

	ret = devm_request_irq(dev, base_irq,
			shmem_irq_handler, IRQF_SHARED, "mif_cp2ap_msg", mld);
	if (ret) {
		pr_err("Can't request cp2ap_msg interrupt!!!\n");
		return -EIO;
	}

	mld->msi_irq_base_enabled = 1;
	mld->s5100_pdev = s5100pcie_get_pcidev();

	return base_irq;
}

static int shmem_register_pcie(struct link_device *ld)
{
	struct modem_ctl *mc = ld->mc;
	struct platform_device *pdev = to_platform_device(mc->dev);
	static int is_registered = 0;
	struct mem_link_device *mld = to_mem_link_device(ld);

	mif_err("S5100 EP driver initialization start.\n");

	msleep(200);

	cp_runtime_link(ld->mc, LINK_REGISTER_PCI, 0);

	if (is_registered == 0) {
		s5100pcie_init(mc->pcie_ch_num);
		mc->s5100_pdev = s5100pcie_get_pcidev();
		if (mc->s5100_pdev == NULL) {
			mif_err("s5100_pdev is NULL. Check if CP wake up is done.\n");
			return -EINVAL;
		}
		/* DBG: mif_info("%s:call l1ss_ctrl(ENABLE) when is_registered = 0\n", __func__); */
		/* exynos_pcie_host_v1_l1ss_ctrl(1, PCIE_L1SS_CTRL_MODEM_IF); */

		is_registered = 1;
		request_pcie_msi_int(ld, pdev);
	} else {
		if (mc->phone_state == STATE_CRASH_RESET) {
			print_msi_register();
			enable_irq(mld->msi_irq_base);
		}
	}

	mc->pcie_registered = true;

	mif_err("S5100 EP driver initialization end.\n");

	return 0;
}

static int rb_ch_id;
static ssize_t rb_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	struct sbd_link_device *sl;
	struct sbd_ring_buffer *rb_tx, *rb_rx;
	modem = (struct modem_data *)dev->platform_data;
	sl = &modem->mld->sbd_link_dev;
	rb_tx = sbd_id2rb(sl, rb_ch_id, TX);
	rb_rx = sbd_id2rb(sl, rb_ch_id, RX);

	if (rb_tx->len && rb_rx->len)
		return sprintf(buf, "rb_ch_id = %d (total: %d)\n"
				"TX(len: %d, rp: %d, wp: %d, space: %d, usage: %d)\n"
				"RX(len: %d, rp: %d, pre_rp: %d,wp: %d, space: %d, usage: %d)\n",
				rb_ch_id, sl->num_channels,
				rb_tx->len, *rb_tx->rp, *rb_tx->wp, rb_space(rb_tx) + 1, rb_usage(rb_tx),
				rb_rx->len, *rb_rx->rp, rb_rx->zerocopy ? rb_rx->zdptr->pre_rp : -1,
				*rb_rx->wp, rb_space(rb_rx) + 1, rb_usage(rb_rx));
	else
		return sprintf(buf, "rb_ch_id = %d(of %d), TX(empty), RX(empty)\n",
				rb_ch_id, sl->num_channels);

}

static ssize_t rb_info_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	struct modem_data *modem;
	struct sbd_link_device *sl;
	int id = 0;

	modem = (struct modem_data *)dev->platform_data;
	sl = &modem->mld->sbd_link_dev;

	ret = sscanf(buf, "%u", &id);
	if ((ret != 1) || (id < 0) || (id >= sl->num_channels))
		return -EINVAL;

	rb_ch_id = id;
	ret = count;
	return ret;
}

#if defined(CONFIG_CP_ZEROCOPY)
static ssize_t zmc_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;

	return sprintf(buf, "memcpy_packet(%d)/zeromemcpy_packet(%d)\n",
			modem->mld->memcpy_packet_count, modem->mld->zeromemcpy_packet_count);
}

static ssize_t zmc_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct modem_data *modem;
	int val = 0;
	int ret;
	modem = (struct modem_data *)dev->platform_data;
	ret = sscanf(buf, "%u", &val);

	if (val == 0) {
		modem->mld->memcpy_packet_count = 0;
		modem->mld->zeromemcpy_packet_count = 0;
	}

	return count;
}

static ssize_t mif_buff_mng_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!g_mif_buff_mng)
		return 0;

	return sprintf(buf, "used(%d)/free(%d)/total(%d)\n",
			g_mif_buff_mng->used_cell_count, g_mif_buff_mng->free_cell_count,
			g_mif_buff_mng->cell_count);
}

static ssize_t force_use_memcpy_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;
	return sprintf(buf, "%d\n", modem->mld->force_use_memcpy);
}

static ssize_t force_use_memcpy_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{

	struct modem_data *modem;
	int val = 0;
	int ret;
	modem = (struct modem_data *)dev->platform_data;
	ret = sscanf(buf, "%u", &val);

	if (val == 0)
		modem->mld->force_use_memcpy = 0;
	else if (val == 1)
		modem->mld->force_use_memcpy = 1;
	return count;
}
#endif

static DEVICE_ATTR_RW(tx_period_ms);
static DEVICE_ATTR_RW(rb_info);
#if defined(CONFIG_CP_ZEROCOPY)
static DEVICE_ATTR_RO(mif_buff_mng);
static DEVICE_ATTR_RW(zmc_count);
static DEVICE_ATTR_RW(force_use_memcpy);
#endif

static struct attribute *shmem_attrs[] = {
	&dev_attr_tx_period_ms.attr,
	&dev_attr_rb_info.attr,
#if defined(CONFIG_CP_ZEROCOPY)
	&dev_attr_mif_buff_mng.attr,
	&dev_attr_zmc_count.attr,
	&dev_attr_force_use_memcpy.attr,
#endif
	NULL,
};

static const struct attribute_group shmem_group = {		\
	.attrs = shmem_attrs,					\
	.name = "shmem",
};

#ifdef CONFIG_LINK_DEVICE_NAPI
static ssize_t rx_napi_list_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	struct napi_struct *n;
	struct net_device *netdev;
	ssize_t count = 0;

	modem = (struct modem_data *)dev->platform_data;
	netdev = &modem->mld->dummy_net;

	count += sprintf(&buf[count], "[%s`s napi_list]\n", netdev_name(netdev));
	list_for_each_entry(n, &netdev->napi_list, dev_list)
		count += sprintf(&buf[count], "state: %s(%ld), weight: %d, poll: 0x%pK\n",
				test_bit(NAPI_STATE_SCHED, &n->state) ? "NAPI_STATE_SCHED" : "NAPI_STATE_COMPLETE",
				n->state, n->weight, (void *)n->poll);
	return count;
}

static ssize_t rx_int_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;
	return sprintf(buf, "%d\n", modem->mld->rx_int_enable);
}

static ssize_t rx_int_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;
	return sprintf(buf, "%d\n", modem->mld->rx_int_count);
}

static ssize_t rx_int_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct modem_data *modem;
	int val = 0;
	int ret;
	modem = (struct modem_data *)dev->platform_data;
	ret = sscanf(buf, "%u", &val);

	if (val == 0)
		modem->mld->rx_int_count = 0;
	return count;
}

static ssize_t rx_poll_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	struct mem_link_device *mld;
	ssize_t count = 0;

	modem = (struct modem_data *)dev->platform_data;
	mld = modem->mld;

	count += sprintf(&buf[count], "%s: %d\n", netdev_name(&mld->dummy_net), mld->rx_poll_count);
	return count;
}

static ssize_t rx_poll_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct modem_data *modem;
	struct mem_link_device *mld;

	modem = (struct modem_data *)dev->platform_data;
	mld = modem->mld;

	mld->rx_poll_count = 0;
	return count;
}

static ssize_t rx_int_disabled_time_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_data *modem;
	modem = (struct modem_data *)dev->platform_data;
	return sprintf(buf, "%lld\n", modem->mld->rx_int_disabled_time);
}

static ssize_t rx_int_disabled_time_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct modem_data *modem;
	int val = 0;
	int ret;
	modem = (struct modem_data *)dev->platform_data;
	ret = sscanf(buf, "%u", &val);

	if (val == 0)
		modem->mld->rx_int_disabled_time = 0;
	return count;
}

static DEVICE_ATTR_RO(rx_napi_list);
static DEVICE_ATTR_RO(rx_int_enable);
static DEVICE_ATTR_RW(rx_int_count);
static DEVICE_ATTR_RW(rx_poll_count);
static DEVICE_ATTR_RW(rx_int_disabled_time);

static struct attribute *napi_attrs[] = {
	&dev_attr_rx_napi_list.attr,
	&dev_attr_rx_int_enable.attr,
	&dev_attr_rx_int_count.attr,
	&dev_attr_rx_poll_count.attr,
	&dev_attr_rx_int_disabled_time.attr,
	NULL,
};

static const struct attribute_group napi_group = {		\
	.attrs = napi_attrs,					\
	.name = "napi",
};
#endif

struct link_device *pcie_create_link_device(struct platform_device *pdev)
{
	struct modem_data *modem;
	struct mem_link_device *mld;
	struct link_device *ld;
	int err;
	mif_err("+++\n");

	/**
	 * Get the modem (platform) data
	 */
	modem = (struct modem_data *)pdev->dev.platform_data;
	if (!modem) {
		mif_err("ERR! modem == NULL\n");
		return NULL;
	}

	if (!modem->mbx) {
		mif_err("%s: ERR! mbx == NULL\n", modem->link_name);
		return NULL;
	}

	if (modem->ipc_version < SIPC_VER_50) {
		mif_err("%s<->%s: ERR! IPC version %d < SIPC_VER_50\n",
			modem->link_name, modem->name, modem->ipc_version);
		return NULL;
	}

	mif_err("MODEM:%s LINK:%s\n", modem->name, modem->link_name);

	/*
	** Alloc an instance of mem_link_device structure
	*/
	mld = kzalloc(sizeof(struct mem_link_device), GFP_KERNEL);
	if (!mld) {
		mif_err("%s<->%s: ERR! mld kzalloc fail\n",
			modem->link_name, modem->name);
		return NULL;
	}

	/*
	** Retrieve modem-specific attributes value
	*/
	mld->attrs = modem->link_attrs;

	/*====================================================================*\
		Initialize "memory snapshot buffer (MSB)" framework
	\*====================================================================*/
	if (msb_init() < 0) {
		mif_err("%s<->%s: ERR! msb_init() fail\n",
			modem->link_name, modem->name);
		goto error;
	}

	/*====================================================================*\
		Set attributes as a "link_device"
	\*====================================================================*/
	ld = &mld->link_dev;

	ld->name = modem->link_name;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_SBD_IPC)) {
		mif_err("%s<->%s: LINK_ATTR_SBD_IPC\n", ld->name, modem->name);
		ld->sbd_ipc = true;
	}

	if (mld->attrs & LINK_ATTR(LINK_ATTR_IPC_ALIGNED)) {
		mif_err("%s<->%s: LINK_ATTR_IPC_ALIGNED\n",
			ld->name, modem->name);
		ld->aligned = true;
	}

	ld->ipc_version = modem->ipc_version;

	ld->mdm_data = modem;

	ld->dev = &pdev->dev;

	/*
	Set up link device methods
	*/
	ld->ioctl = shmem_ioctl;

	ld->init_comm = shmem_init_comm;
	ld->terminate_comm = shmem_terminate_comm;
	ld->send = shmem_send;
	ld->reset_zerocopy = NULL;

	ld->boot_on = shmem_boot_on;
	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_BOOT)) {
		if (mld->attrs & LINK_ATTR(LINK_ATTR_XMIT_BTDLR))
			ld->xmit_boot = shmem_xmit_boot;
		ld->dload_start = shmem_start_download;
		ld->firm_update = shmem_update_firm_info;
		ld->security_req = shmem_security_request;
	}

	ld->shmem_dump = save_shmem_dump;
	ld->force_dump = shmem_force_dump;
	ld->vss_dump = save_vss_dump;
	ld->acpm_dump = save_acpm_dump;

#ifdef CONFIG_CP_RAM_LOGGING
	ld->cplog_dump = save_cplog_dump;
#endif

	ld->register_pcie = shmem_register_pcie;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		ld->dump_start = shmem_start_upload;

	ld->close_tx = shmem_close_tx;
	ld->crash_reason = shmem_crash_reason;
	ld->airplane_mode = shmem_airplane_mode;

#ifdef CONFIG_MODEM_IF_NET_GRO
	ld->gro_flush = gro_flush_timer;
#endif

#ifdef CONFIG_LINK_DEVICE_NAPI
	ld->poll_recv_on_iod = shmem_poll_recv_on_iod;

	ld->enable_rx_int = shmem_enable_rx_int;
	ld->disable_rx_int = shmem_disable_rx_int;

	init_dummy_netdev(&mld->dummy_net);
	netif_napi_add(&mld->dummy_net, &mld->mld_napi, mld_rx_int_poll, 64);
	napi_enable(&mld->mld_napi);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	INIT_LIST_HEAD(&ld->list);

	spin_lock_init(&ld->netif_lock);
	atomic_set(&ld->netif_stopped, 0);
	ld->tx_flowctrl_mask = 0;
	init_completion(&ld->raw_tx_resumed);

	if (shmem_rx_setup(ld) < 0)
		goto error;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_DPRAM_MAGIC)) {
		mif_err("%s<->%s: LINK_ATTR_DPRAM_MAGIC\n",
			ld->name, modem->name);
		mld->dpram_magic = true;
	}
#ifdef CONFIG_LINK_CONTROL_MSG_IOSM
	mld->iosm = true;
	mld->cmd_handler = iosm_event_bh;
	INIT_WORK(&mld->iosm_w, iosm_event_work);
#else
	mld->cmd_handler = shmem_cmd_handler;
#endif

	spin_lock_init(&mld->state_lock);
	mld->state = LINK_STATE_OFFLINE;

	/*
	** Initialize variables for TX & RX
	*/
	msb_queue_head_init(&mld->msb_rxq);
	msb_queue_head_init(&mld->msb_log);

	tasklet_init(&mld->rx_tsk, shmem_rx_task, (unsigned long)mld);

	hrtimer_init(&mld->tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mld->tx_timer.function = tx_timer_func;

	INIT_WORK(&mld->page_reclaim_work, shmem_oom_handler_work);

	/*
	** Initialize variables for CP booting and crash dump
	*/
	INIT_DELAYED_WORK(&mld->udl_rx_dwork, udl_rx_work);

	/**
	 * Link local functions to the corresponding function pointers that are
	 * mandatory for all memory-type link devices
	 */
	mld->recv_cp2ap_irq = recv_cp2ap_irq;
	mld->send_ap2cp_irq = send_ap2cp_irq;
	mld->recv_cp2ap_status = recv_cp2ap_status;

	/**
	 * Link local functions to the corresponding function pointers that are
	 * optional for some memory-type link devices
	 */
	mld->read_ap2cp_irq = read_ap2cp_irq;

	/**
	 * Initialize SHMEM maps for BOOT (physical map -> logical map)
	 */
	mld->shm_size = shm_get_phys_size();

	/**
	 * Initialize SHMEM maps for IPC (physical map -> logical map)
	 */
	mld->size = shm_get_ipc_rgn_size();
	mld->base = (void __iomem *)phys_to_virt(shm_get_phys_base() +
			shm_get_ipc_rgn_offset());
	if (!mld->base) {
		mif_err("Failed to vmap ipc_region\n");
		goto error;
	}
	mif_err("ipc_base=%pK, ipc_size=%lu\n",
		mld->base, (unsigned long)mld->size);

	/**
	 * Initialize SHMEM maps for VSS (physical map -> logical map)
	 */
	mld->vss_base = shm_get_vss_region();
	if (!mld->vss_base) {
		mif_err("Failed to vmap vss_region\n");
		goto error;
	}
	mif_err("vss_base=%pK\n", mld->vss_base);

	/**
	 * Initialize memory maps for ACPM (physical map -> logical map)
	 */
	mld->acpm_base = shm_get_acpm_region();
	mld->acpm_size = shm_get_acpm_size();
	if (!mld->acpm_base) {
		mif_err("Failed to vmap acpm_region\n");
		goto error;
	}
	mif_err("acpm_base=%pK acpm_size:0x%X\n", mld->acpm_base,
			mld->acpm_size);

	remap_4mb_map_to_ipc_dev(mld);

	if (ld->sbd_ipc) {
		hrtimer_init(&mld->sbd_tx_timer,
				CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		mld->sbd_tx_timer.function = sbd_tx_timer_func;

		err = create_sbd_link_device(ld,
				&mld->sbd_link_dev, mld->base, mld->size);
		if (err < 0)
			goto error;
	}

	/* Set MSI interrupt number */
	mld->irq_cp2ap_msg = modem->mbx->irq_cp2ap_msg;
	mld->irq_perf_req_cpu = modem->mbx->irq_cp2ap_perf_req_cpu;
	mld->irq_perf_req_mif = modem->mbx->irq_cp2ap_perf_req_mif;
	mld->irq_perf_req_int = modem->mbx->irq_cp2ap_perf_req_int;

	/* Set doorbell interrupt value for separating interrupts */
	mld->intval_ap2cp_msg = modem->mbx->int_ap2cp_msg + DOORBELL_INT_ADD;
	mld->intval_ap2cp_status = modem->mbx->int_ap2cp_status
				+ DOORBELL_INT_ADD;
	mld->intval_ap2cp_active = modem->mbx->int_ap2cp_active
				+ DOORBELL_INT_ADD;

	/* QOS requests */
	mld->ap_clk_table = modem->mbx->ap_clk_table;
	mld->ap_clk_cnt = modem->mbx->ap_clk_cnt;

	mld->mif_clk_table = modem->mbx->mif_clk_table;
	mld->mif_clk_cnt = modem->mbx->mif_clk_cnt;

	mld->int_clk_table = modem->mbx->int_clk_table;
	mld->int_clk_cnt = modem->mbx->int_clk_cnt;

	pm_qos_add_request(&pm_qos_req_cl0, PM_QOS_CLUSTER0_FREQ_MIN, 0);
	pm_qos_add_request(&pm_qos_req_cl1, PM_QOS_CLUSTER1_FREQ_MIN, 0);
	pm_qos_add_request(&pm_qos_req_mif, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&pm_qos_req_int, PM_QOS_DEVICE_THROUGHPUT, 0);

	INIT_WORK(&mld->pm_qos_work_cpu, shmem_qos_work_cpu);
	INIT_WORK(&mld->pm_qos_work_mif, shmem_qos_work_mif);
	INIT_WORK(&mld->pm_qos_work_int, shmem_qos_work_int);

	/**
	 * Retrieve SHMEM MBOX# and IRQ# for wakelock
	 */
	wake_lock_init(&mld->cp_wakelock, WAKE_LOCK_SUSPEND, ld->name);
	wake_lock_init(&mld->tx_timer_wlock, WAKE_LOCK_SUSPEND,
			"tx_timer_wlock");
	wake_lock_init(&mld->sbd_tx_timer_wlock, WAKE_LOCK_SUSPEND,
			"sbd_tx_timer_wlock");

	/* Parsing devfreq, cpufreq table from ECT */
	mif_err("Parsing MIF table...\n");
	err = exynos_devfreq_parse_ect(mld, "dvfs_mif");
	if (err < 0)
		mif_err("Can't get MIF table!!!!!\n");

	mif_err("Parsing CL0 table...\n");
	err = exynos_devfreq_parse_ect(mld, "dvfs_cpucl0");
	if (err < 0)
		mif_err("Can't get CPU table!!!!!\n");

	mif_err("Parsing CL1 table...\n");
	err = exynos_devfreq_parse_ect(mld, "dvfs_cpucl1");
	if (err < 0)
		mif_err("Can't get CPU table!!!!!\n");

	mif_err("Parsing INT table...\n");
	err = exynos_devfreq_parse_ect(mld, "dvfs_int");
	if (err < 0)
		mif_err("Can't get INT table!!!!!\n");

        /**
	 * For TX Flow-control command from CP
	 **/
	/* TODO: Link with CP interrupt
	mld->mbx_cp2ap_status = modem->mbx->mbx_cp2ap_status;
	mld->irq_cp2ap_status = modem->mbx->irq_cp2ap_status;
        mld->tx_flowctrl_cmd = 0;

        err = mbox_request_irq(MCU_CP, mld->irq_cp2ap_status,
                                shmem_tx_state_handler, mld);
        if (err) {
                mif_err("%s: ERR! mbox_request_irq(MCU_CP, %u) fail (%d)\n",
                        ld->name, mld->irq_cp2ap_status, err);
                goto error;
        */

	mld->pktlog = create_pktlog("shmem");
	if (!mld->pktlog)
		mif_err("packet log device create fail\n");

	mld->syscp_info = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	/* Link mem_link_device to modem_data */
	modem->mld = mld;

	mld->tx_period_ms = TX_PERIOD_MS;

	if (sysfs_create_group(&pdev->dev.kobj, &shmem_group))
		mif_err("failed to create sysfs node related shmem\n");

#ifdef CONFIG_LINK_DEVICE_NAPI
	if (sysfs_create_group(&pdev->dev.kobj, &napi_group))
		mif_err("failed to create sysfs node related napi\n");
#endif

	mif_err("---\n");
	return ld;

error:
	shm_release_regions();

	kfree(mld);
	mif_err("xxx\n");
	return NULL;
}
