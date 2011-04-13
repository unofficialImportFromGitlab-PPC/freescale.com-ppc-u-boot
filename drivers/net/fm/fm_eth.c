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
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <net.h>
#include <hwconfig.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <miiphy.h>
#include <phy.h>
#include <asm/fsl_serdes.h>

#include "fm.h"
#include "dtsec.h"
#include "tgec.h"

DECLARE_GLOBAL_DATA_PTR;

#define MAXCONTROLLERS	(10)
static struct eth_device *devlist[MAXCONTROLLERS];
static int num_controllers = 0;

struct fm_eth_info fm1_dtsec_info[CONFIG_SYS_NUM_FM1_DTSEC] = {
#if (CONFIG_SYS_NUM_FM1_DTSEC >= 1)
	FM_DTSEC_INFO_INITIALIZER(1, 1),
#endif
#if (CONFIG_SYS_NUM_FM1_DTSEC >= 2)
	FM_DTSEC_INFO_INITIALIZER(1, 2),
#endif
#if (CONFIG_SYS_NUM_FM1_DTSEC >= 3)
	FM_DTSEC_INFO_INITIALIZER(1, 3),
#endif
#if (CONFIG_SYS_NUM_FM1_DTSEC >= 4)
	FM_DTSEC_INFO_INITIALIZER(1, 4),
#endif
#if (CONFIG_SYS_NUM_FM1_DTSEC >= 5)
	FM_DTSEC_INFO_INITIALIZER(1, 5),
#endif
};

#ifdef CONFIG_SYS_NUM_FM1_10GEC
struct fm_eth_info fm1_10gec_info[CONFIG_SYS_NUM_FM1_10GEC] = {
	FM_TGEC_INFO_INITIALIZER(1, 1),
};
#endif

#if (CONFIG_SYS_NUM_FMAN == 2)
struct fm_eth_info fm2_dtsec_info[CONFIG_SYS_NUM_FM2_DTSEC] = {
	FM_DTSEC_INFO_INITIALIZER(2, 1),
	FM_DTSEC_INFO_INITIALIZER(2, 2),
	FM_DTSEC_INFO_INITIALIZER(2, 3),
	FM_DTSEC_INFO_INITIALIZER(2, 4),
};

struct fm_eth_info fm2_10gec_info[CONFIG_SYS_NUM_FM2_10GEC] = {
	FM_TGEC_INFO_INITIALIZER(2, 1),
};
#endif

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII) \
	&& !defined(BITBANGMII)

#define TBIANA_SETTINGS ( \
		TBIANA_ASYMMETRIC_PAUSE \
		| TBIANA_SYMMETRIC_PAUSE \
		| TBIANA_FULL_DUPLEX \
		)

#define TBIANA_SGMII_ACK 0x4001

#define TBICR_SETTINGS ( \
		TBICR_ANEG_ENABLE \
		| TBICR_RESTART_ANEG \
		| TBICR_FULL_DUPLEX \
		| TBICR_SPEED1_SET \
		)

/* Configure the TBI for SGMII operation */
void dtsec_configure_serdes(struct fm_eth *priv)
{
	struct dtsec *regs = priv->mac->base;
	struct tsec_mii_mng *phyregs = priv->mac->phyregs;

	/* Access TBI PHY registers at given TSEC register offset as
	 * opposed to the register offset used for external PHY accesses */
	tsec_local_mdio_write(phyregs, in_be32(&regs->tbipa), 0, TBI_TBICON,
			TBICON_CLK_SELECT);
	tsec_local_mdio_write(phyregs, in_be32(&regs->tbipa), 0, TBI_ANA,
			TBIANA_SGMII_ACK);
	tsec_local_mdio_write(phyregs, in_be32(&regs->tbipa), 0,
			TBI_CR, TBICR_SETTINGS);
}

static void dtsec_init_phy(struct eth_device *dev)
{
	struct fm_eth *fm_eth = dev->priv;
	struct dtsec *regs = (struct dtsec *)fm_eth->mac->base;

	/* Assign a Physical address to the TBI */
	out_be32(&regs->tbipa, CONFIG_SYS_TBIPA_VALUE);

	if (fm_eth->enet_if == PHY_INTERFACE_MODE_SGMII)
		dtsec_configure_serdes(fm_eth);
}

static int tgec_is_fibre(struct eth_device *dev)
{
	struct fm_eth *fm = dev->priv;
	char phyopt[20];

	sprintf(phyopt, "fsl_fm%d_xaui_phy", fm->fm_index + 1);

	return hwconfig_arg_cmp(phyopt, "xfi");
}
#endif

static u16 muram_readw(u16 *addr)
{
	u32 base = (u32)addr & ~0x3;
	u32 val32 = *(u32 *)base;
	int byte_pos;
	u16 ret;

	byte_pos = (u32)addr & 0x3;
	if (byte_pos)
		ret = (u16)(val32 & 0x0000ffff);
	else
		ret = (u16)((val32 & 0xffff0000) >> 16);

	return ret;
}

static void muram_writew(u16 *addr, u16 val)
{
	u32 base = (u32)addr & ~0x3;
	u32 org32 = *(u32 *)base;
	u32 val32;
	int byte_pos;

	byte_pos = (u32)addr & 0x3;
	if (byte_pos)
		val32 = (org32 & 0xffff0000) | val;
	else
		val32 = (org32 & 0x0000ffff) | ((u32)val << 16);

	*(u32 *)base = val32;
}


