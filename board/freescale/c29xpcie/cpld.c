/**
 * Copyright 2013 Freescale Semiconductor
 * Author: Mingkai Hu <Mingkai.hu@freescale.com>
 *         Po Liu <Po.Liu@freescale.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This file provides support for the board-specific CPLD used on some Freescale
 * reference boards.
 *
 * The following macros need to be defined:
 *
 * CONFIG_SYS_CPLD_BASE - The virtual address of the base of the
 * CPLD register map
 *
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>

#include "cpld.h"

static u8 __cpld_read(unsigned int reg)
{
	void *p = (void *)CONFIG_SYS_CPLD_BASE;

	return in_8(p + reg);
}

u8 cpld_read(unsigned int reg) __attribute__((weak, alias("__cpld_read")));

static void __cpld_write(unsigned int reg, u8 value)
{
	void *p = (void *)CONFIG_SYS_CPLD_BASE;

	out_8(p + reg, value);
}

void cpld_write(unsigned int reg, u8 value)
	__attribute__((weak, alias("__cpld_write")));

/**
 * Set the boot bank to the alternate bank
 */
void __cpld_set_altbank(u8 banksel)
{
	u8 reg11 = CPLD_READ(flhcsr);

	switch (banksel) {
	case 1:
		CPLD_WRITE(flhcsr, (reg11 & CPLD_BANKSEL_MASK)
			| CPLD_BANKSEL_EN | CPLD_SELECT_BANK1);
		break;
	case 2:
		CPLD_WRITE(flhcsr, (reg11 & CPLD_BANKSEL_MASK)
			| CPLD_BANKSEL_EN | CPLD_SELECT_BANK2);
		break;
	case 3:
		CPLD_WRITE(flhcsr, (reg11 & CPLD_BANKSEL_MASK)
			| CPLD_BANKSEL_EN | CPLD_SELECT_BANK3);
		break;
	case 4:
		CPLD_WRITE(flhcsr, (reg11 & CPLD_BANKSEL_MASK)
			| CPLD_BANKSEL_EN | CPLD_SELECT_BANK4);
		break;
	default:
		printf("Invalid value! [1-4]\n");
		return;
	}

	udelay(100);
	do_reset(NULL, 0, 0, NULL);
}

void cpld_set_altbank(u8 banksel)
	__attribute__((weak, alias("__cpld_set_altbank")));

/**
 * Set the boot bank to the default bank
 */
void __cpld_set_defbank(void)
{
	__cpld_set_altbank(4);
}

void cpld_set_defbank(void)
	__attribute__((weak, alias("__cpld_set_defbank")));

#ifdef DEBUG
static void cpld_dump_regs(void)
{
	printf("chipid1		= 0x%02x\n", CPLD_READ(chipid1));
	printf("chipid2		= 0x%02x\n", CPLD_READ(chipid2));
	printf("hwver		= 0x%02x\n", CPLD_READ(hwver));
	printf("cpldver		= 0x%02x\n", CPLD_READ(cpldver));
	printf("rstcon		= 0x%02x\n", CPLD_READ(rstcon));
	printf("flhcsr		= 0x%02x\n", CPLD_READ(flhcsr));
	printf("wdcsr		= 0x%02x\n", CPLD_READ(wdcsr));
	printf("wdkick		= 0x%02x\n", CPLD_READ(wdkick));
	printf("fancsr		= 0x%02x\n", CPLD_READ(fancsr));
	printf("ledcsr		= 0x%02x\n", CPLD_READ(ledcsr));
	printf("misc		= 0x%02x\n", CPLD_READ(misccsr));
	printf("bootor		= 0x%02x\n", CPLD_READ(bootor));
	printf("bootcfg1	= 0x%02x\n", CPLD_READ(bootcfg1));
	printf("bootcfg2	= 0x%02x\n", CPLD_READ(bootcfg2));
	printf("bootcfg3	= 0x%02x\n", CPLD_READ(bootcfg3));
	printf("bootcfg4	= 0x%02x\n", CPLD_READ(bootcfg4));
	putc('\n');
}
#endif

#ifndef CONFIG_SPL_BUILD
int cpld_cmd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc = 0;
	unsigned char value;

	if (argc <= 1)
		return cmd_usage(cmdtp);

	if (strcmp(argv[1], "reset") == 0) {
		if (!strcmp(argv[2], "altbank") && argv[3]) {
			value = (u8)simple_strtoul(argv[3], NULL, 16);
			cpld_set_altbank(value);
		} else if (!argv[2])
			cpld_set_defbank();
		else
			cmd_usage(cmdtp);
#ifdef DEBUG
	} else if (strcmp(argv[1], "dump") == 0) {
		cpld_dump_regs();
#endif
	} else
		rc = cmd_usage(cmdtp);

	return rc;
}

U_BOOT_CMD(
	cpld_cmd, CONFIG_SYS_MAXARGS, 1, cpld_cmd,
	"Reset the board using the CPLD sequencer",
	"reset - hard reset to default bank 4\n"
	"cpld_cmd reset altbank [bank]- reset to alternate bank\n"
	"	- [bank] bank value select 1-4\n"
	"	- bank 1 on the flash 0x0000000~0x0ffffff\n"
	"	- bank 2 on the flash 0x1000000~0x1ffffff\n"
	"	- bank 3 on the flash 0x2000000~0x2ffffff\n"
	"	- bank 4 on the flash 0x3000000~0x3ffffff\n"
#ifdef DEBUG
	"cpld_cmd dump - display the CPLD registers\n"
#endif
	);
#endif
