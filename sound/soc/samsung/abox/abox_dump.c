/* sound/soc/samsung/abox/abox_dump.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Internal Buffer Dumping driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/pm_runtime.h>
#include <sound/samsung/abox.h>

#include "abox_util.h"
#include "abox.h"
#include "abox_dbg.h"
#include "abox_log.h"

#define BUFFER_MAX (SZ_64)
#define NAME_LENGTH (SZ_32)

struct abox_dump_buffer_info {
	struct device *dev;
	struct list_head list;
	int id;
	char name[NAME_LENGTH];
	struct mutex lock;
	struct snd_dma_buffer buffer;
	struct snd_pcm_substream *substream;
	size_t pointer;
	bool started;
	bool auto_started;
	bool file_created;
	struct file *filp;
	ssize_t auto_pointer;
	struct work_struct auto_work;
};

static struct device *abox_dump_dev_abox;
static struct abox_dump_buffer_info abox_dump_list[BUFFER_MAX];
static LIST_HEAD(abox_dump_list_head);

static struct abox_dump_buffer_info *abox_dump_get_buffer_info(int id)
{
	struct abox_dump_buffer_info *info;

	list_for_each_entry(info, &abox_dump_list_head, list) {
		if (info->id == id)
			return info;
	}

	return NULL;
}

static struct abox_dump_buffer_info *abox_dump_get_buffer_info_by_name(
		const char *name)
{
	struct abox_dump_buffer_info *info;

	list_for_each_entry(info, &abox_dump_list_head, list) {
		if (strncmp(info->name, name, sizeof(info->name)) == 0)
			return info;
	}

	return NULL;
}

static void abox_dump_request_dump(int id)
{
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system = &msg.msg.system;
	bool start = info->started || info->auto_started;

	dev_dbg(abox_dump_dev_abox, "%s(%d)\n", __func__, id);

	msg.ipcid = IPC_SYSTEM;
	system->msgtype = ABOX_REQUEST_DUMP;
	system->param1 = id;
	system->param2 = start ? 1 : 0;
	abox_request_ipc(abox_dump_dev_abox, msg.ipcid, &msg, sizeof(msg),
			1, 0);
}

static ssize_t abox_dump_auto_read(struct file *file, char __user *data,
		size_t count, loff_t *ppos, bool enable)
{
	struct abox_dump_buffer_info *info;
	char buffer[SZ_512] = {0,}, *buffer_p = buffer;

	dev_dbg(abox_dump_dev_abox, "%s(%zu, %lld, %d)\n", __func__, count,
			*ppos, enable);

	list_for_each_entry(info, &abox_dump_list_head, list) {
		if (info->auto_started == enable) {
			buffer_p += snprintf(buffer_p, sizeof(buffer) -
					(buffer_p - buffer),
					"%d(%s) ", info->id, info->name);
		}
	}
	snprintf(buffer_p, 2, "\n");

	return simple_read_from_buffer(data, count, ppos, buffer,
			buffer_p - buffer);
}

static ssize_t abox_dump_auto_write(struct file *file, const char __user *data,
		size_t count, loff_t *ppos, bool enable)
{
	char buffer[SZ_512] = {0,}, name[NAME_LENGTH];
	char *p_buffer = buffer, *token = NULL;
	unsigned int id;
	struct abox_dump_buffer_info *info;
	ssize_t ret;

	dev_dbg(abox_dump_dev_abox, "%s(%zu, %lld, %d)\n", __func__, count,
			*ppos, enable);

	ret = simple_write_to_buffer(buffer, sizeof(buffer), ppos, data, count);
	if (ret < 0)
		return ret;

	while ((token = strsep(&p_buffer, " ")) != NULL) {
		if (sscanf(token, "%11u", &id) == 1)
			info = abox_dump_get_buffer_info(id);
		else if (sscanf(token, "%31s", name) == 1)
			info = abox_dump_get_buffer_info_by_name(name);
		else
			info = NULL;

		if (IS_ERR_OR_NULL(info)) {
			dev_err(abox_dump_dev_abox, "invalid argument\n");
			continue;
		}

		if (info->auto_started != enable) {
			struct device *dev = info->dev;

			if (enable)
				pm_runtime_get_sync(dev);
			else
				pm_runtime_put(dev);
		}

		info->auto_started = enable;
		if (enable) {
			info->file_created = false;
			info->pointer = info->auto_pointer = 0;
		}

		abox_dump_request_dump(info->id);
	}

	return count;
}

static ssize_t abox_dump_auto_start_read(struct file *file,
		char __user *data, size_t count, loff_t *ppos)
{
	return abox_dump_auto_read(file, data, count, ppos, true);
}

static ssize_t abox_dump_auto_start_write(struct file *file,
		const char __user *data, size_t count, loff_t *ppos)
{
	return abox_dump_auto_write(file, data, count, ppos, true);
}

static ssize_t abox_dump_auto_stop_read(struct file *file,
		char __user *data, size_t count, loff_t *ppos)
{
	return abox_dump_auto_read(file, data, count, ppos, false);
}

static ssize_t abox_dump_auto_stop_write(struct file *file,
		const char __user *data, size_t count, loff_t *ppos)
{
	return abox_dump_auto_write(file, data, count, ppos, false);
}

static const struct file_operations abox_dump_auto_start_fops = {
	.read = abox_dump_auto_start_read,
	.write = abox_dump_auto_start_write,
};

static const struct file_operations abox_dump_auto_stop_fops = {
	.read = abox_dump_auto_stop_read,
	.write = abox_dump_auto_stop_write,
};

static int __init samsung_abox_dump_late_initcall(void)
{
	pr_info("%s\n", __func__);

	debugfs_create_file("dump_auto_start", 0660, abox_dbg_get_root_dir(),
			NULL, &abox_dump_auto_start_fops);
	debugfs_create_file("dump_auto_stop", 0660, abox_dbg_get_root_dir(),
			NULL, &abox_dump_auto_stop_fops);

	return 0;
}
late_initcall(samsung_abox_dump_late_initcall);

static struct snd_soc_dai_link abox_dump_dai_links[BUFFER_MAX];

static struct snd_soc_card abox_dump_card = {
	.name = "abox_dump",
	.owner = THIS_MODULE,
	.dai_link = abox_dump_dai_links,
	.num_links = 0,
};

static void abox_dump_auto_dump_work_func(struct work_struct *work)
{
	struct abox_dump_buffer_info *info = container_of(work,
			struct abox_dump_buffer_info, auto_work);
	struct device *dev = info->dev;
	const char *name = info->name;

	if (info->auto_started) {
		mm_segment_t old_fs;
		char filename[SZ_64];
		struct file *filp;

		sprintf(filename, "/data/abox_dump-%s.raw", name);

		old_fs = get_fs();
		set_fs(KERNEL_DS);
		if (likely(info->file_created)) {
			filp = filp_open(filename, O_RDWR | O_APPEND | O_CREAT,
					0660);
			dev_dbg(dev, "appended\n");
		} else {
			filp = filp_open(filename, O_RDWR | O_TRUNC | O_CREAT,
					0660);
			info->file_created = true;
			dev_dbg(dev, "created\n");
		}
		if (!IS_ERR_OR_NULL(filp)) {
			void *area = info->buffer.area;
			size_t bytes = info->buffer.bytes;
			size_t pointer = info->pointer;

			dev_dbg(dev, "%pad, %pK, %zx, %zx)\n",
					&info->buffer.addr, area, bytes,
					info->auto_pointer);
			if (pointer < info->auto_pointer) {
				vfs_write(filp, area + info->auto_pointer,
						bytes - info->auto_pointer,
						&filp->f_pos);
				dev_dbg(dev, "vfs_write(%pK, %zx)\n",
						area + info->auto_pointer,
						bytes - info->auto_pointer);
				info->auto_pointer = 0;
			}
			vfs_write(filp, area + info->auto_pointer,
					pointer - info->auto_pointer,
					&filp->f_pos);
			dev_dbg(dev, "vfs_write(%pK, %zx)\n",
					area + info->auto_pointer,
					pointer - info->auto_pointer);
			info->auto_pointer = pointer;

			vfs_fsync(filp, 1);
			filp_close(filp, NULL);
		} else {
			dev_err(dev, "dump file %s open error: %ld\n", name,
					PTR_ERR(filp));
		}

		set_fs(old_fs);
	}
}

static void abox_dump_check_buffer(struct snd_dma_buffer *buffer)
{
	if (!buffer->bytes)
		buffer->bytes = SZ_64K;
	if (!buffer->area) {
		/* area will be used in kernel only.
		 * kmalloc and virt_to_phys are just enough.
		 */
		buffer->area = devm_kmalloc(abox_dump_dev_abox, buffer->bytes,
				GFP_KERNEL);
		buffer->addr = virt_to_phys(buffer->area);
	}
}

