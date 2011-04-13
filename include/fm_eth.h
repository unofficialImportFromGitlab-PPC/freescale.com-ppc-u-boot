/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 *	Dave Liu <daveliu@freescale.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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

#ifndef __FM_ETH_H__
#define __FM_ETH_H__

#include <common.h>
#include <asm/types.h>
#include <asm/fsl_enet.h>

enum fm_port {
	FM1_DTSEC1,
	FM1_DTSEC2,
	FM1_DTSEC3,
	FM1_DTSEC4,
	FM1_DTSEC5,
	FM1_10GEC1,
	FM2_DTSEC1,
	FM2_DTSEC2,
	FM2_DTSEC3,
	FM2_DTSEC4,
	FM2_10GEC1,
	NUM_FM_PORTS,
};

struct fm_bmi_rx_port {
	u32 fmbm_rcfg;	/* Rx configuration */
	u32 fmbm_rst;	/* Rx status */
	u32 fmbm_rda;	/* Rx DMA attributes */
	u32 fmbm_rfp;	/* Rx FIFO parameters */
	u32 fmbm_rfed;	/* Rx frame end data */
	u32 fmbm_ricp;	/* Rx internal context parameters */
	u32 fmbm_rim;	/* Rx internal margins */
	u32 fmbm_rebm;	/* Rx external buffer margins */
	u32 fmbm_rfne;	/* Rx frame next engine */
	u32 fmbm_rfca;	/* Rx frame command attributes */
	u32 fmbm_rfpne;	/* Rx frame parser next engine */
	u32 fmbm_rpso;	/* Rx parse start offset */
	u32 fmbm_rpp;	/* Rx policer profile */
	u32 fmbm_rccb;	/* Rx coarse classification base */
	u32 res1[0x2];
	u32 fmbm_rprai[0x8];	/* Rx parse results array Initialization */
	u32 fmbm_rfqid;		/* Rx frame queue ID */
	u32 fmbm_refqid;	/* Rx error frame queue ID */
	u32 fmbm_rfsdm;		/* Rx frame status discard mask */
	u32 fmbm_rfsem;		/* Rx frame status error mask */
	u32 fmbm_rfene;		/* Rx frame enqueue next engine */
	u32 res2[0x23];
	u32 fmbm_ebmpi[0x8];	/* buffer manager pool information */
	u32 fmbm_acnt[0x8];	/* allocate counter */
	u32 res3[0x8];
	u32 fmbm_cgm[0x8];	/* congestion group map */
	u32 fmbm_mpd;		/* BMan pool depletion */
	u32 res4[0x1F];
	u32 fmbm_rstc;		/* Rx statistics counters */
	u32 fmbm_rfrc;		/* Rx frame counters */
	u32 fmbm_rfbc;		/* Rx bad frames counter */
	u32 fmbm_rlfc;		/* Rx large frames counter */
	u32 fmbm_rffc;		/* Rx filter frames counter */
	u32 fmbm_rfdc;		/* Rx frame discard counter */
	u32 fmbm_rfldec;	/* Rx frames list DMA error counter */
	u32 fmbm_rodc;		/* Rx out of buffers discard counter */
	u32 fmbm_rbdc;		/* Rx buffers deallocate counter */
	u32 res5[0x17];
	u32 fmbm_rpc;		/* Rx performance counters */
	u32 fmbm_rpcp;		/* Rx performance count parameters */
	u32 fmbm_rccn;		/* Rx cycle counter */
	u32 fmbm_rtuc;		/* Rx tasks utilization counter */
	u32 fmbm_rrquc;		/* Rx receive queue utilization counter */
	u32 fmbm_rduc;		/* Rx DMA utilization counter */
	u32 fmbm_rfuc;		/* Rx FIFO utilization counter */
	u32 fmbm_rpac;		/* Rx pause activation counter */
	u32 res6[0x18];
	u32 fmbm_rdbg;		/* Rx debug configuration */
} __attribute__ ((packed));

/* FMBM_RCFG - Rx configuration
 */
#define FMBM_RCFG_EN		0x80000000 /* port is enabled to receive data */
#define FMBM_RCFG_FDOVR		0x02000000 /* frame discard override */
#define FMBM_RCFG_IM		0x01000000 /* independent mode */

/* FMBM_RST - Rx status
 */
#define FMBM_RST_BSY		0x80000000 /* Rx port is busy */

/* FMBM_RDA - Rx DMA attributes
 */
