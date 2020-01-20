#include "pmucal_cpu.h"
#include "pmucal_powermode.h"
#include "pmucal_rae.h"

unsigned int cpu_inform_c2;
unsigned int cpu_inform_cpd;

/**
 *  pmucal_cpu_enable - enables a core.
 *		        exposed to PWRCAL interface.
 *
 *  @cpu: cpu core index.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_cpu_enable(unsigned int cpu)
{
	int ret;

	if (cpu >= pmucal_cpu_list_size) {
		pr_err("%s core index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cpu, pmucal_cpu_list_size);
		return -EINVAL;
	}

	if (!pmucal_cpu_list[cpu].on) {
		pr_err("%s there is no sequence element for core(%d) power-on.\n",
				PMUCAL_PREFIX, cpu);
		return -ENOENT;
	}

	pmucal_powermode_hint_clear();
	ret = pmucal_rae_handle_seq(pmucal_cpu_list[cpu].on,
				pmucal_cpu_list[cpu].num_on);
	if (ret) {
		pr_err("%s %s: error on handling enable sequence. (cpu : %d)\n",
				PMUCAL_PREFIX, __func__, cpu);
		return ret;
	}

	pmucal_dbg_do_profile(pmucal_cpu_list[cpu].dbg, true);

	return 0;
}

/**
 *  pmucal_cpu_disable - disables a core.
 *		        exposed to PWRCAL interface.
 *
 *  @cpu: cpu core index.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_cpu_disable(unsigned int cpu)
{
	int ret;

	if (cpu >= pmucal_cpu_list_size) {
		pr_err("%s core index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cpu, pmucal_cpu_list_size);
		return -EINVAL;
	}

	if (!pmucal_cpu_list[cpu].off) {
		pr_err("%s there is no sequence element for core(%d) power-off.\n",
				PMUCAL_PREFIX, cpu);
		return -ENOENT;
	}

	pmucal_powermode_hint(cpu_inform_c2);

	pmucal_dbg_set_emulation(pmucal_cpu_list[cpu].dbg);

	ret = pmucal_rae_handle_seq(pmucal_cpu_list[cpu].off,
				pmucal_cpu_list[cpu].num_off);
	if (ret) {
		pr_err("%s %s: error on handling disable sequence. (cpu : %d)\n",
				PMUCAL_PREFIX, __func__, cpu);
		return ret;
	}

	pmucal_dbg_do_profile(pmucal_cpu_list[cpu].dbg, false);

	return 0;
}

/**
 *  pmucal_cpu_is_enabled - checks whether a core is enabled or not.
 *		            exposed to PWRCAL interface.
 *
 *  @cpu: cpu core index.
 *
 *  Returns 1 when the core is enabled, 0 when the core is disabled.
 *  Otherwise, negative error code.
 */
