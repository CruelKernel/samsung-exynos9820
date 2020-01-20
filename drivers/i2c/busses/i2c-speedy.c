/*
 * Driver for Samsung SPEEDY(Serial Protocol in an EffEctive Digital waY)
 *
 * Copyright (C) 2015 Samsung Electronics Ltd.
 * 	Youngmin Nam <youngmin.nam@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include "../../pinctrl/core.h"

/* SPEEDY Register MAP */
#define SPEEDY_CTRL					0x000
#define SPEEDY_FIFO_CTRL				0x004
#define SPEEDY_CMD					0x008
#define SPEEDY_INT_ENABLE				0x00C
#define SPEEDY_INT_STATUS				0x010
#define SPEEDY_FIFO_STATUS				0x030
#define SPEEDY_TX_DATA					0x034
#define SPEEDY_RX_DATA					0x038
#define SPEEDY_PACKET_GAP_TIME				0x044
#define SPEEDY_TIMEOUT_COUNT				0x048
#define SPEEDY_FIFO_DEBUG				0x100
#define SPEEDY_CTRL_STATUS				0x104

/* SPEEDY_CTRL Register bits */
#define SPEEDY_ENABLE					(1 << 0)
#define SPEEDY_TIMEOUT_CMD_DISABLE			(1 << 1)
#define SPEEDY_TIMEOUT_STANDBY_DISABLE			(1 << 2)
#define SPEEDY_TIMEOUT_DATA_DISABLE			(1 << 3)
#define SPEEDY_ALWAYS_PULLUP_EN				(1 << 7)
#define SPEEDY_DATA_WIDTH_8BIT				(0 << 8)
#define SPEEDY_REMOTE_RESET_REQ				(1 << 30)
#define SPEEDY_SW_RST					(1 << 31)

/* SPEEDY_FIFO_CTRL Register bits */
#define SPEEDY_RX_TRIGGER_LEVEL(x)			((x) << 0)
#define SPEEDY_TX_TRIGGER_LEVEL(x)			((x) << 8)
#define SPEEDY_FIFO_DEBUG_INDEX				(0 << 24) // TODO : modify define
#define SPEEDY_FIFO_RESET				(1 << 31)

/* SPEEDY_CMD Register bits */
#define SPEEDY_BURST_LENGTH(x)				((x) << 0)
#define SPEEDY_BURST_FIXED				(0 << 5)
#define SPEEDY_BURST_INCR				(1 << 5)
#define SPEEDY_BURST_EXTENSION				(2 << 5)
#define SPEEDY_ADDRESS(x)				((x & 0xFFF) << 7)
#define SPEEDY_ACCESS_BURST				(0 << 19)
#define SPEEDY_ACCESS_RANDOM				(1 << 19)
#define SPEEDY_DIRECTION_READ				(0 << 20)
#define SPEEDY_DIRECTION_WRITE				(1 << 20)

/* SPEEDY_INT_ENABLE Register bits */
#define SPEEDY_TRANSFER_DONE_EN				(1 << 0)
#define SPEEDY_TIMEOUT_CMD_EN				(1 << 1)
#define SPEEDY_TIMEOUT_STANDBY_EN			(1 << 2)
#define SPEEDY_TIMEOUT_DATA_EN				(1 << 3)
#define SPEEDY_FIFO_TX_ALMOST_EMPTY_EN			(1 << 4)
#define SPEEDY_FIFO_RX_ALMOST_FULL_EN			(1 << 8)
#define SPEEDY_RX_FIFO_INT_TRAILER_EN			(1 << 9)
#define SPEEDY_RX_MODEBIT_ERR_EN			(1 << 16)
#define SPEEDY_RX_GLITCH_ERR_EN				(1 << 17)
#define SPEEDY_RX_ENDBIT_ERR_EN				(1 << 18)
#define SPEEDY_TX_LINE_BUSY_ERR_EN			(1 << 20)
#define SPEEDY_TX_STOPBIT_ERR_EN			(1 << 21)
#define SPEEDY_REMOTE_RESET_REQ_EN			(1 << 31)

/* SPEEDY_INT_STATUS Register bits */
#define SPEEDY_TRANSFER_DONE				(1 << 0)
#define SPEEDY_TIMEOUT_CMD				(1 << 1)
#define SPEEDY_TIMEOUT_STANDBY				(1 << 2)
#define SPEEDY_TIMEOUT_DATA				(1 << 3)
#define SPEEDY_FIFO_TX_ALMOST_EMPTY			(1 << 4)
#define SPEEDY_FIFO_RX_ALMOST_FULL			(1 << 8)
#define SPEEDY_RX_FIFO_INT_TRAILER			(1 << 9)
#define SPEEDY_RX_MODEBIT_ERR				(1 << 16)
#define SPEEDY_RX_GLITCH_ERR				(1 << 17)
#define SPEEDY_RX_ENDBIT_ERR				(1 << 18)
#define SPEEDY_TX_LINE_BUSY_ERR				(1 << 20)
#define SPEEDY_TX_STOPBIT_ERR				(1 << 21)
#define SPEEDY_REMOTE_RESET_REQ_STAT			(1 << 31)

/* SPEEDY_FIFO_STATUS Register bits */
#define SPEEDY_VALID_DATA_CNT				(0 << 0) // TODO : modify define
#define SPEEDY_FIFO_FULL				(1 << 5)
#define SPEEDY_FIFO_EMPTY				(1 << 6)

