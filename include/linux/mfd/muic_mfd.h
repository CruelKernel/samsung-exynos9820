#include <linux/muic/muic.h>

#define MFD_MUIC_DEV_NAME "mfd-muic-universal"

enum max77849_irq {
	MUIC_MFD_IRQ,
	MUIC_MFD_IRQ_NR,
};

enum muic_mfd_irq_src {
	IRQ_SRC_MUIC	= 1 << 0,
	IRQ_SRC_SYS	= 1 << 1,
	IRQ_SRC_CHG	= 1 << 2,
	IRQ_SRC_FUEL	= 1 << 3,
	IRQ_SRC_NR,
};

enum muic_mfd_irq_grp {
	IRQ_GRP_MUIC  = 0,
	IRQ_GRP_SYS   = 1,
	IRQ_GRP_CHG   = 2,
	IRQ_GRP_FUEL  = 3,
	IRQ_GRP_NR,
};

struct muic_mfd_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct i2c_client *chg;
	struct i2c_client *fuel;
	struct i2c_client *muic;
	struct mutex i2c_lock;

	int type;

	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;

	int irq_masks_cur[IRQ_GRP_NR];
	int irq_masks_cache[IRQ_GRP_NR];

	struct muic_mfd_platform_data *pdata;
};

struct muic_mfd_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct muic_platform_data *muic_pdata;

#if 0
	sec_battery_platform_data_t *charger_data;
	sec_battery_platform_data_t *fuelgauge_data;
	int num_regulators;
	struct max77849_regulator_data *regulators;
#endif	

	struct mfd_cell *sub_devices;
	int num_subdevs;
};

