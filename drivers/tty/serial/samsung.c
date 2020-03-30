/*
 * Driver core for Samsung SoC onboard UARTs.
 *
 * Ben Dooks, Copyright (c) 2003-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* Hote on 2410 error handling
 *
 * The s3c2410 manual has a love/hate affair with the contents of the
 * UERSTAT register in the UART blocks, and keeps marking some of the
 * error bits as reserved. Having checked with the s3c2410x01,
 * it copes with BREAKs properly, so I am happy to ignore the RESERVED
 * feature from the latter versions of the manual.
 *
 * If it becomes aparrent that latter versions of the 2410 remove these
 * bits, then action will have to be taken to differentiate the versions
 * and change the policy on BREAK
 *
 * BJD, 04-Nov-2004
*/

#if defined(CONFIG_SERIAL_SAMSUNG_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_s3c.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>

#include <asm/irq.h>

#include "samsung.h"
#include "../../pinctrl/core.h"

#ifdef CONFIG_CPU_IDLE
#include <soc/samsung/exynos-pm.h>
#endif

#ifdef CONFIG_PM_DEVFREQ
#include <linux/pm_qos.h>
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#if	defined(CONFIG_SERIAL_SAMSUNG_DEBUG) &&	\
	defined(CONFIG_DEBUG_LL) &&		\
	!defined(MODULE)

extern void printascii(const char *);

__printf(1, 2)
static void dbg(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vscnprintf(buff, sizeof(buff), fmt, va);
	va_end(va);

	printascii(buff);
}

#else
#define dbg(fmt, ...) do { if (0) no_printk(fmt, ##__VA_ARGS__); } while (0)
#endif

/* UART name and device definitions */

#define S3C24XX_SERIAL_NAME	"ttySAC"
#define S3C24XX_SERIAL_MAJOR	204
#define S3C24XX_SERIAL_MINOR	64

/* Baudrate definition*/
#define MAX_BAUD	4000000
#define MIN_BAUD	0

#define DEFAULT_SOURCE_CLK	200000000

#if defined(CONFIG_SEC_FACTORY)             // SEC_FACTORY
#define SERIAL_UART_TRACE 1
#define PROC_SERIAL_DIR	"serial/uart"
#define SERIAL_UART_PORT_LINE 0
#endif

#define BT_UART_TRACE 1
#define PROC_DIR	"bluetooth/uart"
#define BLUETOOTH_UART_PORT_LINE 1
/* macros to change one thing to another */

#define tx_enabled(port) ((port)->unused[0])
#define rx_enabled(port) ((port)->unused[1])

/* flag to ignore all characters coming in */
#define RXSTAT_DUMMY_READ (0x10000000)

static LIST_HEAD(drvdata_list);
s3c_wake_peer_t s3c2410_serial_wake_peer[CONFIG_SERIAL_SAMSUNG_UARTS];
EXPORT_SYMBOL_GPL(s3c2410_serial_wake_peer);

#define UART_LOOPBACK_MODE	(0x1 << 0)
#define UART_DBG_MODE		(0x1 << 1)

#define RTS_PINCTRL			(1)
#define DEFAULT_PINCTRL		(0)
unsigned char uart_log_buf[256] = {0, };

void s3c24xx_serial_rx_fifo_wait(void);

/* Allocate 800KB of buffer for UART logging */
#define LOG_BUFFER_SIZE		(0x190000)
struct s3c24xx_uart_port *panic_port;

static int exynos_s3c24xx_panic_handler(struct notifier_block *nb,
								unsigned long l, void *p)
{

	struct uart_port *port = &panic_port->port;

	dev_err(panic_port->port.dev, " Register dump\n"
		"ULCON	0x%08x	"
		"UCON	0x%08x	"
		"UFCON	0x%08x\n"
		"UMCON	0x%08x	"
		"UTRSTAT	0x%08x	"
		"UERSTAT	0x%08x	"
		"UMSTAT	0x%08x\n"
		"UBRDIV	0x%08x	"
		"UINTP	0x%08x	"
		"UINTM	0x%08x\n"
		, readl(port->membase + S3C2410_ULCON)
		, readl(port->membase + S3C2410_UCON)
		, readl(port->membase + S3C2410_UFCON)
		, readl(port->membase + S3C2410_UMCON)
		, readl(port->membase + S3C2410_UTRSTAT)
		, readl(port->membase + S3C2410_UERSTAT)
		, readl(port->membase + S3C2410_UMSTAT)
		, readl(port->membase + S3C2410_UBRDIV)
		, readl(port->membase + S3C64XX_UINTP)
		, readl(port->membase + S3C64XX_UINTM)
	);

	return 0;
}

static struct notifier_block exynos_s3c24xx_panic_block = {
	.notifier_call = exynos_s3c24xx_panic_handler,
};

static void uart_sfr_dump(struct s3c24xx_uart_port *ourport)
{
	struct uart_port *port = &ourport->port;

	dev_err(ourport->port.dev, " Register dump\n"
		"ULCON	0x%08x	"
		"UCON	0x%08x	"
		"UFCON	0x%08x	\n"
		"UMCON	0x%08x	"
		"UTRSTAT	0x%08x	"
		"UERSTAT	0x%08x	"
		"UMSTAT	0x%08x	\n"
		"UBRDIV	0x%08x	"
		"UINTP	0x%08x	"
		"UINTM	0x%08x	\n"
		, readl(port->membase + S3C2410_ULCON)
		, readl(port->membase + S3C2410_UCON)
		, readl(port->membase + S3C2410_UFCON)
		, readl(port->membase + S3C2410_UMCON)
		, readl(port->membase + S3C2410_UTRSTAT)
		, readl(port->membase + S3C2410_UERSTAT)
		, readl(port->membase + S3C2410_UMSTAT)
		, readl(port->membase + S3C2410_UBRDIV)
		, readl(port->membase + S3C64XX_UINTP)
		, readl(port->membase + S3C64XX_UINTM)
	);
}

static void change_uart_gpio(int value, struct s3c24xx_uart_port *ourport)
{
	int status = 0;

	if (value) {
		if (!IS_ERR(ourport->uart_pinctrl_tx_dat)) {
			ourport->default_uart_pinctrl->state = NULL;
			status = pinctrl_select_state(ourport->default_uart_pinctrl, ourport->uart_pinctrl_tx_dat);
			if (status)
				dev_err(ourport->port.dev, "Can't set TXD uart pins!!!\n");
			else
				udelay(10);
		}
		if (!IS_ERR(ourport->uart_pinctrl_rts)) {
			ourport->default_uart_pinctrl->state = NULL;
			status = pinctrl_select_state(ourport->default_uart_pinctrl, ourport->uart_pinctrl_rts);
			if (status)
				dev_err(ourport->port.dev, "Can't set RTS uart pins!!!\n");
		}
	} else {
		if (!IS_ERR(ourport->uart_pinctrl_default)) {
			ourport->default_uart_pinctrl->state = NULL;
			status = pinctrl_select_state(ourport->default_uart_pinctrl, ourport->uart_pinctrl_default);
			if (status)
				dev_err(ourport->port.dev, "Can't set default uart pins!!!\n");
		}
	}
}

static void print_uart_mode(struct uart_port *port,
		struct ktermios *termios, unsigned int baud)
{
	printk(KERN_ERR "UART port%d configurations\n", port->line);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		printk(KERN_ERR " - 5bits word length\n");
		break;
	case CS6:
		printk(KERN_ERR " - 6bits word length\n");
		break;
	case CS7:
		printk(KERN_ERR " - 7bits word length\n");
		break;
	case CS8:
	default:
		printk(KERN_ERR " - 8bits word length\n");
		break;
	}

	if (termios->c_cflag & CSTOPB)
		printk(KERN_ERR " - Use TWO stop bit\n");
	else
		printk(KERN_ERR " - Use one stop bit\n");

	if (termios->c_cflag & CRTSCTS)
		printk(KERN_ERR " - Use Autoflow control\n");

	printk(KERN_ERR " - Baudrate : %u\n", baud);
}

static ssize_t
uart_dbg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"UART Debug Mode Configuration.\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"0 : Change loopback & DBG mode.\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"1 : Change DBG mode.\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"2 : Change Normal mode.\n");

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t
uart_dbg_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int input_cmd = 0, ret;
	struct s3c24xx_uart_port *ourport;

	ret = sscanf(buf, "%d", &input_cmd);

	list_for_each_entry(ourport, &drvdata_list, node) {
		if (&ourport->pdev->dev != dev)
			continue;

		switch(input_cmd) {
		case 0:
			printk(KERN_ERR "Change UART%d to Loopback(DBG) mode\n",
						ourport->port.line);
			ourport->dbg_mode = UART_DBG_MODE | UART_LOOPBACK_MODE;
			break;
		case 1:
			printk(KERN_ERR "Change UART%d to DBG mode\n",
						ourport->port.line);
			ourport->dbg_mode = UART_DBG_MODE;
			break;
		case 2:
			printk(KERN_ERR "Change UART%d to normal mode\n",
						ourport->port.line);
			ourport->dbg_mode = 0;
			break;
		default:
			printk(KERN_ERR "Wrong Command!(0/1/2)\n");
		}
	}

	return count;
}

static DEVICE_ATTR(uart_dbg, 0640, uart_dbg_show, uart_dbg_store);

static ssize_t
uart_error_cnt_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct s3c24xx_uart_port *ourport;

	sprintf(buf, "000 000 000 000\n");//init buf : overrun parity frame break count

	list_for_each_entry(ourport, &drvdata_list, node) {
		struct uart_port *port = &ourport->port;

		if (&ourport->pdev->dev != dev)
			continue;

	ret = sprintf(buf, "%03x %03x %03x %03x\n", port->icount.overrun, 0, port->icount.frame, port->icount.brk);

	}
	return ret;
}

static DEVICE_ATTR(error_cnt, 0444, uart_error_cnt_show, NULL);


struct proc_dir_entry *bluetooth_dir, *bt_log_dir;
struct proc_dir_entry *serial_dir, *serial_log_dir;

static void uart_copy_to_local_buf(int dir, struct uart_local_buf *local_buf,
		unsigned char *trace_buf, int len)
{
	unsigned long long time;
	unsigned long rem_nsec;
	int i;
	int cpu = raw_smp_processor_id();

	time = cpu_clock(cpu);
	rem_nsec = do_div(time, NSEC_PER_SEC);

	if (local_buf->index + (len * 3 + 30) >= local_buf->size)
		local_buf->index = 0;

	local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
			local_buf->size - local_buf->index,
			"[%5lu.%06lu] ",
			(unsigned long)time, rem_nsec / NSEC_PER_USEC);

	if (dir == 3) {
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
				local_buf->size - local_buf->index, "[termios] ");
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
				local_buf->size - local_buf->index,
				"%s", trace_buf);
	} else if (dir == 2) {
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
					local_buf->size - local_buf->index, "[reg] ");
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
					local_buf->size - local_buf->index,
					"%s", trace_buf);
	} else {
		if (dir == 1) {
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
				local_buf->size - local_buf->index, "[RX] ");
		} else {
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
				local_buf->size - local_buf->index, "[TX] ");
		}
		for (i = 0; i < len; i++) {
		local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
				local_buf->size - local_buf->index,
				"%02X ", trace_buf[i]);
		}
	}

	local_buf->index += scnprintf(local_buf->buffer + local_buf->index,
			local_buf->size - local_buf->index, "\n");
}

static void exynos_usi_init(struct uart_port *port);
static void exynos_usi_stop(struct uart_port *port);
static void s3c24xx_serial_resetport(struct uart_port *port,
				   struct s3c2410_uartcfg *cfg);
static void s3c24xx_serial_pm(struct uart_port *port, unsigned int level,
			      unsigned int old);
static struct uart_driver s3c24xx_uart_drv;

static inline void uart_clock_enable(struct s3c24xx_uart_port *ourport)
{
	if (ourport->check_separated_clk)
		clk_prepare_enable(ourport->separated_clk);
	clk_prepare_enable(ourport->clk);
}

static inline void uart_clock_disable(struct s3c24xx_uart_port *ourport)
{
	clk_disable_unprepare(ourport->clk);
	if (ourport->check_separated_clk)
		clk_disable_unprepare(ourport->separated_clk);
}

static inline struct s3c24xx_uart_port *to_ourport(struct uart_port *port)
{
	return container_of(port, struct s3c24xx_uart_port, port);
}

/* translate a port to the device name */

static inline const char *s3c24xx_serial_portname(struct uart_port *port)
{
	return to_platform_device(port->dev)->name;
}

