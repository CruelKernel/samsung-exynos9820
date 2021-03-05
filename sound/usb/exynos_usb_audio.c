/*
 *   USB Audio offloading Driver for Exynos
 *
 *   Copyright (c) 2017 by Kyounghye Yun <k-hye.yun@samsung.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */


#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/usb.h>
#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>

#include <sound/samsung/abox.h>
#include <linux/usb/exynos_usb_audio.h>

#define DEBUG 1

void exynos_usb_audio_set_device(struct usb_device *udev)
{
	usb_audio->udev = udev;
}

int exynos_usb_audio_map_buf(struct usb_device *udev)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	struct hcd_hw_info *hwinfo = &udev->hwinfo;
	struct usb_device *usb_dev = udev;
	u32 id;
	int ret;

	if (DEBUG)
		pr_info("USB_AUDIO_IPC : %s ", __func__);

	id = (((usb_dev->descriptor.idVendor) << 16) |
			(usb_dev->descriptor.idProduct));

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_PCM_BUF;

	erap_usb->param1 = id;

	/* iommu map for in data buffer */
	usb_audio->in_buf_addr = hwinfo->in_dma;

	if (DEBUG) {
		pr_info("pcm in data buffer pa addr : %#08x %08x \n",
				upper_32_bits(le64_to_cpu(hwinfo->in_dma)),
				lower_32_bits(le64_to_cpu(hwinfo->in_dma)));
	}

	ret = abox_iommu_map(dev, USB_AUDIO_PCM_INBUF,
					hwinfo->in_dma, PAGE_SIZE * 256, 0);
	if (ret) {
		pr_err("abox iommu mapping for pcm in buf is failed \n");
		return ret;
	}

	/* iommu map for out data buffer */
	usb_audio->out_buf_addr = hwinfo->out_dma;

	if (DEBUG) {
		pr_info("pcm out data buffer pa addr : %#08x %08x \n",
				upper_32_bits(le64_to_cpu(hwinfo->out_dma)),
				lower_32_bits(le64_to_cpu(hwinfo->out_dma)));
	}

	ret = abox_iommu_map(dev, USB_AUDIO_PCM_OUTBUF,
					hwinfo->out_dma, PAGE_SIZE * 256, 0);
	if (ret) {
		pr_err("abox iommu mapping for pcm out buf is failed \n");
		return ret;
	}

	return 0;
}

int exynos_usb_audio_pcmbuf(struct usb_device *udev)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	struct hcd_hw_info *hwinfo = &udev->hwinfo;
	u64 out_dma;
	int ret;

	if (DEBUG)
		pr_info("USB_AUDIO_IPC : %s\n", __func__);

	if (!usb_audio->is_audio || !otg_connection) {
		dev_info(dev, "USB_AUDIO_IPC : is_audio is 0. return!\n");
		return -1;
	}

	out_dma = abox_iova_to_phys(dev, USB_AUDIO_PCM_OUTBUF);

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_PCM_BUF;

	erap_usb->param1 = lower_32_bits(le64_to_cpu(out_dma));
	erap_usb->param2 = upper_32_bits(le64_to_cpu(out_dma));
	erap_usb->param3 = lower_32_bits(le64_to_cpu(hwinfo->in_dma));
	erap_usb->param4 = upper_32_bits(le64_to_cpu(hwinfo->in_dma));

	if (DEBUG) {
		pr_info("pcm out data buffer pa addr : %#08x %08x\n",
				upper_32_bits(le64_to_cpu(out_dma)),
				lower_32_bits(le64_to_cpu(out_dma)));
		pr_info("pcm in data buffer pa addr : %#08x %08x\n",
				upper_32_bits(le64_to_cpu(hwinfo->in_dma)),
				lower_32_bits(le64_to_cpu(hwinfo->in_dma)));
		pr_info("erap param2 : %#08x param1 : %08x\n",
				erap_usb->param2, erap_usb->param1);
		pr_info("erap param4 : %#08x param3 : %08x\n",
				erap_usb->param4, erap_usb->param3);
	}

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "erap usb transfer pcm buffer is failed\n");
		return -1;
	}

	return 0;
}