/* SPEEDY_PACKET_GAP_TIME Register bits */
#define SPEEDY_PULL_EN_CNT				(0xF << 0) // TODO : modify define
#define SPEEDY_PACKET_GAP_TIME_CNT			(0 << 16) // TODO : modify define

/* SPEEDY_CTRL_STATUS Register bits */
#define SPEEDY_FSM_IDLE					(1 << 0)
#define SPEEDY_FSM_INIT					(1 << 1)
#define SPEEDY_FSM_TX_CMD				(1 << 2)
#define SPEEDY_FSM_STANDBY				(1 << 3)
#define SPEEDY_FSM_DATA					(1 << 4)
#define SPEEDY_FSM_TIMEOUT				(1 << 5)
#define SPEEDY_FSM_TRANS_DONE				(1 << 6)
#define SPEEDY_FSM_IO_RX_STAT_MASK			(3 << 7)
#define SPEEDY_FSM_IO_TX_IDLE				(1 << 9)
#define SPEEDY_FSM_IO_TX_GET_PACKET			(1 << 10)
#define SPEEDY_FSM_IO_TX_PACKET				(1 << 11)
#define SPEEDY_FSM_IO_TX_DONE				(1 << 12)

/* IP_BATCHER Register MAP */
#define IPBATCHER_CON					0x0500
#define IPBATCHER_STATE					0x0504
#define IPBATCHER_INT_EN				0x0508
#define IPBATCHER_FSM_UNEXPEN				0x050C
#define IPBATCHER_FSM_TXEN				0x0510
#define IPBATCHER_FSM_RXFIFO				0x0514
#define IPBATCHER_FSM_CON				0x0518
#define IP_FIFO_STATUS					0x051C
#define IP_INT_STATUS					0x0520
#define IP_INTR_UNEXP_STATE				0x0524
#define IP_INTR_TX_STATE				0x0528
#define IP_INTR_RX_STATE				0x052C
#define BATCHER_OPCODE					0x0600
#define BATCHER_START_PAYLOAD				0x1000
#define BATCHER_END_PAYLOAD				0x1060
#define IPBATCHER_SEMA_REL				0x0200

/* IPBATCHER_CON Register bits */
#define BATCHER_ENABLE					(1 << 0)
#define DEDICATED_BATCHER_APB				(1 << 1)
#define START_BATCHER					(1 << 4)
#define APB_RESP_CPU					(1 << 5)
#define IP_SW_RST					(1 << 6)
#define MP_APBSEMA_SW_RST				(1 << 7)
#define MP_APBSEMA_DISABLE				(1 << 8)
#define SW_RESET					(1 << 31)

/* IPBATCHER_STATE Register bits */
#define BATCHER_OPERATION_COMPLETE			(1 << 0)
#define UNEXPECTED_IP_INTR				(1 << 1)
#define BATCHER_FSM_STATE_IDLE				(1 << 3)
#define BATCHER_FSM_STATE_INIT				(1 << 4)
#define BATCHER_FSM_STATE_GET_SEMAPHORE			(1 << 5)
#define BATCHER_FSM_STATE_CONFIG			(1 << 6)
#define BATCHER_FSM_STATE_WAIT_INT			(1 << 7)
#define BATCHER_FSM_STATE_SW_RESET_IP			(1 << 8)
#define BATCHER_FSM_STATE_INTR_ROUTINE			(1 << 9)
#define BATCHER_FSM_STATE_WRITE_TX_DATA			(1 << 10)
#define BATCHER_FSM_STATE_READ_RX_DATA			(1 << 11)
#define BATCHER_FSM_STATE_STOP_I2C			(1 << 12)
#define BATCHER_FSM_STATE_CLEAN_INTR_STAT		(1 << 13)
#define BATCHER_FSM_STATE_REL_SEMAPHORE			(1 << 14)
#define BATCHER_FSM_STATE_GEN_INT			(1 << 15)
#define BATCHER_FSM_STATE_UNEXPECTED_INT		(1 << 16)
#define MP_APBSEMA_CH_LOCK_STATUS			(1 << 20)
#define MP_APBSEMA_DISABLE_STATUS			(1 << 21)
#define MP_APBSEMA_SW_RST_STATUS			(1 << 22)

/* IPBATCHER_INT_EN Register bits */
#define BATCHER_INTERRUPT_ENABLE			(1 << 0)

/* IPBATCHER_FSM_CON Register bits */
#define DISABLE_STOP_CMD				(1 << 0)
#define DISABLE_SEMAPHORE_RELEASE			(1 << 1)

#define EXYNOS_SPEEDY_TIMEOUT				(msecs_to_jiffies(500))
#define BATCHER_INIT_CMD				0xFFFFFFFF

#define ACCESS_BURST					0
#define ACCESS_RANDOM					1
#define DIRECTION_READ 					0
#define DIRECTION_WRITE					1

#define SRP_COUNT					3
#define EMULATOR

struct exynos_speedy {
	struct list_head	node;
	struct i2c_adapter	adap;

	struct i2c_msg		*msg;
	unsigned int		msg_ptr;
	unsigned int 		msg_len;

	unsigned int 		irq;
	void __iomem		*regs;
	struct clk		*clk;
	struct device		*dev;

	unsigned int 		cmd_buffer;
	unsigned int 		cmd_index;
	unsigned int		cmd_pointer;
	unsigned int		desc_pointer;
	unsigned int		batcher_read_addr;

	int			always_intr_high;
	unsigned int		int_en;
};

