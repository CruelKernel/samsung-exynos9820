#include <linux/ccic/core.h>
#include <linux/ccic/s2mm005_usbpd.h>
#include <linux/ccic/s2mm005_usbpd_phy.h>

int s2mm005_usbpd_select_pdo(struct usbpd_dev *udev, unsigned int selector)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	struct i2c_client *i2c = usbpd_data->i2c;
	uint8_t CMD_DATA[3];

	dev_info(&udev->dev, "%s : PDO(%d) is selected to change\n",
						  __func__, selector);

	CMD_DATA[0] = 0x3;
	CMD_DATA[1] = 0x3;
	CMD_DATA[2] = selector;
	s2mm005_write_byte(i2c, REG_I2C_SLV_CMD, &CMD_DATA[0], 3);

	CMD_DATA[0] = 0x3;
	CMD_DATA[1] = 0x2;
	CMD_DATA[2] = State_PE_SNK_Wait_for_Capabilities;
	s2mm005_write_byte(i2c, REG_I2C_SLV_CMD, &CMD_DATA[0], 3);

	return 0;
}

int s2mm005_usbpd_select_svid(struct usbpd_dev *udev, unsigned int sel_svid)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	uint8_t W_DATA[3] = {0,};
	int ret = 0;

	if (usbpd_data == NULL) {
		dev_err(&udev->dev, "%s usbpd_data is invalid data\n",
			__func__);
		return -ENOMEM;
	}

	/* write s2mm005 with TypeC_Dex_SUPPORT SVID */
	/* It will start discover mode with that svid */
	dev_info(&udev->dev, "%s : svid1 is dex station\n", __func__);

	W_DATA[0] = S2MM005_MODE_INTERFACE;	/* Mode Interface */
	W_DATA[1] = S2MM005_SVID_SELECT;   /* SVID select*/
	W_DATA[2] = sel_svid;	/* SVID select with Samsung vendor ID*/
	ret = s2mm005_write_byte(usbpd_data->i2c, 0x10, &W_DATA[0], 3);

	if (ret < 0) {
		dev_err(usbpd_data->dev, "%s has i2c write error.\n", __func__);
		return ret;
	}

	return ret;
}
int s2mm005_usbpd_dp_pin_assignment(struct usbpd_dev *udev, unsigned int pin_assignment)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	uint8_t W_DATA[3] = {0,};
	int ret = 0;

	if (usbpd_data == NULL) {
		dev_err(&udev->dev, "%s usbpd_data is invalid data\n",
			__func__);
		return -ENOMEM;
	}

	W_DATA[0] = S2MM005_MODE_INTERFACE;	/* Mode Interface */
	W_DATA[1] = S2MM005_DP_ALT_MODE_REQ;   /* DP Alternate Mode Request */
	W_DATA[2] = pin_assignment;;	/* DP Pin Assign select */

	ret = s2mm005_write_byte(usbpd_data->i2c, 0x10, &W_DATA[0], 3);

	if (ret < 0) {
		dev_err(usbpd_data->dev, "%s has i2c write error.\n", __func__);
		return ret;
	}

	return ret;
}

void s2mm005_usbpd_clear_discover_mode(struct usbpd_dev *udev)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	u8 W_DATA[2];

	if (usbpd_data == NULL) {
		dev_err(&udev->dev, "%s usbpd_data is invalid data\n",
			__func__);
		return;
	}

	W_DATA[0] = 0x3;
	W_DATA[1] = 0x32;

	s2mm005_write_byte(usbpd_data->i2c, 0x10, &W_DATA[0], 2);

	dev_info(usbpd_data->dev, "%s : clear discover mode! \n", __func__);
}

