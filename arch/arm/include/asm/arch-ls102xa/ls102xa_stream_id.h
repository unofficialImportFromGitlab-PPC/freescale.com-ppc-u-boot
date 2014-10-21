/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FSL_LS102XA_STREAM_ID_H_
#define __FSL_LS102XA_STREAM_ID_H_
#define CONFIG_SMMU_NSCR_OFFSET		0x400
#define CONFIG_SMMU_SMR_OFFSET		0x800
#define CONFIG_SMMU_S2CR_OFFSET		0xc00

#define SMMU_NSCR_CLIENTPD_SHIFT	0
#define SMMU_NSCR_MTCFG_SHIFT		20

#define SMR_SMR_VALID_SHIFT		31
#define SMR_ID_MASK			0x7fff
#define SMR_MASK_SHIFT			16

#define S2CR_WACFG_SHIFT		22
#define S2CR_WACFG_MASK			0x3
#define S2CR_WACFG_WRITE_ALLOCATE	0x2

#define S2CR_RACFG_SHIFT		20
#define S2CR_RACFG_MASK			0x3
#define S2CR_RACFG_READ_ALLOCATE	0x2

#define S2CR_TYPE_SHIFT			16
#define S2CR_TYPE_MASK			0x3
#define S2CR_TYPE_BYPASS		0x01

#define S2CR_MEM_ATTR_SHIFT		12
#define S2CR_MEM_ATTR_MASK		0xf
#define S2CR_MEM_ATTR_CACHEABLE		0xa

#define S2CR_MTCFG			0x00000800

#define S2CR_SHCFG_SHIFT		8
#define S2CR_SHCFG_MASK			0x3
#define S2CR_SHCFG_OUTER_CACHEABLE	0x1
#define S2CR_SHCFG_INNER_CACHEABLE	0x2

struct smmu_stream_id {
	uint16_t offset;
	uint16_t stream_id;
	char dev_name[32];
};

void ls102xa_config_smmu_stream_id(struct smmu_stream_id *id, uint32_t num);
void ls1021x_config_smmu3(uint32_t liodn);
#endif