void abox_dump_register_buffer_work_func(struct work_struct *work)
{
	int id;
	struct abox_dump_buffer_info *info;

	dev_dbg(abox_dump_card.dev, "%s\n", __func__);

	for (info = &abox_dump_list[0]; (info - &abox_dump_list[0]) <
			ARRAY_SIZE(abox_dump_list); info++) {
		id = info->id;
		if (info->dev && !abox_dump_get_buffer_info(id)) {
			abox_dump_check_buffer(&info->buffer);
			dev_info(info->dev, "%s(%d, %s, %#zx)\n", __func__,
					id, info->name, info->buffer.bytes);
			list_add_tail(&info->list, &abox_dump_list_head);
			platform_device_register_data(info->dev,
					"samsung-abox-dump", id, NULL, 0);
		}
	}
}

static DECLARE_WORK(abox_dump_register_buffer_work,
		abox_dump_register_buffer_work_func);

int abox_dump_register_buffer(struct device *dev, int id, const char *name,
		void *area, phys_addr_t addr, size_t bytes)
{
	struct abox_dump_buffer_info *info;

	dev_dbg(dev, "%s[%d](%s, %#zx)\n", __func__, id, name, bytes);

	if (id < 0 || id >= ARRAY_SIZE(abox_dump_list)) {
		dev_err(dev, "invalid id: %d\n", id);
		return -EINVAL;
	}

	info = &abox_dump_list[id];
	if (!strcmp(info->name, name)) {
		dev_dbg(dev, "already registered dump: %d\n", id);
		return 0;
	}

	mutex_init(&info->lock);
	info->id = id;
	strncpy(info->name, name, sizeof(info->name) - 1);
	info->buffer.area = area;
	info->buffer.addr = addr;
	info->buffer.bytes = bytes;
	INIT_WORK(&info->auto_work, abox_dump_auto_dump_work_func);
	abox_dump_dev_abox = info->dev = dev;
	schedule_work(&abox_dump_register_buffer_work);

	return 0;
}

