#include <common.h>
#include <asm/io.h>

#include <debug_uart.h>

#define AST_TIMER_BASE			(0x1e782000)
#define AST_WDT_BASE			(0x1e785000)

DECLARE_GLOBAL_DATA_PTR;

void lowlevel_init(void)
{
}

int board_early_init_f(void)
{
	writel(0xae, AST_TIMER_BASE + 0x38);
	writel(0xf000, AST_TIMER_BASE + 0x3c);
	writel(~0, AST_TIMER_BASE + 0x44);
	writel((3 << 12), AST_TIMER_BASE + 0x30);
#ifndef CONFIG_FIRMWARE_2ND_BOOT
	writel(0, AST_WDT_BASE + 0x2c);
#endif
	writel(0, AST_WDT_BASE + 0x4c);
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

