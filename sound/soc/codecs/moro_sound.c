/*
 * moro_sound.c  --  Sound mod for Madera, S8 sound driver
 *
 * Author	: @morogoku https://github.com/morogoku
 * Edited by	: @noxxxious https://github.com/noxxxious
 *
 * Date		: March 2019 - v1.0
 *
 *
 * Based on the Boeffla Sound 1.6 for Galaxy S3
 *
 * Credits: 	andip71, author of Boeffla Sound
 *		Supercurio, Yank555 and Gokhanmoral.
 *		AndreiLux, for his Madera control sound mod
 *		Flar2, for his speaker gain mod
 *
 */


#include "moro_sound.h"


/* Variables */
static struct regmap *map;

/* Internal moro sound variables */
static int first = 1;
static int moro_sound = 0;

static int headphone_gain_l, headphone_gain_r;
static int earpiece_gain;
static int out2l_mix_source, out2r_mix_source;
static int eq1_mix_source, eq2_mix_source;

static int eq = 0;
static int eq_gains[5];

static unsigned int get_headphone_gain_l(void);
static unsigned int get_headphone_gain_r(void);
static void set_headphone_gain_l(int gain);
static void set_headphone_gain_r(int gain);

static unsigned int get_earpiece_gain(void);
static void set_earpiece_gain(int gain);

static void set_out2l_mix_source(int value);
static void set_out2r_mix_source(int value);

static void set_eq1_mix_source(int value);
static void set_eq2_mix_source(int value);

static void set_eq(void);
static void set_eq_gains(void);

static void reset_moro_sound(void);
static void reset_audio_hub(void);
static void update_audio_hub(void);

/* Internal helper functions */

#define madera_write(reg, val) _regmap_write_nohook(map, reg, val)

#define madera_read(reg, val) regmap_read(map, reg, val)

static unsigned int get_headphone_gain_l(void)
{
	unsigned int val;

	madera_read(MADERA_DAC_DIGITAL_VOLUME_2L, &val);
	val &= MADERA_OUT2L_VOL_MASK;
	val >>= MADERA_OUT2L_VOL_SHIFT;

	return val;
}

static unsigned int get_headphone_gain_r(void)
{
	unsigned int val;

	madera_read(MADERA_DAC_DIGITAL_VOLUME_2R, &val);
	val &= MADERA_OUT2R_VOL_MASK;
	val >>= MADERA_OUT2R_VOL_SHIFT;

	return val;
}

static void set_headphone_gain_l(int gain)
{
	unsigned int val;

	madera_read(MADERA_DAC_DIGITAL_VOLUME_2L, &val);
	val &= ~MADERA_OUT2L_VOL_MASK;
	val |= (gain << MADERA_OUT2L_VOL_SHIFT);
	madera_write(MADERA_DAC_DIGITAL_VOLUME_2L, val);
}

static void set_headphone_gain_r(int gain)
{
	unsigned int val;

	madera_read(MADERA_DAC_DIGITAL_VOLUME_2R, &val);
	val &= ~MADERA_OUT2R_VOL_MASK;
	val |= (gain << MADERA_OUT2R_VOL_SHIFT);
	madera_write(MADERA_DAC_DIGITAL_VOLUME_2R, val);
}

static unsigned int get_earpiece_gain(void)
{
	unsigned int val;

	madera_read(MADERA_DAC_DIGITAL_VOLUME_3L, &val);
	val &= MADERA_OUT3L_VOL_MASK;
	val >>= MADERA_OUT3L_VOL_SHIFT;

	return val;
}

static void set_earpiece_gain(int gain)
{
	unsigned int val;

	madera_read(MADERA_DAC_DIGITAL_VOLUME_3L, &val);
	val &= ~MADERA_OUT3L_VOL_MASK;
	val |= (gain << MADERA_OUT3L_VOL_SHIFT);
	madera_write(MADERA_DAC_DIGITAL_VOLUME_3L, val);
}