static int s3c24xx_serial_txempty_nofifo(struct uart_port *port)
{
	return rd_regl(port, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXE;
}

/*
 * s3c64xx and later SoC's include the interrupt mask and status registers in
 * the controller itself, unlike the s3c24xx SoC's which have these registers
 * in the interrupt controller. Check if the port type is s3c64xx or higher.
 */
static int s3c24xx_serial_has_interrupt_mask(struct uart_port *port)
{
	return to_ourport(port)->info->type == PORT_S3C6400;
}

static void s3c24xx_serial_rx_enable(struct uart_port *port)
{
	unsigned long flags;
	unsigned int ucon, ufcon;
	int count = 10000;

	spin_lock_irqsave(&port->lock, flags);

	while (--count && !s3c24xx_serial_txempty_nofifo(port))
		udelay(100);

	ufcon = rd_regl(port, S3C2410_UFCON);
	ufcon |= S3C2410_UFCON_RESETRX;
	wr_regl(port, S3C2410_UFCON, ufcon);

	ucon = rd_regl(port, S3C2410_UCON);
	ucon |= S3C2410_UCON_RXIRQMODE;
	wr_regl(port, S3C2410_UCON, ucon);

	rx_enabled(port) = 1;
	spin_unlock_irqrestore(&port->lock, flags);
}

static void s3c24xx_serial_rx_disable(struct uart_port *port)
{
	unsigned long flags;
	unsigned int ucon;

	spin_lock_irqsave(&port->lock, flags);

	ucon = rd_regl(port, S3C2410_UCON);
	ucon &= ~S3C2410_UCON_RXIRQMODE;
	wr_regl(port, S3C2410_UCON, ucon);

	rx_enabled(port) = 0;
	spin_unlock_irqrestore(&port->lock, flags);
}

static void s3c24xx_serial_stop_tx(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (tx_enabled(port)) {
		if (s3c24xx_serial_has_interrupt_mask(port))
			__set_bit(S3C64XX_UINTM_TXD,
				portaddrl(port, S3C64XX_UINTM));
		else
			disable_irq_nosync(ourport->tx_irq);
		tx_enabled(port) = 0;
		if (port->flags & UPF_CONS_FLOW)
			s3c24xx_serial_rx_enable(port);
	}
}

static void s3c24xx_serial_start_tx(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (!tx_enabled(port)) {
		if (port->flags & UPF_CONS_FLOW)
			s3c24xx_serial_rx_disable(port);

		if (s3c24xx_serial_has_interrupt_mask(port))
			__clear_bit(S3C64XX_UINTM_TXD,
				portaddrl(port, S3C64XX_UINTM));
		else
			enable_irq(ourport->tx_irq);
		tx_enabled(port) = 1;
	}
}

static void s3c24xx_serial_stop_rx(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (rx_enabled(port)) {
		dbg("s3c24xx_serial_stop_rx: port=%p\n", port);
		if (s3c24xx_serial_has_interrupt_mask(port))
			__set_bit(S3C64XX_UINTM_RXD,
				portaddrl(port, S3C64XX_UINTM));
		else
			disable_irq_nosync(ourport->rx_irq);
		rx_enabled(port) = 0;
	}
}

static inline struct s3c24xx_uart_info
	*s3c24xx_port_to_info(struct uart_port *port)
{
	return to_ourport(port)->info;
}

static inline struct s3c2410_uartcfg
	*s3c24xx_port_to_cfg(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport;

	if (port->dev == NULL)
		return NULL;

	ourport = container_of(port, struct s3c24xx_uart_port, port);
	return ourport->cfg;
}

static int s3c24xx_serial_rx_fifocnt(struct s3c24xx_uart_port *ourport,
				     unsigned long ufstat)
{
	struct s3c24xx_uart_info *info = ourport->info;

	if (ufstat & info->rx_fifofull)
		return ourport->port.fifosize;

	return (ufstat & info->rx_fifomask) >> info->rx_fifoshift;
}

static int s3c24xx_serial_tx_fifocnt(struct s3c24xx_uart_port *ourport,
				     unsigned long ufstat)
{
	struct s3c24xx_uart_info *info = ourport->info;

	if (ufstat & info->tx_fifofull)
		return ourport->port.fifosize;

	return (ufstat & info->tx_fifomask) >> info->tx_fifoshift;
}

/* ? - where has parity gone?? */
#define S3C2410_UERSTAT_PARITY (0x1000)

static irqreturn_t
s3c24xx_serial_rx_chars(int irq, void *dev_id)
{
	struct s3c24xx_uart_port *ourport = dev_id;
	struct uart_port *port = &ourport->port;
	unsigned int ufcon, ch, flag, ufstat, uerstat;
	unsigned long flags;
	int fifocnt = 0;
	int max_count = port->fifosize;
	unsigned char insert_buf[256] = {0, };
	unsigned int insert_cnt = 0;
	unsigned char trace_buf[256] = {0, };
	int trace_cnt = 0;

	spin_lock_irqsave(&port->lock, flags);

	__set_bit(S3C64XX_UINTM_RXD, portaddrl(port, S3C64XX_UINTM));
	wr_regl(port, S3C64XX_UINTP, S3C64XX_UINTM_RXD_MSK);

	while (max_count-- > 0) {
		/*
		 * Receive all characters known to be in FIFO
		 * before reading FIFO level again
		 */
		if (fifocnt == 0) {
			ufstat = rd_regl(port, S3C2410_UFSTAT);
			fifocnt = s3c24xx_serial_rx_fifocnt(ourport, ufstat);
			if (fifocnt == 0)
				break;
		}
		fifocnt--;

		uerstat = rd_regl(port, S3C2410_UERSTAT);
		ch = rd_regb(port, S3C2410_URXH);

		if (port->flags & UPF_CONS_FLOW) {
			int txe = s3c24xx_serial_txempty_nofifo(port);

			if (rx_enabled(port)) {
				if (!txe) {
					rx_enabled(port) = 0;
					continue;
				}
			} else {
				if (txe) {
					ufcon = rd_regl(port, S3C2410_UFCON);
					ufcon |= S3C2410_UFCON_RESETRX;
					wr_regl(port, S3C2410_UFCON, ufcon);
					rx_enabled(port) = 1;
					spin_unlock_irqrestore(&port->lock,
							flags);
					goto out;
				}
				continue;
			}
		}

		/* insert the character into the buffer */

		flag = TTY_NORMAL;
		port->icount.rx++;

		if (unlikely(uerstat & S3C2410_UERSTAT_ANY)) {
			dbg("rxerr: port ch=0x%02x, rxs=0x%08x\n",
			    ch, uerstat);

			uart_sfr_dump(ourport);

			/* check for break */
			if (uerstat & S3C2410_UERSTAT_BREAK) {
				pr_err("[tty] uerstat & S3C2410_UERSTAT_BREAK1!\n");
				port->icount.brk++;
				if (uart_handle_break(port))
					goto ignore_char;
			}

			if (uerstat & S3C2410_UERSTAT_FRAME) {
				pr_err("[tty] uerstat & S3C2410_UERSTAT_FRAME!\n");
				port->icount.frame++;
			}
			if (uerstat & S3C2410_UERSTAT_OVERRUN) {
				pr_err("[tty] uerstat & S3C2410_UERSTAT_OVERRUN!\n");
				port->icount.overrun++;
			}
			uerstat &= port->read_status_mask;

			if (uerstat & S3C2410_UERSTAT_BREAK) {
				pr_err("[tty] uerstat & S3C2410_UERSTAT_BREAK2!\n");
				flag = TTY_BREAK;
			}
			else if (uerstat & S3C2410_UERSTAT_PARITY) {
				pr_err("[tty] uerstat & S3C2410_UERSTAT_PARITY!\n");
				flag = TTY_PARITY;
			}
			else if (uerstat & (S3C2410_UERSTAT_FRAME |
					    S3C2410_UERSTAT_OVERRUN))
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		insert_buf[insert_cnt++] = ch;
		if (ourport->uart_logging)
			trace_buf[trace_cnt++] = ch;

 ignore_char:
		continue;
	}

	if (ourport->uart_logging && trace_cnt)
		uart_copy_to_local_buf(1, &ourport->uart_local_buf, trace_buf, trace_cnt);

	__clear_bit(S3C64XX_UINTM_RXD, portaddrl(port, S3C64XX_UINTM));

	spin_unlock_irqrestore(&port->lock, flags);
	tty_insert_flip_string(&port->state->port, insert_buf, insert_cnt);
	tty_flip_buffer_push(&port->state->port);
	flush_workqueue(system_unbound_wq);

 out:
	if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
		sprintf(uart_log_buf,"[0] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
				rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
				rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
		uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
	}

	return IRQ_HANDLED;
}

static irqreturn_t s3c24xx_serial_tx_chars(int irq, void *id)
{
	struct s3c24xx_uart_port *ourport = id;
	struct uart_port *port = &ourport->port;
	struct circ_buf *xmit = &port->state->xmit;
	unsigned long flags;
	int count = port->fifosize;
	unsigned char trace_buf[256] = {0, };
	int trace_cnt = 0;

	if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
		sprintf(uart_log_buf,"[1] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
				rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
				rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
		uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
	}

	spin_lock_irqsave(&port->lock, flags);

	if (port->x_char) {
		wr_regb(port, S3C2410_UTXH, port->x_char);

		if (ourport->uart_logging)
			trace_buf[trace_cnt++] = port->x_char;

		port->icount.tx++;
		port->x_char = 0;
		goto out;
	}

	/* if there isn't anything more to transmit, or the uart is now
	 * stopped, disable the uart and exit
	*/

	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		s3c24xx_serial_stop_tx(port);
		goto out;
	}

	/* try and drain the buffer... */

	while (!uart_circ_empty(xmit) && count-- > 0) {
		if (rd_regl(port, S3C2410_UFSTAT) & ourport->info->tx_fifofull)
			break;

		wr_regb(port, S3C2410_UTXH, xmit->buf[xmit->tail]);

		if (ourport->uart_logging)
			trace_buf[trace_cnt++] = (unsigned char)xmit->buf[xmit->tail];

		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock_irqrestore(&port->lock, flags);
		uart_write_wakeup(port);
		spin_lock_irqsave(&port->lock, flags);
	}

	if (uart_circ_empty(xmit))
		s3c24xx_serial_stop_tx(port);

out:
	wr_regl(port, S3C64XX_UINTP, S3C64XX_UINTM_TXD_MSK);

	if (ourport->uart_logging && trace_cnt)
		uart_copy_to_local_buf(0, &ourport->uart_local_buf, trace_buf, trace_cnt);


	spin_unlock_irqrestore(&port->lock, flags);
	return IRQ_HANDLED;
}

#ifdef CONFIG_ARM_EXYNOS_DEVFREQ
static void s3c64xx_serial_qos_func(struct work_struct *work)
{
	struct s3c24xx_uart_port *ourport =
		container_of(work, struct s3c24xx_uart_port, qos_work.work);
	struct uart_port *port = &ourport->port;

	if (ourport->mif_qos_val)
		pm_qos_update_request_timeout(&ourport->s3c24xx_uart_mif_qos,
				ourport->mif_qos_val, ourport->qos_timeout);

	if (ourport->cpu_qos_val)
		pm_qos_update_request_timeout(&ourport->s3c24xx_uart_cpu_qos,
				ourport->cpu_qos_val, ourport->qos_timeout);

	if (ourport->uart_irq_affinity)
		irq_set_affinity(port->irq, cpumask_of(ourport->uart_irq_affinity));
}
#endif

/* interrupt handler for s3c64xx and later SoC's.*/
static irqreturn_t s3c64xx_serial_handle_irq(int irq, void *id)
{
	struct s3c24xx_uart_port *ourport = id;
	struct uart_port *port = &ourport->port;
	irqreturn_t ret = IRQ_HANDLED;

#ifdef CONFIG_PM_DEVFREQ
	if ((ourport->mif_qos_val || ourport->cpu_qos_val)
					&& ourport->qos_timeout)
		schedule_delayed_work(&ourport->qos_work,
						msecs_to_jiffies(100));
#endif

	if (rd_regl(port, S3C64XX_UINTP) & S3C64XX_UINTM_RXD_MSK)
		ret = s3c24xx_serial_rx_chars(irq, id);

	if (rd_regl(port, S3C64XX_UINTP) & S3C64XX_UINTM_TXD_MSK)
		ret = s3c24xx_serial_tx_chars(irq, id);

	return ret;
}

static unsigned int s3c24xx_serial_tx_empty(struct uart_port *port)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);
	unsigned long ufstat = rd_regl(port, S3C2410_UFSTAT);
	unsigned long ufcon = rd_regl(port, S3C2410_UFCON);

	if (ufcon & S3C2410_UFCON_FIFOMODE) {
		if ((ufstat & info->tx_fifomask) != 0 ||
		    (ufstat & info->tx_fifofull))
			return 0;

		return 1;
	}

	return s3c24xx_serial_txempty_nofifo(port);
}

