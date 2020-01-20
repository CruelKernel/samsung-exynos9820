/******************************************************************************
 * Copyright (C) 2015 Broadcom Corporation
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 ******************************************************************************/

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/kthread.h>
#include <linux/circ_buf.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/suspend.h>
#include <linux/kernel.h>
#include <linux/ssp_platformdata.h>
#include <linux/gpio.h>
//#include <plat/gpio-cfg.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/kernel_stat.h>

#include "bbd.h"
#include "bcm_gps_spi.h"


bool bcm477x_data_dump(void *);
static const struct bcm_failsafe_data_recordlist g_bcm_failsafe_data_recordlist_4775[] =
{
	{0x800000,   0x120000, 0 }  // CM4 1152 * 1024
	,{0x40400000, 0x8000, 0 }  // CM0   32 * 1024
	,{0x40410FF0, 0x4,    1 }  // CMU Version
	,{0x40202020, 0x40,   1 }  // 8 FOUT registers
	,{0x4040b020, 0x68,   1 }  // I2CM1 Registers
	,{0x4040c020, 0x68,   1 }  // I2CM2 Registers
	,{0x4040d020, 0x68,   1 }  // I2CM3 Registers
	,{0x40500000, 0x8,    1 }  // GIO Registers
	,{0x40500020, 0x8,    1 }  // GIO Registers
	,{0x40500100, 0xc8,   1 }  // GIO Registers
	,{0x40500200, 0x4,    1 }  // GIO Registers
	,{0x40410400, 0x1c,   1 }  // CMU Registers
	,{0x40408000, 0x4c,   1 }  // LPL Registers
	,{0x4050f300, 0x12c,  1 }  // PML Registers
	,{0x00, 0x00, 0 } // the end of list
};


void bcm_filsafe_reset_CPU(struct bcm_spi_priv *priv)
{
	// To reset the CPU you will need to write to the following CMU registers:
	bcm_reg32Iwrite( priv, 0, 0x404100fc, 0xBCBC600D );  // unlock register
	bcm_reg32Iwrite( priv, 0, 0x404100e4, 0xfffffffe );    // bit 0 is the CM4 reset, only 6 bits matter but more don't hurts
}

// void FailSafe::WriteToFile(plain_char *strType, GlIntU32 ulAddress, GlIntU16 usSize, GlIntU8* pucData)
void bcm_filsafe_write2log(char *str_type, unsigned long address, unsigned size, unsigned char * data_p)
{
	const char hex[] = "0123456789abcdef";
	enum {
		CHAR_PER_BYTE = 3,
		BYTES_PER_LINE = 368,
		MAX_TYPE_SIZE = 32,
		MAX_LEN_SIZE = 6, // "(%04x)"
		CHAR_PER_LINE = MAX_TYPE_SIZE + MAX_LEN_SIZE + CHAR_PER_BYTE * BYTES_PER_LINE
	};

	unsigned char line[CHAR_PER_LINE + 1] = {};
	unsigned index = 0;

	if (size == 0 || size > BYTES_PER_LINE)
	{
		pr_info("[SSPBBD]: filsafe_write2log : ERROR *****: usSize = %d\n", (int)strlen(str_type));  
		return;	
	}
	if (str_type)
	{
		if (strlen(str_type) > MAX_TYPE_SIZE)
		{
			pr_info("[SSPBBD]: filsafe_write2log : the length of strType is too long. %d\n", (int)strlen(str_type));
			return;
		}
		index = snprintf(line, MAX_TYPE_SIZE + MAX_LEN_SIZE, "%s(%04x):", str_type, size);
	}
	else
	{
		index = snprintf(line, MAX_TYPE_SIZE + MAX_LEN_SIZE, "%08lx(%04x):", address, size);
	}

	while (size && index < CHAR_PER_LINE - CHAR_PER_BYTE)
	{
		line[index++] = hex[(*data_p) >>  4];
		line[index++] = hex[(*data_p) & 0xf];
		line[index++] = ' ';
		--size;
		++data_p;
	}
	line[index++] = '\n';

	pr_info("[SSPBBD]: filsafe_write2log : %s\n", line); 
}


