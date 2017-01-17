/*
 * Copyright 2017 Google, Inc
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/pinctrl.h>
#include <asm/arch/scu_ast2500.h>
#include <dm/pinctrl.h>

DECLARE_GLOBAL_DATA_PTR;

struct ast2500_pinctrl_priv {
	struct ast2500_scu *scu;
};

static int ast2500_pinctrl_probe(struct udevice *dev)
{
	struct ast2500_pinctrl_priv *priv = (struct ast2500_pinctrl_priv*)dev_get_priv(dev);

	priv->scu = ast_get_scu();

	return 0;
}

static void pinctrl_ast2500_i2c_config(struct ast2500_scu *scu, int func,
				       int flags)
{
	(void)flags;

	if (func >= PERIPH_ID_I2C3) {
		setbits_le32(&scu->pinmux_ctrl[4],
			     SCU_PINMUX_CTRL5_I2C << (func - PERIPH_ID_I2C3));
	}
	/* Previous versions of the chip have these pins hard wired for I2C */
#ifdef CONFIG_ASPEED_AST2500
	if (func == PERIPH_ID_I2C1) {
		setbits_le32(&scu->pinmux_ctrl1[1], SCU_PIN_FUN_SDA1 | SCU_PIN_FUN_SCL1);
	} else if (func == PERIPH_ID_I2C2) {
		setbits_le32(&scu->pinmux_ctrl1[1], SCU_PIN_FUN_SDA2 | SCU_PIN_FUN_SCL2);
	}
#endif
}

static void pinctrl_ast2500_mac_config(struct ast2500_scu *scu, int func,
				       int flags)
{
	switch (func) {
	case PERIPH_ID_MAC1:
#ifdef CONFIG_ASPEED_AST2500
		setbits_le32(&scu->pinmux_ctrl[0], SCU_PIN_FUN_MAC1_PHY_LINK);
#else
		if (readl(&scu->hwstrap) & SCU_HWSTRAP_MAC1_RGMII)
			setbits_le32(&scu->pinmux_ctrl[0],
				     SCU_PIN_FUN_MAC1_PHY_LINK);
		else
			clrbits_le32(&scu->pinmux_ctrl[0],
				     SCU_PIN_FUN_MAC1_PHY_LINK);
#endif
		setbits_le32(&scu->pinmux_ctrl[2],
			     SCU_PIN_FUN_MAC1_MDIO | SCU_PIN_FUN_MAC1_MDC);
		break;
	case PERIPH_ID_MAC2:
		setbits_le32(&scu->pinmux_ctrl[0], SCU_PIN_FUN_MAC2_PHY_LINK);
		setbits_le32(&scu->pinmux_ctrl[4], SCU_PIN_FUN_MAC2_MDIO);
		break;
	}
}

static int ast2500_pinctrl_set_state_full(struct udevice *dev, struct udevice *config)
{
	struct ast2500_pinctrl_priv *priv = (struct ast2500_pinctrl_priv*)dev_get_priv(dev);

	return -1;
}

static int ast2500_pinctrl_request(struct udevice *dev, int func, int flags)
{
	struct ast2500_pinctrl_priv *priv = (struct ast2500_pinctrl_priv*)dev_get_priv(dev);

	debug("%s: func=%x, flags=%x\n", __func__, func, flags);
	switch (func) {
		case PERIPH_ID_I2C1:
		case PERIPH_ID_I2C2:
		case PERIPH_ID_I2C3:
		case PERIPH_ID_I2C4:
		case PERIPH_ID_I2C5:
		case PERIPH_ID_I2C6:
		case PERIPH_ID_I2C7:
		case PERIPH_ID_I2C8:
		case PERIPH_ID_I2C9:
		case PERIPH_ID_I2C10:
		case PERIPH_ID_I2C11:
		case PERIPH_ID_I2C12:
		case PERIPH_ID_I2C13:
		case PERIPH_ID_I2C14:
			pinctrl_ast2500_i2c_config(priv->scu, func, flags);
			break;
		case PERIPH_ID_MAC1:
		case PERIPH_ID_MAC2:
			pinctrl_ast2500_mac_config(priv->scu, func, flags);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int ast2500_pinctrl_get_periph_id(struct udevice *dev,
					 struct udevice *periph)
{
	u32 cell;
	int ret;

	ret = fdtdec_get_int_array(gd->fdt_blob, periph->of_offset, "interrupts",
				 &cell, 1);

	if (ret < 0)
		return -EINVAL;

	switch (cell) {
		case 2:
			return PERIPH_ID_MAC1;
		case 3:
			return PERIPH_ID_MAC2;
	}

	if (cell >= 12 && cell <= 25)
		return PERIPH_ID_I2C1 + cell - 12;
}

static int ast2500_pinctrl_set_state_simple(struct udevice *dev, struct udevice *periph)
{
	int func;

	func = ast2500_pinctrl_get_periph_id(dev, periph);
	if (func < 0)
		return func;

	return ast2500_pinctrl_request(dev, func, 0);
}

static int ast2500_pinctrl_get_groups_count(struct udevice *dev)
{
	debug("PINCTRL: get_groups_count\n");

	return 5;
}

static const char* ast2500_groups[] = {"eth_phy0", "eth_phy1", "eth_mdio0", "eth_mdc0", "eth_mdc_mdio1"};

static const char* ast2500_pinctrl_get_group_name(struct udevice *dev, unsigned selector)
{
	debug("PINCTRL: get_group_name %u\n", selector);
	return ast2500_groups[selector];
}

static int ast2500_pinctrl_group_set(struct udevice *dev, unsigned selector, unsigned func_selector)
{
	debug("PINCTRL: group_set <%u, %u>\n", selector, func_selector);

	return 0;
}

static struct pinctrl_ops ast2500_pinctrl_ops = {
	.set_state = pinctrl_generic_set_state,
	.get_groups_count = ast2500_pinctrl_get_groups_count,
	.get_group_name = ast2500_pinctrl_get_group_name,
	.pinmux_group_set = ast2500_pinctrl_group_set,
};

static const struct udevice_id ast2500_pinctrl_ids[] = {
	{ .compatible = "aspeed,ast2500-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_ast2500) = {
	.name = "aspeed_ast2500_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = ast2500_pinctrl_ids,
	.priv_auto_alloc_size = sizeof(struct ast2500_pinctrl_priv),
	.ops = &ast2500_pinctrl_ops,
	.probe = ast2500_pinctrl_probe,
};