static void dump_speedy_register(struct exynos_speedy *speedy)
{
	dev_err(speedy->dev, "SPEEDY Register dump\n"
		"	CTRL             0x%08x\n"
		"	FIFO_CTRL        0x%08x\n"
		"	CMD              0x%08x\n"
		"	INT_ENABLE       0x%08x\n"
		"	INT_STATUS       0x%08x\n"
		"	FIFO_STATUS      0x%08x\n"
		"	PACKET_GAP_TIME  0x%08x\n"
		"	TIMEOUT_COUNT    0x%08x\n"
		"	CTRL_STATUS      0x%08x\n"
		, readl(speedy->regs + SPEEDY_CTRL)
		, readl(speedy->regs + SPEEDY_FIFO_CTRL)
		, readl(speedy->regs + SPEEDY_CMD)
		, readl(speedy->regs + SPEEDY_INT_ENABLE)
		, readl(speedy->regs + SPEEDY_INT_STATUS)
		, readl(speedy->regs + SPEEDY_FIFO_STATUS)
		, readl(speedy->regs + SPEEDY_PACKET_GAP_TIME)
		, readl(speedy->regs + SPEEDY_TIMEOUT_COUNT)
		, readl(speedy->regs + SPEEDY_CTRL_STATUS)
	);
}

static void dump_batcher_register(struct exynos_speedy *speedy)
{
	int i = 0;
	char buf_opcode[SZ_256];
	char buf_payload[SZ_1K];
	u32 len = 0;

	dev_err(speedy->dev, "Batcher Register dump\n"
		"	CON               0x%08x\n"
		"	State             0x%08x\n"
		"	INT_EN            0x%08x\n"
		"	FSM_UNEXPEN       0x%08x\n"
		"	FSM_TXEN          0x%08x\n"
		"	FSM_RXFIFO        0x%08x\n"
		"	FSM_CON           0x%08x\n"
		"	FIFO_Status       0x%08x\n"
		"	INT_Status        0x%08x\n"
		"	INTR_UNEXP_state  0x%08x\n"
		"	INTR_TX_state     0x%08x\n"
		"	INTR_RX_state     0x%08x\n"
		, readl(speedy->regs + IPBATCHER_CON)
		, readl(speedy->regs + IPBATCHER_STATE)
		, readl(speedy->regs + IPBATCHER_INT_EN)
		, readl(speedy->regs + IPBATCHER_FSM_UNEXPEN)
		, readl(speedy->regs + IPBATCHER_FSM_TXEN)
		, readl(speedy->regs + IPBATCHER_FSM_RXFIFO)
		, readl(speedy->regs + IPBATCHER_FSM_CON)
		, readl(speedy->regs + IP_FIFO_STATUS)
		, readl(speedy->regs + IP_INT_STATUS)
		, readl(speedy->regs + IP_INTR_UNEXP_STATE)
		, readl(speedy->regs + IP_INTR_TX_STATE)
		, readl(speedy->regs + IP_INTR_RX_STATE)
	);

	len += snprintf(buf_opcode + len, sizeof(buf_opcode) - len,
			"Batcher OPCODE dump\n");

	for (i = 0; i < 7; i++) {
		len += snprintf(buf_opcode + len, sizeof(buf_opcode) - len,
			"OPCODE %d = 0x%08x\n",
			i, readl(speedy->regs + (BATCHER_OPCODE + (i * 4))));
	}

	dev_err(speedy->dev, "%s", buf_opcode);

	len = 0;
	len += snprintf(buf_payload + len, sizeof(buf_payload) - len,
			"Batcher PAYLOAD dump\n");

	for (i = 0; i < 25; i++) {
		len += snprintf(buf_payload + len, sizeof(buf_payload) - len,
			"PAYLOAD %02d = 0x%08x   ",
			i, readl(speedy->regs + (BATCHER_START_PAYLOAD + (i * 4))));
		if (i % 5 == 4)
			len += snprintf(buf_payload + len,
					sizeof(buf_payload) - len, "\n");
	}

	dev_err(speedy->dev, "%s", buf_payload);
}

static void write_batcher(struct exynos_speedy *speedy, unsigned int description,
			unsigned int opcode)
{
	if((BATCHER_START_PAYLOAD + (speedy->desc_pointer * 4)) <=
		BATCHER_END_PAYLOAD) {

		/* clear cmd_buffer */
		speedy->cmd_buffer &= ~(0xFF << (8 * speedy->cmd_index));

		/* write opcode to cmd_buffer */
		speedy->cmd_buffer |= (opcode << (8 * speedy->cmd_index));

		/* write opcode to OPCODE_TABLE register */
		writel(speedy->cmd_buffer, speedy->regs + BATCHER_OPCODE +
			(speedy->cmd_pointer * 4));

		/* write payload to PAYLOAD_FIELD register */
		writel(description, speedy->regs + BATCHER_START_PAYLOAD +
			(speedy->desc_pointer * 4));

		/* increase cmd_index for next opcode */
		speedy->cmd_index++;

		/* increase desc_pointer for next payload */
		speedy->desc_pointer++;
	} else {
		/* Error handling for opcode overflow */
		dev_err(speedy->dev, "fail to write speedy batcher\n");
	}

	/* If cmd_index is 4, we need to update cmd_index, cmd_pointer */
	if(speedy->cmd_index == 4) {
		/* initialize cmd_index to use OPCODE_TABLE from start point */
		speedy->cmd_index = 0;
		/* increase OPCODE_TABLE offset to use next OPCODE_TABLE register */
		speedy->cmd_pointer++;
		/* innitialize cmd_buffer */
		speedy->cmd_buffer = BATCHER_INIT_CMD;
	}
}