static void bmi_rx_port_enable(int fm, int port)
{
	struct fm_bmi_rx_port *rx_port;
	u32 val;

	rx_port = (struct fm_bmi_rx_port *)fm_get_base_addr(fm,
					fm_bmi_e, port);
	val = in_be32(&rx_port->fmbm_rcfg);
	val |= FMBM_RCFG_EN;
	out_be32(&rx_port->fmbm_rcfg, val);
}

static void bmi_rx_port_disable(int fm, int port)
{
	struct fm_bmi_rx_port *rx_port;
	u32 val;
	int timeout = 1000000;

	rx_port = (struct fm_bmi_rx_port *)fm_get_base_addr(fm,
					fm_bmi_e, port);
	val = in_be32(&rx_port->fmbm_rcfg);
	val &= ~FMBM_RCFG_EN;
	out_be32(&rx_port->fmbm_rcfg, val);

	/* wait until the rx port is not busy */
	while ((in_be32(&rx_port->fmbm_rst) & FMBM_RST_BSY) && timeout--);
}

static void bmi_rx_port_init(int fm, int port)
{
	struct fm_bmi_rx_port *rx_port;
	rx_port = (struct fm_bmi_rx_port *)fm_get_base_addr(fm,
					fm_bmi_e, port);

	/* set BMI to independent mode, Rx port disable */
	out_be32(&rx_port->fmbm_rcfg, FMBM_RCFG_IM);
	/* set Rx DMA attributes - no swap, no stash/no optimization */
	out_be32(&rx_port->fmbm_rda, FMBM_RDA_INIT);
	/* set Rx FIFO parameters */
	out_be32(&rx_port->fmbm_rfp, FMBM_RFP_DEFAULT);
	/* set Rx frame end parameters */
	out_be32(&rx_port->fmbm_rfed, FMBM_RFED_DEFAULT);
	/* set Rx IC parameters */
	out_be32(&rx_port->fmbm_ricp, FMBM_RICP_DEFAULT);
	/* clear FOF in IM case */
	out_be32(&rx_port->fmbm_rim, 0);
	/* Rx frame next engine -RISC */
	out_be32(&rx_port->fmbm_rfne, NIA_ENG_RISC | NIA_RISC_AC_IM_RX);
	/* Rx command attribute - no order */
	out_be32(&rx_port->fmbm_rfca, FMBM_RFCA_DEFAULT);
	/* enable Rx statistic counters */
	out_be32(&rx_port->fmbm_rstc, FMBM_RSTC_EN);
	/* disable Rx performance counters */
	out_be32(&rx_port->fmbm_rpc, 0);
}

static void bmi_tx_port_enable(int fm, int port)
{
	struct fm_bmi_tx_port *tx_port;
	u32 val;

	tx_port = (struct fm_bmi_tx_port *)fm_get_base_addr(fm,
					fm_bmi_e, port);
	val = in_be32(&tx_port->fmbm_tcfg);
	val |= FMBM_TCFG_EN;
	out_be32(&tx_port->fmbm_tcfg, val);
}

static void bmi_tx_port_disable(int fm, int port)
{
	struct fm_bmi_tx_port *tx_port;
	u32 val;
	int timeout = 1000000;

	tx_port = (struct fm_bmi_tx_port *)fm_get_base_addr(fm,
					fm_bmi_e, port);
	val = in_be32(&tx_port->fmbm_tcfg);
	val &= ~FMBM_TCFG_EN;
	out_be32(&tx_port->fmbm_tcfg, val);

	/* wait until the tx port is not busy */
	while ((in_be32(&tx_port->fmbm_tst) & FMBM_TST_BSY) && timeout--);
}

static void bmi_tx_port_init(int fm, int port)
{
	struct fm_bmi_tx_port *tx_port;

	tx_port = (struct fm_bmi_tx_port *)fm_get_base_addr(fm,
					fm_bmi_e, port);

	/* set BMI to independent mode, Tx port disable */
	out_be32(&tx_port->fmbm_tcfg, FMBM_TCFG_IM);
	/* set Tx DMA attributes - no stash/no swap */
	out_be32(&tx_port->fmbm_tda, FMBM_TDA_INIT);
	/* set Tx FIFO parameters - pipeline depth=1, low comfort 20*256B */
	out_be32(&tx_port->fmbm_tfp, FMBM_TFP_INIT);
	/* set Tx frame margins parameters */
	out_be32(&tx_port->fmbm_tfed, FMBM_TFED_DEFAULT);
	/* set Tx IC parameters */
	out_be32(&tx_port->fmbm_ticp, FMBM_TICP_DEFAULT);
	/* Tx frame next engine -RISC */
	out_be32(&tx_port->fmbm_tfne, NIA_ENG_RISC | NIA_RISC_AC_IM_TX);
	out_be32(&tx_port->fmbm_tfene, NIA_ENG_RISC | NIA_RISC_AC_IM_TX);
	/* Tx command attribute */
	out_be32(&tx_port->fmbm_tfca, FMBM_TFCA_DEFAULT);
	/* enable Tx statistic counters */
	out_be32(&tx_port->fmbm_tstc, FMBM_TSTC_EN);
	/* disable Tx performance counters */
	out_be32(&tx_port->fmbm_tpc, 0);
}

static void fmc_tx_port_graceful_stop_enable(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;

	pram = fm_eth->tx_pram;
	/* graceful stop transmission of frames */
	pram->mode |= PRAM_MODE_GRACEFUL_STOP;
	__asm__ __volatile__ ("sync");
}