int exynos_usb_audio_setrate(int iface, int rate, int alt)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	int ret;

	if (DEBUG)
		pr_info("USB_AUDIO_IPC : %s\n", __func__);

	if (!usb_audio->is_audio || !otg_connection) {
		dev_info(dev, "USB_AUDIO_IPC : is_audio is 0. return!\n");
		return -1;
	}

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_SAMPLE_RATE;

	erap_usb->param1 = iface;
	erap_usb->param2 = rate;
	erap_usb->param3 = alt;

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "erap usb transfer sample rate is failed\n");
		return -1;
	}

	return 0;
}

int exynos_usb_audio_setintf(struct usb_device *udev, int iface, int alt, int direction)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	struct hcd_hw_info *hwinfo = &udev->hwinfo;
	u64 in_offset, out_offset;
	int ret;
	unsigned long left_time;

	if (!usb_audio->pcm_open_done) {
		dev_info(dev, "USB_AUDIO_IPC : pcm node was not opeded!\n");
		return -EPERM;
	}

	if (DEBUG)
		dev_info(&udev->dev, "USB_AUDIO_IPC : %s, alt = %d\n",
				__func__, alt);

	if (!usb_audio->is_audio || !otg_connection) {
		dev_info(dev, "USB_AUDIO_IPC : is_audio is 0. return!\n");
		return -1;
	}

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_SET_INTF;

	erap_usb->param1 = alt;
	erap_usb->param2 = iface;

	if (alt == 0 && direction == SNDRV_PCM_STREAM_PLAYBACK) {
		if (usb_audio->pcm_open_done &&
				(!usb_audio->outdeq_map_done ||
				(hwinfo->out_deq != hwinfo->old_out_deq))) {
			dev_info(dev, "out_deq wait required\n");
			left_time = wait_for_completion_timeout(&usb_audio->out_conn_stop,
								msecs_to_jiffies(1000));
			if (!left_time)
				dev_info(dev, "%s: timeout for OUT connection done\n");
		}
	} else if (alt == 0 && direction == SNDRV_PCM_STREAM_CAPTURE) {
		if (usb_audio->pcm_open_done &&
				(!usb_audio->indeq_map_done ||
				(hwinfo->in_deq != hwinfo->old_in_deq))) {
			dev_info(dev, "in_deq wait required\n");
			left_time = wait_for_completion_timeout(&usb_audio->in_conn_stop,
								msecs_to_jiffies(1000));
			if (!left_time)
				dev_info(dev, "%s: timeout for IN connection done\n");
		}
	}

	if (!usb_audio->is_audio || !otg_connection) {
		dev_info(dev, "USB_AUDIO_IPC : is_audio = 0. return!\n");
		return -1;
	}

	mutex_lock(&usb_audio->lock);
	if (direction) {
		/* IN EP */
		dev_dbg(dev, "in deq : %#08llx, %#08llx, %#08llx, %#08llx\n",
				hwinfo->in_deq, hwinfo->old_in_deq,
				hwinfo->fb_out_deq, hwinfo->fb_old_out_deq);
		if (!usb_audio->indeq_map_done ||
			(hwinfo->in_deq != hwinfo->old_in_deq)) {
			dev_info(dev, "in_deq map required\n");
			if (usb_audio->indeq_map_done) {
				ret = abox_iommu_unmap(dev, USB_AUDIO_IN_DEQ);
				if (ret < 0) {
					pr_err("un-map of in buf failed %d\n", ret);
					goto err;
				}
			}
			usb_audio->indeq_map_done = 1;

			in_offset = hwinfo->in_deq % PAGE_SIZE;
			ret = abox_iommu_map(dev, USB_AUDIO_IN_DEQ,
					(hwinfo->in_deq - in_offset),
					PAGE_SIZE, 0);
			if (ret < 0) {
				pr_err("map for in buf failed %d\n", ret);
				goto err;
			}
		}

		if (hwinfo->fb_out_deq) {
			dev_dbg(dev, "fb_in deq : %#08llx\n", hwinfo->fb_out_deq);
			if (!usb_audio->fb_outdeq_map_done ||
				(hwinfo->fb_out_deq != hwinfo->fb_old_out_deq)) {
				if (usb_audio->fb_outdeq_map_done) {
					ret = abox_iommu_unmap(dev, USB_AUDIO_FBOUT_DEQ);
					if (ret < 0) {
						pr_err("un-map for fb_out buf failed %d\n", ret);
						goto err;
					}
				}
				usb_audio->fb_outdeq_map_done = 1;

				out_offset = hwinfo->fb_out_deq % PAGE_SIZE;
				ret = abox_iommu_map(dev, USB_AUDIO_FBOUT_DEQ,
						(hwinfo->fb_out_deq - out_offset),
						PAGE_SIZE, 0);
				if (ret < 0) {
					pr_err("map for fb_out buf failed %d\n", ret);
					goto err;
				}
			}
		}

		erap_usb->param3 = lower_32_bits(le64_to_cpu(hwinfo->in_deq));
		erap_usb->param4 = upper_32_bits(le64_to_cpu(hwinfo->in_deq));
	} else {
		/* OUT EP */
		dev_dbg(dev, "out deq : %#08llx, %#08llx, %#08llx, %#08llx\n",
				hwinfo->out_deq, hwinfo->old_out_deq,
				hwinfo->fb_in_deq, hwinfo->fb_old_in_deq);
		if (!usb_audio->outdeq_map_done ||
			(hwinfo->out_deq != hwinfo->old_out_deq)) {
			dev_info(dev, "out_deq map required\n");
			if (usb_audio->outdeq_map_done) {
				ret = abox_iommu_unmap(dev, USB_AUDIO_OUT_DEQ);
				if (ret < 0) {
					pr_err("un-map for out buf failed %d\n", ret);
					goto err;
				}
			}
			usb_audio->outdeq_map_done = 1;

			out_offset = hwinfo->out_deq % PAGE_SIZE;
			ret = abox_iommu_map(dev, USB_AUDIO_OUT_DEQ,
					(hwinfo->out_deq - out_offset),
					PAGE_SIZE, 0);
			if (ret < 0) {
				pr_err("map for out buf failed %d\n", ret);
				goto err;
			}
		}

		if (hwinfo->fb_in_deq) {
			dev_info(dev, "fb_in deq : %#08llx\n", hwinfo->fb_in_deq);
			if (!usb_audio->fb_indeq_map_done ||
				(hwinfo->fb_in_deq != hwinfo->fb_old_in_deq)) {
				if (usb_audio->fb_indeq_map_done) {
					ret = abox_iommu_unmap(dev, USB_AUDIO_FBIN_DEQ);
					if (ret < 0) {
						pr_err("un-map for fb_in buf failed %d\n", ret);
						goto err;
					}
				}
				usb_audio->fb_indeq_map_done = 1;

				in_offset = hwinfo->fb_in_deq % PAGE_SIZE;
				ret = abox_iommu_map(dev, USB_AUDIO_FBIN_DEQ,
						(hwinfo->fb_in_deq - in_offset),
						PAGE_SIZE, 0);
				if (ret < 0) {
					pr_err("map for fb_in buf failed %d\n", ret);
					goto err;
				}
			}
		}

		erap_usb->param3 = lower_32_bits(le64_to_cpu(hwinfo->out_deq));
		erap_usb->param4 = upper_32_bits(le64_to_cpu(hwinfo->out_deq));
	}

	/* one more check connection to prevent kernel panic */
	if (!usb_audio->is_audio || !otg_connection) {
		dev_info(dev, "USB_AUDIO_IPC : is_audio is 0. return!\n");
		return -1;
	}

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "erap usb hcd control failed\n");
		goto err;
	}
	mutex_unlock(&usb_audio->lock);

	if (DEBUG) {
		dev_info(&udev->dev, "Alt#%d / Intf#%d / Direction %s / EP DEQ : %#08x %08x\n",
				erap_usb->param1, erap_usb->param2,
				direction ? "IN" : "OUT",
				erap_usb->param4, erap_usb->param3);
	}

	return 0;

