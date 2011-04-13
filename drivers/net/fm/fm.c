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
#include <malloc.h>
#include <asm/io.h>
#include <asm/errno.h>

#include "fm.h"
#include "../../qe/qe.h"		/* For struct qe_firmware */

#ifdef CONFIG_SYS_QE_FW_IN_NAND
#include <nand.h>
#elif defined(CONFIG_SYS_QE_FW_IN_SPIFLASH)
#include <spi_flash.h>
#elif defined(CONFIG_SYS_QE_FW_IN_MMC)
#include <mmc.h>
#endif

struct fm_global *fman[MAX_NUM_FM];

u32 fm_get_base_addr(int fm, enum fm_block block, int port)
{
	u32 addr = 0;

	if (fm == 0)
		addr = CONFIG_SYS_FSL_FM1_ADDR;
#if (CONFIG_SYS_NUM_FMAN == 2)
	else if (fm == 1)
		addr = CONFIG_SYS_FSL_FM2_ADDR;
#endif

	switch (block) {
	case fm_muram_e: /* muram */
		addr += 0;
		break;
	case fm_bmi_e:
		addr += 0x80000 + port * 0x1000;
		break;
	case fm_qmi_e:
		addr += 0x80400 + port * 0x1000;
		break;
	case fm_parser_e:
		addr += 0x80800 + port * 0x1000;
		break;
	case fm_policer_e:
		addr += 0xc0000;
		break;
	case fm_keygen_e:
		addr += 0xc1000;
		break;
	case fm_dma_e:
		addr += 0xc2000;
		break;
	case fm_fpm_e:
		addr += 0xc3000;
		break;
	case fm_imem_e:
		addr += 0xc4000;
		break;
	case fm_soft_paser_e:
		addr += 0xc7000;
		break;
	case fm_mac_e:
		addr += 0xe0000 + port * 0x2000;
		break;
	case fm_10g_mac_e:
		addr += 0xf0000;
		break;
	case fm_mdio_e:
		addr += 0xe1120 + port * 0x2000;
		break;
	case fm_10g_mdio_e:
		addr += 0xf1000;
		break;
	case fm_1588_tmr_e:
		addr += 0xfe000;
		break;
	}
	return addr;
}

int fm_get_port_id(enum fm_port_type type, int num)
{
	int port_id, base;

	switch (type) {
	case fm_port_type_oh_e:
		base = OH_PORT_ID_BASE;
		break;
	case fm_port_type_rx_e:
		base = RX_PORT_1G_BASE;
		break;
	case fm_port_type_rx_10g_e:
		base = RX_PORT_10G_BASE;
		break;
	case fm_port_type_tx_e:
		base = TX_PORT_1G_BASE;
		break;
	case fm_port_type_tx_10g_e:
		base = TX_PORT_10G_BASE;
		break;
	default:
		base = 0;
		break;
	}

	port_id = base + num;
	return port_id;
}

/** fm_upload_ucode - Fman microcode upload worker function
 *
 *  This function does the actual uploading of an Fman microcode
 *  to an Fman.
 */
static void fm_upload_ucode(int fm, u32 *ucode, unsigned int size)
{
	struct fm_iram *iram;
	unsigned int i;
	unsigned int timeout = 1000000;

	iram = (struct fm_iram *)fm_get_base_addr(fm, fm_imem_e, 0);
	/* enable address auto increase */
	out_be32(&iram->iadd, IRAM_IADD_AIE);
	/* write microcode to IRAM */
	for (i = 0; i < size / 4; i++)
		out_be32(&iram->idata, ucode[i]);

	/* verify if the writing is over */
	out_be32(&iram->iadd, 0);
	while ((in_be32(&iram->idata) != ucode[0]) && --timeout);
	if (!timeout)
		printf("Fman%u: microcode upload timeout\n", fm + 1);

	/* enable microcode from IRAM */
	out_be32(&iram->iready, IRAM_READY);
}

