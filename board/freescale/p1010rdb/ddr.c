/*
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/mmu.h>
#include <asm/immap_85xx.h>
#include <asm/processor.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/io.h>
#include <asm/fsl_law.h>

DECLARE_GLOBAL_DATA_PTR;

#define CONFIG_SYS_DRAM_SIZE	1024

fsl_ddr_cfg_regs_t ddr_cfg_regs_800 = {
	.cs[0].bnds = CONFIG_SYS_DDR_CS0_BNDS,
	.cs[0].config = CONFIG_SYS_DDR_CS0_CONFIG,
	.cs[0].config_2 = CONFIG_SYS_DDR_CS0_CONFIG_2,
	.timing_cfg_3 = CONFIG_SYS_DDR_TIMING_3_800,
	.timing_cfg_0 = CONFIG_SYS_DDR_TIMING_0_800,
	.timing_cfg_1 = CONFIG_SYS_DDR_TIMING_1_800,
	.timing_cfg_2 = CONFIG_SYS_DDR_TIMING_2_800,
	.ddr_sdram_cfg = CONFIG_SYS_DDR_CONTROL,
	.ddr_sdram_cfg_2 = CONFIG_SYS_DDR_CONTROL_2,
	.ddr_sdram_mode = CONFIG_SYS_DDR_MODE_1_800,
	.ddr_sdram_mode_2 = CONFIG_SYS_DDR_MODE_2_800,
	.ddr_sdram_md_cntl = CONFIG_SYS_DDR_MODE_CONTROL,
	.ddr_sdram_interval = CONFIG_SYS_DDR_INTERVAL_800,
	.ddr_data_init = CONFIG_MEM_INIT_VALUE,
	.ddr_sdram_clk_cntl = CONFIG_SYS_DDR_CLK_CTRL_800,
	.ddr_init_addr = CONFIG_SYS_DDR_INIT_ADDR,
	.ddr_init_ext_addr = CONFIG_SYS_DDR_INIT_EXT_ADDR,
	.timing_cfg_4 = CONFIG_SYS_DDR_TIMING_4,
	.timing_cfg_5 = CONFIG_SYS_DDR_TIMING_5,
	.ddr_zq_cntl = CONFIG_SYS_DDR_ZQ_CONTROL,
	.ddr_wrlvl_cntl = CONFIG_SYS_DDR_WRLVL_CONTROL,
	.ddr_sr_cntr = CONFIG_SYS_DDR_SR_CNTR,
	.ddr_sdram_rcw_1 = CONFIG_SYS_DDR_RCW_1,
	.ddr_sdram_rcw_2 = CONFIG_SYS_DDR_RCW_2
};

fsl_ddr_cfg_regs_t ddr_cfg_regs_667 = {
	.cs[0].bnds = CONFIG_SYS_DDR_CS0_BNDS,
	.cs[0].config = CONFIG_SYS_DDR_CS0_CONFIG,
	.cs[0].config_2 = CONFIG_SYS_DDR_CS0_CONFIG_2,
	.timing_cfg_3 = CONFIG_SYS_DDR_TIMING_3_667,
	.timing_cfg_0 = CONFIG_SYS_DDR_TIMING_0_667,
	.timing_cfg_1 = CONFIG_SYS_DDR_TIMING_1_667,
	.timing_cfg_2 = CONFIG_SYS_DDR_TIMING_2_667,
	.ddr_sdram_cfg = CONFIG_SYS_DDR_CONTROL,
	.ddr_sdram_cfg_2 = CONFIG_SYS_DDR_CONTROL_2,
	.ddr_sdram_mode = CONFIG_SYS_DDR_MODE_1_667,
	.ddr_sdram_mode_2 = CONFIG_SYS_DDR_MODE_2_667,
	.ddr_sdram_md_cntl = CONFIG_SYS_DDR_MODE_CONTROL,
	.ddr_sdram_interval = CONFIG_SYS_DDR_INTERVAL_667,
	.ddr_data_init = CONFIG_MEM_INIT_VALUE,
	.ddr_sdram_clk_cntl = CONFIG_SYS_DDR_CLK_CTRL_667,
	.ddr_init_addr = CONFIG_SYS_DDR_INIT_ADDR,
	.ddr_init_ext_addr = CONFIG_SYS_DDR_INIT_EXT_ADDR,
	.timing_cfg_4 = CONFIG_SYS_DDR_TIMING_4,
	.timing_cfg_5 = CONFIG_SYS_DDR_TIMING_5,
	.ddr_zq_cntl = CONFIG_SYS_DDR_ZQ_CONTROL,
	.ddr_wrlvl_cntl = CONFIG_SYS_DDR_WRLVL_CONTROL,
	.ddr_sr_cntr = CONFIG_SYS_DDR_SR_CNTR,
	.ddr_sdram_rcw_1 = CONFIG_SYS_DDR_RCW_1,
	.ddr_sdram_rcw_2 = CONFIG_SYS_DDR_RCW_2
};

fixed_ddr_parm_t fixed_ddr_parm_0[] = {
	{800, 900, &ddr_cfg_regs_800},
	{607, 700, &ddr_cfg_regs_667},
	{0, 0, NULL}
};

unsigned long get_sdram_size(void)
{
	struct cpu_type *cpu;
	phys_size_t ddr_size;

	cpu = gd->cpu;
	/* P1014 and it's derivatives support max 16it DDR width */
	if (cpu->soc_ver == SVR_P1014 || cpu->soc_ver == SVR_P1014_E)
		ddr_size = (CONFIG_SYS_DRAM_SIZE / 2);
	else
		ddr_size = CONFIG_SYS_DRAM_SIZE;

	return ddr_size;
}

