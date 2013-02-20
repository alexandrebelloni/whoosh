/* main.c

   written by Marc Singer
   23 Jun 2006

   Copyright (C) 2006 Marc Singer
   Copyright (C) 2012 Adeneo Embedded

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

   ARM kernel bootstrap shim.

   This code may be prepended to a 2.6 Linux kernel in order to setup
   the ATAGS and the machine type in the EXTREME circumstance that the
   platform bootloader cannot be replaced.

   Offset
   ------

   Because we don't know where we're going to be executing, we do a
   little shenanigans to figure out the execution offset.  With it, we
   can reference data that was fixed-up with an absolute address even
   though we are not executing from that address.

   Endianness
   ----------

   This code can be used with either big or little endian kernels, and
   it can be used to switch the endan-ness of the system from the
   boot-up default to the desired orientation for the kernel.
   However, you must swap the bytes of this file and the kernel if you
   are going to write this data to flash.

*/

#define __KERNEL__

#include "types.h"		/* include/asm-arm/types.h */
#include "setup.h"		/* include/asm-arm/setup.h */

#include "config.h"

#define NAKED		__attribute__((naked))
#define BOOT		__attribute__((section(".boot")))

#if defined (COMMANDLINE)
const char cmdline[] = COMMANDLINE;
#endif

void NAKED BOOT boot (u32 r0, u32 r1, u32 r2)
{
	__asm volatile ("nop");	/* Require to get compiler to jump here */
}

