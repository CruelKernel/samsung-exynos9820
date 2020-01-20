/*
 * common.h - common header for framework and plugins
 *
 * author : Yongjin Lee (yongjin0.lee@samsung.com)
 *	    Jeonghoon Jang (jnghn.jang@samsung.com)
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef CONFIG_EXYNOS_ACPM
typedef unsigned int		u32;
typedef unsigned short		u16;
typedef unsigned char		u8;
typedef signed int		s32;
typedef signed short		s16;
typedef signed char		s8;
typedef float			f32;
typedef unsigned char		bool;
#endif

/**
 * struct ipc_cmd_info - RX command buffer info for framework and plugins
 *
 * @rx_cmd:		pointer to the RX buffer entry to be dequeued.
 * @rx_cmd_indr:	pointer to the RX indirection buffer entry to be dequeued.
 */
struct ipc_cmd_info {
	u32 rx_cmd;
	u32 rx_cmd_indr;
};

/**
 * struct dbg_log_info - log buffer entry format
 */
struct dbg_log_info {
	char str[8];
	u32 val;
	u32 log_level;
};

/**
 * struct build_info
 */
struct build_info {
	char build_version[25];
	char build_time[25];
};

/**
 * struct acpm_ops - framework callbacks to be provided to plugins
 *
 * @set_timer_event:	adds timer request which expires at every 'msec'.
 * @del_timer_event:	removes the existing timer request.
 * @insert_dbg_log:	inserts a log entry at predefined log buffer region.
 * @get_tx_dest:	does full check of tx queue and returns ptr to enqueue.
 * @enqueue_tx:		updated tx_front ptr of tx queue and generates mbox irq.
 * @get_total_size:	returns size_of(framework + all plugins)
 */
struct acpm_ops {
	s32 (*set_timer_event) (u32 plugin_id, u32 msec);
	void (*del_timer_event) (u32 plugin_id);
	void (*insert_dbg_log) (u32 plugin_id, struct dbg_log_info *log);
	u32 (*get_tx_dest) (u32 ch_num, u32 *index);
	void (*enqueue_tx) (u32 ch_num, u32 index);
	u32 (*get_total_size) (void);
	s32 (*secure_func) (void *plugin, u32 arg0, u32 arg1,
                        u32 arg2, u32 arg3);
	s32 (*external_plugin_func) (void *plugin, u32 pid,
			u32 *arg0, u32 *arg1, u32 *arg2);
	s32 (*speedy_init)(void);
#ifdef CONFIG_MULTI_PMIC
	s32 (*speedy_read)(u8 channel, u32 addr);
	s32 (*speedy_write)(u8 channel, u32 addr, u32 data);
#else
	s32 (*speedy_read)(u32 addr);
	s32 (*speedy_write)(u32 addr, u32 data);
#endif
	void (*udelay)(u32 udelay);
	void (*intr_enable)(u32 pid, u32 intr);
	void (*intr_disable)(u32 pid, u32 intr);
};

/**
 * struct plugin_ops - plugin callbacks to be provided to framework
 *
 * @ipc_handler:	handler to be executed when ipc for this plugin is arrived.
 * @irq_handler:	handler to be executed when hw irq for this plugin is arrived.
 * @timer_event_handler:handler to be executed when requested timer is expired.
 */
struct plugin_ops {
	s32 (*ipc_handler) (struct ipc_cmd_info *cmd, u32 ch_num);
	s32 (*irq_handler) (u32 intr);
	s32 (*timer_event_handler) (void);
	s32 (*extern_func) (u32 *arg0, u32 *arg1, u32 *arg2);
	struct build_info info;
};

/**
 * struct timer_desc - A descriptor for timer request
 *
 * @period:		requested period that framework executes timer_event_handler.
 * @multiplier:
 */
struct timer_desc {
	u32 period;
	u32 multiplier;
};

/**
 * struct plugin - The basic plugin structure
 *
 * @id:			Predefined id for this plugin.
 * @base_addr:		Predefined base addr for this plugin. (entrypoint)
 * @acpm_ops:		Framework callbacks.
 * @plugin_ops:		Plugin callbacks.
 * @timer:		Timer descriptor for this plugin.
 * @is_attached:	For dynamic plugin support.
 * @size:		The size of this plugin.
 */
struct plugin {
	u32 id;
#ifndef CONFIG_EXYNOS_ACPM
	void *base_addr;
	struct acpm_ops *acpm_ops;
	struct plugin_ops *plugin_ops;
#else
	u32 base_addr;
	u32 acpm_ops;
	u32 plugin_ops;
#endif
	u32 secure_func_mask;
        u32 extern_func_mask;
	struct timer_desc timer;
	u8 is_attached;
	u32 size;
	u8 stay_attached;
#ifndef CONFIG_EXYNOS_ACPM
	const char *fw_name;
#else
	u32 fw_name;
#endif
};

typedef void (*pfn_plugin_init) (struct plugin *p);

enum ret_type {
	RET_OK,
	RET_FAIL,
};

#ifndef CONFIG_EXYNOS_ACPM
#define NULL	0
#define FALSE ((bool) 0)
#define TRUE  ((bool) 1)

#define __raw_writel(data, base)		(*(volatile int*)(base) = (data))
#define __raw_readl(base)			(*(volatile int*)(base))

#define ARRAY_SIZE(a)			(sizeof(a) / sizeof((a)[0]))

#define EBUSY			100
#define EINVAL			101
#endif

#endif
