/*
 *
 * (C) COPYRIGHT 2020 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#include <mali_kbase.h>
#include "debug/mali_kbase_debug_ktrace_internal.h"

/* MALI_SEC_INTEGRATION */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0))
#include <linux/sched/clock.h>
#endif

int kbase_ktrace_init(struct kbase_device *kbdev)
{
/* MALI_SEC_INTEGRATION */
#ifndef CONFIG_MALI_EXYNOS_TRACE
#if KBASE_KTRACE_TARGET_RBUF
	struct kbase_ktrace_msg *rbuf;

	/* See also documentation of enum kbase_ktrace_code */
	compiletime_assert(sizeof(kbase_ktrace_code_t) == sizeof(unsigned long long) ||
			KBASE_KTRACE_CODE_COUNT <= (1ull << (sizeof(kbase_ktrace_code_t) * BITS_PER_BYTE)),
			"kbase_ktrace_code_t not wide enough for KBASE_KTRACE_CODE_COUNT");

	rbuf = kmalloc_array(KBASE_KTRACE_SIZE, sizeof(*rbuf), GFP_KERNEL);

	if (!rbuf)
		return -EINVAL;

	kbdev->ktrace.rbuf = rbuf;
	spin_lock_init(&kbdev->ktrace.lock);
#endif /* KBASE_KTRACE_TARGET_RBUF */
#endif
	return 0;
}

void kbase_ktrace_term(struct kbase_device *kbdev)
{
/* MALI_SEC_INTEGRATION */
#ifndef CONFIG_MALI_EXYNOS_TRACE
#if KBASE_KTRACE_TARGET_RBUF
	kfree(kbdev->ktrace.rbuf);
#endif /* KBASE_KTRACE_TARGET_RBUF */
#endif
}

void kbase_ktrace_hook_wrapper(void *param)
{
	struct kbase_device *kbdev = (struct kbase_device *)param;

	KBASE_KTRACE_DUMP(kbdev);
}

#if KBASE_KTRACE_TARGET_RBUF

static const char * const kbasep_ktrace_code_string[] = {
	/*
	 * IMPORTANT: USE OF SPECIAL #INCLUDE OF NON-STANDARD HEADER FILE
	 * THIS MUST BE USED AT THE START OF THE ARRAY
	 */
#define KBASE_KTRACE_CODE_MAKE_CODE(X) # X
#include "debug/mali_kbase_debug_ktrace_codes.h"
#undef  KBASE_KTRACE_CODE_MAKE_CODE
};

static void kbasep_ktrace_format_header(char *buffer, int sz, s32 written)
{
	written += MAX(snprintf(buffer + written, MAX(sz - written, 0),
			"secs,thread_id,cpu,code,kctx,"), 0);

	kbasep_ktrace_backend_format_header(buffer, sz, &written);

	written += MAX(snprintf(buffer + written, MAX(sz - written, 0),
			",info_val,ktrace_version=%u.%u",
			KBASE_KTRACE_VERSION_MAJOR,
			KBASE_KTRACE_VERSION_MINOR), 0);

	buffer[sz - 1] = 0;
}
/* MALI_SEC_INTEGRATION */
void kbasep_ktrace_format_msg(struct kbase_ktrace_msg *trace_msg,
		char *buffer, int sz)
{
	s32 written = 0;

	/* Initial part of message:
	 *
	 * secs,thread_id,cpu,code,kctx,
	 */
	written += MAX(snprintf(buffer + written, MAX(sz - written, 0),
			"%d.%.6d,%d,%d,%s,%p,",
			(int)trace_msg->timestamp.tv_sec,
			(int)(trace_msg->timestamp.tv_nsec / 1000),
			trace_msg->thread_id, trace_msg->cpu,
			kbasep_ktrace_code_string[trace_msg->backend.code],
			trace_msg->kctx), 0);

	/* Backend parts */
	kbasep_ktrace_backend_format_msg(trace_msg, buffer, sz,
			&written);

	/* Rest of message:
	 *
	 * ,info_val
	 *
	 * Note that the last column is empty, it's simply to hold the ktrace
	 * version in the header
	 */
	written += MAX(snprintf(buffer + written, MAX(sz - written, 0),
			",0x%.16llx",
			(unsigned long long)trace_msg->info_val), 0);
	buffer[sz - 1] = 0;
}