static void fm_init_axi_dma(int fm)
{
	struct fm_dma *axi_dma;
	u32 val;

	axi_dma = (struct fm_dma *)fm_get_base_addr(fm, fm_dma_e, 0);
	/* clear DMA status */
	val = in_be32(&axi_dma->fmdmsr);
	out_be32(&axi_dma->fmdmsr, val | FMDMSR_CLEAR_ALL);
	/* set DMA mode */
	val = in_be32(&axi_dma->fmdmmr);
	out_be32(&axi_dma->fmdmmr, val | FMDMMR_INIT);
	/* set thresholds - high */
	out_be32(&axi_dma->fmdmtr, FMDMTR_DEFAULT);
	/* set hysteresis - low */
	out_be32(&axi_dma->fmdmhy, FMDMHY_DEFAULT);
	/* set emergency threshold */
	out_be32(&axi_dma->fmdmsetr, FMDMSETR_DEFAULT);
}

u32 fm_muram_alloc(struct fm_muram *mem, u32 size, u32 align)
{
	u32 ret;
	u32 align_mask, off;
	u32 save;

	align_mask = align - 1;
	save = mem->alloc;

	if ((off = (save & align_mask)) != 0)
		mem->alloc += (align - off);
	if ((off = size & align_mask) != 0)
		size += (align - off);
	if ((mem->alloc + size) >= mem->top) {
		mem->alloc = save;
		printf("%s: run out of ram.\n", __func__);
	}

	ret = mem->alloc;
	mem->alloc += size;
	memset((void *)ret, 0, size);

	return ret;
}

static void fm_init_muram(int fm, struct fm_muram *mem)
{
	u32 base;

	base = fm_get_base_addr(fm, fm_muram_e, 0);
	mem->fm = fm;
	mem->base = base;
	mem->res_size = FM_MURAM_RES_SIZE;
	mem->size = FM_MURAM_SIZE;
	mem->alloc = base + FM_MURAM_RES_SIZE;
	mem->top = base + FM_MURAM_SIZE;
}

static void fm_reset(int) __attribute__((__unused__));
static void fm_reset(int fm)
{
	struct fm_fpm *fpm;
	fpm = (struct fm_fpm *)fm_get_base_addr(fm, fm_fpm_e, 0);

	/* reset entire FMAN and MACs */
	out_be32(&fpm->fmrstc, FMRSTC_RESET_FMAN);
	udelay(10);
}

static u32 fm_assign_risc(int port_id)
{
	u32 risc_sel, val;
	risc_sel = (port_id & 0x1) ? FMFPPRC_RISC2 : FMFPPRC_RISC1;
	val = (port_id << FMFPPRC_PORTID_SHIFT) & FMFPPRC_PORTID_MASK;
	val |= ((risc_sel << FMFPPRC_ORA_SHIFT) | risc_sel);

	return val;
}

static void fm_init_fpm(int fm)
{
	struct fm_fpm *fpm;
	int i, port_id;
	u32 val;

	fpm = (struct fm_fpm *)fm_get_base_addr(fm, fm_fpm_e, 0);

	val = in_be32(&fpm->fmnee);
	out_be32(&fpm->fmnee, val | 0x0000000f);

	/* IM mode, each even port ID to RISC#1, each odd port ID to RISC#2 */
	/* offline/parser port */
	for (i = 0; i < MAX_NUM_OH_PORT; i++) {
		port_id = fm_get_port_id(fm_port_type_oh_e, i);
		val = fm_assign_risc(port_id);
		out_be32(&fpm->fpmprc, val);
	}
	/* Rx 1G port */
	for (i = 0; i < MAX_NUM_RX_PORT_1G; i++) {
		port_id = fm_get_port_id(fm_port_type_rx_e, i);
		val = fm_assign_risc(port_id);
		out_be32(&fpm->fpmprc, val);
	}
	/* Tx 1G port */
	for (i = 0; i < MAX_NUM_TX_PORT_1G; i++) {
		port_id = fm_get_port_id(fm_port_type_tx_e, i);
		val = fm_assign_risc(port_id);
		out_be32(&fpm->fpmprc, val);
	}
	/* Rx 10G port */
	port_id = fm_get_port_id(fm_port_type_rx_10g_e, 0);
	val = fm_assign_risc(port_id);
	out_be32(&fpm->fpmprc, val);
	/* Tx 10G port */
	port_id = fm_get_port_id(fm_port_type_tx_10g_e, 0);
	val = fm_assign_risc(port_id);
	out_be32(&fpm->fpmprc, val);

	/* disable the dispatch limit in IM case */
	out_be32(&fpm->fpmflc, FMFPFLC_DEFAULT);
	/* set the dispatch thresholds */
	out_be32(&fpm->fpmdis1, FMFPDIST1_DEFAULT);
	out_be32(&fpm->fpmdis2, FMFPDIST2_DEFAULT);
	/* clear events */
	out_be32(&fpm->fmnee, FMFPEE_CLEAR_EVENT);

	/* clear risc events */
	for (i = 0; i < 4; i++)
		out_be32(&fpm->fpmcev[i], 0xffffffff);

	/* clear error */
	out_be32(&fpm->fpmrcr, 0x0000c000);
}