static void set_batcher_enable(struct exynos_speedy *speedy)
{
	u32 ip_batcher_con = readl(speedy->regs + IPBATCHER_CON);

	ip_batcher_con |= BATCHER_ENABLE;
	ip_batcher_con |= DEDICATED_BATCHER_APB;

	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);
}

static void start_batcher(struct exynos_speedy *speedy)
{
	u32 ip_batcher_con = readl(speedy->regs + IPBATCHER_CON);

	ip_batcher_con |= START_BATCHER;
	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);
}

static void mp_apbsema_sw_rst(struct exynos_speedy *speedy)
{
	u32 ip_batcher_con = readl(speedy->regs + IPBATCHER_CON);

	ip_batcher_con |= MP_APBSEMA_SW_RST;
	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);

	ip_batcher_con &= (~MP_APBSEMA_SW_RST);
	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);
}

static void set_batcher_interrupt(struct exynos_speedy *speedy, int enable)
{
	u32 ip_batcher_int_en = readl(speedy->regs + IPBATCHER_INT_EN);

	if (enable)
		ip_batcher_int_en |= BATCHER_INTERRUPT_ENABLE;
	else
		ip_batcher_int_en &= (~BATCHER_INTERRUPT_ENABLE);

	writel(ip_batcher_int_en, speedy->regs + IPBATCHER_INT_EN);
}

static void set_batcher_idle(struct exynos_speedy *speedy)
{
	u32 ip_batcher_con = 0;
	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);
}

static void batcher_swreset(struct exynos_speedy *speedy)
{
	u32 ip_batcher_con = readl(speedy->regs + IPBATCHER_CON);

	ip_batcher_con |= SW_RESET;
	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);

	ip_batcher_con &= (~SW_RESET);
	writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);

	dev_err(speedy->dev, "batcher s/w reset was done\n");
}

static void program_batcher_fsm(struct exynos_speedy *speedy)
{
	u32 ip_batcher_fsm_unexpec_enable = 0;
	u32 ip_batcher_fsm_tx_enable = 0;
	u32 ip_batcher_fsm_rx_fifo = 0;
	u32 ip_batcher_fsm_con = 0;

	/* select unexpected interrupt of IP */
	/* "1" in each bit will be handled as unexpected interrupt */
	ip_batcher_fsm_unexpec_enable =
			(SPEEDY_TIMEOUT_CMD_EN | SPEEDY_TIMEOUT_STANDBY_EN |
			 SPEEDY_TIMEOUT_DATA_EN | SPEEDY_RX_MODEBIT_ERR_EN |
			 SPEEDY_RX_GLITCH_ERR_EN | SPEEDY_RX_ENDBIT_ERR_EN |
			 SPEEDY_TX_LINE_BUSY_ERR_EN | SPEEDY_TX_STOPBIT_ERR_EN);
	writel(ip_batcher_fsm_unexpec_enable, speedy->regs + IPBATCHER_FSM_UNEXPEN);

	/* select Tx, Rx normal interrupt of IP */
	/* "1" in each bit will be handled as Tx normal interrupt */
	/* "0" in each bit will be handled as Rx normal interrupt */
	ip_batcher_fsm_tx_enable =
			(SPEEDY_TRANSFER_DONE_EN | SPEEDY_FIFO_TX_ALMOST_EMPTY_EN);
	writel(ip_batcher_fsm_tx_enable, speedy->regs + IPBATCHER_FSM_TXEN);

	/* select Rx FIFO empty status check bit */
	/* "1" in each bit will monitor IP's RXFIFO empty status */
	ip_batcher_fsm_rx_fifo = SPEEDY_FIFO_EMPTY;
	writel(ip_batcher_fsm_rx_fifo, speedy->regs + IPBATCHER_FSM_RXFIFO);

	ip_batcher_fsm_con = DISABLE_STOP_CMD;
	writel(ip_batcher_fsm_con, speedy->regs + IPBATCHER_FSM_CON);
}

static void release_semaphore(struct exynos_speedy *speedy)
{
	writel(0x01, speedy->regs + IPBATCHER_SEMA_REL);
}

static void speedy_swreset_directly(struct exynos_speedy *speedy)
{
	u32 speedy_ctl = readl(speedy->regs + SPEEDY_CTRL);

	speedy_ctl |= SPEEDY_SW_RST;
	writel(speedy_ctl, speedy->regs + SPEEDY_CTRL);
	/* delay for speedy sw_rst */
	udelay(10);

	dev_err(speedy->dev, "speedy swreset directly was done\n");
}

