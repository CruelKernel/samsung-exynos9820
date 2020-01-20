#ifndef _S2MM003_FW_H
#define _S2MM003_FW_H
void s2mm003_firmware_ver_check(struct s2mm003_data *usbpd_data, char *name);
int s2mm003_firmware_update(struct s2mm003_data *usbpd_data);
#endif
