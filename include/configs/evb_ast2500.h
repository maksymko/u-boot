/*
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 *
 * Copyright 2016 Google Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/aspeed-common.h>


#define CONFIG_SYS_MEMTEST_START		CONFIG_SYS_SDRAM_BASE + 0x300000
#define CONFIG_SYS_MEMTEST_END			(CONFIG_SYS_MEMTEST_START + (80*1024*1024))

#define CONFIG_SYS_UBOOT_BASE		CONFIG_SYS_TEXT_BASE
/*
 * Memory Info
 */
#define CONFIG_SYS_MALLOC_LEN   	(0x1000 + 4*1024*1024) /* malloc() len */

#define CONFIG_SYS_LOAD_ADDR	0x83000000	/* default load address */

#define CONFIG_BOARD_EARLY_INIT_F

#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_BASE     (0x20000000)
#define CONFIG_FLASH_CFI_DRIVER

#define CONFIG_SYS_TIMER_RATE     CONFIG_SYS_HZ

#endif	/* __CONFIG_H */