int NAKED start (void)
{
	struct tag* p;
	extern unsigned long reloc;
	unsigned long offset = (unsigned long) &reloc;

#if defined (FORCE_BIGENDIAN)

	{
		unsigned long v;
		__asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
				"orr %0, %0, #(1<<7)\n\t" /* Switch to bigendian */
				"mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
	}
#endif

#if defined (FORCE_LITTLEENDIAN)

	{
		unsigned long v;
		__asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
				"bic %0, %0, #(1<<7)\n\t" /* Switch to littleendian */
				"mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
	}
#endif

	__asm volatile ("bl reloc\n\t"
			"reloc: sub %0, lr, %0\n\t"
			".globl reloc\n\t"
			:  "+r" (offset)
			:: "lr", "cc");

#if defined (CREATE_ATAGS) && defined (PHYS_PARAMS)
	p = (struct tag*) PHYS_PARAMS;

	/* Always start with the CORE tag */
	p->hdr.tag		= ATAG_CORE;
	p->hdr.size		= tag_size (tag_core);
	p->u.core.flags	= 0;
	p->u.core.pagesize	= 0;
	p->u.core.rootdev	= 0;
	p = tag_next (p);

	/* Memory tags are always second */
	p->hdr.tag		= ATAG_MEM;
	p->hdr.size		= tag_size (tag_mem32);
	p->u.mem.size		= RAM_BANK0_LENGTH;
	p->u.mem.start	= RAM_BANK0_START;
	p = tag_next (p);

# if defined (RAM_BANK1_START)
	p->hdr.tag		= ATAG_MEM;
	p->hdr.size		= tag_size (tag_mem32);
	p->u.mem.size		= RAM_BANK1_LENGTH;
	p->u.mem.start	= RAM_BANK1_START;
	p = tag_next (p);
# endif

# if defined (INITRD_START)
	p->hdr.tag		= ATAG_INITRD2;
	p->hdr.size		= tag_size (tag_initrd);
	p->u.initrd.start	= INITRD_START;
	p->u.initrd.size	= INITRD_LENGTH;
	p = tag_next (p);
# endif

	/* Command line */
# if defined (COMMANDLINE)
	p->hdr.tag		= ATAG_CMDLINE;
	p->hdr.size		= tag_size (tag_cmdline)
		+ (sizeof (cmdline)+3)/4;
	{
		const char* sz = cmdline + offset;
		int i;
		for (i = 0; i < sizeof (cmdline); ++i)
			p->u.cmdline.cmdline[i] = sz[i];
	}
	p = tag_next (p);
# endif

	/* End */
	p->hdr.tag		= ATAG_NONE;
	p->hdr.size		= 0;

#endif

#if !defined (CREATE_ATAGS) && defined (GUARANTEE_ATAG_CMDLINE)\
	&&  defined (PHYS_PARAMS)  && defined (COMMANDLINE)

	{
		char* cmdlineFound = 0;
		for_each_tag (p, (struct tag*) PHYS_PARAMS)
			if (p->hdr.tag == ATAG_CMDLINE)
				cmdlineFound = p->u.cmdline.cmdline;

		/* The pointer p is left at the start of the terminating
		   ATAG_NONE.  If cmdlineFound is non-null, we've found an
		   instance of the ATAG_CMDLINE.  If not, we append the default
		   command line and a new ATAG_NONE.

		   The original author was concerned about the possibility that
		   there may be more than one ATAG_CMDLINE record.  There is no
		   stipulation in the documentation for the cmdline tags, but it
		   makes sense that there would be only one.  Even if there were
		   more than one, I'm not sure it would matter since we won't
		   append one if we find any. */

		if (!cmdlineFound || !*cmdlineFound) {
			p->hdr.tag = ATAG_CMDLINE;
			p->hdr.size	= tag_size (tag_cmdline)
				+ (sizeof (cmdline)+3)/4 - 1;
			{
				const char* sz = cmdline + offset;
				int i;
				for (i = 0; i < sizeof (cmdline); ++i)
					p->u.cmdline.cmdline[i] = sz[i];
			}
			p = tag_next (p);
			p->hdr.tag = ATAG_NONE;
			p->hdr.size = 0;
		}
	}

#endif

#ifdef MX6
	/*
	 * Setup UART1
	 */
	*(volatile unsigned long*) 0x020E0280 = 0x03;
	*(volatile unsigned long*) 0x020E0650 = UART_PAD_CTRL;
	*(volatile unsigned long*) 0x020E0284 = 0x03;
	*(volatile unsigned long*) 0x020E0920 = 0x01;
	*(volatile unsigned long*) 0x020E0654 = UART_PAD_CTRL;

	struct aipstz_regs *aips1, *aips2;

	aips1 = (struct aipstz_regs *)AIPS1_BASE_ADDR;
	aips2 = (struct aipstz_regs *)AIPS2_BASE_ADDR;

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, &aips1->mprot0);
	writel(0x77777777, &aips1->mprot1);
	writel(0x77777777, &aips2->mprot0);
	writel(0x77777777, &aips2->mprot1);

	/*
	 * Set all OPACRx to be non-bufferable, not require
	 * supervisor privilege level for access,allow for
	 * write access and untrusted master access.
	 */
	writel(0x00000000, &aips1->opacr0);
	writel(0x00000000, &aips1->opacr1);
	writel(0x00000000, &aips1->opacr2);
	writel(0x00000000, &aips1->opacr3);
	writel(0x00000000, &aips1->opacr4);
	writel(0x00000000, &aips2->opacr0);
	writel(0x00000000, &aips2->opacr1);
	writel(0x00000000, &aips2->opacr2);
	writel(0x00000000, &aips2->opacr3);
	writel(0x00000000, &aips2->opacr4);

	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	u32 val, reg = (*(volatile unsigned int *)(&anatop->reg_core));

	val = 500 / 25;

	/*
	 * Mask out the REG_CORE[22:18] bits (REG2_TRIG)
	 * and set them to the calculated value (0.7V + val * 0.25V)
	 */
	reg = (reg & ~(0x1F << 18)) | (val << 18);
	(*(volatile unsigned int *)(&anatop->reg_core)) = reg;

	/* TIMER init */
	/* setup GP Timer 1 */
	writel(GPTCR_SWR, &cur_gpt->control);

	/* We have no udelay by now */
	int i;
	for (i = 0; i < 100; i++)
		writel(0, &cur_gpt->control);

	writel(0, &cur_gpt->prescaler);

	/* Freerun Mode, PERCLK1 input */
	i = (*(volatile unsigned int *)(&cur_gpt->control));
	writel(i | GPTCR_CLKSOURCE_32 | GPTCR_TEN, &cur_gpt->control);

	__REG(UART_PHYS + UCR1) = 0x0;
	__REG(UART_PHYS + UCR2) = 0x0;

	while (!(__REG(UART_PHYS + UCR2) & UCR2_SRST));

	__REG(UART_PHYS + UCR3) = 0x0704;
	__REG(UART_PHYS + UCR4) = 0x8000;
	__REG(UART_PHYS + UESC) = 0x002b;
	__REG(UART_PHYS + UTIM) = 0x0;

	__REG(UART_PHYS + UTS) = 0x0;

	__REG(UART_PHYS + UFCR) = 4 << 7; /* divide input clock by 2 */
	__REG(UART_PHYS + UBIR) = 0xf;
	__REG(UART_PHYS + UBMR) = 80000000 / (2 * 115200);

	__REG(UART_PHYS + UCR2) = UCR2_WS | UCR2_IRTS | UCR2_RXEN | UCR2_TXEN | UCR2_SRST;

	__REG(UART_PHYS + UCR1) = UCR1_UARTEN;