err:
	dev_err(&udev->dev, "%s error = %d\n", __func__, ret);
	mutex_unlock(&usb_audio->lock);
	return ret;
}

int exynos_usb_audio_hcd(struct usb_device *udev)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	struct hcd_hw_info *hwinfo = &udev->hwinfo;
	int ret;

	if (DEBUG) {
		dev_info(&udev->dev, "USB_AUDIO_IPC : %s\n", __func__);
		dev_info(&udev->dev, "=======[Check HW INFO] ========\n");
		dev_info(&udev->dev, "slot_id : %d\n", hwinfo->slot_id);
		dev_info(&udev->dev, "dcbaa : %#08llx\n", hwinfo->dcbaa_dma);
		dev_info(&udev->dev, "save : %#08llx\n", hwinfo->save_dma);
		dev_info(&udev->dev, "in_ctx : %#08llx\n", hwinfo->in_ctx);
		dev_info(&udev->dev, "out_ctx : %#08llx\n", hwinfo->out_ctx);
		dev_info(&udev->dev, "erst : %#08x %08x\n",
					upper_32_bits(le64_to_cpu(hwinfo->erst_addr)),
					lower_32_bits(le64_to_cpu(hwinfo->erst_addr)));
		dev_info(&udev->dev, "===============================\n");
	}

	/* back up each address for unmap */
	usb_audio->dcbaa_dma = hwinfo->dcbaa_dma;
	usb_audio->save_dma = hwinfo->save_dma;
	usb_audio->in_ctx = hwinfo->in_ctx;
	usb_audio->out_ctx = hwinfo->out_ctx;
	usb_audio->erst_addr = hwinfo->erst_addr;
	usb_audio->speed = hwinfo->speed;

	if (DEBUG)
		dev_info(dev, "USB_AUDIO_IPC : SFR MAPPING!\n");

	mutex_lock(&usb_audio->lock);

	ret = abox_iommu_map(dev, USB_AUDIO_XHCI_BASE, USB_AUDIO_XHCI_BASE, PAGE_SIZE * 16, 0);
	/*
	 * Check whether usb buffer was unmapped already.
	 * If not, unmap all buffers and try map again.
	 */
	if (ret == -EADDRINUSE) {
		cancel_work_sync(&usb_audio->usb_work);
		pr_err("iommu unmapping not done. unmap here\n", ret);
		exynos_usb_audio_unmap_all();
		ret = abox_iommu_map(dev, USB_AUDIO_XHCI_BASE,
				USB_AUDIO_XHCI_BASE, PAGE_SIZE * 16, 0);
	}

	if (ret) {
		pr_err("iommu mapping for in buf failed %d\n", ret);
		goto err;
	}

	/*DCBAA mapping*/
	ret = abox_iommu_map(dev, USB_AUDIO_SAVE_RESTORE, hwinfo->save_dma, PAGE_SIZE, 0);
	if (ret) {
		pr_err(" abox iommu mapping for save_restore buffer is failed\n");
		goto err;
	}

	/*Device Context mapping*/
	ret = abox_iommu_map(dev, USB_AUDIO_DEV_CTX, hwinfo->out_ctx, PAGE_SIZE, 0);
	if (ret) {
		pr_err(" abox iommu mapping for device ctx is failed\n");
		goto err;
	}

	/*Input Context mapping*/
	ret = abox_iommu_map(dev, USB_AUDIO_INPUT_CTX, hwinfo->in_ctx, PAGE_SIZE, 0);
	if (ret) {
		pr_err(" abox iommu mapping for input ctx is failed\n");
		goto err;
	}

	/*ERST mapping*/
	ret = abox_iommu_map(dev, USB_AUDIO_ERST, hwinfo->erst_addr, PAGE_SIZE, 0);
	if (ret) {
		pr_err(" abox iommu mapping for erst is failed\n");
		goto err;
	}

	/* notify to Abox descriptor is ready*/
	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_XHCI;

	erap_usb->param1 = usb_audio->is_first_probe;
	erap_usb->param2 = hwinfo->slot_id;
	erap_usb->param3 = lower_32_bits(le64_to_cpu(hwinfo->erst_addr));
	erap_usb->param4 = upper_32_bits(le64_to_cpu(hwinfo->erst_addr));

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "erap usb hcd control failed\n");
		goto err;
	}
	usb_audio->is_first_probe = 0;
	mutex_unlock(&usb_audio->lock);

	return 0;

