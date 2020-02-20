#ifndef _NPU_CONFIG_H_
#define _NPU_CONFIG_H_

/* Timeout - ns unit (ms * 1000000) */
#define NPU_TIMEOUT_HARD_PROTODRV_COMPLETED	(5000 * 1000000L)	/* PROCESSING to COMPLETED */
#define NPU_TIMEOUT_HARD_PROTODRV_PROCESSING	(1000 * 1000000L)	/* REQUESTED to PROCESSING */

/*
 * =================================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =================================================================================================
 */


#define NPU_MAX_BUFFER			16
#define NPU_MAX_PLANE			3

#define NPU_MAX_GRAPH			32
#define NPU_MAX_FRAME			NPU_MAX_BUFFER

/* Device identification */
#define NPU_MINOR			10
#define NPU_VERTEX_NAME			"npu"
/* Number of message id of mailbox -> Valid message ID will be 0 ~ MAX_MSG_ID_CNT-1 */
#define NPU_MAX_MSG_ID_CNT		32

/* Queue between Session manager and Network/Frame manager */
/* Maximum number of elements can be hold by frame request queue */
#define BUFFER_Q_LIST_SIZE		(1 << 10)

/* Maximum number of elements can be hold by network management request queue */
#define NCP_MGMT_LIST_SIZE		(1 << 8)

/* Interface between Network/Frame anager and npu-interface */
/* Use same size for request and response */
#define NW_Q_SIZE			(1 << 10)
#define FRAME_Q_SIZE			(1 << 10)

/* Log Keeper settings */
/* Size of buffer for firmware log keeper */
#define NPU_FW_LOG_KEEP_SIZE		(4096 * 1024)

/* npu_queue and vb_queue constants */
/* Maximum number of index allowed for mapping */
#define NPU_MAX_BUFFER			16
#ifdef CONFIG_NPU_HARDWARE
/* ioremap area definition of NPU IP */
/* TODO: Those constants shall be read from DT later */
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
#define NPU_IOMEM_TCUSRAM_START         0x17A00000
#define NPU_IOMEM_TCUSRAM_END           0x17A80000
#define NPU_IOMEM_IDPSRAM_START         0x17B00000
#define NPU_IOMEM_IDPSRAM_END           0x17C00000
#define NPU_IOMEM_SFR_NPU0_START        0x17900000
#define NPU_IOMEM_SFR_NPU0_END          0x179C0000
#define NPU_IOMEM_SFR_NPU1_START        0x179C0000
#define NPU_IOMEM_SFR_NPU1_END          0x17A00000

#else//#elif EVT_1
#define NPU_IOMEM_TCUSRAM_START		0x19200000
#define NPU_IOMEM_TCUSRAM_END		0x19280000
#define NPU_IOMEM_IDPSRAM_START		0x19300000
#define NPU_IOMEM_IDPSRAM_END		0x19400000
#define NPU_IOMEM_SFR_NPU0_START	0x17900000
#define NPU_IOMEM_SFR_NPU0_END          0x17A00000
#define NPU_IOMEM_SFR_NPU1_START        0x17A00000
#define NPU_IOMEM_SFR_NPU1_END          0x17AF0000
#endif


#define NPU_IOMEM_PMU_NPU_START         0x15861D00
#define NPU_IOMEM_PMU_NPU_END           0x15861E00
#define NPU_IOMEM_PMU_NPU_CPU_START     0x15862F00
#define NPU_IOMEM_PMU_NPU_CPU_END       0x15863000

#define NPU_IOMEM_BAAW_NPU_START        0x1A350000
#define NPU_IOMEM_BAAW_NPU_END          0x1A3500A0

#define NPU_IOMEM_MBOX_SFR_START	0x178B0000
#define NPU_IOMEM_MBOX_SFR_END		0x178B017C

#define NPU_IOMEM_PWM_START	        0x178C0000
#define NPU_IOMEM_PWM_END		0x178D0000
#endif

/* Hack */
#define NPU_CM7_RELEASE_HACK

/* Offset of PWM conunter used for profiling */
#define TCNTO0_OFF                      0x0014

/* Definition for ZEBU */
//#define ZEBU_EMULATION
#ifdef CONFIG_NPU_HARDWARE
//[BAE]
#define FORCE_HWACG_DISABLE
/* #define FORCE_WDT_DISABLE */
#define NPU_MAILBOX_DEFAULT_TID         0

#ifdef CONFIG_NPU_ZEBU_EMULATION
#define NPU_CM7_RELEASE_HACK
//#define CLEAR_ON_SECOND_LOAD_ONLY
//#define CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS       /* defined in npu-util-memdump.[ch] */
#define T32_GROUP_TRACE_SUPPORT
//#define REINIT_NPU_BAAW
#endif
#endif  /* CONFIG_NPU_ZEBU_EMULATION */

#ifdef CONFIG_NPU_HARDWARE
/* Clear TCU/IDP SRAM area before firmware loading */
#define CLEAR_SRAM_ON_FIRMWARE_LOADING

/* Start address of firmware address space */
#define NPU_FW_BASE_ADDR                0

/* Skip sending PURGE command(Trigerred by STREAM_OFF) to NPU H/W */
//#define BYPASS_HW_STREAMOFF

/*
 * Mailbox size configuration
 * Please refer the mailbox.h for detail
 */
#define K_SIZE	(1024)
#define NPU_MAILBOX_HDR_SECTION_LEN	(4 * K_SIZE)
#define NPU_MAILBOX_SIZE		(32 * K_SIZE)
#define NPU_MAILBOX_BASE		0x80000
#endif

/* WA to prevent TCU hang */
//#define CLKGate1_DRCG_EN
#define CLKGate23_SOC_HWACG
#define CLKGate4_IP_HWACG
//#define CLKGate5_IP_DRCG_EN

//#define MBOX_MOCK_ENABLE

/*
 * Delay insearted before streamoff and power down sequence on EMERGENCY.
 * It is required to give the H/W finish its DMA transfer.
 */
#define POWER_DOWN_DELAY_ON_EMERGENCY	(300u)

/* Stream off delay should be longer than timeout of firmware response */
#define STREAMOFF_DELAY_ON_EMERGENCY	(12000u)

#endif /* _NPU_CONFIG_H_ */
