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
#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
#include <miiphy.h>
#endif
#include <net.h>
#include <asm/io.h>
#include <linux/mii.h>
#include "ast_nic.h"

#define ETH_ZLEN	60
#define RBSR_DEFAULT_VALUE	0x640

#define PKTBUFSTX	4

#define MAX_PHY_ADDR 32

struct ast_nic_xdes {
	u32 des[4];
} __aligned(16);

struct ast_nic_xdes ast_txdes[PKTBUFSTX];
struct ast_nic_xdes ast_rxdes[PKTBUFSRX];

struct ast_nic_priv {
	struct ast_nic_xdes *txdes;
	struct ast_nic_xdes *rxdes;
	struct ast_nic_regs *regs;
	int tx_index;
	int rx_index;
	int phy_addr;
};

static int ast_nic_ofdata_to_platdata(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct eth_pdata *platdata = dev_get_platdata(dev);

	priv->regs = dev_get_addr_ptr(dev);
	priv->txdes = ast_txdes;
	priv->rxdes = ast_rxdes;
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
	 * Only needed for ast2400, for ast2500 this is the no-op,
	 * because the register is marked read-only.
	 */
	setbits_le32(&priv->regs->fear0, MAC_FEAR_NEW_MD_IFACE);
}

static int ast_nic_phy_read(struct udevice *dev, int phy_addr,
			    int regnum, u16 *value)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	int phycr;
	int i;

	phycr = MAC_PHYCR_FIRE | MAC_PHYCR_ST_22 | MAC_PHYCR_READ |
	    (phy_addr << MAC_PHYCR_PHYAD_SHIFT) |
	    (regnum << MAC_PHYCR_REGAD_SHIFT);

	writel(phycr, &priv->regs->phycr);

	for (i = 0; i < 10; i++) {
		phycr = readl(&priv->regs->phycr);

		if ((phycr & MAC_PHYCR_FIRE) == 0) {
			int data;

			data = readl(&priv->regs->phydata);
			*value = (data & MAC_PHYDATA_MIIRDATA_MASK) >>
			    MAC_PHYDATA_MIIRDATA_SHIFT;

			return 0;
		}

		mdelay(10);
	}

	debug("mdio read timed out\n");
	return -ETIMEDOUT;
}

static int ast_nic_phy_write(struct udevice *dev, int phy_addr,
	int regnum, u16 value)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	int phycr;
	int i;

	phycr = (value << MAC_PHYDATA_MIIWDATA_SHIFT) |
			MAC_PHYCR_FIRE | MAC_PHYCR_ST_22 |
			MAC_PHYCR_WRITE |
			(phy_addr << MAC_PHYCR_PHYAD_SHIFT) |
			(regnum << MAC_PHYCR_REGAD_SHIFT);

	writel(phycr, &priv->regs->phycr);

	for (i = 0; i < 10; i++) {
		phycr = readl(&priv->regs->phycr);

		if ((phycr & MAC_PHYCR_FIRE) == 0) {
			debug("(phycr & MAC_PHYCR_MIIWR) == 0: phy_addr: %x\n",
			      phy_addr);
			return 0;
		}

		mdelay(10);
	}

	debug("mdio write timed out\n");

	return -ETIMEDOUT;
}

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
static int ast_nic_reg_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct udevice *dev = eth_get_dev_by_name(bus->name);
	u16 value;

	ast_nic_phy_read(dev, addr, reg, &value);

	return value;
}

static int ast_nic_reg_write(struct mii_dev *bus, int addr, int devad, int reg,
			     u16 value)
{
	struct udevice *dev = eth_get_dev_by_name(bus->name);

	return ast_nic_phy_write(dev, addr, reg, value);
}
#endif

