#if !defined(CONFIG_INPUT_BOOSTER) // Input Booster +
#ifndef _INPUT_BOOSTER_H_
#define _INPUT_BOOSTER_H_

#include <linux/pm_qos.h>
#include <linux/of.h>
#include <linux/exynos-ucc.h>

#ifdef CONFIG_SCHED_HMP
#define USE_HMP_BOOST
#elif defined CONFIG_SCHED_EMS
#define USE_EHMP_BOOST
#endif

#define pr_booster(format, ...) { \
	if (debug_flag) \
		printk(format, ## __VA_ARGS__); \
}

#define MAX_MULTI_TOUCH_EVENTS		3
#define MAX_EVENTS			(MAX_MULTI_TOUCH_EVENTS * 10)

#define INPUT_BOOSTER_NULL	0
#define INIT_ZERO	0

#define HEADGAGE "******"
#define TAILGAGE "****  "

#define set_qos(req, pm_qos_class, value) { \
	if (value) { \
		if (pm_qos_request_active(req)) {\
			pr_booster("[Input Booster2] %s      pm_qos_update_request : %d\n", glGage, value); \
			pm_qos_update_request(req, value); \
		} else { \
			pr_booster("[Input Booster2] %s      pm_qos_add_request : %d\n", glGage, value); \
			pm_qos_add_request(req, pm_qos_class, value); \
		} \
	} else { \
		pr_booster("[Input Booster2] %s      remove_qos\n", glGage); \
		remove_qos(req); \
	} \
}

#define remove_qos(req) { \
	if (pm_qos_request_active(req)) \
		pm_qos_remove_request(req); \
}

#ifdef USE_HMP_BOOST
#define set_hmp(enable)	 { \
	if (enable != current_hmp_boost) { \
		pr_booster("[Input Booster2] ******      set_hmp : %d ( %s )\n", enable, __FUNCTION__); \
		if (set_hmp_boost(enable) < 0) { \
			pr_booster("[Input Booster2] ******            !!! fail to HMP !!!\n"); \
		} \
		current_hmp_boost = enable; \
	} \
}
#elif defined USE_EHMP_BOOST
#include <linux/ems_service.h>

static DEFINE_MUTEX(input_lock);
int hmp_boost_value = INIT_ZERO;

static struct kpp kpp_ta;
static struct kpp kpp_fg;

#define set_hmp(enable) { \
	mutex_lock(&input_lock); \
	if (enable != current_hmp_boost) { \
		if (hmp_boost_value <= 0 && !enable) { \
			pr_booster("[Input Booster2] ******      ERROR : set_ehmp unexpected disable request happened ( %s )\n", __FUNCTION__); \
		} else if (hmp_boost_value >= 1 && enable) { \
			pr_booster("[Input Booster2] ******      ERROR : set_ehmp unexpected enable request happened ( %s )\n", __FUNCTION__); \
		} else { \
			pr_booster("[Input Booster2] ******      set_ehmp : %d ( %s )\n", enable, __FUNCTION__); \
			if (enable) { \
				hmp_boost_value++; \
				kpp_request(STUNE_TOPAPP, &kpp_ta, enable); \
				kpp_request(STUNE_FOREGROUND, &kpp_fg, enable); \
			} else { \
				hmp_boost_value--; \
				kpp_request(STUNE_TOPAPP, &kpp_ta, 0); \
				kpp_request(STUNE_FOREGROUND, &kpp_fg, 0); \
			} \
			current_hmp_boost = enable; \
		} \
	} \
	mutex_unlock(&input_lock); \
}
#else
#define set_hmp(enable)
#endif

#ifdef CONFIG_SOC_EXYNOS9820
static struct ucc_req ucc_request =
{
	.name = "input",
};
int ucc_boost_value = INIT_ZERO;

#define set_ucc(enable) { \
	mutex_lock(&input_lock); \
	if (enable != current_ucc_boost) { \
		if (ucc_boost_value <= 0 && !enable) { \
			pr_booster("[Input Booster2] ******      ERROR : set_ucc unexpected disable request happened ( %s )\n", __FUNCTION__); \
		} else if (ucc_boost_value >= 1 && enable) { \
			pr_booster("[Input Booster2] ******      ERROR : set_ucc unexpected enable request happened ( %s )\n", __FUNCTION__); \
		} else { \
			pr_booster("[Input Booster2] ******      set_ucc : %d ( %s )\n", enable, __FUNCTION__); \
			if (enable) { \
				ucc_boost_value++; \
				ucc_add_request(&ucc_request, enable); \
			} else { \
				ucc_boost_value--; \
				ucc_remove_request(&ucc_request);\
			} \
			current_ucc_boost = enable; \
		} \
	} \
	mutex_unlock(&input_lock); \
}
#else
#define set_ucc(enable)
#endif

