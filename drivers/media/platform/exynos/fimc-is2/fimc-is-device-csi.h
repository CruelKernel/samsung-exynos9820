#ifndef FIMC_IS_DEVICE_CSI_H
#define FIMC_IS_DEVICE_CSI_H

#include <linux/interrupt.h>
#include <media/v4l2-device.h>
#include "fimc-is-hw.h"
#include "fimc-is-type.h"
#include "fimc-is-framemgr.h"
#include "fimc-is-subdev-ctrl.h"
#include "fimc-is-work.h"

#ifndef ENABLE_IS_CORE
#define CSI_NOTIFY_VSYNC	10
#define CSI_NOTIFY_VBLANK	11
#endif

#define CSI_LINE_RATIO		14	/* 70% */
#define CSI_ERR_COUNT		10  	/* 10frame */

#define CSI_VALID_ENTRY_TO_CH(id) ((id) >= ENTRY_SSVC0 && (id) <= ENTRY_SSVC3)
#define CSI_ENTRY_TO_CH(id) ({BUG_ON(!CSI_VALID_ENTRY_TO_CH(id));id - ENTRY_SSVC0;}) /* range : vc0(0) ~ vc3(3) */
#define CSI_CH_TO_ENTRY(id) (id + ENTRY_SSVC0) /* range : ENTRY_SSVC0 ~ ENTRY_SSVC3 */

/* for use multi buffering */
#define CSI_GET_PREV_FRAMEPTR(frameptr, num_frames, offset) \
	((frameptr) == 0 ? (num_frames) - offset : (frameptr) - offset)
#define CSI_GET_NEXT_FRAMEPTR(frameptr, num_frames) \
	(((frameptr) + 1) % num_frames)

extern int debug_csi;
extern struct fimc_is_sysfs_debug sysfs_debug;

enum fimc_is_csi_state {
	/* flite join ischain */
	CSIS_JOIN_ISCHAIN,
	/* one the fly output */
	CSIS_OTF_WITH_3AA,
	/* If it's dummy, H/W setting can't be applied */
	CSIS_DUMMY,
	/* WDMA flag */
	CSIS_DMA_ENABLE,
	CSIS_START_STREAM,
	/* runtime buffer done state for error */
	CSIS_BUF_ERR_VC0,
	CSIS_BUF_ERR_VC1,
	CSIS_BUF_ERR_VC2,
	CSIS_BUF_ERR_VC3,
	/* csi vc multibuffer setting state */
	CSIS_SET_MULTIBUF_VC1,
	CSIS_SET_MULTIBUF_VC2,
	CSIS_SET_MULTIBUF_VC3,
};

/* MIPI-CSI interface */
enum itf_vc_buf_data_type {
	VC_BUF_DATA_TYPE_INVALID = -1,
	VC_BUF_DATA_TYPE_SENSOR_STAT1 = 0,
	VC_BUF_DATA_TYPE_GENERAL_STAT1,
	VC_BUF_DATA_TYPE_SENSOR_STAT2,
	VC_BUF_DATA_TYPE_GENERAL_STAT2,
	VC_BUF_DATA_TYPE_MAX
};

struct fimc_is_device_csi {
	/* channel information */
	u32				instance;
	enum subdev_ch_mode		scm;
	u32 __iomem			*base_reg;
	u32 __iomem			*vc_reg[SCM_MAX][CSI_VIRTUAL_CH_MAX];
	u32 __iomem			*cmn_reg[SCM_MAX][CSI_VIRTUAL_CH_MAX];
	u32 __iomem			*phy_reg;
	resource_size_t			regs_start;
	resource_size_t			regs_end;
	int				irq;
	int				vc_irq[SCM_MAX][CSI_VIRTUAL_CH_MAX];
	unsigned long			vc_irq_state;

	/* csi common dma */
	struct fimc_is_device_csi_dma	*csi_dma;

	/* for settle time */
	struct fimc_is_sensor_cfg	*sensor_cfg;

	/* image configuration */
	struct fimc_is_image		image;

	unsigned long			state;

	/* for DMA feature */
	struct fimc_is_framemgr		*framemgr;
	u32				overflow_cnt;
	u32				sw_checker;
	atomic_t			fcount;
	struct tasklet_struct		tasklet_csis_str;
	struct tasklet_struct		tasklet_csis_end;
	struct tasklet_struct		tasklet_csis_line;
	struct workqueue_struct		*workqueue;
	struct work_struct		wq_csis_dma[CSI_VIRTUAL_CH_MAX];
	struct fimc_is_work_list	work_list[CSI_VIRTUAL_CH_MAX];
	int				pre_dma_enable[CSI_VIRTUAL_CH_MAX];

	/* subdev slots for dma */
	struct fimc_is_subdev		*dma_subdev[CSI_VIRTUAL_CH_MAX];

	/* pointer address from device sensor */
	struct v4l2_subdev		**subdev;
	struct phy			*phy;

	u32 error_id[CSI_VIRTUAL_CH_MAX];
	u32 error_count;

#ifndef ENABLE_IS_CORE
	atomic_t                        vblank_count; /* Increase at CSI frame end */

	atomic_t			vvalid; /* set 1 while vvalid period */
#endif
	char				name[FIMC_IS_STR_LEN];
};

struct fimc_is_device_csi_dma {
	u32 __iomem			*base_reg;
	u32 __iomem			*base_reg_stat;
	resource_size_t			regs_start;
	resource_size_t			regs_end;

	atomic_t			rcount; /* CSI open count check */

	spinlock_t			barrier;
};

void csi_frame_start_inline(struct fimc_is_device_csi *csi);
int __must_check fimc_is_csi_dma_probe(struct fimc_is_device_csi_dma *csi_dma, struct platform_device *pdev);

int __must_check fimc_is_csi_probe(void *parent, u32 instance);
int __must_check fimc_is_csi_open(struct v4l2_subdev *subdev, struct fimc_is_framemgr *framemgr);
int __must_check fimc_is_csi_close(struct v4l2_subdev *subdev);
/* for DMA feature */
extern u32 __iomem *notify_fcount_sen0;
extern u32 __iomem *notify_fcount_sen1;
extern u32 __iomem *notify_fcount_sen2;
extern u32 __iomem *last_fcount0;
extern u32 __iomem *last_fcount1;
#endif
