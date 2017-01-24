/*
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 * Copyright 2016 IBM Corporation
 * Copyright 2017 Google, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <i2c.h>
#include <asm/io.h>
#include "ast_i2c.h"

#define I2C_TIMEOUT_US 100000
#define I2C_SLEEP_STEP_US 20

DECLARE_GLOBAL_DATA_PTR;

/*
 * Device private data
 */
struct ast_i2c_priv {
	/* Device registers */
	struct ast_i2c_regs *regs;
	/* I2C speed in Hz */
	int speed;
};

/*
 * Given desired divider ratio, return the value that needs to be set
 * in Clock and AC Timing Control register
 */
static u32 get_clk_reg_val(ulong divider_ratio)
{
	ulong inc = 0, div;
	ulong scl_low, scl_high, data;

	for (div = 0; divider_ratio >= 16; div++) {
		inc |= (divider_ratio & 1);
		divider_ratio >>= 1;
	}
	divider_ratio += inc;
	scl_low = (divider_ratio >> 1) - 1;
	scl_high = divider_ratio - scl_low - 2;
	data = I2CD_CACTC_BASE
			| (scl_high << I2CD_TCKHIGH_SHIFT)
			| (scl_low << I2CD_TCKLOW_SHIFT)
			| (div << I2CD_BASE_DIV_SHIFT);

	return data;
}

static void ast_i2c_clear_interrupts(struct ast_i2c_regs *i2c_base)
{
	writel(~0, &i2c_base->isr);
}

static void ast_i2c_init_bus(struct udevice *dev)
{
	struct ast_i2c_priv *priv = dev_get_priv(dev);

	/* Reset device */
	writel(0, &priv->regs->fcr);
	/* Enable Master Mode. Assuming single-master */
	debug("Enable Master for %p\n", i2c_bus->regs);
	writel(I2CD_MASTER_EN
	       | I2CD_M_SDA_LOCK_EN
	       | I2CD_MULTI_MASTER_DIS | I2CD_M_SCL_DRIVE_EN,
	       &priv->regs->fcr);
	debug("FCR: %p\n", &priv->regs->fcr);
	/* Enable Interrupts */
	writel(I2CD_INTR_TX_ACK
	       | I2CD_INTR_TX_NAK
	       | I2CD_INTR_RX_DONE
	       | I2CD_INTR_BUS_RECOVER_DONE
	       | I2CD_INTR_NORMAL_STOP
	       | I2CD_INTR_ABNORMAL, &priv->regs->icr);
}

static int ast_i2c_ofdata_to_platdata(struct udevice *dev)
{
	struct ast_i2c_priv *priv = dev_get_priv(dev);
	struct ast_i2c_regs *regs = dev_get_addr_ptr(dev);

	if (IS_ERR(regs))
		return PTR_ERR(regs);

	priv->regs = i2c_base;

	return 0;
}

static int ast_i2c_probe(struct udevice *dev)
{
	struct ast_i2c_priv *priv = dev_get_priv(dev);

	debug("Enabling I2C%u\n", dev->seq);

	ast_i2c_init_bus(dev);

	return 0;
}

static inline int ast_i2c_wait_isr(struct ast_i2c_regs *i2c_base, u32 flag)
{
	int timeout = I2C_TIMEOUT_US;

	while (!(readl(&i2c_base->isr) & flag) && timeout > 0) {
		udelay(I2C_SLEEP_STEP_US);
		timeout -= I2C_SLEEP_STEP_US;
	}

	ast_i2c_clear_interrupts(i2c_base);
	if (timeout <= 0)
		return -ETIMEDOUT;

	return 0;
}

static inline int ast_i2c_send_stop(struct ast_i2c_regs *i2c_base)
{
	writel(I2CD_M_STOP_CMD, &i2c_base->csr);

	return ast_i2c_wait_isr(i2c_base, I2CD_INTR_NORMAL_STOP);
}

