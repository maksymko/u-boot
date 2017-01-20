/*
 * Copyright 2017 Google, Inc
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <misc.h>
#include <reset.h>
#include <reset-uclass.h>
#include <wdt.h>
#include <asm/arch/wdt.h>

DECLARE_GLOBAL_DATA_PTR;

struct ast2500_reset_priv {
	/* WDT used to perform resets. */
	struct udevice *wdt;
};

static int ast2500_ofdata_to_platdata(struct udevice *dev)
{
	struct ast2500_reset_priv *priv = dev_get_priv(dev);
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_WDT, dev, "aspeed,wdt", &priv->wdt);
	if (ret) {
		debug("%s: can't find WDT for reset controller", __func__);
		return ret;
	}

	return 0;
}

static int ast2500_reset_assert(struct reset_ctl *reset_ctl)
{
	struct ast2500_reset_priv *priv = dev_get_priv(reset_ctl->dev);

	if (IS_ERR(priv))
		return PTR_ERR(priv);

	return wdt_reset(priv->wdt, reset_ctl->id);
}

static int ast2500_reset_request(struct reset_ctl *reset_ctl)
{
	debug("%s(reset_ctl=%p) (dev=%p, id=%lu)\n", __func__, reset_ctl,
	      reset_ctl->dev, reset_ctl->id);

	return 0;
}

static int ast2500_reset_probe(struct udevice *dev)
{
	debug("%s()\n", __func__);

	return 0;
}

static const struct udevice_id ast2500_reset_ids[] = {
    { .compatible = "aspeed,ast2500-reset" },
    { }
};

struct reset_ops ast2500_reset_ops = {
	.rst_assert = ast2500_reset_assert,
	.request = ast2500_reset_request,
};

U_BOOT_DRIVER(ast2500_reset) = {
	.name		= "ast2500_reset",
	.id		= UCLASS_RESET,
	.probe		= ast2500_reset_probe,
	.of_match = ast2500_reset_ids,
	.ops = &ast2500_reset_ops,
	.ofdata_to_platdata = ast2500_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct ast2500_reset_priv),
};
