/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <config.h>
#include <errno.h>
#include <malloc.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/macro.h>
#include <asm/arch/soc.h>
#ifdef CONFIG_FSL_LSCH3
#include <asm/arch/immap_lsch3.h>
#elif defined(CONFIG_FSL_LSCH2)
#include <asm/arch/immap_lsch2.h>
#endif
#include <asm/arch/ppa.h>

DECLARE_GLOBAL_DATA_PTR;

extern void c_runtime_cpu_setup(void);

#define LS_PPA_FIT_FIRMWARE_IMAGE	"firmware"
#define LS_PPA_FIT_CNF_NAME		"config@1"
#define PPA_MEM_SIZE_ENV_VAR		"ppamemsize"

/*
 * Return the actual size of the PPA private DRAM block.
 */
unsigned long ppa_get_dram_block_size(void)
{
	unsigned long dram_block_size = CONFIG_SYS_LS_PPA_DRAM_BLOCK_MIN_SIZE;

	char *dram_block_size_env_var = getenv(PPA_MEM_SIZE_ENV_VAR);

	if (dram_block_size_env_var) {
		dram_block_size = simple_strtoul(dram_block_size_env_var, NULL,
						 10);

		if (dram_block_size < CONFIG_SYS_LS_PPA_DRAM_BLOCK_MIN_SIZE) {
			printf("fsl-ppa: WARNING: Invalid value for \'"
			       PPA_MEM_SIZE_ENV_VAR
			       "\' environment variable: %lu\n",
			       dram_block_size);

			dram_block_size = CONFIG_SYS_LS_PPA_DRAM_BLOCK_MIN_SIZE;
		}
	}

	return dram_block_size;
}

/*
 * PPA firmware FIT image parser checks if the image is in FIT
 * format, verifies integrity of the image and calculates raw
 * image address and size values.
 *
 * Returns 0 on success and a negative errno on error task fail.
 */
static int parse_ppa_firmware_fit_image(const void **raw_image_addr,
				size_t *raw_image_size)
{
	const void *ppa_data;
	size_t ppa_size;
	void *fit_hdr;
	int conf_node_off, fw_node_off;
	char *conf_node_name = NULL;

#ifdef CONFIG_SYS_LS_PPA_FW_IN_NOR
	fit_hdr = (void *)CONFIG_SYS_LS_PPA_FW_ADDR;
#else
#error "No CONFIG_SYS_LS_PPA_FW_IN_xxx defined"
#endif

	conf_node_name = LS_PPA_FIT_CNF_NAME;

	if (fdt_check_header(fit_hdr)) {
		printf("fsl-ppa: Bad firmware image (not a FIT image)\n");
		return -EINVAL;
	}

	if (!fit_check_format(fit_hdr)) {
		printf("fsl-ppa: Bad firmware image (bad FIT header)\n");
		return -EINVAL;
	}

	conf_node_off = fit_conf_get_node(fit_hdr, conf_node_name);
	if (conf_node_off < 0) {
		printf("fsl-ppa: %s: no such config\n", conf_node_name);
		return -ENOENT;
	}

	fw_node_off = fit_conf_get_prop_node(fit_hdr, conf_node_off,
			LS_PPA_FIT_FIRMWARE_IMAGE);
	if (fw_node_off < 0) {
		printf("fsl-ppa: No '%s' in config\n",
				LS_PPA_FIT_FIRMWARE_IMAGE);
		return -ENOLINK;
	}

	/* Verify PPA firmware image */
	if (!(fit_image_verify(fit_hdr, fw_node_off))) {
		printf("fsl-ppa: Bad firmware image (bad CRC)\n");
		return -EINVAL;
	}

	if (fit_image_get_data(fit_hdr, fw_node_off, &ppa_data, &ppa_size)) {
		printf("fsl-ppa: Can't get %s subimage data/size",
				LS_PPA_FIT_FIRMWARE_IMAGE);
		return -ENOENT;
	}

	debug("fsl-ppa: raw_image_addr = 0x%p, raw_image_size = 0x%lx\n",
			ppa_data, ppa_size);
	*raw_image_addr = ppa_data;
	*raw_image_size = ppa_size;

	return 0;
}

static int ppa_copy_image(const char *title,
			 u64 image_addr, u32 image_size, u64 ppa_ram_addr)
{
	debug("%s copied to address 0x%p\n", title, (void *)ppa_ram_addr);
	memcpy((void *)ppa_ram_addr, (void *)image_addr, image_size);
	flush_dcache_range(ppa_ram_addr, ppa_ram_addr + image_size);

	return 0;
}

int ppa_init_pre(u64 *entry)
{
	u64 ppa_ram_addr;
	const void *raw_image_addr;
	size_t raw_image_size = 0;
	size_t ppa_ram_size = ppa_get_dram_block_size();
	int ret;

	debug("fsl-ppa: ppa size(0x%lx)\n", ppa_ram_size);

	/*
	 * The PPA must be stored in secure memory.
	 * Append PPA to secure mmu table.
	 */
	ppa_ram_addr = (gd->secure_ram & MEM_RESERVE_SECURE_ADDR_MASK) +
			gd->arch.tlb_size;

	/* Align PPA base address to 4K */
	ppa_ram_addr = (ppa_ram_addr + 0xfff) & ~0xfff;
	debug("fsl-ppa: PPA load address (0x%llx)\n", ppa_ram_addr);

	ret = parse_ppa_firmware_fit_image(&raw_image_addr, &raw_image_size);
	if (ret < 0)
		goto out;

	if (ppa_ram_size < raw_image_size) {
		ret = -ENOSPC;
		goto out;
	}

	ppa_copy_image("PPA firmware", (u64)raw_image_addr,
			raw_image_size, ppa_ram_addr);

	debug("fsl-ppa: PPA entry: 0x%llx\n", ppa_ram_addr);
	*entry = ppa_ram_addr;

	return 0;

out:
	printf("fsl-ppa: error (%d)\n", ret);
	*entry = 0;

	return ret;
}

int ppa_init_entry(void *ppa_entry)
{
	int ret;
	u32 *boot_loc_ptr_l, *boot_loc_ptr_h;

#ifdef CONFIG_FSL_LSCH3
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	boot_loc_ptr_l = &gur->bootlocptrl;
	boot_loc_ptr_h = &gur->bootlocptrh;
#elif defined(CONFIG_FSL_LSCH2)
	struct ccsr_scfg __iomem *scfg = (void *)(CONFIG_SYS_FSL_SCFG_ADDR);
	boot_loc_ptr_l = &scfg->scratchrw[1];
	boot_loc_ptr_h = &scfg->scratchrw[0];
#endif

	debug("fsl-ppa: boot_loc_ptr_l = 0x%p, boot_loc_ptr_h =0x%p\n",
			boot_loc_ptr_l, boot_loc_ptr_h);
	ret = ppa_init(ppa_entry, boot_loc_ptr_l, boot_loc_ptr_h);
	if (ret < 0)
		return ret;

	debug("fsl-ppa: Return from PPA: current_el = %d\n", current_el());

	/*
	 * The PE will be turned into EL2 when run out of PPA.
	 * First, set vector for EL2.
	 */
	c_runtime_cpu_setup();

	/* Enable caches and MMU for EL2. */
	enable_caches();

	return 0;
}
