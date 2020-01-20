/*
 *  LED driver for SEP
 *
 * Copyright (C) 2018 Samsung Electronics
 * Author: Hyuk Kang, HyungJae Im
 *
 */

#define LED_ALL			0xFF

#define LED_W_MASK		0xFF000000
#define LED_R_MASK		0x00FF0000
#define LED_G_MASK		0x0000FF00
#define LED_B_MASK		0x000000FF

#define LED_W(color)	((color & LED_W_MASK) >> 24)
#define LED_R(color)	((color & LED_R_MASK) >> 16)
#define LED_G(color)	((color & LED_G_MASK) >> 8)
#define LED_B(color)	(color & LED_B_MASK)

#define LED_C(i, color)	((color >> (i * 8)) & 0xFF) /* 0=B 1=G 2=R 3=W */
#define LED_I(i, color) (i == 0 ? LED_R(color) : \
						i == 1 ? LED_G(color) : \
						i == 2 ? LED_B(color) : 0)

#define COLOR(R, G, B)	((R & 0xFF) << 16 | \
						(G & 0xFF) << 8 | \
						(B & 0xFF))

#define COLOR_MAX	0xFF

enum led_sep_pattern {
	PATTERN_OFF,
	CHARGING,
	CHARGING_ERR,
	MISSED_NOTI,
	LOW_BATTERY,
	FULLY_CHARGED,
	POWERING,
	POWERING_OFF,
	FOTA,
};

static inline
const char *pattern_string(enum led_sep_pattern ptn)
{
	switch (ptn) {
	case PATTERN_OFF:	return "pattern_off";
	case CHARGING:		return "charging";
	case CHARGING_ERR:	return "charging_err";
	case MISSED_NOTI:	return "missed_noti";
	case LOW_BATTERY:	return "low_battery";
	case FULLY_CHARGED:	return "fully_charged";
	case POWERING:		return "powering";
	case POWERING_OFF:	return "powering_off";
	case FOTA:		return "fota";
	default:
		return "undefined";
	}
}

struct led_sep {
	struct device *sdev;
	struct led_sep_ops *ops;

	int maxpower;
	int lowpower_mode;
	int lowpower_current;
};

struct led_sep_ops {
	const char *name;
	int (*light)(struct led_sep_ops *ops, int id,
			int on, int color);
	int (*blink)(struct led_sep_ops *ops, int id,
			int color, int on_time, int off_time, int blink_count);
	int (*test)(struct led_sep_ops *ops, int reg, int val);
	int (*reset)(struct led_sep_ops *ops);
	int (*pattern)(struct led_sep_ops *ops, int mode);

	int current_max;
	int current_lowpower;
	void *ops_data;
/* Lower 16 bits reserved */
#define SEP_NODE_BLINK				(1 << 16)
#define SEP_NODE_LOWPOWER			(1 << 17)
#define SEP_NODE_PATTERN			(1 << 18)
#define SEP_NODE_CONTROL			(1 << 19)
#define SEP_NODE_TEST				(1 << 20)
#define SEP_NODE_RESET				(1 << 21)
	unsigned long capability;
};

static inline void led_sep_set_cap(struct led_sep_ops *ops,
		unsigned long cap)
{
	if (ops)
		ops->capability |= cap;
}

static inline void led_sep_clear_cap(struct led_sep_ops *ops,
		unsigned long cap)
{
	unsigned long val = 0;

	if (ops) {
		val = ops->capability & ~cap;
		ops->capability = val;
	}
}

static inline void *led_sep_get_opsdata(struct led_sep_ops *ops)
{
	return ops->ops_data;
}

static inline void led_sep_set_opsdata(struct led_sep_ops *ops, void *data)
{
	ops->ops_data = data;
}

extern int led_sep_register_device(struct led_sep_ops *ops);