static struct snd_pcm_hardware abox_dump_hardware = {
	.info		= SNDRV_PCM_INFO_INTERLEAVED
			| SNDRV_PCM_INFO_BLOCK_TRANSFER
			| SNDRV_PCM_INFO_MMAP
			| SNDRV_PCM_INFO_MMAP_VALID,
	.formats	= ABOX_SAMPLE_FORMATS,
	.rates		= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min	= 8000,
	.rate_max	= 384000,
	.channels_min	= 1,
	.channels_max	= 8,
	.periods_min	= 2,
	.periods_max	= 32,
};

static int abox_dump_platform_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);
	struct snd_dma_buffer *dmab = &substream->dma_buffer;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	abox_dump_hardware.buffer_bytes_max = dmab->bytes;
	abox_dump_hardware.period_bytes_min = dmab->bytes /
			abox_dump_hardware.periods_max;
	abox_dump_hardware.period_bytes_max = dmab->bytes /
			abox_dump_hardware.periods_min;

	snd_soc_set_runtime_hwparams(substream, &abox_dump_hardware);

	info->substream = substream;

	return 0;
}

static int abox_dump_platform_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	info->substream = NULL;

	return 0;
}

static int abox_dump_platform_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
}

static int abox_dump_platform_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return snd_pcm_lib_free_pages(substream);
}