/* no modem control lines */
static unsigned int s3c24xx_serial_get_mctrl(struct uart_port *port)
{
	unsigned int umstat = rd_regb(port, S3C2410_UMSTAT);

	if (umstat & S3C2410_UMSTAT_CTS)
		return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
	else
		return TIOCM_CAR | TIOCM_DSR;
}

static void s3c24xx_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	unsigned int umcon = rd_regl(port, S3C2410_UMCON);
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (mctrl & TIOCM_RTS)
		umcon |= S3C2410_UMCOM_RTS_LOW;
	else
		umcon &= ~S3C2410_UMCOM_RTS_LOW;

	wr_regl(port, S3C2410_UMCON, umcon);
	if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
		sprintf(uart_log_buf,"[2] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
				rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
				rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
		uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
	}

}

static void s3c24xx_serial_break_ctl(struct uart_port *port, int break_state)
{
	unsigned long flags;
	unsigned int ucon;

	spin_lock_irqsave(&port->lock, flags);

	ucon = rd_regl(port, S3C2410_UCON);

	if (break_state)
		ucon |= S3C2410_UCON_SBREAK;
	else
		ucon &= ~S3C2410_UCON_SBREAK;

	wr_regl(port, S3C2410_UCON, ucon);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void s3c24xx_serial_shutdown(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (ourport->tx_claimed) {
		if (!s3c24xx_serial_has_interrupt_mask(port))
			free_irq(ourport->tx_irq, ourport);
		tx_enabled(port) = 0;
		ourport->tx_claimed = 0;
	}

	if (ourport->rx_claimed) {
		if (!s3c24xx_serial_has_interrupt_mask(port))
			free_irq(ourport->rx_irq, ourport);
		ourport->rx_claimed = 0;
		rx_enabled(port) = 0;
	}

	/* Clear pending interrupts and mask all interrupts */
	if (s3c24xx_serial_has_interrupt_mask(port)) {
		free_irq(port->irq, ourport);

		wr_regl(port, S3C64XX_UINTP, 0xf);
		wr_regl(port, S3C64XX_UINTM, 0xf);
	}
}

static int s3c24xx_serial_startup(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	int ret;

	dbg("s3c24xx_serial_startup: port=%p (%08llx,%p)\n",
	    port, (unsigned long long)port->mapbase, port->membase);

	ourport->cfg->wake_peer[port->line] =
				s3c2410_serial_wake_peer[port->line];

	rx_enabled(port) = 1;

	ret = request_irq(ourport->rx_irq, s3c24xx_serial_rx_chars, 0,
			  s3c24xx_serial_portname(port), ourport);

	if (ret != 0) {
		dev_err(port->dev, "cannot get irq %d\n", ourport->rx_irq);
		return ret;
	}

	ourport->rx_claimed = 1;

	dbg("requesting tx irq...\n");

	tx_enabled(port) = 1;

	ret = request_irq(ourport->tx_irq, s3c24xx_serial_tx_chars, 0,
			  s3c24xx_serial_portname(port), ourport);

	if (ret) {
		dev_err(port->dev, "cannot get irq %d\n", ourport->tx_irq);
		goto err;
	}

	ourport->tx_claimed = 1;

	dbg("s3c24xx_serial_startup ok\n");

	/* the port reset code should have done the correct
	 * register setup for the port controls */

	return ret;

err:
	s3c24xx_serial_shutdown(port);
	return ret;
}

static int s3c64xx_serial_startup(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	unsigned long flags;
	int ret;

	dbg("s3c64xx_serial_startup: port=%p (%08llx,%p)\n",
	    port, (unsigned long long)port->mapbase, port->membase);

	ourport->cfg->wake_peer[port->line] =
				s3c2410_serial_wake_peer[port->line];

	spin_lock_irqsave(&port->lock, flags);
	wr_regl(port, S3C64XX_UINTM, 0xf);
	spin_unlock_irqrestore(&port->lock, flags);

	if (ourport->use_default_irq == 1)
		ret = devm_request_irq(port->dev, port->irq, s3c64xx_serial_handle_irq,
				IRQF_SHARED, s3c24xx_serial_portname(port), ourport);
	else
		ret = request_threaded_irq(port->irq, NULL, s3c64xx_serial_handle_irq,
				IRQF_ONESHOT, s3c24xx_serial_portname(port), ourport);

	if (ret) {
		dev_err(port->dev, "cannot get irq %d\n", port->irq);
		return ret;
	}

	/* For compatibility with s3c24xx Soc's */
	rx_enabled(port) = 1;
	ourport->rx_claimed = 1;
	tx_enabled(port) = 0;
	ourport->tx_claimed = 1;

	/* Enable Rx Interrupt */
	spin_lock_irqsave(&port->lock, flags);
	__clear_bit(S3C64XX_UINTM_RXD, portaddrl(port, S3C64XX_UINTM));
	spin_unlock_irqrestore(&port->lock, flags);
	dbg("s3c64xx_serial_startup ok\n");
	return ret;
}

/* power power management control */
static void s3c24xx_serial_pm(struct uart_port *port, unsigned int level,
			      unsigned int old)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	unsigned int umcon;

	switch (level) {
	case S3C24XX_UART_PORT_SUSPEND:
		if (!ourport->in_band_wakeup) {
			/* disable auto flow control & set nRTS for High */
			umcon = rd_regl(port, S3C2410_UMCON);
			umcon &= ~(S3C2410_UMCOM_AFC | S3C2410_UMCOM_RTS_LOW);
			wr_regl(port, S3C2410_UMCON, umcon);
			if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
				sprintf(uart_log_buf,"[3] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
						rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
						rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
				uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
			}
		}

		uart_clock_disable(ourport);
		break;

	case S3C24XX_UART_PORT_RESUME:
		uart_clock_enable(ourport);

		if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
			sprintf(uart_log_buf,"[4] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
					rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
					rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
			uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
		}
		exynos_usi_init(port);
		s3c24xx_serial_resetport(port, s3c24xx_port_to_cfg(port));
		break;
	default:
		dev_err(port->dev, "s3c24xx_serial: unknown pm %d\n", level);
	}
}

/* baud rate calculation
 *
 * The UARTs on the S3C2410/S3C2440 can take their clocks from a number
 * of different sources, including the peripheral clock ("pclk") and an
 * external clock ("uclk"). The S3C2440 also adds the core clock ("fclk")
 * with a programmable extra divisor.
 *
 * The following code goes through the clock sources, and calculates the
 * baud clocks (and the resultant actual baud rates) and then tries to
 * pick the closest one and select that.
 *
*/

#define MAX_CLK_NAME_LENGTH 20

static unsigned int s3c24xx_serial_getclk(struct s3c24xx_uart_port *ourport,
			unsigned int req_baud, struct clk **best_clk,
			unsigned int *clk_num)
{
	struct s3c24xx_uart_info *info = ourport->info;
	unsigned long rate;
	unsigned int cnt, baud, quot, clk_sel, best_quot = 0;
	int calc_deviation, deviation = (1 << 30) - 1;
	int ret;

	clk_sel = (ourport->cfg->clk_sel) ? ourport->cfg->clk_sel :
			ourport->info->def_clk_sel;
	for (cnt = 0; cnt < info->num_clks; cnt++) {
		if (!(clk_sel & (1 << cnt)))
			continue;

		rate = clk_get_rate(ourport->clk);

		if (ourport->src_clk_rate && rate != ourport->src_clk_rate)
		{
			ret = clk_set_rate(ourport->clk, ourport->src_clk_rate);
			if (ret < 0)
				dev_err(&ourport->pdev->dev, "UART clk set failed\n");

			rate = clk_get_rate(ourport->clk);
		} else {
			ret = clk_set_rate(ourport->clk, ourport->src_clk_rate);
			if (ret < 0)
				dev_err(&ourport->pdev->dev, "UART Default clk set failed\n");

			rate = clk_get_rate(ourport->clk);
		}

		dev_info(&ourport->pdev->dev, " Clock rate : %ld\n", rate);

		if (!rate)
			continue;

		if (ourport->info->has_divslot) {
			unsigned long div = rate / req_baud;

			/* The UDIVSLOT register on the newer UARTs allows us to
			 * get a divisor adjustment of 1/16th on the baud clock.
			 *
			 * We don't keep the UDIVSLOT value (the 16ths we
			 * calculated by not multiplying the baud by 16) as it
			 * is easy enough to recalculate.
			 */

			quot = div / 16;
			baud = rate / div;
		} else {
			quot = (rate + (8 * req_baud)) / (16 * req_baud);
			baud = rate / (quot * 16);
		}
		quot--;

		calc_deviation = req_baud - baud;
		if (calc_deviation < 0)
			calc_deviation = -calc_deviation;

		if (calc_deviation < deviation) {
			*best_clk = ourport->clk;
			best_quot = quot;
			*clk_num = cnt;
			deviation = calc_deviation;
		}
	}

	return best_quot;
}

/* udivslot_table[]
 *
 * This table takes the fractional value of the baud divisor and gives
 * the recommended setting for the UDIVSLOT register.
 */
static u16 udivslot_table[16] = {
	[0] = 0x0000,
	[1] = 0x0080,
	[2] = 0x0808,
	[3] = 0x0888,
	[4] = 0x2222,
	[5] = 0x4924,
	[6] = 0x4A52,
	[7] = 0x54AA,
	[8] = 0x5555,
	[9] = 0xD555,
	[10] = 0xD5D5,
	[11] = 0xDDD5,
	[12] = 0xDDDD,
	[13] = 0xDFDD,
	[14] = 0xDFDF,
	[15] = 0xFFDF,
};

static void s3c24xx_serial_set_termios(struct uart_port *port,
				       struct ktermios *termios,
				       struct ktermios *old)
{
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	struct clk *clk = ERR_PTR(-EINVAL);
	unsigned long flags;
	unsigned int baud, quot, clk_sel = 0;
	unsigned int ulcon;
	unsigned int umcon;
	unsigned int udivslot = 0;
	unsigned int real_baud_rd, real_baud_ru = 0;
	int calc_deviation_rd, calc_deviation_ru = 0;

	/*
	 * We don't support modem control lines.
	 */
	termios->c_cflag &= ~(HUPCL | CMSPAR);
	termios->c_cflag |= CLOCAL;

	/*
	 * Ask the core to calculate the divisor for us.
	 */

	baud = uart_get_baud_rate(port, termios, old, MIN_BAUD, MAX_BAUD);
	quot = s3c24xx_serial_getclk(ourport, baud, &clk, &clk_sel);
	if (baud == 38400 && (port->flags & UPF_SPD_MASK) == UPF_SPD_CUST)
		quot = port->custom_divisor;
	if (IS_ERR(clk))
		return;

	/* setting clock for baud rate */
	if (ourport->baudclk != clk) {
		ourport->baudclk = clk;
		ourport->baudclk_rate = clk ? clk_get_rate(clk) : 0;
	}