#define FMBM_RDA_SWAP_MASK	0xc0000000
#define FMBM_RDA_SWAP_SHIFT	30
#define FMBM_RDA_NO_SWAP	0x00000000
#define FMBM_RDA_SWAP_LE	0x40000000 /* swapped in powerpc little endian mode */
#define FMBM_RDA_SWAP_BE	0x80000000 /* swapped in big endian mode */
#define FMBM_RDA_ICC_MASK	0x30000000
#define FMBM_RDA_ICC_NOSTASH	0x00000000 /* IC write cacheable, no allocate */
#define FMBM_RDA_ICC_STASH	0x10000000 /* IC write cacheable and allocate */
#define FMBM_RDA_FHC_MASK	0x0c000000
#define FMBM_RDA_FHC_NOSTASH	0x00000000 /* frame header write cacheable, no allocate */
#define FMBM_RDA_FHC_STASH	0x04000000 /* frame header write cacheable and allocate */
#define FMBM_RDA_SGC_MASK	0x03000000
#define FMBM_RDA_SGC_NOSTASH	0x00000000 /* scatter gather write cacheable, no alloc */
#define FMBM_RDA_SGC_STASH	0x01000000 /* scatter gather write cacheable and alloc */
#define FMBM_RDA_WOPT_MASK	0x00300000
#define FMBM_RDA_NO_OPT		0x00000000
#define FMBM_RDA_WRITE_OPT	0x00100000 /* allow to write more bytes than the actual */

#define FMBM_RDA_MASK_ALL	(FMBM_RDA_SWAP_MASK | FMBM_RDA_ICC_MASK \
				| FMBM_RDA_FHC_MASK | FMBM_RDA_SGC_MASK \
				| FMBM_RDA_WOPT_MASK)

#define FMBM_RDA_INIT		0x00000000

/* FMBM_RFP - Rx FIFO parameters
 */
#define FMBM_RFP_DEFAULT	0x03ff03ff

/* FMBM_RFED - Rx Frame end data
 */
#define FMBM_RFED_DEFAULT	0x00000000

/* FMBM_RICP - Rx internal context
 */
#define FMBM_RICP_DEFAULT	0x00000002

/* FMBM_RFCA - Rx frame command attributes
 */
#define FMBM_RFCA_ORDER		0x80000000
#define FMBM_RFCA_SYNC		0x02000000
#define FMBM_RFCA_DEFAULT	0x003c0000

/* FMBM_RSTC - Rx statistics
 */
#define FMBM_RSTC_EN		0x80000000 /* statistics counters enable */

struct fm_bmi_tx_port {
	u32 fmbm_tcfg;	/* Tx configuration */
	u32 fmbm_tst;	/* Tx status */
	u32 fmbm_tda;	/* Tx DMA attributes */
	u32 fmbm_tfp;	/* Tx FIFO parameters */
	u32 fmbm_tfed;	/* Tx frame end data */
	u32 fmbm_ticp;	/* Tx internal context parameters */
	u32 fmbm_tfne;	/* Tx frame next engine */
	u32 fmbm_tfca;	/* Tx frame command attributes */
	u32 fmbm_tcfqid;/* Tx confirmation frame queue ID */
	u32 fmbm_tfeqid;/* Tx error frame queue ID */
	u32 fmbm_tfene;	/* Tx frame enqueue next engine */
	u32 fmbm_trlmts;/* Tx rate limiter scale */
	u32 fmbm_trlmt;	/* Tx rate limiter */
	u32 res0[0x73];
	u32 fmbm_tstc;	/* Tx statistics counters */
	u32 fmbm_tfrc;	/* Tx frame counter */
	u32 fmbm_tfdc;	/* Tx frames discard counter */
	u32 fmbm_tfledc;/* Tx frame length error discard counter */
	u32 fmbm_tfufdc;/* Tx frame unsupported format discard counter */
	u32 fmbm_tbdc;	/* Tx buffers deallocate counter */
	u32 res1[0x1a];
	u32 fmbm_tpc;	/* Tx performance counters */
	u32 fmbm_tpcp;	/* Tx performance count parameters */
	u32 fmbm_tccn;	/* Tx cycle counter */
	u32 fmbm_ttuc;	/* Tx tasks utilization counter */
	u32 fmbm_ttcquc;/* Tx transmit confirm queue utilization counter */
	u32 fmbm_tduc;	/* Tx DMA utilization counter */
	u32 fmbm_tfuc;	/* Tx FIFO utilization counter */
	u32 res2[0x19];
	u32 fmbm_tdcfg;	/* Tx debug configuration */
} __attribute__ ((packed));

