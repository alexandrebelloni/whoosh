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

#include "config.h"
#include "types.h"		/* include/asm-arm/types.h */
#include "setup.h"		/* include/asm-arm/setup.h */

#define NAKED		__attribute__((naked))
#define BOOT		__attribute__((section(".boot")))

#if defined (COMMANDLINE)
const char cmdline[] = COMMANDLINE;
#endif


/* Register definitions */
#define URXD  0x0  /* Receiver Register */
#define UTXD  0x40 /* Transmitter Register */
#define UCR1  0x80 /* Control Register 1 */
#define UCR2  0x84 /* Control Register 2 */
#define UCR3  0x88 /* Control Register 3 */
#define UCR4  0x8c /* Control Register 4 */
#define UFCR  0x90 /* FIFO Control Register */
#define USR1  0x94 /* Status Register 1 */
#define USR2  0x98 /* Status Register 2 */
#define UESC  0x9c /* Escape Character Register */
#define UTIM  0xa0 /* Escape Timer Register */
#define UBIR  0xa4 /* BRM Incremental Register */
#define UBMR  0xa8 /* BRM Modulator Register */
#define UBRC  0xac /* Baud Rate Count Register */
#define UTS   0xb4 /* UART Test Register (mx31) */

/* UART Control Register Bit Fields.*/
#define  URXD_CHARRDY    (1<<15)
#define  URXD_ERR        (1<<14)
#define  URXD_OVRRUN     (1<<13)
#define  URXD_FRMERR     (1<<12)
#define  URXD_BRK        (1<<11)
#define  URXD_PRERR      (1<<10)
#define  URXD_RX_DATA    (0xFF)
#define  UCR1_ADEN       (1<<15) /* Auto dectect interrupt */
#define  UCR1_ADBR       (1<<14) /* Auto detect baud rate */
#define  UCR1_TRDYEN     (1<<13) /* Transmitter ready interrupt enable */
#define  UCR1_IDEN       (1<<12) /* Idle condition interrupt */
#define  UCR1_RRDYEN     (1<<9)	 /* Recv ready interrupt enable */
#define  UCR1_RDMAEN     (1<<8)	 /* Recv ready DMA enable */
#define  UCR1_IREN       (1<<7)	 /* Infrared interface enable */
#define  UCR1_TXMPTYEN   (1<<6)	 /* Transimitter empty interrupt enable */
#define  UCR1_RTSDEN     (1<<5)	 /* RTS delta interrupt enable */
#define  UCR1_SNDBRK     (1<<4)	 /* Send break */
#define  UCR1_TDMAEN     (1<<3)	 /* Transmitter ready DMA enable */
#define  UCR1_UARTCLKEN  (1<<2)	 /* UART clock enabled */
#define  UCR1_DOZE       (1<<1)	 /* Doze */
#define  UCR1_UARTEN     (1<<0)	 /* UART enabled */
#define  UCR2_ESCI	 (1<<15) /* Escape seq interrupt enable */
#define  UCR2_IRTS	 (1<<14) /* Ignore RTS pin */
#define  UCR2_CTSC	 (1<<13) /* CTS pin control */
#define  UCR2_CTS        (1<<12) /* Clear to send */
#define  UCR2_ESCEN      (1<<11) /* Escape enable */
#define  UCR2_PREN       (1<<8)  /* Parity enable */
#define  UCR2_PROE       (1<<7)  /* Parity odd/even */
#define  UCR2_STPB       (1<<6)	 /* Stop */
#define  UCR2_WS         (1<<5)	 /* Word size */
#define  UCR2_RTSEN      (1<<4)	 /* Request to send interrupt enable */
#define  UCR2_TXEN       (1<<2)	 /* Transmitter enabled */
#define  UCR2_RXEN       (1<<1)	 /* Receiver enabled */
#define  UCR2_SRST	 (1<<0)	 /* SW reset */
#define  UCR3_DTREN	 (1<<13) /* DTR interrupt enable */
#define  UCR3_PARERREN   (1<<12) /* Parity enable */
#define  UCR3_FRAERREN   (1<<11) /* Frame error interrupt enable */
#define  UCR3_DSR        (1<<10) /* Data set ready */
#define  UCR3_DCD        (1<<9)  /* Data carrier detect */
#define  UCR3_RI         (1<<8)  /* Ring indicator */
#define  UCR3_TIMEOUTEN  (1<<7)  /* Timeout interrupt enable */
#define  UCR3_RXDSEN	 (1<<6)  /* Receive status interrupt enable */
#define  UCR3_AIRINTEN   (1<<5)  /* Async IR wake interrupt enable */
#define  UCR3_AWAKEN	 (1<<4)  /* Async wake interrupt enable */
#define  UCR3_REF25	 (1<<3)  /* Ref freq 25 MHz */
#define  UCR3_REF30	 (1<<2)  /* Ref Freq 30 MHz */
#define  UCR3_INVT	 (1<<1)  /* Inverted Infrared transmission */
#define  UCR3_BPEN	 (1<<0)  /* Preset registers enable */
#define  UCR4_CTSTL_32   (32<<10) /* CTS trigger level (32 chars) */
#define  UCR4_INVR	 (1<<9)  /* Inverted infrared reception */
#define  UCR4_ENIRI	 (1<<8)  /* Serial infrared interrupt enable */
#define  UCR4_WKEN	 (1<<7)  /* Wake interrupt enable */
#define  UCR4_REF16	 (1<<6)  /* Ref freq 16 MHz */
#define  UCR4_IRSC	 (1<<5)  /* IR special case */
#define  UCR4_TCEN	 (1<<3)  /* Transmit complete interrupt enable */
#define  UCR4_BKEN	 (1<<2)  /* Break condition interrupt enable */
#define  UCR4_OREN	 (1<<1)  /* Receiver overrun interrupt enable */
#define  UCR4_DREN	 (1<<0)  /* Recv data ready interrupt enable */
#define  UFCR_RXTL_SHF   0       /* Receiver trigger level shift */
#define  UFCR_RFDIV      (7<<7)  /* Reference freq divider mask */
#define  UFCR_TXTL_SHF   10      /* Transmitter trigger level shift */
#define  USR1_PARITYERR  (1<<15) /* Parity error interrupt flag */
#define  USR1_RTSS	 (1<<14) /* RTS pin status */
#define  USR1_TRDY	 (1<<13) /* Transmitter ready interrupt/dma flag */
#define  USR1_RTSD	 (1<<12) /* RTS delta */
#define  USR1_ESCF	 (1<<11) /* Escape seq interrupt flag */
#define  USR1_FRAMERR    (1<<10) /* Frame error interrupt flag */
#define  USR1_RRDY       (1<<9)	 /* Receiver ready interrupt/dma flag */
#define  USR1_TIMEOUT    (1<<7)	 /* Receive timeout interrupt status */
#define  USR1_RXDS	 (1<<6)	 /* Receiver idle interrupt flag */
#define  USR1_AIRINT	 (1<<5)	 /* Async IR wake interrupt flag */
#define  USR1_AWAKE	 (1<<4)	 /* Aysnc wake interrupt flag */
#define  USR2_ADET	 (1<<15) /* Auto baud rate detect complete */
#define  USR2_TXFE	 (1<<14) /* Transmit buffer FIFO empty */
#define  USR2_DTRF	 (1<<13) /* DTR edge interrupt flag */
#define  USR2_IDLE	 (1<<12) /* Idle condition */
#define  USR2_IRINT	 (1<<8)	 /* Serial infrared interrupt flag */
#define  USR2_WAKE	 (1<<7)	 /* Wake */
#define  USR2_RTSF	 (1<<4)	 /* RTS edge interrupt flag */
#define  USR2_TXDC	 (1<<3)	 /* Transmitter complete */
#define  USR2_BRCD	 (1<<2)	 /* Break condition */
#define  USR2_ORE        (1<<1)	 /* Overrun error */
#define  USR2_RDR        (1<<0)	 /* Recv data ready */
#define  UTS_FRCPERR	 (1<<13) /* Force parity error */
#define  UTS_LOOP        (1<<12) /* Loop tx and rx */
#define  UTS_TXEMPTY	 (1<<6)	 /* TxFIFO empty */
#define  UTS_RXEMPTY	 (1<<5)	 /* RxFIFO empty */
#define  UTS_TXFULL	 (1<<4)	 /* TxFIFO full */
#define  UTS_RXFULL	 (1<<3)	 /* RxFIFO full */
#define  UTS_SOFTRST	 (1<<0)	 /* Software reset */

