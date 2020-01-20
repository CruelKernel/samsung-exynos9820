/*
 * s2mm005_ext.h - S2MM005 USB CC/PD external function definintion
 *
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __S2MM005_EXT_H
#define __S2MM005_EXT_H

#include <linux/i2c.h>
#include <linux/ccic/s2mm005.h>
#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// external functions in s2mm005.c
////////////////////////////////////////////////////////////////////////////////
extern void s2mm005_reset(struct s2mm005_data *usbpd_data);
extern void s2mm005_hard_reset(struct s2mm005_data *usbpd_data);
extern void s2mm005_reset_enable(struct s2mm005_data *usbpd_data);
extern void s2mm005_sram_reset(struct s2mm005_data *usbpd_data);
extern void s2mm005_system_reset(struct s2mm005_data *usbpd_data);
extern void s2mm005_int_clear(struct s2mm005_data *usbpd_data);
extern int s2mm005_read_byte(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size);
extern int s2mm005_read_byte_flash(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size);
extern int s2mm005_write_byte(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size);
extern int s2mm005_read_byte_16(const struct i2c_client *i2c, u16 reg, u8 *val);
extern int s2mm005_write_byte_16(const struct i2c_client *i2c, u16 reg, u8 val);
extern void s2mm005_rprd_mode_change(struct s2mm005_data *usbpd_data, u8 mode);
extern void s2mm005_manual_JIGON(struct s2mm005_data *usbpd_data, int mode);
extern void s2mm005_manual_LPM(struct s2mm005_data *usbpd_data, int cmd);
extern void s2mm005_control_option_command(struct s2mm005_data *usbpd_data, int cmd);
extern void s2mm005_set_upsm_mode(void);
////////////////////////////////////////////////////////////////////////////////
// external functions in s2mm005_cc.c
////////////////////////////////////////////////////////////////////////////////
extern void process_cc_attach(void * data, u8 *plug_attach_done);
extern void process_cc_detach(void * data);
extern void process_cc_get_int_status(void *data, uint32_t *pPRT_MSG, MSG_IRQ_STATUS_Type *MSG_IRQ_State);
extern void process_cc_rid(void * data);
extern void ccic_event_work(void *data, int dest, int id, int attach, int event,
			    int sub);
extern void process_cc_water_det(void * data);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
extern void role_swap_check(struct work_struct *work);
extern int dual_role_get_local_prop(struct dual_role_phy_instance *dual_role,
				    enum dual_role_property prop,
				    unsigned int *val);
extern int dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			      enum dual_role_property prop,
			      const unsigned int *val);
extern int dual_role_is_writeable(struct dual_role_phy_instance *drp,
				  enum dual_role_property prop);
extern void dp_detach(void *data);
#endif
////////////////////////////////////////////////////////////////////////////////
// external functions in ccic_alternate.c
////////////////////////////////////////////////////////////////////////////////
extern void send_alternate_message(void * data, int cmd);
extern void receive_alternate_message(void * data, VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State);
extern int ccic_register_switch_device(int mode);
extern void acc_detach_check(struct work_struct *work);
extern void send_unstructured_vdm_message(void * data, int cmd);
extern void receive_unstructured_vdm_message(void * data, SSM_MSG_IRQ_STATUS_Type *SSM_MSG_IRQ_State);
extern void send_role_swap_message(void * data, int cmd);
extern void send_attention_message(void * data, int cmd);
extern void do_alternate_mode_step_by_step(void * data, int cmd);
extern void set_enable_alternate_mode(int mode);
extern void set_clear_discover_mode(void);
extern void set_host_turn_on_event(int mode);
////////////////////////////////////////////////////////////////////////////////
// external functions in s2mm005_pd.c
////////////////////////////////////////////////////////////////////////////////
extern void s2mm005_select_pdo(int num);
extern void vbus_turn_on_ctrl(bool enable);
extern void process_pd(void *data, u8 plug_attach_done, u8 *pdic_attach, MSG_IRQ_STATUS_Type *MSG_IRQ_State);

#endif /* __S2MM005_EXT_H */
