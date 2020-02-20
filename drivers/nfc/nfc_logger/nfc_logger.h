#ifndef _NFC_LOGGER_H_
#define _NFC_LOGGER_H_

#ifdef CONFIG_SEC_NFC_LOGGER

#define NFC_LOG_ERR(fmt, ...) \
	do { \
		pr_err(fmt, ##__VA_ARGS__); \
		nfc_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)
#define NFC_LOG_INFO(fmt, ...) \
	do { \
		pr_info(fmt, ##__VA_ARGS__); \
		nfc_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)
#define NFC_LOG_DBG(fmt, ...)		pr_debug(fmt, ##__VA_ARGS__)
#define NFC_LOG_REC(fmt, ...)		nfc_logger_print(fmt, ##__VA_ARGS__)

void nfc_logger_set_max_count(int count);
void nfc_logger_print(const char *fmt, ...);
void nfc_print_hex_dump(void *buf, void *pref, size_t len);
int nfc_logger_init(void);

#else /*CONFIG_SEC_NFC_LOGGER*/

#define NFC_LOG_ERR(fmt, ...)		pr_err(fmt, ##__VA_ARGS__)
#define NFC_LOG_INFO(fmt, ...)		pr_info(fmt, ##__VA_ARGS__)
#define NFC_LOG_DBG(fmt, ...)		pr_debug(fmt, ##__VA_ARGS__)
#define NFC_LOG_REC(fmt, ...)		do { } while (0)

#define nfc_print_hex_dump(a, b, c)	do { } while (0)
#define nfc_logger_init()		do { } while (0)
#define nfc_logger_set_max_count(a)	do { } while (0)
#endif

#endif