#define PAD_CTL_HYS		(1 << 16)
#define PAD_CTL_PUS_47K_UP	(1 << 14)
#define PAD_CTL_PUS_100K_UP	(2 << 14)

#define PAD_CTL_PUE		(1 << 13)
#define PAD_CTL_PKE		(1 << 12)
#define PAD_CTL_SPEED_LOW	(1 << 6)
#define PAD_CTL_SPEED_MED	(2 << 6)
#define PAD_CTL_DSE_DISABLE	(0 << 3)
#define PAD_CTL_DSE_240ohm	(1 << 3)
#define PAD_CTL_DSE_120ohm	(2 << 3)
#define PAD_CTL_DSE_80ohm	(3 << 3)
#define PAD_CTL_DSE_60ohm	(4 << 3)
#define PAD_CTL_DSE_48ohm	(5 << 3)
#define PAD_CTL_DSE_40ohm	(6 << 3)
#define PAD_CTL_DSE_34ohm	(7 << 3)
#define PAD_CTL_SRE_FAST	(1 << 0)
#define PAD_CTL_SRE_SLOW	(0 << 0)

#define AIPS2_BASE_ADDR             AIPS2_ON_BASE_ADDR
#define AIPS1_BASE_ADDR             AIPS1_ON_BASE_ADDR
#define AIPS2_ON_BASE_ADDR          (ATZ2_BASE_ADDR + 0x7C000)
#define AIPS1_ON_BASE_ADDR          (ATZ1_BASE_ADDR + 0x7C000)

