#ifdef CONFIG_KUNIT
#include <kunit/mock.h>
#endif
#include <linux/dsms.h>
#include <linux/kmod.h>

#ifdef CONFIG_KUNIT
extern char *dsms_alloc_user_string(const char *string);
extern char *dsms_alloc_user_value(int64_t value);
extern void dsms_free_user_string(const char *string);
extern void dsms_message_cleanup(struct subprocess_info *info);
extern inline int dsms_send_allowed_message(const char *feature_code,
		const char *detail,
		int64_t value);
#endif