static inline int ast_i2c_wait_tx(struct ast_i2c_regs *i2c_base)
{
	int timeout = I2C_TIMEOUT_US;
	u32 flag = I2CD_INTR_TX_ACK | I2CD_INTR_TX_NAK;
	u32 status = readl(&i2c_base->isr) & flag;
	int ret = 0;

	while (!status && timeout > 0) {
		status = readl(&i2c_base->isr) & flag;
		udelay(I2C_SLEEP_STEP_US);
		timeout -= I2C_SLEEP_STEP_US;
	}

	if (status == I2CD_INTR_TX_NAK)
		ret = -EREMOTEIO;

	if (timeout <= 0)
		ret = -ETIMEDOUT;

	ast_i2c_clear_interrupts(i2c_base);

	return ret;
}

static int ast_i2c_start_txn(struct ast_i2c_regs *i2c_base, uint devaddr)
{
	/* Start and Send Device Address */
	writel(devaddr, &i2c_base->trbbr);
	writel(I2CD_M_START_CMD | I2CD_M_TX_CMD, &i2c_base->csr);

	return ast_i2c_wait_tx(i2c_base);
}

static int ast_i2c_read_data(struct ast_i2c *i2c_bus, u8 chip_addr, u8 *buffer,
			     size_t len, bool send_stop)
{
	struct ast_i2c_regs *i2c_base = i2c_bus->regs;
	int i2c_error =
	    ast_i2c_start_txn(i2c_base, (chip_addr << 1) | I2C_M_RD);
	u32 i2c_cmd = I2CD_M_RX_CMD;

	if (i2c_error < 0)
		return i2c_error;

	for (; len > 0; len--, buffer++) {
		if (len == 1)
			i2c_cmd |= I2CD_M_S_RX_CMD_LAST;
		writel(i2c_cmd, &i2c_base->csr);
		i2c_error = ast_i2c_wait_isr(i2c_base, I2CD_INTR_RX_DONE);
		if (i2c_error < 0)
			return i2c_error;
		*buffer = (readl(&i2c_base->trbbr) & I2CD_RX_DATA_MASK)
				>> I2CD_RX_DATA_SHIFT;
	}
	ast_i2c_clear_interrupts(i2c_base);

	if (send_stop)
		return ast_i2c_send_stop(i2c_base);

	return 0;
}

static int ast_i2c_write_data(struct ast_i2c *i2c_bus, u8 chip_addr, u8
			      *buffer, size_t len, bool send_stop)
{
	struct ast_i2c_regs *i2c_base = i2c_bus->regs;
	int i2c_error = ast_i2c_start_txn(i2c_base, (chip_addr << 1));

	if (i2c_error < 0)
		return i2c_error;

	for (; len > 0; len--, buffer++) {
		writel(*buffer, &i2c_base->trbbr);
		writel(I2CD_M_TX_CMD, &i2c_base->csr);
		i2c_error = ast_i2c_wait_tx(i2c_base);
		if (i2c_error < 0)
			return i2c_error;
	}

	if (send_stop)
		return ast_i2c_send_stop(i2c_base);

	return 0;
}

static int ast_i2c_deblock(struct udevice *dev)
{
	struct ast_i2c *i2c_bus = dev_get_priv(dev);
	struct ast_i2c_regs *i2c_base = i2c_bus->regs;
	u32 csr = readl(&i2c_base->csr);
	bool sda_high = csr & I2CD_SDA_LINE_STS;
	bool scl_high = csr & I2CD_SCL_LINE_STS;

	int ret = 0;

	if (sda_high && scl_high) {
		/* Bus is idle, no deblocking needed. */
		return 0;
	} else if (sda_high) {
		/* Send stop command */
		debug("Unterminated TXN in (%x), sending stop\n", csr);
		ret = ast_i2c_send_stop(i2c_base);
	} else if (scl_high) {
		/* Possibly stuck slave */
		debug("Bus stuck (%x), attempting recovery\n", csr);
		writel(I2CD_BUS_RECOVER_CMD, &i2c_base->csr);
		ret = ast_i2c_wait_isr(i2c_base, I2CD_INTR_BUS_RECOVER_DONE);
	} else {
		/* Just try to reinit the device. */
		ast_i2c_init_bus(i2c_bus);
	}

	return ret;
}

