/*
 * Copyright 2016 Google Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <net.h>

#define RETURN_IF_ERROR(expr) do {\
	int ret = (expr);\
	if (ret < 0) {\
		debug(#expr "in %s failed.\n", __func__);\
		return ret;\
	}\
} while (0)

#define ECHECKSUM	(2001)

int do_mac(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("Not implemented. EEPROM is read-only.\n");
	return 0;
}

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
	for (; i < len; ++i, ++p) {
		result += *p;
	}

	return result == 0 ? 0 : -ECHECKSUM;
}

int mac_read_from_eeprom(void)
{
	debug("Setting MACs from FRU\n");
	struct udevice *eeprom;

	RETURN_IF_ERROR(i2c_get_chip_for_busnum
			(CONFIG_EEPROM_I2C_BUS_NUM, CONFIG_EEPROM_I2C_ADDR, 1,
			 &eeprom));
	RETURN_IF_ERROR(i2c_set_chip_offset_len
			(eeprom, CONFIG_EEPROM_OFFSET_LEN));

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
	debug("Number of Addrs: %d\n", fru_iua.num_mac_addrs);
	for (; i < fru_iua.num_mac_addrs; ++i) {
		/* env always takes priority */
		if (!eth_getenv_enetaddr_by_index("eth", i, temp_mac_addr)) {
			eth_setenv_enetaddr_by_index("eth", i,
						     fru_iua.mac_addr);
		} else {
			debug("eth%daddr already in env", i);
		}
		fru_iua.mac_addr[5] += 1;
	}

	return 0;
}
