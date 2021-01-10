#ifdef CONFIG_SEC_INPUT_BOOSTER
int chk_next_data(struct evdev_client *dev, int idx, int input_type)
{
	int ret_val = 0;
	int next_type = -1;
	int next_code = -1;

	int next_idx = (idx+1) & (dev->bufsize - 1);

	next_type = dev->buffer[next_idx].type;
	next_code = dev->buffer[next_idx].code;

	switch (input_type) {
	case BTN_TOUCH:
		if (next_type == EV_ABS && next_code == ABS_PRESSURE)
			ret_val = 1;
		break;
	case EV_KEY:
		ret_val = 1;
		break;
	default:
		break;
	}

	return ret_val;
}

int chk_boost_on_off(struct evdev_client *dev, int idx, int dev_type)
{
	int ret_val = -1;

	if (dev_type < 0)
		return ret_val;

	/* In case of SPEN or HOVER, it must be empty multi event
	 * Before starting input booster.
	 */
	if (dev_type == SPEN || dev_type == HOVER) {
		if (!evdev_mt_event[dev_type] && dev->buffer[idx].value)
			ret_val = 1;
		else if (evdev_mt_event[dev_type] && !dev->buffer[idx].value)
			ret_val = 0;
	} else if (dev_type == TOUCH || dev_type == MULTI_TOUCH) {
		if (dev->buffer[idx].value >= 0)
			ret_val = 1;
		else
			ret_val = 0;
	} else if (dev->buffer[idx].value > 0)
		ret_val = 1;
	else if (dev->buffer[idx].value <= 0)
		ret_val = 0;

	return ret_val;
}

/*
 * get_device_type : Define type of device for input_booster.
 * dev : Current device that in which input events triggered.
 * keyId : Each device get given unique keyid using Type, Code, Slot values
 *  to identify which booster will be triggered.
 * cur_idx : Pointing current handling index from input booster.
 *  cur_idx will be updated when cur_idx is same as head.
 * head : End of events set. Input booster handles from tail event to head events.
 *  Must check if the event is the last one referring to head.
 */
int get_device_type(struct evdev_client *dev, unsigned int *keyId, int *cur_idx, int head)
{
	int i;
	int ret_val = -1;
	int dev_type = NONE_TYPE_DEVICE;
	int uniq_slot = 0;
	int next_idx = 0 ;
	int target_idx = 0;

	if (dev == NULL || dev->ev_cnt > MAX_EVENTS) {
		pr_err("evdev client is null and exceed max event number");
		return ret_val;
	}

	/* Initializing device type before finding the proper device type. */
	dev->device_type = dev_type;

	for (i = *cur_idx; i != head; i = (i+1) & (dev->bufsize - 1)) {
		pr_booster("%s Type : %d, Code : %d, Value : %d, Head : %d, idx : %d\n",
			"Input Data || ", dev->buffer[i].type,
			dev->buffer[i].code, dev->buffer[i].value,
			head, i);

		if (dev->buffer[i].type == EV_SYN || dev->buffer[i].code == SYN_REPORT) {
			break;
		}

		if (dev->buffer[i].type == EV_KEY) {
			target_idx = i;
			switch (dev->buffer[i].code) {
			case BTN_TOUCH:
				if (!chk_next_data(dev, i, BTN_TOUCH))
					break;
				dev_type = SPEN;
				break;
			case BTN_TOOL_PEN:
				dev_type = HOVER;
				break;
			case KEY_BACK:
			case KEY_HOMEPAGE:
			case KEY_RECENT:
				dev_type = TOUCH_KEY;
				break;
			case KEY_VOLUMEUP:
			case KEY_VOLUMEDOWN:
			case KEY_POWER:
			case KEY_WINK:
				dev_type = KEY;
				break;
			default:
				break;
			}

		} else if (dev->buffer[i].type == EV_ABS) {
			target_idx = i;
			switch (dev->buffer[i].code) {
			case ABS_MT_TRACKING_ID:

				if (dev->buffer[i].value >= 0) {
					evdev_mt_slot++;
				} else {
					evdev_mt_slot--;
				}

				if (dev->buffer[i].value >= 0) {
					if (evdev_mt_slot == 1) {
						dev_type = TOUCH;
						uniq_slot = 1;
					} else if (evdev_mt_slot == 2) {
						dev_type = MULTI_TOUCH;
						uniq_slot = 2;
					}
				} else if (dev->buffer[i].value < 0) {
					//ret_val = 0;
					if (evdev_mt_slot == 0) {
						dev_type = TOUCH;
						uniq_slot = 1;
					} else if (evdev_mt_slot == 1) {
						dev_type = MULTI_TOUCH;
						uniq_slot = 2;
					}
				}

				pr_booster("Touch Booster Trigger(%d), Type(%d), Code(%d), Val(%d), head(%d), Tail(%d), uniq_slot(%d), Idx(%d), Cnt(%d)",
					evdev_mt_slot, dev->buffer[i].type, dev->buffer[i].code, dev->buffer[i].value, head, dev->tail, uniq_slot, i, dev->ev_cnt);

				break;
			}
		} else if (dev->buffer[i].type == EV_MSC &&
					dev->buffer[i].code == MSC_SCAN) {

			if (!chk_next_data(dev, i, EV_KEY)) {
				break;
			}
			next_idx = (i+1) & (dev->bufsize - 1);
			target_idx = next_idx;
			switch (dev->buffer[next_idx].code) {
			case BTN_LEFT: /* Checking Touch Button Event */
			case BTN_RIGHT:
			case BTN_MIDDLE:
				dev_type = MOUSE;
				//Remain the last of CODE value as a uniq_slot to recognize BTN Type (LEFT, RIGHT, MIDDLE)
				uniq_slot = dev->buffer[next_idx].code;
				break;
			default: /* Checking Keyboard Event */
				dev_type = KEYBOARD;
				uniq_slot = dev->buffer[next_idx].code;

				pr_booster("KBD Booster Trigger(%d), Type(%d), Code(%d), Val(%d), head(%d), Tail(%d), Idx(%d), Cnt(%d)\n",
						dev->buffer[next_idx].code, dev->buffer[i].type,
						dev->buffer[i].code, dev->buffer[i].value,
						head, dev->tail, i, dev->ev_cnt);
				break;
			}
		}

		if (dev_type != NONE_TYPE_DEVICE) {
			*keyId = create_uniq_id(dev->buffer[i].type, dev->buffer[i].code, uniq_slot);
			ret_val = chk_boost_on_off(dev, target_idx, dev_type);
			pr_booster("Dev type Find(%d), KeyID(%d), enable(%d), Target(%d)\n",
				dev_type, *keyId, ret_val, target_idx);
			break;
		}

	}
	// if for loop reach the end, cur_idx is set as head value or pointing next one with plus one.
	// Especially, dev->bufsize has to be set as a 2^n.
	// In the code that set bufzise, we can see bufsize would be set using roundup_pow_of_two function.
	// return roundup_pow_of_two(n_events);
	// which means "A power of two is a number of the form 2n where n is an integer"

	*cur_idx = (i == head) ? head : ((i+1) & (dev->bufsize - 1));

	dev->device_type = dev_type;
	return ret_val;
}