static void set_out2l_mix_source(int value)
{
	unsigned int val;

	madera_read(MADERA_OUT2LMIX_INPUT_1_SOURCE, &val);
	val &= ~MADERA_MIXER_SOURCE_MASK;
	val |= (value << MADERA_MIXER_SOURCE_SHIFT);
	madera_write(MADERA_OUT2LMIX_INPUT_1_SOURCE, val);
}

static void set_out2r_mix_source(int value)
{
	unsigned int val;

	madera_read(MADERA_OUT2RMIX_INPUT_1_SOURCE, &val);
	val &= ~MADERA_MIXER_SOURCE_MASK;
	val |= (value << MADERA_MIXER_SOURCE_SHIFT);
	madera_write(MADERA_OUT2RMIX_INPUT_1_SOURCE, val);
}

static void set_eq1_mix_source(int value)
{
	unsigned int val;

	madera_read(MADERA_EQ1MIX_INPUT_1_SOURCE, &val);
	val &= ~MADERA_MIXER_SOURCE_MASK;
	val |= (value << MADERA_MIXER_SOURCE_SHIFT);
	madera_write(MADERA_EQ1MIX_INPUT_1_SOURCE, val);
}


static void set_eq2_mix_source(int value)
{
	unsigned int val;

	madera_read(MADERA_EQ2MIX_INPUT_1_SOURCE, &val);
	val &= ~MADERA_MIXER_SOURCE_MASK;
	val |= (value << MADERA_MIXER_SOURCE_SHIFT);
	madera_write(MADERA_EQ2MIX_INPUT_1_SOURCE, val);
}

static void set_eq(void)
{
	unsigned int val;

	if (eq && moro_sound) {
		madera_read(MADERA_EQ1_1, &val);
		val &= ~MADERA_EQ1_ENA_MASK;
		val |= 1 << MADERA_EQ1_ENA_SHIFT;
		madera_write(MADERA_EQ1_1, val);
		madera_read(MADERA_EQ2_1, &val);
		val &= ~MADERA_EQ2_ENA_MASK;
		val |= 1 << MADERA_EQ2_ENA_SHIFT;
		madera_write(MADERA_EQ2_1, val);
		set_eq1_mix_source(32);
		set_eq2_mix_source(33);
		set_out2l_mix_source(80);
		set_out2r_mix_source(81);
	} else {
		madera_read(MADERA_EQ1_1, &val);
		val &= ~MADERA_EQ1_ENA_MASK;
		val |= 0 << MADERA_EQ1_ENA_SHIFT;
		madera_write(MADERA_EQ1_1, val);
		madera_read(MADERA_EQ2_1, &val);
		val &= ~MADERA_EQ2_ENA_MASK;
		val |= 0 << MADERA_EQ2_ENA_SHIFT;
		madera_write(MADERA_EQ2_1, val);
		eq1_mix_source = EQ1_MIX_DEFAULT;
		eq2_mix_source = EQ2_MIX_DEFAULT;
		set_eq1_mix_source(eq1_mix_source);
		set_eq2_mix_source(eq2_mix_source);
		out2l_mix_source = OUT2L_MIX_DEFAULT;
		out2r_mix_source = OUT2R_MIX_DEFAULT;
		set_out2l_mix_source(out2l_mix_source);
		set_out2r_mix_source(out2r_mix_source);
	}

	set_eq_gains();
}

