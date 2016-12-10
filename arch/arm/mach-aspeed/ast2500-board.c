#include <common.h>
#include <asm/arch/timer.h>
#include <asm/arch/wdt.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/uclass.h>
#include <linux/err.h>
#include <ram.h>
#include <timer.h>

/* Second Watchdog Timer by default is configured
 * to trigger secondary boot source.
 */
#define AST_2ND_BOOT_WDT		(1)

/* Third Watchdog Timer by default is configured
 * to toggle Flash address mode switch before reset.
 */
#define AST_FLASH_ADDR_DETECT_WDT	(2)

#include <debug_uart.h>

DECLARE_GLOBAL_DATA_PTR;

static int stop_wdts(void)
{
	struct ast_wdt *flash_addr_wdt = ast_get_wdt(AST_FLASH_ADDR_DETECT_WDT);
	if (IS_ERR(flash_addr_wdt))
		return PTR_ERR(flash_addr_wdt);
	wdt_stop(flash_addr_wdt);

#ifndef CONFIG_FIRMWARE_2ND_BOOT
	struct ast_wdt *sec_boot_wdt = ast_get_wdt(AST_2ND_BOOT_WDT);

	if (IS_ERR(sec_boot_wdt))
		return PTR_ERR(sec_boot_wdt);
	wdt_stop(sec_boot_wdt);
#endif

	return 0;
}

void lowlevel_init(void)
{
}

int mach_cpu_init(void)
{
	/*
	 * The board hangs if I put this in later stages, even in
	 * board_early_init_f
	 */
	debug_uart_init();
	return stop_wdts();
}

int board_early_init_f(void)
{
	printascii("\r\n<Early Init>\r\n");
	return 0;
}

int board_init(void)
{
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

int dram_init(void)
{
	struct udevice *dev;
	int ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		printascii("DRAM FAIL1\r\n");
		return ret;
	}

	struct ram_info ram;
	ret = ram_get_info(dev, &ram);
	if (ret) {
		printascii("DRAM FAIL2\r\n");
		return ret;
	}


	gd->ram_size = ram.size;
	return 0;
}

