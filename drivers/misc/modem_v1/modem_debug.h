#ifndef __MODEM_DEBUG_H__
#define __MODEM_DEBUG_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/spinlock.h>

enum modemctl_event {
	MDM_EVENT_CP_FORCE_RESET,
	MDM_EVENT_CP_FORCE_CRASH,
	MDM_EVENT_CP_ABNORMAL_RX,
	MDM_CRASH_PM_FAIL,
	MDM_CRASH_PM_CP_FAIL,
	MDM_CRASH_INVALID_RB,
	MDM_CRASH_INVALID_IOD,
	MDM_CRASH_INVALID_SKBCB,
	MDM_CRASH_INVALID_SKBIOD,
	MDM_CRASH_NO_MEM,
	MDM_CRASH_CMD_RESET = 90,
	MDM_CRASH_CMD_EXIT,
};

int register_cp_crash_notifier(struct notifier_block *nb);
void modemctl_notify_event(enum modemctl_event evt);

#endif
