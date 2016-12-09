/*
 * Copyright 2016 Google, Inc
 *
 * SPDX-License-Identifier:		GPL-2.0
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
#include <asm/arch/wdt.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <regmap.h>

#include <debug_uart.h>

/* TODO: Move to Kconfig */
#define CONFIG_AST_DDR1600

#define DDR4_MR46_MODE		0x08000000
#define DDR4_MR5_MODE		0x400
#define DDR4_MR13_MODE		0x101
#define PHY_CFG_SIZE		15

#if defined(CONFIG_AST_DDR1600)

#define AST_DDR_FREQ		(400*1000*1000)
#define DDR4_MR02_MODE		0x410
#define DDR4_TRFC		0x88714545
static const u32 ddr4_ac_timing[3] = {0x63604e37, 0xe97afa99, 0x00019000};
static const struct {
	u32 index[PHY_CFG_SIZE];
	u32 value[PHY_CFG_SIZE];
} ddr4_phy_config = {
	.index = {0, 1, 3, 4, 5, 56, 57, 58, 59, 60, 61, 62, 36, 49, 50},
	.value = {
		0x42492aae, 0x09002000, 0x55e00b0b, 0x20000000, 0x24, 0x03002900,
		0x0e0000a0, 0x000e001c, 0x35b8c106, 0x08080607, 0x9b000900, 0x0e400a00,
		0x00100008, 0x3c183c3c, 0x00631e0e,
	},
};

#elif defined(CONFIG_AST_DDR1333)

#define AST_DDR_FREQ		(331*1000*1000)
#define DDR4_MR02_MODE		0x200
#define DDR4_TRFC		0x7f5e3a3a
static const u32 ddr4_ac_timing[3] = {0x53503d26, 0xe878d87f, 0x00019000};
static const struct {
	u32 index[PHY_CFG_SIZE];
	u32 value[PHY_CFG_SIZE];
} ddr4_phy_config = {
	.index = {0, 1, 3, 4, 5, 56, 57, 58, 59, 60, 61, 62, 36, 49, 50},
	.value = {
		0x42492aae, 0x09002000, 0x55e00b0b, 0x20000000, 0x20, 0x03002200,
		0x0c000085, 0x000ba01a, 0x2cb92106, 0x07060606, 0x81000700, 0x0c400800,
		0x00100008, 0x3c183c3c, 0x00631e0e,
	},
};

#else
#error "DDR rate is not defined"
#endif

#define SDRAM_MAX_SIZE		(1024 * 1024 * 1024)

/*
 * Bandwidth configuration parameters for different SDRAM requests.
 * These are hardcoded settings taken from Aspeed SDK.
 */
static const u32 ddr_max_grant_params[4] = {0x88448844, 0x24422288, 0x22222222, 0x22222222};

#define VREF_PHY_RETRY_COUNT		5

/*
 * DDR PHY registers are not documented by Aspeed at all.
 * All writes and reads are taken pretty much as is from SDK.
 */
struct ast2500_ddr_phy {
	u32 phy[117];
};

struct dram_info {
  struct ram_info info;
  struct clk ddr_clk;
  struct ast2500_sdrammc_regs *regs;
  struct ast2500_scu *scu;
  struct ast2500_ddr_phy *phy;
};

static int ast2500_sdrammc_reset(void)
{
  /* Reset SDRAM using WDT */
  struct ast_wdt *wdt = ast_get_wdt(0);
  if (IS_ERR(wdt))
	return PTR_ERR(wdt);

  /* Only Reset SDRAM */
  writel(WDT_RESET_SDRAM, &wdt->reset_mask);
  clrbits_le32(&wdt->ctrl, WDT_CTRL_RESET_MASK << WDT_CTRL_RESET_MODE_OFFSET);
  wdt_start(wdt, 1);

  /* Wait for WDT to reset */
  while (readl(&wdt->ctrl) & WDT_CTRL_EN);
  wdt_stop(wdt);

  return 0;
}

