#ifndef _ASM_ARCH_WDT_H
#define _ASM_ARCH_WDT_H

#define WDT_BASE			(0x1e785000)

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
