#include "pmucal_local.h"
#include "pmucal_rae.h"
#ifdef CONFIG_SOC_EXYNOS9820
#include <soc/samsung/exynos-debug.h>
#endif

#ifndef PWRCAL_TARGET_LINUX
struct pmucal_pd *pmucal_blkpwr_list[PMUCAL_NUM_PDS];
#endif

/**
 *  pmucal_local_enable - enables a power domain.
 *		        exposed to PWRCAL interface.
 *
 *  @pd_id: power domain index.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_local_enable(unsigned int pd_id)
{
	int ret;

	dbg_snapshot_pmu(pd_id, __func__, DSS_FLAG_IN);

	if (pd_id >= pmucal_pd_list_size) {
		pr_err("%s pd index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, pd_id, pmucal_pd_list_size);
		return -EINVAL;
	}

	if (!pmucal_pd_list[pd_id].on) {
		pr_err("%s there is no sequence element for pd(%d) power-on.\n",
				PMUCAL_PREFIX, pd_id);
		return -ENOENT;
	}

	ret = pmucal_rae_handle_seq(pmucal_pd_list[pd_id].on,
				pmucal_pd_list[pd_id].num_on);
	if (ret) {
		pr_err("%s %s: error on handling enable sequence. (pd_id : %d)\n",
				PMUCAL_PREFIX, __func__, pd_id);
		return ret;
	}

	if (pmucal_pd_list[pd_id].need_smc) {
		ret = exynos_pd_tz_restore(pmucal_pd_list[pd_id].need_smc);
		if (ret) {
			pr_err("%s %s: DTZPC restore smc error. (pd_id : %d)\n",
					PMUCAL_PREFIX, __func__, pd_id);
			return ret;
		}
	}

	ret = pmucal_rae_restore_seq(pmucal_pd_list[pd_id].save,
				pmucal_pd_list[pd_id].num_save);
	if (ret) {
		pr_err("%s %s: error on handling restore sequence. (pd_id : %d)\n",
				PMUCAL_PREFIX, __func__, pd_id);
		return ret;
	}

	dbg_snapshot_pmu(pd_id, __func__, DSS_FLAG_OUT);

	pmucal_dbg_do_profile(pmucal_pd_list[pd_id].dbg, true);

	return 0;
}

/**
 *  pmucal_local_disable - disables a power domain.
 *		        exposed to PWRCAL interface.
 *
 *  @pd_id: power domain index.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_local_disable(unsigned int pd_id)
{
	int ret, i;

	dbg_snapshot_pmu(pd_id, __func__, DSS_FLAG_IN);

	if (pd_id >= pmucal_pd_list_size) {
		pr_err("%s pd index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, pd_id, pmucal_pd_list_size);
		return -EINVAL;
	}

	if (!pmucal_pd_list[pd_id].off) {
		pr_err("%s there is no sequence element for pd(%d) power-off.\n",
				PMUCAL_PREFIX, pd_id);
		return -ENOENT;
	}

	pmucal_rae_save_seq(pmucal_pd_list[pd_id].save,
				pmucal_pd_list[pd_id].num_save);

	if (pmucal_pd_list[pd_id].need_smc) {
		ret = exynos_pd_tz_save(pmucal_pd_list[pd_id].need_smc);
		if (ret) {
			pr_err("%s %s: DTZPC save smc error. (pd_id : %d)\n",
					PMUCAL_PREFIX, __func__, pd_id);
			return ret;
		}
	}

	pmucal_dbg_set_emulation(pmucal_pd_list[pd_id].dbg);

	ret = pmucal_rae_handle_seq(pmucal_pd_list[pd_id].off,
				pmucal_pd_list[pd_id].num_off);
	if (ret) {
		pr_err("%s %s: error on handling disable sequence. (pd_id : %d)\n",
				PMUCAL_PREFIX, __func__, pd_id);

		for (i = 0; i < pmucal_pd_list[pd_id].num_save; i++) {
			pr_err("%s[0x%x] = 0x%x\n", pmucal_pd_list[pd_id].save[i].sfr_name,
							pmucal_pd_list[pd_id].save[i].offset,
							pmucal_pd_list[pd_id].save[i].value);
		}

#ifdef CONFIG_SOC_EXYNOS9820
		if (pd_id == 13 || pd_id == 14)
			s3c2410wdt_set_emergency_reset(0, 0);
#endif

		return ret;
	}

	dbg_snapshot_pmu(pd_id, __func__, DSS_FLAG_OUT);

	pmucal_dbg_do_profile(pmucal_pd_list[pd_id].dbg, false);

	return 0;
}

/**
 *  pmucal_local_is_enabled - checks whether a power domain is enabled or not.
 *		            exposed to PWRCAL interface.
 *
 *  @pd_id: power domain index.
 *
 *  Returns 1 when the pd is enabled, 0 when disabled.
 *  Otherwise, negative error code.
 */
int pmucal_local_is_enabled(unsigned int pd_id)
{
	int i;

	if (pd_id >= pmucal_pd_list_size) {
		pr_err("%s pd index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, pd_id, pmucal_pd_list_size);
		return -EINVAL;
	}

	if (!pmucal_pd_list[pd_id].status) {
		pr_err("%s there is no sequence element for pd(%d) status.\n",
				PMUCAL_PREFIX, pd_id);
		return -ENOENT;
	}

	pmucal_rae_handle_seq(pmucal_pd_list[pd_id].status,
				pmucal_pd_list[pd_id].num_status);

	for (i = 0; i < pmucal_pd_list[pd_id].num_status; i++) {
		if (pmucal_pd_list[pd_id].status[i].value !=
				pmucal_pd_list[pd_id].status[i].mask)
			break;
	}

	if (i == pmucal_pd_list[pd_id].num_status)
		return 1;
	else
		return 0;
}

void pmucal_local_set_smc_id(unsigned int pd_id, unsigned int need_smc)
{
	if (need_smc) {
		if (pd_id >= pmucal_pd_list_size) {
			pr_err("%s pd index(%d) is out of supported range (0~%d).\n",
					PMUCAL_PREFIX, pd_id, pmucal_pd_list_size);
			return;
		}

		pmucal_pd_list[pd_id].need_smc = need_smc;
	}
}

/**
 *  pmucal_local_init - Init function of PMUCAL LOCAL common logic.
 *		            exposed to PWRCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int __init pmucal_local_init(void)
{
	int ret = 0, i;

	if (!pmucal_pd_list_size) {
		pr_err("%s %s: there is no pd list. aborting init...\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	/* convert physical base address to virtual addr */
	for (i = 0; i < pmucal_pd_list_size; i++) {
		/* skip non-existing pd */
		if (!pmucal_pd_list[i].num_on && !pmucal_pd_list[i].num_off && !pmucal_pd_list[i].num_status)
			continue;

		ret = pmucal_rae_phy2virt(pmucal_pd_list[i].on,
					pmucal_pd_list[i].num_on);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:on, pd_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_pd_list[i].save,
					pmucal_pd_list[i].num_save);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:save, pd_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_pd_list[i].off,
					pmucal_pd_list[i].num_off);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:off, pd_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_pd_list[i].status,
					pmucal_pd_list[i].num_status);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:status, pd_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

#ifndef PWRCAL_TARGET_LINUX
		pmucal_blkpwr_list[i] = pmucal_pd_list + i;
#endif
	}

out:
	return ret;
}
