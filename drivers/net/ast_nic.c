/*
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * (C) Copyright 2010 Andes Technology
 * Macpaul Lin <macpaul@andestech.com>
 *
 * Copyright 2017 Google Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * This device is basically Faraday FTGMAC100, with some differences,
 * which do not seem to be very big, but are in very random places, like
 * some registers removed and completely different ones put in their place.
 */

#include <common.h>
#include <dm.h>
#include <clk.h>
#include <net.h>
#include <asm/io.h>
#include "ast_nic.h"

struct ast_nic_priv {
	struct ast_nic_regs *regs;
};

static int ast_nic_ofdata_to_platdata(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct eth_pdata *platdata = dev_get_platdata(dev);

	priv->regs = dev_get_addr_ptr(dev);
	platdata->iobase = (phys_addr_t)priv->regs;

	return 0;
}

static void ast_nic_reset(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);

	setbits_le32(&priv->regs->maccr, MAC_MACCR_SW_RST);
	while (readl(&priv->regs->maccr) & MAC_MACCR_SW_RST)
		;
	/*
	 * TODO(maxims@google.com) this register is marked as read-only
	 * in the data sheet. Verify if this write really needed.
	 */
	setbits_le32(&priv->regs->fear0, MAC_FEAR_NEW_MD_IFACE);
}

static int ast_nic_probe(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct eth_pdata *platdata = dev_get_platdata(dev);
	struct clk mac_clk;
	int ret;

	debug("%s()\n", __func__);

	ret = clk_get_by_index(dev, 0, &mac_clk);
	if (ret) {
		debug("%s(): get_clk_by_index failed: %d\n", __func__, ret);
		return ret;
	}

	/* The rate parameter is ignored for this clock */
	clk_set_rate(&mac_clk, 0);

	ret = clk_get_by_index(dev, 1, &mac_clk);
	if (ret) {
		debug("%s(): get_clk_by_index failed: %d\n", __func__, ret);
		return ret;
	}

	clk_set_rate(&mac_clk, 250 * 1000 * 1000);

	/* TODO(maxims@google.com): MII init */

	ast_nic_reset(dev);

	return 0;
}

static int ast_nic_start(struct udevice *dev)
{
	debug("%s()\n", __func__);

	/* TODO */
	return 0;
}

static void ast_nic_stop(struct udevice *dev)
{
	debug("%s()\n", __func__);

	/* TODO */
}

static int ast_nic_send(struct udevice *dev, void *packet, int length)
{
	debug("%s()\n", __func__);

	/* TODO */
	return -ENOSYS;
}

static int ast_nic_recv(struct udevice *dev, int flags, uchar **packetp)
{
	debug("%s()\n", __func__);

	/* TODO */
	return -ENOSYS;
}

static int ast_nic_write_hwaddr(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct eth_pdata *platdata = dev_get_platdata(dev);
	u32 madr, ladr;

	debug("%s(%pM)\n", __func__, platdata->enetaddr);

	madr = platdata->enetaddr[0] << 8 | platdata->enetaddr[1];
	ladr = platdata->enetaddr[2] << 24 | platdata->enetaddr[3] << 16
	    | platdata->enetaddr[4] << 8 | platdata->enetaddr[5];

	writel(madr, &priv->regs->mac_madr);
	writel(ladr, &priv->regs->mac_ladr);

	return 0;
}

static const struct eth_ops ast_nic_ops = {
	.start = ast_nic_start,
	.send = ast_nic_send,
	.recv = ast_nic_recv,
	.stop = ast_nic_stop,
	.write_hwaddr = ast_nic_write_hwaddr,
};

static const struct udevice_id ast_nic_ids[] = {
	{ .compatible = "aspeed,ast2500-nic" },
	{ .compatible = "aspeed,ast2400-nic" },
	{ }
};

U_BOOT_DRIVER(ast_nic) = {
	.name	= "ast_nic",
	.id	= UCLASS_ETH,
	.of_match = ast_nic_ids,
	.probe	= ast_nic_probe,
	.ops	= &ast_nic_ops,
	.ofdata_to_platdata = ast_nic_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct ast_nic_priv),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};
