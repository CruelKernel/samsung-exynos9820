#include <linux/input/input_booster.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>

spinlock_t write_ib_lock;
spinlock_t write_qos_lock;
spinlock_t ib_type_lock;
struct mutex trigger_ib_lock;

struct workqueue_struct *ev_unbound_wq;
struct workqueue_struct *ib_unbound_highwq;

int total_ib_cnt = 0;
int ib_init_succeed = 0;

int level_value = IB_MAX;

unsigned int debug_flag = 0;
unsigned int enable_event_booster = INIT_ZERO;

// Input Booster Init Variables
int release_val[MAX_RES_COUNT];
int device_count = 0;
struct t_ib_device_tree *ib_device_trees;
struct t_ib_trigger *ib_trigger;
int max_resource_size;

struct list_head *ib_list;
struct list_head *qos_list;

// @evdev_mt_slot : save the number of inputed touch slot.
int evdev_mt_slot = 0;
// @evdev_mt_event[] : save count of each boooter's events.
int evdev_mt_event[MAX_DEVICE_TYPE_NUM];
int trigger_cnt = 0;
int send_ev_enable = 0;

struct t_ib_info *find_release_ib(int dev_type, int key_id);
struct t_ib_info *create_ib_instance(struct t_ib_trigger *p_IbTrigger, int uniqId);
bool is_validate_uniqid(int dev_type, unsigned int uniq_id);
struct t_ib_target *find_update_target(int uniq_id, int res_id);
unsigned long get_qos_value(int res_id);
void remove_ib_instance(struct t_ib_info *ib);

void trigger_input_booster(struct work_struct *work)
{
	unsigned int uniq_id = 0;
	int res_type = -1;

	struct t_ib_info *ib;
	struct t_ib_trigger *p_IbTrigger = container_of(work, struct t_ib_trigger, ib_trigger_work);

	if (p_IbTrigger == NULL) {
		return;
	}

	pr_booster("IB Trigger :: %s(%d) %s || key_id : %d\n =========================",
	ib_device_trees[p_IbTrigger->dev_type].label, p_IbTrigger->dev_type,
	(p_IbTrigger->event_type) ? "PRESS" : "RELEASE", p_IbTrigger->key_id);

	mutex_lock(&trigger_ib_lock);

	// Input booster On/Off handling
	if (p_IbTrigger->event_type == BOOSTER_ON) {

		if (find_release_ib(p_IbTrigger->dev_type, p_IbTrigger->key_id) != NULL) {
			pr_err(ITAG" IB Trigger :: ib already exist. Key(%d)", p_IbTrigger->key_id);
			mutex_unlock(&trigger_ib_lock);
			return;
		}

		// Check if uniqId exits.
		do {
			uniq_id = total_ib_cnt++;

			if (total_ib_cnt == MAX_IB_COUNT)
				total_ib_cnt = 0;

		} while (!is_validate_uniqid(p_IbTrigger->dev_type, uniq_id));

		// Make ib instance with all needed factor.
		ib = create_ib_instance(p_IbTrigger, uniq_id);
		pr_booster("IB Uniq Id(%d)", uniq_id);

		if (ib == NULL) {
			mutex_unlock(&trigger_ib_lock);
			return;
		}

		ib->press_flag = FLAG_ON;

		// When create ib instance, insert resource info in qos list with value 0.
		for (res_type = 0; res_type < max_resource_size; res_type++) {
			if (ib != NULL && ib->ib_dt->res[res_type].head_value != 0) {
				struct t_ib_target* tv;
				tv = kmalloc(sizeof(struct t_ib_target), GFP_KERNEL);

				if (tv == NULL)
					continue;

				tv->uniq_id = ib->uniq_id;
				tv->value = 0;

				spin_lock(&write_qos_lock);
				list_add_tail_rcu(&(tv->list), &qos_list[res_type]);
				spin_unlock(&write_qos_lock);
			}
		}

		queue_work(ib_unbound_highwq, &(ib->ib_state_work[IB_HEAD]));

	} else {
		/*  Find ib instance in the list. if not, ignore this event.
		 *  if exists, Release flag on. Call ib's Release func.
		 */

		ib = find_release_ib(p_IbTrigger->dev_type, p_IbTrigger->key_id);

		if (ib == NULL) {
			pr_err("IB is null on release");
			mutex_unlock(&trigger_ib_lock);
			return;
		}
		pr_booster("IB Trigger Release :: Uniq ID(%d)", ib->uniq_id);

		mutex_lock(&ib->lock);

		ib->rel_flag = FLAG_ON;

		// If head operation is already finished, tail timeout work will be triggered.
		if (ib->isHeadFinished) {
			if (!delayed_work_pending(&(ib->ib_timeout_work[IB_TAIL]))) {
				queue_delayed_work(ib_unbound_highwq,
					&(ib->ib_timeout_work[IB_TAIL]),
					msecs_to_jiffies(ib->ib_dt->tail_time));
			} else {
				pr_err(ITAG" IB Trigger Release :: tail timeout start");
			}
		}
		mutex_unlock(&ib->lock);

	}
	mutex_unlock(&trigger_ib_lock);

}

