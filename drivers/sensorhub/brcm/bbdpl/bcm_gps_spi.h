/*
 * Copyright 2015 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * The BBD (Broadcom Bridge Driver)
 *
 * tabstop = 8
 */

#ifndef __BCM_GPS_SPI_H__
#define __BCM_GPS_SPI_H__

#define WORD_BURST_SIZE			4
#define CONFIG_SPI_DMA_BYTES_PER_WORD	4
#define CONFIG_SPI_DMA_BITS_PER_WORD	32

#define SSI_MODE_STREAM		0x00
#define SSI_MODE_DEBUG		0x80

#define SSI_MODE_HALF_DUPLEX	0x00
#define SSI_MODE_FULL_DUPLEX	0x40

#define SSI_WRITE_TRANS		0x00
#define SSI_READ_TRANS		0x20

#define SSI_PCKT_1B_LENGTH        0
#define SSI_PCKT_2B_LENGTH        0x10

#define SSI_FLOW_CONTROL_DISABLED 0
#define SSI_FLOW_CONTROL_ENABLED  0x08

#define SSI_WRITE_HD (SSI_WRITE_TRANS | SSI_MODE_HALF_DUPLEX)
#define SSI_READ_HD  (SSI_READ_TRANS  | SSI_MODE_HALF_DUPLEX)

#define HSI_F_MOSI_CTRL_CNT_SHIFT  0
#define HSI_F_MOSI_CTRL_CNT_SIZE   3
#define HSI_F_MOSI_CTRL_CNT_MASK   0x07

#define HSI_F_MOSI_CTRL_SZE_SHIFT  3
#define HSI_F_MOSI_CTRL_SZE_SIZE   2
#define HSI_F_MOSI_CTRL_SZE_MASK   0x18

#define HSI_F_MOSI_CTRL_RSV_SHIFT  5
#define HSI_F_MOSI_CTRL_RSV_SIZE   1
#define HSI_F_MOSI_CTRL_RSV_MASK   0x20

#define HSI_F_MOSI_CTRL_PE_SHIFT   6
#define HSI_F_MOSI_CTRL_PE_SIZE    1
#define HSI_F_MOSI_CTRL_PE_MASK    0x40

#define HSI_F_MOSI_CTRL_PZC_SHIFT   0
#define HSI_F_MOSI_CTRL_PZC_SIZE    (HSI_F_MOSI_CTRL_CNT_SIZE + HSI_F_MOSI_CTRL_SZE_SIZE + HSI_F_MOSI_CTRL_PE_SIZE)
#define HSI_F_MOSI_CTRL_PZC_MASK    (HSI_F_MOSI_CTRL_CNT_MASK | HSI_F_MOSI_CTRL_SZE_MASK | HSI_F_MOSI_CTRL_PE_MASK )

#define HSI_PZC_MAX_RX_BUFFER      0x10000

#define CONFIG_MEM_CORRUPT_CHECK  4
#define CONFIG_MEM_PATTERN     0x1A
#define CONFIG_MEM_ADD         0x11

#define DEBUG_TIME_STAT
#define CONFIG_TRANSFER_STAT
#define CONFIG_REG_IO 1
// unsigned long packet_received;
#define CONFIG_PACKET_RECEIVED (31*1024)
#define WORK_TYPE_RX 1
#define WORK_TYPE_TX 2


struct bcm_spi_strm_protocol  {
	int  pckt_len;
	int  fc_len;
	int  ctrl_len;
	unsigned char  ctrl_byte;
	unsigned short frame_len;
} __attribute__((__packed__));


#ifdef CONFIG_TRANSFER_STAT
struct bcm_spi_transfer_stat  {
	int  len_255;
	int  len_1K;
	int  len_2K; 
	int  len_4K; 
	int  len_8K;
	int  len_16K;
	int  len_32K;
	int  len_64K;
} __attribute__((__packed__));
#endif


// v10/proprietary/deliverables/lhe2_dev/fail_safe/fail_safe.cpp & .h
//   class FailSafe : struct DataRecordList
struct bcm_failsafe_data_recordlist {
     unsigned long  start_addr;
     unsigned       size;
     int            mem_type;
};     

//--------------------------------------------------------------
//
//			   Structs
//
//--------------------------------------------------------------

#define BCM_SPI_READ_BUF_SIZE	(8*PAGE_SIZE)
#define BCM_SPI_WRITE_BUF_SIZE	(8*PAGE_SIZE)