static int ast_i2c_xfer(struct udevice *dev, struct i2c_msg *msg, int nmsgs)
{
	struct ast_i2c *i2c_bus = dev_get_priv(dev);
	int ret = ast_i2c_deblock(dev);

	if (ret < 0)
		return ret;
	debug("i2c_xfer: %d messages\n", nmsgs);
	for (; nmsgs > 0; nmsgs--, msg++) {
		if (msg->flags & I2C_M_RD) {
			debug("i2c_read: chip=0x%x, len=0x%x, flags=0x%x\n",
			      msg->addr, msg->len, msg->flags);
			ret = ast_i2c_read_data(i2c_bus, msg->addr, msg->buf,
						msg->len, (nmsgs == 1));
		} else {
			debug("i2c_write: chip=0x%x, len=0x%x, flags=0x%x\n",
			      msg->addr, msg->len, msg->flags);
			ret = ast_i2c_write_data(i2c_bus, msg->addr, msg->buf,
						 msg->len, (nmsgs == 1));
		}
		if (ret) {
			debug("%s: error (%d)\n", __func__, ret);
			return -EREMOTEIO;
		}
	}

	return 0;
}

static int ast_i2c_set_speed(struct udevice *dev, unsigned int speed)
{
	struct ast_i2c *i2c_bus = dev_get_priv(dev);
	struct ast_i2c_regs *i2c_base = i2c_bus->regs;
	ulong pclk, divider;

	debug("Setting speed ofr I2C%d to <%u>\n", dev->seq, speed);
	if (!speed) {
		debug("No valid speed specified\n");
		return -EINVAL;
	}

	pclk = ast_get_apbclk();
	divider = pclk / speed;

	i2c_bus->speed = speed;
	if (speed > 400000) {
		debug("Enabling High Speed\n");
		setbits_le32(&i2c_base->fcr, I2CD_M_HIGH_SPEED_EN
			     | I2CD_M_SDA_DRIVE_1T_EN
			     | I2CD_SDA_DRIVE_1T_EN);
		writel(0x3, &i2c_base->cactcr2);
		writel(get_clk_reg_val(divider), &i2c_base->cactcr1);
	} else {
		debug("Enabling Normal Speed\n");
		writel(get_clk_reg_val(divider), &i2c_base->cactcr1);
		writel(I2CD_NO_TIMEOUT_CTRL, &i2c_base->cactcr2);
	}

	ast_i2c_clear_interrupts(i2c_base);

	return 0;
}

static const struct dm_i2c_ops ast_i2c_ops = {
	.xfer = ast_i2c_xfer,
	.set_bus_speed = ast_i2c_set_speed,
	.deblock = ast_i2c_deblock,
};

static const struct udevice_id ast_i2c_ids[] = {
	{ .compatible = "aspeed,ast2400-i2c-bus" },
	{ .compatible = "aspeed,ast2500-i2c-bus" },
	{ },
};

static const struct udevice_id ast_i2c_ctrl_ids[] = {
	{ .compatible = "aspeed,ast2400-i2c-controller" },
	{ .compatible = "aspeed,ast2500-i2c-controller" },
	{ },
};

U_BOOT_DRIVER(ast_i2c) = {
	.name = "ast_i2c",
	.id = UCLASS_I2C,
	.of_match = ast_i2c_ids,
	.probe = ast_i2c_probe,
	.ofdata_to_platdata = ast_i2c_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct ast_i2c_priv),
	.ops = &ast_i2c_ops,
};

/* Controller is a separate device that controls *all* i2c buses */
U_BOOT_DRIVER(ast_i2c_ctrl) = {
	.name = "ast_i2c_ctrl",
	.id = UCLASS_I2C,
	.of_match = ast_i2c_ctrl_ids,
	.probe = ast_i2c_ctrl_probe,
};