static void fmc_tx_port_graceful_stop_disable(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;

	pram = fm_eth->tx_pram;
	/* re-enable transmission of frames */
	pram->mode &= ~PRAM_MODE_GRACEFUL_STOP;
	__asm__ __volatile__ ("sync");
}

static int fm_eth_rx_port_parameter_init(struct fm_eth *fm_eth)
{
	struct fm_global *fm;
	struct fm_muram *muram;
	struct fm_port_global_pram *pram;
	u32 pram_page_offset;
	void *rx_bd_ring_base;
	void *rx_buf_pool;
	struct fm_port_bd *rxbd;
	struct fm_port_qd *rxqd;
	struct fm_bmi_rx_port *bmi_rx_port;
	int i;

	fm = fm_eth->fm;
	muram = &fm->muram;

	/* alloc global parameter ram at MURAM */
	pram = (struct fm_port_global_pram *)fm_muram_alloc(muram,
		FM_PRAM_SIZE, FM_PRAM_ALIGN);
	fm_eth->rx_pram = pram;

	/* parameter page offset to MURAM */
	pram_page_offset = (u32)pram - muram->base;

	/* enable global mode- snooping data buffers and BDs */
	pram->mode = PRAM_MODE_GLOBAL;

	/* init the Rx queue descriptor pionter */
	pram->rxqd_ptr = pram_page_offset + 0x20;

	/* set the max receive buffer length, power of 2 */
	muram_writew(&pram->mrblr, MAX_RXBUF_LOG2);

	/* alloc Rx buffer descriptors from main memory */
	rx_bd_ring_base = malloc(sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);
	if (!rx_bd_ring_base)
		return 0;
	memset(rx_bd_ring_base, 0, sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);

	/* alloc Rx buffer from main memory */
	rx_buf_pool = malloc(MAX_RXBUF_LEN * RX_BD_RING_SIZE);
	if (!rx_buf_pool)
		return 0;
	memset(rx_buf_pool, 0, MAX_RXBUF_LEN * RX_BD_RING_SIZE);

	/* save them to fm_eth */
	fm_eth->rx_bd_ring = rx_bd_ring_base;
	fm_eth->cur_rxbd = rx_bd_ring_base;
	fm_eth->rx_buf = rx_buf_pool;

	/* init Rx BDs ring */
	rxbd = (struct fm_port_bd *)rx_bd_ring_base;
	for (i = 0; i < RX_BD_RING_SIZE; i++) {
		rxbd->status = RxBD_EMPTY;
		rxbd->len = 0;
		rxbd->buf_ptr_hi = 0;
		rxbd->buf_ptr_lo = (u32)rx_buf_pool + i * MAX_RXBUF_LEN;
		rxbd++;
	}

	/* set the Rx queue descriptor */
	rxqd = &pram->rxqd;
	muram_writew(&rxqd->gen, 0);
	muram_writew(&rxqd->bd_ring_base_hi, 0);
	rxqd->bd_ring_base_lo = (u32)rx_bd_ring_base;
	muram_writew(&rxqd->bd_ring_size, sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);
	muram_writew(&rxqd->offset_in, 0);
	muram_writew(&rxqd->offset_out, 0);

	/* set IM parameter ram pointer to Rx Frame Queue ID */
	bmi_rx_port = (struct fm_bmi_rx_port *)fm_get_base_addr(fm->fm,
					fm_bmi_e, fm_eth->rx_port);
	out_be32(&bmi_rx_port->fmbm_rfqid, pram_page_offset);

	return 1;
}