#endif /* MX6 */

#ifdef MX53
	/*
	 * Because the kernel doesn't relock/reconfigure the PLL,
	 * we have to do it in the bootloader. This code is part of
	 * u-boot and @todo should be to do it in the kernel board
	 * specific code.
	 */
	/* init clock */
	struct clkctl* ccm_base = (unsigned long*) CCM_BASE_ADDR;
	volatile unsigned long* arm_base = (unsigned long*) ARM_BASE_ADDR;

	ccm_base->ccgr0 = 0x3FFFFFFF;
	ccm_base->ccgr1 = 0x0;
	ccm_base->ccgr2 = 0x0;
	ccm_base->ccgr3 = 0x0;
	ccm_base->ccgr7 = 0x0;

	ccm_base->ccgr4 = 0x00030000;
	ccm_base->ccgr5 = 0x00FFF030;
	ccm_base->ccgr6 = 0x0F00030F;

	/* Switch ARM to step clock */
	ccm_base->ccsr = 0x4;

	/* setup PLL1 */
	struct dpll* pll1_base = (volatile unsigned long*) PLL1_BASE_ADDR;
	pll1_base->dp_ctl = 0x00001232;
	pll1_base->dp_config = 0x2;

	pll1_base->dp_op = W_DP_OP_800;
	pll1_base->dp_hfs_op = W_DP_OP_800;

	pll1_base->dp_mfd = W_DP_MFD_800;
	pll1_base->dp_hfs_mfd = W_DP_MFD_800;

	pll1_base->dp_mfn = W_DP_MFN_800;
	pll1_base->dp_hfs_mfn = W_DP_MFN_800;

	pll1_base->dp_ctl = 0x00001232;

	volatile unsigned long temp;
	do {
		temp = pll1_base->dp_ctl & 0x1;
	}
	while(temp);

	/* setup PLL3 */
	struct dpll* pll3_base = (volatile unsigned long*) PLL3_BASE_ADDR;
	pll3_base->dp_ctl = 0x00001232;
	pll3_base->dp_config = 0x2;

	pll3_base->dp_op = W_DP_OP_216;
	pll3_base->dp_hfs_op = W_DP_OP_216;

	pll3_base->dp_mfd = W_DP_MFD_216;
	pll3_base->dp_hfs_mfd = W_DP_MFD_216;

	pll3_base->dp_mfn = W_DP_MFN_216;
	pll3_base->dp_hfs_mfn = W_DP_MFN_216;

	pll3_base->dp_ctl = 0x00001232;

	do {
		temp = pll3_base->dp_ctl & 0x1;
	}
	while(temp);

	/* Set the platform clock dividers */
	*(arm_base) = 0x00000725;

	ccm_base->cacrr = 0x0;
	ccm_base->ccsr = 0x0;

	/* Restore the default values in the Gate registers */
	ccm_base->ccgr0 = 0xFFFFFFFF;
	ccm_base->ccgr1 = 0xFFFFFFFF;
	ccm_base->ccgr2 = 0xFFFFFFFF;
	ccm_base->ccgr3 = 0xFFFFFFFF;
	ccm_base->ccgr4 = 0xFFFFFFFF;
	ccm_base->ccgr5 = 0xFFFFFFFF;
	ccm_base->ccgr6 = 0xFFFFFFFF;
	ccm_base->ccgr7 = 0xFFFFFFFF;

	/* Switch peripheral to PLL2 */
	volatile unsigned long val = 0x00808145;
	val |= (2 << 10);
	val |= (1 << 19);
	ccm_base->cbcdr = val;
	ccm_base->cbcmr = 0x00016154;
	val = ccm_base->cscmr1;
	val &= 0xfcffffff;
	val |= 0x01000000;
	ccm_base->cscmr1 = val;

	val = ccm_base->cscdr1;
	val &= 0xffffffc0;
	val |= 0x0a;
	ccm_base->cscdr1 = val;

	/* make sure divider effective */
	while(ccm_base->cdhipr) {}

	ccm_base->ccdr = 0x0;

	/* for cko - for ARM div by 8 */
	val = 0x000A0000;
	val &= 0x00000F0;
	ccm_base->ccosr = val;
#endif /* MX53 */

	boot (0, MACH_TYPE, PHYS_PARAMS);
}
