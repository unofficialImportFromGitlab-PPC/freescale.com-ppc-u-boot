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

#ifndef __FM_H__
#define __FM_H__

#include <common.h>
#include <fm_eth.h>
#include <asm/fsl_enet.h>
#include <asm/fsl_fman.h>

#define MAX_NUM_FM		2

/* Port ID
 */
#define OH_PORT_ID_BASE		0x01
#define MAX_NUM_OH_PORT		7
#define RX_PORT_1G_BASE		0x08
#define MAX_NUM_RX_PORT_1G	CONFIG_SYS_NUM_FM1_DTSEC
#define RX_PORT_10G_BASE	0x10
#define TX_PORT_1G_BASE		0x28
#define MAX_NUM_TX_PORT_1G	CONFIG_SYS_NUM_FM1_DTSEC
#define TX_PORT_10G_BASE	0x30

enum fm_port_type {
	fm_port_type_oh_e,
	fm_port_type_rx_e,
	fm_port_type_rx_10g_e,
	fm_port_type_tx_e,
	fm_port_type_tx_10g_e
};

/* NIA - next invoked action
 */
#define NIA_ENG_RISC		0x00000000
#define NIA_ENG_MASK		0x007c0000

/* action code
 */
#define NIA_RISC_AC_CC		0x00000006
#define NIA_RISC_AC_IM_TX	0x00000008 /* independent mode Tx */
#define NIA_RISC_AC_IM_RX	0x0000000a /* independent mode Rx */
#define NIA_RISC_AC_HC		0x0000000c

enum fm_block {
	fm_muram_e,
	fm_bmi_e,
	fm_qmi_e,
	fm_parser_e,
	fm_policer_e,
	fm_keygen_e,
	fm_dma_e,
	fm_fpm_e,
	fm_imem_e,
	fm_soft_paser_e,
	fm_mac_e, /* 1gmac */
	fm_10g_mac_e, /* 10gmac */
	fm_mdio_e, /* 1g mdio */
	fm_10g_mdio_e, /* 10g mdio */
	fm_1588_tmr_e
};

/* FM MURAM
 */
struct fm_muram {
	u32 base;
	u32 top;
	u32 size;
	u32 res_size;
	u32 fm;
	u32 alloc;
};
#define FM_MURAM_SIZE		CONFIG_SYS_FM_MURAM_SIZE
#define FM_MURAM_RES_SIZE	0x01000

/* FM IRAM registers
 */
struct fm_iram {
	u32 iadd; /* instruction address register */
	u32 idata; /* instruction data register */
	u32 itcfg; /* timing config register */
	u32 iready; /* ready register */
	u32 res[0x1FFFC];
} __attribute__ ((packed));

#define IRAM_IADD_AIE		0x80000000 /* address auto increase enable */
#define IRAM_READY		0x80000000 /* ready to use */


/* FMDMSR - Fman DMA status register */
#define FMDMSR_CMDQNE		0x10000000 /* command queue not empty */
#define FMDMSR_BER		0x08000000 /* bus error event occurred on bus */
#define FMDMSR_RDB_ECC		0x04000000 /* read buffer ECC error */
#define FMDMSR_WRB_SECC		0x02000000 /* write buffer ECC error on system side */
#define FMDMSR_WRB_FECC		0x01000000 /* write buffer ECC error on Fman side */
#define FMDMSR_DPEXT_SECC	0x00800000 /* dual port external ECC error on system side */
#define FMDMSR_DPEXT_FECC	0x00400000 /* dual port external ECC error on Fman side */
#define FMDMSR_DPDAT_SECC	0x00200000 /* dual port data ECC error on system side */
#define FMDMSR_DPDAT_FECC	0x00100000 /* dual port data ECC error on Fman side */
#define FMDMSR_SPDAT_FECC	0x00080000 /* single port data ECC error on Fman side */

#define FMDMSR_CLEAR_ALL	(FMDMSR_BER | FMDMSR_RDB_ECC \
				| FMDMSR_WRB_SECC | FMDMSR_WRB_FECC \
				| FMDMSR_DPEXT_SECC | FMDMSR_DPEXT_FECC \
				| FMDMSR_DPDAT_SECC | FMDMSR_DPDAT_FECC \
				| FMDMSR_SPDAT_FECC)

/* FMDMMR - FMan DMA mode register
 */