// void bcm_failsafe_data_recordread(GlIntU32 ulAddress, GlIntU16 usSize, GlIntU8 ucAccessSize, GlIntU8* pucData)
void bcm_failsafe_mem_read(struct bcm_spi_priv *priv)
{
	enum {
		MAX_REG_SIZE  = 32,
	};

	unsigned data_index = 0;
	unsigned data_size = g_bcm_failsafe_data_recordlist_4775[data_index].size;
	unsigned long data_start_addr;
	int data_mem_type;
	unsigned long int flags;

	pr_info("[SSPBBD]: %s ++\n", __func__);

#ifdef CONFIG_TRANSFER_STAT
	bcm_ssi_print_trans_stat(priv);
#endif

	atomic_set(&priv->suspending, 1);

	/* Disable irq */
	spin_lock_irqsave(&priv->irq_lock, flags);
	if (atomic_xchg(&priv->irq_enabled, 0))
		disable_irq_nosync(priv->spi->irq);

	spin_unlock_irqrestore(&priv->irq_lock, flags);

	// TODO: Need to clear circle buffers
	//mutex_lock(&priv->wlock);
	//mutex_lock(&priv->rlock);

	/* Reset circ buffer */
	//priv->read_buf.head = priv->read_buf.tail = 0;
	//priv->write_buf.head = priv->write_buf.tail = 0;

	//mutex_unlock(&priv->wlock);
	//mutex_unlock(&priv->rlock);

	// TODO: Because sensors are enabled, work queue is in forever loop and cannot be flushed 
	//       Need to turn off MCU : 

	while (data_size && data_index < sizeof(g_bcm_failsafe_data_recordlist_4775)/sizeof(struct bcm_failsafe_data_recordlist))
	{
		data_start_addr = g_bcm_failsafe_data_recordlist_4775[data_index].start_addr;
		data_mem_type = g_bcm_failsafe_data_recordlist_4775[data_index].mem_type;

		if ( data_mem_type == 0 )  // memory
		{
		} else if ( data_mem_type == 1 ) // Indirect register
		{
			unsigned regval[MAX_REG_SIZE / 4];
			while ( data_size )
			{   
				unsigned reg_num_size = data_size >= MAX_REG_SIZE ? MAX_REG_SIZE: data_size;
				reg_num_size &= ~3;  // aligned at 4 bytes because it is registers                           
				bcm_reg32Iread( priv,0, data_start_addr, regval, (reg_num_size / 4) );
				bcm_filsafe_write2log(0, data_start_addr, reg_num_size, (unsigned char *)regval);   
				data_start_addr += reg_num_size;
				data_size -= reg_num_size;          
			}

		}  else {
			pr_info("[SSPBBD]: filsafe_mem_read : unknown memory type. %d in index %d\n", data_mem_type, data_index ); 
		}

		data_size = g_bcm_failsafe_data_recordlist_4775[data_index].size;
		data_index++; 
	}

#ifdef CONFIG_TRANSFER_STAT
	bcm_ssi_clear_trans_stat(priv);
#endif     

	/* Reset circ buffer */
	// priv->read_buf.head = priv->read_buf.tail = 0;
	// priv->write_buf.head = priv->write_buf.tail = 0;

	priv->packet_received = 0;

	atomic_set(&priv->suspending, 0);

	/* Enable irq */
	spin_lock_irqsave(&priv->irq_lock, flags);
	if (!atomic_xchg(&priv->irq_enabled, 1))
		enable_irq(priv->spi->irq);

	spin_unlock_irqrestore(&priv->irq_lock, flags);     

	pr_info("[SSPBBD]: %s --\n", __func__);
}    


// SWGNSSAND-1660, FW4775-1147
bool bcm477x_data_dump(void *p)
{
	unsigned long start_time, delta;
	struct bcm_spi_priv *priv = (struct bcm_spi_priv *)p;

	start_time = bcm_clock_get_ms();

	if ( p == NULL )
	{
		priv = bcm_get_bcm_gps();
	}
	// printk("[SSPBBD] filsafe_write2log : Reset the CPU\n");     
	// bcm_filsafe_reset_CPU(priv); 

	printk("[SSPBBD] filsafe_write2log : dump started\n");            
	bcm_failsafe_mem_read(priv);

#if 0    
	// gpio_set_value(priv->mcu_req, 1);
	// mdelay(15);

	RegRead( priv,  "HSI_ERROR_STATUS(R) ", HSI_ERROR_STATUS, &regvalc, 1 );
	bcm_reg32Iread( priv,"HSI_CTRL " ,HSI_CTRL, regval,4 );
	bcm_reg32Iread( priv,"RNGDMA_RX" ,HSI_RNGDMA_RX_BASE_ADDR, regval,4 );
	bcm_reg32Iread( priv,"ADDR_OFFS" ,HSI_RNGDMA_RX_SW_ADDR_OFFSET, regval,4 );
	bcm_reg32Iread( priv,"RNGDMA_TX" ,HSI_RNGDMA_TX_BASE_ADDR, regval,4 );
	bcm_reg32Iread( priv,"ADDR_OFFS" ,HSI_RNGDMA_TX_SW_ADDR_OFFSET, regval,4 );
	bcm_reg32Iread( priv,"ADL_ABR  " ,HSI_ADL_ABR_CONTROL    , regval,4 );
#endif     

	delta = bcm_clock_get_ms() - start_time;

	printk("[SSPBBD] filsafe_write2log : dump consumed %lu = clock_get_ms() - bcm_start_time; msec\n", delta);

	return true;
}
