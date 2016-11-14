/* TODO: Move to Sysreset driver */

#include <common.h>
#include <asm/arch/wdt.h>
#include <asm/io.h>

#define AST_WDT_RESET_TIMEOUT			(0x10)

#ifdef CONFIG_AST_WDT_FOR_RESET
void reset_cpu(ulong addr)
{
	struct ast_wdt* wdt = ast_get_wdt(CONFIG_AST_WDT_FOR_RESET);
	// Only reset ARM CPU
	clrbits_le32(&wdt->ctrl, WDT_RESET_MODE(WDT_CTRL_RESET_MASK));
	setbits_le32(&wdt->ctrl, WDT_RESET_MODE(WDT_CTRL_RESET_CPU));
	wdt_start(wdt, AST_WDT_RESET_TIMEOUT);

	while (1) /* Wait for reset */;
}
#endif