struct t_ib_info *create_ib_instance(struct t_ib_trigger *p_IbTrigger, int uniqId)
{
	struct t_ib_info *ib = kmalloc(sizeof(struct t_ib_info), GFP_KERNEL);
	int dev_type = p_IbTrigger->dev_type;

	if (ib == NULL)
		return NULL;

	ib->dev_name = p_IbTrigger->dev_name;
	ib->key_id = p_IbTrigger->key_id;
	ib->uniq_id = uniqId;
	ib->press_flag = FLAG_OFF;
	ib->rel_flag = FLAG_OFF;
	ib->isHeadFinished = 0;

	ib->ib_dt = &ib_device_trees[dev_type];

	INIT_WORK(&ib->ib_state_work[IB_HEAD], press_state_func);
	INIT_DELAYED_WORK(&ib->ib_timeout_work[IB_HEAD], press_timeout_func);
	INIT_WORK(&ib->ib_state_work[IB_TAIL], release_state_func);
	INIT_DELAYED_WORK(&ib->ib_timeout_work[IB_TAIL], release_timeout_func);
	mutex_init(&ib->lock);

	spin_lock(&write_ib_lock);
	list_add_tail_rcu(&(ib->list), &ib_list[dev_type]);
	spin_unlock(&write_ib_lock);

	return ib;
}

bool is_validate_uniqid(int dev_type, unsigned int uniq_id)
{
	int cnt = 0;
	struct t_ib_info *ib = NULL;
	rcu_read_lock();

	if (list_empty(&ib_list[dev_type])) {
		rcu_read_unlock();
		pr_booster("IB list empty");
		return true;
	}

	list_for_each_entry_rcu(ib, &ib_list[dev_type], list) {
		cnt++;
		if (ib != NULL && ib->uniq_id == uniq_id) {
			rcu_read_unlock();
			pr_booster("uniq id find :: IB Idx(%d) old(%d) new(%d)", cnt, ib->uniq_id, uniq_id);
			return false;
		}
	}

	rcu_read_unlock();
	pr_booster("This can be used(IB Total:%d)", cnt);
	return true;
}

struct t_ib_info *find_release_ib(int dev_type, int key_id)
{
	struct t_ib_info *ib = NULL;
	rcu_read_lock();
	if (list_empty(&ib_list[dev_type])) {
		rcu_read_unlock();
		pr_booster("Release IB(%d) Not Exist & List Empty", key_id);
		return NULL;
	}

	list_for_each_entry_rcu(ib, &ib_list[dev_type], list) {
		if (ib != NULL && ib->key_id == key_id && ib->rel_flag == FLAG_OFF) {
			rcu_read_unlock();
			pr_booster("Release IB(%d) Found", key_id);
			return ib;
		}
	}

	rcu_read_unlock();
	pr_booster("Release IB(%d) Not Exist", key_id);
	return NULL;

}

void press_state_func(struct work_struct *work)
{

	struct t_ib_res_info res;
	struct t_ib_target *tv;
	int qos_values[MAX_RES_COUNT] = { 0, };
	int res_type = 0;

	struct t_ib_info *target_ib = container_of(work, struct t_ib_info, ib_state_work[IB_HEAD]);

	pr_booster("Press State Func :::: Unique_Id(%d)", target_ib->uniq_id);

	// //To-Do : Get_Res_List(head) and update head value.
	for (res_type = 0; res_type < max_resource_size; res_type++) {
		res = target_ib->ib_dt->res[res_type];
		if (res.head_value == 0)
			continue;

		//find already added target value instance and update value as a head.
		tv = find_update_target(target_ib->uniq_id, res.res_id);

		if (tv == NULL) {
			pr_booster("Press State Func :::: tv is null T.T");
			continue;
		}

		tv->value = res.head_value;
		pr_booster("Press State Func :::: Uniq(%d)'s Update Res(%d) Head Val(%d)",
			tv->uniq_id, res.res_id, res.head_value);

		qos_values[res.res_id] = get_qos_value(res.res_id);

	}

	ib_set_booster(qos_values);
	pr_booster("Press State Func :::: Press Delay Time(%lu)",
		msecs_to_jiffies(target_ib->ib_dt->head_time));

	queue_delayed_work(ib_unbound_highwq, &(target_ib->ib_timeout_work[IB_HEAD]),
		msecs_to_jiffies(target_ib->ib_dt->head_time));
}