static void speedy_swreset_with_batcher(struct exynos_speedy *speedy)
{
	u32 ip_batcher_state = readl(speedy->regs + IPBATCHER_STATE);
	u32 ip_batcher_con;
	unsigned long timeout;

	if (ip_batcher_state & MP_APBSEMA_CH_LOCK_STATUS) {
		dev_err(speedy->dev, "speedy reset is started with semaphore\n");

		if (ip_batcher_state & BATCHER_FSM_STATE_WAIT_INT) {
			ip_batcher_con = readl(speedy->regs + IPBATCHER_CON);
			ip_batcher_con |= IP_SW_RST;
			writel(ip_batcher_con, speedy->regs + IPBATCHER_CON);
			/* delay for speedy sw_rst */
			udelay(10);
			dev_err(speedy->dev, "speedy swreset through batcher was done\n");
		} else {
			/* SPEEDY SW reset directly */
			speedy_swreset_directly(speedy);
		}

		udelay(100);
		ip_batcher_state = readl(speedy->regs + IPBATCHER_STATE);
		if (!(ip_batcher_state & BATCHER_FSM_STATE_INIT)) {
			mp_apbsema_sw_rst(speedy);
			udelay(100);
			batcher_swreset(speedy);
			program_batcher_fsm(speedy);

			timeout = jiffies + EXYNOS_SPEEDY_TIMEOUT;

			/* Wait IDLE or INIT state of IPBATCHER */
			while (time_before(jiffies, timeout)) {
				ip_batcher_state = readl(speedy->regs + IPBATCHER_STATE);
				if ((ip_batcher_state & BATCHER_FSM_STATE_IDLE) ||
				    (ip_batcher_state & BATCHER_FSM_STATE_INIT)) {
					timeout = 0;
					break;
				} else
					udelay(10);
			}
			if (timeout)
				dev_err(speedy->dev, "Timeout for waiting IDLE or INIT \n");
		}
		release_semaphore(speedy);
	} else {
		dev_err(speedy->dev, "speedy reset can't be done by no semaphore\n");
		batcher_swreset(speedy);
		program_batcher_fsm(speedy);
	}

	timeout = jiffies + EXYNOS_SPEEDY_TIMEOUT;

	/* Check IDLE or INIT state of IPBATCHER */
	while (time_before(jiffies, timeout)) {
		ip_batcher_state = readl(speedy->regs + IPBATCHER_STATE);
		if ((ip_batcher_state & BATCHER_FSM_STATE_IDLE) ||
		    (ip_batcher_state & BATCHER_FSM_STATE_INIT)) {
			timeout = 0;
			break;
		}
		else
			udelay(10);
	}
	if (timeout)
		dev_err(speedy->dev, "Timeout for waiting IDLE or INIT \n");

	dev_err(speedy->dev, "speedy recovery is done\n");
}

static void speedy_set_cmd(struct exynos_speedy *speedy, int direction, u16 address,
			int random, int burst_length)
{
	u32 speedy_fifo_ctl = 0;
	u32 speedy_int_en = 0;
	u32 speedy_command = 0;

	speedy_fifo_ctl |= SPEEDY_FIFO_RESET;
	speedy_command |= SPEEDY_ADDRESS(address);

	switch (random) {
	case ACCESS_BURST:
		speedy_command |= (SPEEDY_ACCESS_BURST | SPEEDY_BURST_INCR |
				   SPEEDY_BURST_LENGTH(burst_length-1));

		/* To prevent batcher timeout, interrupt state should be set as high */
		/* So, FIFO trigger level shoud be set to trigger interrupt always */
		if (speedy->always_intr_high) {
			if (direction == DIRECTION_READ) {
				speedy_fifo_ctl |= (
					SPEEDY_RX_TRIGGER_LEVEL(burst_length) |
					SPEEDY_TX_TRIGGER_LEVEL(16)
				);
			} else {
				speedy_fifo_ctl |= (
					SPEEDY_RX_TRIGGER_LEVEL(0) |
					SPEEDY_TX_TRIGGER_LEVEL(1)
				);
			}
		} else {
			speedy_fifo_ctl |= (
				SPEEDY_RX_TRIGGER_LEVEL(burst_length) |
				SPEEDY_TX_TRIGGER_LEVEL(1)
			);
		}
		break;

	case ACCESS_RANDOM:
		speedy_command |= SPEEDY_ACCESS_RANDOM;
		speedy_fifo_ctl |= (SPEEDY_RX_TRIGGER_LEVEL(1) |
				SPEEDY_TX_TRIGGER_LEVEL(1));
		break;
	}

	/* make opcode and payload to configure SPEEDY_FIFO_CTRL */
	write_batcher(speedy, speedy_fifo_ctl, SPEEDY_FIFO_CTRL);

	speedy_int_en |= (SPEEDY_TIMEOUT_CMD_EN | SPEEDY_TIMEOUT_STANDBY_EN |
			SPEEDY_TIMEOUT_DATA_EN);

	switch (direction) {
	case DIRECTION_READ:
		speedy_command |= SPEEDY_DIRECTION_READ;
		speedy_int_en |= (SPEEDY_FIFO_RX_ALMOST_FULL_EN |
				SPEEDY_RX_FIFO_INT_TRAILER_EN |
				SPEEDY_RX_MODEBIT_ERR_EN |
				SPEEDY_RX_GLITCH_ERR_EN |
				SPEEDY_RX_ENDBIT_ERR_EN);

		/* To prevent batcher timeout, interrupt state should be set as high */
		if (speedy->always_intr_high) {
			speedy_int_en |= SPEEDY_FIFO_TX_ALMOST_EMPTY_EN;
		}

		break;

	case DIRECTION_WRITE:
		speedy_command |= SPEEDY_DIRECTION_WRITE;
		speedy_int_en |= (SPEEDY_TRANSFER_DONE_EN |
				SPEEDY_FIFO_TX_ALMOST_EMPTY_EN |
				SPEEDY_TX_LINE_BUSY_ERR_EN |
				SPEEDY_TX_STOPBIT_ERR_EN);

		/* To prevent batcher timeout, interrupt state should be set as high */
		if (speedy->always_intr_high) {
			speedy_int_en |= SPEEDY_FIFO_RX_ALMOST_FULL_EN;
		}

		break;
	}

	/* store speedy_interrupt_enable status for re-configuration later */
	speedy->int_en = speedy_int_en;

	/* clear speedy interrupt status */
	write_batcher(speedy, 0xFFFFFFFF, SPEEDY_INT_STATUS);

	/* make opcode and payload to configure SPEEDY_INT_ENABLE */
	write_batcher(speedy, speedy_int_en, SPEEDY_INT_ENABLE);

	/* make opcode and payload to configure SPEEDY_CMD */
	write_batcher(speedy, speedy_command, SPEEDY_CMD);
}

