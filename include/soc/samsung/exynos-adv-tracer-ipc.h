#ifndef __EXYNOS_ADV_TRACER_IPC_H_
#define __EXYNOS_ADV_TRACER_IPC_H_

struct adv_tracer_ipc_cmd_raw {
	u32 cmd			:16;
	u32 response		:1;
	u32 overlay		:1;
	u32 ret			:1;
	u32 ok			:1;
	u32 busy		:1;
	u32 manual_polling	:1;
	u32 one_way		:1;
	u32 reserved		:1;
	u32 id			:4;
	u32 size		:4;
};

struct adv_tracer_ipc_cmd {
	union {
		struct adv_tracer_ipc_cmd_raw cmd_raw;
		unsigned int buffer[20];
	};
	unsigned int len;
};

struct adv_tracer_ipc_ch {
	struct list_head list;
	unsigned int id;
	unsigned int offset;
	unsigned int len;
	char id_name[4];

	struct adv_tracer_ipc_cmd *cmd;
	void __iomem *buff_regs;
	void (*ipc_callback)(struct adv_tracer_ipc_cmd *cmd, unsigned int len);

	spinlock_t ch_lock;
	struct mutex wait_lock;

	struct completion wait;
	bool polling;
	bool used;
};

struct adv_tracer_ipc_main {
	unsigned int num_channels;
	struct device *dev;
	struct adv_tracer_ipc_ch *channel;
	unsigned int irq;
	void __iomem *mailbox_base;
	void __iomem *shared_buffer;
	bool w_mode;
	unsigned int mailbox_status;
	bool recovery;
};

struct adv_tracer_plugin {
	struct device_node *np;
	unsigned int id;
	unsigned int len;
	unsigned int enable;
};

typedef void (*ipc_callback)(struct adv_tracer_ipc_cmd *cmd, unsigned int size);

enum ipc_frmk_cmd {
	EAT_IPC_CMD_CH_INIT = 1,
	EAT_IPC_CMD_CH_RELEASE,
	EAT_IPC_CMD_CH_CLEAR,
	EAT_IPC_CMD_BOOT_DBGC,
	EAT_IPC_CMD_EXCEPTION_DBGC,
	EAT_IPC_CMD_FRM_LOAD_BINARY = 0x10ad,
	EAT_IPC_CMD_ARRAYDUMP = 0x8080,
};

enum ipc_frmk_cmd_id {
	ARR_IPC_CMD_ID_KERNEL_ARRAYDUMP = 0x1,
};

#define EAT_MAX_CHANNEL				(8)
#define EAT_FRM_CHANNEL				(0)
#define EAT_IPC_TIMEOUT				(100 * NSEC_PER_MSEC)

#define INTGR0					0x0008
#define INTCR0					0x000C
#define INTMR0					0x0010
#define INTSR0					0x0014
#define INTMSR0					0x0018
#define INTGR1					0x001C
#define INTCR1					0x0020
#define INTMR1					0x0024
#define INTSR1					0x0028
#define INTMSR1					0x002C
#define INTGR_DBGC_TO_AP			INTGR0
#define AP_INTCR				INTCR0
#define AP_INTMR				INTMR0
#define AP_INTSR				INTSR0
#define AP_INTMSR				INTMSR0
#define INTGR_AP_TO_DBGC			INTGR1
#define DBGC_INTCR				INTCR1
#define DBGC_INTMR				INTMR1
#define DBGC_INTSR				INTSR1
#define DBGC_INTMSR				INTMSR1
#define SR(n)					(0x0080 + (n << 2))
#define INTR_FLAG_OFFSET                        16
#define FRAMEWORK_NAME				"FRM"

extern int adv_tracer_ipc_init(struct platform_device *pdev);

#ifdef CONFIG_EXYNOS_ADV_TRACER
int adv_tracer_arraydump(void);
int adv_tracer_ipc_request_channel(struct device_node *np,
		ipc_callback handler, unsigned int *id, unsigned int *len);
int adv_tracer_ipc_release_channel(unsigned int id);
int adv_tracer_ipc_send_data(unsigned int id, struct adv_tracer_ipc_cmd *cmd);
int adv_tracer_ipc_send_data_polling(unsigned int id, struct adv_tracer_ipc_cmd *cmd);
int adv_tracer_ipc_send_data_polling_timeout(unsigned int id, struct adv_tracer_ipc_cmd *cmd,
					unsigned long timeout_ns);
int adv_tracer_ipc_send_data_async(unsigned int id, struct adv_tracer_ipc_cmd *cmd);
void exynos_adv_tracer_reboot(void);
void adv_tracer_ipc_release_channel_by_name(const char *name);
#else
static inline int adv_tracer_ipc_request_channel(struct device_node *np,
		ipc_callback handler, unsigned int *id, unsigned int *len)
{
	return 0;
}
static inline int adv_tracer_ipc_release_channel(unsigned int channel_id)
{
	return 0;
}
static inline int adv_tracer_ipc_send_data(unsigned int id, struct adv_tracer_ipc_cmd *cmd)
{
	return 0;
}
int adv_tracer_ipc_send_data_polling(unsigned int id, struct adv_tracer_ipc_cmd *cmd)
{
	return 0;
}
int adv_tracer_ipc_send_data_polling_timeout(unsigned int id, struct adv_tracer_ipc_cmd *cmd,
					unsigned long timeout_ns)
{
	return 0;
}
int adv_tracer_ipc_send_data_async(unsigned int id, struct adv_tracer_ipc_cmd *cmd)
{
	return 0;
}
static inline void exynos_adv_tracer_reboot(void)
{
	return;
}
inline void adv_tracer_ipc_release_channel_by_name(const char *name)
{
}
#endif
#endif