static int ast2500_sdrammc_init_phy(struct ast2500_ddr_phy *phy)
{
	writel(0, &phy->phy[2]);
	writel(0, &phy->phy[6]);
	writel(0, &phy->phy[8]);
	writel(0, &phy->phy[10]);
	writel(0, &phy->phy[12]);
	writel(0, &phy->phy[42]);
	writel(0, &phy->phy[44]);

	writel(0x86000000, &phy->phy[16]);
	writel(0x00008600, &phy->phy[17]);
	writel(0x80000000, &phy->phy[18]);
	writel(0x80808080, &phy->phy[19]);

	return 0;
}

static void ast2500_ddr_phy_init_process(struct dram_info *info)
{
	struct ast2500_sdrammc_regs *regs = info->regs;
	writel(0, &regs->phy_ctrl[0]);
	writel(0x4040, &info->phy->phy[51]);

	writel(SDRAM_PHYCTRL0_NRST | SDRAM_PHYCTRL0_INIT, &regs->phy_ctrl[0]);
	while ((readl(&regs->phy_ctrl[0]) & SDRAM_PHYCTRL0_INIT)) ;
	writel(SDRAM_PHYCTRL0_NRST | SDRAM_PHYCTRL0_AUTO_UPDATE,
	       &regs->phy_ctrl[0]);
}

static void ast2500_sdrammc_set_vref(struct dram_info *info, u32 vref)
{
	writel(0, &info->regs->phy_ctrl[0]);
    writel((vref << 8) | 0x6, &info->phy->phy[48]);
    ast2500_ddr_phy_init_process(info);
}

static int ast2500_ddr_cbr_test(struct dram_info *info)
{
	struct ast2500_sdrammc_regs *regs = info->regs;

	writel((1 << SDRAM_REFRESH_CYCLES_SHIFT) |
	       (0x5c << SDRAM_REFRESH_PERIOD_SHIFT), &regs->refresh_timing);
	writel((0xfff << SDRAM_TEST_LEN_SHIFT), &regs->test_addr);
	writel(0xff00ff00, &regs->test_init_val);
	writel(SDRAM_TEST_EN | (SDRAM_TEST_MODE_RW << SDRAM_TEST_MODE_SHIFT) |
	       SDRAM_TEST_ERRSTOP, &regs->ecc_test_ctrl);

	while (!(readl(&regs->ecc_test_ctrl) & SDRAM_TEST_DONE)) ;

	int ret = 0;
	if (readl(&regs->ecc_test_ctrl) & SDRAM_TEST_FAIL) {
		ret = -EIO;
	} else {

		/* cbr_test_burst */
		int i;
		const u32 test_params = SDRAM_TEST_EN
				| SDRAM_TEST_ERRSTOP
				| SDRAM_TEST_TWO_MODES;
		for (i = 0; i <= SDRAM_TEST_GEN_MODE_MASK; ++i) {
			writel((i << SDRAM_TEST_GEN_MODE_SHIFT) | test_params,
			       &regs->ecc_test_ctrl);
			while (!(readl(&regs->ecc_test_ctrl)
					 & SDRAM_TEST_DONE)) ;
			if (readl(&regs->ecc_test_ctrl) & SDRAM_TEST_FAIL) {
				ret = -EIO;
				break;
			}
		}
	}

	writel(0, &regs->refresh_timing);
	writel(0, &regs->ecc_test_ctrl);
	return ret;
}

static int ast2500_sdrammc_ddr4_calibrate_vref(struct dram_info *info)
{
	int i;
	int vref_min = 0xff;
	int vref_max = 0;
	int range_size = 0;

	printch('T');
	for (i = 1; i < 0x40; ++i) {
		ast2500_sdrammc_set_vref(info, i);

		int res = ast2500_ddr_cbr_test(info);
		if (res < 0) {
			printch('0');
			if (range_size > 0)
				break;
		} else {
			++range_size;
			vref_min = min(vref_min, i);
			vref_max = max(vref_max, i);
			printch('1');
		}
	}

	printascii("\r\n");

	/* Pick average setting */
	ast2500_sdrammc_set_vref(info, (vref_min + vref_max + 1) / 2);

	return 0;
}

