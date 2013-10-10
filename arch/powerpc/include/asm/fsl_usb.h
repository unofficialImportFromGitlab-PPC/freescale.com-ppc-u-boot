/*
 * Freescale USB Controller
 *
 * Copyright 2013 Freescale Semiconductor, Inc.
 *
 * This software may be used and distributed according to the
 * terms of the GNU Public License, Version 2, incorporated
 * herein by reference.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _ASM_FSL_USB_H_
#define _ASM_FSL_USB_H_

#include <stdbool.h>

#ifdef CONFIG_SYS_FSL_USB_DUAL_PHY_ENABLE
struct ccsr_usb_port_ctrl {
	u32	ctrl;
	u32	drvvbuscfg;
	u32	pwrfltcfg;
	u32	sts;
	u8	res_14[0xc];
	u32	bistcfg;
	u32	biststs;
	u32	abistcfg;
	u32	abiststs;
	u8	res_30[0x10];
	u32	xcvrprg;
	u32	anaprg;
	u32	anadrv;
	u32	anasts;
};

struct ccsr_usb_phy {
	u32	id;
	struct ccsr_usb_port_ctrl port1;
	u8	res_50[0xc];
	u32	tvr;
	u32	pllprg[4];
	u8	res_70[0x4];
	u32	anaccfg;
	u32	dbg;
	u8	res_7c[0x4];
	struct ccsr_usb_port_ctrl port2;
	u8	res_dc[0x334];
};

#define CONFIG_SYS_FSL_USB_CTRL_PHY_EN (1 << 0)
#define CONFIG_SYS_FSL_USB_DRVVBUS_CR_EN (1 << 1)
#define CONFIG_SYS_FSL_USB_PWRFLT_CR_EN (1 << 1)
#define CONFIG_SYS_FSL_USB_PLLPRG1_PHY_DIV (1 << 0)
#define CONFIG_SYS_FSL_USB_PLLPRG2_PHY2_CLK_EN (1 << 0)
#define CONFIG_SYS_FSL_USB_PLLPRG2_PHY1_CLK_EN (1 << 1)
#define CONFIG_SYS_FSL_USB_PLLPRG2_FRAC_LPF_EN (1 << 13)
#define CONFIG_SYS_FSL_USB_PLLPRG2_REF_DIV (1 << 4)
#define CONFIG_SYS_FSL_USB_PLLPRG2_MFI (5 << 16)
#define CONFIG_SYS_FSL_USB_PLLPRG2_PLL_EN (1 << 21)
#define CONFIG_SYS_FSL_USB_SYS_CLK_VALID (1 << 0)
#define CONFIG_SYS_FSL_USB_XCVRPRG_HS_DCNT_PROG_EN (1 << 7)
#define CONFIG_SYS_FSL_USB_XCVRPRG_HS_DCNT_PROG_MASK (3 << 4)

#define INC_DCNT_THRESHOLD_25MV        (0 << 4)
#define INC_DCNT_THRESHOLD_50MV        (1 << 4)
#define DEC_DCNT_THRESHOLD_25MV        (2 << 4)
#define DEC_DCNT_THRESHOLD_50MV        (3 << 4)

#ifdef CONFIG_SYS_FSL_ERRATUM_A006918
extern bool	has_fsl_erratum_a006918;
#define FSL_MAX_USBPLL_RETRY_COUNT	7
#endif

#else
struct ccsr_usb_phy {
	u32     config1;
	u32     config2;
	u32     config3;
	u32     config4;
	u32     config5;
	u32     status1;
	u32	usb_enable_override;
	u8	res[0xe4];
};
#define CONFIG_SYS_FSL_USB_HS_DISCNCT_INC (3 << 22)
#define CONFIG_SYS_FSL_USB_RX_AUTO_CAL_RD_WR_SEL (1 << 20)
#define CONFIG_SYS_FSL_USB_SQUELCH_PROG_WR_0 13
#define CONFIG_SYS_FSL_USB_SQUELCH_PROG_WR_3 16
#define CONFIG_SYS_FSL_USB_SQUELCH_PROG_RD_0 0
#define CONFIG_SYS_FSL_USB_SQUELCH_PROG_RD_3 3
#define CONFIG_SYS_FSL_USB_ENABLE_OVERRIDE 1
#define CONFIG_SYS_FSL_USB_SQUELCH_PROG_MASK 0x07

#endif

#ifdef CONFIG_SYS_FSL_ERRATUM_A005728
/* Retry count for checking UTMI PHY CLK validity */
#define UTMI_PHY_CLK_VALID_CHK_RETRY 5
extern bool has_erratum_a005728(void);
#endif

#ifdef CONFIG_SYS_FSL_ERRATUM_A006261
extern bool has_erratum_a006261(void);
#endif

#endif /*_ASM_FSL_USB_H_ */