static int speedy_batcher_wait_complete(struct exynos_speedy *speedy)
{
	u32 ip_batcher_state;
	u32 ip_batcher_int_status;
	int ret = -EBUSY;
	unsigned long timeout;

	timeout = jiffies + EXYNOS_SPEEDY_TIMEOUT;

	while (time_before(jiffies, timeout)) {
		ip_batcher_state = readl(speedy->regs + IPBATCHER_STATE);
		ip_batcher_int_status = readl(speedy->regs + IP_INT_STATUS);

		if (ip_batcher_state & BATCHER_OPERATION_COMPLETE) {
			if ((ip_batcher_int_status & SPEEDY_TIMEOUT_CMD) |
				(ip_batcher_int_status & SPEEDY_TIMEOUT_STANDBY) |
				(ip_batcher_int_status & SPEEDY_TIMEOUT_DATA) |
				(ip_batcher_int_status & SPEEDY_RX_MODEBIT_ERR) |
				(ip_batcher_int_status & SPEEDY_RX_GLITCH_ERR) |
				(ip_batcher_int_status & SPEEDY_RX_ENDBIT_ERR) |
				(ip_batcher_int_status & SPEEDY_TX_LINE_BUSY_ERR) |
				(ip_batcher_int_status & SPEEDY_TX_STOPBIT_ERR)) {
				ret = -EIO;
				break;
			} else {
				ret = 0;
				break;
			}
		} else if (ip_batcher_state & UNEXPECTED_IP_INTR) {
			if ((ip_batcher_int_status & SPEEDY_TIMEOUT_CMD) |
				(ip_batcher_int_status & SPEEDY_TIMEOUT_STANDBY) |
				(ip_batcher_int_status & SPEEDY_TIMEOUT_DATA) |
				(ip_batcher_int_status & SPEEDY_RX_MODEBIT_ERR) |
				(ip_batcher_int_status & SPEEDY_RX_GLITCH_ERR) |
				(ip_batcher_int_status & SPEEDY_RX_ENDBIT_ERR) |
				(ip_batcher_int_status & SPEEDY_TX_LINE_BUSY_ERR) |
				(ip_batcher_int_status & SPEEDY_TX_STOPBIT_ERR)) {
				ret = -EIO;
				break;
			}
		} else {
			udelay(10);
		}
	}

	if (ret != 0) {
		if (ret == -EIO)
			dev_err(speedy->dev,
				"speedy timeout or error is occurred ");
		else
			dev_err(speedy->dev,
				"speedy batcher operation timeout is occurred ");
	}

	if (ip_batcher_state & BATCHER_OPERATION_COMPLETE) {
		/* clear batcher operation complete */
		ip_batcher_state |= BATCHER_OPERATION_COMPLETE;
		writel(ip_batcher_state, speedy->regs + IPBATCHER_STATE);
	}

	return ret;
}

static void finalize_batcher(struct exynos_speedy *speedy)
{
	write_batcher(speedy, 0x00, 0xFF);

	/* Initialize variables related opcode and payload of batcher */
	speedy->cmd_buffer = BATCHER_INIT_CMD;
	speedy->cmd_index = 0;
	speedy->cmd_pointer = 0;
	speedy->desc_pointer = 0;
}

static void speedy_set_srp(struct exynos_speedy *speedy)
{
	int ret;
	int i;
	u32 speedy_ctl;

	for (i = 0; i < SRP_COUNT; i++) {
		speedy_ctl = 0x30051;
		/* set batcher IDLE state */
		set_batcher_idle(speedy);

		/* set batcher IDLE->INIT state */
		set_batcher_enable(speedy);

		speedy_set_cmd(speedy, DIRECTION_WRITE, 0x0, ACCESS_RANDOM, 0);

		speedy_ctl |= SPEEDY_REMOTE_RESET_REQ;
		write_batcher(speedy, speedy_ctl, SPEEDY_CTRL);

		write_batcher(speedy, 0x00, SPEEDY_TX_DATA);

		speedy_ctl &= (~SPEEDY_REMOTE_RESET_REQ);
		write_batcher(speedy, speedy_ctl, SPEEDY_CTRL);

		finalize_batcher(speedy);
		/* TODO : for polling mode, need to enable batcher interrupt ? */
		set_batcher_interrupt(speedy, 1);
		start_batcher(speedy);
		ret = speedy_batcher_wait_complete(speedy);
		/* TODO : for polling mode, need to enable batcher interrupt ? */
		set_batcher_interrupt(speedy, 0);
		set_batcher_idle(speedy);

		if (!ret) {
			dev_err(speedy->dev, "SRP was done successfully\n");
			break;
		} else {
			dev_err(speedy->dev, "SRP timeout was occured\n");
			dump_speedy_register(speedy);
			dump_batcher_register(speedy);
			speedy_swreset_with_batcher(speedy);
		}
	}
}

static int exynos_speedy_xfer_batcher(struct exynos_speedy *speedy,
				struct i2c_msg *msgs)
{
	int i = 0;
	int ret;

	/* speedy read / write direction */
	int direction;
	/* speedy random(single) / burst access way */
	int random;
	unsigned char byte;
	unsigned int speedy_int_en;

	/* initialize as reset value of SPEEDY_CTRL */
	u32 speedy_ctl = 0x30050;

