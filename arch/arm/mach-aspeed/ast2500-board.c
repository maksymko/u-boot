#include <common.h>
#include <asm/io.h>
#include <asm/arch/wdt.h>
#include <linux/err.h>

#include <debug_uart.h>

#define AST_TIMER_BASE			(0x1e782000)

DECLARE_GLOBAL_DATA_PTR;

void lowlevel_init(void)
{
}

int board_early_init_f(void)
{
#ifndef CONFIG_FIRMWARE_2ND_BOOT
	struct ast_wdt *sec_boot_wdt = ast_get_wdt(AST_2ND_BOOT_WDT);
	if (IS_ERR(sec_boot_wdt))
		return PTR_ERR(sec_boot_wdt);
	wdt_stop(sec_boot_wdt);
#endif
	struct ast_wdt *flash_addr_wdt = ast_get_wdt(AST_FLASH_ADDR_DETECT_WDT);
	if (IS_ERR(flash_addr_wdt))
		return PTR_ERR(flash_addr_wdt);
	wdt_stop(flash_addr_wdt);
	debug_uart_init();
	printch('E');
	printascii("<Early Init>\r\n");
	return 0;
}

int board_init(void)
{
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

int dram_init(void)
{
	// Huge TODO
	return 0;
}

