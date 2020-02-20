#ifndef _SEC_AUDIO_SYSFS_H
#define _SEC_AUDIO_SYSFS_H

struct sec_audio_sysfs_data {
	struct class *audio_class;

#ifdef CONFIG_SND_SOC_SAMSUNG_SYSFS_EARJACK
	struct device *jack_dev;
	dev_t jack_dev_id;
	int (*get_jack_state)(void);
	int (*get_key_state)(void);
	int (*set_jack_state)(int);
	int (*get_mic_adc)(void);
	int (*get_water_state)(void);
#endif

	struct device *codec_dev;
	dev_t codec_dev_id;
	int (*get_codec_id_state)(void);
};

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
#ifdef CONFIG_SND_SOC_SAMSUNG_SYSFS_EARJACK
int audio_register_jack_select_cb(int (*set_jack) (int));
int audio_register_jack_state_cb(int (*jack_status) (void));
int audio_register_key_state_cb(int (*key_state) (void));
int audio_register_mic_adc_cb(int (*mic_adc) (void));
#endif

int audio_register_codec_id_state_cb(int (*codec_id_state) (void));
#else
#ifdef CONFIG_SND_SOC_SAMSUNG_SYSFS_EARJACK
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
#endif

inline int audio_register_codec_id_state_cb(int (*codec_id_state) (void))
{
	return -EACCES;
}
#endif
#endif /* _SEC_AUDIO_SYSFS_H */