static void fm_init_bmi(int fm, u32 offset, u32 pool_size)
{
	struct fm_bmi_common *bmi;
	int blk, i, port_id;
	u32 val;

	bmi = (struct fm_bmi_common *)fm_get_base_addr(fm, fm_bmi_e, 0);

	/* Need 128KB total free buffer pool size */
	val = offset / 256;
	blk = pool_size / 256;
	/* in IM, we must not begin from offset 0 in MURAM */
	val |= ((blk - 1) << FMBM_CFG1_FBPS_SHIFT);
	out_be32(&bmi->fmbm_cfg1, val);

	/* max outstanding tasks/dma transfer = 96/24 */
	out_be32(&bmi->fmbm_cfg2, FMBM_CFG2_INIT);

	/* disable all BMI interrupt */
	out_be32(&bmi->fmbm_ier, FMBM_IER_DISABLE_ALL);

	/* clear all events */
	out_be32(&bmi->fmbm_ievr, FMBM_IEVR_CLEAR_ALL);

	/* set port parameters - FMBM_PP_x
	 * max tasks 10G Rx/Tx=12, 1G Rx/Tx 4, others is 1
	 * max dma 10G Rx/Tx=3, others is 1
	 * set port FIFO size - FMBM_PFS_x
	 * 4KB for all Rx and Tx ports
	 */
	/* offline/parser port */
	for (i = 0; i < MAX_NUM_OH_PORT; i++) {
		port_id = fm_get_port_id(fm_port_type_oh_e, i);
		/* max tasks=1, max dma=1, no extra */
		out_be32(&bmi->fmbm_pp[port_id - 1], 0);
		/* port FIFO size - 256 bytes, no extra */
		out_be32(&bmi->fmbm_pfs[port_id - 1], 0);
	}
	/* Rx 1G port */
	for (i = 0; i < MAX_NUM_RX_PORT_1G; i++) {
		port_id = fm_get_port_id(fm_port_type_rx_e, i);
		/* max tasks=4, max dma=1, no extra */
		out_be32(&bmi->fmbm_pp[port_id - 1], 0x03000000);
		/* FIFO size - 4KB, no extra */
		out_be32(&bmi->fmbm_pfs[port_id - 1], 0x0000000f);
	}
	/* Tx 1G port FIFO size - 4KB, no extra */
	for (i = 0; i < MAX_NUM_TX_PORT_1G; i++) {
		port_id = fm_get_port_id(fm_port_type_tx_e, i);
		/* max tasks=4, max dma=1, no extra */
		out_be32(&bmi->fmbm_pp[port_id - 1], 0x03000000);
		/* FIFO size - 4KB, no extra */
		out_be32(&bmi->fmbm_pfs[port_id - 1], 0x0000000f);
	}
	/* Rx 10G port */
	port_id = fm_get_port_id(fm_port_type_rx_10g_e, 0);
	/* max tasks=12, max dma=3, no extra */
	out_be32(&bmi->fmbm_pp[port_id - 1], 0x0b000200);
	/* FIFO size - 4KB, no extra */
	out_be32(&bmi->fmbm_pfs[port_id - 1], 0x0000000f);

	/* Tx 10G port */
	port_id = fm_get_port_id(fm_port_type_tx_10g_e, 0);
	/* max tasks=12, max dma=3, no extra */
	out_be32(&bmi->fmbm_pp[port_id - 1], 0x0b000200);
	/* FIFO size - 4KB, no extra */
	out_be32(&bmi->fmbm_pfs[port_id - 1], 0x0000000f);

	/* initialize internal buffers data base (linked list) */
	out_be32(&bmi->fmbm_init, FMBM_INIT_START);
}