	if (ourport->info->has_divslot) {
		unsigned int div = ourport->baudclk_rate / baud;

		/*
		 * Find udivslot of the lowest error rate
		 *
		 * udivslot cannot be round-up to 0 because quot is fixed
		 */
		if((div & 15) != 15) {
			real_baud_rd = ourport->baudclk_rate / div;
			real_baud_ru = ourport->baudclk_rate / (div + 1);

			calc_deviation_rd = baud - real_baud_rd;
			calc_deviation_ru = baud - real_baud_ru;

			if(calc_deviation_rd < 0)
				calc_deviation_rd = -calc_deviation_rd;
			if(calc_deviation_ru < 0)
				calc_deviation_ru = -calc_deviation_ru;

			if(calc_deviation_rd > calc_deviation_ru)
			  div = div + 1;
		}

		if (cfg->has_fracval) {
			udivslot = (div & 15);
			dbg("fracval = %04x\n", udivslot);
		} else {
			udivslot = udivslot_table[div & 15];
			dbg("udivslot = %04x (div %d)\n", udivslot, div & 15);
		}
	}

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		dbg("config: 5bits/char\n");
		ulcon = S3C2410_LCON_CS5;
		break;
	case CS6:
		dbg("config: 6bits/char\n");
		ulcon = S3C2410_LCON_CS6;
		break;
	case CS7:
		dbg("config: 7bits/char\n");
		ulcon = S3C2410_LCON_CS7;
		break;
	case CS8:
	default:
		dbg("config: 8bits/char\n");
		ulcon = S3C2410_LCON_CS8;
		break;
	}

	/* preserve original lcon IR settings */
	if (!ourport->usi_v2)
		ulcon |= (unsigned int)(cfg->ulcon & S3C2410_LCON_IRM);

	if (termios->c_cflag & CSTOPB)
		ulcon |= S3C2410_LCON_STOPB;

	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			ulcon |= S3C2410_LCON_PODD;
		else
			ulcon |= S3C2410_LCON_PEVEN;
	} else {
		ulcon |= S3C2410_LCON_PNONE;
	}

	spin_lock_irqsave(&port->lock, flags);

	dbg("setting ulcon to %08x, brddiv to %d, udivslot %08x\n",
	    ulcon, quot, udivslot);

	wr_regl(port, S3C2410_ULCON, ulcon);
	wr_regl(port, S3C2410_UBRDIV, quot);

	if (ourport->info->has_divslot)
		wr_regl(port, S3C2443_DIVSLOT, udivslot);
	port->status &= ~UPSTAT_AUTOCTS;

	umcon = rd_regl(port, S3C2410_UMCON);
	if (termios->c_cflag & CRTSCTS) {
		umcon |= S3C2410_UMCOM_AFC;
		port->status = UPSTAT_AUTOCTS;
	} else {
		umcon &= ~S3C2410_UMCOM_AFC;
	}
	wr_regl(port, S3C2410_UMCON, umcon);

	dbg("uart: ulcon = 0x%08x, ucon = 0x%08x, ufcon = 0x%08x\n",
	    rd_regl(port, S3C2410_ULCON),
	    rd_regl(port, S3C2410_UCON),
	    rd_regl(port, S3C2410_UFCON));

	if (ourport->dbg_mode & UART_DBG_MODE)
		print_uart_mode(port, termios, baud);

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * Which character status flags are we interested in?
	 */
	port->read_status_mask = S3C2410_UERSTAT_OVERRUN;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= S3C2410_UERSTAT_FRAME |
			S3C2410_UERSTAT_PARITY;
	/*
	 * Which character status flags should we ignore?
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= S3C2410_UERSTAT_OVERRUN;
	if (termios->c_iflag & IGNBRK && termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= S3C2410_UERSTAT_FRAME;

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= RXSTAT_DUMMY_READ;

	if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
		sprintf(uart_log_buf,"baudrate:%u ULCON:0x%08x UBRDIV:0x%08x UFRAVAL:0x%08x",
				baud, rd_regl(port, S3C2410_ULCON),
				rd_regl(port, S3C2410_UBRDIV),
				rd_regl(port, S3C2443_DIVSLOT));
		uart_copy_to_local_buf(3, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
	}
	spin_unlock_irqrestore(&port->lock, flags);

}

static const char *s3c24xx_serial_type(struct uart_port *port)
{
	switch (port->type) {
	case PORT_S3C2410:
		return "S3C2410";
	case PORT_S3C2440:
		return "S3C2440";
	case PORT_S3C2412:
		return "S3C2412";
	case PORT_S3C6400:
		return "S3C6400/10";
	default:
		return NULL;
	}
}

#define MAP_SIZE (0x100)

static void s3c24xx_serial_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, MAP_SIZE);
}

static int s3c24xx_serial_request_port(struct uart_port *port)
{
	const char *name = s3c24xx_serial_portname(port);
	return request_mem_region(port->mapbase, MAP_SIZE, name) ? 0 : -EBUSY;
}

static void s3c24xx_serial_config_port(struct uart_port *port, int flags)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	if (flags & UART_CONFIG_TYPE &&
	    s3c24xx_serial_request_port(port) == 0)
		port->type = info->type;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int
s3c24xx_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);

	if (ser->type != PORT_UNKNOWN && ser->type != info->type)
		return -EINVAL;

	return 0;
}

static void s3c24xx_serial_wake_peer(struct uart_port *port)
{
	struct s3c2410_uartcfg *cfg = s3c24xx_port_to_cfg(port);

	if (cfg->wake_peer[port->line])
		cfg->wake_peer[port->line](port);
}

#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE

static struct console s3c24xx_serial_console;

static int __init s3c24xx_serial_console_init(void)
{
	struct clk *console_clk;
	char pclk_name[16], sclk_name[16];

	snprintf(pclk_name, sizeof(pclk_name), "console-pclk%d", CONFIG_S3C_LOWLEVEL_UART_PORT);
	snprintf(sclk_name, sizeof(sclk_name), "console-sclk%d", CONFIG_S3C_LOWLEVEL_UART_PORT);

	pr_info("Enable clock for console to add reference counter\n");

	console_clk = clk_get(NULL, pclk_name);
	if (IS_ERR(console_clk)) {
		pr_err("Can't get %s!(it's not err)\n", pclk_name);
	} else {
		clk_prepare_enable(console_clk);
	}

	console_clk = clk_get(NULL, sclk_name);
	if (IS_ERR(console_clk)) {
		pr_err("Can't get %s!(it's not err)\n", sclk_name);
	} else {
		clk_prepare_enable(console_clk);
	}

	register_console(&s3c24xx_serial_console);
	return 0;
}
console_initcall(s3c24xx_serial_console_init);

#define S3C24XX_SERIAL_CONSOLE &s3c24xx_serial_console
#else
#define S3C24XX_SERIAL_CONSOLE NULL
#endif

#if defined(CONFIG_SERIAL_SAMSUNG_CONSOLE) && defined(CONFIG_CONSOLE_POLL)
static int s3c24xx_serial_get_poll_char(struct uart_port *port);
static void s3c24xx_serial_put_poll_char(struct uart_port *port,
			 unsigned char c);
#endif

static struct uart_ops s3c24xx_serial_ops = {
	.pm		= s3c24xx_serial_pm,
	.tx_empty	= s3c24xx_serial_tx_empty,
	.get_mctrl	= s3c24xx_serial_get_mctrl,
	.set_mctrl	= s3c24xx_serial_set_mctrl,
	.stop_tx	= s3c24xx_serial_stop_tx,
	.start_tx	= s3c24xx_serial_start_tx,
	.stop_rx	= s3c24xx_serial_stop_rx,
	.break_ctl	= s3c24xx_serial_break_ctl,
	.startup	= s3c24xx_serial_startup,
	.shutdown	= s3c24xx_serial_shutdown,
	.set_termios	= s3c24xx_serial_set_termios,
	.type		= s3c24xx_serial_type,
	.release_port	= s3c24xx_serial_release_port,
	.request_port	= s3c24xx_serial_request_port,
	.config_port	= s3c24xx_serial_config_port,
	.verify_port	= s3c24xx_serial_verify_port,
	.wake_peer	= s3c24xx_serial_wake_peer,
#if defined(CONFIG_SERIAL_SAMSUNG_CONSOLE) && defined(CONFIG_CONSOLE_POLL)
	.poll_get_char = s3c24xx_serial_get_poll_char,
	.poll_put_char = s3c24xx_serial_put_poll_char,
#endif
};

static struct uart_driver s3c24xx_uart_drv = {
	.owner		= THIS_MODULE,
	.driver_name	= "s3c2410_serial",
	.nr		= CONFIG_SERIAL_SAMSUNG_UARTS,
	.cons		= S3C24XX_SERIAL_CONSOLE,
	.dev_name	= S3C24XX_SERIAL_NAME,
	.major		= S3C24XX_SERIAL_MAJOR,
	.minor		= S3C24XX_SERIAL_MINOR,
};

#define __PORT_LOCK_UNLOCKED(i) \
	__SPIN_LOCK_UNLOCKED(s3c24xx_serial_ports[i].port.lock)
static struct s3c24xx_uart_port
s3c24xx_serial_ports[CONFIG_SERIAL_SAMSUNG_UARTS] = {
	[0] = {
		.port = {
			.lock		= __PORT_LOCK_UNLOCKED(0),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 0,
		}
	},
	[1] = {
		.port = {
			.lock		= __PORT_LOCK_UNLOCKED(1),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 1,
		}
	},
#if CONFIG_SERIAL_SAMSUNG_UARTS > 2

	[2] = {
		.port = {
			.lock		= __PORT_LOCK_UNLOCKED(2),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 2,
		}
	},
#endif
#if CONFIG_SERIAL_SAMSUNG_UARTS > 3
	[3] = {
		.port = {
			.lock		= __PORT_LOCK_UNLOCKED(3),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 3,
		}
	},
#endif
#if CONFIG_SERIAL_SAMSUNG_UARTS > 4
	[4] = {
		.port = {
			.lock		= __PORT_LOCK_UNLOCKED(4),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 4,
		}
	},
#endif
#if CONFIG_SERIAL_SAMSUNG_UARTS > 5
	[5] = {
		.port = {
			.lock		= __PORT_LOCK_UNLOCKED(5),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.fifosize	= 16,
			.ops		= &s3c24xx_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 5,
		}
	},
#endif
};

static struct s3c24xx_uart_port *exynos_serial_default_port(int port_index)
{
	s3c24xx_serial_ports[port_index].port.lock = __PORT_LOCK_UNLOCKED(port_index);
	s3c24xx_serial_ports[port_index].port.iotype = UPIO_MEM;
	s3c24xx_serial_ports[port_index].port.uartclk = 0;
	s3c24xx_serial_ports[port_index].port.fifosize = 0;
	s3c24xx_serial_ports[port_index].port.ops =
		&s3c24xx_serial_ops;
	s3c24xx_serial_ports[port_index].port.flags = UPF_BOOT_AUTOCONF;
	s3c24xx_serial_ports[port_index].port.line = port_index;

	return &s3c24xx_serial_ports[port_index];
}
#undef __PORT_LOCK_UNLOCKED

static void exynos_usi_init(struct uart_port *port)
{
	/* USI_RESET is active High signal.
	 * Reset value of USI_RESET is 'h1 to drive stable value to PAD.
	 * Due to this feature, the USI_RESET must be cleared (set as '0')
	 * before transaction starts.
	 */
	wr_regl(port, USI_CON, USI_SET_RESET);
	udelay(1);

	wr_regl(port, USI_CON, USI_RESET);
	udelay(1);

	/* set the HWACG option bit in case of UART Rx mode.
	 * CLKREQ_ON = 1, CLKSTOP_ON = 0 (set USI_OPTION[2:1] = 2'h1)
	 */
	wr_regl(port, USI_OPTION, USI_HWACG_CLKREQ_ON);
	udelay(1);
}

static void exynos_usi_stop(struct uart_port *port)
{
	/* when USI CLKSTOP_ON is set high, this makes the
	 * Q-ch state enter into STOP state by driving both the
	 * IP_CLKREQ and IP_BUSACTREQ as low
	 */
	wr_regl(port, USI_OPTION, USI_HWACG_CLKSTOP_ON);
}

/* s3c24xx_serial_resetport
 *
 * reset the fifos and other the settings.
*/

static void s3c24xx_serial_resetport(struct uart_port *port,
				   struct s3c2410_uartcfg *cfg)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	unsigned long ucon = rd_regl(port, S3C2410_UCON);
	unsigned long umcon = rd_regl(port, S3C2410_UMCON);
	unsigned int ucon_mask;

	ucon_mask = info->clksel_mask;
	if (info->type == PORT_S3C2440)
		ucon_mask |= S3C2440_UCON0_DIVMASK;

	ucon &= ucon_mask;
	if (ourport->dbg_mode & UART_LOOPBACK_MODE) {
		dev_err(port->dev, "Change Loopback mode!\n");
		ucon |= S3C2443_UCON_LOOPBACK;
	}

	/* Set rts trigger level */
	umcon &= ~S5PV210_UMCON_RTSTRIG32;
	if (ourport->rts_trig_level && info->rts_trig_shift)
		umcon |= ourport->rts_trig_level << info->rts_trig_shift;
	wr_regl(port, S3C2410_UMCON, umcon);

	/* To prevent unexpected Interrupt before enabling the channel */
	wr_regl(port, S3C64XX_UINTM, 0xf);

	/* reset both fifos */
	wr_regl(port, S3C2410_UFCON, cfg->ufcon);

	wr_regl(port, S3C2410_UCON,  ucon | cfg->ucon);
}

/* s3c24xx_serial_init_port
 *
 * initialise a single serial port from the platform device given
 */

static int s3c24xx_serial_init_port(struct s3c24xx_uart_port *ourport,
				    struct platform_device *platdev)
{
	struct uart_port *port = &ourport->port;
	struct s3c2410_uartcfg *cfg = ourport->cfg;
	struct resource *res;
	char clkname[MAX_CLK_NAME_LENGTH];
	int ret;

	dbg("s3c24xx_serial_init_port: port=%p, platdev=%p\n", port, platdev);

