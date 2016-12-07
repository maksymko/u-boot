/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <asm/arch/scu_ast2500.h>
#include <asm/io.h>
#include <clk-uclass.h>
#include <dm.h>
#include <dm/lists.h>
#include <dt-bindings/clock/ast2500-scu.h>
#include <timer.h>

#include <debug_uart.h>

DECLARE_GLOBAL_DATA_PTR;

/* For H-PLL and M-PLL the formula is
 * (Output Frequency) = CLKIN * ((M + 1) / (N + 1)) / (P + 1)
 * M - Numerator
 * N - Denumerator
 * P - Post Divider
 * They have the same layout in their control register.
 */
#define _HM_NUM_SHIFT			(5)
#define _HM_NUM_MASK			(0xff)
#define _HM_DENUM_SHIFT			(0)
#define _HM_DENUM_MASK			(0x1f)
#define _HM_POSTDIV_SHIFT		(0x13)
#define _HM_POSTDIV_MASK		(0x3f)

#define _get_masked(var, mask, shift)	(((var) >> shift) & mask)

#define _HM_NUM(reg)	_get_masked(reg, _HM_NUM_MASK, _HM_NUM_SHIFT)
#define _HM_DENUM(reg)	_get_masked(reg, _HM_DENUM_MASK, _HM_DENUM_SHIFT)
#define _HM_POSTDIV(reg) _get_masked(reg, _HM_POSTDIV_MASK, _HM_POSTDIV_SHIFT)

static inline ulong ast2500_get_hm_pll_rate(u32 clkin, u32 pll_param)
{
	u32 multiplier = (_HM_NUM(pll_param) + 1) / (_HM_DENUM(pll_param) + 1);
	ulong prediv = clkin * multiplier;
	return prediv / (_HM_POSTDIV(pll_param) + 1);
}

static inline u32 ast2500_get_clkin(struct ast2500_scu *scu)
{
	return readl(&scu->hwstrap) & AST_SCU_HWSTRAP_CLKIN_25MHZ
			? 25*1000*1000 : 24*1000*1000;
}

static ulong ast2500_clk_get_rate(struct clk *clk)
{
	struct ast2500_clk_priv *priv = dev_get_priv(clk->dev);

	u32 clkin = ast2500_get_clkin(priv->scu);
	ulong rate;
	switch (clk->id) {
	case PLL_HPLL:
	case ARMCLK:
		/* This ignores dynamic/static slowdown of ARMCLK and may
		 * be inacurate. */
		rate =
		    ast2500_get_hm_pll_rate(clkin,
					    readl(&priv->scu->h_pll_param));
		break;
	case MCLK_DDR:
		rate =
		    ast2500_get_hm_pll_rate(clkin,
					    readl(&priv->scu->m_pll_param));
		break;
	default:
		return -ENOENT;
	}

	return rate;
}

static ulong ast2500_configure_ddr(struct ast2500_scu *scu, ulong rate)
{
	u32 clkin = ast2500_get_clkin(scu);

	ulong new_rate = rate;
	u16 num;
	u16 denum = 1;
	u16 post_div = 1;

	/* First, see if we can get away with just num. */
	u32 rem = new_rate % clkin;
	num = new_rate / clkin;
	denum = 1;
	post_div = 1;
	if (rem > 0) {
		u32 rev_err = clkin / rem;
		if (rev_err * num < (_HM_NUM_MASK + 1) && (rev_err < (_HM_POSTDIV_MASK + 1))) {
			/* Some of the error can be corrected. */
			num *= rev_err;
			post_div = rev_err;
		}
	}

	/* To get values to set in the register, subtract 1 */
	num -= 1;
	denum -= 1;
	post_div -= 1;

	u32 mpll_reg = readl(&scu->m_pll_param);
	const u32 div_mask = (_HM_NUM_MASK << _HM_NUM_SHIFT) | (_HM_DENUM_MASK << _HM_DENUM_SHIFT) | (_HM_POSTDIV_MASK << _HM_POSTDIV_SHIFT);
	mpll_reg &= ~div_mask;
	mpll_reg |= ((_HM_NUM_MASK & num) << _HM_NUM_SHIFT) | ((_HM_DENUM_MASK & denum) << _HM_DENUM_SHIFT) | ((_HM_POSTDIV_MASK & post_div) << _HM_POSTDIV_SHIFT);

	printascii("MPLL Set To: ");
	printhex8(mpll_reg);
	printascii("\r\n");

	/* FIXME */
	/* writel(mpll_reg, &scu->m_pll_param); */
	writel(0x93002400, &scu->m_pll_param);

	mdelay(3);

	return ast2500_get_hm_pll_rate(clkin, mpll_reg);
}

static ulong ast2500_clk_set_rate(struct clk *clk, ulong rate)
{
	printascii("Set CLK Rate");
	printhex4(clk->id);
	printascii("\r\n");
	struct ast2500_clk_priv *priv = dev_get_priv(clk->dev);

	ulong new_rate;
	switch (clk->id) {
	case PLL_MPLL:
	case MCLK_DDR:
		new_rate = ast2500_configure_ddr(priv->scu, rate);
		break;
	default:
		return -ENOENT;
	}

	return new_rate;
}

struct clk_ops ast2500_clk_ops = {
	.get_rate = ast2500_clk_get_rate,
	.set_rate = ast2500_clk_set_rate,
};

static int ast2500_clk_probe(struct udevice *dev)
{
	struct ast2500_clk_priv *priv = dev_get_priv(dev);
	priv->scu = (struct ast2500_scu*)dev_get_addr(dev);

	printascii("CLK probed\n\r");
	return 0;
}

static int ast2500_clk_bind(struct udevice *dev)
{
	int ret;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(gd->dm_root, "ast2500_sysreset", "reset", &dev);
	if (ret)
		debug("Warning: No AST2500 reset driver: ret=%d\n", ret);

	printascii("CLK Bound\r\n");
	return 0;
}

static const struct udevice_id ast2500_clk_ids[] = {
	{ .compatible = "aspeed,ast2500-scu" },
	{ }
};

U_BOOT_DRIVER(aspeed_ast2500_scu) = {
	.name		= "aspeed_ast2500_scu",
	.id		= UCLASS_CLK,
	.of_match	= ast2500_clk_ids,
	.priv_auto_alloc_size = sizeof(struct ast2500_clk_priv),
	.ops		= &ast2500_clk_ops,
	.bind		= ast2500_clk_bind,
	.probe		= ast2500_clk_probe,
};
