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
	struct ast2500_pinctrl_priv *priv = dev_get_priv(dev);

	priv->scu = ast_get_scu();

	return 0;
}

struct ast2500_group_config {
	char* group_name;
	/* Control register number (1-10) */
	unsigned reg_num;
	/* The shift of the control bit in the register */
	u32 ctrl_bit_shift;
};

static const struct ast2500_group_config ast2500_groups[] = {
	{ .group_name = "I2C1_SDA", .reg_num = 8, .ctrl_bit_shift = 13 },
	{ .group_name = "I2C1_SCL", .reg_num = 8, .ctrl_bit_shift = 12 },
	{ .group_name = "I2C2_SDA", .reg_num = 8, .ctrl_bit_shift = 15 },
	{ .group_name = "I2C2_SCL", .reg_num = 8, .ctrl_bit_shift = 14 },
	{ .group_name = "I2C3", .reg_num = 5, .ctrl_bit_shift = 16 },
	{ .group_name = "I2C4", .reg_num = 5, .ctrl_bit_shift = 17 },
	{ .group_name = "I2C5", .reg_num = 5, .ctrl_bit_shift = 18 },
	{ .group_name = "I2C6", .reg_num = 5, .ctrl_bit_shift = 19 },
	{ .group_name = "I2C7", .reg_num = 5, .ctrl_bit_shift = 20 },
	{ .group_name = "I2C8", .reg_num = 5, .ctrl_bit_shift = 21 },
	{ .group_name = "I2C9", .reg_num = 5, .ctrl_bit_shift = 22 },
	{ .group_name = "I2C10", .reg_num = 5, .ctrl_bit_shift = 23 },
	{ .group_name = "I2C11", .reg_num = 5, .ctrl_bit_shift = 24 },
	{ .group_name = "I2C12", .reg_num = 5, .ctrl_bit_shift = 25 },
	{ .group_name = "I2C13", .reg_num = 5, .ctrl_bit_shift = 26 },
	{ .group_name = "I2C14", .reg_num = 5, .ctrl_bit_shift = 27 },
};

static const char *ast2500_functions[] = {
	"I2C1",
	"I2C2",
	"I2C3",
	"I2C4",
	"I2C5",
	"I2C6",
	"I2C7",
	"I2C8",
	"I2C9",
	"I2C10",
	"I2C11",
	"I2C12",
	"I2C13",
	"I2C14",
};

static int ast2500_pinctrl_get_functions_count(struct udevice *dev)
{
	debug("PINCTRL: get_functions_count\n");

	return ARRAY_SIZE(ast2500_functions);
}

static const char* ast2500_pinctrl_get_function_name(struct udevice *dev, unsigned selector)
{
	debug("PINCTRL: get_function_name %u\n", selector);

	return ast2500_functions[selector];
}

static int ast2500_pinctrl_get_groups_count(struct udevice *dev)
{
	debug("PINCTRL: get_groups_count\n");

	return ARRAY_SIZE(ast2500_groups);
}

static const char* ast2500_pinctrl_get_group_name(struct udevice *dev, unsigned selector)
{
	debug("PINCTRL: get_group_name %u\n", selector);

	return ast2500_groups[selector].group_name;
}

static int ast2500_pinctrl_group_set(struct udevice *dev, unsigned selector,
				     unsigned func_selector)
{
	struct ast2500_pinctrl_priv *priv = dev_get_priv(dev);
	const struct ast2500_group_config *config;
	u32 *ctrl_reg;

	debug("PINCTRL: group_set <%u, %u>\n", selector, func_selector);
	if (selector >= ARRAY_SIZE(ast2500_groups))
		return -EINVAL;

	config = &ast2500_groups[selector];
	if (config->reg_num > 6)
		ctrl_reg = &priv->scu->pinmux_ctrl1[config->reg_num - 7];
	else
		ctrl_reg = &priv->scu->pinmux_ctrl[config->reg_num - 1];

	ast_scu_unlock(priv->scu);
	setbits_le32(ctrl_reg, (1 << config->ctrl_bit_shift));
	ast_scu_lock(priv->scu);

	return 0;
}

static struct pinctrl_ops ast2500_pinctrl_ops = {
	.set_state = pinctrl_generic_set_state,
	.get_groups_count = ast2500_pinctrl_get_groups_count,
	.get_group_name = ast2500_pinctrl_get_group_name,
	.get_functions_count = ast2500_pinctrl_get_functions_count,
	.get_function_name = ast2500_pinctrl_get_function_name,
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
