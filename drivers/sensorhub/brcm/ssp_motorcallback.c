#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ssp_motorcallback.h>
#include "ssp.h"

int (*sensorCallback)(int);
int setMotorCallback(int (*callback)(int state))
{
	if (!callback)
		sensorCallback = callback;
	else
		pr_debug("sensor callback is Null !%s\n", __func__);
	return 0;
}
void setSensorCallback(bool flag, int duration)

{
	int current_state = get_current_motor_state();

	if ((flag ==  true && duration >= 20) || (flag == false && current_state)) {
		if (sensorCallback != NULL) {
			sensorCallback(flag);
		} else {
			pr_debug("%s sensorCallback is null.start\n", __func__);
			sensorCallback = getMotorCallback();
			sensorCallback(flag);
		}
	}
}
