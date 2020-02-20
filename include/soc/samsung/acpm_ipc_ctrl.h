#ifndef __ACPM_IPC_CTRL_H__
#define __ACPM_IPC_CTRL_H__

typedef void (*ipc_callback)(unsigned int *cmd, unsigned int size);

struct ipc_config {
	unsigned int *cmd;
	unsigned int *indirection_base;
	unsigned int indirection_size;
	bool response;
	bool indirection;
};

#define ACPM_IPC_PROTOCOL_OWN			(31)
#define ACPM_IPC_PROTOCOL_RSP			(30)
#define ACPM_IPC_PROTOCOL_INDIRECTION		(29)
#define ACPM_IPC_PROTOCOL_ID			(26)
#define ACPM_IPC_PROTOCOL_IDX			(0x7 << ACPM_IPC_PROTOCOL_ID)
#define ACPM_IPC_PROTOCOL_DP_A			(25)
#define ACPM_IPC_PROTOCOL_DP_D			(24)
#define ACPM_IPC_PROTOCOL_DP_CMD		(0x3 << ACPM_IPC_PROTOCOL_DP_D)
#define ACPM_IPC_PROTOCOL_TEST			(23)
#define ACPM_IPC_PROTOCOL_STOP			(22)
#define ACPM_IPC_PROTOCOL_SEQ_NUM		(16)

#ifdef CONFIG_EXYNOS_ACPM
unsigned int acpm_ipc_request_channel(struct device_node *np, ipc_callback handler,
		unsigned int *id, unsigned int *size);
unsigned int acpm_ipc_release_channel(struct device_node *np, unsigned int channel_id);
int acpm_ipc_send_data(unsigned int channel_id, struct ipc_config *cfg);
int acpm_ipc_send_data_sync(unsigned int channel_id, struct ipc_config *cfg);
int acpm_ipc_send_data_lazy(unsigned int channel_id, struct ipc_config *cfg);
int acpm_ipc_set_ch_mode(struct device_node *np, bool polling);
void exynos_acpm_reboot(void);
void acpm_stop_log(void);
#else

static inline unsigned int acpm_ipc_request_channel(struct device_node *np, ipc_callback handler,
		unsigned int *id, unsigned int *size)
{
	return 0;
}

static inline unsigned int acpm_ipc_release_channel(struct device_node *np, unsigned int channel_id)
{
	return 0;
}

static inline int acpm_ipc_send_data(unsigned int channel_id, struct ipc_config *cfg)
{
	return 0;
}

static inline int acpm_ipc_send_data_sync(unsigned int channel_id, struct ipc_config *cfg)
{
	return 0;
}

static inline int acpm_ipc_set_ch_mode(struct device_node *np, bool polling)
{
	return 0;
}

static inline void exynos_acpm_reboot(void)
{
	return;
}

static inline void acpm_stop_log(void)
{
	return;
}
#endif

#ifdef CONFIG_EXYNOS_ACPM_S2D
void exynos_acpm_set_s2d_enable(int en);
void exynos_acpm_select_s2d_blk(u32 sel);
int exynos_acpm_s2d_update_en(void);
#endif

#endif