// TODO: limit max payload to 254 because of exynos3 bug
#define MIN_SPI_FRAME_LEN 254
#define MAX_SPI_FRAME_LEN (BCM_SPI_READ_BUF_SIZE / 4)  // TODO: MAX_SPI_FRAME_LEN = 8K should be less TRANSPORT_RX_BUFFER_SIZE, Now it is 12K (SWGNSSAND-1647)

struct bcm_ssi_tx_frame {
	unsigned char cmd;
	unsigned char data[MAX_SPI_FRAME_LEN-1];
} __attribute__((__packed__));

struct bcm_ssi_rx_frame {
	unsigned char status;
	unsigned char data[MAX_SPI_FRAME_LEN-1];
} __attribute__((__packed__));

struct bcm_spi_priv;

struct bcm_rxtx_work {
	struct work_struct work;
	int type;
	struct bcm_spi_priv *priv;
};

struct bcm_spi_priv {
	struct spi_device *spi;

	/* Char device stuff */
	struct miscdevice misc;
	bool busy;
	struct circ_buf read_buf;
	struct circ_buf write_buf;
	struct mutex rlock;			/* Lock for read_buf */
	struct mutex wlock;			/* Lock for write_buf */
	char _read_buf[BCM_SPI_READ_BUF_SIZE];
	char _write_buf[BCM_SPI_WRITE_BUF_SIZE];
	wait_queue_head_t poll_wait;		/* for poll */

		/* GPIO pins */
	int host_req;
	int mcu_req;
	int mcu_resp;

		/* IRQ and its control */
	atomic_t irq_enabled;
	spinlock_t irq_lock;

	/* Work */
	struct bcm_rxtx_work tx_work;
	struct bcm_rxtx_work rx_work;
	struct workqueue_struct *serial_wq;
	atomic_t suspending;

	/* SPI tx/rx buf */
#if CONFIG_MEM_CORRUPT_CHECK    
    unsigned char *chk_mem_frame_1; 
#endif    
	struct bcm_ssi_tx_frame *tx_buf;
#if CONFIG_MEM_CORRUPT_CHECK    
    unsigned char *chk_mem_frame_2; 
#endif    
	struct bcm_ssi_rx_frame *rx_buf;
#if CONFIG_MEM_CORRUPT_CHECK    
    unsigned char *chk_mem_frame_3; 
#endif    

	/* 4775 SPI tx/rx strm protocol */
	struct bcm_spi_strm_protocol tx_strm;
	struct bcm_spi_strm_protocol rx_strm;

#ifdef CONFIG_TRANSFER_STAT
	struct bcm_spi_transfer_stat trans_stat[2];
#endif

	struct wake_lock bcm_wake_lock;

	/* To make sure  that interface is detected on chip side !=0 */
	unsigned long packet_received;
};

/*
 * bcm_gps_regs.cpp
 */
int bcm477x_rpc_get_version(void *prv, char *id);
int RegWrite( struct bcm_spi_priv *priv, char *id, unsigned char cmdRegOffset, unsigned char *cmdRegData,  unsigned char cmdByteNum  );
int RegRead( struct bcm_spi_priv *priv, char *id, unsigned char cmdRegOffset, unsigned char *cmdRegData,  unsigned char cmdByteNum  );
int bcm_reg32Iwrite( struct bcm_spi_priv *priv, char *id, unsigned regaddr, unsigned regval );
int bcm_reg32Iread( struct bcm_spi_priv *priv, char *id, unsigned regaddr, unsigned *regval, int n );

/*
 * bcm_gps_mem.cpp
 */
bool bcm477x_data_dump(void *);

/*
 * bcm_gps_spi.cpp
 */
struct bcm_spi_priv *bcm_get_bcm_gps(void); 
void bcm_ssi_clear_mem_corruption(struct bcm_spi_priv *priv);
void bcm_ssi_check_mem_corruption(struct bcm_spi_priv *priv);
void bcm_ssi_print_trans_stat(struct bcm_spi_priv *priv);
void bcm_ssi_clear_trans_stat(struct bcm_spi_priv *priv);
int bcm_ssi_rx(struct bcm_spi_priv *priv, size_t *length);
int bcm_spi_sync(struct bcm_spi_priv *priv, void *tx_buf, void *rx_buf, int len, int bits_per_word);
unsigned long bcm_clock_get_ms(void);

#endif /* __BBD_H__ */
