/*
 * Bluetooth Broadcom GPIO and Low Power Mode control
 *
 *  Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *  Copyright (C) 2011 Google, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/serial_core.h>
#include <linux/wakelock.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/serial_s3c.h>
#include <soc/samsung/exynos-cpupm.h>

//#include <../../arch/arm/include/asm/mach-types.h>

//#include <../../arch/arm/mach-exynos/include/mach/gpio.h>
//#include <plat/gpio-cfg.h>

#define BT_LPM_ENABLE

#define BT_UPORT 1

#define STATUS_IDLE	1
#define STATUS_BUSY	0

extern s3c_wake_peer_t s3c2410_serial_wake_peer[CONFIG_SERIAL_SAMSUNG_UARTS];

static struct rfkill *bt_rfkill;
#ifdef BT_LPM_ENABLE
static int bt_wake_state = -1;
#endif

struct bcm_bt_lpm {
	int host_wake;
	int dev_wake;

	struct hrtimer enter_lpm_timer;
	ktime_t enter_lpm_delay;

	struct uart_port *uport;

	struct wake_lock host_wake_lock;
	struct wake_lock bt_wake_lock;
} bt_lpm;

struct bcm_bt_gpio {
	int bt_en;
	int bt_wake;
	int bt_hostwake;
	int irq;
} bt_gpio;

int idle_ip_index;

static int bcm43xx_bt_rfkill_set_power(void *data, bool blocked)
{
	/* rfkill_ops callback. Turn transmitter on when blocked is false */
	if (!blocked) {
		pr_info("[BT] Bluetooth Power On.\n");

#ifdef BT_LPM_ENABLE
		if ( irq_set_irq_wake(bt_gpio.irq, 1)) {
			pr_err("[BT] Set_irq_wake failed.\n");
			return -1;
		}
#endif

#ifndef BT_LPM_ENABLE
		gpio_set_value(bt_gpio.bt_wake, 1);
#endif
		gpio_set_value(bt_gpio.bt_en, 1);
		exynos_update_ip_idle_status(idle_ip_index, STATUS_BUSY);

		msleep(100);

	} else {
		pr_info("[BT] Bluetooth Power Off.\n");

#ifdef BT_LPM_ENABLE
		if (gpio_get_value(bt_gpio.bt_en) && irq_set_irq_wake(bt_gpio.irq, 0)) {
			pr_err("[BT] Release_irq_wake failed.\n");
			return -1;
		}
#endif

		exynos_update_ip_idle_status(idle_ip_index, STATUS_IDLE);
		gpio_set_value(bt_gpio.bt_en, 0);
	}
	return 0;
}

static const struct rfkill_ops bcm43xx_bt_rfkill_ops = {
	.set_block = bcm43xx_bt_rfkill_set_power,
};

#ifdef BT_LPM_ENABLE
static void set_wake_locked(int wake)
{
#ifdef CONFIG_BT_UART_IN_AUDIO
	struct uart_port *port = bt_lpm.uport;
#endif

	if (wake)
		wake_lock(&bt_lpm.bt_wake_lock);

	gpio_set_value(bt_gpio.bt_wake, wake);
	bt_lpm.dev_wake = wake;

	if (bt_wake_state != wake)
	{
#ifdef CONFIG_BT_UART_IN_AUDIO
		if(bt_lpm.host_wake)
		{
			if(wake)
				port->ops->set_wake(port, wake);
		}
		else
		{
			port->ops->set_wake(port, wake);
		}
#endif
		pr_err("[BT] set_wake_locked value = %d\n", wake);
		bt_wake_state = wake;
	}
}

static enum hrtimer_restart enter_lpm(struct hrtimer *timer)
{
	if (bt_lpm.uport != NULL)
		set_wake_locked(0);

    if (bt_lpm.host_wake == 0)
	    exynos_update_ip_idle_status(idle_ip_index, STATUS_IDLE);

	wake_lock_timeout(&bt_lpm.bt_wake_lock, HZ/2);

	return HRTIMER_NORESTART;
}

