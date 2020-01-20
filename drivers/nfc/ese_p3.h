/*
 *
 * Copyright (C) 2015 Samsung Electronics. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

#define DEFAULT_BUFFER_SIZE 260

#define FEATURE_ESE_WAKELOCK

#define P3_MAGIC 0xED
#define P3_SET_PWR _IOW(P3_MAGIC, 0x01, unsigned long)
#define P3_SET_DBG _IOW(P3_MAGIC, 0x02, unsigned long)
#define P3_SET_POLL _IOW(P3_MAGIC, 0x03, unsigned long)

/* To set SPI configurations like gpio, clks */
#define P3_SET_SPI_CONFIG _IO(P3_MAGIC, 0x04)
/*To prepare spi clock */
#define P3_ENABLE_SPI_CLK _IO(P3_MAGIC, 0x05)
/* To unprepare spi clock */
#define P3_DISABLE_SPI_CLK _IO(P3_MAGIC, 0x06)
/* only nonTZ +++++*/
/* Transmit data to the device and retrieve data from it simultaneously.*/
#define P3_RW_SPI_DATA _IOWR(P3_MAGIC, 0x07, unsigned long)
/* only nonTZ -----*/
/* To change SPI clock */
#define P3_SET_SPI_CLK	_IOW(P3_MAGIC, 0x08, unsigned long)
/* To enable spi cs pin (make low) */
#define P3_ENABLE_SPI_CS _IO(P3_MAGIC, 0x09)
/* To disable spi cs pin  */
#define P3_DISABLE_SPI_CS _IO(P3_MAGIC, 0x0A)

/* To enable spi clock & cs */
#define P3_ENABLE_CLK_CS _IO(P3_MAGIC, 0x0B)
/* To disable spi clock & cs */
#define P3_DISABLE_CLK_CS _IO(P3_MAGIC, 0x0C)
/* To swing(shake) cs */
#define P3_SWING_CS _IOW(P3_MAGIC, 0x0D, unsigned long)

#ifdef CONFIG_COMPAT
/*#define P3_RW_SPI_DATA_32 _IOWR(P3_MAGIC, 0x07, unsigned int)*/
struct spip3_ioc_transfer_32 {
	unsigned int   rx_buffer;
	unsigned int   tx_buffer;
	unsigned short  len;
};
#endif

struct p3_ioctl_transfer {
	unsigned char *rx_buffer;
	unsigned char *tx_buffer;
	unsigned int len;
};

struct p3_spi_platform_data {
	unsigned int irq_gpio;
	unsigned int rst_gpio;
};