int pmucal_cpu_is_enabled(unsigned int cpu)
{
	int i;

	if (cpu >= pmucal_cpu_list_size) {
		pr_err("%s core index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cpu, pmucal_cpu_list_size);
		return -EINVAL;
	}

	if (!pmucal_cpu_list[cpu].status) {
		pr_err("%s there is no sequence element for core(%d) status.\n",
				PMUCAL_PREFIX, cpu);
		return -ENOENT;
	}

	pmucal_rae_handle_seq(pmucal_cpu_list[cpu].status,
				pmucal_cpu_list[cpu].num_status);

	for (i = 0; i < pmucal_cpu_list[cpu].num_status; i++) {
		if (pmucal_cpu_list[cpu].status[i].value !=
				pmucal_cpu_list[cpu].status[i].mask)
			break;
	}

	if (i == pmucal_cpu_list[cpu].num_status)
		return 1;
	else
		return 0;
}

/**
 *  pmucal_cpu_cluster_enable - enables a cluster.
 *		            exposed to PWRCAL interface.
 *
 *  @cluster: cpu cluster index.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_cpu_cluster_enable(unsigned int cluster)
{
	int ret;

	if (cluster >= pmucal_cluster_list_size) {
		pr_err("%s cluster index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cluster, pmucal_cluster_list_size);
		return -EINVAL;
	}

	pmucal_powermode_hint_clear();

	if (pmucal_cluster_list[cluster].num_on) {
		ret = pmucal_rae_handle_seq(pmucal_cluster_list[cluster].on,
				pmucal_cluster_list[cluster].num_on);
		if (ret) {
			pr_err("%s %s: error on handling enable sequence. (cluster : %d)\n",
					PMUCAL_PREFIX, __func__, cluster);
			return ret;
		}
	}

	pmucal_dbg_do_profile(pmucal_cluster_list[cluster].dbg, true);

	return 0;
}

/**
 *  pmucal_cpu_cluster_disable - disables a cluster.
 *		             exposed to PWRCAL interface.
 *
 *  @cluster: cpu cluster index.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int pmucal_cpu_cluster_disable(unsigned int cluster)
{
	int ret;

	if (cluster >= pmucal_cluster_list_size) {
		pr_err("%s cluster index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cluster, pmucal_cluster_list_size);
		return -EINVAL;
	}

	pmucal_powermode_hint(cpu_inform_cpd);

	pmucal_dbg_set_emulation(pmucal_cluster_list[cluster].dbg);

	if (pmucal_cluster_list[cluster].num_off) {
		ret = pmucal_rae_handle_seq(pmucal_cluster_list[cluster].off,
				pmucal_cluster_list[cluster].num_off);
		if (ret) {
			pr_err("%s %s: error on handling disable sequence. (cluster : %d)\n",
					PMUCAL_PREFIX, __func__, cluster);
			return ret;
		}
	}

	pmucal_dbg_do_profile(pmucal_cluster_list[cluster].dbg, false);

	return 0;
}

/**
 *  pmucal_cpu_cluster_is_enabled - checks whether a cluster is enabled or not.
 *		                exposed to PWRCAL interface.
 *
 *  @cluster: cpu cluster index.
 *
 *  Returns 1 when the cluster is enabled, 0 when disabled.
 *  Otherwise, negative error code.
 */
int pmucal_cpu_cluster_is_enabled(unsigned int cluster)
{
	int i;

	if (cluster >= pmucal_cluster_list_size) {
		pr_err("%s cluster index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cluster, pmucal_cluster_list_size);
		return -EINVAL;
	}

	if (!pmucal_cluster_list[cluster].status) {
		pr_err("%s there is no sequence element for cluster(%d) status.\n",
				PMUCAL_PREFIX, cluster);
		return -ENOENT;
	}

	pmucal_rae_handle_seq(pmucal_cluster_list[cluster].status,
				pmucal_cluster_list[cluster].num_status);

	for (i = 0; i < pmucal_cluster_list[cluster].num_status; i++) {
		if (pmucal_cluster_list[cluster].status[i].value !=
				pmucal_cluster_list[cluster].status[i].mask)
			break;
	}

	if (i == pmucal_cluster_list[cluster].num_status)
		return 1;
	else
		return 0;
}

/**
 *  pmucal_cpu_cluster_req_emulation - requests en/disabling of emulation at next CPD enter.
 *		                exposed to CAL interface.
 *
 *  @cluster: cpu cluster index.
 *  @en	    : en/disable emulation.
 *
 *  Returns 0 if it succeeds.
 *  Otherwise, negative error code.
 */
int pmucal_cpu_cluster_req_emulation(unsigned int cluster, bool en)
{
	if (cluster >= pmucal_cluster_list_size) {
		pr_err("%s cluster index(%d) is out of supported range (0~%d).\n",
				PMUCAL_PREFIX, cluster, pmucal_cluster_list_size);
		return -EINVAL;
	}

	pmucal_dbg_req_emulation(pmucal_cluster_list[cluster].dbg, en);

	return 0;
}

/**
 *  pmucal_cpu_init - Init function of PMUCAL CPU common logic.
 *		            exposed to PWRCAL interface.
 *
 *  Returns 0 on success. Otherwise, negative error code.
 */
int __init pmucal_cpu_init(void)
{
	int ret = 0, i;

	if (!pmucal_cpu_list_size || !pmucal_cluster_list_size) {
		pr_err("%s %s: there is no cpu/cluster list. aborting init...\n",
				PMUCAL_PREFIX, __func__);
		return -ENOENT;
	}

	/* convert physical base address to virtual addr */
	for (i = 0; i < pmucal_cpu_list_size; i++) {
		/* skip non-existing cores */
		if (!pmucal_cpu_list[i].num_on && !pmucal_cpu_list[i].num_off && !pmucal_cpu_list[i].num_status)
			continue;

		ret = pmucal_rae_phy2virt(pmucal_cpu_list[i].on,
					pmucal_cpu_list[i].num_on);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:enable, core_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_cpu_list[i].off,
					pmucal_cpu_list[i].num_off);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:disable, core_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_cpu_list[i].status,
					pmucal_cpu_list[i].num_status);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:status, core_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}
	}

	for (i = 0; i < pmucal_cluster_list_size; i++) {
		/* skip non-existing clusters */
		if (!pmucal_cluster_list[i].num_on && !pmucal_cluster_list[i].num_off && !pmucal_cluster_list[i].num_status)
			continue;

		ret = pmucal_rae_phy2virt(pmucal_cluster_list[i].on,
					pmucal_cluster_list[i].num_on);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:enable, cluster_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_cluster_list[i].off,
					pmucal_cluster_list[i].num_off);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:disable, cluster_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}

		ret = pmucal_rae_phy2virt(pmucal_cluster_list[i].status,
					pmucal_cluster_list[i].num_status);
		if (ret) {
			pr_err("%s %s: error on PA2VA conversion. seq:status, cluster_id:%d. aborting init...\n",
					PMUCAL_PREFIX, __func__, i);
			goto out;
		}
	}

out:
	return ret;
}
