#ifndef _WACOM_I2C_FLASH_H_
#define _WACOM_I2C_FLASH_H_

/* --------------------------------------------- */
#define FLASH_START0				'f'
#define FLASH_START1				'l'
#define FLASH_START2				'a'
#define FLASH_START3				's'
#define FLASH_START4				'h'
#define FLASH_START5				'\r'
#define FLASH_ACK				0x06

#define WACOM_QUERY_SIZE			19
#define pen_QUERY				'*'
#define ACK					0

#define MPU_W9018				0x42

#define FLASH_BLOCK_SIZE			256
#define DATA_SIZE				(65536 * 2)
#define BLOCK_NUM				31
#define W9018_START_ADDR			0x3000
#define W9018_END_ADDR				0x1efff

#define CMD_GET_FEATURE				2
#define CMD_SET_FEATURE				3

/* Added for using this prog. in Linux user-space program */
#define RTYPE_FEATURE				0x03	/* : Report type -> feature(11b) */
#define GET_FEATURE				0x02
#define SET_FEATURE				0x03
#define GFEATURE_SIZE				6
#define SFEATURE_SIZE				8

#define COMM_REG				0x04
#define DATA_REG				0x05
#define REPORT_ID_1				0x07
#define REPORT_ID_2				0x08
#define BOOT_QUERY_SIZE				5

#define REPORT_ID_ENTER_BOOT			2
#define BOOT_CMD_SIZE				(0x010c + 0x02)	/* 78 */
#define BOOT_RSP_SIZE				6
#define BOOT_CMD_REPORT_ID			7
#define BOOT_WRITE_FLASH			1
#define BOOT_EXIT				3
#define BOOT_BLVER				4
#define BOOT_MPU				5
#define BOOT_QUERY				7

#define QUERY_CMD				0x07
#define BOOT_CMD				0x04
#define MPU_CMD					0x05
/* #define ERS_CMD				0x00 */
#define ERS_ALL_CMD				0x10
#define WRITE_CMD				0x01

#define ERS_ECH2				0x03
#define QUERY_RSP				0x06

#define CMD_SIZE				(72 + 6)
#define RSP_SIZE				6

#define PROCESS_INPROGRESS			0xff
#define PROCESS_COMPLETED			0x00
#define PROCESS_CHKSUM1_ERR			0x81
#define PROCESS_CHKSUM2_ERR			0x82
#define PROCESS_TIMEOUT_ERR			0x87
#define RETRY_COUNT				5

/*
 * exit codes
 */
#define EXIT_OK					(0)
#define EXIT_REBOOT				(1)
#define EXIT_FAIL				(2)
#define EXIT_USAGE				(3)
#define EXIT_NO_SUCH_FILE			(4)
#define EXIT_NO_INTEL_HEX			(5)
#define EXIT_FAIL_OPEN_COM_PORT			(6)
#define EXIT_FAIL_ENTER_FLASH_MODE		(7)
#define EXIT_FAIL_FLASH_QUERY			(8)
#define EXIT_FAIL_BAUDRATE_CHANGE		(9)
#define EXIT_FAIL_WRITE_FIRMWARE		(10)
#define EXIT_FAIL_EXIT_FLASH_MODE		(11)
#define EXIT_CANCEL_UPDATE			(12)
#define EXIT_SUCCESS_UPDATE			(13)
#define EXIT_FAIL_HID2SERIAL			(14)
#define EXIT_FAIL_VERIFY_FIRMWARE		(15)
#define EXIT_FAIL_MAKE_WRITING_MARK		(16)
#define EXIT_FAIL_ERASE_WRITING_MARK		(17)
#define EXIT_FAIL_READ_WRITING_MARK		(18)
#define EXIT_EXIST_MARKING			(19)
#define EXIT_FAIL_MISMATCHING			(20)
#define EXIT_FAIL_ERASE				(21)
#define EXIT_FAIL_GET_BOOT_LOADER_VERSION	(22)
#define EXIT_FAIL_GET_MPU_TYPE			(23)
#define EXIT_MISMATCH_BOOTLOADER		(24)
#define EXIT_MISMATCH_MPUTYPE			(25)
#define EXIT_FAIL_ERASE_BOOT			(26)
#define EXIT_FAIL_WRITE_BOOTLOADER		(27)
#define EXIT_FAIL_SWAP_BOOT			(28)
#define EXIT_FAIL_WRITE_DATA			(29)
#define EXIT_FAIL_GET_FIRMWARE_VERSION		(30)
#define EXIT_FAIL_GET_UNIT_ID			(31)
#define EXIT_FAIL_SEND_STOP_COMMAND		(32)
#define EXIT_FAIL_SEND_QUERY_COMMAND		(33)
#define EXIT_NOT_FILE_FOR_535			(34)
#define EXIT_NOT_FILE_FOR_514			(35)
#define EXIT_NOT_FILE_FOR_503			(36)
#define EXIT_MISMATCH_MPU_TYPE			(37)
#define EXIT_NOT_FILE_FOR_515			(38)
#define EXIT_NOT_FILE_FOR_1024			(39)
#define EXIT_FAIL_VERIFY_WRITING_MARK		(40)
#define EXIT_DEVICE_NOT_FOUND			(41)
#define EXIT_FAIL_WRITING_MARK_NOT_SET		(42)
#define EXIT_FAIL_SET_PDCT			(43)
#define ERR_SET_PDCT				(44)
#define ERR_GET_PDCT				(45)

#endif /* _WACOM_I2C_FLASH_H_ */
