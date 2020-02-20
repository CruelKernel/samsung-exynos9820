/* sec_nad_balancer.c
 *
 * NAD Balancer Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Hyeokseon Yu <hyeokseon.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/sec_nad_balancer.h>
#include <linux/sec_ext.h>

#define NAD_PRINT(format, ...) pr_info("[NAD_BALANCER] " format, ##__VA_ARGS__)
#define DEBUG_NAD_BALANCER

#if defined(CONFIG_SEC_FACTORY)
#ifdef CONFIG_OF
static int parse_qos_data(struct device *dev,
			  struct nad_balancer_platform_data *pdata,
			  struct device_node *np)

{
	struct device_node *cnp;
	struct nad_balancer_pm_qos *cqos;
	int ncount = 0;
	int i;
	u32 freq_item;

	for_each_child_of_node(np, cnp) {
		cqos = &pdata->qos_items[ncount++];

		//cqos->desc = of_get_property(cnp, "qos,label", NULL);
		if(of_property_read_string(cnp, "qos,label", (const char**)&cqos->desc)) {
			dev_err(dev, "Failed to get label name: node not exist\n");
			return -EINVAL;
		}

#if defined(DEBUG_NAD_BALANCER)
		NAD_PRINT("qos desc : %s, 0x%x\n", cqos->desc, &cqos->desc);
#endif

		if (of_property_read_u32(cnp, "qos,delay_time", &cqos->delay_time)) {
			dev_err(dev, "Failed to get delay time: node not exist\n");
			return -EINVAL;
		}

		if (of_property_read_u32(cnp, "qos,table_size", &cqos->table_size)) {
			dev_err(dev, "Failed to get table size: node not exist\n");

			if (of_property_read_u32(cnp, "qos,big_turbo_enable", &cqos->big_turbo_enable)) {
				dev_err(dev, "Failed to get big_turbo_enable: node not exist\n");
				return -EINVAL;
			}
		}

		/* single dvfs tables */
		if (cqos->table_size != 0) {
			if (!cqos->tables) {
				cqos->tables = devm_kzalloc(dev, sizeof(struct freq_table) * cqos->table_size,
						    GFP_KERNEL);
				if (!cqos->tables) {
					dev_err(dev, "Failed to allocate memory of freq_table\n");
					return -ENOMEM;
				}
			}
			for (i = 0; i < cqos->table_size; i++) {
				if (of_property_read_u32_index(cnp, "qos,table", i, &freq_item))
					return -EINVAL;
				cqos->tables[i].freq = freq_item;
			}
		}
		/* multi dvfs tables - big cluster */
		else {
			if ((cqos->big_turbo_enable & NAD_BALANCER_MODE_SINGLE) == NAD_BALANCER_MODE_SINGLE) {
				if (of_property_read_u32(cnp, "qos,s_table_size", &cqos->single_table_size)) {
					dev_err(dev, "Failed to get s_table_size : check configurations\n");
					return -EINVAL;
				}
				if (!cqos->single_tables) {
					cqos->single_tables = devm_kzalloc(dev, sizeof(struct freq_table) * cqos->single_table_size,
						    GFP_KERNEL);
					if (!cqos->single_tables) {
						dev_err(dev, "Failed to allocate memory of single_freq_table\n");
						return -ENOMEM;
					}
				}
				for (i = 0; i < cqos->single_table_size; i++) {
					if (of_property_read_u32_index(cnp, "qos,s_table", i, &freq_item)) {
						dev_err(dev, "Fail to read s_table [%d]\n", i);
						return -EINVAL;
					}
					cqos->single_tables[i].freq = freq_item;
				}
			}
			if ((cqos->big_turbo_enable & NAD_BALANCER_MODE_DUAL) == NAD_BALANCER_MODE_DUAL) {
				if (of_property_read_u32(cnp, "qos,d_table_size", &cqos->dual_table_size)) {
					dev_err(dev, "Failed to get d_table_size : check configurations\n");
					return -EINVAL;
				}
				if (!cqos->dual_tables) {
					cqos->dual_tables = devm_kzalloc(dev, sizeof(struct freq_table) * cqos->dual_table_size,
						    GFP_KERNEL);
					if (!cqos->dual_tables) {
						dev_err(dev, "Failed to allocate memory of dual_freq_table\n");
						return -ENOMEM;
					}
				}
				for (i = 0; i < cqos->dual_table_size; i++) {
					if (of_property_read_u32_index(cnp, "qos,d_table", i, &freq_item)) {
						dev_err(dev, "Fail to read d_table [%d]\n", i);
						return -EINVAL;
					}
					cqos->dual_tables[i].freq = freq_item;
				}
			}
			if ((cqos->big_turbo_enable & NAD_BALANCER_MODE_QUAD) == NAD_BALANCER_MODE_QUAD) {
				if (of_property_read_u32(cnp, "qos,q_table_size", &cqos->quad_table_size)) {
					dev_err(dev, "Failed to get q_table_size : check configurations\n");
					return -EINVAL;
				}
				if (!cqos->quad_tables) {
					cqos->quad_tables = devm_kzalloc(dev, sizeof(struct freq_table) * cqos->quad_table_size,
						    GFP_KERNEL);
					if (!cqos->quad_tables) {
						dev_err(dev, "Failed to allocate memory of quad_freq_table\n");
						return -ENOMEM;
					}
				}
				for (i = 0; i < cqos->quad_table_size; i++) {
					if (of_property_read_u32_index(cnp, "qos,q_table", i, &freq_item)) {
						dev_err(dev, "Fail to read q_table [%d]\n", i);
						return -EINVAL;
					}
					cqos->quad_tables[i].freq = freq_item;
				}
			}
			/* set as default quad mode */
			cqos->current_mode = NAD_BALANCER_MODE_QUAD;
		}
	}
	return 0;
}