static void kbasep_ktrace_dump_msg(struct kbase_device *kbdev,
		struct kbase_ktrace_msg *trace_msg)
{
	char buffer[KTRACE_DUMP_MESSAGE_SIZE];

	lockdep_assert_held(&kbdev->ktrace.lock);

	kbasep_ktrace_format_msg(trace_msg, buffer, sizeof(buffer));
	dev_dbg(kbdev->dev, "%s", buffer);
}

/* MALI_SEC_INTEGRATION */
#ifdef CONFIG_MALI_EXYNOS_TRACE
bool check_trace_code(enum kbase_ktrace_code code)
{
#ifdef CONFIG_MALI_EXYNOS_TRACE_ALL
	return true;
#else
	unsigned int temp = code;

	switch (temp) {
	case KBASE_KTRACE_CODE(PM_PWROFF_L2):
	case KBASE_KTRACE_CODE(PM_PWRON_L2):
	case KBASE_KTRACE_CODE(CORE_CTX_DESTROY):
	case KBASE_KTRACE_CODE(CORE_GPU_SOFT_RESET):
	case KBASE_KTRACE_CODE(CORE_GPU_HARD_RESET):
	case KBASE_KTRACE_CODE(JM_SOFTSTOP):
	case KBASE_KTRACE_CODE(JM_HARDSTOP):
	case KBASE_KTRACE_CODE(LSI_KATOM_REMOVED):
	case KBASE_KTRACE_CODE(JM_SUBMIT):
	case KBASE_KTRACE_CODE(JM_JOB_DONE):
	case KBASE_KTRACE_CODE(LSI_GPU_RPM_RESUME_API):
	case KBASE_KTRACE_CODE(LSI_GPU_RPM_SUSPEND_API):
	case KBASE_KTRACE_CODE(LSI_SUSPEND_CALLBACK):
	case KBASE_KTRACE_CODE(KBASE_DEVICE_SUSPEND):
	case KBASE_KTRACE_CODE(KBASE_DEVICE_RESUME):
	case KBASE_KTRACE_CODE(KBASE_DEVICE_PM_WAIT_WQ_QUEUE_WORK):
	case KBASE_KTRACE_CODE(LSI_JM_IRQ_E):
	case KBASE_KTRACE_CODE(LSI_MMU_IRQ_E):
	case KBASE_KTRACE_CODE(LSI_GPU_IRQ_E):
	case KBASE_KTRACE_CODE(LSI_GPU_ON):
	case KBASE_KTRACE_CODE(LSI_GPU_OFF):
	case KBASE_KTRACE_CODE(LSI_RESUME_FREQ):
	case KBASE_KTRACE_CODE(LSI_CLOCK_VALUE):
	case KBASE_KTRACE_CODE(LSI_TMU_VALUE):
	case KBASE_KTRACE_CODE(LSI_GPU_MAX_LOCK):
	case KBASE_KTRACE_CODE(LSI_GPU_MIN_LOCK):
	case KBASE_KTRACE_CODE(LSI_RESET_GPU_EARLY_DUPE):
	case KBASE_KTRACE_CODE(LSI_RESET_RACE_DETECTED_EARLY_OUT):
	case KBASE_KTRACE_CODE(LSI_PM_SUSPEND):
		return true;
	default:
		return false;
	}
	return true;
#endif
}
#endif

struct kbase_ktrace_msg *kbasep_ktrace_reserve(struct kbase_ktrace *ktrace)
{
	struct kbase_ktrace_msg *trace_msg;

	lockdep_assert_held(&ktrace->lock);

	trace_msg = &ktrace->rbuf[ktrace->next_in];

