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

#ifdef CONFIG_SPU_VERIFY
extern long spu_firmware_signature_verify(const char* fw_name, const u8* fw_data, const long fw_size);
#else
static inline long spu_firmware_signature_verify(const char* fw_name, const u8* fw_data, const long fw_size) {
	const static struct {
		const char *tag;
		int len;
		int metadata_size;
	} tags[] = {
		{ TSP_TAG,       TAG_LEN(TSP),       SPU_METADATA_SIZE(TSP) },
		{ MFC_TAG,       TAG_LEN(MFC),       SPU_METADATA_SIZE(MFC) },
		{ WACOM_TAG,     TAG_LEN(WACOM),     SPU_METADATA_SIZE(WACOM) },
		{ PDIC_TAG,      TAG_LEN(PDIC),      SPU_METADATA_SIZE(PDIC) },
		{ SENSORHUB_TAG, TAG_LEN(SENSORHUB), SPU_METADATA_SIZE(SENSORHUB) },
	};
	int i;

	if (!fw_name || !fw_data || fw_size < 0) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(tags); ++i) {
		if(!strncmp(fw_name, tags[i].tag, tags[i].len)) {
			long offset = fw_size - tags[i].metadata_size;
			if (!strncmp(fw_name, fw_data + offset, tags[i].len)) {
				return offset;
			} else {
				return -EINVAL;
			}
		}
	}

	return -EINVAL;
}
#endif

#endif //end _SPU_VERIFY_H_
