#ifndef __s2MU004_MUIC_SYSFS_H__
#define __s2MU004_MUIC_SYSFS_H__

extern void hv_muic_change_afc_voltage(int tx_data);
int s2mu004_muic_init_sysfs(struct s2mu004_muic_data *muic_data);
void s2mu004_muic_deinit_sysfs(struct s2mu004_muic_data *muic_data);

#endif
