#include <kunit/test.h>
#include <kunit/mock.h>
#include <linux/sched.h>
#include "icd.h"
#include "oemflag.h"
#include "icd_protect_list.gen.h"

static void contains_str_test(struct test *test)
{
	char *path = "/system/bin/mediaserver";
	bool ret = contains_str(tz_drm_list, path);

	test_info(test, "contains_str tz_drm_list ret = %d", ret);
	EXPECT_TRUE(test, ret);

	path = "/system/bin/qseecomd";
	ret = contains_str(fido_list, path);
	test_info(test, "contains_str fido_list ret = %d", ret);
	EXPECT_TRUE(test, ret);

	path = "/system/bin/charon";
	ret = contains_str(cc_list, path);
	test_info(test, "contains_str cc_list ret = %d", ret);
	EXPECT_TRUE(test, ret);
}

static void oem_flags_test(struct test *test)
{
	enum oemflag_id oemid = OEMFLAG_TZ_DRM;
	int ret;

	ret = oem_flags_set(oemid);
	test_info(test, "oemflag set ret = %d", ret);
	EXPECT_EQ(test, ret, 0);
	ret = oem_flags_get(oemid);
	test_info(test, "oemflag get ret = %d", ret);
	EXPECT_EQ(test, ret, 1);
}

static int security_icdrv_test_init(struct test *test)
{
	return 0;
}

static void security_icdrv_test_exit(struct test *test)
{

}

static struct test_case security_icdrv_testcases[] = {
	TEST_CASE(contains_str_test),
	TEST_CASE(oem_flags_test),
	{},
};

static struct test_module security_icdrv_testmodule = {
	.name = "security-icdrv-test",
	.init = security_icdrv_test_init,
	.exit = security_icdrv_test_exit,
	.test_cases = security_icdrv_testcases,
};

module_test(security_icdrv_testmodule);