static void set_eq_gains(void)
{
	unsigned int val;
	unsigned int gain1, gain2, gain3, gain4, gain5;

	gain1 = eq_gains[0];
	gain2 = eq_gains[1];
	gain3 = eq_gains[2];
	gain4 = eq_gains[3];
	gain5 = eq_gains[4];

	madera_read(MADERA_EQ1_1, &val);

	val &= MADERA_EQ1_ENA_MASK;
	val |= ((gain1 + EQ_GAIN_OFFSET) << MADERA_EQ1_B1_GAIN_SHIFT);
	val |= ((gain2 + EQ_GAIN_OFFSET) << MADERA_EQ1_B2_GAIN_SHIFT);
	val |= ((gain3 + EQ_GAIN_OFFSET) << MADERA_EQ1_B3_GAIN_SHIFT);

	madera_write(MADERA_EQ1_1, val);
	madera_write(MADERA_EQ2_1, val);

	madera_read(MADERA_EQ1_2, &val);

	val &= MADERA_EQ1_B1_MODE_MASK;
	val |= ((gain4 + EQ_GAIN_OFFSET) << MADERA_EQ1_B4_GAIN_SHIFT);
	val |= ((gain5 + EQ_GAIN_OFFSET) << MADERA_EQ1_B5_GAIN_SHIFT);

	madera_write(MADERA_EQ1_2, val);
	madera_write(MADERA_EQ2_2, val);
}

/* Sound hook functions */
void moro_sound_hook_madera_pcm_probe(struct regmap *pmap)
{
	map = pmap;
	moro_sound = MORO_SOUND_DEFAULT;
	eq = EQ_DEFAULT;
	set_eq();

	if (moro_sound)
		reset_moro_sound();
}

unsigned int moro_sound_write_hook(unsigned int reg, unsigned int val)
{
	if (!moro_sound)
		return val;

	switch (reg) {
		case MADERA_DAC_DIGITAL_VOLUME_2L:
			val &= ~MADERA_OUT2L_VOL_MASK;
			val |= (headphone_gain_l << MADERA_OUT2L_VOL_SHIFT);
				break;
		case MADERA_DAC_DIGITAL_VOLUME_2R:
			val &= ~MADERA_OUT2R_VOL_MASK;
			val |= (headphone_gain_r << MADERA_OUT2R_VOL_SHIFT);
				break;
		case MADERA_DAC_DIGITAL_VOLUME_3L:
			val &= ~MADERA_OUT3L_VOL_MASK;
			val |= (earpiece_gain << MADERA_OUT3L_VOL_SHIFT);
			break;
		if (eq) {
			case MADERA_OUT2LMIX_INPUT_1_SOURCE:
				val &= ~MADERA_MIXER_SOURCE_MASK;
				val |= (out2l_mix_source << MADERA_MIXER_SOURCE_SHIFT);
				break;
			case MADERA_OUT2RMIX_INPUT_1_SOURCE:
				val &= ~MADERA_MIXER_SOURCE_MASK;
				val |= (out2r_mix_source << MADERA_MIXER_SOURCE_SHIFT);
				break;
		}
		default:
			break;
	}

	return val;
}

/* Initialization functions */

static void reset_moro_sound(void)
{
	headphone_gain_l = HEADPHONE_DEFAULT;
	headphone_gain_r = HEADPHONE_DEFAULT;

	earpiece_gain = EARPIECE_DEFAULT;

	out2l_mix_source = OUT2L_MIX_DEFAULT;
	out2r_mix_source = OUT2R_MIX_DEFAULT;

	eq1_mix_source = EQ1_MIX_DEFAULT;
	eq2_mix_source = EQ2_MIX_DEFAULT;
}


static void reset_audio_hub(void)
{
	set_headphone_gain_l(HEADPHONE_DEFAULT);
	set_headphone_gain_r(HEADPHONE_DEFAULT);

	set_earpiece_gain(EARPIECE_DEFAULT);

	set_out2l_mix_source(OUT2L_MIX_DEFAULT);
	set_out2r_mix_source(OUT2R_MIX_DEFAULT);

	set_eq1_mix_source(EQ1_MIX_DEFAULT);
	set_eq2_mix_source(EQ2_MIX_DEFAULT);

	set_eq();
}

static void update_audio_hub(void)
{
	set_headphone_gain_l(headphone_gain_l);
	set_headphone_gain_r(headphone_gain_r);

	set_earpiece_gain(earpiece_gain);

	set_out2l_mix_source(out2l_mix_source);
	set_out2r_mix_source(out2r_mix_source);

	set_eq1_mix_source(eq1_mix_source);
	set_eq2_mix_source(eq2_mix_source);

	set_eq();
}