	speedy->msg = msgs;
	speedy->msg_ptr = 0;

	speedy->cmd_buffer = BATCHER_INIT_CMD;
	speedy->cmd_index = 0;
	speedy->cmd_pointer = 0;
	speedy->desc_pointer = 0;

	/* set batcher IDLE state */
	set_batcher_idle(speedy);

	/* set batcher IDLE->INIT state */
	set_batcher_enable(speedy);

	/* enable speedy master */
	speedy_ctl |= SPEEDY_ENABLE;
	write_batcher(speedy, speedy_ctl, SPEEDY_CTRL);

	if (speedy->msg->flags & I2C_M_RD)
		direction = DIRECTION_READ;
	else
		direction = DIRECTION_WRITE;

	if (speedy->msg->len > 1)
		random = ACCESS_BURST;
	else
		random = ACCESS_RANDOM;

	speedy_set_cmd(speedy, direction, speedy->msg->addr, random, speedy->msg->len);

	if (direction == DIRECTION_READ) {
		speedy->batcher_read_addr = BATCHER_START_PAYLOAD +
					   ((speedy->desc_pointer) * 4);

		for (i = 0; i < speedy->msg->len; i++)
			write_batcher(speedy, 0x77, SPEEDY_RX_DATA);
	} else {
		/* direction == DIRECTION_WRITE */
		for (i = 0; i < speedy->msg->len; i++) {
			byte = speedy->msg->buf[i];
			write_batcher(speedy, byte, SPEEDY_TX_DATA);
		}

		/*
		 * To prevent interrupt pending by FIFO_TX_ALMOST_EMPTY
		 * We should disable FIFO_TX_ALMOST_EMPTY_EN after Tx
		 */
		speedy_int_en = speedy->int_en & (~SPEEDY_FIFO_TX_ALMOST_EMPTY_EN);
		write_batcher(speedy, speedy_int_en, SPEEDY_INT_ENABLE);
	}

	finalize_batcher(speedy);

	/* TODO : for polling mode, need to enable batcher interrupt ? */
	set_batcher_interrupt(speedy, 1);

	start_batcher(speedy);

	ret = speedy_batcher_wait_complete(speedy);

	/* TODO : for polling mode, need to enable batcher interrupt ? */
	set_batcher_interrupt(speedy, 0);

	if (!ret) {
		if (direction == DIRECTION_READ) {
			for (i = 0; i < speedy->msg->len; i++) {
				byte  = (unsigned char)readl(speedy->regs +
					speedy->batcher_read_addr + (i * 4));
				speedy->msg->buf[i] = byte;
			}
		}
		set_batcher_idle(speedy);
	} else {
		set_batcher_idle(speedy);
		if (direction == DIRECTION_READ)
			dev_err(speedy->dev, "at Read\n");
		else
			dev_err(speedy->dev, "at Write\n");

		dump_speedy_register(speedy);
		dump_batcher_register(speedy);

		speedy_swreset_with_batcher(speedy);
		speedy_set_srp(speedy);
		udelay(1);
		ret = -EAGAIN;
	}
	return ret;
}
#if 0
static irqreturn_t exynos_speedy_irq_batcher(int irqno, void *dev_id)
{
	/* TODO : implementation is needed more */
	/* In ISR, we will only handle error situation */

	struct exynos_speedy *speedy = dev_id;
	u32 ip_batcher_state;
	u32 ip_batcher_int_status;

	ip_batcher_state = readl(speedy->regs + IPBATCHER_STATE);
	ip_batcher_int_status = readl(speedy->regs + IP_INT_STATUS);

	if (ip_batcher_int_status & SPEEDY_REMOTE_RESET_REQ_STAT) {
		dev_err(speedy->dev, "remote_reset_req is occured\n");
	}

	if (ip_batcher_state & UNEXPECTED_IP_INTR) {
		dev_err(speedy->dev, "unexpected interrupt is occured\n");

		if (ip_batcher_int_status & SPEEDY_TIMEOUT_CMD)
			dev_err(speedy->dev, "timout_cmd is occured\n");
		if (ip_batcher_int_status & SPEEDY_TIMEOUT_STANDBY)
			dev_err(speedy->dev, "timeout_standby is occured\n");
		if (ip_batcher_int_status & SPEEDY_TIMEOUT_DATA)
			dev_err(speedy->dev, "timeout_data is occured\n");
		if (ip_batcher_int_status & SPEEDY_RX_MODEBIT_ERR)
			dev_err(speedy->dev, "rx_modebit_err is occured\n");
		if (ip_batcher_int_status & SPEEDY_RX_GLITCH_ERR)
			dev_err(speedy->dev, "rx_glitch_err is occured\n");
		if (ip_batcher_int_status & SPEEDY_RX_ENDBIT_ERR)
			dev_err(speedy->dev, "rx_endbit_err interrupt is occured\n");
		if (ip_batcher_int_status & SPEEDY_TX_LINE_BUSY_ERR)
			dev_err(speedy->dev, "tx_line_busy_err interrupt is occured\n");
		if (ip_batcher_int_status & SPEEDY_TX_STOPBIT_ERR)
			dev_err(speedy->dev, "tx_stopbit_err interrupt is occured\n");
	}

	if (ip_batcher_state & BATCHER_OPERATION_COMPLETE) {
		dev_err(speedy->dev, "batcher operation is completed\n");

		/* clear batcher operation complete */
		ip_batcher_state |= BATCHER_OPERATION_COMPLETE;
		writel(BATCHER_OPERATION_COMPLETE, speedy->regs + IPBATCHER_STATE);
	}

	return IRQ_HANDLED;
}
#endif