void press_timeout_func(struct work_struct *work)
{

	struct t_ib_info *target_ib = container_of(work, struct t_ib_info, ib_timeout_work[IB_HEAD].work);

	pr_booster("Press Timeout Func :::: Unique_Id(%d)", target_ib->uniq_id);

	int res_type;
	struct t_ib_res_info res;
	struct t_ib_target *tv;
	int qos_values[MAX_RES_COUNT] = { 0, };

	if (target_ib->ib_dt->tail_time != 0) {
		mutex_lock(&target_ib->lock);
		target_ib->isHeadFinished = 1;
		queue_work(ib_unbound_highwq, &(target_ib->ib_state_work[IB_TAIL]));
		mutex_unlock(&target_ib->lock);
	}
	else {

		//NO TAIL Scenario : Delete Ib instance and free all memory space.
		for (res_type = 0; res_type < max_resource_size; res_type++) {
			res = target_ib->ib_dt->res[res_type];

			tv = find_update_target(target_ib->uniq_id, res.res_id);
			if (tv == NULL)
				continue;

			spin_lock(&write_qos_lock);
			list_del_rcu(&(tv->list));
			spin_unlock(&write_qos_lock);
			synchronize_rcu();
			kfree(tv);

			rcu_read_lock();
			if (!list_empty(&qos_list[res.res_id])) {
				rcu_read_unlock();
				qos_values[res.res_id] = get_qos_value(res.res_id);
				pr_booster("Press Timeout ::: Remove Val Cuz No Tail ::: Res(%d) Qos Val(%d)",
					res.res_id, qos_values[res.res_id]);
			}
			else {
				rcu_read_unlock();
				pr_booster("Press Timeout ::: Release Booster(%d) ::: No Tail and List Empty",
					res.res_id);
				ib_release_booster(res.res_id);
			}
		}
		remove_ib_instance(target_ib);
		ib_set_booster(qos_values);
	}

}

void release_state_func(struct work_struct *work)
{

	int qos_values[MAX_RES_COUNT] = { 0, };
	int isHeadFinished = -1;
	int res_type = 0;
	struct t_ib_target *tv;
	struct t_ib_res_info res;

	struct t_ib_info *target_ib = container_of(work, struct t_ib_info, ib_state_work[IB_TAIL]);

	mutex_lock(&target_ib->lock);

	isHeadFinished = target_ib->isHeadFinished;

	pr_booster("Release State Func :::: Unique_Id(%d) HeadFinish(%d) Rel_Flag(%d)",
		target_ib->uniq_id, isHeadFinished, target_ib->rel_flag)

	for (res_type = 0; res_type < max_resource_size; res_type++) {
		res = target_ib->ib_dt->res[res_type];
		if (res.tail_value == 0)
			continue;

		tv = find_update_target(target_ib->uniq_id, res.res_id);
		if (tv == NULL)
			continue;

		spin_lock(&write_qos_lock);
		tv->value = res.tail_value;
		spin_unlock(&write_qos_lock);

		pr_booster("Release State Func :::: Uniq(%d)'s Update Tail Val (%d), Qos_Val(%d)", tv->uniq_id, tv->value, qos_values[res.res_id]);
		qos_values[res.res_id] = get_qos_value(res.res_id);
	}

	ib_set_booster(qos_values);

	// If release event already triggered, tail delay work will be triggered after relese state func.
	if (target_ib->rel_flag == FLAG_ON) {
		if (!delayed_work_pending(&(target_ib->ib_timeout_work[IB_TAIL]))) {
			queue_delayed_work(ib_unbound_highwq,
				&(target_ib->ib_timeout_work[IB_TAIL]),
				msecs_to_jiffies(target_ib->ib_dt->tail_time));
		} else {
			pr_err(ITAG" Release State Func :: tail timeout start");
		}
	}
	mutex_unlock(&target_ib->lock);
}

