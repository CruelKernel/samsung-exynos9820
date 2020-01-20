#ifndef FIMC_IS_EEPROM_FRONT_5E3_V001_H
#define FIMC_IS_EEPROM_FRONT_5E3_V001_H

/* Header referenced section */
#define ROM_HEADER_CAL_MAP_VER_START_ADDR_FRONT  0x30
#define ROM_HEADER_VERSION_START_ADDR_FRONT      0x20
#define ROM_HEADER_AWB_START_ADDR_FRONT          0x0
#define ROM_HEADER_AWB_END_ADDR_FRONT            0x4
#define ROM_HEADER_AP_SHADING_START_ADDR_FRONT   0x8
#define ROM_HEADER_AP_SHADING_END_ADDR_FRONT     0xC
#define ROM_HEADER_PROJECT_NAME_START_ADDR_FRONT 0x38

/* AWB referenced section */
#define ROM_AWB_VER_START_ADDR_FRONT             0x200

/* AP Shading referenced section */
#define ROM_AP_SHADING_VER_START_ADDR_FRONT      0x300

/* Checksum referenced section */
#define ROM_CHECKSUM_HEADER_ADDR_FRONT           0xFC
#define ROM_CHECKSUM_OEM_ADDR_FRONT              0x1FC
#define ROM_CHECKSUM_AWB_ADDR_FRONT              0x2FC
#define ROM_CHECKSUM_AP_SHADING_ADDR_FRONT       0x1FFC

/* etc section */
#define FIMC_IS_MAX_CAL_SIZE_FRONT               (8 * 1024)
#define HEADER_CRC32_LEN_FRONT                   (96)

#endif /* FIMC_IS_EEPROM_FRONT_5E3_V001_H */