static int ast_nic_probe(struct udevice *dev)
{
	struct clk mac_clk;
	struct clk d2pll_clk;
	int ret;

	debug("%s()\n", __func__);

	ret = clk_get_by_index(dev, 0, &mac_clk);
	if (ret) {
		debug("%s(): get_clk_by_index failed: %d\n", __func__, ret);
		return ret;
	}

	clk_enable(&mac_clk);

	ret = clk_get_by_index(dev, 1, &d2pll_clk);
	if (ret) {
		debug("%s(): get_clk_by_index failed: %d\n", __func__, ret);
		return ret;
	}

	clk_enable(&d2pll_clk);

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	struct mii_dev *mdiodev = mdio_alloc();
	if (!mdiodev)
		return -ENOMEM;
	strncpy(mdiodev->name, dev->name, MDIO_NAME_LEN);
	mdiodev->read = ast_nic_reg_read;
	mdiodev->write = ast_nic_reg_write;

	ret = mdio_register(mdiodev);
	if (ret < 0)
		return ret;
#endif

	ast_nic_reset(dev);

	return 0;
}

static int ast_nic_phy_reset(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	int i;
	u16 status, adv;

	adv = ADVERTISE_CSMA | ADVERTISE_ALL;

	ast_nic_phy_write(dev, priv->phy_addr, MII_ADVERTISE, adv);

	printf("%s: Starting autonegotiation...\n", dev->name);

	ast_nic_phy_write(dev, priv->phy_addr,
			  MII_BMCR, (BMCR_ANENABLE | BMCR_ANRESTART));

	for (i = 0; i < 1000; i++) {
		ast_nic_phy_read(dev, priv->phy_addr, MII_BMSR, &status);

		if (status & BMSR_ANEGCOMPLETE)
			break;
		mdelay(1);
	}

	if (status & BMSR_ANEGCOMPLETE) {
		printf("%s: Autonegotiation complete\n", dev->name);
	} else {
		printf("%s: Autonegotiation timed out (status=0x%04x)\n",
		       dev->name, status);
		return -ETIMEDOUT;
	}

	return 0;
}

static int ast_nic_phy_init(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	int phy_addr;
	u16 phy_id, status, adv, lpa, stat_ge;
	int media, speed, duplex;
	int i;

	/* Check if the PHY is up to snuff... */
	for (phy_addr = 0; phy_addr < MAX_PHY_ADDR; phy_addr++) {
		ast_nic_phy_read(dev, phy_addr, MII_PHYSID1, &phy_id);

		/*
		 * When it is unable to found PHY,
		 * the interface usually return 0xffff or 0x0000
		 */
		if (phy_id != 0xffff && phy_id != 0x0) {
			debug("%s: found PHY at 0x%02x\n", dev->name, phy_addr);
			priv->phy_addr = phy_addr;
			break;
		}
	}

	if (phy_id == 0xffff || phy_id == 0x0) {
		debug("%s: no PHY present\n", dev->name);
		return -ENODEV;
	}

	ast_nic_phy_read(dev, priv->phy_addr, MII_BMSR, &status);

	if (!(status & BMSR_LSTATUS)) {
		/* Try to re-negotiate if we don't have link already. */
		ast_nic_phy_reset(dev);

		for (i = 0; i < 100000 / 100; i++) {
			ast_nic_phy_read(dev, priv->phy_addr,
					 MII_BMSR, &status);
			if (status & BMSR_LSTATUS)
				break;
			udelay(100);
		}
	}

	if (!(status & BMSR_LSTATUS)) {
		printf("%s: link down\n", dev->name);
		return -ENOLINK;
	}

	/* 1000 Base-T Status Register */
	ast_nic_phy_read(dev, priv->phy_addr, MII_STAT1000, &stat_ge);

	speed = (stat_ge & (LPA_1000FULL | LPA_1000HALF)
		 ? 1 : 0);

	duplex = ((stat_ge & LPA_1000FULL)
		  ? 1 : 0);

	if (speed) {		/* Speed is 1000 */
		debug("%s: link up, 1000bps %s-duplex\n",
		      dev->name, duplex ? "full" : "half");
		return 0;
	}

	ast_nic_phy_read(dev, priv->phy_addr, MII_ADVERTISE, &adv);
	ast_nic_phy_read(dev, priv->phy_addr, MII_LPA, &lpa);

	media = mii_nway_result(lpa & adv);
	speed = (media & (ADVERTISE_100FULL | ADVERTISE_100HALF) ? 1 : 0);
	duplex = (media & ADVERTISE_FULL) ? 1 : 0;

	debug("%s: link up, %sMbps %s-duplex\n",
	      dev->name, speed ? "100" : "10", duplex ? "full" : "half");

	return 0;
}