err:
	dev_err(&udev->dev, "%s error = %d\n", __func__, ret);
	usb_audio->is_first_probe = 0;
	mutex_unlock(&usb_audio->lock);
	return ret;
}

int exynos_usb_audio_desc(struct usb_device *udev)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	int ret, configuration, cfgno, i;
	unsigned char *buffer;
	unsigned int len = udev->rawdesc_length;
	u64 desc_addr;
	u64 offset;

	if (DEBUG)
		dev_info(&udev->dev, "USB_AUDIO_IPC : %s\n", __func__);

	configuration = usb_choose_configuration(udev);

	cfgno = -1;
	for (i = 0; i < udev->descriptor.bNumConfigurations; i++) {
		if (udev->config[i].desc.bConfigurationValue ==
				configuration) {
			cfgno = i;
			pr_info("%s - chosen = %d, c = %d\n", __func__,
				i, configuration);
			break;
		}
	}

	if (cfgno == -1) {
		pr_info("%s - config select error, i=%d, c=%d\n",
			__func__, i, configuration);
		cfgno = 0;
	}

	/* need to memory mapping for usb descriptor */
	buffer = udev->rawdescriptors[cfgno];
	desc_addr = virt_to_phys(buffer);
	offset = desc_addr % PAGE_SIZE;

	/* store address information */
	usb_audio->desc_addr = desc_addr;
	usb_audio->offset = offset;

	desc_addr -= offset;

	mutex_lock(&usb_audio->lock);
	ret = abox_iommu_map(dev, USB_AUDIO_DESC, desc_addr, (PAGE_SIZE * 2), 0);
	if (ret) {
		dev_err(&udev->dev, "USB AUDIO: abox iommu mapping for usb descriptor is failed\n");
		goto err;
	}

	/* notify to Abox descriptor is ready*/
	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_DESC;

	erap_usb->param1 = 1;
	erap_usb->param2 = len;
	erap_usb->param3 = offset;
	erap_usb->param4 = usb_audio->speed;

	if (DEBUG)
		dev_info(&udev->dev, "paddr : %#08llx / offset : %#08llx / len : %d / speed : %d\n",
							desc_addr + offset , offset, len, usb_audio->speed);

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "erap usb desc  control failed\n");
		goto err;
	}
	mutex_unlock(&usb_audio->lock);

	dev_info(&udev->dev, "USB AUDIO: Mapping descriptor for using on Abox USB F/W & Nofity mapping is done!");

	return 0;

