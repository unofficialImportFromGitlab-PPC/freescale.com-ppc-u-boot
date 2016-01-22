/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/system.h>
#include <asm/macro.h>

ENTRY(ppa_init)
	/*
	 * x0: PPA entry point
	 * x1: Boot Location Pointer Low
	 * x2: Boot Location Pointer High
	 */

	/* Save stack pointer for EL2 */
	mov	x3, sp
	msr	sp_el2, x3

	/* Set boot loc pointer */
        adr	x4, 1f
        mov	x3, x4
#if defined(CONFIG_FSL_LSCH2)
        rev	w3, w3
#endif
        str	w3, [x1]
        lsr	x3, x4, #32
#if defined(CONFIG_FSL_LSCH2)
        rev	w3, w3
#endif
        str	w3, [x2]

/* Call PPA monitor */
        br	x0

1:
        mov	x0, #0
        ret
ENDPROC(ppa_init)