#if defined(CONFIG_ARCH_EXYNOS) //______________________________________________________________________________
#define SET_BOOSTER  { \
	int value = INPUT_BOOSTER_NULL; \
	int uccvalue = INPUT_BOOSTER_NULL; \
	_this->level++; \
	MAX_T_INPUT_BOOSTER(value, hmp_boost); \
	MAX_T_INPUT_BOOSTER(uccvalue, ucc_requested_val); \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	if (uccvalue == INPUT_BOOSTER_NULL) { \
		uccvalue = 0; \
	} \
	set_hmp(value); \
	set_ucc(uccvalue); \
	set_qos(&_this->cpu2_qos, PM_QOS_CLUSTER2_FREQ_MIN/*PM_QOS_CPU_FREQ_MIN*/, _this->param[_this->index].cpu2_freq);  \
	set_qos(&_this->cpu1_qos, PM_QOS_CLUSTER1_FREQ_MIN/*PM_QOS_CPU_FREQ_MIN*/, _this->param[_this->index].cpu1_freq);  \
	set_qos(&_this->kfc_qos, PM_QOS_CLUSTER0_FREQ_MIN/*PM_QOS_KFC_FREQ_MIN*/, _this->param[_this->index].kfc_freq);  \
	set_qos(&_this->mif_qos, PM_QOS_BUS_THROUGHPUT, _this->param[_this->index].mif_freq);  \
	set_qos(&_this->int_qos, PM_QOS_DEVICE_THROUGHPUT, _this->param[_this->index].int_freq);  \
}
#define REMOVE_BOOSTER  { \
	int value = INPUT_BOOSTER_NULL; \
	int uccvalue = INPUT_BOOSTER_NULL; \
	_this->level = -1; \
	MAX_T_INPUT_BOOSTER(value, hmp_boost); \
	MAX_T_INPUT_BOOSTER(uccvalue, ucc_requested_val); \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	if (uccvalue == INPUT_BOOSTER_NULL) { \
		uccvalue = 0; \
	} \
	set_hmp(value); \
	set_ucc(uccvalue); \
	remove_qos(&_this->cpu2_qos);  \
	remove_qos(&_this->cpu1_qos);  \
	remove_qos(&_this->kfc_qos);  \
	remove_qos(&_this->mif_qos);  \
	remove_qos(&_this->int_qos);  \
}
#define PROPERTY_BOOSTER(_device_param_, _dt_param_, _time_)  { \
	_device_param_.cpu2_freq = _dt_param_.cpu2_freq; \
	_device_param_.cpu1_freq = _dt_param_.cpu1_freq; \
	_device_param_.kfc_freq = _dt_param_.kfc_freq; \
	_device_param_.mif_freq = _dt_param_.mif_freq; \
	_device_param_.int_freq = _dt_param_.int_freq; \
	_device_param_.time = _dt_param_._time_; \
	_device_param_.hmp_boost = _dt_param_.hmp_boost; \
	_device_param_.ucc_requested_val = _dt_param_.ucc_requested_val; \
}
#endif //______________________________________________________________________________
#define GET_BOOSTER_PARAM(_GENDER_, _HEAD_PARAM_, _TAIL_PARAM_) { \
	int levels[][3] = { \
		{1, 2, 0}, \
		{2, 2, 3}, \
		{3, 1, 1}, \
		{4, 1, 2} }; \
	int j, k; \
	for (j = 0; j < (int)(sizeof(levels)/(3*sizeof(int))); j++) {\
		if ((_GENDER_->pDT->nlevels > 2 && levels[j][0] == _GENDER_->level) || (_GENDER_->pDT->nlevels == 1 && j == 2) || (_GENDER_->pDT->nlevels == 2 && j == 3)) { \
			if (levels[j][1] > 0) { \
				for (k = 0; k < _GENDER_->pDT->nlevels; k++) { \
					if (levels[j][1] == _GENDER_->pDT->param_tables[k].ilevels) { \
						_HEAD_PARAM_ = (_GENDER_->pDT->param_tables[k].head_time > 0) ? &_GENDER_->pDT->param_tables[k] : NULL; \
						break; \
					} \
				} \
			} \
			if (levels[j][2] > 0) { \
				for (k = 0; k < dt_gender->pDT->nlevels; k++) { \
					if (levels[j][2] == dt_gender->pDT->param_tables[k].ilevels) { \
						_TAIL_PARAM_ = &_GENDER_->pDT->param_tables[k]; \
						break; \
					} \
				} \
			} \
			break; \
		} \
	} \
}
#define CHANGE_BOOSTER { \
	struct t_input_booster_device_tree_param *head_param = NULL, *tail_param = NULL; \
	GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
	memset(dt_gender->pBooster->param, 0x00, sizeof(struct t_input_booster_param)*2); \
	if (head_param != NULL) { \
		PROPERTY_BOOSTER(dt_gender->pBooster->param[0], (*head_param), head_time) \
	} \
	if (tail_param != NULL) { \
		PROPERTY_BOOSTER(dt_gender->pBooster->param[1], (*tail_param), tail_time) \
	} \
}