err:
	dev_err(&udev->dev, "%s error = %d\n", __func__, ret);
	mutex_unlock(&usb_audio->lock);
	return ret;
}

int exynos_usb_audio_conn(int is_conn)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	int ret;

	if (DEBUG)
		pr_info("USB_AUDIO_IPC : %s\n", __func__);

	pr_info("USB DEVICE IS %s\n", is_conn? "CONNECTION" : "DISCONNECTION");

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_CONN;

	erap_usb->param1 = is_conn;

	if (!is_conn) {
		if (usb_audio->is_audio) {
			usb_audio->is_audio = 0;
			usb_audio->usb_audio_state = USB_AUDIO_REMOVING;
			ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
			if (ret) {
				pr_err("erap usb dis_conn control failed\n");
				return -1;
			}
		} else {
			pr_err("Is not USB Audio device\n");
		}
	} else {
		cancel_work_sync(&usb_audio->usb_work);
		usb_audio->indeq_map_done = 0;
		usb_audio->outdeq_map_done = 0;
		usb_audio->fb_indeq_map_done = 0;
		usb_audio->fb_outdeq_map_done = 0;
		usb_audio->pcm_open_done = 0;
		reinit_completion(&usb_audio->discon_done);
		ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
		if (ret) {
			pr_err("erap usb conn control failed\n");
			return -1;
		}
		usb_audio->usb_audio_state = USB_AUDIO_CONNECT;
	}

	return 0;
}

int exynos_usb_audio_pcm(int is_open, int direction)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	int ret;

	if (DEBUG)
		dev_info(dev, "USB_AUDIO_IPC : %s\n", __func__);

	if (!usb_audio->is_audio || !otg_connection) {
		dev_info(dev, "USB_AUDIO_IPC : is_audio is 0. return!\n");
		return -1;
	}

	if (is_open)
		usb_audio->pcm_open_done = 1;
	dev_info(dev, "PCM %s dir %s\n", is_open? "OPEN" : "CLOSE",
				direction ? "IN" : "OUT");

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_PCM_OPEN;

	erap_usb->param1 = is_open;
	erap_usb->param2 = direction;

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "ERAP USB PCM control failed\n");
		return -1;
	}

	return 0;
}