static int ast_nic_update_link_speed(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	u16 stat_fe;
	u16 stat_ge;
	u32 maccr;

	/* 1000 Base-T Status Register */
	ast_nic_phy_read(dev, priv->phy_addr, MII_STAT1000, &stat_ge);
	ast_nic_phy_read(dev, priv->phy_addr, MII_BMSR, &stat_fe);

	if (!(stat_fe & BMSR_LSTATUS))	/* link status up? */
		return -EIO;

	/* read MAC control register and clear related bits */
	maccr = readl(&priv->regs->maccr) &
		~(MAC_MACCR_GIGA_MODE |
		  MAC_MACCR_FAST_MODE |
		  MAC_MACCR_FULLDUP);

	if (stat_ge & LPA_1000FULL) {
		/* set gmac for 1000BaseTX and Full Duplex */
		maccr |= MAC_MACCR_GIGA_MODE | MAC_MACCR_FULLDUP;
	}

	if (stat_ge & LPA_1000HALF) {
		/* set gmac for 1000BaseTX and Half Duplex */
		maccr |= MAC_MACCR_GIGA_MODE;
	}

	if (stat_fe & BMSR_100FULL) {
		/* set MII for 100BaseTX and Full Duplex */
		maccr |= MAC_MACCR_FAST_MODE | MAC_MACCR_FULLDUP;
	}

	if (stat_fe & BMSR_10FULL) {
		/* set MII for 10BaseT and Full Duplex */
		maccr |= MAC_MACCR_FULLDUP;
	}

	if (stat_fe & BMSR_100HALF) {
		/* set MII for 100BaseTX and Half Duplex */
		maccr |= MAC_MACCR_FAST_MODE;
	}

	if (stat_fe & BMSR_10HALF) {
		/* set MII for 10BaseT and Half Duplex */
		/* we have already clear these bits, do nothing */
		;
	}

	/* update MII config into maccr */
	writel(maccr, &priv->regs->maccr);

	return 0;
}

static int ast_nic_start(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct ast_nic_xdes *txdes = priv->txdes;
	struct ast_nic_xdes *rxdes = priv->rxdes;
	u32 maccr;
	int i;

	debug("%s()\n", __func__);

	ast_nic_reset(dev);

	/* disable all interrupts */
	writel(0, &priv->regs->ier);

	/* initialize descriptors */
	priv->tx_index = 0;
	priv->rx_index = 0;

	txdes[PKTBUFSTX - 1].des[0]	= cpu_to_le32(MAC_TXDES0_EDOTR);
	rxdes[PKTBUFSRX - 1].des[0]	= cpu_to_le32(MAC_RXDES0_EDORR);

	for (i = 0; i < PKTBUFSTX; i++) {
		/* TXBUF_BADR */
		txdes[i].des[3] = 0;
		txdes[i].des[1] = 0;
	}

	for (i = 0; i < PKTBUFSRX; i++) {
		/* RXBUF_BADR */
		rxdes[i].des[3] = (u32)net_rx_packets[i];

		rxdes[i].des[0] &= ~MAC_RXDES0_RXPKT_RDY;
	}

	/* transmit ring */
	writel((u32)txdes, &priv->regs->txr_badr);

	/* receive ring */
	writel((u32)rxdes, &priv->regs->rxr_badr);

	/* poll receive descriptor automatically */
	writel((1 << MAC_APTC_RXPOLL_CNT_SHIFT), &priv->regs->aptc);

	/* config receive buffer size register */
	writel(RBSR_DEFAULT_VALUE, &priv->regs->rbsr);

	/* enable transmitter, receiver */
	maccr = MAC_MACCR_TXMAC_EN |
		MAC_MACCR_RXMAC_EN |
		MAC_MACCR_TXDMA_EN |
		MAC_MACCR_RXDMA_EN |
		MAC_MACCR_FULLDUP |
		MAC_MACCR_CRC_APD |
		MAC_MACCR_RX_RUNT |
		MAC_MACCR_RX_BROADPKT;

	writel(maccr, &priv->regs->maccr);

	ast_nic_phy_init(dev);

	return ast_nic_update_link_speed(dev);
}