	if (platdev == NULL)
		return -ENODEV;

	if (port->mapbase != 0)
		return -EINVAL;

	/* setup info for port */
	port->dev	= &platdev->dev;
	ourport->pdev	= platdev;

	/* Startup sequence is different for s3c64xx and higher SoC's */
	if (s3c24xx_serial_has_interrupt_mask(port))
		s3c24xx_serial_ops.startup = s3c64xx_serial_startup;

	port->uartclk = 1;

	if (cfg->uart_flags & UPF_CONS_FLOW) {
		dbg("s3c24xx_serial_init_port: enabling flow control\n");
		port->flags |= UPF_CONS_FLOW;
	}

	/* sort our the physical and virtual addresses for each UART */

	res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(port->dev, "failed to find memory resource for uart\n");
		return -EINVAL;
	}

	dbg("resource %pR)\n", res);

	port->membase = devm_ioremap(port->dev, res->start, resource_size(res));
	if (!port->membase) {
		dev_err(port->dev, "failed to remap controller address\n");
		return -EBUSY;
	}

	port->mapbase = res->start;
	ret = platform_get_irq(platdev, 0);
	if (ret < 0)
		port->irq = 0;
	else {
		port->irq = ret;
		ourport->rx_irq = ret;
		ourport->tx_irq = ret + 1;
	}

	ret = platform_get_irq(platdev, 1);
	if (ret > 0)
		ourport->tx_irq = ret;

	if (of_get_property(platdev->dev.of_node,
			"samsung,separate-uart-clk", NULL))
		ourport->check_separated_clk = 1;
	else
		ourport->check_separated_clk = 0;

	if (of_property_read_u32(platdev->dev.of_node, "samsung,source-clock-rate", &ourport->src_clk_rate)){
		dev_err(&platdev->dev, "No explicit src-clk. Use default src-clk\n");
		ourport->src_clk_rate = DEFAULT_SOURCE_CLK;
	}

	snprintf(clkname, sizeof(clkname), "ipclk_uart%d", ourport->port.line);
	ourport->clk = devm_clk_get(&platdev->dev, clkname);
	if (IS_ERR(ourport->clk)) {
		pr_err("%s: Controller clock not found\n",
				dev_name(&platdev->dev));
		return PTR_ERR(ourport->clk);
	}

	if (ourport->check_separated_clk) {
		snprintf(clkname, sizeof(clkname), "gate_uart_clk%d", ourport->port.line);
		ourport->separated_clk = devm_clk_get(&platdev->dev, clkname);
		if (IS_ERR(ourport->separated_clk)) {
			pr_err("%s: Controller clock not found\n",
					dev_name(&platdev->dev));
			return PTR_ERR(ourport->separated_clk);
		}

		ret = clk_prepare_enable(ourport->separated_clk);
		if (ret) {
			pr_err("uart: clock failed to prepare+enable: %d\n", ret);
			return ret;
		}
	}

	ret = clk_prepare_enable(ourport->clk);
	if (ret) {
		pr_err("uart: clock failed to prepare+enable: %d\n", ret);
		return ret;
	}

	exynos_usi_init(port);

	/* Keep all interrupts masked and cleared */
	if (s3c24xx_serial_has_interrupt_mask(port)) {
		wr_regl(port, S3C64XX_UINTM, 0xf);
		wr_regl(port, S3C64XX_UINTP, 0xf);
		wr_regl(port, S3C64XX_UINTSP, 0xf);
	}

	dbg("port: map=%pa, mem=%p, irq=%d (%d,%d), clock=%u\n",
	    &port->mapbase, port->membase, port->irq,
	    ourport->rx_irq, ourport->tx_irq, port->uartclk);

	/* reset the fifos (and setup the uart) */
	s3c24xx_serial_resetport(port, cfg);
	return 0;
}

#ifdef CONFIG_SAMSUNG_CLOCK
static ssize_t s3c24xx_serial_show_clksrc(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct uart_port *port = s3c24xx_dev_to_port(dev);
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (IS_ERR(ourport->baudclk))
		return -EINVAL;

	return snprintf(buf, PAGE_SIZE, "* %s\n",
			ourport->baudclk->name ?: "(null)");
}

static DEVICE_ATTR(clock_source, S_IRUGO, s3c24xx_serial_show_clksrc, NULL);
#endif

/* Device driver serial port probe */

static const struct of_device_id s3c24xx_uart_dt_match[];
static int probe_index;

static inline struct s3c24xx_serial_drv_data *s3c24xx_get_driver_data(
			struct platform_device *pdev)
{
#ifdef CONFIG_OF
	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		match = of_match_node(s3c24xx_uart_dt_match, pdev->dev.of_node);
		return (struct s3c24xx_serial_drv_data *)match->data;
	}
#endif
	return (struct s3c24xx_serial_drv_data *)
			platform_get_device_id(pdev)->driver_data;
}


void s3c24xx_serial_rx_fifo_wait(void)
{
	struct s3c24xx_uart_port *ourport;
	struct uart_port *port;
	unsigned int fifo_stat;
	unsigned long wait_time;
	unsigned int fifo_count;
	unsigned long flags;

	fifo_count = 0;

	list_for_each_entry(ourport, &drvdata_list, node) {
		if (ourport->port.line != CONFIG_S3C_LOWLEVEL_UART_PORT)
			continue;

		port = &ourport->port;

		spin_lock_irqsave(&port->lock, flags);

		fifo_stat = rd_regl(port, S3C2410_UFSTAT);
		fifo_count = s3c24xx_serial_rx_fifocnt(ourport, fifo_stat);
		if (fifo_count) {
			uart_clock_enable(ourport);
			__clear_bit(S3C64XX_UINTM_RXD, portaddrl(port, S3C64XX_UINTM));
			uart_clock_disable(ourport);
			rx_enabled(port) = 1;
		}
		wait_time = jiffies + HZ;
		do {
			port = &ourport->port;
			fifo_stat = rd_regl(port, S3C2410_UFSTAT);
			cpu_relax();
		} while (s3c24xx_serial_rx_fifocnt(ourport, fifo_stat) && time_before(jiffies, wait_time));

		spin_unlock_irqrestore(&port->lock, flags);

		if (rx_enabled(port))
			s3c24xx_serial_stop_rx(port);
	}
}

EXPORT_SYMBOL_GPL(s3c24xx_serial_rx_fifo_wait);

void s3c24xx_serial_fifo_wait(void)
{
	struct s3c24xx_uart_port *ourport;
	struct uart_port *port;
	unsigned int fifo_stat;
	unsigned long wait_time;

	list_for_each_entry(ourport, &drvdata_list, node) {
		if (ourport->port.line != CONFIG_S3C_LOWLEVEL_UART_PORT)
			continue;

		wait_time = jiffies + HZ / 4;
		do {
			port = &ourport->port;
			fifo_stat = rd_regl(port, S3C2410_UFSTAT);
			cpu_relax();
		} while (s3c24xx_serial_tx_fifocnt(ourport, fifo_stat)
				&& time_before(jiffies, wait_time));
	}
}
EXPORT_SYMBOL_GPL(s3c24xx_serial_fifo_wait);

#define PT_L(x) x ? "BT" : "UART"
static void s3c24xx_print_reg_status(struct s3c24xx_uart_port *ourport)
{
		struct uart_port *port = &ourport->port;

		unsigned int ulcon = rd_regl(port, S3C2410_ULCON);
		unsigned int ucon = rd_regl(port, S3C2410_UCON);
		unsigned int ufcon = rd_regl(port, S3C2410_UFCON);
		unsigned int umcon = rd_regl(port, S3C2410_UMCON);
		unsigned int utrstat = rd_regl(port, S3C2410_UTRSTAT);
		unsigned int ufstat = rd_regl(port, S3C2410_UFSTAT);
		unsigned int umstat = rd_regl(port, S3C2410_UMSTAT);
		unsigned int uerstat = rd_regl(port, S3C2410_UERSTAT);

		int tx_fifo_full = ufstat & S5PV210_UFSTAT_TXFULL;
		int tx_fifo_count = s3c24xx_serial_tx_fifocnt(ourport, ufstat);

		int rx_fifo_full = ufstat & S5PV210_UFSTAT_RXFULL;
		int rx_fifo_count = s3c24xx_serial_rx_fifocnt(ourport, ufstat);

		pr_err("[%s]: ulcon = 0x%08x, ucon = 0x%08x, ufcon = 0x%08x\n, umcon = 0x%08x\n", PT_L(port->line), ulcon, ucon, ufcon, umcon);
		pr_err("[%s]: utrstat = 0x%08x, ufstat = 0x%08x, umstat = 0x%08x\n", PT_L(port->line), utrstat, ufstat, umstat);
		pr_err("[%s]: uerstat = 0x%08x\n", PT_L(port->line), uerstat);
		pr_err("[%s]: tx_fifo_full = %d, tx_fifo_count = %d\n", PT_L(port->line), tx_fifo_full, tx_fifo_count);
		pr_err("[%s]: rx_fifo_full = %d, rx_fifo_count = %d\n", PT_L(port->line), rx_fifo_full, rx_fifo_count);
}

#ifdef BT_UART_TRACE
static ssize_t s3c24xx_serial_bt_log(struct file *file, char __user *userbuf, size_t bytes, loff_t *off)
{
	int ret;
	struct s3c24xx_uart_port *ourport = &s3c24xx_serial_ports[BLUETOOTH_UART_PORT_LINE];
	static int copied_bytes;

	if (copied_bytes >= LOG_BUFFER_SIZE) {
		struct uart_port *port;

		port = &ourport->port;

		copied_bytes = 0;

		if (port && port->state->pm_state == UART_PM_STATE_ON)
			s3c24xx_print_reg_status(ourport);
		return 0;
	}

	if (copied_bytes + bytes < LOG_BUFFER_SIZE) {
		ret = copy_to_user(userbuf, ourport->uart_local_buf.buffer+copied_bytes, bytes);
		if (ret) {
			pr_err("Failed to s3c24xx_serial_bt_log : %d\n", (int)ret);
			return ret;
		}
		copied_bytes += bytes;
		return bytes;
	} else {
		int byte_to_read = LOG_BUFFER_SIZE-copied_bytes;

		ret = copy_to_user(userbuf, ourport->uart_local_buf.buffer+copied_bytes, byte_to_read);
		if (ret) {
			pr_err("Failed to s3c24xx_serial_bt_log : %d\n", (int)ret);
			return ret;
		}
		copied_bytes += byte_to_read;
		return byte_to_read;
	}

	return 0;
}
static const struct file_operations proc_fops_btlog = {
	.owner = THIS_MODULE,
	.read = s3c24xx_serial_bt_log,
};
#endif


