#ifndef _SEC_AUDIO_SYSFS_H
#define _SEC_AUDIO_SYSFS_H

struct sec_audio_sysfs_data {
	struct class *audio_class;
	struct device *jack_dev;
	struct device *codec_dev;
	int (*get_jack_state)(void);
	int (*get_key_state)(void);
	int (*set_jack_state)(int);
	int (*get_mic_adc)(void);
	int (*get_water_state)(void);
	int (*get_codec_id_state)(void);
};

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
int audio_register_jack_select_cb(int (*set_jack) (int));
int audio_register_jack_state_cb(int (*jack_status) (void));
int audio_register_key_state_cb(int (*key_state) (void));
int audio_register_mic_adc_cb(int (*mic_adc) (void));
int audio_register_codec_id_state_cb(int (*codec_id_state) (void));
#else
inline int audio_register_jack_select_cb(int (*set_jack) (int))
{
	return -EACCES;
}

inline int audio_register_jack_state_cb(int (*jack_status) (void))
{
	return -EACCES;
}

inline int audio_register_key_state_cb(int (*key_state) (void))
{
	return -EACCES;
}

inline int audio_register_mic_adc_cb(int (*mic_adc) (void))
{
	return -EACCES;
}

inline int audio_register_codec_id_state_cb(int (*codec_id_state) (void))
{
	return -EACCES;
}
#endif

#endif /* _SEC_AUDIO_SYSFS_H */