	/* Update the ringbuffer indices */
	ktrace->next_in = (ktrace->next_in + 1) & KBASE_KTRACE_MASK;
	if (ktrace->next_in == ktrace->first_out)
		ktrace->first_out = (ktrace->first_out + 1) & KBASE_KTRACE_MASK;

	return trace_msg;
}
void kbasep_ktrace_msg_init(struct kbase_ktrace *ktrace,
		struct kbase_ktrace_msg *trace_msg, enum kbase_ktrace_code code,
		struct kbase_context *kctx, kbase_ktrace_flag_t flags,
		u64 info_val)
{
        /* MALI_SEC_INTEGRATION */
#ifdef CONFIG_MALI_EXYNOS_TRACE
        u64 time;
        unsigned long rem_nsec;
#endif

	lockdep_assert_held(&ktrace->lock);

	trace_msg->thread_id = task_pid_nr(current);

	trace_msg->cpu = task_cpu(current);

	/* MALI_SEC_INTEGRATION */
#ifdef CONFIG_MALI_EXYNOS_TRACE
	time = local_clock();
	rem_nsec = do_div(time, 1000000000);
	trace_msg->timestamp.tv_sec = time;
	trace_msg->timestamp.tv_nsec = rem_nsec;
#else
	ktime_get_real_ts64(&trace_msg->timestamp);
#endif

	trace_msg->kctx = kctx;

	trace_msg->info_val = info_val;
	trace_msg->backend.code = code;
	trace_msg->backend.flags = flags;
}

void kbasep_ktrace_add(struct kbase_device *kbdev, enum kbase_ktrace_code code,
		struct kbase_context *kctx, kbase_ktrace_flag_t flags,
		u64 info_val)
{
	unsigned long irqflags;
	struct kbase_ktrace_msg *trace_msg;

	/* MALI_SEC_INTEGRATION */
#ifdef CONFIG_MALI_EXYNOS_TRACE
        if (!check_trace_code(code))
                return;

	if (code == KBASE_KTRACE_CODE(JM_SOFTSTOP) || code == KBASE_KTRACE_CODE(JM_HARDSTOP))
                gpu_dump_register_hooks(kbdev);
#endif

	spin_lock_irqsave(&kbdev->ktrace.lock, irqflags);

	/* Reserve and update indices */
	trace_msg = kbasep_ktrace_reserve(&kbdev->ktrace);

	/* Fill the common part of the message (including backend.flags) */
	kbasep_ktrace_msg_init(&kbdev->ktrace, trace_msg, code, kctx, flags,
			info_val);

	/* Done */
	spin_unlock_irqrestore(&kbdev->ktrace.lock, irqflags);
}

static void kbasep_ktrace_clear_locked(struct kbase_device *kbdev)
{
	lockdep_assert_held(&kbdev->ktrace.lock);
	kbdev->ktrace.first_out = kbdev->ktrace.next_in;
}
void kbasep_ktrace_clear(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->ktrace.lock, flags);
	kbasep_ktrace_clear_locked(kbdev);
	spin_unlock_irqrestore(&kbdev->ktrace.lock, flags);
}

void kbasep_ktrace_dump(struct kbase_device *kbdev)
{
	unsigned long flags;
	u32 start;
	u32 end;
	char buffer[KTRACE_DUMP_MESSAGE_SIZE] = "Dumping trace:\n";

	kbasep_ktrace_format_header(buffer, sizeof(buffer), strlen(buffer));
	dev_dbg(kbdev->dev, "%s", buffer);

	spin_lock_irqsave(&kbdev->ktrace.lock, flags);
	start = kbdev->ktrace.first_out;
	end = kbdev->ktrace.next_in;

	while (start != end) {
		struct kbase_ktrace_msg *trace_msg = &kbdev->ktrace.rbuf[start];

		kbasep_ktrace_dump_msg(kbdev, trace_msg);

		start = (start + 1) & KBASE_KTRACE_MASK;
	}
	dev_dbg(kbdev->dev, "TRACE_END");

	kbasep_ktrace_clear_locked(kbdev);

	spin_unlock_irqrestore(&kbdev->ktrace.lock, flags);
}