static int parse_sleep_data(struct device *dev,
			    struct nad_balancer_platform_data *pdata,
			    struct device_node *np)

{
	struct nad_balancer_sleep_info *csleep;

	csleep = pdata->sleep_items;

	if (of_property_read_u32(np, "sleep,suspend_threshold", &csleep->suspend_threshold)) {
		dev_err(dev, "Failed to get suspend time: node not exist\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "sleep,resume_threshold", &csleep->resume_threshold)) {
		dev_err(dev, "Failed to get resume time: node not exist\n");
		return -EINVAL;
	}

	return 0;
}

static int nad_balancer_parse_dt(struct device *dev)
{
	struct nad_balancer_platform_data *pdata = dev->platform_data;
	struct device_node *np;
	struct device_node *qos_np;
	struct device_node *sleep_np;

	np = dev->of_node;

	if (of_property_read_u32(np, "nad_balancer,timeout", &pdata->timeout)) {
		dev_err(dev, "error to read timeout value\n");
		return -EINVAL;
	}

	pdata->nItem = of_get_child_count(np);
	if (!pdata->nItem) {
		dev_err(dev, "There are no items\n");
		return -ENODEV;
	}

	qos_np = of_find_node_by_name(np, "qos");
	pdata->nQos = of_get_child_count(qos_np);
	pdata->qos_items = devm_kzalloc(dev,
					sizeof(struct nad_balancer_pm_qos) * pdata->nQos, GFP_KERNEL);
	if (!pdata->qos_items)
		return -ENOMEM;

	sleep_np = of_find_node_by_name(np, "sleep");
	pdata->nSleep = of_get_child_count(sleep_np);
	pdata->sleep_items = devm_kzalloc(dev,
					  sizeof(struct nad_balancer_sleep_info), GFP_KERNEL);
	if (!pdata->sleep_items)
		return -ENOMEM;

	if (qos_np)
		parse_qos_data(dev, pdata, qos_np);

	if (sleep_np)
		parse_sleep_data(dev, pdata, sleep_np);

	return 0;
}
#endif

void report_sleep_info(struct device *dev, pm_message_t state,
		       unsigned long long usec)
{
	struct nad_balancer_info *pinfo;
	struct nad_balancer_sleep_info *psleep;

	if (!sleep_test_enable)
		return;

	pinfo = dev_get_drvdata(sec_nad_balancer);
	psleep = pinfo->pdata->sleep_items;

	switch (state.event) {
	case PM_EVENT_SUSPEND:
		if ((usec / USEC_PER_MSEC) > psleep->suspend_threshold) {
			panic("NAD_BALANCER: over suspend time! \
			      dev:%s, %Ld.%03Ld msecs\n",
				dev_name(dev),
				usec / USEC_PER_MSEC,
				usec % USEC_PER_MSEC);
		}
		break;
	case PM_EVENT_RESUME:
		if ((usec / USEC_PER_MSEC) > psleep->resume_threshold) {
			panic("NAD_BALANCER: over resume time!  \
			      dev:%s, %Ld.%03Ld msecs\n",
				dev_name(dev),
				usec / USEC_PER_MSEC,
				usec % USEC_PER_MSEC);
		}
		break;
	default:
		break;
	}
}

static bool sec_nad_balancer_enabled(void)
{
	return nad_balancer_enable;
}

static void sec_nad_balancer_enable(void)
{
	mutex_lock(&nad_balancer_lock);
	nad_balancer_enable = true;
	mutex_unlock(&nad_balancer_lock);
}

static void sec_nad_balancer_disable(void)
{
	mutex_lock(&nad_balancer_lock);
	nad_balancer_enable = false;
	mutex_unlock(&nad_balancer_lock);
}

static void sec_nad_balancer_timeout_work(struct work_struct *work)
{
	NAD_PRINT("exprired nad qos work.\n");
	sec_nad_balancer_disable();
}

static void update_temperature(struct nad_balancer_pm_qos *pqos, int type)
{
	int temp[MAX_TMU_COUNT];
	int i;

#if !defined(CONFIG_SEC_BOOTSTAT)
	return;
#endif

	if (type == UPDATE_FIRST_TEMPERATURE) {
		sec_bootstat_get_thermal(pqos->temperature);
		NAD_PRINT("first temperature ");
		for (i = 0; i < MAX_TMU_COUNT; i++)
			pr_info("[%d]", pqos->temperature[i]);
		pr_info("\n");
	} else if (type == UPDATE_CONTI_TEMPERATURE) {
		sec_bootstat_get_thermal(temp);
		for (i = 0; i < MAX_TMU_COUNT; i++) {
			if (pqos->temperature[i] != max(pqos->temperature[i], temp[i])) {
				NAD_PRINT("update temperature[%d] prev[%d] new [%d]\n",
					  i, pqos->temperature[i], temp[i]);
				pqos->temperature[i] = temp[i];
			}
		}
	} else if (type == UPDATE_FINAL_TEMPERATURE) {
		sec_bootstat_get_thermal(temp);
		for (i = 0; i < MAX_TMU_COUNT; i++) {
			if (pqos->temperature[i] != max(pqos->temperature[i], temp[i])) {
				NAD_PRINT("update temperature[%d] prev[%d] new [%d]\n",
					  i, pqos->temperature[i], temp[i]);
				pqos->temperature[i] = temp[i];
			}
		}
		/* print out final temperature */
		NAD_PRINT("final temperature [BIG][MID][LIT][GPU][ISP] = ");
		for (i = 0; i < MAX_TMU_COUNT; i++)
			pr_info("[%d]", pqos->temperature[i]);
		pr_info("\n");
	}
}

static int on_run(void *data)
{
	struct nad_balancer_pm_qos *pqos = data;
	int delay = pqos->delay_time;
	int policy = pqos->policy;
	int idx = 0;
	int req_lit = 0;
	int req_mid = 0;
	int req_big = 0;
	int req_mif = 0;
	int temp_check_period = 0;

	NAD_PRINT("%s start.\n", pqos->desc);
	while (!kthread_should_stop()) {
		if (!sec_nad_balancer_enabled()) {
			NAD_PRINT("%s no more working nad balancer qos thread.\n", pqos->desc);
			if (!strncmp(pqos->desc, "LIT", 3) && req_lit == 1)
				REMOVE_PM_QOS(&pqos->lit_qos);
			if (!strncmp(pqos->desc, "MID", 3) && req_mid == 1)
				REMOVE_PM_QOS(&pqos->mid_qos);
			if (!strncmp(pqos->desc, "BIG", 3) && req_big == 1)
				REMOVE_PM_QOS(&pqos->big_qos);
			if (!strncmp(pqos->desc, "MIF", 3) && req_mif == 1)
				REMOVE_PM_QOS(&pqos->mif_qos);

			/* update final temperature */
			if (!strncmp(pqos->desc, "LIT", 3))
				update_temperature(pqos, UPDATE_FINAL_TEMPERATURE);

			break;
		}

		if (!pqos->tables) {
			if (pqos->current_mode == NAD_BALANCER_MODE_SINGLE)
				idx = prandom_u32() % pqos->single_table_size;
			else if (pqos->current_mode == NAD_BALANCER_MODE_DUAL)
				idx = prandom_u32() % pqos->dual_table_size;
			else if (pqos->current_mode == NAD_BALANCER_MODE_QUAD)
				idx = prandom_u32() % pqos->quad_table_size;
		} else {
			idx = prandom_u32() % pqos->table_size;
		}

		/* limit qos cl0 min throughput */
		if (!strncmp(pqos->desc, "LIT", 3)) {
			if (req_lit == 0) {
				UPDATE_PM_QOS(&pqos->lit_qos, policy ?
					      PM_QOS_CLUSTER0_FREQ_MAX : PM_QOS_CLUSTER0_FREQ_MIN,
					      pqos->tables[idx].freq);
				req_lit = 1;
			} else {
				REMOVE_PM_QOS(&pqos->lit_qos);
				req_lit = 0;
			}
			temp_check_period++;
			if (temp_check_period % 50 == 0)
				update_temperature(pqos, UPDATE_CONTI_TEMPERATURE);
		}

		/* limit qos cl1 min throughput */
		if (!strncmp(pqos->desc, "MID", 3)) {
			if (req_mid == 0) {
				UPDATE_PM_QOS(&pqos->mid_qos, policy ?
					      PM_QOS_CLUSTER1_FREQ_MAX : PM_QOS_CLUSTER1_FREQ_MIN,
					      pqos->tables[idx].freq);
				req_mid = 1;
			} else {
				REMOVE_PM_QOS(&pqos->mid_qos);
				req_mid = 0;
			}
			temp_check_period++;
			if (temp_check_period % 50 == 0)
				update_temperature(pqos, UPDATE_CONTI_TEMPERATURE);
		}


		/* limit qos cl2 max throughput */
		if (!strncmp(pqos->desc, "BIG", 3)) {
			if (req_big == 0) {
				if (!pqos->tables) {
					if (pqos->current_mode == NAD_BALANCER_MODE_SINGLE) {
						//pr_info("single\n");
						UPDATE_PM_QOS(&pqos->big_qos, policy ?
							PM_QOS_CLUSTER2_FREQ_MAX : PM_QOS_CLUSTER2_FREQ_MIN,
							pqos->single_tables[idx].freq);

					} else if (pqos->current_mode == NAD_BALANCER_MODE_DUAL) {
						//pr_info("dual\n");
						UPDATE_PM_QOS(&pqos->big_qos, policy ?
							PM_QOS_CLUSTER2_FREQ_MAX : PM_QOS_CLUSTER2_FREQ_MIN,
							pqos->dual_tables[idx].freq);

					} else if (pqos->current_mode == NAD_BALANCER_MODE_QUAD) {
						//pr_info("quad\n");
						UPDATE_PM_QOS(&pqos->big_qos, policy ?
							PM_QOS_CLUSTER2_FREQ_MAX : PM_QOS_CLUSTER2_FREQ_MIN,
							pqos->quad_tables[idx].freq);
					}

				} else {
					//pr_info("normal\n");
					UPDATE_PM_QOS(&pqos->big_qos, policy ?
						PM_QOS_CLUSTER2_FREQ_MAX : PM_QOS_CLUSTER2_FREQ_MIN,
						pqos->tables[idx].freq);

				}
				req_big = 1;
			} else {
				REMOVE_PM_QOS(&pqos->big_qos);
				req_big = 0;
			}
		}

		/* limit bus max throughput */
		if (!strncmp(pqos->desc, "MIF", 3)) {
			if (req_mif == 0) {
				UPDATE_PM_QOS(&pqos->mif_qos, policy ?
					      PM_QOS_BUS_THROUGHPUT_MAX : PM_QOS_BUS_THROUGHPUT,
					      pqos->tables[idx].freq);
				req_mif = 1;
			} else {
				REMOVE_PM_QOS(&pqos->mif_qos);
				req_mif = 0;
			}
		}

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout_interruptible(msecs_to_jiffies(delay));
		set_current_state(TASK_RUNNING);
	}
	NAD_PRINT("%s thread done.\n", pqos->desc);

	return 0;
}

static int get_policy_num(char *str)
{
	if (!strncmp(str, "max", 3))
		return NAD_MAX_FREQ;
	else
		return NAD_MIN_FREQ;
}

static ssize_t store_nad_balancer(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct nad_balancer_info *info = dev_get_drvdata(dev);
	struct nad_balancer_pm_qos *pqos;
	char cmd_temp[NAD_BUFF_SIZE];
	char nad_cmd[BALANCER_CMD][NAD_BUFF_SIZE];
	char *nad_ptr, *string;
	int i, j, idx = 0, ret = -1, expire_time;
	unsigned int len = 0;

	/* Copy buf to nad cmd */
	len = (unsigned int)min(count, sizeof(cmd_temp) - 1);
	strncpy(cmd_temp, buf, len);
	cmd_temp[len] = '\0';
	string = cmd_temp;

	/* Parse AT CMD */
	while (idx < BALANCER_CMD) {
		nad_ptr = strsep(&string, ",");
		if (nad_ptr ==  NULL || strlen(nad_ptr) >= NAD_BUFF_SIZE) {
				NAD_PRINT(" %s: invalid input\n",__func__);
				return -EINVAL;	
		}
		strcpy(nad_cmd[idx++], nad_ptr);
	}

	/* Get thread expire time */
	ret = sscanf(nad_cmd[1], "%d\n", &expire_time);
	if (ret != 1)
		return -EINVAL;

#if defined(DEBUG_NAD_BALANCER)
	NAD_PRINT("cmd : %s, expire_time : %d\n", nad_cmd[0], expire_time);

	idx = 0;
	while (idx < BALANCER_CMD) {
		NAD_PRINT("cmd[%d] : %s\n", idx, nad_cmd[idx]);
		idx++;
	}

	for (i = 0; i < info->pdata->nQos; i++) {
		pqos = &info->pdata->qos_items[i];
		NAD_PRINT("%s freq table\n", pqos->desc);

		for(j = 0; j < pqos->table_size; j++) {
			NAD_PRINT("%d\n", pqos->tables[j].freq);
		}
	}

#endif
	sleep_test_enable = true;


	if (!strncmp(buf, "start", 5)) {
		/* Enable qos thread */
		if (sec_nad_balancer_enabled()) {
			NAD_PRINT("already running skip start nad balancer!\n");
			return count;
		}
		sec_nad_balancer_enable();
		for (i = 0; i < info->pdata->nQos; i++) {
			pqos = &info->pdata->qos_items[i];
			if (i == 0)
				update_temperature(pqos, UPDATE_FIRST_TEMPERATURE);
			/* set qos policy */
			info->pdata->qos_items[i].policy =
				get_policy_num(nad_cmd[i + 2]);
#if defined(DEBUG_NAD_BALANCER)
			NAD_PRINT("%s thread use %s qos policy\n",
				  info->pdata->qos_items[i].desc, nad_cmd[i + 2]);
#endif

			info->pdata->qos_items[i].thread = kthread_run(on_run, pqos, "nad_balancer_qos_thread");
			if (IS_ERR_OR_NULL(info->pdata->qos_items[i].thread)) {
				NAD_PRINT("Failed in creation of thread.\n");
				return count;
			}
		}

		/* Trigger delayed workqueue */
		schedule_delayed_work(&info->sec_nad_balancer_work, HZ * expire_time);
	}
	return count;
}

static ssize_t show_nad_balancer(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	NAD_PRINT("%s\n", __func__);
	sec_nad_balancer_disable();
	sleep_test_enable = false;

	return sprintf(buf, "OK\n");
}
static DEVICE_ATTR(balancer, 0644, show_nad_balancer, store_nad_balancer);

static ssize_t store_nad_status(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char *envp[2] = {"NAD_BALANCER_UEVENT", NULL};

	NAD_PRINT("%s\n", __func__);

	kobject_uevent_env(&sec_nad_balancer->kobj, KOBJ_CHANGE, envp);
	return count;
}

static ssize_t show_nad_status(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	NAD_PRINT("%s\n", __func__);

	return sprintf(buf, "OK\n");
}

static DEVICE_ATTR(status, 0644, show_nad_status, store_nad_status);


static ssize_t store_nad_print_log(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	NAD_PRINT("%s\n", __func__);
	NAD_PRINT("%s\n", buf);

	return count;
}

static ssize_t show_nad_print_log(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	NAD_PRINT("%s\n", __func__);

	return sprintf(buf, "OK\n");
}

static DEVICE_ATTR(print_log, 0644, show_nad_print_log, store_nad_print_log);

static ssize_t store_nad_timeout(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	NAD_PRINT("%s\n", __func__);

	return count;
}

static ssize_t show_nad_timeout(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct nad_balancer_info *pinfo = dev_get_drvdata(dev);
	int timeout_val = pinfo->pdata->timeout;

	NAD_PRINT("%s: timeout val (%d)\n", __func__, timeout_val);

	return sprintf(buf, "%d\n", timeout_val);
}

static DEVICE_ATTR(timeout, 0644, show_nad_timeout, store_nad_timeout);

static ssize_t store_nad_big_turbo_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct nad_balancer_info *pinfo = dev_get_drvdata(dev);
	int mode;
	int ret, i;