static int fm_eth_tx_port_parameter_init(struct fm_eth *fm_eth)
{
	struct fm_global *fm;
	struct fm_muram *muram;
	struct fm_port_global_pram *pram;
	u32 pram_page_offset;
	void *tx_bd_ring_base;
	struct fm_port_bd *txbd;
	struct fm_port_qd *txqd;
	struct fm_bmi_tx_port *bmi_tx_port;
	int i;

	fm = fm_eth->fm;
	muram = &fm->muram;

	/* alloc global parameter ram at MURAM */
	pram = (struct fm_port_global_pram *)fm_muram_alloc(muram,
		FM_PRAM_SIZE, FM_PRAM_ALIGN);
	fm_eth->tx_pram = pram;

	/* parameter page offset to MURAM */
	pram_page_offset = (u32)pram - muram->base;

	/* enable global mode- snooping data buffers and BDs */
	pram->mode = PRAM_MODE_GLOBAL;

	/* init the Tx queue descriptor pionter */
	pram->txqd_ptr = pram_page_offset + 0x40;

	/* alloc Tx buffer descriptors from main memory */
	tx_bd_ring_base = malloc(sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	if (!tx_bd_ring_base)
		return 0;
	memset(tx_bd_ring_base, 0, sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	/* save it to fm_eth */
	fm_eth->tx_bd_ring = tx_bd_ring_base;
	fm_eth->cur_txbd = tx_bd_ring_base;

	/* init Tx BDs ring */
	txbd = (struct fm_port_bd *)tx_bd_ring_base;
	for (i = 0; i < TX_BD_RING_SIZE; i++) {
		txbd->status = TxBD_LAST;
		txbd->len = 0;
		txbd->buf_ptr_hi = 0;
		txbd->buf_ptr_lo = 0;
	}

	/* set the Tx queue decriptor */
	txqd = &pram->txqd;
	muram_writew(&txqd->bd_ring_base_hi, 0);
	txqd->bd_ring_base_lo = (u32)tx_bd_ring_base;
	muram_writew(&txqd->bd_ring_size, sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	muram_writew(&txqd->offset_in, 0);
	muram_writew(&txqd->offset_out, 0);

	/* set IM parameter ram pointer to Tx Confirmation Frame Queue ID */
	bmi_tx_port = (struct fm_bmi_tx_port *)fm_get_base_addr(fm->fm,
					fm_bmi_e, fm_eth->tx_port);
	out_be32(&bmi_tx_port->fmbm_tcfqid, pram_page_offset);

	return 1;
}

static int fm_eth_init(struct fm_eth *fm_eth)
{

	if (!fm_eth_rx_port_parameter_init(fm_eth))
		return 0;

	if (!fm_eth_tx_port_parameter_init(fm_eth))
		return 0;

	return 1;
}

static int fm_eth_startup(struct fm_eth *fm_eth)
{
	struct fsl_enet_mac *mac;
	mac = fm_eth->mac;

	/* Rx/TxBDs, Rx/TxQDs, Rx buff and parameter ram init */
	if (!fm_eth_init(fm_eth))
		return 0;
	/* setup the MAC controller */
	if (mac->init_mac)
		mac->init_mac(mac);

	/* For some reason we need to set SPEED_100 */
	if ((fm_eth->enet_if == PHY_INTERFACE_MODE_SGMII) && mac->set_if_mode)
		mac->set_if_mode(mac, fm_eth->enet_if, SPEED_100);

	/* init bmi rx port, IM mode and disable */
	bmi_rx_port_init(fm_eth->fm_index, fm_eth->rx_port);
	/* init bmi tx port, IM mode and disable */
	bmi_tx_port_init(fm_eth->fm_index, fm_eth->tx_port);

	return 1;
}

static int fm_eth_open(struct eth_device *dev, bd_t *bd)
{
	struct fm_eth *fm_eth;
	struct fsl_enet_mac *mac;

	fm_eth = (struct fm_eth *)dev->priv;
	mac = fm_eth->mac;

	/* setup the MAC address */
	if (dev->enetaddr[0] & 0x01) {
		printf("%s: MacAddress is multcast address\n",	__func__);
		return 1;
	}
	if (mac->set_mac_addr)
		mac->set_mac_addr(mac, dev->enetaddr);

	/* enable bmi Rx port */
	bmi_rx_port_enable(fm_eth->fm_index, fm_eth->rx_port);
	/* enable MAC rx/tx port */
	if (mac->enable_mac)
		mac->enable_mac(mac);
	/* enable bmi Tx port */
	bmi_tx_port_enable(fm_eth->fm_index, fm_eth->tx_port);
	/* re-enable transmission of frame */
	fmc_tx_port_graceful_stop_disable(fm_eth);

#ifdef CONFIG_PHYLIB
	phy_startup(fm_eth->phydev);
#else
	fm_eth->phydev->speed = SPEED_1000;
	fm_eth->phydev->link = 1;
	fm_eth->phydev->duplex = DUPLEX_FULL;
#endif

	/* set the MAC-PHY mode */
	if (mac->set_if_mode)
		mac->set_if_mode(mac, fm_eth->enet_if, fm_eth->phydev->speed);

	if (!fm_eth->phydev->link)
		printf("%s: No link.\n", fm_eth->phydev->dev->name);

	return fm_eth->phydev->link ? 0 : -1;
}

static void fm_eth_halt(struct eth_device *dev)
{
	struct fm_eth *fm_eth;
	struct fsl_enet_mac *mac;

	fm_eth = (struct fm_eth *)dev->priv;
	mac = fm_eth->mac;

	/* graceful stop the transmission of frames */
	fmc_tx_port_graceful_stop_enable(fm_eth);
	/* disable bmi Tx port */
	bmi_tx_port_disable(fm_eth->fm_index, fm_eth->tx_port);
	/* disable MAC rx/tx port */
	if (mac->disable_mac)
		mac->disable_mac(mac);
	/* disable bmi Rx port */
	bmi_rx_port_disable(fm_eth->fm_index, fm_eth->rx_port);

	phy_shutdown(fm_eth->phydev);
}

static int fm_eth_send(struct eth_device *dev, volatile void *buf, int len)
{
	struct fm_eth *fm_eth;
	struct fm_port_global_pram *pram;
	volatile struct fm_port_bd *txbd, *txbd_base;
	u16 offset_in;
	int i;

	fm_eth = (struct fm_eth *)dev->priv;
	pram = fm_eth->tx_pram;
	txbd = fm_eth->cur_txbd;

	/* find one empty TxBD */
	for (i = 0; txbd->status & TxBD_READY; i++) {
		udelay(100);
		if (i > 0x1000) {
			printf("%s: Tx buffer not ready\n", dev->name);
			return 0;
		}
	}
	/* setup TxBD */
	txbd->buf_ptr_hi = 0;
	txbd->buf_ptr_lo = (u32)buf;
	txbd->len = len;
	__asm__ __volatile__ ("sync");
	txbd->status = TxBD_READY | TxBD_LAST;
	__asm__ __volatile__ ("sync");

	/* update TxQD, let RISC to send the packet */
	offset_in = muram_readw(&pram->txqd.offset_in);
	offset_in += SIZEOFBD;
	if (offset_in >= muram_readw(&pram->txqd.bd_ring_size))
		offset_in = 0;
	muram_writew(&pram->txqd.offset_in, offset_in);
	__asm__ __volatile__ ("sync");

	/* wait for buffer to be transmitted */
	for (i = 0; txbd->status & TxBD_READY; i++) {
		udelay(100);
		if (i > 0x10000) {
			printf("%s: Tx error\n", dev->name);
			return 0;
		}
	}

	/* advance the TxBD */
	txbd++;
	txbd_base = (struct fm_port_bd *)fm_eth->tx_bd_ring;
	if (txbd >= (txbd_base + TX_BD_RING_SIZE))
		txbd = txbd_base;
	/* update current txbd */
	fm_eth->cur_txbd = (void *)txbd;

	return 1;
}

static int fm_eth_recv(struct eth_device *dev)
{
	struct fm_eth *fm_eth;
	struct fm_port_global_pram *pram;
	volatile struct fm_port_bd *rxbd, *rxbd_base;
	u16 status, len;
	u8 *data;
	u16 offset_out;

	fm_eth = (struct fm_eth *)dev->priv;
	pram = fm_eth->rx_pram;
	rxbd = fm_eth->cur_rxbd;
	status = rxbd->status;

	while (!(status & RxBD_EMPTY)) {
		if (!(status & RxBD_ERROR)) {
			data = (u8 *)rxbd->buf_ptr_lo;
			len = rxbd->len;
			NetReceive(data, len);
		} else {
			printf("%s: Rx error\n", dev->name);
			return 0;
		}

		/* clear the RxBDs */
		rxbd->status = RxBD_EMPTY;
		rxbd->len = 0;
		__asm__ __volatile__ ("sync");

		/* advance RxBD */
		rxbd++;
		rxbd_base = (struct fm_port_bd *)fm_eth->rx_bd_ring;
		if (rxbd >= (rxbd_base + RX_BD_RING_SIZE))
			rxbd = rxbd_base;
		/* read next status */
		status = rxbd->status;

		/* update RxQD */
		offset_out = muram_readw(&pram->rxqd.offset_out);
		offset_out += SIZEOFBD;
		if (offset_out >= muram_readw(&pram->rxqd.bd_ring_size))
			offset_out = 0;
		muram_writew(&pram->rxqd.offset_out, offset_out);
		__asm__ __volatile__ ("sync");
	}
	fm_eth->cur_rxbd = (void *)rxbd;

	return 1;
}

static int fm_eth_init_mac(struct fm_eth *fm_eth)
{
	struct fsl_enet_mac *mac;
	int fm, num;
	void *base, *phyregs = NULL;

	fm = fm_eth->fm_index;
	num = fm_eth->num;

	/* Get the mac registers base address */
	if (fm_eth->type == fm_eth_1g_e) {
		base = (void *)fm_get_base_addr(fm, fm_mac_e, num);
		phyregs = (void *)fm_get_base_addr(fm, fm_mdio_e, num);
	} else {
		base = (void *)fm_get_base_addr(fm, fm_10g_mac_e, num);
		phyregs = (void *)fm_get_base_addr(fm, fm_10g_mdio_e, num);
	}

	/* alloc mac controller */
	mac = malloc(sizeof(struct fsl_enet_mac));
	if (!mac)
		return 0;
	memset(mac, 0, sizeof(struct fsl_enet_mac));

	/* save the mac to fm_eth struct */
	fm_eth->mac = mac;

	if (fm_eth->type == fm_eth_1g_e)
		init_dtsec(mac, base, phyregs, MAX_RXBUF_LEN);
	else
		init_tgec(mac, base, phyregs, MAX_RXBUF_LEN);

	return 1;
}

static int init_phy(struct eth_device *dev)
{
	struct fm_eth *fm_eth = dev->priv;
	struct phy_device *phydev = NULL;
	u32 supported;

#ifdef CONFIG_PHYLIB
	if (fm_eth->type == fm_eth_1g_e)
		dtsec_init_phy(dev);

	if (fm_eth->bus) {
		phydev = phy_connect(fm_eth->bus, fm_eth->phyaddr, dev,
					fm_eth->enet_if);
	}

	if (!phydev) {
		printf("Failed to connect\n");
		return -1;
	}

	if (fm_eth->type == fm_eth_1g_e) {
		supported = (SUPPORTED_10baseT_Half |
				SUPPORTED_10baseT_Full |
				SUPPORTED_100baseT_Half |
				SUPPORTED_100baseT_Full |
				SUPPORTED_1000baseT_Full);
	} else {
		supported = SUPPORTED_10000baseT_Full;

		if (tgec_is_fibre(dev))
			phydev->port = PORT_FIBRE;
	}

	phydev->supported &= supported;
	phydev->advertising = phydev->supported;

	fm_eth->phydev = phydev;

	phy_config(phydev);
#endif

	return 0;
}

static int fm_eth_initialize(struct fm_global *fm, struct fm_eth_info *info)
{
	struct eth_device *dev;
	struct fm_eth *fm_eth;
	int rx_port, tx_port;
	int i, num;

	/* alloc eth device */
	dev = (struct eth_device *)malloc(sizeof(struct eth_device));
	if (!dev)
		return 0;
	memset(dev, 0, sizeof(struct eth_device));

	/* alloc the FMan ethernet private struct */
	fm_eth = (struct fm_eth *)malloc(sizeof(struct fm_eth));
	if (!fm_eth)
		return 0;
	memset(fm_eth, 0, sizeof(struct fm_eth));

	/* save the fm global to Fman ethernet struct */
	fm_eth->fm = fm;
	/* save the fm num# to Fman ethernet struct */
	fm_eth->fm_index = fm->fm;
	/* save num to Fman ethenet (0-3: dTSEC, 4: TGEC) */
	num = info->num;
	fm_eth->num = num;
	fm_eth->port = info->port;
	fm_eth->type = info->type;

	/* Rx port and Tx port for the Fman ethernet */
	if (fm_eth->type == fm_eth_1g_e) {
		rx_port = fm_get_port_id(fm_port_type_rx_e, num);
		tx_port = fm_get_port_id(fm_port_type_tx_e, num);
	} else {
		rx_port = fm_get_port_id(fm_port_type_rx_10g_e, 0);
		tx_port = fm_get_port_id(fm_port_type_tx_10g_e, 0);
	}
	fm_eth->rx_port = rx_port;
	fm_eth->tx_port = tx_port;

	/* set the ethernet max receive length */
	fm_eth->max_rx_len = MAX_RXBUF_LEN;

	/* init global mac structure */
	if (!fm_eth_init_mac(fm_eth))
		return 0;

	/* keep same as the manual, we call FMAN1, FMAN2, DTSEC1, DTSEC2, etc */
	if (fm_eth->type == fm_eth_1g_e)
		sprintf(dev->name, "FM%d@DTSEC%d", fm->fm + 1, num + 1);
	else
		sprintf(dev->name, "FM%d@TGEC%d", fm->fm + 1, 1);

	devlist[num_controllers++] = dev;
	dev->iobase = 0;
	dev->priv = (void *)fm_eth;
	dev->init = fm_eth_open;
	dev->halt = fm_eth_halt;
	dev->send = fm_eth_send;
	dev->recv = fm_eth_recv;
	fm_eth->dev = dev;
	fm_eth->bus = info->bus;
	fm_eth->phyaddr = info->phy_addr;
	fm_eth->enet_if = info->enet_if;


	/* startup the FM im */
	if (!fm_eth_startup(fm_eth))
		return 0;

	if (init_phy(dev))
		return 0;

	/* clear the ethernet address */
	for (i = 0; i < 6; i++)
		dev->enetaddr[i] = 0;
	eth_register(dev);

	return 1;
}

int fm_standard_init(bd_t *bis)
{
	int i;
	struct fm_global *fm;

	if (fm_init_common(0))
		return 0;

	fm = fm_get_global(0);

	for (i = 0; i < CONFIG_SYS_NUM_FM1_DTSEC; i++)
		if (fm1_dtsec_info[i].enabled)
			fm_eth_initialize(fm, &fm1_dtsec_info[i]);

#ifdef CONFIG_SYS_NUM_FM1_10GEC
	for (i = 0; i < CONFIG_SYS_NUM_FM1_10GEC; i++)
		if (fm1_10gec_info[i].enabled)
			fm_eth_initialize(fm, &fm1_10gec_info[i]);
#endif

#if (CONFIG_SYS_NUM_FMAN == 2)
	if (fm_init_common(1))
		return 0;

	fm = fm_get_global(1);

	for (i = 0; i < CONFIG_SYS_NUM_FM2_DTSEC; i++)
		if (fm2_dtsec_info[i].enabled)
			fm_eth_initialize(fm, &fm2_dtsec_info[i]);

	for (i = 0; i < CONFIG_SYS_NUM_FM2_10GEC; i++)
		if (fm2_10gec_info[i].enabled)
			fm_eth_initialize(fm, &fm2_10gec_info[i]);
#endif

	return 1;
}

#if defined(CONFIG_P1017) || defined(CONFIG_P1023)
static int is_device_enabled(u32 devdisr_mask)
{
	ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	u32 devdisr = in_be32(&gur->devdisr);

	return !(devdisr_mask & devdisr);
}

/* Init data structures based on HW cfg */
void fman_enet_init(void)
{
	int i;
/* For P1023, only init the two ports for RGMII */
/* xxx: clean this up based on how HW is configured */
	for (i = 0; i < CONFIG_SYS_NUM_FM1_DTSEC; i++) {
		if (is_device_enabled(fm1_dtsec_info[i].devdisr_mask))
			fm1_dtsec_info[i].enabled = 1;
		fm1_dtsec_info[i].enet_if = PHY_INTERFACE_MODE_RGMII;
	}

	if (is_serdes_configured(SGMII_FM1_DTSEC1))
		fm1_dtsec_info[0].enet_if = PHY_INTERFACE_MODE_SGMII;

	if (is_serdes_configured(SGMII_FM1_DTSEC2))
		fm1_dtsec_info[1].enet_if = PHY_INTERFACE_MODE_SGMII;

	return ;
}
#else

/*
 * Returns true if the given Ethernet device (as specified by it's DEVDISR
 * mask) is enabled (i.e. not disabled in DEVDISR2)
 *
 * This function is called only for SERDES-based devices, because devices are
 * disabled in DEVDISR2 based only on the SERDES configuration and SERDES
 * errata.  If a device is configured for RGMII, we assume it's enabled.
 */
static int is_device_enabled(u32 devdisr_mask)
{
	ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	u32 devdisr2 = in_be32(&gur->devdisr2);

	return !(devdisr_mask & devdisr2);
}

/* Init data structures based on HW cfg */
void fman_enet_init(void)
{
	int i;
	ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	u32 rcwsr11 = in_be32(&gur->rcwsr[11]);

	for (i = 0; i < CONFIG_SYS_NUM_FM1_DTSEC; i++) {
		if (is_device_enabled(fm1_dtsec_info[i].devdisr_mask)) {
			fm1_dtsec_info[i].enabled = 1;
			fm1_dtsec_info[i].enet_if = PHY_INTERFACE_MODE_SGMII;
		} else {
			fm1_dtsec_info[i].enabled = 0;
		}
	}
#ifdef CONFIG_SYS_NUM_FM1_10GEC
	for (i = 0; i < CONFIG_SYS_NUM_FM1_10GEC; i++) {
		if (is_device_enabled(fm1_10gec_info[i].devdisr_mask)) {
			fm1_10gec_info[i].enabled = 1;
			fm1_10gec_info[i].enet_if = PHY_INTERFACE_MODE_XGMII;
		} else {
			fm1_10gec_info[i].enabled = 0;
		}
		if (!is_serdes_configured(XAUI_FM1))
			fm1_10gec_info[i].enabled = 0;
	}
#endif

#ifdef CONFIG_PPC_P4080
	if (!is_serdes_configured(SGMII_FM1_DTSEC1))
		fm1_dtsec_info[0].enabled = 0;

	if ((rcwsr11 & FSL_CORENET_RCWSR11_EC1) ==
		FSL_CORENET_RCWSR11_EC1_FM1_DTSEC1) {
		fm1_dtsec_info[0].enabled = 1;
		fm1_dtsec_info[0].enet_if = PHY_INTERFACE_MODE_RGMII;
	}

	if ((rcwsr11 & FSL_CORENET_RCWSR11_EC2) ==
		FSL_CORENET_RCWSR11_EC2_FM1_DTSEC2) {
		fm1_dtsec_info[1].enabled = 1;
		fm1_dtsec_info[1].enet_if = PHY_INTERFACE_MODE_RGMII;
	}
#endif
#if defined(CONFIG_PPC_P2040) || defined(CONFIG_PPC_P3041) \
	|| defined(CONFIG_PPC_P5020)

	/* EC1 - controls DTSEC4 */
	switch (rcwsr11 & FSL_CORENET_RCWSR11_EC1) {
	case FSL_CORENET_RCWSR11_EC1_FM1_DTSEC4_RGMII:
		fm1_dtsec_info[3].enabled = 1;
		fm1_dtsec_info[3].enet_if = PHY_INTERFACE_MODE_RGMII;
		break;
	case FSL_CORENET_RCWSR11_EC1_FM1_DTSEC4_MII:
		fm1_dtsec_info[3].enabled = 1;
		fm1_dtsec_info[3].enet_if = PHY_INTERFACE_MODE_MII;
		break;
	}

	/* EC2 - controls DTSEC5 */
	/* We use the P4080 mask for EC2 even though bit 365 is reserved on p3041/p5020 */
	switch (rcwsr11 & FSL_CORENET_RCWSR11_EC2) {
	case FSL_CORENET_RCWSR11_EC2_FM1_DTSEC5_RGMII:
		fm1_dtsec_info[4].enabled = 1;
		fm1_dtsec_info[4].enet_if = PHY_INTERFACE_MODE_RGMII;
		break;
	case FSL_CORENET_RCWSR11_EC2_FM1_DTSEC5_MII:
		fm1_dtsec_info[4].enabled = 1;
		fm1_dtsec_info[4].enet_if = PHY_INTERFACE_MODE_MII;
		break;
	}
#endif

#if (CONFIG_SYS_NUM_FMAN == 2)
	for (i = 0; i < CONFIG_SYS_NUM_FM2_DTSEC; i++) {
		if (is_device_enabled(fm2_dtsec_info[i].devdisr_mask)) {
			fm2_dtsec_info[i].enabled = 1;
			fm2_dtsec_info[i].enet_if = PHY_INTERFACE_MODE_SGMII;
		} else {
			fm2_dtsec_info[i].enabled = 0;
		}
	}
	for (i = 0; i < CONFIG_SYS_NUM_FM2_10GEC; i++) {
		if (is_device_enabled(fm2_10gec_info[i].devdisr_mask)) {
			fm2_10gec_info[i].enabled = 1;
			fm2_10gec_info[i].enet_if = PHY_INTERFACE_MODE_XGMII;
		} else {
			fm2_10gec_info[i].enabled = 0;
		}
		if (!is_serdes_configured(XAUI_FM2))
			fm2_10gec_info[i].enabled = 0;
	}

#ifdef CONFIG_PPC_P4080
	if ((rcwsr11 & FSL_CORENET_RCWSR11_EC2) ==
		FSL_CORENET_RCWSR11_EC2_FM2_DTSEC1) {
		fm2_dtsec_info[0].enabled = 1;
		fm2_dtsec_info[0].enet_if = PHY_INTERFACE_MODE_RGMII;
	}
#endif
#endif

	return ;
}
#endif

void fm_info_set_mdio(enum fm_port port, struct mii_dev *bus)
{
	switch (port) {
	case FM1_DTSEC1:
	case FM1_DTSEC2:
	case FM1_DTSEC3:
	case FM1_DTSEC4:
	case FM1_DTSEC5:
		fm1_dtsec_info[port - FM1_DTSEC1].bus = bus;
		break;
#ifdef CONFIG_SYS_NUM_FM1_10GEC
	case FM1_10GEC1:
		fm1_10gec_info[port - FM1_10GEC1].bus = bus;
		break;
#endif
#if (CONFIG_SYS_NUM_FMAN == 2)
	case FM2_DTSEC1:
	case FM2_DTSEC2:
	case FM2_DTSEC3:
	case FM2_DTSEC4:
		fm2_dtsec_info[port - FM2_DTSEC1].bus = bus;
		break;
	case FM2_10GEC1:
		fm2_10gec_info[port - FM2_10GEC1].bus = bus;
		break;
#endif
	default:
		break;
	};
}

void fm_info_set_phy_address(enum fm_port port, int address)
{
	switch (port) {
	case FM1_DTSEC1:
	case FM1_DTSEC2:
	case FM1_DTSEC3:
	case FM1_DTSEC4:
	case FM1_DTSEC5:
		fm1_dtsec_info[port - FM1_DTSEC1].phy_addr = address;
		break;
#ifdef CONFIG_SYS_NUM_FM1_10GEC
	case FM1_10GEC1:
		fm1_10gec_info[port - FM1_10GEC1].phy_addr = address;
		break;
#endif
#if (CONFIG_SYS_NUM_FMAN == 2)
	case FM2_DTSEC1:
	case FM2_DTSEC2:
	case FM2_DTSEC3:
	case FM2_DTSEC4:
		fm2_dtsec_info[port - FM2_DTSEC1].phy_addr = address;
		break;
	case FM2_10GEC1:
		fm2_10gec_info[port - FM2_10GEC1].phy_addr = address;
		break;
#endif
	default:
		break;
	};
}

/*
 * Returns the type of the data interface between the given MAC and its PHY.
 * This is typically determined by the RCW.
 */
phy_interface_t fm_info_get_enet_if(enum fm_port port)
{
	switch (port) {
	case FM1_DTSEC1:
	case FM1_DTSEC2:
	case FM1_DTSEC3:
	case FM1_DTSEC4:
	case FM1_DTSEC5:
		if (fm1_dtsec_info[port - FM1_DTSEC1].enabled)
			return fm1_dtsec_info[port - FM1_DTSEC1].enet_if;
		break;
#ifdef CONFIG_SYS_NUM_FM1_10GEC
	case FM1_10GEC1:
		if (fm1_10gec_info[port - FM1_10GEC1].enabled)
			return fm1_10gec_info[port - FM1_10GEC1].enet_if;
		break;
#endif
#if (CONFIG_SYS_NUM_FMAN == 2)
	case FM2_DTSEC1:
	case FM2_DTSEC2:
	case FM2_DTSEC3:
	case FM2_DTSEC4:
		if (fm2_dtsec_info[port - FM2_DTSEC1].enabled)
			return fm2_dtsec_info[port - FM2_DTSEC1].enet_if;
		break;
	case FM2_10GEC1:
		if (fm2_10gec_info[port - FM2_10GEC1].enabled)
			return fm2_10gec_info[port - FM2_10GEC1].enet_if;
		break;
#endif
	default:
		break;
	};

	/* Not clear how this is possible */
	return PHY_INTERFACE_MODE_NONE;
}

static void
__def_board_ft_fman_fixup_port(void *blob, char * prop, phys_addr_t pa,
				enum fm_port port, int offset)
{
	return ;
}

void board_ft_fman_fixup_port(void *blob, char * prop, phys_addr_t pa,
				enum fm_port port, int offset)
	 __attribute__((weak, alias("__def_board_ft_fman_fixup_port")));

static void ft_fman_fixup_port(void *blob, struct fm_eth_info *info, char *prop)
{
	int off, ph;
	phys_addr_t paddr = CONFIG_SYS_CCSRBAR_PHYS + info->compat_offset;

	off = fdt_node_offset_by_compat_reg(blob, prop, paddr);

	if (info->enabled) {
		fdt_fixup_phy_connection(blob, off, info->enet_if);
		board_ft_fman_fixup_port(blob, prop, paddr, info->port, off);
		return ;
	}

	/* board code might have caused offset to change */
	off = fdt_node_offset_by_compat_reg(blob, prop, paddr);

	/* disable both the mac node and the node that has a handle to it */
	fdt_setprop_string(blob, off, "status", "disabled");

	ph = fdt_get_phandle(blob, off);
	do_fixup_by_prop(blob, "fsl,fman-mac", &ph, sizeof(ph),
		"status", "disabled", strlen("disabled") + 1, 1);
}

void fdt_fixup_fman_ethernet(void *blob)
{
	int i;

	for (i = 0; i < CONFIG_SYS_NUM_FM1_DTSEC; i++)
		ft_fman_fixup_port(blob, &fm1_dtsec_info[i], "fsl,fman-1g-mac");
#ifdef CONFIG_SYS_NUM_FM1_10GEC
	for (i = 0; i < CONFIG_SYS_NUM_FM1_10GEC; i++)
		ft_fman_fixup_port(blob, &fm1_10gec_info[i], "fsl,fman-10g-mac");
#endif

#if (CONFIG_SYS_NUM_FMAN == 2)
	for (i = 0; i < CONFIG_SYS_NUM_FM2_DTSEC; i++)
		ft_fman_fixup_port(blob, &fm2_dtsec_info[i], "fsl,fman-1g-mac");
	for (i = 0; i < CONFIG_SYS_NUM_FM2_10GEC; i++)
		ft_fman_fixup_port(blob, &fm2_10gec_info[i], "fsl,fman-10g-mac");
#endif
}