void release_timeout_func(struct work_struct *work)
{
	int qos_values[MAX_RES_COUNT] = { 0, };
	struct t_ib_target *tv;
	struct t_ib_res_info res;
	int res_type;

	struct t_ib_info *target_ib = container_of(work, struct t_ib_info, ib_timeout_work[IB_TAIL].work);
	pr_booster("Release Timeout Func :::: Unique_Id(%d)", target_ib->uniq_id);

	// Remove all booster
	// delete instance in the ib list and delete instance in the qos list.

	for (res_type = 0; res_type < max_resource_size; res_type++) {
		res = target_ib->ib_dt->res[res_type];

		tv = find_update_target(target_ib->uniq_id, res.res_id);
		if (tv == NULL)
			continue;

		pr_booster("Release Timeout Func :::: Delete Uniq(%d)'s TV Val (%d)",
			tv->uniq_id, tv->value);

		spin_lock(&write_qos_lock);
		list_del_rcu(&(tv->list));
		spin_unlock(&write_qos_lock);
		synchronize_rcu();
		kfree(tv);

		rcu_read_lock();
		if (!list_empty(&qos_list[res.res_id])) {
			rcu_read_unlock();
			qos_values[res.res_id] = get_qos_value(res.res_id);
			pr_booster("Release Timeout Func :::  Res(%d) Qos Val(%d)",
				res.res_id, qos_values[res.res_id]);
		}
		else {
			rcu_read_unlock();
			pr_booster("Release Timeout ::: Release Booster(%d) ::: List Empty",
				res.res_id);
			ib_release_booster(res.res_id);
		}
	}

	ib_set_booster(qos_values);
	remove_ib_instance(target_ib);

}

struct t_ib_target *find_update_target(int uniq_id, int res_id)
{
	struct t_ib_target *tv;

	rcu_read_lock();
	list_for_each_entry_rcu(tv, &qos_list[res_id], list) {
		if (tv->uniq_id == uniq_id) {
			rcu_read_unlock();
			return tv;
		}
	}
	rcu_read_unlock();

	return NULL;
}

unsigned long get_qos_value(int res_id)
{
	//Find tv instance that has max value in the qos_list that has the passed res_id.
	struct t_ib_target *tv;
	int ret_val = 0;

	rcu_read_lock();

	if (list_empty(&qos_list[res_id])) {
		rcu_read_unlock();
		return 0;
	}

	list_for_each_entry_rcu(tv, &qos_list[res_id], list) {
		if (tv->value > ret_val)
			ret_val = tv->value;
	}
	rcu_read_unlock();

	return ret_val;
}

void remove_ib_instance(struct t_ib_info *ib)
{
	pr_booster("Del Ib Instance's Id : %d", ib->uniq_id);
	//Delete Kmalloc instances

	spin_lock(&write_ib_lock);
	list_del_rcu(&(ib->list));
	spin_unlock(&write_ib_lock);
	synchronize_rcu();
	kfree(ib);
}

unsigned int create_uniq_id(int type, int code, int slot)
{
	//id1 | (id2 << num_bits_id1) | (id3 << (num_bits_id2 + num_bits_id1))
	pr_booster("Create Key Id -> type(%d), code(%d), slot(%d)", type, code, slot);
	return (type << (TYPE_BITS + CODE_BITS)) | (code << CODE_BITS) | slot;
}

void ib_auto_test(int type, int code, int val)
{
	send_ev_enable = 1;
}


//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR SYSFS  +++++++++++++++++++++++++++++++++++++++++++++++//
SYSFS_CLASS(enable_event, (buf, "%u\n", enable_event), 1)
SYSFS_CLASS(debug_level, (buf, "%u\n", debug_level), 1)
SYSFS_CLASS(sendevent, (buf, "%d\n", sendevent), 3)
HEAD_TAIL_SYSFS_DEVICE(head)
HEAD_TAIL_SYSFS_DEVICE(tail)
LEVEL_SYSFS_DEVICE(level)

struct attribute *dvfs_attributes[] = {
	&dev_attr_head.attr,
	&dev_attr_tail.attr,
	&dev_attr_level.attr,
	NULL,
};

struct attribute_group dvfs_attr_group = {
	.attrs = dvfs_attributes,
};