#define writel(v, a) (*(volatile unsigned int *)(a)) = v
#define __REG(x)     (*((volatile u32 *)(x)))

#define AIPS1_ARB_BASE_ADDR             0x02000000
#define AIPS2_ARB_BASE_ADDR             0x02100000
#define ATZ1_BASE_ADDR              AIPS1_ARB_BASE_ADDR
#define ATZ2_BASE_ADDR              AIPS2_ARB_BASE_ADDR
#define AIPS1_OFF_BASE_ADDR         (ATZ1_BASE_ADDR + 0x80000)
#define ANATOP_BASE_ADDR            (AIPS1_OFF_BASE_ADDR + 0x48000)

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |            \
		PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
		PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define GPT1_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x18000)
#define UART_PHYS  (ATZ1_BASE_ADDR + 0x20000)

/* General purpose timers registers */
struct mxc_gpt {
	unsigned int control;
	unsigned int prescaler;
	unsigned int status;
	unsigned int nouse[6];
	unsigned int counter;
};

static struct mxc_gpt *cur_gpt = (struct mxc_gpt *)GPT1_BASE_ADDR;

struct aipstz_regs {
	u32	mprot0;
	u32	mprot1;
	u32	rsvd[0xe];
	u32	opacr0;
	u32	opacr1;
	u32	opacr2;
	u32	opacr3;
	u32	opacr4;
};

