/*
 * Copyright (C) 2018 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/shm_ipc.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_dump.h"
#include "link_device_memory.h"

struct pktproc_global_desc *pktproc_gdesc;
struct pktproc_desc *pktproc_desc;
u8 __iomem *pktproc_data;

int pktproc_pio_rx(struct link_device *ld)
{
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct sk_buff *skb = NULL;
	int ret;
	u8 *src;
	u32 out = pktproc_gdesc->rear_ptr;
	u16 len = pktproc_desc[out].length;
	u32 usage = circ_get_usage(pktproc_gdesc->num_of_desc,
				   pktproc_gdesc->fore_ptr,
				   pktproc_gdesc->rear_ptr);

	if (pktproc_gdesc->num_of_desc - usage < 50)
		mif_err_limited("could be buffer shortage... %d\n", usage);

	if (len > MAX_PKTPROC_SKB_SIZE) {
		mif_err_limited("len is %d. skip this.\n", len);
		ret = -EPERM;
		goto rx_error;
	}
	if (pktproc_desc[out].channel_id == 0xFF) {
		mif_err_limited("channel_id is invalid(0x%x@%d). skip this.\n",
				pktproc_desc[out].channel_id, out);
		ret = -EPERM;
		goto rx_error;
	}

	src = (u8 *)pktproc_data +
		(pktproc_desc[out].data_addr & ~PKTPROC_CP_BASE_ADDR_MASK) - PKTPROC_DATA_BASE_OFFSET;
	if ((src < pktproc_data) || (src > pktproc_data + shm_get_pktproc_data_size())) {
		mif_err_limited("src range error:%pK base:%llx size:0x%x\n",
				src, virt_to_phys(pktproc_data), shm_get_pktproc_data_size());
		ret = -EINVAL;
		goto rx_error;
	}
	__inval_dcache_area(src, len);
#ifdef PKTPROC_DEBUG
	mif_info("addr:0x%08X len:%d ch:%d\n", pktproc_desc[out].data_addr, len, pktproc_desc[out].channel_id);
#endif

#ifdef CONFIG_LINK_DEVICE_NAPI
	skb = napi_alloc_skb(&mld->mld_napi, len);
#else
	skb = dev_alloc_skb(len);
#endif
	if (unlikely(!skb)) {
		mif_err_limited("alloc_skb() error\n");
		return -ENOMEM;
	}
	skb_put(skb, len);
	skb_copy_to_linear_data(skb, src, len);
#ifdef PKTPROC_DEBUG
	pr_buffer("pktproc", (char *)skb->data, (size_t)len, (size_t)40);
#endif

	skbpriv(skb)->lnk_hdr = 0;
	skbpriv(skb)->sipc_ch = pktproc_desc[out].channel_id;
	skbpriv(skb)->iod = link_get_iod_with_channel(ld, skbpriv(skb)->sipc_ch);
	skbpriv(skb)->ld = ld;

	pass_skb_to_net(mld, skb);

	pktproc_gdesc->rear_ptr = circ_new_ptr(pktproc_gdesc->num_of_desc, pktproc_gdesc->rear_ptr, 1);

	return 0;

rx_error:
	if (skb)
		dev_kfree_skb_any(skb);
	pktproc_gdesc->rear_ptr = circ_new_ptr(pktproc_gdesc->num_of_desc, pktproc_gdesc->rear_ptr, 1);

	return ret;
}

/*
 * sysfs
 */
static ssize_t pktproc_gdesc_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	count += sprintf(&buf[count], "Global descriptor:\n");
	count += sprintf(&buf[count], " desc_base_addr:0x%08X\n", pktproc_gdesc->desc_base_addr);
	count += sprintf(&buf[count], " num_of_desc:0x%08X\n", pktproc_gdesc->num_of_desc);
	count += sprintf(&buf[count], " fore_ptr:%d(0x%X)\n", pktproc_gdesc->fore_ptr, pktproc_gdesc->fore_ptr);
	count += sprintf(&buf[count], " rear_ptr:%d(0x%X)\n", pktproc_gdesc->rear_ptr, pktproc_gdesc->rear_ptr);

	return count;
}

static DEVICE_ATTR_RO(pktproc_gdesc_info);

static struct attribute *pktproc_attrs[] = {
	&dev_attr_pktproc_gdesc_info.attr,
	NULL,
};

static const struct attribute_group pktproc_group = {
	.attrs = pktproc_attrs,
	.name = "pktproc",
};

/*
 * init
 */
int setup_pktproc(struct platform_device *pdev)
{
	u8 __iomem *gdesc_base = shm_get_pktproc_desc_region();

	pktproc_gdesc = (struct pktproc_global_desc *)gdesc_base;
	pktproc_gdesc->desc_base_addr = PKTPROC_CP_BASE_ADDR_MASK | PKTPROC_DESC_BASE_OFFSET;
	pktproc_gdesc->num_of_desc = MAX_PKTPROC_DESC_NUM;
	pktproc_gdesc->data_base_addr = PKTPROC_CP_BASE_ADDR_MASK | PKTPROC_DATA_BASE_OFFSET;
	pktproc_gdesc->fore_ptr = 0;
	pktproc_gdesc->rear_ptr = 0;

	if (!pktproc_data) {
		if (sysfs_create_group(&pdev->dev.kobj, &pktproc_group))
			mif_err("failed to create sysfs node related pktproc\n");
	}

	pktproc_desc = (struct pktproc_desc *)(gdesc_base + PKTPROC_DESC_BASE_OFFSET);

	pktproc_data = shm_get_pktproc_data_region();
	__inval_dcache_area(pktproc_data, shm_get_pktproc_data_size());

	mif_info("addr data:%llx legacy:%llx\n",
		virt_to_phys(shm_get_pktproc_data_base_addr()), virt_to_phys(shm_get_zmb_region()));
	mif_info("size desc:0x%08X data:0x%08X legacy:0x%08X\n",
		shm_get_pktproc_desc_size(), shm_get_pktproc_data_size(), shm_get_zmb_size());

	return 0;

}