void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport)
{
	bt_lpm.uport = uport;

	hrtimer_try_to_cancel(&bt_lpm.enter_lpm_timer);

	exynos_update_ip_idle_status(idle_ip_index, STATUS_BUSY);
	set_wake_locked(1);

//	pr_info("[BT] bcm_bt_lpm_exit_lpm_locked\n");
	hrtimer_start(&bt_lpm.enter_lpm_timer, bt_lpm.enter_lpm_delay,
		HRTIMER_MODE_REL);
}

static void update_host_wake_locked(int host_wake)
{
	if (host_wake == bt_lpm.host_wake)
		return;

	bt_lpm.host_wake = host_wake;

	if (host_wake) {
        exynos_update_ip_idle_status(idle_ip_index, STATUS_BUSY);
		wake_lock(&bt_lpm.host_wake_lock);
	} else  {
		/* Take a timed wakelock, so that upper layers can take it.
		 * The chipset deasserts the hostwake lock, when there is no
		 * more data to send.
		 */
		pr_info("[BT] update_host_wake_locked host_wake is deasserted. release wakelock in 1s\n");
		wake_lock_timeout(&bt_lpm.host_wake_lock, HZ);

        if (bt_lpm.dev_wake == 0)
            exynos_update_ip_idle_status(idle_ip_index, STATUS_IDLE);
	}
}

static irqreturn_t host_wake_isr(int irq, void *dev)
{
#ifdef CONFIG_BT_UART_IN_AUDIO
	struct uart_port *port = bt_lpm.uport;
#endif
	int host_wake;

	host_wake = gpio_get_value(bt_gpio.bt_hostwake);
	irq_set_irq_type(irq, host_wake ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING);

	if (!bt_lpm.uport) {
		bt_lpm.host_wake = host_wake;
		pr_err("[BT] host_wake_isr uport is null\n");
		return IRQ_HANDLED;
	}

#ifdef CONFIG_BT_UART_IN_AUDIO
	if(bt_lpm.dev_wake)
	{
		if(host_wake)
			port->ops->set_wake(port, host_wake);
	}
	else
	{
		port->ops->set_wake(port, host_wake);
	}
#endif
	update_host_wake_locked(host_wake);

	return IRQ_HANDLED;
}