int s2mm005_usbpd_pd_next_state(struct usbpd_dev *udev)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	uint8_t W_DATA[3] = {0,};
	int ret = 0;

	if (usbpd_data == NULL) {
		dev_err(&udev->dev, "%s usbpd_data is invalid data\n",
			__func__);
		return -ENOMEM;
	}

	W_DATA[0] = S2MM005_MODE_INTERFACE;	/* Mode Interface */
	W_DATA[1] = S2MM005_PD_NEXT_STATE;   /* Select mode as pd next state*/
	W_DATA[2] = 89;	/* PD next state*/

	ret = s2mm005_write_byte(usbpd_data->i2c, REG_I2C_SLV_CMD, &W_DATA[0], 3);
	if (ret < 0) {
		dev_err(usbpd_data->dev, "%s has i2c write error.\n",
			__func__);
		return ret;
	}

	return ret;
}

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
static void s2mm005_new_toggling_control(struct s2mm005_data *usbpd_data, u8 mode)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD;
	u8 W_DATA[2];

	pr_info("%s, mode=0x%x\n", __func__, mode);

	W_DATA[0] = 0x03;
	W_DATA[1] = mode; /* 0x12 : detach, 0x13 : SRC, 0x14 : SNK */

	REG_ADD = 0x10;
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 2);
}

static void s2mm005_toggling_control(struct s2mm005_data *usbpd_data, u8 mode)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 buf;

	pr_info("%s, mode=0x%x\n", __func__, mode);

	buf = mode;
	s2mm005_sel_write(i2c, 0x5000, &buf, SEL_WRITE_BYTE);
}

int s2mm005_usbpd_set_rprd_mode (struct usbpd_dev *udev, unsigned int mode)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);

	dev_info(usbpd_data->dev, "%s, mode=0x%x\n", __func__, mode);

	switch (mode) {
	case TYPE_C_ATTACH_DFP: /* SRC */
		s2mm005_new_toggling_control(usbpd_data, 0x12);
		msleep(1000);
		s2mm005_new_toggling_control(usbpd_data, 0x13);
		break;
	case TYPE_C_ATTACH_UFP: /* SNK */
		s2mm005_new_toggling_control(usbpd_data, 0x12);
		msleep(1000);
		s2mm005_new_toggling_control(usbpd_data, 0x14);
		break;
	case TYPE_C_ATTACH_DRP: /* DRP */
		s2mm005_toggling_control(usbpd_data, TYPE_C_ATTACH_DRP);
		break;
	};

	return 0;
}
#endif

void s2mm005_usbpd_set_upsm(struct usbpd_dev *udev)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	uint8_t W_DATA[2] = {0,};

	W_DATA[0] = 0x3;
	W_DATA[1] = 0x40;
	s2mm005_write_byte(usbpd_data->i2c, 0x10, &W_DATA[0], 2);
}

void s2mm005_usbpd_disable_irq(struct usbpd_dev *udev)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	struct i2c_client *i2c = usbpd_data->i2c;

	disable_irq(i2c->irq);
}

void s2mm005_usbpd_enable_irq(struct usbpd_dev *udev)
{
	struct s2mm005_data *usbpd_data = udev_get_drvdata(udev);
	struct i2c_client *i2c = usbpd_data->i2c;

	enable_irq(i2c->irq);
}

struct usbpd_ops s2mm005_usbpd_ops = {
	.usbpd_select_pdo		= s2mm005_usbpd_select_pdo,
	.usbpd_set_upsm			= s2mm005_usbpd_set_upsm,
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	.usbpd_set_rprd_mode		= s2mm005_usbpd_set_rprd_mode,
	.usbpd_disable_irq		= s2mm005_usbpd_disable_irq,
	.usbpd_enable_irq		= s2mm005_usbpd_enable_irq,
#endif
/* TODO
	.usbpd_set_dp_pin		= s2mm005_usbpd_set_dp_pin,
	.usbpd_control_option		= s2mm005_usbpd_control_option,
*/
	.usbpd_pd_next_state		= s2mm005_usbpd_pd_next_state,
	.usbpd_clear_discover_mode	= s2mm005_usbpd_clear_discover_mode,
	.usbpd_select_svid		= s2mm005_usbpd_select_svid,
	.usbpd_dp_pin_assignment	= s2mm005_usbpd_dp_pin_assignment,
};

void s2mm005_usbpd_ops_register(struct usbpd_desc *desc)
{
	desc->ops = &s2mm005_usbpd_ops;
}