#define FMDMMR_CACHE_OVRD_MASK	0xc0000000 /* override cache field one the command bus */
#define FMDMMR_CACHE_NO_CACHING	0x00000000 /* 00 - no override, no caching */
#define FMDMMR_CACHE_NO_STASH	0x40000000 /* 01 - data should not be stashed in cache */
#define FMDMMR_CACHE_MAY_STASH	0x80000000 /* 10 - data may be stashed in cache */
#define FMDMMR_CACHE_STASH	0xc0000000 /* 11 - data should be stashed in cache */
#define FMDMMR_AID_OVRD		0x20000000 /* AID override, AID='0000' */
#define FMDMMR_SBER		0x10000000 /* stop the DMA transaction if a bus error */
#define FMDMMR_AXI_DBG_MASK	0x0f000000 /* number of beates to be written to external mem */
#define FMDMMR_AXI_DBG_SHIFT	24
#define FMDMMR_ERRD_EN		0x00800000 /* enable read port emergency */
#define FMDMMR_ERWR_EN		0x00400000 /* enable write port emergency */
#define FMDMMR_BER_EN		0x00200000 /* enable external bus error event */
#define FMDMMR_EB_EN		0x00100000 /* enable emergency towards external bus */
#define FMDMMR_ERRD_EME		0x00080000 /* set manual emergency on read port */
#define FMDMMR_ERWR_EME		0x00040000 /* set manual emergency on write port */
#define FMDMMR_EB_EME_MASK	0x00030000 /* priority on external bus */
#define FMDMMR_EB_EME_NORMAL	0x00000000 /* 00 - normal */
#define FMDMMR_EB_EME_EBS	0x00010000 /* 01 - extended bus service */
#define FMDMMR_EB_EME_SOS	0x00020000 /* 10 - SOS priority */
#define FMDMMR_EB_EME_EBSSOS	0x00030000 /* 11 - EBS + SOS priority */
#define FMDMMR_PROT0		0x00001000 /* bus protection - privilege */
#define FMDMMR_PROT2		0x00000400 /* bus protection - instruction */
#define FMDMMR_BMI_EMR		0x00000040 /* SOS emergency is set by BMI */
#define FMDMMR_ECC_MASK		0x00000020 /* enable ECC error events */
#define FMDMMR_AID_TNUM		0x00000010 /* choose the 4 LSB bits of TNUM output on AID */

#define FMDMMR_INIT		(FMDMMR_SBER)

/* FMDMTR - FMan DMA threshold register
 */
#define FMDMTR_DEFAULT		0x18600060 /* high- cmd=24, read/write internal buf=96 */

/* FMDMHY - FMan DMA hysteresis register
 */
#define FMDMHY_DEFAULT		0x10400040 /* low- cmd=16, read/write internal buf=64 */

/* FMDMSETR - FMan DMA SOS emergency threshold register
 */
#define FMDMSETR_DEFAULT	0x00000000

#define FMFPPRC_PORTID_MASK	0x3f000000
#define FMFPPRC_PORTID_SHIFT	24
#define FMFPPRC_ORA_SHIFT	16
#define FMFPPRC_RISC1		0x00000001
#define FMFPPRC_RISC2		0x00000002
#define FMFPPRC_RISC_ALL	(FMFPPRC_RISC1 | FMFPPRC_RSIC2)

#define FMFPFLC_DEFAULT		0x00000000 /* no dispatch limitation */
#define FMFPDIST1_DEFAULT	0x10101010
#define FMFPDIST2_DEFAULT	0x10101010

#define FMRSTC_RESET_FMAN	0x80000000 /* reset entire FMAN and MACs */

/* FMFP_EE - FPM event and enable register
 */
#define FMFPEE_DECC		0x80000000 /* double ECC erorr on FPM ram access */
#define FMFPEE_STL		0x40000000 /* stall of task ... */
#define FMFPEE_SECC		0x20000000 /* single ECC error */
#define FMFPEE_RFM		0x00010000 /* release FMan */
#define FMFPEE_DECC_EN		0x00008000 /* double ECC interrupt enable */
#define FMFPEE_STL_EN		0x00004000 /* stall of task interrupt enable */
#define FMFPEE_SECC_EN		0x00002000 /* single ECC error interrupt enable */
#define FMFPEE_EHM		0x00000008 /* external halt enable */
#define FMFPEE_UEC		0x00000004 /* FMan is not halted */
#define FMFPEE_CER		0x00000002 /* only errornous task stalled */
#define FMFPEE_DER		0x00000001 /* DMA error is just reported */

#define FMFPEE_CLEAR_EVENT	(FMFPEE_DECC | FMFPEE_STL | FMFPEE_SECC | FMFPEE_EHM | FMFPEE_UEC | FMFPEE_CER | FMFPEE_DER | FMFPEE_RFM)

/* FMBM_INIT - BMI initialization register
 */
#define FMBM_INIT_START		0x80000000 /* start to init the internal buf linked list */

/* FMBM_CFG1 - BMI configuration 1
 */
#define FMBM_CFG1_FBPS_MASK	0x03ff0000 /* Free buffer pool size */
#define FMBM_CFG1_FBPS_SHIFT	16
#define FMBM_CFG1_FBPO_MASK	0x000003ff /* Free buffer pool offset */
#define FMBM_CFG1_FBPO_INIT	0x00000010

/* FMBM_CFG2 - BMI configuration 2
 */
#define FMBM_CFG2_TNTSKS_MASK	0x007f0000 /* Total number of task */
#define FMBM_CFG2_TNTSKS_SHIFT	16
#define FMBM_CFG2_TDMA_MASK	0x0000003f /* Total DMA */

#define FMBM_CFG2_INIT		0x00600018 /* max outstanding tasks/dma transfer = 96/24 */