/*
 * Fixed sdram init -- doesn't use serial presence detect.
 */
phys_size_t fixed_sdram(void)
{
	int i;
	sys_info_t sysinfo;
	char buf[32];
	fsl_ddr_cfg_regs_t ddr_cfg_regs;
	phys_size_t ddr_size;
	ulong ddr_freq, ddr_freq_mhz;
	struct cpu_type *cpu;

#if defined(CONFIG_SYS_RAMBOOT)
	return CONFIG_SYS_SDRAM_SIZE * 1024 * 1024;
#endif

	ddr_freq = get_ddr_freq(0);
	ddr_freq_mhz = ddr_freq / 1000000;

	get_sys_info(&sysinfo);
	printf("Configuring DDR for %s MT/s data rate\n",
				strmhz(buf, ddr_freq_mhz));

	for (i = 0; fixed_ddr_parm_0[i].max_freq > 0; i++) {
		if ((ddr_freq_mhz > fixed_ddr_parm_0[i].min_freq) &&
		   (ddr_freq_mhz <= fixed_ddr_parm_0[i].max_freq)) {
			memcpy(&ddr_cfg_regs, fixed_ddr_parm_0[i].ddr_settings,
							sizeof(ddr_cfg_regs));
			break;
		}
	}

	if (fixed_ddr_parm_0[i].max_freq == 0)
		panic("Unsupported DDR data rate %s MT/s data rate\n",
					strmhz(buf, ddr_freq_mhz));

	cpu = gd->cpu;
	/* P1014 and it's derivatives support max 16bit DDR width */
	if (cpu->soc_ver == SVR_P1014 || cpu->soc_ver == SVR_P1014_E) {
		ddr_cfg_regs.ddr_sdram_cfg |= SDRAM_CFG_16_BE;
		ddr_cfg_regs.cs[0].bnds = CONFIG_SYS_DDR_CS0_BNDS >> 1;
	}

	ddr_size = (phys_size_t) CONFIG_SYS_SDRAM_SIZE * 1024 * 1024;
	fsl_ddr_set_memctl_regs(&ddr_cfg_regs, 0);

	if (set_ddr_laws(CONFIG_SYS_DDR_SDRAM_BASE, ddr_size,
					LAW_TRGT_IF_DDR_1) < 0) {
		printf("ERROR setting Local Access Windows for DDR\n");
		return 0;
	}

	return ddr_size;
}
