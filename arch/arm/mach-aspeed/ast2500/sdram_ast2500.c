/*
 * Copyright 2016 Google, Inc
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dt-bindings/clock/ast2500-scu.h>
#include <errno.h>
#include <ram.h>
#include <asm/io.h>
#include <asm/arch/scu_ast2500.h>
#include <asm/arch/sdram_ast2500.h>
#include <linux/err.h>

#include <debug_uart.h>

#define CONFIG_AST_DDR_FREQ     (400*1000*1000)

struct dram_info {
  struct ram_info info;
  struct clk ddr_clk;
  struct ast2500_sdrammc_regs *regs;
  struct ast2500_scu *scu;
};

static int ast2500_sdrammc_probe(struct udevice *dev)
{
  struct dram_info *priv = (struct dram_info *)dev_get_priv(dev);
  struct ast2500_sdrammc_regs *regs = (struct ast2500_sdrammc_regs *)dev_get_addr(dev);

  int ret = clk_get_by_index(dev, 0, &priv->ddr_clk);
  if (ret) {
    printascii("D:No CLK\r\n");
    return ret;
  }

  printascii("D: Clock ID: ");
  printhex4(priv->ddr_clk.id);
  printascii("\r\n");

  priv->scu = ast_get_scu();
  if (IS_ERR(priv->scu))
    return PTR_ERR(priv->scu);

  ulong new_rate = clk_set_rate(&priv->ddr_clk, CONFIG_AST_DDR_FREQ);

  priv->info.base = 0x80000000;

  printascii("New Rate: ");
  printhex8(new_rate);
  printascii("\r\n");

  printascii("MPLL_PARAM: ");
  printhex8(readl(&priv->scu->m_pll_param));
  printascii("\r\n");

  (void)regs;
  (void)priv;
  return 0;
}

static int ast2500_sdrammc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_info *priv = dev_get_priv(dev);

  printascii("DRAM INFO\r\n");
	*info = priv->info;

	return 0;
}

static struct ram_ops ast2500_sdrammc_ops = {
  .get_info = ast2500_sdrammc_get_info,
};

static const struct udevice_id ast2500_sdrammc_ids[] = {
  { .compatible = "aspeed,ast2500-sdrammc" },
  { }
};

U_BOOT_DRIVER(sdrammc_ast2500) = {
  .name = "aspeed_ast2500_sdrammc",
  .id = UCLASS_RAM,
  .of_match = ast2500_sdrammc_ids,
  .ops = &ast2500_sdrammc_ops,
  .probe = ast2500_sdrammc_probe,
  .priv_auto_alloc_size = sizeof(struct dram_info),
};
