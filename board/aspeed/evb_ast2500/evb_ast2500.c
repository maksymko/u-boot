/*
 * Copyright (c) 2016 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <misc.h>
#include <net.h>
#include <fdtdec.h>
#include <dm/uclass.h>

#define ECHECKSUM	(2001)
#define MACADDR_SIZE	(6)

DECLARE_GLOBAL_DATA_PTR;

static struct __attribute__((packed)) {
	uint8_t version;
	uint8_t iua_offset;
	uint8_t cia_offset;
	uint8_t ba_offset;
	uint8_t pia_offset;
	uint8_t mra_offset;
	uint8_t padding;
	uint8_t checksum;
} fru_header;

static struct __attribute__((packed)) {
	uint8_t version;
	uint8_t type;
	uint8_t length;
	uint8_t mac_addr[MACADDR_SIZE];
	uint8_t num_mac_addrs;
	uint8_t padding[21];
	uint8_t checksum;
} fru_iua;

static bool is_eeprom_read = false;

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

static int read_eeprom(void)
{
	struct udevice *eeprom = NULL;
	int node;
	int ret;

	if (is_eeprom_read)
		return 0;

	node = fdtdec_get_chosen_node(gd->fdt_blob, "fru-eeprom");
	if (node < 0) {
		debug("FRU EEPROM chosen node not found: %d\n", node);

		return node;
	}

	ret = uclass_get_device_by_of_offset(UCLASS_I2C_EEPROM, node, &eeprom);
	if (ret) {
		debug("Can't get FRU EEPROM device: %d", ret);
		return ret;
	}

	ret = misc_read(eeprom, 0, &fru_header, sizeof(fru_header));
	if (ret) {
		debug("FRU Header read failure: %d\n", ret);
		return ret;
	}

	ret = fru_checksum(&fru_header, sizeof(fru_header));
	if (ret) {
		debug("FRU Header checksum failed: %d\n", ret);
		return ret;
	}

	ret = misc_read(eeprom, fru_header.iua_offset * 8, &fru_iua, sizeof(fru_iua));
	if (ret) {
		debug("FRU IUA read failure: %d\n", ret);
		return ret;
	}

	ret = fru_checksum(&fru_iua, sizeof(fru_iua));
	if (ret) {
		debug("FRU IUA checksum failed: %d\n", ret);
		return ret;
	}

	is_eeprom_read = true;

	return 0;
}

/* CONFIG_ID_EEPROM setting, that enables mac_read_from_eeprom call
 * in board_r also enables this function/command.
 * We don't need any functionality of this command, definitely don't need
 * eeprom writing functionality, so I just made it print info about MACs.
 */
int do_mac(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = read_eeprom();
	if (ret < 0) {
		printf("Can't read EEPROM: %d\n", ret);
		return ret;
	}

	printf("Number of MAC Addrs in EEPROM: %d\n", fru_iua.num_mac_addrs);
	uint8_t fru_mac_addr[MACADDR_SIZE];
	memcpy(fru_mac_addr, fru_iua.mac_addr, MACADDR_SIZE);
	int i = 0;
	for (; i < fru_iua.num_mac_addrs; ++i) {
		printf("#%d: %02x:%02x:%02x:%02x:%02x:%02x\n", i + 1,
			   fru_mac_addr[0],
			   fru_mac_addr[1],
			   fru_mac_addr[2],
			   fru_mac_addr[3],
			   fru_mac_addr[4],
			   fru_mac_addr[5]);
		fru_mac_addr[5] += 1;
	}
	return 0;
}

int mac_read_from_eeprom(void)
{
	debug("Setting MACs from FRU\n");
	/* The failure to read eeprom is not fatal to boot,
	 * so just returning zero anyway.
	 */
	int err = read_eeprom();
	if (err < 0) {
		debug("Failed to read FRU EEPROM\n");
		return 0;
	}

	uint8_t temp_mac_addr[MACADDR_SIZE];
	uint8_t fru_mac_addr[MACADDR_SIZE];
	memcpy(fru_mac_addr, fru_iua.mac_addr, MACADDR_SIZE);

	int i = 0;
	debug("Number of Addrs: %d\n", fru_iua.num_mac_addrs);
	for (; i < fru_iua.num_mac_addrs; ++i) {
		/* env always takes priority */
		if (!eth_getenv_enetaddr_by_index("eth", i, temp_mac_addr)) {
			eth_setenv_enetaddr_by_index("eth", i,
						     fru_mac_addr);
		} else {
			debug("eth%daddr already in env\n", i);
		}
		fru_mac_addr[5] += 1;
	}

	return 0;
}