#if defined(SERIAL_UART_TRACE)
static ssize_t s3c24xx_serial_uart_log(struct file *file, char __user *userbuf, size_t bytes, loff_t *off)
{
	int ret;
	struct s3c24xx_uart_port *ourport = &s3c24xx_serial_ports[SERIAL_UART_PORT_LINE];
	static int copied_bytes;

	if (copied_bytes >= LOG_BUFFER_SIZE) {
		struct uart_port *port;

		port = &ourport->port;

		copied_bytes = 0;

		if (port && port->state->pm_state == UART_PM_STATE_ON)
			s3c24xx_print_reg_status(ourport);
		return 0;
	}

	if (copied_bytes + bytes < LOG_BUFFER_SIZE) {
		ret = copy_to_user(userbuf, ourport->uart_local_buf.buffer+copied_bytes, bytes);
		if (ret) {
			pr_err("Failed to s3c24xx_serial_serial_log : %d\n", (int)ret);
			return ret;
		}
		copied_bytes += bytes;
		return bytes;
	} else {
		int byte_to_read = LOG_BUFFER_SIZE-copied_bytes;

		ret = copy_to_user(userbuf, ourport->uart_local_buf.buffer+copied_bytes, byte_to_read);
		if (ret) {
			pr_err("Failed to s3c24xx_serial_log : %d\n", (int)ret);
			return ret;
		}
		copied_bytes += byte_to_read;
		return byte_to_read;
	}

	return 0;
}
static const struct file_operations proc_fops_serial_log = {
	.owner = THIS_MODULE,
	.read = s3c24xx_serial_uart_log,
};
#endif
#ifdef CONFIG_CPU_IDLE
static int s3c24xx_serial_notifier(struct notifier_block *self,
				unsigned long cmd, void *v)
{
	struct s3c24xx_uart_port *ourport;
	struct uart_port *port;
	unsigned long flags;
	unsigned int umcon;

	switch (cmd) {
	case LPA_ENTER:
		s3c24xx_serial_fifo_wait();
		break;

	case SICD_ENTER:
	case SICD_AUD_ENTER:
		list_for_each_entry(ourport, &drvdata_list, node) {
			if (ourport->port.line == CONFIG_S3C_LOWLEVEL_UART_PORT)
				continue;

			port = &ourport->port;

			if (port->state->pm_state == UART_PM_STATE_OFF)
				continue;

			spin_lock_irqsave(&port->lock, flags);

			/* disable auto flow control & set nRTS for High */
			umcon = rd_regl(port, S3C2410_UMCON);
			umcon &= ~(S3C2410_UMCOM_AFC | S3C2410_UMCOM_RTS_LOW);
			wr_regl(port, S3C2410_UMCON, umcon);

			spin_unlock_irqrestore(&port->lock, flags);

			if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
				sprintf(uart_log_buf,"[5] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
						rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
						rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
				uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
			}

			if (ourport->rts_control)
				change_uart_gpio(RTS_PINCTRL, ourport);
		}

		s3c24xx_serial_fifo_wait();
		break;

	case SICD_EXIT:
	case SICD_AUD_EXIT:
		list_for_each_entry(ourport, &drvdata_list, node) {
			if (ourport->port.line == CONFIG_S3C_LOWLEVEL_UART_PORT)
				continue;

			port = &ourport->port;

			if (port->state->pm_state == UART_PM_STATE_OFF)
				continue;

			spin_lock_irqsave(&port->lock, flags);

			/* enable auto flow control */
			umcon = rd_regl(port, S3C2410_UMCON);
			umcon |= S3C2410_UMCOM_AFC;
			wr_regl(port, S3C2410_UMCON, umcon);

			spin_unlock_irqrestore(&port->lock, flags);

			if (ourport->port.line == BLUETOOTH_UART_PORT_LINE) {
				sprintf(uart_log_buf,"[6] UMCON:0x%08x UFSTAT:0x%08x UINTP:0x%08x UCON:0x%08x UMSTAT:0x%08x\n",
						rd_regl(port, S3C2410_UMCON), rd_regl(port, S3C2410_UFSTAT), rd_regl(port, S3C2410_UINTP),
						rd_regl(port, S3C2410_UCON), rd_regl(port, S3C2410_UMSTAT));
				uart_copy_to_local_buf(2, &ourport->uart_local_buf, uart_log_buf, sizeof(uart_log_buf));
			}

			if (ourport->rts_control)
				change_uart_gpio(DEFAULT_PINCTRL, ourport);
		}
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block s3c24xx_serial_notifier_block = {
	.notifier_call = s3c24xx_serial_notifier,
};
#endif

static int s3c24xx_serial_probe(struct platform_device *pdev)
{
	struct s3c24xx_uart_port *ourport;
	int index = probe_index;
	int ret, fifo_size;
	int port_index = probe_index;
	int rts_trig_level;

	dbg("s3c24xx_serial_probe(%p) %d\n", pdev, index);

	if (pdev->dev.of_node) {
		ret = of_alias_get_id(pdev->dev.of_node, "uart");
		if (ret < 0) {
			dev_err(&pdev->dev, "UART aliases are not defined(%d).\n",
				ret);
		} else {
			port_index = ret;
		}
	}
	ourport = &s3c24xx_serial_ports[port_index];

	if (ourport->port.line != port_index)
		ourport = exynos_serial_default_port(port_index);

	if (ourport->port.line >= CONFIG_SERIAL_SAMSUNG_UARTS) {
		dev_err(&pdev->dev,
			"the port %d exceeded CONFIG_SERIAL_SAMSUNG_UARTS(%d)\n"
			, ourport->port.line, CONFIG_SERIAL_SAMSUNG_UARTS);
		return -EINVAL;
	}
	ourport->drv_data = s3c24xx_get_driver_data(pdev);
	if (!ourport->drv_data) {
		dev_err(&pdev->dev, "could not find driver data\n");
		return -ENODEV;
	}

	ourport->baudclk = ERR_PTR(-EINVAL);
	ourport->info = ourport->drv_data->info;
	ourport->cfg = (dev_get_platdata(&pdev->dev)) ?
			dev_get_platdata(&pdev->dev) :
			ourport->drv_data->def_cfg;

	ourport->port.fifosize = (ourport->info->fifosize) ?
		ourport->info->fifosize :
		ourport->drv_data->fifosize[port_index];

	if (of_get_property(pdev->dev.of_node, "samsung,uart-panic-log", NULL))
		ourport->uart_panic_log = 1;
	else
		ourport->uart_panic_log = 0;

	if (ourport->uart_panic_log) {
		atomic_notifier_chain_register(&panic_notifier_list, &exynos_s3c24xx_panic_block);
		panic_port = ourport;
	}

	if (of_get_property(pdev->dev.of_node, "samsung,usi-serial-v2", NULL))
		ourport->usi_v2 = 1;
	else
		ourport->usi_v2 = 0;

	if (of_get_property(pdev->dev.of_node, "samsung,rts-gpio-control", NULL)) {
		ourport->rts_control = 1;
		ourport->default_uart_pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR(ourport->default_uart_pinctrl))
			dev_err(&pdev->dev, "Can't get uart pinctrl!!!\n");
		else {
			ourport->uart_pinctrl_rts = pinctrl_lookup_state(ourport->default_uart_pinctrl,
						"rts");
			if (IS_ERR(ourport->uart_pinctrl_rts))
				dev_err(&pdev->dev, "Can't get RTS pinstate!!!\n");
			ourport->uart_pinctrl_tx_dat = pinctrl_lookup_state(ourport->default_uart_pinctrl,
						"tx_dat");

			ourport->uart_pinctrl_default = pinctrl_lookup_state(ourport->default_uart_pinctrl,
						"default");
			if (IS_ERR(ourport->uart_pinctrl_default))
				dev_err(&pdev->dev, "Can't get Default pinstate!!!\n");
		}
	}
	if (!of_property_read_u32(pdev->dev.of_node, "samsung,rts-trig-level",
				 &rts_trig_level)) {
		ourport->rts_trig_level = rts_trig_level;
	}

	if (!of_property_read_u32(pdev->dev.of_node, "samsung,fifo-size",
				&fifo_size)) {
		ourport->port.fifosize = fifo_size;
		ourport->info->fifosize = fifo_size;
	} else {
		dev_err(&pdev->dev,
				"Please add FIFO size in device tree!(UART%d)\n", port_index);
		return -EINVAL;
	}

	probe_index++;

	dbg("%s: initialising port %p...\n", __func__, ourport);

#ifdef CONFIG_ARM_EXYNOS_DEVFREQ
	if (of_property_read_u32(pdev->dev.of_node, "mif_qos_val",
						&ourport->mif_qos_val))
		ourport->mif_qos_val = 0;

	if (of_property_read_u32(pdev->dev.of_node, "cpu_qos_val",
						&ourport->cpu_qos_val))
		ourport->cpu_qos_val = 0;

	if (of_property_read_u32(pdev->dev.of_node, "irq_affinity",
						&ourport->uart_irq_affinity))
		ourport->uart_irq_affinity = 0;

	if (of_property_read_u64(pdev->dev.of_node, "qos_timeout",
					(u64 *)&ourport->qos_timeout))
		ourport->qos_timeout = 0;

	if ((ourport->mif_qos_val || ourport->cpu_qos_val)
					&& ourport->qos_timeout) {
		INIT_DELAYED_WORK(&ourport->qos_work,
						s3c64xx_serial_qos_func);
		/* request pm qos */
		if (ourport->mif_qos_val)
			pm_qos_add_request(&ourport->s3c24xx_uart_mif_qos,
						PM_QOS_BUS_THROUGHPUT, 0);

		if (ourport->cpu_qos_val)
			pm_qos_add_request(&ourport->s3c24xx_uart_cpu_qos,
						PM_QOS_CLUSTER1_FREQ_MIN, 0);
	}
#endif
	if (of_get_property(pdev->dev.of_node, "samsung,in-band-wakeup", NULL))
		ourport->in_band_wakeup = 1;
	else
		ourport->in_band_wakeup = 0;

	if (of_get_property(pdev->dev.of_node, "samsung,uart-logging", NULL))
		ourport->uart_logging = 1;
	else
		ourport->uart_logging = 0;

	if (of_find_property(pdev->dev.of_node, "samsung,use-default-irq", NULL))
		ourport->use_default_irq =1;
	else
		ourport->use_default_irq =0;

	ret = s3c24xx_serial_init_port(ourport, pdev);
	if (ret < 0)
		return ret;

	
	if (ourport->uart_logging == 1) {
		/* Allocate memory for UART logging */
		ourport->uart_local_buf.buffer = kzalloc(LOG_BUFFER_SIZE, GFP_KERNEL);

		if (!ourport->uart_local_buf.buffer)
			dev_err(&pdev->dev, "could not allocate buffer for UART logging\n");

		ourport->uart_local_buf.size = LOG_BUFFER_SIZE;
		ourport->uart_local_buf.index = 0;

#ifdef BT_UART_TRACE
		if (port_index == BLUETOOTH_UART_PORT_LINE) {
			struct proc_dir_entry *ent;

			bluetooth_dir = proc_mkdir("bluetooth", NULL);
			if (bluetooth_dir == NULL) {
				pr_err("Unable to create /proc/bluetooth directory\n");
				return -ENOMEM;
			}

			bt_log_dir = proc_mkdir("uart", bluetooth_dir);
			if (bt_log_dir == NULL) {
				pr_err("Unable to create /proc/%s directory\n", PROC_DIR);
				return -ENOMEM;
			}

			ent = proc_create("log", 0440, bt_log_dir, &proc_fops_btlog);
			if (ent == NULL) {
				pr_err("Unable to create /proc/%s/log entry\n", PROC_DIR);
				return -ENOMEM;
			}
		}
#endif
#ifdef SERIAL_UART_TRACE
	    	if (port_index == SERIAL_UART_PORT_LINE) {
			struct proc_dir_entry *uent;

			serial_dir = proc_mkdir("serial", NULL);
			if (serial_dir == NULL) {
		    		pr_err("Unable to create /proc/serial directory\n");
		    		return -ENOMEM;
			}

			serial_log_dir = proc_mkdir("uart", serial_dir);
			if (serial_log_dir == NULL) {
		    		pr_err("Unable to create /proc/%s directory\n", PROC_SERIAL_DIR);
		    		return -ENOMEM;
			}

			uent = proc_create("log", 0444, serial_log_dir, &proc_fops_serial_log);
			if (uent == NULL) {
		    		pr_err("Unable to create /proc/%s/log entry\n", PROC_SERIAL_DIR);
		    		return -ENOMEM;
			}
	    	}
#endif
	}

	dbg("%s: adding port\n", __func__);
	uart_add_one_port(&s3c24xx_uart_drv, &ourport->port);
	platform_set_drvdata(pdev, &ourport->port);

	/*
	 * Deactivate the clock enabled in s3c24xx_serial_init_port here,
	 * so that a potential re-enablement through the pm-callback overlaps
	 * and keeps the clock enabled in this case.
	 */
	uart_clock_disable(ourport);

#ifdef CONFIG_SAMSUNG_CLOCK
	ret = device_create_file(&pdev->dev, &dev_attr_clock_source);
	if (ret < 0)
		dev_err(&pdev->dev, "failed to add clock source attr.\n");
#endif

	list_add_tail(&ourport->node, &drvdata_list);

	ret = device_create_file(&pdev->dev, &dev_attr_uart_dbg);
	if (ret < 0)
		dev_err(&pdev->dev, "failed to create sysfs file.\n");

	ret = device_create_file(&pdev->dev, &dev_attr_error_cnt);
	if (ret < 0)
		dev_err(&pdev->dev, "failed to create sysfs file.\n");

	ourport->dbg_mode = 0;

	return 0;
}

static int s3c24xx_serial_remove(struct platform_device *dev)
{
	struct uart_port *port = s3c24xx_dev_to_port(&dev->dev);

#ifdef CONFIG_PM_DEVFREQ
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (ourport->mif_qos_val && ourport->qos_timeout)
		pm_qos_remove_request(&ourport->s3c24xx_uart_mif_qos);

	if (ourport->cpu_qos_val && ourport->qos_timeout)
		pm_qos_remove_request(&ourport->s3c24xx_uart_cpu_qos);
#endif

	if (port) {
		device_remove_file(&dev->dev, &dev_attr_error_cnt);

#ifdef CONFIG_SAMSUNG_CLOCK
		device_remove_file(&dev->dev, &dev_attr_clock_source);
#endif

	if (ourport->uart_logging == 1) {
		kfree(ourport->uart_local_buf.buffer);
	}
		uart_remove_one_port(&s3c24xx_uart_drv, port);
	}

	uart_unregister_driver(&s3c24xx_uart_drv);

	return 0;
}

/* UART power management code */
#ifdef CONFIG_PM_SLEEP
static int s3c24xx_serial_suspend(struct device *dev)
{
	struct uart_port *port = s3c24xx_dev_to_port(dev);
	struct s3c24xx_uart_port *ourport = to_ourport(port);
#ifdef CONFIG_SERIAL_SAMSUNG_HWACG
	unsigned int ucon;
#endif

	if (port) {
		/*
		 * If rts line must be protected while suspending
		 * we change the gpio pad as output high
		*/
		if (ourport->rts_control)
			change_uart_gpio(RTS_PINCTRL, ourport);

		udelay(300);//delay for sfr update
		s3c24xx_serial_rx_fifo_wait();

		uart_suspend_port(&s3c24xx_uart_drv, port);
#ifdef CONFIG_SERIAL_SAMSUNG_HWACG
		uart_clock_enable(ourport);
		/* disable Tx, Rx mode bit for suspend in case of HWACG */
		ucon = rd_regl(port, S3C2410_UCON);
		ucon &= ~(S3C2410_UCON_RXIRQMODE | S3C2410_UCON_TXIRQMODE);
		wr_regl(port, S3C2410_UCON, ucon);
		exynos_usi_stop(port);
		uart_clock_disable(ourport);

		rx_enabled(port) = 0;
		tx_enabled(port) = 0;
#endif
		if (ourport->dbg_mode & UART_DBG_MODE)
			dev_err(dev, "UART suspend notification for tty framework.\n");
	}

	return 0;
}

static int s3c24xx_serial_resume(struct device *dev)
{
	struct uart_port *port = s3c24xx_dev_to_port(dev);
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (port) {

		uart_resume_port(&s3c24xx_uart_drv, port);

		if (ourport->rts_control)
			change_uart_gpio(DEFAULT_PINCTRL, ourport);

		if (ourport->dbg_mode & UART_DBG_MODE)
			dev_err(dev, "UART resume notification for tty framework.\n");
	}

	return 0;
}

static int s3c24xx_serial_resume_noirq(struct device *dev)
{
	struct uart_port *port = s3c24xx_dev_to_port(dev);
	struct s3c24xx_uart_port *ourport = to_ourport(port);

	if (port) {
		/* restore IRQ mask */
		if (s3c24xx_serial_has_interrupt_mask(port)) {
			unsigned int uintm = 0xf;

			if (tx_enabled(port))
				uintm &= ~S3C64XX_UINTM_TXD_MSK;
			if (rx_enabled(port))
				uintm &= ~S3C64XX_UINTM_RXD_MSK;
			uart_clock_enable(ourport);
			wr_regl(port, S3C64XX_UINTM, uintm);
			uart_clock_disable(ourport);
		}
	}

	return 0;
}

static const struct dev_pm_ops s3c24xx_serial_pm_ops = {
	.suspend = s3c24xx_serial_suspend,
	.resume = s3c24xx_serial_resume,
	.resume_noirq = s3c24xx_serial_resume_noirq,
};
#define SERIAL_SAMSUNG_PM_OPS	(&s3c24xx_serial_pm_ops)

#else /* !CONFIG_PM_SLEEP */

#define SERIAL_SAMSUNG_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

/* Console code */

#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE

static struct uart_port *cons_uart;

static int
s3c24xx_serial_console_txrdy(struct uart_port *port, unsigned int ufcon)
{
	struct s3c24xx_uart_info *info = s3c24xx_port_to_info(port);
	unsigned long ufstat, utrstat;

	if (ufcon & S3C2410_UFCON_FIFOMODE) {
		/* fifo mode - check amount of data in fifo registers... */

		ufstat = rd_regl(port, S3C2410_UFSTAT);
		return (ufstat & info->tx_fifofull) ? 0 : 1;
	}

	/* in non-fifo mode, we go and use the tx buffer empty */

	utrstat = rd_regl(port, S3C2410_UTRSTAT);
	return (utrstat & S3C2410_UTRSTAT_TXE) ? 1 : 0;
}

static bool
s3c24xx_port_configured(unsigned int ucon)
{
	/* consider the serial port configured if the tx/rx mode set */
	return (ucon & 0xf) != 0;
}

#ifdef CONFIG_CONSOLE_POLL
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int s3c24xx_serial_get_poll_char(struct uart_port *port)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	unsigned int ufstat;

	ufstat = rd_regl(port, S3C2410_UFSTAT);
	if (s3c24xx_serial_rx_fifocnt(ourport, ufstat) == 0)
		return NO_POLL_CHAR;

	return rd_regb(port, S3C2410_URXH);
}

