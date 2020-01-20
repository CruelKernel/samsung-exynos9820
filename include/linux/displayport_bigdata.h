#ifndef DISPLAYPORT_BIGDATA_H
#define DISPLAYPORT_BIGDATA_H
#include <linux/platform_device.h>
#include <stdarg.h>

enum DP_BD_ITEM_LIST {
	ERR_AUX,
	ERR_EDID,
	ERR_HDCP_AUTH,
	ERR_LINK_TRAIN,
	ERR_INF_IRQHPD,

	BD_LINK_CONFIGURE,
	BD_ADAPTER_HWID,
	BD_ADAPTER_FWVER,
	BD_ADAPTER_TYPE,
	BD_MAX_LANE_COUNT,
	BD_MAX_LINK_RATE,
	BD_CUR_LANE_COUNT,
	BD_CUR_LINK_RATE,
	BD_HDCP_VER,
	BD_ORIENTATION,
	BD_RESOLUTION,
	BD_EDID,
	BD_ADT_VID,
	BD_ADT_PID,
	BD_DP_MODE,
	BD_SINK_NAME,
	BD_AUD_CH,
	BD_AUD_FREQ,
	BD_AUD_BIT,
	BD_ITEM_MAX,
};

void secdp_bigdata_save_item(enum DP_BD_ITEM_LIST item, ...);
void secdp_bigdata_inc_error_cnt(enum DP_BD_ITEM_LIST err);
void secdp_bigdata_clr_error_cnt(enum DP_BD_ITEM_LIST err);
void secdp_bigdata_connection(void);
void secdp_bigdata_disconnection(void);
void secdp_bigdata_init(struct class *dp_class);
#endif /* DISPLAYPORT_BIGDATA_H */