/* FMBM_TCFG - Tx configuration
 */
#define FMBM_TCFG_EN	0x80000000 /* port is enabled to transmit data */
#define FMBM_TCFG_IM	0x01000000 /* independent mode enable */

/* FMBM_TST - Tx status
 */
#define FMBM_TST_BSY		0x80000000 /* Tx port is busy */

/* FMBM_TDA - Tx DMA attributes
 */
#define FMBM_TDA_INIT		0x00000000 /* No swap, no stashing */

/* FMBM_TFP - Tx FIFO parameters
 */
#define FMBM_TFP_INIT		0x00000013 /* pipeline=1, low comfort=5KB */

/* FMBM_TFED - Tx frame end data
 */
#define FMBM_TFED_DEFAULT	0x00000000

/* FMBM_TICP - Tx internal context parameters
 */
#define FMBM_TICP_DEFAULT	0x00000000

/* FMBM_TFCA - Tx frame command attributes
 */
#define FMBM_TFCA_DEFAULT	0x00040000

/* FMBM_TSTC - Tx statistics counters
 */
#define FMBM_TSTC_EN		0x80000000

/* Rx/Tx buffer descriptor
 */
struct fm_port_bd {
	u16 status;
	u16 len;
	u32 res0;
	u16 res1;
	u16 buf_ptr_hi;
	u32 buf_ptr_lo;
} __attribute__ ((packed));

#define SIZEOFBD		sizeof(struct fm_port_bd)

/* Common BD flags
*/
#define BD_LAST			0x0800

/* Rx BD status flags
*/
#define RxBD_EMPTY		0x8000
#define RxBD_LAST		BD_LAST
#define RxBD_FIRST		0x0400
#define RxBD_PHYS_ERR		0x0008
#define RxBD_SIZE_ERR		0x0004
#define RxBD_ERROR		(RxBD_PHYS_ERR | RxBD_SIZE_ERR)

/* Tx BD status flags
*/
#define TxBD_READY		0x8000
#define TxBD_LAST		BD_LAST

/* Rx/Tx queue descriptor
 */
struct fm_port_qd {
	u16 gen;
	u16 bd_ring_base_hi;
	u32 bd_ring_base_lo;
	u16 bd_ring_size;
	u16 offset_in;
	u16 offset_out;
	u16 res0;
	u32 res1[0x4];
} __attribute__ ((packed));

#define QD_RXF_INT_MASK		0x0010 /* 1=set interrupt for receive frame */
#define QD_BSY_INT_MASK		0x0008 /* 1=set interrupt for receive busy event */
#define QD_FPMEVT_SEL_MASK	0x0003

/* IM global parameter RAM
 */
struct fm_port_global_pram {
	u32 mode;	/* independent mode register */
	u32 rxqd_ptr;	/* Rx queue descriptor pointer, RxQD size=32 bytes
			   8 bytes aligned, independent mode page address + 0x20 */
	u32 txqd_ptr;	/* Tx queue descriptor pointer, TxQD size=32 bytes
			   8 bytes aligned, independent mode page address + 0x40 */
	u16 mrblr;	/* max Rx buffer length, set its value to the power of 2 */
	u16 rxqd_bsy_cnt;	/* RxQD busy counter, should be cleared */
	u32 res0[0x4];
	struct fm_port_qd rxqd;	/* Rx queue descriptor */
	struct fm_port_qd txqd;	/* Tx queue descriptor */
	u32 res1[0x28];
} __attribute__ ((packed));

#define FM_PRAM_SIZE		sizeof(struct fm_port_global_pram)
#define FM_PRAM_ALIGN		256
#define PRAM_MODE_GLOBAL	0x20000000
#define PRAM_MODE_GRACEFUL_STOP	0x00800000

/****************************
 * Fman ethernet
 ****************************/
enum fm_eth_type {
	fm_eth_1g_e,
	fm_eth_10g_e,
};

/* Fman MAC controller */

#define CONFIG_SYS_FM1_DTSEC1_MDIO_ADDR	(CONFIG_SYS_FSL_FM1_ADDR + 0xe1120)
#define CONFIG_SYS_FM1_TGEC_MDIO_ADDR	(CONFIG_SYS_FSL_FM1_ADDR + 0xf1000)