static void s3c24xx_serial_put_poll_char(struct uart_port *port,
		unsigned char c)
{
	unsigned int ufcon = rd_regl(port, S3C2410_UFCON);
	unsigned int ucon = rd_regl(port, S3C2410_UCON);

	/* not possible to xmit on unconfigured port */
	if (!s3c24xx_port_configured(ucon))
		return;

	while (!s3c24xx_serial_console_txrdy(port, ufcon))
		cpu_relax();
	wr_regb(port, S3C2410_UTXH, c);
}

#endif /* CONFIG_CONSOLE_POLL */

static void
s3c24xx_serial_console_putchar(struct uart_port *port, int ch)
{
	unsigned int ufcon = rd_regl(port, S3C2410_UFCON);

	while (!s3c24xx_serial_console_txrdy(port, ufcon))
		cpu_relax();
	wr_regb(port, S3C2410_UTXH, ch);
}

static void
s3c24xx_serial_console_write(struct console *co, const char *s,
			     unsigned int count)
{
	unsigned int ucon = rd_regl(cons_uart, S3C2410_UCON);

	/* not possible to xmit on unconfigured port */
	if (!s3c24xx_port_configured(ucon))
		return;

	uart_console_write(cons_uart, s, count, s3c24xx_serial_console_putchar);
}

static void __init
s3c24xx_serial_get_options(struct uart_port *port, int *baud,
			   int *parity, int *bits)
{
	struct s3c24xx_uart_port *ourport = to_ourport(port);
	unsigned int ulcon;
	unsigned int ucon;
	unsigned int ubrdiv;
	unsigned long rate;
	int ret;

	ulcon  = rd_regl(port, S3C2410_ULCON);
	ucon   = rd_regl(port, S3C2410_UCON);
	ubrdiv = rd_regl(port, S3C2410_UBRDIV);

	dbg("s3c24xx_serial_get_options: port=%p\n"
	    "registers: ulcon=%08x, ucon=%08x, ubdriv=%08x\n",
	    port, ulcon, ucon, ubrdiv);

	if (s3c24xx_port_configured(ucon)) {
		switch (ulcon & S3C2410_LCON_CSMASK) {
		case S3C2410_LCON_CS5:
			*bits = 5;
			break;
		case S3C2410_LCON_CS6:
			*bits = 6;
			break;
		case S3C2410_LCON_CS7:
			*bits = 7;
			break;
		case S3C2410_LCON_CS8:
		default:
			*bits = 8;
			break;
		}

		switch (ulcon & S3C2410_LCON_PMASK) {
		case S3C2410_LCON_PEVEN:
			*parity = 'e';
			break;

		case S3C2410_LCON_PODD:
			*parity = 'o';
			break;

		case S3C2410_LCON_PNONE:
		default:
			*parity = 'n';
		}

		/* now calculate the baud rate */
		rate = clk_get_rate(ourport->clk);

		if (ourport->src_clk_rate && rate != ourport->src_clk_rate)
		{
			ret = clk_set_rate(ourport->clk, ourport->src_clk_rate);
			if (ret < 0)
				dev_err(&ourport->pdev->dev, "UART clk set failed\n");

			rate = clk_get_rate(ourport->clk);
		}

		*baud = rate / (16 * (ubrdiv + 1));
		dbg("calculated baud %d\n", *baud);
	}

}

static int __init
s3c24xx_serial_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	dbg("s3c24xx_serial_console_setup: co=%p (%d), %s\n",
	    co, co->index, options);

	/* is this a valid port */

	if (co->index == -1 || co->index >= CONFIG_SERIAL_SAMSUNG_UARTS)
		co->index = 0;

	port = &s3c24xx_serial_ports[co->index].port;

	/* is the port configured? */

	if (port->mapbase == 0x0)
		return -ENODEV;

	cons_uart = port;

	dbg("s3c24xx_serial_console_setup: port=%p (%d)\n", port, co->index);

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		s3c24xx_serial_get_options(port, &baud, &parity, &bits);

	dbg("s3c24xx_serial_console_setup: baud %d\n", baud);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct console s3c24xx_serial_console = {
	.name		= S3C24XX_SERIAL_NAME,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.write		= s3c24xx_serial_console_write,
	.setup		= s3c24xx_serial_console_setup,
	.data		= &s3c24xx_uart_drv,
};
#endif /* CONFIG_SERIAL_SAMSUNG_CONSOLE */

#ifdef CONFIG_CPU_S3C2410
static struct s3c24xx_serial_drv_data s3c2410_serial_drv_data = {
	.info = &(struct s3c24xx_uart_info) {
		.name		= "Samsung S3C2410 UART",
		.type		= PORT_S3C2410,
		.fifosize	= 16,
		.rx_fifomask	= S3C2410_UFSTAT_RXMASK,
		.rx_fifoshift	= S3C2410_UFSTAT_RXSHIFT,
		.rx_fifofull	= S3C2410_UFSTAT_RXFULL,
		.tx_fifofull	= S3C2410_UFSTAT_TXFULL,
		.tx_fifomask	= S3C2410_UFSTAT_TXMASK,
		.tx_fifoshift	= S3C2410_UFSTAT_TXSHIFT,
		.def_clk_sel	= S3C2410_UCON_CLKSEL0,
		.num_clks	= 2,
		.clksel_mask	= S3C2410_UCON_CLKMASK,
		.clksel_shift	= S3C2410_UCON_CLKSHIFT,
	},
	.def_cfg = &(struct s3c2410_uartcfg) {
		.ucon		= S3C2410_UCON_DEFAULT,
		.ufcon		= S3C2410_UFCON_DEFAULT,
	},
};
#define S3C2410_SERIAL_DRV_DATA ((kernel_ulong_t)&s3c2410_serial_drv_data)
#else
#define S3C2410_SERIAL_DRV_DATA (kernel_ulong_t)NULL
#endif

#ifdef CONFIG_CPU_S3C2412
static struct s3c24xx_serial_drv_data s3c2412_serial_drv_data = {
	.info = &(struct s3c24xx_uart_info) {
		.name		= "Samsung S3C2412 UART",
		.type		= PORT_S3C2412,
		.fifosize	= 64,
		.has_divslot	= 1,
		.rx_fifomask	= S3C2440_UFSTAT_RXMASK,
		.rx_fifoshift	= S3C2440_UFSTAT_RXSHIFT,
		.rx_fifofull	= S3C2440_UFSTAT_RXFULL,
		.tx_fifofull	= S3C2440_UFSTAT_TXFULL,
		.tx_fifomask	= S3C2440_UFSTAT_TXMASK,
		.tx_fifoshift	= S3C2440_UFSTAT_TXSHIFT,
		.def_clk_sel	= S3C2410_UCON_CLKSEL2,
		.num_clks	= 4,
		.clksel_mask	= S3C2412_UCON_CLKMASK,
		.clksel_shift	= S3C2412_UCON_CLKSHIFT,
	},
	.def_cfg = &(struct s3c2410_uartcfg) {
		.ucon		= S3C2410_UCON_DEFAULT,
		.ufcon		= S3C2410_UFCON_DEFAULT,
	},
};
#define S3C2412_SERIAL_DRV_DATA ((kernel_ulong_t)&s3c2412_serial_drv_data)
#else
#define S3C2412_SERIAL_DRV_DATA (kernel_ulong_t)NULL
#endif

#if defined(CONFIG_CPU_S3C2440) || defined(CONFIG_CPU_S3C2416) || \
	defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2442)
