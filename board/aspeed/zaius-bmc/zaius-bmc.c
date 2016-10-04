/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <i2c.h>
#include <netdev.h>
#include <net.h>

#include <asm/arch/ast_scu.h>
#include <asm/arch/ast-sdmc.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

#define RETURN_IF_ERROR(expr) do {\
	int ret = (expr);\
	if (ret < 0) {\
		debug(#expr "in %s failed.\n", __func__);\
		return;\
	}\
} while (0)

#define ECHECKSUM	(2001)

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

struct zaius_fru_common_header {
	uint8_t version;
	uint8_t iua_offset;
	uint8_t cia_offset;
	uint8_t ba_offset;
	uint8_t pia_offset;
	uint8_t mra_offset;
	uint8_t padding;
	uint8_t checksum;
} __attribute__((packed));

struct zaius_fru_iua {
	uint8_t version;
	uint8_t type;
	uint8_t length;
	uint8_t mac_addr[6];
	uint8_t num_mac_addrs;
	uint8_t padding[21];
	uint8_t checksum;
} __attribute__((packed));

static int fru_checksum(void *data, size_t len)
{
	uint8_t result = 0;
	uint8_t *p = (uint8_t *) data;

	size_t i = 0;
	for (; i < len; ++i) {
		result += *p;
	}

	return result == 0 ? 0 : -ECHECKSUM;
}

static void read_macs_from_fru(void)
{
	struct udevice *eeprom;
	RETURN_IF_ERROR(i2c_get_chip_for_busnum
			(CONFIG_EEPROM_I2C_BUS_NUM, CONFIG_EEPROM_I2C_ADDR, 1,
			 &eeprom));

	struct zaius_fru_common_header fru_header;
	RETURN_IF_ERROR(dm_i2c_read
			(eeprom, 0, (uint8_t *) & fru_header,
			 sizeof(fru_header)));
	RETURN_IF_ERROR(fru_checksum(&fru_header, sizeof(fru_header)));

	struct zaius_fru_iua fru_iua;
	RETURN_IF_ERROR(dm_i2c_read
			(eeprom, fru_header.iua_offset * 8,
			 (uint8_t *) & fru_iua, sizeof(fru_iua)));
	RETURN_IF_ERROR(fru_checksum(&fru_iua, sizeof(fru_iua)));
	uint8_t temp_mac_addr[6];
	int i = 0;
	for (; i < fru_iua.num_mac_addrs; ++i) {
		/* env always takes priority */
		if (!eth_getenv_enetaddr_by_index("eth", i, temp_mac_addr)) {
			eth_setenv_enetaddr_by_index("eth", i,
						     fru_iua.mac_addr);
		}
		fru_iua.mac_addr[5] += 1;
	}
}


int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

int dram_init(void)
{
	/* dram_init must store complete ramsize in gd->ram_size */
	u32 vga = ast_scu_get_vga_memsize();
	u32 dram = ast_sdmc_get_mem_size();
	gd->ram_size = (dram - vga);

	return 0;
}

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bd)
{
	read_macs_from_fru();
	return ftgmac100_initialize(bd);
}
#endif