#ifdef CONFIG_DEBUG_FS
struct trace_seq_state {
	struct kbase_ktrace_msg trace_buf[KBASE_KTRACE_SIZE];
	u32 start;
	u32 end;
};

static void *kbasep_ktrace_seq_start(struct seq_file *s, loff_t *pos)
{
	struct trace_seq_state *state = s->private;
	int i;

	if (*pos == 0)
		/* See Documentation/filesystems/seq_file.txt */
		return SEQ_START_TOKEN;

	if (*pos > KBASE_KTRACE_SIZE)
		return NULL;
	i = state->start + *pos;
	if ((state->end >= state->start && i >= state->end) ||
			i >= state->end + KBASE_KTRACE_SIZE)
		return NULL;

	i &= KBASE_KTRACE_MASK;

	return &state->trace_buf[i];
}

static void kbasep_ktrace_seq_stop(struct seq_file *s, void *data)
{
}

static void *kbasep_ktrace_seq_next(struct seq_file *s, void *data, loff_t *pos)
{
	struct trace_seq_state *state = s->private;
	int i;

	if (data != SEQ_START_TOKEN)
		(*pos)++;

	i = (state->start + *pos) & KBASE_KTRACE_MASK;
	if (i == state->end)
		return NULL;

	return &state->trace_buf[i];
}

static int kbasep_ktrace_seq_show(struct seq_file *s, void *data)
{
	struct kbase_ktrace_msg *trace_msg = data;
	char buffer[KTRACE_DUMP_MESSAGE_SIZE];

	/* If this is the start, print a header */
	if (data == SEQ_START_TOKEN)
		kbasep_ktrace_format_header(buffer, sizeof(buffer), 0);
	else
		kbasep_ktrace_format_msg(trace_msg, buffer, sizeof(buffer));

	seq_printf(s, "%s\n", buffer);
	return 0;
}

static const struct seq_operations kbasep_ktrace_seq_ops = {
	.start = kbasep_ktrace_seq_start,
	.next = kbasep_ktrace_seq_next,
	.stop = kbasep_ktrace_seq_stop,
	.show = kbasep_ktrace_seq_show,
};

static int kbasep_ktrace_debugfs_open(struct inode *inode, struct file *file)
{
	struct kbase_device *kbdev = inode->i_private;
	unsigned long flags;

	struct trace_seq_state *state;

	state = __seq_open_private(file, &kbasep_ktrace_seq_ops,
			sizeof(*state));
	if (!state)
		return -ENOMEM;

	spin_lock_irqsave(&kbdev->ktrace.lock, flags);
	state->start = kbdev->ktrace.first_out;
	state->end = kbdev->ktrace.next_in;
	memcpy(state->trace_buf, kbdev->ktrace.rbuf, sizeof(state->trace_buf));
	spin_unlock_irqrestore(&kbdev->ktrace.lock, flags);

	return 0;
}

/* MALI_SEC_INTEGRATIONS : REMOVE STATIC */
const struct file_operations kbasep_ktrace_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = kbasep_ktrace_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release_private,
};

void kbase_ktrace_debugfs_init(struct kbase_device *kbdev)
{
/* MALI_SEC_INTEGRATION */
#ifndef CONFIG_MALI_EXYNOS_TRACE
	debugfs_create_file("mali_trace", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&kbasep_ktrace_debugfs_fops);
#endif
}
#endif /* CONFIG_DEBUG_FS */

#else /* KBASE_KTRACE_TARGET_RBUF  */

#ifdef CONFIG_DEBUG_FS
void kbase_ktrace_debugfs_init(struct kbase_device *kbdev)
{
	CSTD_UNUSED(kbdev);
}
#endif /* CONFIG_DEBUG_FS */
#endif /* KBASE_KTRACE_TARGET_RBUF */
