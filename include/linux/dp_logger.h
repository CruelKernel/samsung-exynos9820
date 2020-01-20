#ifndef _DP_LOGGER_H_
#define _DP_LOGGER_H_

#ifdef CONFIG_SEC_DISPLAYPORT_LOGGER
void dp_logger_set_max_count(int count);
void dp_logger_print(const char *fmt, ...);
void dp_print_hex_dump(void *buf, void *pref, size_t len);
int init_dp_logger(void);
#else
#define dp_logger_set_max_count(x)		do { } while (0)
#define dp_logger_print(fmt, ...)		do { } while (0)
#define dp_print_hex_dump(buf, pref, len)	do { } while (0)
#define init_dp_logger()			do { } while (0)
#endif

#endif
