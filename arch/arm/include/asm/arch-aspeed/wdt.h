#ifndef _ASM_ARCH_WDT_H
#define _ASM_ARCH_WDT_H

#define WDT_BASE			(0x1e785000)

/* Second Watchdog Timer by default is configured
 * to trigger secondary boot source.
 */
#define AST_2ND_BOOT_WDT		(1)
/* Third Watchdog Timer by default is configured
 * to toggle Flash address mode switch before reset.
 */
#define AST_FLASH_ADDR_DETECT_WDT	(2)

/* Special value that needs to be written to counter_restart register to
 * (re)start the timer.
 */
#define WDT_COUNTER_RESTART_VAL			(0x4755)

#define WDT_CTRL_PRE_TIMEOUT_OFFSET			(12)
#define WDT_CTRL_2ND_BOOT_OFFSET			(7)
#define WDT_CTRL_RESET_MODE_OFFSET			(5)
#define WDT_CTRL_CLK_SELECT_OFFSET			(4)
#define WDT_CTRL_EXT_EN_OFFSET			(3)
#define WDT_CTRL_PRE_TIMEOUT_INTR_OFFSET			(2)
#define WDT_CTRL_RESET_OFFSET			(1)
#define WDT_CTRL_EN_OFFSET			(0)

#define WDT_CTRL_RESET_SOC			(0x0)
#define WDT_CTRL_RESET_CHIP			(0x1)
#define WDT_CTRL_RESET_CPU			(0x2)
#define WDT_CTRL_RESET_MASK			(0x3)

#define WDT_CTRL_EN			(0x1 << WDT_CTRL_EN_OFFSET)
#define WDT_CTRL_RESET			(0x1 << WDT_CTRL_RESET_OFFSET)
#define WDT_RESET_MODE(x)			((x) << WDT_CTRL_RESET_MODE_OFFSET)

#define WDT_RESET_ARM     (1 << 0)
#define WDT_RESET_COPROC  (1 << 1)
#define WDT_RESET_SDRAM (1 << 2)
#define WDT_RESET_AHB (1 << 3)
#define WDT_RESET_I2C (1 << 4)
#define WDT_RESET_MAC1  (1 << 5)
#define WDT_RESET_MAC2  (1 << 6)
#define WDT_RESET_GCRT  (1 << 7)
#define WDT_RESET_USB20 (1 << 8)
#define WDT_RESET_USB11_HOST  (1 << 9)
#define WDT_RESET_USB11_EHCI2 (1 << 10)
#define WDT_RESET_VIDEO (1 << 11)
#define WDT_RESET_HAC (1 << 12)
#define WDT_RESET_LPC (1 << 13)
#define WDT_RESET_SDSDIO  (1 << 14)
#define WDT_RESET_MIC (1 << 15)
#define WDT_RESET_CRT2C (1 << 16)
#define WDT_RESET_PWM (1 << 17)
#define WDT_RESET_PECI  (1 << 18)
#define WDT_RESET_JTAG  (1 << 19)
#define WDT_RESET_ADC (1 << 20)
#define WDT_RESET_GPIO  (1 << 21)
#define WDT_RESET_MCTP  (1 << 22)
#define WDT_RESET_XDMA  (1 << 23)
#define WDT_RESET_SPI (1 << 24)
#define WDT_RESET_MISC  (1 << 25)

#ifndef __ASSEMBLY__
struct ast_wdt {
	u32 counter_status;
	u32 counter_reload_val;
	u32 counter_restart;
	u32 ctrl;
	u32 timeout_status;
	u32 clr_timeout_status;
	u32 reset_width;
#ifdef CONFIG_ASPEED_AST2500
	u32 reset_mask;
#else
	u32 reserved0;
#endif
};

void wdt_stop(struct ast_wdt* wdt);
void wdt_start(struct ast_wdt* wdt, u32 timeout);
struct ast_wdt* ast_get_wdt(u8 wdt_number);
#endif  /* __ASSEMBLY__ */

#endif /* _ASM_ARCH_WDT_H */