/* sysfs interface functions */
static ssize_t moro_sound_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", moro_sound);
}


static ssize_t moro_sound_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (((val == 0) || (val == 1))) {
		if (moro_sound != val) {
			moro_sound = val;
			if (first) {
				reset_moro_sound();
				first = 0;
			}

			if (val == 1)
				update_audio_hub();
			else if (val == 0)
				reset_audio_hub();
		}
	}

	return count;
}


/* Headphone volume */
static ssize_t headphone_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d\n", headphone_gain_l, headphone_gain_r);
}


static ssize_t headphone_gain_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val_l;
	int val_r;

	if (!moro_sound)
		return count;

	if (sscanf(buf, "%d %d", &val_l, &val_r) < 2)
		return -EINVAL;

	if (val_l < HEADPHONE_MIN)
		val_l = HEADPHONE_MIN;

	if (val_l > HEADPHONE_MAX)
		val_l = HEADPHONE_MAX;

	if (val_r < HEADPHONE_MIN)
		val_r = HEADPHONE_MIN;

	if (val_r > HEADPHONE_MAX)
		val_r = HEADPHONE_MAX;

	headphone_gain_l = val_l;
	headphone_gain_r = val_r;

	set_headphone_gain_l(headphone_gain_l);
	set_headphone_gain_r(headphone_gain_r);

	return count;
}

static ssize_t headphone_limits_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Min: %d Max: %d Def: %d\n", HEADPHONE_MIN, HEADPHONE_MAX, HEADPHONE_DEFAULT);
}

/* Earpiece Volume */

static ssize_t earpiece_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", earpiece_gain);
}

static ssize_t earpiece_gain_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val;

	if (!moro_sound)
		return count;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (val < EARPIECE_MIN)
		val = EARPIECE_MIN;

	if (val > EARPIECE_MAX)
		val = EARPIECE_MAX;

	earpiece_gain = val;
	set_earpiece_gain(earpiece_gain);

	return count;
}

static ssize_t earpiece_limits_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// return version information
	return sprintf(buf, "Min: %d Max: %d Def:%d\n", EARPIECE_MIN, EARPIECE_MAX, EARPIECE_DEFAULT);
}

/* EQ */
static ssize_t eq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", eq);
}

static ssize_t eq_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int val;

	if (!moro_sound)
		return count;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (((val == 0) || (val == 1))) {
		if (eq != val) {
			eq = val;
			set_eq();
		}
	}

	return count;
}


/* EQ GAIN */

static ssize_t eq_gains_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d %d %d %d\n", eq_gains[0], eq_gains[1], eq_gains[2], eq_gains[3], eq_gains[4]);
}

static ssize_t eq_gains_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int gains[5];
	int i;

	if (!moro_sound)
		return count;

	if (sscanf(buf, "%d %d %d %d %d", &gains[0], &gains[1], &gains[2], &gains[3], &gains[4]) < 5)
		return -EINVAL;

	for (i = 0; i <= 4; i++) {
		if (gains[i] < EQ_GAIN_MIN)
			gains[i] = EQ_GAIN_MIN;
		if (gains[i] > EQ_GAIN_MAX)
			gains[i] = EQ_GAIN_MAX;
		eq_gains[i] = gains[i];
	}

	set_eq_gains();

	return count;
}

static ssize_t eq_b1_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", eq_gains[0]);
}

static ssize_t eq_b1_gain_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int val;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (val < EQ_GAIN_MIN)
		val = EQ_GAIN_MIN;

	if (val > EQ_GAIN_MAX)
		val = EQ_GAIN_MAX;

	eq_gains[0] = val;
	set_eq_gains();

	return count;
}

static ssize_t eq_b2_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", eq_gains[1]);
}

static ssize_t eq_b2_gain_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int val;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (val < EQ_GAIN_MIN)
		val = EQ_GAIN_MIN;

	if (val > EQ_GAIN_MAX)
		val = EQ_GAIN_MAX;

	eq_gains[1] = val;
	set_eq_gains();

	return count;
}