void init_sysfs_device(struct class *sysfs_class, struct t_ib_device_tree *ib_dt)
{
	struct device *sysfs_dev;
	int ret = 0;
	sysfs_dev = device_create(sysfs_class, NULL, 0, ib_dt, "%s", ib_dt->label);
	if (IS_ERR(sysfs_dev)) {
		ret = IS_ERR(sysfs_dev);
		pr_booster("[Input Booster] Failed to create %s sysfs device[%d]n", ib_dt->label, ret);
		return;
	}
	ret = sysfs_create_group(&sysfs_dev->kobj, &dvfs_attr_group);
	if (ret) {
		pr_booster("[Input Booster] Failed to create %s sysfs groupn", ib_dt->label);
		return;
	}
}

int is_ib_init_succeed(void)
{
	return (ib_trigger != NULL && ib_device_trees != NULL &&
		ib_list != NULL && qos_list != NULL) ? 1 : 0;
}


void input_booster_exit(void)
{
	int i = 0;

	kfree(ib_trigger);
	kfree(ib_device_trees);
	kfree(ib_list);
	kfree(qos_list);
	input_booster_exit_vendor();
}

// ********** Init Booster ********** //

void input_booster_init(void)
{
	// ********** Load Frequency data from DTSI **********
	struct device_node *np;
	int i;

	int ib_dt_size = sizeof(struct t_ib_device_tree);
	int ib_res_size = sizeof(struct t_ib_res_info);
	int list_head_size = sizeof(struct list_head);
	int ddr_info_size = sizeof(struct t_ddr_info);

	int res_type = 0;
	int ndevice_in_dt = 0;
	char rel_val_str[100];
	size_t rel_val_size = 0;
	char *rel_val_pointer = NULL;
	const char *token = NULL;

	spin_lock_init(&write_ib_lock);
	spin_lock_init(&write_qos_lock);
	spin_lock_init(&ib_type_lock);
	mutex_init(&trigger_ib_lock);

	ev_unbound_wq =
		alloc_ordered_workqueue("ev_unbound_wq", WQ_HIGHPRI);

	ib_unbound_highwq =
		alloc_workqueue("ib_unbound_high_wq", WQ_UNBOUND | WQ_HIGHPRI,
					 MAX_IB_COUNT);

	if (ev_unbound_wq == NULL || ib_unbound_highwq == NULL)
		goto out;

	//Input Booster Trigger Strcut Init
	ib_trigger = kcalloc(ABS_CNT, sizeof(struct t_ib_trigger) * MAX_IB_COUNT, GFP_KERNEL);
	if (ib_trigger == NULL) {
		pr_err(ITAG" ib_trigger mem alloc fail");
		goto out;
	}

	for (i = 0; i < MAX_IB_COUNT; i++) {
		INIT_WORK(&(ib_trigger[i].ib_trigger_work), trigger_input_booster);
	}

	np = of_find_compatible_node(NULL, NULL, "input_booster");
	if (np == NULL) {
		goto out;
	}

	// Geting the count of devices.
	ndevice_in_dt = of_get_child_count(np);
	pr_info(ITAG" %s   ndevice_in_dt : %d\n", __func__, ndevice_in_dt);


	ib_device_trees = kcalloc(ABS_CNT, ib_dt_size * ndevice_in_dt, GFP_KERNEL);
	if (ib_device_trees == NULL) {
		pr_err(ITAG" dt_infor mem alloc fail");
		goto out;
	}

	// ib list mem alloc
	ib_list = kcalloc(ABS_CNT, list_head_size * ndevice_in_dt, GFP_KERNEL);
	if (ib_list == NULL) {
		pr_err(ITAG" ib list mem alloc fail");
		goto out;
	}

	sscanf((of_get_property(np, "max_resource_count", NULL)), "%d", &max_resource_size);

	pr_info(ITAG" resource size : %d", max_resource_size);

	//qos list mem alloc
	qos_list = kcalloc(ABS_CNT, list_head_size * max_resource_size, GFP_KERNEL);
	if (qos_list == NULL) {
		pr_err(ITAG" ib list mem alloc fail");
		goto out;
	}

	//Init Resource Release Values
	const char *rel_vals = of_get_property(np, "ib_release_values", NULL);

	rel_val_size = strlcpy(rel_val_str, rel_vals, sizeof(char) * 100);
	rel_val_pointer = rel_val_str;
	token = strsep(&rel_val_pointer, ",");
	res_type = 0;

	while (token != NULL) {
		pr_booster("Rel %d's Type Value(%s)", res_type, token);

		//Release Values inserted inside array
		sscanf(token, "%d", &release_val[res_type]);

		//Init Qos List
		INIT_LIST_HEAD(&qos_list[res_type]);

		token = strsep(&rel_val_pointer, ",");
		res_type++;
	}

	if (res_type < max_resource_size) {
		pr_err(ITAG" release value parse fail");
		goto out;
	}

	struct device_node *cnp;

	for_each_child_of_node(np, cnp) {
		/************************************************/
		// fill all needed data into res_info instance that is in dt instance.
		struct t_ib_device_tree *ib_dt = (ib_device_trees + device_count);
		struct device_node *child_resource_node;
		struct device_node *resource_node = of_find_compatible_node(cnp, NULL, "resource");

		ib_dt->res = kcalloc(ABS_CNT, ib_res_size * max_resource_size, GFP_KERNEL);

		int resource_node_index = 0;
		int res_type = 0;
		for_each_child_of_node(resource_node, child_resource_node) {
			// resource_node_index is same as Resource's ID.
			ib_dt->res[resource_node_index].res_id = resource_node_index;
			ib_dt->res[resource_node_index].label = of_get_property(child_resource_node, "resource,label", NULL);

			int inputbooster_size = 0;

			const u32 *is_exist_inputbooster_size = of_get_property(child_resource_node, "resource,value", &inputbooster_size);

			if (is_exist_inputbooster_size && inputbooster_size) {
				inputbooster_size = inputbooster_size / sizeof(u32);
			}
			pr_info(ITAG" inputbooster_size : %d", inputbooster_size);

			if (inputbooster_size != 2)
				return; // error

			for (res_type = 0; res_type < inputbooster_size; ++res_type) {
				if (res_type == IB_HEAD) {
					of_property_read_u32_index(child_resource_node, "resource,value",
						res_type, &ib_dt->res[resource_node_index].head_value);
					pr_info(ITAG"res[%d]->values[%d] : %d", resource_node_index,
						res_type, ib_dt->res[resource_node_index].head_value);
				}
				else if (res_type == IB_TAIL) {
					of_property_read_u32_index(child_resource_node, "resource,value",
						res_type, &ib_dt->res[resource_node_index].tail_value);
					pr_info(ITAG"res[%d]->values[%d] : %d", resource_node_index,
						res_type, ib_dt->res[resource_node_index].tail_value);
				}
			}

			resource_node_index++;
		}

		ib_dt->label = of_get_property(cnp, "input_booster,label", NULL);
		pr_info(ITAG" %s   ib_dt->label : %s\n", __func__, ib_dt->label);

		if (of_property_read_u32(cnp, "input_booster,type", &ib_dt->type)) {
			pr_err(ITAG" Failed to get type property\n");
			break;
		}
		if (of_property_read_u32(cnp, "input_booster,head_time", &ib_dt->head_time)) {
			pr_err(ITAG" Fail Get Head Time\n");
			break;
		}
		if (of_property_read_u32(cnp, "input_booster,tail_time", &ib_dt->tail_time)) {
			pr_err(ITAG" Fail Get Tail Time\n");
			break;
		}

		//Init all type of ib list.
		INIT_LIST_HEAD(&ib_list[device_count]);

		device_count++;
	}

	ib_init_succeed = is_ib_init_succeed();

out:
	// ********** Initialize Sysfs **********
	{
		struct class *sysfs_class;
		int ret;
		int ib_type;
		sysfs_class = class_create(THIS_MODULE, "input_booster");

		if (IS_ERR(sysfs_class)) {
			pr_err(" Failed to create class\n");
			return;
		}
		if (ib_init_succeed) {
			INIT_SYSFS_CLASS(enable_event)
			INIT_SYSFS_CLASS(debug_level)
			INIT_SYSFS_CLASS(sendevent)

			for (ib_type = 0; ib_type < MAX_DEVICE_TYPE_NUM; ib_type++) {
				init_sysfs_device(sysfs_class, &ib_device_trees[ib_type]);
			}
		}
	}
#if !defined(CONFIG_ARCH_QCOM) && !defined (CONFIG_ARCH_EXYNOS)
	pr_err(ITAG" At least, one vendor feature needed\n");
#else
	input_booster_init_vendor(release_val);
#endif
}