static int abox_dump_platform_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	info->pointer = 0;

	return 0;
}

static int abox_dump_platform_trigger(struct snd_pcm_substream *substream,
		int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);

	dev_dbg(dev, "%s[%d](%d)\n", __func__, id, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		info->started = true;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		info->started = false;
		break;
	default:
		dev_err(dev, "invalid command: %d\n", cmd);
		return -EINVAL;
	}

	abox_dump_request_dump(id);

	return 0;
}

void abox_dump_period_elapsed(int id, size_t pointer)
{
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);
	struct device *dev = info->dev;

	dev_dbg(dev, "%s[%d](%zx)\n", __func__, id, pointer);

	info->pointer = pointer;
	schedule_work(&info->auto_work);
	snd_pcm_period_elapsed(info->substream);
}

void abox_dump_transfer(int id, const char *buf, size_t bytes)
{
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);
	struct device *dev = info->dev;
	size_t size, pointer;

	dev_dbg(dev, "%s[%d](%pK, %zx): %zx\n", __func__, id, buf, bytes,
			info->pointer);

	size = min(bytes, info->buffer.bytes - info->pointer);
	memcpy(info->buffer.area + info->pointer, buf, size);
	if (bytes - size > 0)
		memcpy(info->buffer.area, buf + size, bytes - size);

	pointer = (info->pointer + bytes) % info->buffer.bytes;
	abox_dump_period_elapsed(id, pointer);
}

static snd_pcm_uframes_t abox_dump_platform_pointer(
		struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return bytes_to_frames(substream->runtime, info->pointer);
}

static struct snd_pcm_ops abox_dump_platform_ops = {
	.open		= abox_dump_platform_open,
	.close		= abox_dump_platform_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= abox_dump_platform_hw_params,
	.hw_free	= abox_dump_platform_hw_free,
	.prepare	= abox_dump_platform_prepare,
	.trigger	= abox_dump_platform_trigger,
	.pointer	= abox_dump_platform_pointer,
};

static void abox_dump_register_card_work_func(struct work_struct *work)
{
	int i;

	pr_debug("%s\n", __func__);

	for (i = 0; i < abox_dump_card.num_links; i++) {
		struct snd_soc_dai_link *link = &abox_dump_card.dai_link[i];

		if (link->name)
			continue;

		link->name = link->stream_name =
				kasprintf(GFP_KERNEL, "dummy%d", i);
		link->cpu_name = "snd-soc-dummy";
		link->cpu_dai_name = "snd-soc-dummy-dai";
		link->codec_name = "snd-soc-dummy";
		link->codec_dai_name = "snd-soc-dummy-dai";
		link->no_pcm = 1;
	}
	abox_register_extra_sound_card(abox_dump_card.dev, &abox_dump_card, 2);
}

static DECLARE_DELAYED_WORK(abox_dump_register_card_work,
		abox_dump_register_card_work_func);