static int exynos_speedy_xfer(struct i2c_adapter *adap,
			  struct i2c_msg *msgs, int num)
{
	struct exynos_speedy *speedy = (struct exynos_speedy *)adap->algo_data;
	struct i2c_msg *msgs_ptr = msgs;

	int retry, i = 0;
	int ret = 0;

	return 0;
	for (retry = 0; retry < adap->retries; retry++) {
		for (i = 0; i < num; i++) {
			ret = exynos_speedy_xfer_batcher(speedy, msgs_ptr);

			msgs_ptr++;

			if (ret == -EAGAIN) {
				msgs_ptr = msgs;
				break;
			} else if (ret < 0) {
				goto out;
			}
		}

		if ((i == num) && (ret != -EAGAIN))
			break;

		dev_err(speedy->dev, "retrying transfer (%d)\n", retry);

		udelay(100);
	}

	if (i == num) {
		ret = num;
	} else {
		ret = -EREMOTEIO;
		dev_err(speedy->dev, "xfer message failed\n");
	}

 out:
	return ret;
}

static u32 exynos_speedy_func(struct i2c_adapter *adap)
{
        return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm exynos_speedy_algorithm = {
        .master_xfer            = exynos_speedy_xfer,
        .functionality          = exynos_speedy_func,
};

static int exynos_speedy_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct exynos_speedy *speedy;
	struct resource *mem;
	int ret;

	dev_info(&pdev->dev, "speedy driver probe started\n");

	if (!np) {
		dev_err(&pdev->dev, "no device node\n");
		return -ENOENT;
	}

	speedy = devm_kzalloc(&pdev->dev, sizeof(struct exynos_speedy), GFP_KERNEL);
	if (!speedy) {
		dev_err(&pdev->dev, "no memory for driver data\n");
		return -ENOMEM;
	}

	if (of_get_property(np, "samsung,always-interrupt-high", NULL))
		speedy->always_intr_high = 1;
	else
		speedy->always_intr_high = 0;

	strlcpy(speedy->adap.name, "exynos-speedy", sizeof(speedy->adap.name));
	speedy->adap.owner   = THIS_MODULE;
	speedy->adap.algo    = &exynos_speedy_algorithm;
	speedy->adap.retries = 2;
	speedy->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD;

	speedy->dev = &pdev->dev;
#if 0
	speedy->clk = devm_clk_get(&pdev->dev, "gate_speedy");
	if (IS_ERR(speedy->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		return -ENOENT;
	}
#endif
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	speedy->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (speedy->regs == NULL) {
		dev_err(&pdev->dev, "cannot map speedy SFR register\n");
		ret = PTR_ERR(speedy->regs);
		goto err_probe;
	}

	speedy->adap.dev.of_node = np;
	speedy->adap.algo_data = speedy;
	speedy->adap.dev.parent = &pdev->dev;

	speedy->irq = irq_of_parse_and_map(np, 0);
	if(speedy->irq <= 0) {
		dev_err(&pdev->dev, "cannot find speedy IRQ\n");
		ret = -EINVAL;
		goto err_probe;
	}
	platform_set_drvdata(pdev, speedy);

#if 0
	/* clear speedy interrupt status */
	writel(0xFFFFFFFF, speedy->regs + SPEEDY_INT_STATUS);

	/* reset speedy ctrl SFR. It may be used by bootloader */
	speedy_swreset_directly(speedy);

	/* Do we need to register ISR for batcher polling mode? */
	ret = devm_request_irq(&pdev->dev, speedy->irq,
		exynos_speedy_irq_batcher, 0, dev_name(&pdev->dev), speedy);

	disable_irq(speedy->irq);
	if (ret != 0) {
		dev_err(&pdev->dev, "cannot request speedy IRQ %d\n", speedy->irq);
		goto err_probe;
	}
	/* release semaphore after direct SPEEDY SFR access */
	release_semaphore(speedy);

	/* reset batcher */
	batcher_swreset(speedy);
	/* select bitfield to monitor interrupt and status by batcher */
	program_batcher_fsm(speedy);
#endif

	speedy->adap.nr = -1;
	ret = i2c_add_numbered_adapter(&speedy->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add bus to i2c core\n");
		goto err_probe;
	}

	dev_info(&pdev->dev, "speedy driver probe was succeeded\n");

	return 0;

 err_probe:
	dev_err(&pdev->dev, "speedy driver probe failed\n");
	return ret;
}

static const struct of_device_id exynos_speedy_match[] = {
	{ .compatible = "samsung,exynos-speedy" },
	{}
};
MODULE_DEVICE_TABLE(of, exynos_speedy_match);

static struct platform_driver exynos_speedy_driver = {
	.probe		= exynos_speedy_probe,
	.driver		= {
		.name	= "exynos-speedy",
		.owner	= THIS_MODULE,
		.of_match_table = exynos_speedy_match,
	},
};

static int __init exynos_speedy_init(void)
{
	return platform_driver_register(&exynos_speedy_driver);
}
subsys_initcall(exynos_speedy_init);

static void __exit exynos_speedy_exit(void)
{
	platform_driver_unregister(&exynos_speedy_driver);
}
module_exit(exynos_speedy_exit);

MODULE_DESCRIPTION("Exynos SPEEDY driver");
MODULE_AUTHOR("Youngmin Nam, <youngmin.nam@samsung.com>");
MODULE_LICENSE("GPL v2");
