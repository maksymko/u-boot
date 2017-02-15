/*
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * (C) Copyright 2010 Andes Technology
 * Macpaul Lin <macpaul@andestech.com>
 *
 * Copyright 2017 Google Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __AST_NIC_H
#define __AST_NIC_H

struct ast_nic_regs {
	u32	isr;		/* 0x00 */
	u32	ier;		/* 0x04 */
	u32	mac_madr;	/* 0x08 */
	u32	mac_ladr;	/* 0x0c */
	u32	maht0;		/* 0x10 */
	u32	maht1;		/* 0x14 */
	u32	txpd;		/* 0x18 */
	u32	rxpd;		/* 0x1c */
	u32	txr_badr;	/* 0x20 */
	u32	rxr_badr;	/* 0x24 */
	u32	hptxpd;		/* 0x28 */
	u32	hptxpd_badr;	/* 0x2c */
	u32	itc;		/* 0x30 */
	u32	aptc;		/* 0x34 */
	u32	dblac;		/* 0x38 */
	u32	dmafifos;	/* 0x3c */
	u32	fear0;		/* 0x40 */
	u32	fear1;		/* 0x44 */
	u32	tpafcr;		/* 0x48 */
	u32	rbsr;		/* 0x4c */
	u32	maccr;		/* 0x50 */
	u32	macsr;		/* 0x54 */
	u32	tm;		/* 0x58 */
	u32	physts;		/* 0x5c */
	u32	phycr;		/* 0x60 */
	u32	phydata;	/* 0x64 */
	u32	fcr;		/* 0x68 */
	u32	bpr;		/* 0x6c */
	u32	wolcr;		/* 0x70 */
	u32	wolsr;		/* 0x74 */
	u32	wfbm1m;		/* 0x78 */
	u32	wfbm1l;		/* 0x7c */
	u32	wfbm2m;		/* 0x80 */
	u32	wfbm2l;		/* 0x84 */
	u32	wfbm3m;		/* 0x88 */
	u32	wfbm3l;		/* 0x8c */
	u32	nptxr_ptr;	/* 0x90 */
	u32	hptxr_ptr;	/* 0x94 */
	u32	rxr_ptr;	/* 0x98 */
	u32	resv3;		/* 0x9c */
	u32	tx;		/* 0xa0 */
	u32	tx_mcol_scol;	/* 0xa4 */
	u32	tx_ecol_fail;	/* 0xa8 */
	u32	tx_lcol_und;	/* 0xac */
	u32	rx;		/* 0xb0 */
	u32	rx_bc;		/* 0xb4 */
	u32	rx_mc;		/* 0xb8 */
	u32	rx_pf_aep;	/* 0xbc */
	u32	rx_runt;	/* 0xc0 */
	u32	rx_crcer_ftl;	/* 0xc4 */
	u32	rx_col_lost;	/* 0xc8 */
};

/*
 * Interrupt status register & interrupt enable register
 */
#define MAC_INT_RPKT_BUF		(1 << 0)
#define MAC_INT_RPKT_FIFO		(1 << 1)
#define MAC_INT_NO_RXBUF		(1 << 2)
#define MAC_INT_RPKT_LOST		(1 << 3)
#define MAC_INT_XPKT_ETH		(1 << 4)
#define MAC_INT_XPKT_FIFO		(1 << 5)
#define MAC_INT_NO_NPTXBUF	(1 << 6)
#define MAC_INT_XPKT_LOST		(1 << 7)
#define MAC_INT_AHB_ERR		(1 << 8)
#define MAC_INT_PHYSTS_CHG	(1 << 9)
#define MAC_INT_NO_HPTXBUF	(1 << 10)
#define MAC_INT_PHY_CHG		(1 << 28)
#define MAC_INT_PHY_TIMEOUT	(1 << 29)
#define MAC_INT_FLAG_ACK		(1 << 30)
#define MAC_INT_FLAG_REQ		(1 << 31)

/*
 * MAC control register
 */
#define MAC_MACCR_TXDMA_EN	(1 << 0)
#define MAC_MACCR_RXDMA_EN	(1 << 1)
#define MAC_MACCR_TXMAC_EN	(1 << 2)
#define MAC_MACCR_RXMAC_EN	(1 << 3)
#define MAC_MACCR_RM_VLAN		(1 << 4)
#define MAC_MACCR_HPTXR_EN	(1 << 5)
#define MAC_MACCR_LOOP_EN		(1 << 6)
#define MAC_MACCR_ENRX_IN_HALFTX	(1 << 7)
#define MAC_MACCR_FULLDUP		(1 << 8)
#define MAC_MACCR_GIGA_MODE	(1 << 9)
#define MAC_MACCR_CRC_APD		(1 << 10)
#define MAC_MACCR_LOW_SEN		(1 << 11)
#define MAC_MACCR_RX_RUNT		(1 << 12)
#define MAC_MACCR_JUMBO_LF	(1 << 13)
#define MAC_MACCR_RX_ALL		(1 << 14)
#define MAC_MACCR_HT_MULTI_EN	(1 << 15)
#define MAC_MACCR_RX_MULTIPKT	(1 << 16)
#define MAC_MACCR_RX_BROADPKT	(1 << 17)
#define MAC_MACCR_DISCARD_CRCERR	(1 << 18)
#define MAC_MACCR_FAST_MODE	(1 << 19)
#define MAC_MACCR_SW_RST		(1 << 31)

/*
 * Feature Enable Register
 */
#define MAC_FEAR_NEW_MD_IFACE		(1 << 31)
#define MAC_FEAR_LOOPBACK			(1 << 30)


#endif  /* __AST_NIC_H */
