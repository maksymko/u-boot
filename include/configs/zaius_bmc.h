/*
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 *
 * Copyright 2016 Google Inc
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/aspeed-common.h>

#define CONFIG_SYS_MEMTEST_START	(CONFIG_SYS_SDRAM_BASE + 0x300000)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x5000000)

#define CONFIG_SYS_UBOOT_BASE		CONFIG_SYS_TEXT_BASE

/* Memory Info */
#define CONFIG_SYS_LOAD_ADDR		0x83000000

#define CONFIG_ENV_SIZE			0x20000

#define CONFIG_ID_EEPROM

#define CONFIG_RESET_PHY_R

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND \
		"fdt addr 20080000; " \
		"if fdt get value ramdisk_conf /configurations/conf@1 ramdisk; then " \
		"    bootm 20080000; else bootm 20080000 20300000; " \
		"fi"

#endif	/* __CONFIG_H */