// ********** Detect Events ********** //
void input_booster(struct evdev_client *dev, int dev_head)
{
	int dev_type = 0;
	int keyId = 0;
	unsigned int uniqId = 0;
	int res_type = 0;
	int cnt = 0;
	int cur_idx = -1;
	int head = 0;

	if (dev == NULL) {
		pr_err(ITAG"dev is Null");
		return;
	}

	if (!ib_init_succeed || dev->ev_cnt == 0) {
		pr_err(ITAG"ev_cnt(%d) dt_infor hasn't mem alloc", dev->ev_cnt);
		return;
	}

	head = dev_head;
	cur_idx = (head - dev->ev_cnt) & (dev->bufsize - 1);

	while (cur_idx != head) {
		keyId = 0;
		int enable = get_device_type(dev, &keyId, &cur_idx, head);
		if (enable < 0 || keyId == 0) {
			continue;
		}

		dev_type = dev->device_type;
		if (dev_type <= NONE_TYPE_DEVICE || dev_type >= MAX_DEVICE_TYPE_NUM) {
			continue;
		}

		if (enable == BOOSTER_ON) {
			evdev_mt_event[dev_type]++;
		} else {
			evdev_mt_event[dev_type]--;
		}

		if (cnt == 0 && dev->evdev->handle.dev != NULL) {
			while (dev->evdev->handle.dev->name[cnt] != '\0') {
				ib_trigger[trigger_cnt].dev_name[cnt] = dev->evdev->handle.dev->name[cnt];
				cnt++;
			}
			ib_trigger[trigger_cnt].dev_name[cnt] = '\0';
		}

		pr_booster("Dev Name : %s(%d), Key Id(%d), IB_Cnt(%d)", ib_trigger[trigger_cnt].dev_name, dev_type, keyId, trigger_cnt);

		ib_trigger[trigger_cnt].key_id = keyId;
		ib_trigger[trigger_cnt].event_type = enable;
		ib_trigger[trigger_cnt].dev_type = dev_type;

		queue_work(ev_unbound_wq, &(ib_trigger[trigger_cnt++].ib_trigger_work));
		trigger_cnt = (trigger_cnt == MAX_IB_COUNT) ? 0 : trigger_cnt;
	}
}
#endif //--CONFIG_SEC_INPUT_BOOSTER
