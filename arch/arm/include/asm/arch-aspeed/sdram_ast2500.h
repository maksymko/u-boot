#ifndef _ASM_ARCH_SDRAM_AST2500_H
#define _ASM_ARCH_SDRAM_AST2500_H

#ifndef __ASSEMBLY__

struct ast2500_sdrammc_regs {
  u32 protection_key;       /* MCR00 */
  u32 config;               /* MCR04 */
  u32 gm_protection_key;    /* MCR08 */
  u32 refresh_timing;       /* MCR0C */
  u32 ac_timing1;           /* MCR10 */
  u32 ac_timing2;           /* MCR14 */
  u32 ac_timing3;           /* MCR18 */
  u32 misc_control;         /* MCR1C */
  u32 mr45_mode_setting;    /* MCR20 */
  u32 mr5_mode_setting;     /* MCR24 */
  u32 mode_setting_control; /* MCR28 */
  u32 mr02_mode_setting;    /* MCR2C */
  u32 m13_mode_setting;     /* MCR30 */
  u32 power_control;        /* MCR34 */
  u32 rq_limit_mask;        /* MCR38 */
  u32 pri_group_setting;    /* MCR3C */
  u32 max_grant_len1;       /* MCR40 */
  u32 max_grant_len2;       /* MCR44 */
  u32 max_grant_len3;       /* MCR48 */
  u32 max_grant_len4;       /* MCR4C */
  u32 intr_ctrl;            /* MCR50 */
  u32 ecc_range_ctrl;       /* MCR54 */
  u32 first_ecc_err_addr;   /* MCR58 */
  u32 last_ecc_err_addr;    /* MCR5C */
  u32 phy_ctrl1;            /* MCR60 */
  u32 phy_ctrl2;            /* MCR64 */
  u32 phy_ctrl3;            /* MCR68 */
  u32 phy_ctrl4;            /* MCR6C */
  u32 ecc_test_ctrl;        /* MCR70 */
  u32 test_addr;            /* MCR74 */
  u32 test_fail_dq_bit;     /* MCR78 */
  u32 test_init_val;        /* MCR7C */
  u32 phy_debug_ctrl;       /* MCR80 */
  u32 phy_debug_data;       /* MCR84 */
  u32 reserved1[30];
  u32 scu_passwd;           /* MCR100 */
  u32 reserved2[7];
  u32 scu_mpll;             /* MCR120 */
  u32 reserved3[19];
  u32 scu_hwstrap;          /* MCR170 */
};

#endif  /* __ASSEMBLY__ */

#endif  /* _ASM_ARCH_SDRAM_AST2500_H */