#define INIT_BOOSTER(_DEVICE_) { \
	_DEVICE_##_booster.input_booster_state = input_booster_idle_state; \
	INIT_DELAYED_WORK(&_DEVICE_##_booster.input_booster_timeout_work[0], TIMEOUT_FUNC(_DEVICE_)); \
	INIT_DELAYED_WORK(&_DEVICE_##_booster.input_booster_timeout_work[1], TIMEOUT_FUNC(_DEVICE_)); \
	INIT_WORK(&_DEVICE_##_booster.input_booster_set_booster_work, SET_BOOSTER_FUNC(_DEVICE_)); \
	INIT_WORK(&_DEVICE_##_booster.input_booster_reset_booster_work, RESET_BOOSTER_FUNC(_DEVICE_)); \
	mutex_init(&_DEVICE_##_booster.lock); \
	_DEVICE_##_booster.change_on_release = 0; \
	_DEVICE_##_booster.multi_events = 0; \
	{ \
		int i; \
		for (i = 0; i < sizeof(_DEVICE_##_booster.param)/sizeof(struct t_input_booster_param); i++) { \
			_DEVICE_##_booster.level = -1; \
		} \
		for (i = 0; i < ndevice_in_dt; i++) { \
			if (device_tree_infor[i].type == _DEVICE_##_booster_dt.type) { \
				struct t_input_booster_device_tree_gender *dt_gender = &_DEVICE_##_booster_dt; \
				dt_gender->pDT = &device_tree_infor[i]; \
				dt_gender->pBooster = &_DEVICE_##_booster; \
				CHANGE_BOOSTER \
				break; \
			} \
		} \
	} \
}

#define TIMEOUT_FUNC(_DEVICE_) input_booster_##_DEVICE_##_timeout_work_func

#define DECLARE_TIMEOUT_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_timeout_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = &_DEVICE_##_booster; \
	int param_max = sizeof(_this->param)/sizeof(struct t_input_booster_param), temp_index = -1;  \
	mutex_lock(&_this->lock); \
	pr_booster("[Input Booster] %s           Timeout : changed  index : %d (%s)\n", HEADGAGE, _this->index, __FUNCTION__); \
	if (_this->index >= 2 && delayed_work_pending(&_this->input_booster_timeout_work[_this->index-2])) { \
		mutex_unlock(&_this->lock); \
		return; \
	} \
	if (_this->index == param_max && delayed_work_pending(&_this->input_booster_timeout_work[_this->index-1])) { \
		temp_index = _this->index; \
		_this->index = (_this->index) ? _this->index-1 : 0; \
	} \
	pr_booster("[Input Booster] %s           Timeout : changed  index : %d (%s)\n", HEADGAGE, _this->index, __FUNCTION__); \
	if (_this->index < param_max) { \
		pr_booster("[Input Booster] %s           Timeout : changed  index : %d, time : %d (%s)\n", HEADGAGE, _this->index, _this->param[_this->index].time, __FUNCTION__); \
		pr_booster("[Input Booster] %s           hmp : %d  ucc_requested_val : %d cpu2 : %d cpu1 : %d (%s)\n", TAILGAGE, _this->param[_this->index].hmp_boost, _this->param[_this->index].ucc_requested_val, _this->param[_this->index].cpu2_freq, _this->param[_this->index].cpu1_freq, __FUNCTION__); \
		if (_this->param[(_this->index) ? _this->index-1 : 0].time > 0) { \
			SET_BOOSTER; \
			if (_this->change_on_release) { \
				schedule_delayed_work(&_this->input_booster_timeout_work[_this->index], msecs_to_jiffies(_this->param[_this->index].time)); \
				_this->index++; \
				CHANGE_STATE_TO(idle); \
			} \
		} \
		_this->index = (temp_index >= 0) ? temp_index : _this->index; \
	} else { \
		pr_booster("[Input Booster] Timeout : completed   param_max : %d (%s)\n", param_max, __FUNCTION__); \
		pr_booster("[Input Booster]\n"); \
		REMOVE_BOOSTER; \
		_this->index = 0; \
		_this->multi_events = (_this->multi_events > 0) ? 0 : _this->multi_events; \
		CHANGE_STATE_TO(idle); \
	} \
	mutex_unlock(&_this->lock); \
}

#define SET_BOOSTER_FUNC(_DEVICE_) input_booster_##_DEVICE_##_set_booster_work_func

#define DECLARE_SET_BOOSTER_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_set_booster_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = (struct t_input_booster *)(&_DEVICE_##_booster); \
	mutex_lock(&_this->lock); \
	_this->input_booster_state(_this, _this->event_type); \
	mutex_unlock(&_this->lock); \
}

#define RESET_BOOSTER_FUNC(_DEVICE_) input_booster_##_DEVICE_##_reset_booster_work_func

#define DECLARE_RESET_BOOSTER_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_reset_booster_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = (struct t_input_booster *)(&_DEVICE_##_booster); \
	int i; \
	mutex_lock(&_this->lock); \
	_this->multi_events = 0; \
	_this->index = 0; \
	for (i = 0; i < 2; i++) { \
		if (delayed_work_pending(&_this->input_booster_timeout_work[i])) { \
			pr_booster("[Input Booster] ****             cancel the pending workqueue for reset\n"); \
			cancel_delayed_work(&_this->input_booster_timeout_work[i]); \
		} \
	} \
	CHANGE_STATE_TO(idle); \
	REMOVE_BOOSTER; \
	mutex_unlock(&_this->lock); \
}

#define DECLARE_STATE_FUNC(_STATE_) void input_booster_##_STATE_##_state(void *__this, int input_booster_event)

#define CHANGE_STATE_TO(_STATE_) {_this->input_booster_state = input_booster_##_STATE_##_state; }

#define RUN_BOOSTER(_DEVICE_, _EVENT_) { \
	if (_DEVICE_##_booster_dt.level > 0) { \
		_DEVICE_##_booster.event_type = _EVENT_; \
		(_EVENT_ == BOOSTER_ON)  ? _DEVICE_##_booster.multi_events++ : _DEVICE_##_booster.multi_events--; \
		schedule_work(&_DEVICE_##_booster.input_booster_set_booster_work); \
	} \
}

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR SYSFS  +++++++++++++++++++++++++++++++++++++++++++++++//
#define SYSFS_CLASS(_ATTR_, _ARGU_, _COUNT_) \
	static ssize_t input_booster_sysfs_class_show_##_ATTR_(struct class *dev, struct class_attribute *attr, char *buf) \
	{ \
		struct t_input_booster_device_tree_gender *dt_gender = &touch_booster_dt; \
		ssize_t ret; int level; \
		unsigned int debug_level = 0, cpu2_freq = 0, cpu1_freq = 0, kfc_freq = 0, mif_freq = 0, int_freq = 0, hmp_boost = 0, ucc_requested_val = 0, head_time = 0, tail_time = 0; \
		struct t_input_booster_device_tree_param *head_param = NULL, *tail_param = NULL; \
		GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
		debug_level = debug_flag; \
		level = dt_gender->level; \
		if (strcmp(#_ATTR_, "head") == 0 && head_param != NULL) { \
			cpu2_freq = head_param->cpu2_freq; \
			cpu1_freq = head_param->cpu1_freq; \
			kfc_freq = head_param->kfc_freq; \
			mif_freq = head_param->mif_freq; \
			int_freq = head_param->int_freq; \
			hmp_boost = head_param->hmp_boost; \
			ucc_requested_val = head_param->ucc_requested_val; \
			head_time = head_param->head_time; \
			tail_time = head_param->tail_time; \
		} \
		if (strcmp(#_ATTR_, "tail") == 0 && tail_param != NULL) { \
			cpu2_freq = tail_param->cpu2_freq; \
			cpu1_freq = tail_param->cpu1_freq; \
			kfc_freq = tail_param->kfc_freq; \
			mif_freq = tail_param->mif_freq; \
			int_freq = tail_param->int_freq; \
			hmp_boost = tail_param->hmp_boost; \
			ucc_requested_val = tail_param->ucc_requested_val; \
			head_time = tail_param->head_time; \
			tail_time = tail_param->tail_time; \
		} \
		ret = sprintf _ARGU_; \
		pr_booster("[Input Booster8] %s buf : %s\n", __FUNCTION__, buf); \
		return ret; \
	} \
	static ssize_t input_booster_sysfs_class_store_##_ATTR_(struct class *dev, struct class_attribute *attr, const char *buf, size_t count) \
	{ \
		struct t_input_booster_device_tree_gender *dt_gender = &touch_booster_dt; \
		int level[1] = {-1}, len; \
		unsigned int debug_level[1] = {-1}, cpu2_freq[1] = {-1}, cpu1_freq[1] = {-1}, kfc_freq[1] = {-1}, mif_freq[1] = {-1}, int_freq[1] = {-1}, hmp_boost[1] = {-1}, ucc_requested_val[1] = {-1}, head_time[1] = {-1}, tail_time[1] = {-1}; \
		struct t_input_booster_device_tree_param *head_param = NULL, *tail_param = NULL; \
		GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
		len = sscanf _ARGU_; \
		pr_booster("[Input Booster8] %s buf : %s\n", __FUNCTION__, buf); \
		if (sscanf _ARGU_ != _COUNT_) { \
			return count; \
		} \
		debug_flag = (*debug_level == (unsigned int)(-1)) ? debug_flag : *debug_level; \
		dt_gender->level = (*level == (unsigned int)(-1)) ? dt_gender->level : *level; \
		if (*head_time != (unsigned int)(-1) && head_param != NULL) { \
			head_param->cpu2_freq = (*cpu2_freq == (unsigned int)(-1)) ? head_param->cpu2_freq : *cpu2_freq; \
			head_param->cpu1_freq = (*cpu1_freq == (unsigned int)(-1)) ? head_param->cpu1_freq : *cpu1_freq; \
			head_param->kfc_freq = (*kfc_freq == (unsigned int)(-1)) ? head_param->kfc_freq : *kfc_freq; \
			head_param->mif_freq = (*mif_freq == (unsigned int)(-1)) ? head_param->mif_freq : *mif_freq; \
			head_param->int_freq = (*int_freq == (unsigned int)(-1)) ? head_param->int_freq : *int_freq; \
			head_param->hmp_boost = (*hmp_boost == (unsigned int)(-1)) ? head_param->hmp_boost : *hmp_boost; \
			head_param->ucc_requested_val = (*ucc_requested_val == (unsigned int)(-1)) ? head_param->ucc_requested_val : *ucc_requested_val; \
			head_param->head_time = (*head_time == (unsigned int)(-1)) ? head_param->head_time : *head_time; \
			head_param->tail_time = (*tail_time == (unsigned int)(-1)) ? head_param->tail_time : *tail_time; \
		} \
		if (*tail_time != (unsigned int)(-1) && tail_param != NULL) { \
			tail_param->cpu2_freq = (*cpu2_freq == (unsigned int)(-1)) ? tail_param->cpu2_freq : *cpu2_freq; \
			tail_param->cpu1_freq = (*cpu1_freq == (unsigned int)(-1)) ? tail_param->cpu1_freq : *cpu1_freq; \
			tail_param->kfc_freq = (*kfc_freq == (unsigned int)(-1)) ? tail_param->kfc_freq : *kfc_freq; \
			tail_param->mif_freq = (*mif_freq == (unsigned int)(-1)) ? tail_param->mif_freq : *mif_freq; \
			tail_param->int_freq = (*int_freq == (unsigned int)(-1)) ? tail_param->int_freq : *int_freq; \
			tail_param->hmp_boost = (*hmp_boost == (unsigned int)(-1)) ? tail_param->hmp_boost : *hmp_boost; \
			tail_param->ucc_requested_val = (*ucc_requested_val == (unsigned int)(-1)) ? tail_param->ucc_requested_val : *ucc_requested_val; \
			tail_param->head_time = (*head_time == (unsigned int)(-1)) ? tail_param->head_time : *head_time; \
			tail_param->tail_time = (*tail_time == (unsigned int)(-1)) ? tail_param->tail_time : *tail_time; \
		} \
		CHANGE_BOOSTER \
		return count; \
	} \
	static struct class_attribute class_attr_##_ATTR_ = __ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_class_show_##_ATTR_, input_booster_sysfs_class_store_##_ATTR_);
#define SYSFS_DEVICE(_ATTR_, _ARGU_, _COUNT_) \
	static ssize_t input_booster_sysfs_device_show_##_ATTR_(struct device *dev, struct device_attribute *attr, char *buf) \
	{ \
		struct t_input_booster_device_tree_gender *dt_gender = dev_get_drvdata(dev); \
		ssize_t ret = 0; \
		int level, Arg_count = _COUNT_; \
		unsigned int cpu2_freq, cpu1_freq, kfc_freq, mif_freq, int_freq, hmp_boost, ucc_requested_val, head_time, tail_time, phase_time; \
		struct t_input_booster_device_tree_param *head_param = NULL, *tail_param = NULL; \
		if (dt_gender == NULL) { \
			return  ret; \
		} \
		GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
		if (Arg_count == 1) { \
			level = dt_gender->level; \
			ret = sprintf _ARGU_; \
			pr_booster("[Input Booster8] %s buf : %s\n", __FUNCTION__, buf); \
		} else { \
			if (head_param != NULL) { \
				level = head_param->ilevels; \
				cpu2_freq = head_param->cpu2_freq; \
				cpu1_freq = head_param->cpu1_freq; \
				kfc_freq = head_param->kfc_freq; \
				mif_freq = head_param->mif_freq; \
				int_freq = head_param->int_freq; \
				hmp_boost = head_param->hmp_boost; \
				ucc_requested_val = head_param->ucc_requested_val; \
				head_time = head_param->head_time; \
				tail_time = head_param->tail_time; \
				phase_time = head_param->phase_time; \
				ret = sprintf _ARGU_; \
				buf = buf + ret - 1; \
			} \
			*buf = '|'; \
			*(buf+1) = '\n';\
			if (tail_param != NULL) { \
				buf = buf + 1; \
				level = tail_param->ilevels; \
				cpu2_freq = tail_param->cpu2_freq; \
				cpu1_freq = tail_param->cpu1_freq; \
				kfc_freq = tail_param->kfc_freq; \
				mif_freq = tail_param->mif_freq; \
				int_freq = tail_param->int_freq; \
				hmp_boost = tail_param->hmp_boost; \
				ucc_requested_val = tail_param->ucc_requested_val; \
				head_time = tail_param->head_time; \
				tail_time = tail_param->tail_time; \
				phase_time = tail_param->phase_time; \
				ret += sprintf _ARGU_; \
			} \
			pr_booster("[Input Booster8] %s buf : %s\n", __FUNCTION__, buf); \
		} \
		return  ret; \
	} \
	static ssize_t input_booster_sysfs_device_store_##_ATTR_(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) \
	{ \
		struct t_input_booster_device_tree_gender *dt_gender = dev_get_drvdata(dev); \
		struct t_input_booster_device_tree_infor *dt_infor = (dt_gender) ? dt_gender->pDT : NULL; \
		int level[1] = {-1}, len; \
		unsigned int cpu2_freq[1] = {-1}, cpu1_freq[1] = {-1}, kfc_freq[1] = {-1}, mif_freq[1] = {-1}, int_freq[1] = {-1}, hmp_boost[1] = {-1}, ucc_requested_val[1] = {-1}, head_time[1] = {-1}, tail_time[1] = {-1}, phase_time[1] = {-1}; \
		len = sscanf _ARGU_; \
		pr_booster("[Input Booster8] %s buf : %s\n", __FUNCTION__, buf); \
		if (dt_infor == NULL) { \
			return  count; \
		} \
		if (len != _COUNT_) { \
			pr_booster("### Keep this format : [level cpu2_freq cpu1_freq kfc_freq mif_freq int_freq hmp_boost ucc_requested_val] (Ex: 1 1600000 950000 0 1500000 667000 333000 1###\n"); \
			pr_booster("### Keep this format : [level head_time tail_time phase_time] (Ex: 1 130 500 50 ###\n"); \
			pr_booster("### Keep this format : [type value] (Ex: 2 1 ###\n"); \
			return count; \
		} \
		if (level[0] >= 0) { \
			int Arg_count = _COUNT_; \
			if (Arg_count == 1) { \
				dt_gender->level = level[0]; \
			} else { \
				int k; \
				for (k = 0; k < dt_infor->nlevels; k++) { \
					if (level[0] == dt_infor->param_tables[k].ilevels) { \
						dt_infor->param_tables[k].cpu2_freq = (*cpu2_freq == (unsigned int)(-1)) ? dt_infor->param_tables[k].cpu2_freq : *cpu2_freq; \
						dt_infor->param_tables[k].cpu1_freq = (*cpu1_freq == (unsigned int)(-1)) ? dt_infor->param_tables[k].cpu1_freq : *cpu1_freq; \
						dt_infor->param_tables[k].kfc_freq = (*kfc_freq == (unsigned int)(-1)) ? dt_infor->param_tables[k].kfc_freq : *kfc_freq; \
						dt_infor->param_tables[k].mif_freq = (*mif_freq == (unsigned int)(-1)) ? dt_infor->param_tables[k].mif_freq : *mif_freq; \
						dt_infor->param_tables[k].int_freq = (*int_freq == (unsigned int)(-1)) ? dt_infor->param_tables[k].int_freq : *int_freq; \
						dt_infor->param_tables[k].hmp_boost = (*hmp_boost == (unsigned int)(-1)) ? dt_infor->param_tables[k].hmp_boost : *hmp_boost; \
						dt_infor->param_tables[k].ucc_requested_val = (*ucc_requested_val == (unsigned int)(-1)) ? dt_infor->param_tables[k].ucc_requested_val : *ucc_requested_val; \
						dt_infor->param_tables[k].head_time = (*head_time == (unsigned int)(-1)) ? dt_infor->param_tables[k].head_time : *head_time; \
						dt_infor->param_tables[k].tail_time = (*tail_time == (unsigned int)(-1)) ? dt_infor->param_tables[k].tail_time : *tail_time; \
						dt_infor->param_tables[k].phase_time = (*phase_time == (unsigned int)(-1)) ? dt_infor->param_tables[k].phase_time : *phase_time; \
						pr_booster("[Input Booster8] %s time : %d %d %d\n", __FUNCTION__, dt_infor->param_tables[*level].head_time, dt_infor->param_tables[k].tail_time, dt_infor->param_tables[*level].phase_time); \
					} \
				} \
			} \
			CHANGE_BOOSTER \
		} \
		return count; \
	} \
	static DEVICE_ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_device_show_##_ATTR_, input_booster_sysfs_device_store_##_ATTR_);
#define INIT_SYSFS_CLASS(_CLASS_) { \
		int ret = class_create_file(sysfs_class, &class_attr_##_CLASS_); \
		if (ret) { \
			pr_booster("[Input Booster] Failed to create class\n"); \
			class_destroy(sysfs_class); \
			return; \
		} \
	}
#define INIT_SYSFS_DEVICE(_DEVICE_) { \
		struct device   *sysfs_dev; int ret = 0;\
		sysfs_dev = device_create(sysfs_class, NULL, 0, &_DEVICE_##_booster_dt, #_DEVICE_); \
		if (IS_ERR(sysfs_dev)) { \
			ret = IS_ERR(sysfs_dev); \
			pr_booster("[Input Booster] Failed to create %s sysfs device[%d]\n", #_DEVICE_, ret); \
			return; \
		} \
		ret = sysfs_create_group(&sysfs_dev->kobj, &dvfs_attr_group); \
		if (ret) { \
			pr_booster("[Input Booster] Failed to create %s sysfs group\n", #_DEVICE_); \
			return; \
		} \
	}
//-----------------------------------------------  STRUCT & VARIABLE FOR SYSFS  -----------------------------------------------//

enum booster_mode_on_off {
	BOOSTER_OFF = 0,
	BOOSTER_ON,
};


struct input_value input_events[MAX_EVENTS+1];

struct t_input_booster_param {
	u32 cpu2_freq;
	u32 cpu1_freq;
	u32 kfc_freq;
	u32 mif_freq;
	u32 int_freq;

	u16 time;

	u8 hmp_boost;
	u8 ucc_requested_val;
	u8 dummy;
};

struct t_input_booster {
	struct mutex lock;
	struct t_input_booster_param param[2];

	struct pm_qos_request	cpu2_qos;
	struct pm_qos_request	cpu1_qos;
	struct pm_qos_request	kfc_qos;
	struct pm_qos_request	mif_qos;
	struct pm_qos_request	int_qos;

	struct delayed_work     input_booster_timeout_work[2];
	struct work_struct      input_booster_set_booster_work;
	struct work_struct      input_booster_reset_booster_work;

	int index;
	int multi_events;
	int event_type;
	int change_on_release;
	int level;

	void (*input_booster_state)(void *__this, int input_booster_event);
};

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR DEVICE TREE  +++++++++++++++++++++++++++++++++++++++++++++++//
struct t_input_booster_device_tree_param {
	u8      ilevels;

	u8      hmp_boost;
	u8      ucc_requested_val;

	u16     head_time;
	u16     tail_time;
	u16     phase_time;

	u32     cpu2_freq;
	u32     cpu1_freq;
	u32     kfc_freq;
	u32     mif_freq;
	u32     int_freq;
};

struct t_input_booster_device_tree_infor {
	const char     *label;
	int     type;
	int     nlevels;

	struct t_input_booster_device_tree_param      *param_tables;
};

struct t_input_booster_device_tree_gender {
	int type;
	int level;
	struct t_input_booster	*pBooster;
	struct t_input_booster_device_tree_infor *pDT;
};

//______________________________________________________________________________	<<< in DTSI file >>>
//______________________________________________________________________________	input_booster,type = <4>;	/* BOOSTER_DEVICE_KEYBOARD */
//______________________________________________________________________________
struct t_input_booster_device_tree_gender	key_booster_dt = {0, 1,};			// type : 0,  level : 1
struct t_input_booster_device_tree_gender	touchkey_booster_dt = {1, 1,};		// type : 1,  level : 1
struct t_input_booster_device_tree_gender	touch_booster_dt = {2, 2,};			// type : 2,  level : 2
struct t_input_booster_device_tree_gender	multitouch_booster_dt = {3, 1,};		// type : 3,  level : 1
struct t_input_booster_device_tree_gender	keyboard_booster_dt = {4, 1,};		// type : 4,  level : 1
struct t_input_booster_device_tree_gender	mouse_booster_dt = {5, 1,};			// type : 5,  level : 1
struct t_input_booster_device_tree_gender	mouse_wheel_booster_dt = {6, 1,};	// type : 6,  level : 1
struct t_input_booster_device_tree_gender	hover_booster_dt = {7, 1,};			// type : 7,  level : 1
struct t_input_booster_device_tree_gender	pen_booster_dt = {8, 1,};			// type : 8,  level : 1
struct t_input_booster_device_tree_gender	key_two_booster_dt = {9, 1,};			// type : 9,  level : 1
struct t_input_booster_device_tree_infor	*device_tree_infor;

int ndevice_in_dt;
//----------------------------------------------  STRUCT & VARIABLE FOR DEVICE TREE  ----------------------------------------------//

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR SYSFS  +++++++++++++++++++++++++++++++++++++++++++++++//
unsigned int debug_flag = INIT_ZERO;

SYSFS_CLASS(debug_level, (buf, "%u\n", debug_level), 1)
SYSFS_CLASS(head, (buf, "%d %u %u %u %u %u %u %u\n", head_time, cpu2_freq, cpu1_freq, kfc_freq, mif_freq, int_freq, hmp_boost, ucc_requested_val), 8)
SYSFS_CLASS(tail, (buf, "%d %u %u %u %u %u %u %u\n", tail_time, cpu2_freq, cpu1_freq, kfc_freq, mif_freq, int_freq, hmp_boost, ucc_requested_val), 8)
SYSFS_CLASS(level, (buf, "%d\n", level), 1)
SYSFS_DEVICE(level, (buf, "%d\n", level), 1)
SYSFS_DEVICE(freq, (buf, "%d %u %u %u %u %u %u %u\n", level, cpu2_freq, cpu1_freq, kfc_freq, mif_freq, int_freq, hmp_boost, ucc_requested_val), 8)
SYSFS_DEVICE(time, (buf, "%d %u %u %u\n", level, head_time, tail_time, phase_time), 4)
static ssize_t input_booster_sysfs_device_store_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct t_input_booster_device_tree_gender *dt_gender = dev_get_drvdata(dev);
	struct t_input_booster *dt_booster = (dt_gender) ? dt_gender->pBooster : NULL;
	int value;
	unsigned int type;

	if (dt_booster == NULL) {
		return count;
	}
	if (sscanf(buf, "%u %d", &type, &value) != 2) {
		pr_booster("### Keep this format : [type value] (Ex: 2 1 ###\n");
		return count;
	}
	dt_booster->event_type = value;
	schedule_work(&dt_booster->input_booster_set_booster_work);

	return count;
}
static DEVICE_ATTR(control, S_IRUGO | S_IWUSR, NULL, input_booster_sysfs_device_store_control);

static struct attribute *dvfs_attributes[] = {
	&dev_attr_level.attr,
	&dev_attr_freq.attr,
	&dev_attr_time.attr,
	&dev_attr_control.attr,
	NULL,
};

static struct attribute_group dvfs_attr_group = {
	.attrs = dvfs_attributes,
};

//----------------------------------------------  STRUCT & VARIABLE FOR SYSFS  ----------------------------------------------//

int TouchIDs[MAX_MULTI_TOUCH_EVENTS];
char *glGage = HEADGAGE;
bool current_hmp_boost = INIT_ZERO;
bool current_ucc_boost = INIT_ZERO;

struct t_input_booster	touch_booster;
struct t_input_booster	multitouch_booster;
struct t_input_booster	key_booster;
struct t_input_booster	touchkey_booster;
struct t_input_booster	keyboard_booster;
struct t_input_booster	mouse_booster;
struct t_input_booster	mouse_wheel_booster;
struct t_input_booster	pen_booster;
struct t_input_booster	hover_booster;
struct t_input_booster	key_two_booster;

struct t_input_booster *t_input_boosters[] = {
	&touch_booster,
	&multitouch_booster,
	&key_booster,
	&touchkey_booster,
	&keyboard_booster,
	&mouse_booster,
	&mouse_wheel_booster,
	&pen_booster,
	&hover_booster,
	&key_two_booster
};

#define MAX_T_INPUT_BOOSTER(ref, _PARAM_) { \
		size_t i = 0; \
		int max = INPUT_BOOSTER_NULL; \
		for (i = 0; i < sizeof(t_input_boosters)/sizeof(struct t_input_booster *); i++) { \
			if (t_input_boosters[i]->level >= 0 && t_input_boosters[i]->level < (int)(sizeof(t_input_boosters[i]->param)/sizeof(struct t_input_booster_param))) { \
				pr_booster("[Input Booster3] %s booster type : %lu    level : %d    value : %d\n", #_PARAM_, i, t_input_boosters[i]->level, t_input_boosters[i]->param[t_input_boosters[i]->level]._PARAM_); \
				if (max < (int)(t_input_boosters[i]->param[t_input_boosters[i]->level]._PARAM_)) { \
					max = (int)(t_input_boosters[i]->param[t_input_boosters[i]->level]._PARAM_); \
				} \
			} \
		} \
		if (max == INPUT_BOOSTER_NULL) { \
			ref = INPUT_BOOSTER_NULL; \
		} else { \
			ref = max; \
		} \
		pr_booster("[Input Booster3] %s max value : %d\n", #_PARAM_, max); \
	} \

int input_count = 0, key_back = 0, key_home = 0, key_recent = 0;

void input_booster_idle_state(void *__this, int input_booster_event);
void input_booster_press_state(void *__this, int input_booster_event);
void input_booster(struct input_dev *dev);
void input_booster_init(void);
#endif
#endif // Input Booster -