static ssize_t eq_b3_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", eq_gains[2]);
}

static ssize_t eq_b3_gain_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int val;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (val < EQ_GAIN_MIN)
		val = EQ_GAIN_MIN;

	if (val > EQ_GAIN_MAX)
		val = EQ_GAIN_MAX;

	eq_gains[2] = val;
	set_eq_gains();

	return count;
}

static ssize_t eq_b4_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", eq_gains[3]);
}

static ssize_t eq_b4_gain_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int val;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (val < EQ_GAIN_MIN)
		val = EQ_GAIN_MIN;

	if (val > EQ_GAIN_MAX)
		val = EQ_GAIN_MAX;

	eq_gains[3] = val;
	set_eq_gains();

	return count;
}

static ssize_t eq_b5_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", eq_gains[4]);
}

static ssize_t eq_b5_gain_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int val;

	if (sscanf(buf, "%d", &val) < 1)
		return -EINVAL;

	if (val < EQ_GAIN_MIN)
		val = EQ_GAIN_MIN;

	if (val > EQ_GAIN_MAX)
		val = EQ_GAIN_MAX;

	eq_gains[4] = val;
	set_eq_gains();

	return count;
}

static ssize_t reg_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int out2_ena, out2l_mix, out2r_mix, eq1_ena, eq2_ena, eq1_mix, eq2_mix, eq_b1,
			eq_b2, eq_b3, eq_b4, eq_b5;

	madera_read(MADERA_OUTPUT_ENABLES_1, &out2_ena);
		out2_ena = (out2_ena & MADERA_OUT2L_ENA_MASK) >> MADERA_OUT2L_ENA_SHIFT;

	madera_read(MADERA_OUT2LMIX_INPUT_1_SOURCE, &out2l_mix);
	madera_read(MADERA_OUT2RMIX_INPUT_1_SOURCE, &out2r_mix);
	madera_read(MADERA_EQ1_1, &eq1_ena);
		eq1_ena = (eq1_ena & MADERA_EQ1_ENA_MASK) >> MADERA_EQ1_ENA_SHIFT;
	madera_read(MADERA_EQ2_1, &eq2_ena);
		eq2_ena = (eq2_ena & MADERA_EQ2_ENA_MASK) >> MADERA_EQ2_ENA_SHIFT;
	madera_read(MADERA_EQ1MIX_INPUT_1_SOURCE, &eq1_mix);
	madera_read(MADERA_EQ2MIX_INPUT_1_SOURCE, &eq2_mix);
	madera_read(MADERA_EQ1_1, &eq_b1);
		eq_b1 = ((eq_b1 & MADERA_EQ1_B1_GAIN_MASK) >> MADERA_EQ1_B1_GAIN_SHIFT) - EQ_GAIN_OFFSET;
	madera_read(MADERA_EQ1_1, &eq_b2);
		eq_b2 = ((eq_b2 & MADERA_EQ1_B2_GAIN_MASK) >> MADERA_EQ1_B2_GAIN_SHIFT) - EQ_GAIN_OFFSET;
	madera_read(MADERA_EQ1_1, &eq_b3);
		eq_b3 = ((eq_b3 & MADERA_EQ1_B3_GAIN_MASK) >> MADERA_EQ1_B3_GAIN_SHIFT) - EQ_GAIN_OFFSET;
	madera_read(MADERA_EQ1_2, &eq_b4);
		eq_b4 = ((eq_b4 & MADERA_EQ1_B4_GAIN_MASK) >> MADERA_EQ1_B4_GAIN_SHIFT) - EQ_GAIN_OFFSET;
	madera_read(MADERA_EQ1_2, &eq_b5);
		eq_b5 = ((eq_b5 & MADERA_EQ1_B5_GAIN_MASK) >> MADERA_EQ1_B5_GAIN_SHIFT) - EQ_GAIN_OFFSET;

	return sprintf(buf, "\
headphone_gain_l: reg: %d, variable: %d \
headphone_gain_r: reg: %d, variable: %d \
earpiece_gain: %d \
HPOUT Enabled: %d \
HPOUT2L Source: %d \
HPOUT2R Source: %d \
EQ1 Enabled: %d \
EQ2 Enabled: %d \
EQ1MIX source: %d \
EQ2MIX source: %d \
EQ b1 gain: %d \
EQ b2 gain: %d \
EQ b3 gain: %d \
EQ b4 gain: %d \
EQ b5 gain: %d \
",
get_headphone_gain_l(),
get_headphone_gain_r(),
headphone_gain_l,
headphone_gain_r,
first,
get_earpiece_gain(),
out2_ena,
out2l_mix,
out2r_mix,
eq1_ena,
eq2_ena,
eq1_mix,
eq2_mix,
eq_b1,
eq_b2,
eq_b3,
eq_b4,
eq_b5);
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", MORO_SOUND_VERSION);
}


