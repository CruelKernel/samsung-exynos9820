#ifndef SSP_MOTORCALLBACK_H
#define SSP_MOTORCALLBACK_H
int (*getMotorCallback(void))(int);
int setMotorCallback(int (*callback)(int state));

void setSensorCallback(bool flag, int duration);
int setMotorCallback(int (*callback)(int state));
#endif  // SSP_MOTORCALLBACK_H
