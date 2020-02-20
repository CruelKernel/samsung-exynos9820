/*
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Egor Ulesykiy, <e.uelyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __ICD_PROTECT_LIST_GEN_H
#define __ICD_PROTECT_LIST_GEN_H

/* TODO: This file should be generated automatically */

static const char * const tz_drm_list[] = {
	"/system/bin/mediaserver",
	"/system/bin/drmserver",
	"/system/bin/scranton_RD",
	"/system/bin/qseecomd",
	"/system/bin/taadaemon",
	"/system/bin/tzdaemon",
	"/system/bin/icd_tz_drm",
	NULL,
};

static const char * const fido_list[] = {
	"/system/bin/qseecomd",
	"/system/vendor/bin/qseecomd",
	"/vendor/bin/qseecomd",
	"/system/bin/mcDriverDaemon",
	"/system/bin/mcDriverDaemon_static",
	"/system/vendor/bin/mcDriverDaemon",
	"/vendor/bin/mcDriverDaemon",
	"/system/vendor/bin/mcDriverDaemon_static",
	"/vendor/bin/mcDriverDaemon_static",
	"/system/vendor/bin/hw/vendor.samsung.hardware.biometrics.fingerprint@2.1-service",
	"/vendor/bin/hw/vendor.samsung.hardware.biometrics.fingerprint@2.1-service",
	"/system/bin/irisd",
	"/system/bin/faced",
	"/system/bin/icd_fido",
	NULL,
};

static const char * const cc_list[] = {
	"/system/bin/charon",
	"/system/bin/gatekeeperd",
	"/system/bin/keystore",
	"/system/bin/mcDriverDaemon",
	"/system/bin/mdf_fota",
	"/system/bin/netd",
	"/system/bin/qseecomd",
	"/system/bin/restorecon",
	"/system/bin/sdp_cryptod",
	"/system/bin/secure_storage_daemon",
	"/system/bin/time_daemon",
	"/system/bin/vold",
	"/system/bin/wlandutservice",
	"/system/bin/hw/macloader",
	"/system/bin/hw/mfgloader",
	"/system/bin/hw/wpa_supplicant",
	"/system/bin/icd_cc",
	NULL,
};

static const char * const etc_list[] = {
	"/system/bin/icd_etc",
	NULL,
};

#endif /* __ICD_PROTECT_LIST_GEN_H */