static int bcm_bt_lpm_init(struct platform_device *pdev)
{
	int ret;

	hrtimer_init(&bt_lpm.enter_lpm_timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	bt_lpm.enter_lpm_delay = ktime_set(1, 0);  /* 1 sec */ /*1->3*//*3->4*/
	bt_lpm.enter_lpm_timer.function = enter_lpm;

	bt_lpm.host_wake = 0;

	wake_lock_init(&bt_lpm.host_wake_lock, WAKE_LOCK_SUSPEND,
			 "BT_host_wake");
	wake_lock_init(&bt_lpm.bt_wake_lock, WAKE_LOCK_SUSPEND,
			 "BT_bt_wake");

	s3c2410_serial_wake_peer[BT_UPORT] = (s3c_wake_peer_t) bcm_bt_lpm_exit_lpm_locked;

	bt_gpio.irq = gpio_to_irq(bt_gpio.bt_hostwake);
	ret = request_irq(bt_gpio.irq, host_wake_isr, IRQF_TRIGGER_RISING,
		"bt_host_wake", NULL);
	if (ret) {
		pr_err("[BT] Request_host wake irq failed.\n");
		return ret;
	}

	return 0;
}
#endif

static int bcm43xx_bluetooth_probe(struct platform_device *pdev)
{
	int rc = 0;
#ifdef BT_LPM_ENABLE
	int ret;
#endif
	pr_info("[BT] bcm43xx_bluetooth_probe.\n");

	bt_gpio.bt_en = of_get_gpio(pdev->dev.of_node, 0);

	if (!gpio_is_valid(bt_gpio.bt_en)) {
		pr_err("[BT] bt_gpio.bt_en get gpio failed.\n");
		return -EINVAL;
	}

	rc = gpio_request(bt_gpio.bt_en, "bten_gpio");

	if (unlikely(rc)) {
		pr_err("[BT] bt_gpio.bt_en request failed.\n");
		return rc;
	}

	bt_gpio.bt_wake =of_get_gpio(pdev->dev.of_node, 1);

	if (!gpio_is_valid(bt_gpio.bt_wake)) {
		pr_err("[BT] bt_gpio.bt_wake get gpio failed.\n");
		return -EINVAL;
	}

	rc = gpio_request(bt_gpio.bt_wake, "btwake_gpio");

	if (unlikely(rc)) {
		pr_err("[BT] bt_gpio.bt_wake request failed.\n");
		gpio_free(bt_gpio.bt_en);
		return rc;
	}

	bt_gpio.bt_hostwake =of_get_gpio(pdev->dev.of_node, 2);

	if (!gpio_is_valid(bt_gpio.bt_hostwake)) {
		pr_err("[BT] bt_gpio.bt_hostwake get gpio failed.\n");
		return -EINVAL;
	}

	rc = gpio_request(bt_gpio.bt_hostwake,"bthostwake_gpio");

	if (unlikely(rc)) {
		pr_err("[BT] bt_gpio.bt_hostwake request failed.\n");
		gpio_free(bt_gpio.bt_wake);
		gpio_free(bt_gpio.bt_en);
		return rc;
	}

	gpio_direction_input(bt_gpio.bt_hostwake);
	gpio_direction_output(bt_gpio.bt_wake, 0);
	gpio_direction_output(bt_gpio.bt_en, 0);

	bt_rfkill = rfkill_alloc("bcm43xx Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &bcm43xx_bt_rfkill_ops,
				NULL);

	if (unlikely(!bt_rfkill)) {
		pr_err("[BT] bt_rfkill alloc failed.\n");
		gpio_free(bt_gpio.bt_hostwake);
		gpio_free(bt_gpio.bt_wake);
		gpio_free(bt_gpio.bt_en);
		return -ENOMEM;
	}

	rfkill_init_sw_state(bt_rfkill, 0);

	rc = rfkill_register(bt_rfkill);

	if (unlikely(rc)) {
		pr_err("[BT] bt_rfkill register failed.\n");
		rfkill_destroy(bt_rfkill);
		gpio_free(bt_gpio.bt_hostwake);
		gpio_free(bt_gpio.bt_wake);
		gpio_free(bt_gpio.bt_en);
		return -1;
	}

	rfkill_set_sw_state(bt_rfkill, true);

#ifdef BT_LPM_ENABLE
	ret = bcm_bt_lpm_init(pdev);
	if (ret) {
		rfkill_unregister(bt_rfkill);
		rfkill_destroy(bt_rfkill);

		gpio_free(bt_gpio.bt_hostwake);
		gpio_free(bt_gpio.bt_wake);
		gpio_free(bt_gpio.bt_en);
	}
#endif
	idle_ip_index = exynos_get_idle_ip_index("bluetooth");
    exynos_update_ip_idle_status(idle_ip_index, STATUS_IDLE);

	pr_info("[BT] bcm43xx_bluetooth_probe End \n");

	return rc;
}

static int bcm43xx_bluetooth_remove(struct platform_device *pdev)
{
	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);

	gpio_free(bt_gpio.bt_en);
	gpio_free(bt_gpio.bt_wake);
	gpio_free(bt_gpio.bt_hostwake);

	wake_lock_destroy(&bt_lpm.host_wake_lock);
	wake_lock_destroy(&bt_lpm.bt_wake_lock);

	return 0;
}

//#if defined (CONFIG_OF)
static const struct of_device_id exynos_bluetooth_match[] = {
	{
		.compatible = "samsung,bcm43xx",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_bluetooth_match);

static struct platform_driver bcm43xx_bluetooth_platform_driver = {
	.probe = bcm43xx_bluetooth_probe,
	.remove = bcm43xx_bluetooth_remove,
	.driver = {
		   .name = "bcm43xx_bluetooth",
		   .owner = THIS_MODULE,
		   .of_match_table = exynos_bluetooth_match,
		   },
};

module_platform_driver(bcm43xx_bluetooth_platform_driver);
//#endif
MODULE_ALIAS("platform:bcm43xx");
MODULE_DESCRIPTION("bcm43xx_bluetooth");
MODULE_LICENSE("GPL");
