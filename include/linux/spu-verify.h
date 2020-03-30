/*
 * spu-verify.h
 *
 * Author: JJungs-lee <jhs2.lee@samsung.com>
 *
 */
#ifndef _SPU_VERIFY_H_
#define _SPU_VERIFY_H_

#include <linux/string.h>

/* TAG NAME */
#define TSP_TAG 		"TSP"
#define MFC_TAG 		"MFC"
#define WACOM_TAG 		"WACOM"
#define PDIC_TAG 		"PDIC"
#define SENSORHUB_TAG 	"SENSORHUB"

/* METADATA LEN */
#define DIGEST_LEN 		32
#define SIGN_LEN 		512

/* TAG LEN */
#define TAG_LEN(FW)			strlen(FW##_TAG)

/* TOTAL METADATA SIZE */
#define SPU_METADATA_SIZE(FW) 	( (TAG_LEN(FW)) + (DIGEST_LEN) + (SIGN_LEN) )

extern long spu_firmware_signature_verify(const char* fw_name, const u8* fw_data, const long fw_size);

#endif //end _SPU_VERIFY_H_