static void fm_init_qmi(int fm)
{
	struct fm_qmi_common *qmi;

	qmi = (struct fm_qmi_common *)fm_get_base_addr(fm, fm_qmi_e, 0);

	/* disable enqueue and dequeue of QMI */
	out_be32(&qmi->fmqm_gc, FMQM_GC_INIT);

	/* disable all error interrupts */
	out_be32(&qmi->fmqm_eien, FMQM_EIEN_DISABLE_ALL);
	/* clear all error events */
	out_be32(&qmi->fmqm_eie, FMQM_EIE_CLEAR_ALL);

	/* disable all interrupts */
	out_be32(&qmi->fmqm_ien, FMQM_IEN_DISABLE_ALL);
	/* clear all interrupts */
	out_be32(&qmi->fmqm_ie, FMQM_IE_CLEAR_ALL);
}

struct fm_global *fm_get_global(int index)
{
	return fman[index];
}

/*
 * Upload an Fman firmware
 *
 * This function is similar to qe_upload_firmware(), exception that it uploads
 * a microcode to the Fman instead of the QE.
 *
 * Because the process for uploading a microcode to the Fman is similar for
 * that of the QE, the QE firmware binary format is used for Fman microcode.
 * It should be possible to unify these two functions, but for now we keep them
 * separate.
 */
static int fman_upload_firmware(struct fm_global *fm,
				const struct qe_firmware *firmware)
{
	unsigned int i;
	u32 crc;
	size_t calc_size = sizeof(struct qe_firmware);
	size_t length;
	const struct qe_header *hdr;

	if (!firmware) {
		printf("Fman%u: Invalid address for firmware\n", fm->fm + 1);
		return -EINVAL;
	}

	hdr = &firmware->header;
	length = be32_to_cpu(hdr->length);

	/* Check the magic */
	if ((hdr->magic[0] != 'Q') || (hdr->magic[1] != 'E') ||
		(hdr->magic[2] != 'F')) {
		printf("Fman%u: Data at %p is not a firmware\n", fm->fm + 1,
		       firmware);
		return -EPERM;
	}

	/* Check the version */
	if (hdr->version != 1) {
		printf("Fman%u: Unsupported firmware version %u\n", fm->fm + 1,
		       hdr->version);
		return -EPERM;
	}

	/* Validate some of the fields */
	if ((firmware->count != 1)) {
		printf("Fman%u: Invalid data in firmware header\n", fm->fm + 1);
		return -EINVAL;
	}

	/* Validate the length and check if there's a CRC */
	calc_size += (firmware->count - 1) * sizeof(struct qe_microcode);

	for (i = 0; i < firmware->count; i++)
		/*
		 * For situations where the second RISC uses the same microcode
		 * as the first, the 'code_offset' and 'count' fields will be
		 * zero, so it's okay to add those.
		 */
		calc_size += sizeof(u32) *
			be32_to_cpu(firmware->microcode[i].count);

	/* Validate the length */
	if (length != calc_size + sizeof(u32)) {
		printf("Fman%u: Invalid length in firmware header\n",
		       fm->fm + 1);
		return -EPERM;
	}

	/*
	 * Validate the CRC.  We would normally call crc32_no_comp(), but that
	 * function isn't available unless you turn on JFFS support.
	 */
	crc = be32_to_cpu(*(u32 *)((void *)firmware + calc_size));
	if (crc != (crc32(-1, (const void *)firmware, calc_size) ^ -1)) {
		printf("Fman%u: Firmware CRC is invalid\n", fm->fm + 1);
		return -EIO;
	}

	/* Loop through each microcode. */
	for (i = 0; i < firmware->count; i++) {
		const struct qe_microcode *ucode = &firmware->microcode[i];

		/* Upload a microcode if it's present */
		if (ucode->code_offset) {
			printf("Fman%u: Uploading microcode version %u.%u.%u\n",
			       fm->fm + 1, ucode->major, ucode->minor,
			       ucode->revision);
			fm->ucode = (void *)firmware + ucode->code_offset;
			fm->ucode_size = sizeof(u32) * ucode->count;
			fm->ucode_ver = (ucode->major << 16) |
				(ucode->major << 8) | ucode->revision;
			fm_upload_ucode(fm->fm, fm->ucode, fm->ucode_size);
		}
	}

	return 0;
}

