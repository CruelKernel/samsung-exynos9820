#ifndef __ACPM_MFD_H__
#define __ACPM_MFD_H__

/* Define shift */
#define SHIFT_BYTE			(8)
#define SHIFT_REG			(0)
#define SHIFT_TYPE			(8)
#define SHIFT_FUNC			(0)
#define SHIFT_DEST			(8)
#define SHIFT_CNT			(8)
#define SHIFT_WRITE_VAL			(8)
#define SHIFT_UPDATE_VAL		(8)
#define SHIFT_UPDATE_MASK		(16)
#define SHIFT_RETURN			(24)
#define SHIFT_BULK_VAL   		SHIFT_BYTE
#define SHIFT_CHANNEL			(12)

/* Define mask */
#define MASK_BYTE			(0xFF)
#define MASK_REG			MASK_BYTE
#define MASK_TYPE			(0xF)
#define MASK_FUNC       		MASK_BYTE
#define MASK_DEST			MASK_BYTE
#define MASK_CNT			MASK_BYTE
#define MASK_WRITE_VAL			MASK_BYTE
#define MASK_UPDATE_VAL			MASK_BYTE
#define MASK_UPDATE_MASK		MASK_BYTE
#define MASK_RETURN			MASK_BYTE
#define MASK_BULK_VAL 			MASK_BYTE
#define MASK_CHANNEL			(0xF)

/* Command */
#define set_protocol(data, protocol)	        	((data & MASK_##protocol) << SHIFT_##protocol)
#define set_bulk_protocol(data, protocol, n)	        ((data & MASK_##protocol) << (SHIFT_##protocol * (n)))
#define read_protocol(data, protocol)	        	((data >> SHIFT_##protocol) & MASK_##protocol)
#define read_bulk_protocol(data, protocol, n)	        ((data >> (SHIFT_##protocol * (n))) & MASK_##protocol)

#define ACPM_MFD_PREFIX		"ACPM-MFD: "

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#ifdef ACPM_MFD_DEBUG
#define ACPM_MFD_PRINT(fmt, ...) printk(ACPM_MFD_PREFIX pr_fmt(fmt), ##__VA_ARGS__)
#else
#define ACPM_MFD_PRINT(fmt, ...)
#endif

enum mfd_func {
        FUNC_READ,
        FUNC_WRITE,
        FUNC_UPDATE,
        FUNC_BULK_READ,
        FUNC_BULK_WRITE,
};

#ifdef CONFIG_EXYNOS_ACPM
extern int exynos_acpm_read_reg(u8 channel, u16 type, u8 reg, u8 *dest);
extern int exynos_acpm_bulk_read(u8 channel, u16 type, u8 reg, int count, u8 *buf);
extern int exynos_acpm_write_reg(u8 channel, u16 type, u8 reg, u8 value);
extern int exynos_acpm_bulk_write(u8 channel, u16 type, u8 reg, int count, u8 *buf);
extern int exynos_acpm_update_reg(u8 channel, u16 type, u8 reg, u8 value, u8 mask);
#else
static inline int exynos_acpm_read_reg(u8 channel, u16 type, u8 reg, u8 *dest)
{
	return 0;
}
static inline int exynos_acpm_bulk_read(u8 channel, u16 type, u8 reg, int count, u8 *buf)
{
	return 0;
}
static inline int exynos_acpm_write_reg(u8 channel, u16 type, u8 reg, u8 value)
{
	return 0;
}
static inline int exynos_acpm_bulk_write(u8 channel, u16 type, u8 reg, int count, u8 *buf)
{
	return 0;
}
static inline int exynos_acpm_update_reg(u8 channel, u16 type, u8 reg, u8 value, u8 mask)
{
	return 0;
}
#endif
#endif /* __ACPM_MFD_H__ */