static int ast2500_sdrammc_calc_size(struct dram_info *info)
{
	/* The controller supports 128/256/512/1024 MB ram */

	/* Disable oversized write protection */
	setbits_le32(&info->regs->config, (SDRAM_CONF_CAP_MASK << SDRAM_CONF_CAP_SHIFT));

	/* Initially set the size to minimum supported */
	ulong ram_size = 128 * 1024 * 1024;
	const int write_test_offset = 1024 * 1024;
	u32 test_pattern = 0xdeadbeef;

	u32 cap_param = 0;
	for (; ram_size < SDRAM_MAX_SIZE; ram_size <<= 1, ++cap_param) {
		/* Check if the size can be extended */
		u32 write_addr =
		    CONFIG_SYS_SDRAM_BASE + ram_size + write_test_offset;
		writel(test_pattern, write_addr);
		if (readl(write_addr) != test_pattern)
			break;

		/* The pattern needs to be changed to avoid reading correct value because
		 * of wrapped read rather then correct write.
		 */

		test_pattern = (test_pattern >> 4) | (test_pattern << 28);
	}

	printascii("Calc RAM Size: ");
	printhex8(ram_size);
	printascii("\r\n");

	info->info.base = CONFIG_SYS_SDRAM_BASE;
	info->info.size = ram_size;
	clrsetbits_le32(&info->regs->config,
			(SDRAM_CONF_CAP_MASK << SDRAM_CONF_CAP_SHIFT),
			(cap_param << SDRAM_CONF_CAP_SHIFT));

	return 0;
}

static int ast2500_sdrammc_calibration_end(struct dram_info *info)
{
	struct ast2500_sdrammc_regs *regs = info->regs;
	u32 trfc = DDR4_TRFC & SDRAM_AC_TRFC_MASK;

	clrsetbits_le32(&regs->ac_timing[1],
					(SDRAM_AC_TRFC_MASK << SDRAM_AC_TRFC_SHIFT),
					(trfc << SDRAM_AC_TRFC_SHIFT));

	setbits_le32(&regs->config, SDRAM_CONF_CACHE_INIT_EN);
	while (!(readl(&regs->config) & SDRAM_CONF_CACHE_INIT_DONE)) ;
	setbits_le32(&regs->config, SDRAM_CONF_CACHE_EN);

	writel(SDRAM_MISC_DDR4_TREFRESH, &regs->misc_control);

	/* Enable all requests except video & display */
	writel(SDRAM_REQ_USB20_EHCI1
		   | SDRAM_REQ_USB20_EHCI2
		   | SDRAM_REQ_CPU
		   | SDRAM_REQ_AHB2
		   | SDRAM_REQ_AHB
		   | SDRAM_REQ_MAC0
		   | SDRAM_REQ_MAC1
		   | SDRAM_REQ_PCIE
		   | SDRAM_REQ_XDMA
		   | SDRAM_REQ_ENCRYPTION
		   | SDRAM_REQ_VIDEO_FLAG
		   | SDRAM_REQ_VIDEO_LOW_PRI_WRITE
		   | SDRAM_REQ_2D_RW
		   | SDRAM_REQ_MEMCHECK, &regs->req_limit_mask);

	return 0;
}