	NAD_PRINT("%s\n", __func__);

	ret = kstrtoint(buf, 10, &mode);
        if (ret) {
                pr_err("Invalid input %s ret %d\n", buf, ret);
                return 0;
        }

	if (!((mode == NAD_BALANCER_MODE_SINGLE) || (mode == NAD_BALANCER_MODE_DUAL)
		|| (mode == NAD_BALANCER_MODE_QUAD))) {
		pr_err("nad balancer big rutbo only support below mode\n");
		pr_err("1. single\n");
		pr_err("2. dual\n");
		pr_err("4. quad\n");
		return 0;
	}

	for (i = 0; i < pinfo->pdata->nQos; i++) {
		if (!pinfo->pdata->qos_items[i].tables) {
			pinfo->pdata->qos_items[i].current_mode = mode;
			pr_info("update big turbo mode[%s][%d]\n",
				pinfo->pdata->qos_items[i].desc,
				pinfo->pdata->qos_items[i].current_mode);
			break;
		}
	}
	return count;
}

static ssize_t show_nad_big_turbo_mode(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct nad_balancer_info *pinfo = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < pinfo->pdata->nQos; i++) {
		if (!pinfo->pdata->qos_items[i].tables) {
			break;
		}
	}
	NAD_PRINT("%s: current mode (%s)(%d)\n", __func__,
			 pinfo->pdata->qos_items[i].desc,
			 pinfo->pdata->qos_items[i].current_mode);
	return sprintf(buf, "%d\n", pinfo->pdata->qos_items[i].current_mode);
}

