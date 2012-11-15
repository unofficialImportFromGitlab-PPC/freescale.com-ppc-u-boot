/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <netdev.h>
#include <linux/compiler.h>
#include <asm/mmu.h>
#include <asm/processor.h>
#include <asm/cache.h>
#include <asm/immap_85xx.h>
#include <asm/fsl_law.h>
#include <asm/fsl_serdes.h>
#include <asm/fsl_portals.h>
#include <asm/fsl_liodn.h>
#include <fm_eth.h>

#include "../common/qixis.h"
#include "t1040qds.h"
#include "t1040qds_qixis.h"

#define CLK_MUX_SEL_MASK	0x4
#define ETH_PHY_CLK_OUT		0x4

DECLARE_GLOBAL_DATA_PTR;

int checkboard(void)
{
	char buf[64];
	u8 sw;
	struct cpu_type *cpu = gd->cpu;
	ccsr_gur_t *gur = (void __iomem *)CONFIG_SYS_MPC85xx_GUTS_ADDR;
	unsigned int i;
	static const char *const freq[] = {"100", "125", "156.25", "161.13",
						"122.88", "122.88", "122.88"};
	int clock;

	printf("Board: %sQDS, ", cpu->name);
	printf("Sys ID: 0x%02x, Sys Ver: 0x%02x, ",
		QIXIS_READ(id), QIXIS_READ(arch));

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

	if (sw < 0x8)
		printf("vBank: %d\n", sw);
	else if (sw >= 0x8 && sw <= 0xE)
		puts("NAND\n");
	else
		printf("invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);

	printf("FPGA: v%d (%s), build %d",
		(int)QIXIS_READ(scver), qixis_read_tag(buf),
		(int)qixis_read_minor());
	/* the timestamp string contains "\n" at the end */
	printf(" on %s", qixis_read_time(buf));


	/* Display the RCW, so that no one gets confused as to what RCW
	 * we're actually using for this boot.
	 */
	puts("Reset Configuration Word (RCW):");
	for (i = 0; i < ARRAY_SIZE(gur->rcwsr); i++) {
		u32 rcw = in_be32(&gur->rcwsr[i]);

		if ((i % 4) == 0)
			printf("\n       %08x:", i * 4);
		printf(" %08x", rcw);
	}
	puts("\n");

	/*
	 * Display the actual SERDES reference clocks as configured by the
	 * dip switches on the board.  Note that the SWx registers could
	 * technically be set to force the reference clocks to match the
	 * values that the SERDES expects (or vice versa).  For now, however,
	 * we just display both values and hope the user notices when they
	 * don't match.
	 */
	puts("SERDES Reference Clocks: ");
	sw = QIXIS_READ(brdcfg[2]);
	clock = (sw >> 5) & 7;
	printf("Bank1=%sMHz ", freq[clock]);
	sw = QIXIS_READ(brdcfg[4]);
	clock = (sw >> 6) & 3;
	printf("Bank2=%sMHz\n", freq[clock]);

	return 0;
}

int board_early_init_r(void)
{
	const unsigned int flashbase = CONFIG_SYS_FLASH_BASE;
	const u8 flash_esel = find_tlb_idx((void *)flashbase, 1);

	/*
	 * Remap Boot flash + PROMJET region to caching-inhibited
	 * so that flash can be erased properly.
	 */

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash + promjet */
	disable_tlb(flash_esel);

	set_tlb(1, flashbase, CONFIG_SYS_FLASH_BASE_PHYS,
			MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
			0, flash_esel, BOOKE_PAGESZ_256M, 1);

	set_liodns();
#ifdef CONFIG_SYS_DPAA_QBMAN
	setup_portals();
#endif

	return 0;
}

unsigned long get_board_sys_clk(void)
{
	u8 sysclk_conf = QIXIS_READ(brdcfg[1]);

	switch ((sysclk_conf & 0x0C) >> 2) {
	case QIXIS_CLK_100:
		return 100000000;
	case QIXIS_CLK_125:
		return 125000000;
	case QIXIS_CLK_133:
		return 133333333;
	}
	return 66666666;
}

unsigned long get_board_ddr_clk(void)
{
	u8 ddrclk_conf = QIXIS_READ(brdcfg[1]);

	switch (ddrclk_conf & 0x03) {
	case QIXIS_CLK_100:
		return 100000000;
	case QIXIS_CLK_125:
		return 125000000;
	case QIXIS_CLK_133:
		return 133333333;
	}
	return 66666666;
}

static int serdes_refclock(u8 sw, u8 sdclk)
{
	unsigned int clock;
	u32 ret = -1;
	u8 brdcfg4;

	if (sdclk == 1) {
		brdcfg4 = QIXIS_READ(brdcfg[4]);
		if ((brdcfg4 & CLK_MUX_SEL_MASK) == ETH_PHY_CLK_OUT)
			return SRDS_PLLCR0_RFCK_SEL_125;
		else
			clock = (sw >> 5) & 7;
	} else
		clock = (sw >> 6) & 3;

	switch (clock) {
	case 0:
		ret = SRDS_PLLCR0_RFCK_SEL_100;
		break;
	case 1:
		ret = SRDS_PLLCR0_RFCK_SEL_125;
		break;
	case 2:
		ret = SRDS_PLLCR0_RFCK_SEL_156_25;
		break;
	case 3:
		ret = SRDS_PLLCR0_RFCK_SEL_161_13;
		break;
	case 4:
	case 5:
	case 6:
		ret = SRDS_PLLCR0_RFCK_SEL_122_88;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}
static const char *serdes_clock_to_string(u32 clock)
{
	switch (clock) {
	case SRDS_PLLCR0_RFCK_SEL_100:
		return "100";
	case SRDS_PLLCR0_RFCK_SEL_125:
		return "125";
	case SRDS_PLLCR0_RFCK_SEL_156_25:
		return "156.25";
	case SRDS_PLLCR0_RFCK_SEL_161_13:
		return "161.13";
	default:
		return "122.88";
	}
}

#define NUM_SRDS_BANKS	2

int misc_init_r(void)
{
	u8 sw;
	serdes_corenet_t *srds_regs =
		(void *)CONFIG_SYS_FSL_CORENET_SERDES_ADDR;
	u32 actual[NUM_SRDS_BANKS];
	unsigned int i;
	int clock;

	sw = QIXIS_READ(brdcfg[2]);
	clock = serdes_refclock(sw, 1);
	if (clock >= 0)
		actual[0] = clock;
	else
		printf("Warning: SDREFCLK1 switch setting is unsupported\n");

	sw = QIXIS_READ(brdcfg[4]);
	clock = serdes_refclock(sw, 2);
	if (clock >= 0)
		actual[1] = clock;
	else
		printf("Warning: SDREFCLK2 switch setting unsupported\n");

	for (i = 0; i < NUM_SRDS_BANKS; i++) {
		u32 pllcr0 = srds_regs->bank[i].pllcr0;
		u32 expected = pllcr0 & SRDS_PLLCR0_RFCK_SEL_MASK;
		if (expected != actual[i]) {
			printf("Warning: SERDES bank %u expects reference clock"
			       " %sMHz, but actual is %sMHz\n", i + 1,
			       serdes_clock_to_string(expected),
			       serdes_clock_to_string(actual[i]));
		}
	}

	return 0;
}

void ft_board_setup(void *blob, bd_t *bd)
{
	phys_addr_t base;
	phys_size_t size;

	ft_cpu_setup(blob, bd);

	base = getenv_bootm_low();
	size = getenv_bootm_size();

	fdt_fixup_memory(blob, (u64)base, (u64)size);

#ifdef CONFIG_PCI
	pci_of_setup(blob, bd);
#endif

	fdt_fixup_liodn(blob);

#ifdef CONFIG_HAS_FSL_DR_USB
	fdt_fixup_dr_usb(blob, bd);
#endif

#ifdef CONFIG_SYS_DPAA_FMAN
	fdt_fixup_fman_ethernet(blob);
	fdt_fixup_board_enet(blob);
#endif
}
