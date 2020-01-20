#ifndef _NPU_ERRNO_H_
#define _NPU_ERRNO_H_

#include <linux/types.h>

/*
 *
 * 31:28 Error detection point
 * 27:24 Severity
 * 23:16 Reserved
 * 15:0  Error code
 */

typedef u32		npu_errno_t;

/* Error detection point */
#define	DRV_ERR		(0xD << 28)	/* D means Driver */
#define FW_ERR		(0xF << 28)	/* F means Firmware */

/* Severity - 4bit*/
#define SEV_FATAL	(0xF << 24)	/* The system shuold be reset */
#define SEV_CRITICAL	(0xC << 24)	/* The session(ncp) can not be processed anymore */
#define SEV_ERR		(0xE << 24)	/* The request will be canceled */
#define SEV_NOERR	(0x0 << 24)	/* No error */

/* Error code defined in driver - Base code is 1024, to avoid collision
   of pre-defined errno values of Linux */
typedef enum {
	NPU_ERR_NO_ERROR = 0,
	NPU_NW_JUST_STARTED,
	NPU_SYSTEM_JUST_STARTED,
	NPU_ERR_IN_EMERGENCY,
	NPU_ERR_NO_MEMORY = 1024+1,
	NPU_ERR_SIZE_NOT_MATCH,
	/* 4096: Error generated from protodrv */
	NPU_ERR_FRAME_CANCELED = 4096 + 1,
	NPU_ERR_SESSION_REGISTER_FAILED,
	NPU_ERR_INVALID_UID,
	NPU_ERR_INVALID_STATE,
	NPU_ERR_NPU_TIMEOUT,
	NPU_ERR_QUEUE_TIMEOUT,
	NPU_ERR_SCHED_TIMEOUT,
} npu_driver_err_codes;


/* Putting all together */
#define NPU_ERR_DRIVER(CODE)		((npu_errno_t)(DRV_ERR | SEV_ERR | ((CODE) & 0xFFFF)))
#define NPU_FATAL_DRIVER(CODE)		((npu_errno_t)(DRV_ERR | SEV_FATAL | ((CODE) & 0xFFFF)))
#define NPU_CRITICAL_DRIVER(CODE)	((npu_errno_t)(DRV_ERR | SEV_CRITICAL | ((CODE) & 0xFFFF)))

/* Mask for error code */
#define NPU_ERR_CODE(result)		((u32)((result) & 0xFFFF))

struct system_result {
	npu_errno_t result_code;
};

#endif /* _NPU_ERRNO_H_ */