static DEVICE_ATTR(big_turbo_mode, 0644, show_nad_big_turbo_mode, store_nad_big_turbo_mode);


static const struct dev_pm_ops sec_nad_balancer_pm = {
	.prepare = sec_nad_balancer_prepare,
	.resume = sec_nad_balancer_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id sec_nad_balancer_dt_match[] = {
	{ .compatible = "samsung,sec_nad_balancer" },
	{ }
};
#endif

static struct platform_driver sec_nad_balancer_driver = {
	.probe = sec_nad_balancer_probe,
	.remove = sec_nad_balancer_remove,
	.driver = {
		.name = "sec_nad_balancer",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &sec_nad_balancer_pm,
#endif
#if CONFIG_OF
		.of_match_table = of_match_ptr(sec_nad_balancer_dt_match),
#endif
	},
};

static int sec_nad_balancer_prepare(struct device *dev)
{
	return 0;
}

static int sec_nad_balancer_resume(struct device *dev)
{
	return 0;
}

static int sec_nad_balancer_remove(struct platform_device *pdev)
{
	return 0;
}

static int sec_nad_balancer_probe(struct platform_device *pdev)
{
	struct nad_balancer_platform_data *pdata;
	struct nad_balancer_info *pinfo;
	int ret = 0;

	NAD_PRINT("%s\n", __func__);
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct nad_balancer_platform_data), GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate platform data\n");
			return -ENOMEM;
		}

		pdev->dev.platform_data = pdata;
		ret = nad_balancer_parse_dt(&pdev->dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data\n");
			return ret;
		}

		pr_info("%s: parse dt done\n", __func__);
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data\n");
		return -EINVAL;
	}

	if (!pdata->nItem) {
		dev_err(&pdev->dev, "There are no devices\n");
		return -EINVAL;
	}

	pinfo = devm_kzalloc(&pdev->dev, sizeof(struct nad_balancer_info), GFP_KERNEL);

	if (!pinfo)
		return -ENOMEM;

	pinfo->dev = sec_device_create(pinfo, "sec_nad_balancer");
	if (IS_ERR(sec_nad_balancer)) {
		pr_err("%s Failed to create device(sec_nad_balancer)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}
	ret = device_create_file(pinfo->dev, &dev_attr_balancer);
	if (ret) {
		pr_err("%s: Failed to create balnacer device file\n", __func__);
		goto err_create_nad_balancer_sysfs;
	}
	ret = device_create_file(pinfo->dev, &dev_attr_status);
	if (ret) {
		pr_err("%s: Failed to create status device file\n", __func__);
		goto err_create_nad_balancer_sysfs;
	}
	ret = device_create_file(pinfo->dev, &dev_attr_print_log);
	if (ret) {
		pr_err("%s: Failed to create print log device file\n", __func__);
		goto err_create_nad_balancer_sysfs;
	}

	ret = device_create_file(pinfo->dev, &dev_attr_timeout);
	if (ret) {
		pr_err("%s: Failed to create timeout device file\n", __func__);
		goto err_create_nad_balancer_sysfs;
	}
	ret = device_create_file(pinfo->dev, &dev_attr_big_turbo_mode);
	if (ret) {
		pr_err("%s: Failed to create big_turbo_mode device file\n", __func__);
		goto err_create_nad_balancer_sysfs;
	}

	INIT_DELAYED_WORK(&pinfo->sec_nad_balancer_work,
			  sec_nad_balancer_timeout_work);

	pinfo->pdata = pdata;
	sec_nad_balancer = pinfo->dev;
	platform_set_drvdata(pdev, pinfo);

	return ret;

err_create_nad_balancer_sysfs:
	sec_device_destroy(sec_nad_balancer->devt);
out:
	if (!pinfo)
		devm_kfree(&pdev->dev, pinfo);
	if (!pdata)
		devm_kfree(&pdev->dev, pdata);
	return ret;
}
#endif

static int __init sec_nad_balancer_init(void)
{
#if defined(CONFIG_SEC_FACTORY)
	NAD_PRINT("%s\n", __func__);
	return platform_driver_register(&sec_nad_balancer_driver);
#else
	NAD_PRINT("Not support NAD balancer.\n");
	return 0;
#endif
}

static void __exit sec_nad_balancer_exit(void)
{
#if defined(CONFIG_SEC_FACTORY)
	NAD_PRINT("%s\n", __func__);
	return platform_driver_unregister(&sec_nad_balancer_driver);
#endif
}

module_init(sec_nad_balancer_init);
module_exit(sec_nad_balancer_exit);

MODULE_DESCRIPTION("Samsung NAD balancer Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