struct anatop_regs {
	u32	pll_sys;		/* 0x000 */
	u32	pll_sys_set;		/* 0x004 */
	u32	pll_sys_clr;		/* 0x008 */
	u32	pll_sys_tog;		/* 0x00c */
	u32	usb1_pll_480_ctrl;	/* 0x010 */
	u32	usb1_pll_480_ctrl_set;	/* 0x014 */
	u32	usb1_pll_480_ctrl_clr;	/* 0x018 */
	u32	usb1_pll_480_ctrl_tog;	/* 0x01c */
	u32	usb2_pll_480_ctrl;	/* 0x020 */
	u32	usb2_pll_480_ctrl_set;	/* 0x024 */
	u32	usb2_pll_480_ctrl_clr;	/* 0x028 */
	u32	usb2_pll_480_ctrl_tog;	/* 0x02c */
	u32	pll_528;		/* 0x030 */
	u32	pll_528_set;		/* 0x034 */
	u32	pll_528_clr;		/* 0x038 */
	u32	pll_528_tog;		/* 0x03c */
	u32	pll_528_ss;		/* 0x040 */
	u32	rsvd0[3];
	u32	pll_528_num;		/* 0x050 */
	u32	rsvd1[3];
	u32	pll_528_denom;		/* 0x060 */
	u32	rsvd2[3];
	u32	pll_audio;		/* 0x070 */
	u32	pll_audio_set;		/* 0x074 */
	u32	pll_audio_clr;		/* 0x078 */
	u32	pll_audio_tog;		/* 0x07c */
	u32	pll_audio_num;		/* 0x080 */
	u32	rsvd3[3];
	u32	pll_audio_denom;	/* 0x090 */
	u32	rsvd4[3];
	u32	pll_video;		/* 0x0a0 */
	u32	pll_video_set;		/* 0x0a4 */
	u32	pll_video_clr;		/* 0x0a8 */
	u32	pll_video_tog;		/* 0x0ac */
	u32	pll_video_num;		/* 0x0b0 */
	u32	rsvd5[3];
	u32	pll_video_denom;	/* 0x0c0 */
	u32	rsvd6[3];
	u32	pll_mlb;		/* 0x0d0 */
	u32	pll_mlb_set;		/* 0x0d4 */
	u32	pll_mlb_clr;		/* 0x0d8 */
	u32	pll_mlb_tog;		/* 0x0dc */
	u32	pll_enet;		/* 0x0e0 */
	u32	pll_enet_set;		/* 0x0e4 */
	u32	pll_enet_clr;		/* 0x0e8 */
	u32	pll_enet_tog;		/* 0x0ec */
	u32	pfd_480;		/* 0x0f0 */
	u32	pfd_480_set;		/* 0x0f4 */
	u32	pfd_480_clr;		/* 0x0f8 */
	u32	pfd_480_tog;		/* 0x0fc */
	u32	pfd_528;		/* 0x100 */
	u32	pfd_528_set;		/* 0x104 */
	u32	pfd_528_clr;		/* 0x108 */
	u32	pfd_528_tog;		/* 0x10c */
	u32	reg_1p1;		/* 0x110 */
	u32	reg_1p1_set;		/* 0x114 */
	u32	reg_1p1_clr;		/* 0x118 */
	u32	reg_1p1_tog;		/* 0x11c */
	u32	reg_3p0;		/* 0x120 */
	u32	reg_3p0_set;		/* 0x124 */
	u32	reg_3p0_clr;		/* 0x128 */
	u32	reg_3p0_tog;		/* 0x12c */
	u32	reg_2p5;		/* 0x130 */
	u32	reg_2p5_set;		/* 0x134 */
	u32	reg_2p5_clr;		/* 0x138 */
	u32	reg_2p5_tog;		/* 0x13c */
	u32	reg_core;		/* 0x140 */
	u32	reg_core_set;		/* 0x144 */
	u32	reg_core_clr;		/* 0x148 */
	u32	reg_core_tog;		/* 0x14c */
	u32	ana_misc0;		/* 0x150 */
	u32	ana_misc0_set;		/* 0x154 */
	u32	ana_misc0_clr;		/* 0x158 */
	u32	ana_misc0_tog;		/* 0x15c */
	u32	ana_misc1;		/* 0x160 */
	u32	ana_misc1_set;		/* 0x164 */
	u32	ana_misc1_clr;		/* 0x168 */
	u32	ana_misc1_tog;		/* 0x16c */
	u32	ana_misc2;		/* 0x170 */
	u32	ana_misc2_set;		/* 0x174 */
	u32	ana_misc2_clr;		/* 0x178 */
	u32	ana_misc2_tog;		/* 0x17c */
	u32	tempsense0;		/* 0x180 */
	u32	tempsense0_set;		/* 0x184 */
	u32	tempsense0_clr;		/* 0x188 */
	u32	tempsense0_tog;		/* 0x18c */
	u32	tempsense1;		/* 0x190 */
	u32	tempsense1_set;		/* 0x194 */
	u32	tempsense1_clr;		/* 0x198 */
	u32	tempsense1_tog;		/* 0x19c */
	u32	usb1_vbus_detect;	/* 0x1a0 */
	u32	usb1_vbus_detect_set;	/* 0x1a4 */
	u32	usb1_vbus_detect_clr;	/* 0x1a8 */
	u32	usb1_vbus_detect_tog;	/* 0x1ac */
	u32	usb1_chrg_detect;	/* 0x1b0 */
	u32	usb1_chrg_detect_set;	/* 0x1b4 */
	u32	usb1_chrg_detect_clr;	/* 0x1b8 */
	u32	usb1_chrg_detect_tog;	/* 0x1bc */
	u32	usb1_vbus_det_stat;	/* 0x1c0 */
	u32	usb1_vbus_det_stat_set;	/* 0x1c4 */
	u32	usb1_vbus_det_stat_clr;	/* 0x1c8 */
	u32	usb1_vbus_det_stat_tog;	/* 0x1cc */
	u32	usb1_chrg_det_stat;	/* 0x1d0 */
	u32	usb1_chrg_det_stat_set;	/* 0x1d4 */
	u32	usb1_chrg_det_stat_clr;	/* 0x1d8 */
	u32	usb1_chrg_det_stat_tog;	/* 0x1dc */
	u32	usb1_loopback;		/* 0x1e0 */
	u32	usb1_loopback_set;	/* 0x1e4 */
	u32	usb1_loopback_clr;	/* 0x1e8 */
	u32	usb1_loopback_tog;	/* 0x1ec */
	u32	usb1_misc;		/* 0x1f0 */
	u32	usb1_misc_set;		/* 0x1f4 */
	u32	usb1_misc_clr;		/* 0x1f8 */
	u32	usb1_misc_tog;		/* 0x1fc */
	u32	usb2_vbus_detect;	/* 0x200 */
	u32	usb2_vbus_detect_set;	/* 0x204 */
	u32	usb2_vbus_detect_clr;	/* 0x208 */
	u32	usb2_vbus_detect_tog;	/* 0x20c */
	u32	usb2_chrg_detect;	/* 0x210 */
	u32	usb2_chrg_detect_set;	/* 0x214 */
	u32	usb2_chrg_detect_clr;	/* 0x218 */
	u32	usb2_chrg_detect_tog;	/* 0x21c */
	u32	usb2_vbus_det_stat;	/* 0x220 */
	u32	usb2_vbus_det_stat_set;	/* 0x224 */
	u32	usb2_vbus_det_stat_clr;	/* 0x228 */
	u32	usb2_vbus_det_stat_tog;	/* 0x22c */
	u32	usb2_chrg_det_stat;	/* 0x230 */
	u32	usb2_chrg_det_stat_set;	/* 0x234 */
	u32	usb2_chrg_det_stat_clr;	/* 0x238 */
	u32	usb2_chrg_det_stat_tog;	/* 0x23c */
	u32	usb2_loopback;		/* 0x240 */
	u32	usb2_loopback_set;	/* 0x244 */
	u32	usb2_loopback_clr;	/* 0x248 */
	u32	usb2_loopback_tog;	/* 0x24c */
	u32	usb2_misc;		/* 0x250 */
	u32	usb2_misc_set;		/* 0x254 */
	u32	usb2_misc_clr;		/* 0x258 */
	u32	usb2_misc_tog;		/* 0x25c */
	u32	digprog;		/* 0x260 */
};

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

#if defined (IXP4XX_SDR_CONFIG)
  *(volatile unsigned long*) 0xcc000000 = IXP4XX_SDR_CONFIG;
#endif

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

  /* General purpose timers bitfields */
#define GPTCR_SWR		(1 << 15)	/* Software reset */
#define GPTCR_FRR		(1 << 9)	/* Freerun / restart */
#define GPTCR_CLKSOURCE_32	(4 << 6)	/* Clock source */
#define GPTCR_TEN		1		/* Timer enable */
#define CLK_32KHZ		32768		/* 32Khz input */

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

	boot (0, MACH_TYPE, PHYS_PARAMS);
}