#define DEFAULT_FM_MDIO_NAME "FSL_MDIO0"
#define DEFAULT_FM_TGEC_MDIO_NAME "FM_TGEC_MDIO"

/* Fman ethernet info struct */
#define FM_ETH_INFO_INITIALIZER(idx, pregs) \
	.fm		= idx,						\
	.phy_regs	= (void *)pregs,				\
	.enet_if	= PHY_INTERFACE_MODE_NONE,			\

#ifdef CONFIG_FSL_CORENET
#define FM_DEVDISR_DTSEC1_1	FSL_CORENET_DEVDISR2_DTSEC1_1
#define FM_DEVDISR_DTSEC1_2	FSL_CORENET_DEVDISR2_DTSEC1_2
#define FM_DEVDISR_DTSEC1_3	FSL_CORENET_DEVDISR2_DTSEC1_3
#define FM_DEVDISR_DTSEC1_4	FSL_CORENET_DEVDISR2_DTSEC1_4
#define FM_DEVDISR_DTSEC1_5	FSL_CORENET_DEVDISR2_DTSEC1_5
#define FM_DEVDISR_DTSEC2_1	FSL_CORENET_DEVDISR2_DTSEC2_1
#define FM_DEVDISR_DTSEC2_2	FSL_CORENET_DEVDISR2_DTSEC2_2
#define FM_DEVDISR_DTSEC2_3	FSL_CORENET_DEVDISR2_DTSEC2_3
#define FM_DEVDISR_DTSEC2_4	FSL_CORENET_DEVDISR2_DTSEC2_4
#else
#define FM_DEVDISR_DTSEC1_1	MPC85xx_DEVDISR_TSEC1
#define FM_DEVDISR_DTSEC1_2	MPC85xx_DEVDISR_TSEC2
#define FM_DEVDISR_DTSEC1_3	0
#define FM_DEVDISR_DTSEC1_4	0
#define FM_DEVDISR_DTSEC1_5	0
#define FM_DEVDISR_DTSEC2_1	0
#define FM_DEVDISR_DTSEC2_2	0
#define FM_DEVDISR_DTSEC2_3	0
#define FM_DEVDISR_DTSEC2_4	0
#endif

#define FM_DTSEC_INFO_INITIALIZER(idx, n) \
{									\
	FM_ETH_INFO_INITIALIZER(idx, CONFIG_SYS_FM1_DTSEC1_MDIO_ADDR)	\
	.num		= n - 1,					\
	.type		= fm_eth_1g_e,					\
	.port		= FM##idx##_DTSEC##n,				\
	.devdisr_mask	= FM_DEVDISR_DTSEC##idx##_##n,			\
	.compat_offset	= CONFIG_SYS_FSL_FM##idx##_OFFSET +		\
				offsetof(struct ccsr_fman, mac[n-1]),	\
}

#define FM_TGEC_INFO_INITIALIZER(idx, n) \
{									\
	FM_ETH_INFO_INITIALIZER(idx, CONFIG_SYS_FM1_TGEC_MDIO_ADDR)	\
	.num		= n + 3,					\
	.type		= fm_eth_10g_e,					\
	.port		= FM##idx##_10GEC##n,				\
	.devdisr_mask	= FSL_CORENET_DEVDISR2_10GEC##idx,		\
	.compat_offset	= CONFIG_SYS_FSL_FM##idx##_OFFSET +		\
				offsetof(struct ccsr_fman, fm_10gec),	\
}

struct fm_eth_info {
	int enabled;
	u32 devdisr_mask;
	int fm;
	int num;
	enum fm_port port;
	enum fm_eth_type type;
	u8 phy_addr;
	void *phy_regs;
	phy_interface_t enet_if;
	u32 compat_offset;
	struct mii_dev *bus;
};

struct tgec_mdio_info {
	struct tgec_mdio_controller *regs;
	char *name;
};

int fm_tgec_mdio_init(bd_t *bis, struct tgec_mdio_info *info);

int fm_standard_init(bd_t *bis);
int fm_initialize(int index, struct fm_eth_info *fm_info, int num);
void fman_enet_init(void);
void fdt_fixup_fman_ethernet(void *fdt);
phy_interface_t fm_info_get_enet_if(enum fm_port port);
void fm_info_set_phy_address(enum fm_port port, int address);
void fm_info_set_mdio(enum fm_port port, struct mii_dev *bus);

#endif
