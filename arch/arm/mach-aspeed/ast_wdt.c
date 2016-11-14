#include <common.h>
#include <asm/arch/wdt.h>
#include <asm/io.h>

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

struct ast_wdt* ast_get_wdt(u8 wdt_number)
{
	return (struct ast_wdt*)(WDT_BASE + sizeof(struct ast_wdt) * wdt_number);
}
