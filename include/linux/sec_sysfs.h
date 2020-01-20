#ifndef SEC_SYSFS_H
#define SEC_SYSFS_H

#ifdef CONFIG_SEC_SYSFS
extern struct device *sec_device_create(void *drvdata, const char *fmt);
extern void sec_device_destroy(dev_t devt);
#else
static inline struct device *sec_device_create(void *drvdata, const char *fmt)
{
	pr_err("No rule to make sec sysfs\n");
	return NULL;
}
static inline void sec_device_destroy(dev_t devt)
{
	return;
}
#endif

#endif /* SEC_SYSFS_H */