/* Init common part of FM, index is fm num# like fm as above */
int fm_init_common(int index)
{
	struct fm_global *fm;
	struct fm_muram *muram;
	u32 free_pool_offset;
#ifdef CONFIG_SYS_FMAN_FW
	int rc;
	void *addr;
	char env_addr[32];
#endif
#ifdef CONFIG_SYS_QE_FW_IN_NAND
	size_t fw_length = CONFIG_SYS_FMAN_FW_LENGTH;
#endif

	fm = (struct fm_global *)malloc(sizeof(struct fm_global));
	if (!fm) {
		printf("%s: no memory\n", __FUNCTION__);
		return -ENOMEM;
	}
	/* set it to global */
	fman[index] = fm;
	/* zero the fm_global */
	memset(fm, 0, sizeof(struct fm_global));

	/* init muram */
	muram = &fm->muram;
	fm_init_muram(index, muram);

	/* save these information */
	fm->fm = index;

	/* Upload the Fman microcode if it's present */
#ifdef CONFIG_SYS_FMAN_FW
#ifdef CONFIG_SYS_FMAN_FW_ADDR
	addr = (void *)CONFIG_SYS_FMAN_FW_ADDR;
#endif
#ifdef CONFIG_SYS_QE_FW_IN_NAND
	addr = malloc(CONFIG_SYS_FMAN_FW_LENGTH);

	rc = nand_read(&nand_info[0], (loff_t)CONFIG_SYS_QE_FW_IN_NAND,
		       &fw_length, (u_char *)addr);
	if (rc == -EUCLEAN) {
		printf("NAND read of FMAN firmware at offset 0x%x failed %d\n",
			CONFIG_SYS_QE_FW_IN_NAND, rc);
	}
#endif
#ifdef	CONFIG_SYS_QE_FW_IN_SPIFLASH
	struct spi_flash *ucode_flash;
	addr = malloc(CONFIG_SYS_FMAN_FW_LENGTH);
	int ret = 0;

	ucode_flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
			CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
	if (!ucode_flash)
		printf("SF: probe for ucode failed\n");
	else {
		ret = spi_flash_read(ucode_flash, CONFIG_SYS_QE_FW_IN_SPIFLASH,
				CONFIG_SYS_FMAN_FW_LENGTH, addr);
		if (ret)
			printf("SF: read for ucode failed\n");
		spi_flash_free(ucode_flash);
	}
#endif
#ifdef	CONFIG_SYS_QE_FW_IN_MMC
	int dev = CONFIG_SYS_MMC_ENV_DEV;
	addr = malloc(CONFIG_SYS_FMAN_FW_LENGTH);
	u32 cnt = CONFIG_SYS_FMAN_FW_LENGTH / 512;
	u32 n;
	u32 blk = CONFIG_SYS_QE_FW_IN_MMC / 512;
	struct mmc *mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);

	if (!mmc)
		printf("\nMMC cannot find device for ucode\n");
	else {
		printf("\nMMC read: dev # %u, block # %u, count %u ...\n",
				dev, blk, cnt);
		mmc_init(mmc);
		n = mmc->block_dev.block_read(dev, blk, cnt, addr);
		/* flush cache after read */
		flush_cache((ulong)addr, cnt * 512);
	}
#endif
	rc = fman_upload_firmware(fm, addr);
	if (rc)
		return rc;
	sprintf(env_addr, "0x%lx", (long unsigned int)addr);
	setenv("fman_ucode", env_addr);
#endif

	/* alloc free buffer pool in MURAM */
	fm->free_pool_base = fm_muram_alloc(muram, FM_FREE_POOL_SIZE,
				FM_FREE_POOL_ALIGN);
	if (!fm->free_pool_base) {
		printf("%s: no muram for free buffer pool\n", __FUNCTION__);
		return -ENOMEM;
	}
	free_pool_offset = fm->free_pool_base - muram->base;
	fm->free_pool_size = FM_FREE_POOL_SIZE;

	/* init qmi */
	fm_init_qmi(index);
	/* init fpm */
	fm_init_fpm(index);
	/* int axi dma */
	fm_init_axi_dma(index);
	/* init bmi common */
	fm_init_bmi(index, free_pool_offset, fm->free_pool_size);

	return 0;
}