/* Sysfs permissions */
static DEVICE_ATTR(moro_sound, 0664, moro_sound_show, moro_sound_store);
static DEVICE_ATTR(headphone_gain, 0664, headphone_gain_show, headphone_gain_store);
static DEVICE_ATTR(headphone_limits, 0664, headphone_limits_show, NULL);
static DEVICE_ATTR(earpiece_gain, 0664, earpiece_gain_show, earpiece_gain_store);
static DEVICE_ATTR(earpiece_limits, 0664, earpiece_limits_show, NULL);
static DEVICE_ATTR(eq, 0664, eq_show, eq_store);
static DEVICE_ATTR(eq_gains, 0664, eq_gains_show, eq_gains_store);
static DEVICE_ATTR(eq_b1_gain, 0664, eq_b1_gain_show, eq_b1_gain_store);
static DEVICE_ATTR(eq_b2_gain, 0664, eq_b2_gain_show, eq_b2_gain_store);
static DEVICE_ATTR(eq_b3_gain, 0664, eq_b3_gain_show, eq_b3_gain_store);
static DEVICE_ATTR(eq_b4_gain, 0664, eq_b4_gain_show, eq_b4_gain_store);
static DEVICE_ATTR(eq_b5_gain, 0664, eq_b5_gain_show, eq_b5_gain_store);
static DEVICE_ATTR(version, 0664, version_show, NULL);
static DEVICE_ATTR(reg_dump, 0664, reg_dump_show, NULL);

static struct attribute *moro_sound_attributes[] = {
	&dev_attr_moro_sound.attr,
	&dev_attr_headphone_gain.attr,
	&dev_attr_headphone_limits.attr,
	&dev_attr_earpiece_gain.attr,
	&dev_attr_earpiece_limits.attr,
	&dev_attr_eq.attr,
	&dev_attr_eq_gains.attr,
	&dev_attr_eq_b1_gain.attr,
	&dev_attr_eq_b2_gain.attr,
	&dev_attr_eq_b3_gain.attr,
	&dev_attr_eq_b4_gain.attr,
	&dev_attr_eq_b5_gain.attr,
	&dev_attr_version.attr,
	&dev_attr_reg_dump.attr,
	NULL
};

static struct attribute_group moro_sound_control_group = {
	.attrs = moro_sound_attributes,
};

static struct miscdevice moro_sound_control_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "moro_sound",
};

static int moro_sound_init(void)
{
	misc_register(&moro_sound_control_device);

	if (sysfs_create_group(&moro_sound_control_device.this_device->kobj,
				&moro_sound_control_group) < 0) {
		return 0;
	}

	reset_moro_sound();

	return 0;
}

static void moro_sound_exit(void)
{
	sysfs_remove_group(&moro_sound_control_device.this_device->kobj,
                           &moro_sound_control_group);
}

/* Driver init and exit functions */
module_init(moro_sound_init);
module_exit(moro_sound_exit);
