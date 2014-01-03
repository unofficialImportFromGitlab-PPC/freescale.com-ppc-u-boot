/*
 * Copyright 2013 Freescale Semiconductor, Inc.
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
 *
 */

#ifndef _ASM_FSL_ERRATA_H
#define _ASM_FSL_ERRATA_H

#include <common.h>
#include <asm/processor.h>

#ifdef CONFIG_SYS_FSL_ERRATUM_A006379
static inline bool has_erratum_a006379(void)
{
	u32 svr = get_svr();
	if (((SVR_SOC_VER(svr) == SVR_T4240) && SVR_MAJ(svr) <= 1) ||
	    ((SVR_SOC_VER(svr) == SVR_T4160) && SVR_MAJ(svr) <= 1) ||
	    ((SVR_SOC_VER(svr) == SVR_B4860) && SVR_MAJ(svr) <= 2) ||
	    ((SVR_SOC_VER(svr) == SVR_B4420) && SVR_MAJ(svr) <= 2) ||
	    ((SVR_SOC_VER(svr) == SVR_T2080) && SVR_MAJ(svr) <= 1) ||
	    ((SVR_SOC_VER(svr) == SVR_T2081) && SVR_MAJ(svr) <= 1))
		return true;

	return false;
}
#endif

#endif
