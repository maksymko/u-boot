/*
 * Copyright 2016 Google Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* Core Clocks */

#define PLL_HPLL	(1)
#define PLL_DPLL	(2)
#define PLL_D2PLL	(3)
#define PLL_MPLL	(4)
#define ARMCLK		(5)


/* Bus Clocks, derived from core clocks */
#define BCLK_PCLK	(101)
#define BCLK_LHCLK	(102)
#define BCLK_MACCLK	(103)
#define BCLK_SDCLK	(104)
#define BCLK_ARMCLK	(105)

#define MCLK_DDR	(201)