static int abox_dump_add_dai_link(struct device *dev)
{
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);
	struct snd_soc_dai_link *link = &abox_dump_dai_links[id];

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (id > ARRAY_SIZE(abox_dump_dai_links)) {
		dev_err(dev, "Too many dump request\n");
		return -ENOMEM;
	}

	cancel_delayed_work_sync(&abox_dump_register_card_work);
	info->dev = dev;
	kfree(link->name);
	link->name = link->stream_name = kstrdup(info->name, GFP_KERNEL);
	link->cpu_name = "snd-soc-dummy";
	link->cpu_dai_name = "snd-soc-dummy-dai";
	link->platform_name = dev_name(dev);
	link->codec_name = "snd-soc-dummy";
	link->codec_dai_name = "snd-soc-dummy-dai";
	link->ignore_suspend = 1;
	link->ignore_pmdown_time = 1;
	link->no_pcm = 0;
	link->capture_only = true;

	if (abox_dump_card.num_links <= id)
		abox_dump_card.num_links = id + 1;

	schedule_delayed_work(&abox_dump_register_card_work, HZ);

	return 0;
}

static int abox_dump_platform_probe(struct snd_soc_platform *platform)
{
	struct device *dev = platform->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return 0;
}

static int abox_dump_platform_new(struct snd_soc_pcm_runtime *runtime)
{
	struct device *dev = runtime->platform->dev;
	struct snd_pcm *pcm = runtime->pcm;
	struct snd_pcm_str *stream = &pcm->streams[SNDRV_PCM_STREAM_CAPTURE];
	struct snd_pcm_substream *substream = stream->substream;
	struct snd_dma_buffer *dmab = &substream->dma_buffer;
	int id = to_platform_device(dev)->id;
	struct abox_dump_buffer_info *info = abox_dump_get_buffer_info(id);

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	dmab->dev.type = SNDRV_DMA_TYPE_DEV;
	dmab->dev.dev = dev;
	dmab->area = info->buffer.area;
	dmab->addr = info->buffer.addr;
	dmab->bytes = info->buffer.bytes;

	return 0;
}

static void abox_dump_platform_free(struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *runtime = pcm->private_data;
	struct device *dev = runtime->platform->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);
}

struct snd_soc_platform_driver abox_dump_platform = {
	.probe		= abox_dump_platform_probe,
	.ops		= &abox_dump_platform_ops,
	.pcm_new	= abox_dump_platform_new,
	.pcm_free	= abox_dump_platform_free,
};

static int samsung_abox_dump_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int id = to_platform_device(dev)->id;
	int ret = 0;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (id < 0) {
		abox_dump_card.dev = &pdev->dev;
		schedule_delayed_work(&abox_dump_register_card_work, 0);
	} else {
		pm_runtime_no_callbacks(dev);
		pm_runtime_enable(dev);
		devm_snd_soc_register_platform(dev, &abox_dump_platform);
		ret = abox_dump_add_dai_link(dev);
	}

	return ret;
}

static int samsung_abox_dump_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int id = to_platform_device(dev)->id;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	return 0;
}

static const struct platform_device_id samsung_abox_dump_driver_ids[] = {
	{
		.name = "samsung-abox-dump",
	},
	{},
};
MODULE_DEVICE_TABLE(platform, samsung_abox_dump_driver_ids);

static struct platform_driver samsung_abox_dump_driver = {
	.probe  = samsung_abox_dump_probe,
	.remove = samsung_abox_dump_remove,
	.driver = {
		.name = "samsung-abox-dump",
		.owner = THIS_MODULE,
	},
	.id_table = samsung_abox_dump_driver_ids,
};

module_platform_driver(samsung_abox_dump_driver);

void abox_dump_init(struct device *dev_abox)
{
	dev_info(dev_abox, "%s\n", __func__);

	abox_dump_dev_abox = dev_abox;
	debugfs_create_file("dump_auto_start", 0660, abox_dbg_get_root_dir(),
			dev_abox, &abox_dump_auto_start_fops);
	debugfs_create_file("dump_auto_stop", 0660, abox_dbg_get_root_dir(),
			dev_abox, &abox_dump_auto_stop_fops);
	platform_device_register_data(dev_abox,
			"samsung-abox-dump", -1, NULL, 0);
}

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box Internal Buffer Dumping Driver");
MODULE_ALIAS("platform:samsung-abox-dump");
MODULE_LICENSE("GPL");