static void ast_nic_stop(struct udevice *dev)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);

	debug("%s()\n", __func__);

	clrbits_le32(&priv->regs->maccr,
		     MAC_MACCR_TXDMA_EN | MAC_MACCR_RXDMA_EN |
		     MAC_MACCR_TXMAC_EN | MAC_MACCR_RXMAC_EN);
}

static int ast_nic_send(struct udevice *dev, void *packet, int length)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct ast_nic_xdes *curr_des = &priv->txdes[priv->tx_index];
	unsigned long start;
	unsigned long now;
	unsigned long diff_time;

	if (curr_des->des[0] & MAC_TXDES0_TXDMA_OWN) {
		debug("%s(): no TX descriptor available\n", __func__);
		return -EIO;
	}

	debug("%s(%x, %x)\n", __func__, (int)packet, length);

	length = (length < ETH_ZLEN) ? ETH_ZLEN : length;

	/* initiate a transmit sequence */
	curr_des->des[3] = (u32) packet;	/* TXBUF_BADR */

	curr_des->des[0] &= MAC_TXDES0_EDOTR;

	curr_des->des[0] |= (MAC_TXDES0_FTS |
			     MAC_TXDES0_LTS |
			     ((length << MAC_TXDES0_TXBUF_SIZE_SHIFT) &
			      MAC_TXDES0_TXBUF_SIZE_MASK) |
			     MAC_TXDES0_TXDMA_OWN);

	/* start transmit */
	writel(1, &priv->regs->txpd);
	invalidate_dcache_range((u32) curr_des,
				(u32) curr_des + sizeof(*curr_des));

	/* wait for transfer to succeed */
	start = get_timer(0);

	while (curr_des->des[0] & MAC_TXDES0_TXDMA_OWN) {
		now = get_timer(0);
		if (now < start)
			now += 0xffffffff;
		diff_time = now - start;
		if (diff_time >= 5000) {
			debug("%s(): timed out\n", __func__);
			return -ETIMEDOUT;
		}
	}

	debug("%s(): packet sent\n", __func__);

	priv->tx_index = (priv->tx_index + 1) % PKTBUFSTX;

	return 0;
}

static int ast_nic_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct ast_nic_priv *priv = dev_get_priv(dev);
	struct ast_nic_xdes *curr_des;
	unsigned short rxlen;

	curr_des = &priv->rxdes[priv->rx_index];

	invalidate_dcache_range((u32) curr_des,
				(u32) curr_des + sizeof(*curr_des));

	if (!(curr_des->des[0] & MAC_RXDES0_RXPKT_RDY))
		return -ENOMSG;

	if (curr_des->des[0] & (MAC_RXDES0_RX_ERR |
				MAC_RXDES0_CRC_ERR |
				MAC_RXDES0_FTL |
				MAC_RXDES0_RUNT | MAC_RXDES0_RX_ODD_NB)) {
		return -EIO;
	}

	rxlen =
	    (curr_des->des[0] >> MAC_RXDES0_VDBC_SHIFT) & MAC_RXDES0_VDBC_MASK;

	debug("%s(): RX buffer %d, %x received\n",
	      __func__, priv->rx_index, rxlen);

	invalidate_dcache_range((u32) curr_des->des[3],
				(u32) curr_des->des[3] + rxlen);

	/* pass the packet up to the protocol layers. */
	net_process_received_packet((void *)curr_des->des[3], rxlen);

	curr_des->des[0] &= ~MAC_RXDES0_RXPKT_RDY;

	priv->rx_index = (priv->rx_index + 1) % PKTBUFSRX;

	return 0;
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
