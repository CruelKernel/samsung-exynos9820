#ifndef SEC_HQM_DEVICE_H
#define SEC_HQM_DEVICE_H

#define HQM_FEATURE_LEN 5

typedef struct hqm_device_info {
	
	char feature[HQM_FEATURE_LEN];
	
}hqm_device_info;

#ifdef CONFIG_SEC_HQM_DEVICE
extern void send_uevent_via_hqm_device(hqm_device_info);
#else
static inline void send_uevent_via_hqm_device(hqm_device_info hqm_info)
{
	pr_err("Not support HQM device\n");
	return;
}
#endif

#endif