static int ast2500_sdrammc_init_ddr4(struct dram_info *info)
{
	printascii("DDR4 INIT\r\n");

	int i;
	u32 power_control;
	u32 conf = (SDRAM_CONF_CAP_1024M << SDRAM_CONF_CAP_SHIFT)
#ifdef CONFIG_DUALX8_RAM
			| SDRAM_CONF_DUALX8
#endif
			| SDRAM_CONF_SCRAMBLE
			| SDRAM_CONF_SCRAMBLE_PAT2
			| SDRAM_CONF_DDR4;

	writel(conf, &info->regs->config);
	for (i = 0; i < ARRAY_SIZE(ddr4_ac_timing); ++i) {
		writel(ddr4_ac_timing[i], &info->regs->ac_timing[i]);
	}

	writel(DDR4_MR46_MODE, &info->regs->mr46_mode_setting);
	writel(DDR4_MR5_MODE, &info->regs->mr5_mode_setting);
	writel(DDR4_MR02_MODE, &info->regs->mr02_mode_setting);
	writel(DDR4_MR13_MODE, &info->regs->mr13_mode_setting);

	for (i = 0; i < PHY_CFG_SIZE; ++i) {
		writel(ddr4_phy_config.value[i], &info->phy->phy[ddr4_phy_config.index[i]]);
	}

	power_control = SDRAM_PCR_CKE_EN
			| (1 << SDRAM_PCR_CKE_DELAY_SHIFT)
			| (2 << SDRAM_PCR_TCKE_PW_SHIFT)
			| SDRAM_PCR_RESETN_DIS
			| SDRAM_PCR_RGAP_CTRL_EN
			| SDRAM_PCR_ODT_EN
			| SDRAM_PCR_ODT_EXT_EN;
	writel(power_control, &info->regs->power_control);

	ast2500_ddr_phy_init_process(info);

	int ret = ast2500_sdrammc_ddr4_calibrate_vref(info);
	if (ret < 0) {
		printascii("CBR FAIL!\r\n");
		return ret;
	}

#ifdef CONFIG_DRAM_EXT_TEMP
	writel(0x42aa2f81, &info->regs->refresh_timing);
#else
	writel(0x42aa5c81, &info->regs->refresh_timing);
#endif

	writel(0x1af93, &info->regs->power_control);

	ret = ast2500_sdrammc_calibration_end(info);
	if (ret < 0)
		return ret;

	return ast2500_sdrammc_calc_size(info);
}

static void ast2500_sdrammc_unlock(struct dram_info * info)
{
	writel(SDRAM_UNLOCK_KEY, &info->regs->protection_key);
	while (!readl(&info->regs->protection_key));
}

static void ast2500_sdrammc_lock(struct dram_info * info)
{
	writel(~SDRAM_UNLOCK_KEY, &info->regs->protection_key);
	while (readl(&info->regs->protection_key));
}

static int ast2500_sdrammc_probe(struct udevice *dev)
{
	struct dram_info *priv = (struct dram_info *)dev_get_priv(dev);
	struct ast2500_sdrammc_regs *regs = priv->regs;
	int ret = clk_get_by_index(dev, 0, &priv->ddr_clk);
	int i;

	if (ret) {
		printascii("D:No CLK\r\n");
		return ret;
	}

	priv->scu = ast_get_scu();
	if (IS_ERR(priv->scu))
		return PTR_ERR(priv->scu);

	ulong new_rate = clk_set_rate(&priv->ddr_clk, AST_DDR_FREQ);
	ret = ast2500_sdrammc_reset();
	if (ret)
		return ret;

	ast2500_sdrammc_unlock(priv);

	writel(SDRAM_PCR_MREQI_DIS | SDRAM_PCR_RESETN_DIS, &regs->power_control);
	writel(SDRAM_VIDEO_UNLOCK_KEY, &regs->gm_protection_key);

	/* Mask all requests except CPU and AHB during PHY init */
	writel(~(SDRAM_REQ_CPU | SDRAM_REQ_AHB), &regs->req_limit_mask);

	for (i = 0; i < ARRAY_SIZE(ddr_max_grant_params); ++i) {
		writel(ddr_max_grant_params[i], &regs->max_grant_len[i]);
	}

	setbits_le32(&regs->intr_ctrl, SDRAM_ICR_RESET_ALL);

	ast2500_sdrammc_init_phy(priv->phy);
	if (readl(&priv->scu->hwstrap) & SCU_HWSTRAP_DDR4) {
		ast2500_sdrammc_init_ddr4(priv);
	} else {
		printascii("Unsupported DRAM3\r\n");
		return -EINVAL;
	}

	clrbits_le32(&regs->intr_ctrl, SDRAM_ICR_RESET_ALL);
	ast2500_sdrammc_lock(priv);

	return 0;
}

static int ast2500_sdrammc_ofdata_to_platdata(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);
	struct regmap *map;
	int ret = regmap_init_mem(dev, &map);

	if (ret)
		return ret;

	priv->regs = regmap_get_range(map, 0);
	priv->phy = regmap_get_range(map, 1);

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
  .ofdata_to_platdata = ast2500_sdrammc_ofdata_to_platdata,
  .probe = ast2500_sdrammc_probe,
  .priv_auto_alloc_size = sizeof(struct dram_info),
};
