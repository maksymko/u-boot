#include <common.h>
#include <asm/arch/wdt.h>
#include <asm/io.h>
#include <linux/err.h>

/* Number of available watchdog timers */
#ifdef CONFIG_ASPEED_AST2500
#define WDT_MAX_NUM			2
#else
#define WDT_MAX_NUM			1
#endif

void wdt_stop(struct ast_wdt* wdt)
{
	clrbits_le32(&wdt->ctrl, WDT_CTRL_EN);
}

void wdt_start(struct ast_wdt* wdt, u32 timeout)
{
	writel(timeout, &wdt->counter_reload_val);
	writel(WDT_COUNTER_RESTART_VAL, &wdt->counter_restart);
	setbits_le32(&wdt->ctrl, WDT_CTRL_EN | WDT_CTRL_RESET);
}

struct ast_wdt *ast_get_wdt(u8 wdt_number)
{
	if (wdt_number > WDT_MAX_NUM)
		return ERR_PTR(-EINVAL);

	return (struct ast_wdt *)(WDT_BASE +
				  sizeof(struct ast_wdt) * wdt_number);
}