/* FMBM_IEVR - interrupt event
 */
#define FMBM_IEVR_PEC		0x80000000 /* pipeline table ECC error detected */
#define FMBM_IEVR_LEC		0x40000000 /* linked list RAM ECC error */
#define FMBM_IEVR_SEC		0x20000000 /* statistics count RAM ECC error */
#define FMBM_IEVR_CLEAR_ALL	(FMBM_IEVR_PEC | FMBM_IEVR_LEC | FMBM_IEVR_SEC)

/* FMBM_IER - interrupt enable
 */
#define FMBM_IER_PECE		0x80000000 /* PEC interrupt enable */
#define FMBM_IER_LECE		0x40000000 /* LEC interrupt enable */
#define FMBM_IER_SECE		0x20000000 /* SEC interrupt enable */

#define FMBM_IER_DISABLE_ALL	0x00000000

/* FMQM_GC - global configuration
 */
#define FMQM_GC_ENQ_EN		0x80000000 /* enqueue enable */
#define FMQM_GC_DEQ_EN		0x40000000 /* dequeue enable */
#define FMQM_GC_STEN		0x10000000 /* enable global statistic counters */
#define FMQM_GC_ENQ_THR_MASK	0x00003f00 /* max number of enqueue Tnum */
#define FMQM_GC_ENQ_THR_SHIFT	8
#define FMQM_GC_DEQ_THR_MASK	0x0000003f /* max number of dequeue Tnum */

#define FMQM_GC_INIT		0x00003030 /* enqueue/dequeue disable */

/* FMQM_EIE - error interrupt event register
 */
#define FMQM_EIE_DEE		0x80000000 /* double-bit ECC detected */
#define FMQM_EIE_DFUPE		0x40000000 /* dequeue from unkown PortID error */
#define FMQM_EIE_CLEAR_ALL	(FMQM_EIE_DEE | FMQM_EIE_DFUPE)

/* FMQM_EIEN - error interrupt enable register
 */
#define FMQM_EIEN_DEEN		0x80000000 /* double-bit ECC interrupt enable */
#define FMQM_EIEN_DFUPEN	0x40000000 /* dequeue from unkown PortID int enable */
#define FMQM_EIEN_DISABLE_ALL	0x00000000

/* FMQM_IE - interrupt event register
 */
#define FMQM_IE_SEE		0x80000000 /* single-bit ECC error detected */
#define FMQM_IE_CLEAR_ALL	FMQM_IE_SEE

/* FMQM_IEN - interrupt enable register
 */
#define FMQM_IEN_SEE		0x80000000 /* single-bit ECC error interrupt enable */
#define FMQM_IEN_DISABLE_ALL	0x00000000

struct fm_global {
	int fm;			/* 0-FM1, 1-FM2 */
	u32 ucode_ver;		/* microcode version */
	u32 *ucode;		/* microcode */
	u32 ucode_size;
	struct fm_muram muram;	/* muram info structure */
	u32 free_pool_base;	/* Free buffer pool base - FIFO */
	u32 free_pool_size;
};

#if defined(CONFIG_P1017) || defined(CONFIG_P1023)
#define FM_FREE_POOL_SIZE	0x2000 /* 8K bytes */
#else
#define FM_FREE_POOL_SIZE	0x20000 /* 128K bytes */
#endif
#define FM_FREE_POOL_ALIGN	256

u32 fm_get_base_addr(int fm, enum fm_block block, int port);
int fm_get_port_id(enum fm_port_type type, int num);
u32 fm_muram_alloc(struct fm_muram *mem, u32 size, u32 align);
int fm_init_common(int index);
struct fm_global *fm_get_global(int index);

/* Fman ethernet private struct */
typedef struct fm_eth {
	enum fm_port port;
	int fm_index;			/* Fman index */
	u32 num;			/* 0-3: dTSEC0-3 and 4: TGEC */
	int rx_port;			/* Rx port for the ethernet */
	int tx_port;			/* Tx port for the ethernet */
	enum fm_eth_type type;		/* 1G or 10G ethernet */
	phy_interface_t enet_if;
	struct fm_global *fm;		/* Fman global information */
	struct fsl_enet_mac *mac;	/* MAC controller */
	struct mii_dev *bus;
	struct phy_device *phydev;
	int phyaddr;
	struct eth_device *dev;
	int max_rx_len;
	struct fm_port_global_pram *rx_pram; /* Rx parameter table */
	struct fm_port_global_pram *tx_pram; /* Tx parameter table */
	void *rx_bd_ring;		/* Rx BD ring base */
	void *cur_rxbd;			/* current Rx BD */
	void *rx_buf;			/* Rx buffer base */
	void *tx_bd_ring;		/* Tx BD ring base */
	void *cur_txbd;			/* current Tx BD */
} __attribute__ ((packed)) fm_eth_t;

#define RX_BD_RING_SIZE		8
#define TX_BD_RING_SIZE		8
#define MAX_RXBUF_LOG2		11
#define MAX_RXBUF_LEN		(1 << MAX_RXBUF_LOG2)

#endif /* __FM_H__ */
