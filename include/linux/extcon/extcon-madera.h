/*
 * extcon-madera.h - public extcon driver API for Cirrus Logic Madera codecs
 *
 * Copyright 2015-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef EXTCON_MADERA_H
#define EXTCON_MADERA_H

#include <linux/mfd/madera/registers.h>

/* Conversion between ohms and hundredths of an ohm. */
static inline unsigned int madera_hohm_to_ohm(unsigned int hohms)
{
	if (hohms == INT_MAX)
		return hohms;
	else
		return (hohms + 50) / 100;
}

static inline unsigned int madera_ohm_to_hohm(unsigned int ohms)
{
	if (ohms >= (INT_MAX / 100))
		return INT_MAX;
	else
		return ohms * 100;
}

#define MADERA_MICD_LVL_1_TO_7 \
		(MADERA_MICD_LVL_1 | MADERA_MICD_LVL_2 | \
		 MADERA_MICD_LVL_3 | MADERA_MICD_LVL_4 | \
		 MADERA_MICD_LVL_5 | MADERA_MICD_LVL_6 | \
		 MADERA_MICD_LVL_7)

#define MADERA_MICD_LVL_0_TO_7 (MADERA_MICD_LVL_0 | MADERA_MICD_LVL_1_TO_7)

#define MADERA_MICD_LVL_0_TO_8 (MADERA_MICD_LVL_0_TO_7 | MADERA_MICD_LVL_8)

/* Notify data structure for MADERA_NOTIFY_HPDET */
struct madera_hpdet_notify_data {
	unsigned int impedance_x100;	/* ohms * 100 */
};

/* Notify data structure for MADERA_NOTIFY_MICDET */
struct madera_micdet_notify_data {
	unsigned int impedance_x100;	/* ohms * 100 */
	bool present;
	int out_num;			/* 1 = OUT1, 2 = OUT2 */
};

struct madera_micd_bias {
	unsigned int bias;
	bool enabled;
};

struct madera_hpdet_trims {
	int off_x4;
	int grad_x4;
};

struct madera_extcon {
	struct device *dev;
	struct madera *madera;
	const struct madera_accdet_pdata *pdata;
	struct mutex lock;
	struct regulator *micvdd;
	struct input_dev *input;
	struct extcon_dev *edev;
	struct gpio_desc *micd_pol_gpio;

	u16 last_jackdet;
	int hp_tuning_level;

	const struct madera_hpdet_calibration_data *hpdet_ranges;
	int num_hpdet_ranges;
	unsigned int hpdet_init_range;
	const struct madera_hpdet_trims *hpdet_trims;

	unsigned int hpdet_short_x100;

	int micd_mode;
	const struct madera_micd_config *micd_modes;
	int num_micd_modes;

	const struct madera_micd_range *micd_ranges;
	int num_micd_ranges;

	int micd_res_old;
	int micd_debounce;
	int micd_count;

	int moisture_count;

	int mic_impedance;
	struct completion manual_mic_completion;

	struct delayed_work micd_detect_work;

	bool have_mic;
	bool detecting;
	bool wait_for_buttons;
	int jack_flips;

	const struct madera_jd_state *state;
	const struct madera_jd_state *old_state;
	struct delayed_work state_timeout_work;

	struct wakeup_source detection_wake_lock;

	struct madera_micd_bias micd_bias;
};

enum madera_accdet_mode {
	MADERA_ACCDET_MODE_MIC,
	MADERA_ACCDET_MODE_HPL,
	MADERA_ACCDET_MODE_HPR,
	MADERA_ACCDET_MODE_HPM,
	MADERA_ACCDET_MODE_ADC,
	MADERA_ACCDET_MODE_INVALID,
};

struct madera_jd_state {
	enum madera_accdet_mode mode;

	int (*start)(struct madera_extcon *);
	void (*restart)(struct madera_extcon *);
	int (*reading)(struct madera_extcon *, int);
	void (*stop)(struct madera_extcon *);

	int (*timeout_ms)(struct madera_extcon *);
	void (*timeout)(struct madera_extcon *);
};

int madera_jds_set_state(struct madera_extcon *info,
			 const struct madera_jd_state *new_state);

void madera_set_headphone_imp(struct madera_extcon *info, int ohms_x100);

extern const struct madera_jd_state madera_hpdet_left;
extern const struct madera_jd_state madera_hpdet_right;
extern const struct madera_jd_state madera_hpdet_moisture;
extern const struct madera_jd_state madera_micd_button;
extern const struct madera_jd_state madera_micd_microphone;
extern const struct madera_jd_state madera_micd_adc_mic;

int madera_hpdet_start(struct madera_extcon *info);
void madera_hpdet_restart(struct madera_extcon *info);
void madera_hpdet_stop(struct madera_extcon *info);
int madera_hpdet_reading(struct madera_extcon *info, int val);

int madera_micd_start(struct madera_extcon *info);
int madera_micd_button_start(struct madera_extcon *info);
void madera_micd_stop(struct madera_extcon *info);
int madera_micd_button_reading(struct madera_extcon *info, int val);

int madera_micd_mic_start(struct madera_extcon *info);
void madera_micd_mic_stop(struct madera_extcon *info);
int madera_micd_mic_reading(struct madera_extcon *info, int val);
int madera_micd_mic_timeout_ms(struct madera_extcon *info);
void madera_micd_mic_timeout(struct madera_extcon *info);

void madera_extcon_report(struct madera_extcon *info, int which, bool attached);

int madera_extcon_manual_mic_reading(struct madera_extcon *info);
#endif
