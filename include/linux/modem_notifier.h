/* Copyright (C) 2015 Samsung Electronics.
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

#ifndef __MODEM_NOTIFIER_H__
#define __MODEM_NOTIFIER_H__

/* refer to enum modem_state  */
enum modem_event {
	MODEM_EVENT_RESET	= 1,
	MODEM_EVENT_EXIT,
	MODEM_EVENT_ONLINE	= 4,
	MODEM_EVENT_WATCHDOG	= 9,
};

#if IS_ENABLED(CONFIG_SHM_IPC)
extern int register_modem_event_notifier(struct notifier_block *nb);
#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
extern void modem_notify_event(enum modem_event evt, void *mc);
#else
extern void modem_notify_event(enum modem_event evt);
#endif
#else
static inline int register_modem_event_notifier(struct notifier_block *nb)
{
	return 0;
}
static inline void modem_notify_event(enum modem_event evt) {}
#endif

#endif/*__MODEM_NOTIFIER_H__*/