static struct s3c24xx_serial_drv_data s3c2440_serial_drv_data = {
	.info = &(struct s3c24xx_uart_info) {
		.name		= "Samsung S3C2440 UART",
		.type		= PORT_S3C2440,
		.fifosize	= 64,
		.has_divslot	= 1,
		.rx_fifomask	= S3C2440_UFSTAT_RXMASK,
		.rx_fifoshift	= S3C2440_UFSTAT_RXSHIFT,
		.rx_fifofull	= S3C2440_UFSTAT_RXFULL,
		.tx_fifofull	= S3C2440_UFSTAT_TXFULL,
		.tx_fifomask	= S3C2440_UFSTAT_TXMASK,
		.tx_fifoshift	= S3C2440_UFSTAT_TXSHIFT,
		.def_clk_sel	= S3C2410_UCON_CLKSEL2,
		.num_clks	= 4,
		.clksel_mask	= S3C2412_UCON_CLKMASK,
		.clksel_shift	= S3C2412_UCON_CLKSHIFT,
	},
	.def_cfg = &(struct s3c2410_uartcfg) {
		.ucon		= S3C2410_UCON_DEFAULT,
		.ufcon		= S3C2410_UFCON_DEFAULT,
	},
};
#define S3C2440_SERIAL_DRV_DATA ((kernel_ulong_t)&s3c2440_serial_drv_data)
#else
#define S3C2440_SERIAL_DRV_DATA (kernel_ulong_t)NULL
#endif

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
static struct s3c24xx_serial_drv_data s3c6400_serial_drv_data = {
	.info = &(struct s3c24xx_uart_info) {
		.name		= "Samsung S3C6400 UART",
		.type		= PORT_S3C6400,
		.fifosize	= 64,
		.has_divslot	= 1,
		.rx_fifomask	= S3C2440_UFSTAT_RXMASK,
		.rx_fifoshift	= S3C2440_UFSTAT_RXSHIFT,
		.rx_fifofull	= S3C2440_UFSTAT_RXFULL,
		.tx_fifofull	= S3C2440_UFSTAT_TXFULL,
		.tx_fifomask	= S3C2440_UFSTAT_TXMASK,
		.tx_fifoshift	= S3C2440_UFSTAT_TXSHIFT,
		.def_clk_sel	= S3C2410_UCON_CLKSEL2,
		.num_clks	= 4,
		.clksel_mask	= S3C6400_UCON_CLKMASK,
		.clksel_shift	= S3C6400_UCON_CLKSHIFT,
	},
	.def_cfg = &(struct s3c2410_uartcfg) {
		.ucon		= S3C2410_UCON_DEFAULT,
		.ufcon		= S3C2410_UFCON_DEFAULT,
	},
};
#define S3C6400_SERIAL_DRV_DATA ((kernel_ulong_t)&s3c6400_serial_drv_data)
#else
#define S3C6400_SERIAL_DRV_DATA (kernel_ulong_t)NULL
#endif

#ifdef CONFIG_CPU_S5PV210
static struct s3c24xx_serial_drv_data s5pv210_serial_drv_data = {
	.info = &(struct s3c24xx_uart_info) {
		.name		= "Samsung S5PV210 UART",
		.type		= PORT_S3C6400,
		.has_divslot	= 1,
		.rx_fifomask	= S5PV210_UFSTAT_RXMASK,
		.rx_fifoshift	= S5PV210_UFSTAT_RXSHIFT,
		.rx_fifofull	= S5PV210_UFSTAT_RXFULL,
		.tx_fifofull	= S5PV210_UFSTAT_TXFULL,
		.tx_fifomask	= S5PV210_UFSTAT_TXMASK,
		.tx_fifoshift	= S5PV210_UFSTAT_TXSHIFT,
		.def_clk_sel	= S3C2410_UCON_CLKSEL0,
		.num_clks	= 2,
		.clksel_mask	= S5PV210_UCON_CLKMASK,
		.clksel_shift	= S5PV210_UCON_CLKSHIFT,
	},
	.def_cfg = &(struct s3c2410_uartcfg) {
		.ucon		= S5PV210_UCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	.fifosize = { 256, 64, 16, 16 },
};
#define S5PV210_SERIAL_DRV_DATA ((kernel_ulong_t)&s5pv210_serial_drv_data)
#else
#define S5PV210_SERIAL_DRV_DATA	(kernel_ulong_t)NULL
#endif

#if defined(CONFIG_ARCH_EXYNOS)
#define EXYNOS_COMMON_SERIAL_DRV_DATA				\
	.info = &(struct s3c24xx_uart_info) {			\
		.name		= "Samsung Exynos UART",	\
		.type		= PORT_S3C6400,			\
		.has_divslot	= 1,				\
		.rx_fifomask	= S5PV210_UFSTAT_RXMASK,	\
		.rx_fifoshift	= S5PV210_UFSTAT_RXSHIFT,	\
		.rx_fifofull	= S5PV210_UFSTAT_RXFULL,	\
		.tx_fifofull	= S5PV210_UFSTAT_TXFULL,	\
		.tx_fifomask	= S5PV210_UFSTAT_TXMASK,	\
		.tx_fifoshift	= S5PV210_UFSTAT_TXSHIFT,	\
		.rts_trig_shift = S5PV210_UMCON_RTSTRIG_SHIFT,	\
		.def_clk_sel	= S3C2410_UCON_CLKSEL0,		\
		.num_clks	= 1,				\
		.clksel_mask	= 0,				\
		.clksel_shift	= 0,				\
	},							\
	.def_cfg = &(struct s3c2410_uartcfg) {			\
		.ucon		= S5PV210_UCON_DEFAULT,		\
		.ufcon		= S5PV210_UFCON_DEFAULT,	\
		.has_fracval	= 1,				\
	}							\

static struct s3c24xx_serial_drv_data exynos4210_serial_drv_data = {
	EXYNOS_COMMON_SERIAL_DRV_DATA,
	.fifosize = { 256, 64, 16, 16 },
};

static struct s3c24xx_serial_drv_data exynos_serial_drv_data = {
	EXYNOS_COMMON_SERIAL_DRV_DATA,
	.fifosize = { 0, },
};

#define EXYNOS4210_SERIAL_DRV_DATA ((kernel_ulong_t)&exynos4210_serial_drv_data)
#define EXYNOS_SERIAL_DRV_DATA ((kernel_ulong_t)&exynos_serial_drv_data)
#else
#define EXYNOS4210_SERIAL_DRV_DATA (kernel_ulong_t)NULL
#define EXYNOS_SERIAL_DRV_DATA (kernel_ulong_t)NULL
#endif

static const struct platform_device_id s3c24xx_serial_driver_ids[] = {
	{
		.name		= "s3c2410-uart",
		.driver_data	= S3C2410_SERIAL_DRV_DATA,
	}, {
		.name		= "s3c2412-uart",
		.driver_data	= S3C2412_SERIAL_DRV_DATA,
	}, {
		.name		= "s3c2440-uart",
		.driver_data	= S3C2440_SERIAL_DRV_DATA,
	}, {
		.name		= "s3c6400-uart",
		.driver_data	= S3C6400_SERIAL_DRV_DATA,
	}, {
		.name		= "s5pv210-uart",
		.driver_data	= S5PV210_SERIAL_DRV_DATA,
	}, {
		.name		= "exynos4210-uart",
		.driver_data	= EXYNOS4210_SERIAL_DRV_DATA,
	}, {
		.name		= "exynos-uart",
		.driver_data	= EXYNOS_SERIAL_DRV_DATA,
	},
	{ },
};
MODULE_DEVICE_TABLE(platform, s3c24xx_serial_driver_ids);

#ifdef CONFIG_OF
static const struct of_device_id s3c24xx_uart_dt_match[] = {
	{ .compatible = "samsung,s3c2410-uart",
		.data = (void *)S3C2410_SERIAL_DRV_DATA },
	{ .compatible = "samsung,s3c2412-uart",
		.data = (void *)S3C2412_SERIAL_DRV_DATA },
	{ .compatible = "samsung,s3c2440-uart",
		.data = (void *)S3C2440_SERIAL_DRV_DATA },
	{ .compatible = "samsung,s3c6400-uart",
		.data = (void *)S3C6400_SERIAL_DRV_DATA },
	{ .compatible = "samsung,s5pv210-uart",
		.data = (void *)S5PV210_SERIAL_DRV_DATA },
	{ .compatible = "samsung,exynos4210-uart",
		.data = (void *)EXYNOS4210_SERIAL_DRV_DATA },
	{ .compatible = "samsung,exynos-uart",
		.data = (void *)EXYNOS_SERIAL_DRV_DATA },
	{},
};
MODULE_DEVICE_TABLE(of, s3c24xx_uart_dt_match);
#endif

static struct platform_driver samsung_serial_driver = {
	.probe		= s3c24xx_serial_probe,
	.remove		= s3c24xx_serial_remove,
	.id_table	= s3c24xx_serial_driver_ids,
	.driver		= {
		.name	= "samsung-uart",
		.pm	= SERIAL_SAMSUNG_PM_OPS,
		.of_match_table	= of_match_ptr(s3c24xx_uart_dt_match),
	},
};

/* module initialisation code */

static int __init s3c24xx_serial_modinit(void)
{
	int ret;

	ret = uart_register_driver(&s3c24xx_uart_drv);
	if (ret < 0) {
		pr_err("Failed to register Samsung UART driver\n");
		return ret;
	}

#ifdef CONFIG_CPU_IDLE
	exynos_pm_register_notifier(&s3c24xx_serial_notifier_block);
#endif

	return platform_driver_register(&samsung_serial_driver);
}

static void __exit s3c24xx_serial_modexit(void)
{
	platform_driver_unregister(&samsung_serial_driver);
	uart_unregister_driver(&s3c24xx_uart_drv);
}

module_init(s3c24xx_serial_modinit);
module_exit(s3c24xx_serial_modexit);

#ifdef CONFIG_SERIAL_SAMSUNG_CONSOLE
/*
 * Early console.
 */

struct samsung_early_console_data {
	u32 txfull_mask;
};

static void samsung_early_busyuart(struct uart_port *port)
{
	while (!(readl(port->membase + S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXFE))
		;
}

static void samsung_early_busyuart_fifo(struct uart_port *port)
{
	struct samsung_early_console_data *data = port->private_data;

	while (readl(port->membase + S3C2410_UFSTAT) & data->txfull_mask)
		;
}

static void samsung_early_putc(struct uart_port *port, int c)
{
	if (readl(port->membase + S3C2410_UFCON) & S3C2410_UFCON_FIFOMODE)
		samsung_early_busyuart_fifo(port);
	else
		samsung_early_busyuart(port);

	writeb(c, port->membase + S3C2410_UTXH);
}

static void samsung_early_write(struct console *con, const char *s, unsigned n)
{
	struct earlycon_device *dev = con->data;

	uart_console_write(&dev->port, s, n, samsung_early_putc);
}

static int __init samsung_early_console_setup(struct earlycon_device *device,
					      const char *opt)
{
	if (!device->port.membase)
		return -ENODEV;

	device->con->write = samsung_early_write;
	return 0;
}

/* S3C2410 */
static struct samsung_early_console_data s3c2410_early_console_data = {
	.txfull_mask = S3C2410_UFSTAT_TXFULL,
};

static int __init s3c2410_early_console_setup(struct earlycon_device *device,
					      const char *opt)
{
	device->port.private_data = &s3c2410_early_console_data;
	return samsung_early_console_setup(device, opt);
}
OF_EARLYCON_DECLARE(s3c2410, "samsung,s3c2410-uart",
			s3c2410_early_console_setup);

/* S3C2412, S3C2440, S3C64xx */
static struct samsung_early_console_data s3c2440_early_console_data = {
	.txfull_mask = S3C2440_UFSTAT_TXFULL,
};

static int __init s3c2440_early_console_setup(struct earlycon_device *device,
					      const char *opt)
{
	device->port.private_data = &s3c2440_early_console_data;
	return samsung_early_console_setup(device, opt);
}
OF_EARLYCON_DECLARE(s3c2412, "samsung,s3c2412-uart",
			s3c2440_early_console_setup);
OF_EARLYCON_DECLARE(s3c2440, "samsung,s3c2440-uart",
			s3c2440_early_console_setup);
OF_EARLYCON_DECLARE(s3c6400, "samsung,s3c6400-uart",
			s3c2440_early_console_setup);

/* S5PV210, EXYNOS */
static struct samsung_early_console_data s5pv210_early_console_data = {
	.txfull_mask = S5PV210_UFSTAT_TXFULL,
};

static int __init s5pv210_early_console_setup(struct earlycon_device *device,
					      const char *opt)
{
	device->port.private_data = &s5pv210_early_console_data;
	return samsung_early_console_setup(device, opt);
}
OF_EARLYCON_DECLARE(s5pv210, "samsung,s5pv210-uart",
			s5pv210_early_console_setup);
OF_EARLYCON_DECLARE(exynos4210, "samsung,exynos4210-uart",
			s5pv210_early_console_setup);
#endif

MODULE_ALIAS("platform:samsung-uart");
MODULE_DESCRIPTION("Samsung SoC Serial port driver");
MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>");
MODULE_LICENSE("GPL v2");