int exynos_usb_audio_l2(int is_l2)
{
	ABOX_IPC_MSG msg;
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_USB_AUDIO_PARAM *erap_usb = &erap_msg->param.usbaudio;
	int ret;

	if (!usb_audio->is_audio || !otg_connection)
		return 0;

	if (DEBUG)
		dev_info(dev, "USB_AUDIO_IPC : %s\n", __func__);
	dev_info(dev, " USB Audio  %s the L2 power mode\n", is_l2 ? "ENTER" : "EXIT");

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_USB;
	erap_usb->type = IPC_USB_L2;

	/* 1: device entered L2 , 0: device exited L2 */
	erap_usb->param1 = is_l2;

	ret = abox_start_ipc_transaction(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (ret) {
		dev_err(&usb_audio->udev->dev, "ERAP USB L2 control failed\n");
		return -1;
	}

	return 0;
}

void exynos_usb_audio_work(struct work_struct *w)
{
	pr_info("%s\n", __func__);

	/* Don't unmap in USB_AUDIO_TIMEOUT_PROBE state */
	if (usb_audio->usb_audio_state !=
			USB_AUDIO_REMOVING) {
		pr_info("%s, not removing state\n", __func__);
		return;
	}

	exynos_usb_audio_unmap_all();
	usb_audio->usb_audio_state = USB_AUDIO_DISCONNECT;
	complete(&usb_audio->discon_done);
}

irqreturn_t exynos_usb_audio_irq_handler(int irq, void *dev_id, ABOX_IPC_MSG *msg)
{
	struct IPC_ERAP_MSG *erap_msg = &msg->msg.erap;

	if (DEBUG)
		pr_info("%s: IRQ = %d, type=%d, par1 = %d\n", __func__,
				irq, erap_msg->param.usbaudio.type,
				erap_msg->param.usbaudio.param1);

	if ( irq == IPC_ERAP ) {
		if (erap_msg->msgtype == REALTIME_USB) {
			switch (erap_msg->param.usbaudio.type) {
			case IPC_USB_TASK:
				if (erap_msg->param.usbaudio.param1) {
					pr_info("irq : %d /* param1 : 1 , IN EP task done */\n", irq);
				} else {
					pr_info("irq : %d /* param0 : 0 , OUT EP task done */\n", irq);
					/* Don't schedule in TIMEOUT_PROBE */
					if (usb_audio->usb_audio_state ==
							USB_AUDIO_REMOVING)
						schedule_work(&usb_audio
							->usb_work);
				}
				break;
			case IPC_USB_STOP_DONE:
				if (erap_msg->param.usbaudio.param1 ==
						SNDRV_PCM_STREAM_PLAYBACK)
					complete(&usb_audio->out_conn_stop);
				else if (erap_msg->param.usbaudio.param1 ==
						SNDRV_PCM_STREAM_CAPTURE)
					complete(&usb_audio->in_conn_stop);
				else
					pr_err("Unexpected USB_STOP_DONE param!!\n");

				break;
			default:
				pr_err("%s: unknown usb msg type\n", __func__);
				break;
			}
		} else {
			pr_err("%s: unknown message type\n", __func__);
		}
	} else {
		pr_err("%s: unknown command\n", __func__);
	}
	/* FIXME */
	return IRQ_HANDLED;
}

int exynos_usb_audio_init(struct device *dev, struct platform_device *pdev)
{
	struct device_node *np = dev->of_node;
	struct device_node *np_abox;
	struct platform_device *pdev_abox;

	if (DEBUG)
		dev_info(dev, "USB_AUDIO_IPC : %s\n", __func__);

	np_abox =of_parse_phandle(np, "abox", 0);
	if(!np_abox) {
		dev_err(dev, "Failed to get abox device node\n");
		return -EPROBE_DEFER;
	}

	pdev_abox = of_find_device_by_node(np_abox);
	if (!pdev_abox) {
		dev_err(&usb_audio->udev->dev, "Failed to get abox platform device\n");
		return -EPROBE_DEFER;
	}

	mutex_init(&usb_audio->lock);
	init_completion(&usb_audio->in_conn_stop);
	init_completion(&usb_audio->out_conn_stop);
	init_completion(&usb_audio->discon_done);
	usb_audio->abox = pdev_abox;
	usb_audio->hcd_pdev = pdev;
	usb_audio->udev = NULL;
	usb_audio->is_audio = 0;
	usb_audio->is_first_probe = 1;
	usb_audio->usb_audio_state = USB_AUDIO_DISCONNECT;

	abox_register_ipc_handler(&pdev_abox->dev, IPC_ERAP,
				exynos_usb_audio_irq_handler, &usb_audio);

	return 0;
}

int exynos_usb_audio_unmap_all(void)
{
	struct platform_device *pdev = usb_audio->abox;
	struct device *dev = &pdev->dev;
	u64 addr;
	u64 offset;
	int ret;

	if (DEBUG)
		dev_info(dev, "USB_AUDIO_IPC : %s\n", __func__);

	/* unmapping in pcm buffer */
	addr = usb_audio->in_buf_addr;
	if (DEBUG)
		pr_info("PCM IN BUFFER FREE: paddr = %#08llx\n", addr);

	ret = abox_iommu_unmap(dev, USB_AUDIO_PCM_INBUF);
	if (ret < 0) {
		pr_err("iommu un-mapping for in buf failed %d\n", ret);
		return ret;
	}

	addr = usb_audio->out_buf_addr;
	if (DEBUG)
		pr_info("PCM OUT BUFFER FREE: paddr = %#08llx\n", addr);

	ret = abox_iommu_unmap(dev, USB_AUDIO_PCM_OUTBUF);
	if (ret < 0) {
		pr_err("iommu unmapping for pcm out buf failed\n");
		return ret;
	}

	/* unmapping usb descriptor */
	addr = usb_audio->desc_addr;
	offset = usb_audio->offset;

	if (DEBUG)
		pr_info("DESC BUFFER: paddr : %#08llx / offset : %#08llx\n",
				addr, offset);

	ret = abox_iommu_unmap(dev, USB_AUDIO_DESC);
	if (ret < 0) {
		pr_err("iommu unmapping for descriptor failed\n");
		return ret;
	}

	ret = abox_iommu_unmap(dev, USB_AUDIO_SAVE_RESTORE);
	if (ret < 0) {
		pr_err(" abox iommu unmapping for dcbaa failed\n");
		return ret;
	}

	/*Device Context unmapping*/
	ret = abox_iommu_unmap(dev, USB_AUDIO_DEV_CTX);
	if (ret < 0) {
		pr_err(" abox iommu unmapping for device ctx failed\n");
		return ret;
	}

	/*Input Context unmapping*/
	ret = abox_iommu_unmap(dev, USB_AUDIO_INPUT_CTX);
	if (ret < 0) {
		pr_err(" abox iommu unmapping for input ctx failed\n");
		return ret;
	}

	/*ERST unmapping*/
	ret = abox_iommu_unmap(dev, USB_AUDIO_ERST);
	if (ret < 0) {
		pr_err(" abox iommu Un-mapping for erst failed\n");
		return ret;
	}

	ret = abox_iommu_unmap(dev, USB_AUDIO_IN_DEQ);
	if (ret < 0) {
		pr_err("abox iommu un-mapping for in buf failed %d\n", ret);
		return ret;
	}

	ret = abox_iommu_unmap(dev, USB_AUDIO_FBOUT_DEQ);
	if (ret < 0) {
		pr_err("abox iommu un-mapping for fb_out buf failed %d\n", ret);
		return ret;
	}

	ret = abox_iommu_unmap(dev, USB_AUDIO_OUT_DEQ);
	if (ret < 0) {
		pr_err("iommu un-mapping for out buf failed %d\n", ret);
		return ret;
	}

	ret = abox_iommu_unmap(dev, USB_AUDIO_FBIN_DEQ);
	if (ret < 0) {
		pr_err("iommu un-mapping for fb_in buf failed %d\n", ret);
		return ret;
	}

	ret = abox_iommu_unmap(dev, USB_AUDIO_XHCI_BASE);
	if (ret < 0) {
		pr_err(" abox iommu Un-mapping for in buf failed\n");
		return ret;
	}

	return 0;
}

int exynos_usb_audio_exit(void)
{
	/* future use */
	return 0;
}

