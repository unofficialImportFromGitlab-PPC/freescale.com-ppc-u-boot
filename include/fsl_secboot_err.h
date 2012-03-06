/*
 * Copyright 2010-2012 Freescale Semiconductor, Inc.
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

#ifndef _FSL_SECBOOT_ERR_H
#define _FSL_SECBOOT_ERR_H

#define ERROR_ESBC_PAMU_INIT					0x100000
#define ERROR_ESBC_SEC_RESET					0x200000
#define ERROR_ESBC_SEC_INIT					0x400000
#define ERROR_ESBC_SEC_DEQ					0x800000
#define ERROR_ESBC_SEC_DEQ_TO					0x1000000
#define ERROR_ESBC_SEC_ENQ					0x2000000
#define ERROR_ESBC_SEC_JOBQ_STATUS				0x4000000
#define ERROR_ESBC_CLIENT_MAX					0x0

struct fsl_secboot_errcode {
	int errcode;
	const char *name;
};

static const struct fsl_secboot_errcode fsl_secboot_errcodes[] = {
	{ ERROR_ESBC_PAMU_INIT,
		"Error in initializing PAMU"},
	{ ERROR_ESBC_SEC_RESET,
		"Error in resetting Job ring of SEC"},
	{ ERROR_ESBC_SEC_INIT,
		"Error in initializing SEC"},
	{ ERROR_ESBC_SEC_ENQ,
		"Error in enqueue operation by SEC"},
	{ ERROR_ESBC_SEC_DEQ_TO,
		"Dequeue operation by SEC is timed out"},
	{ ERROR_ESBC_SEC_DEQ,
		"Error in dequeue operation by SEC"},
	{ ERROR_ESBC_SEC_JOBQ_STATUS,
		"Error in status of the job submitted to SEC"},
	{ ERROR_ESBC_CLIENT_MAX, "NULL" }
};

void fsl_secboot_handle_error(int error);
#endif
