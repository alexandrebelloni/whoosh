#define MX53

#include "types.h"		/* include/asm-arm/types.h */
#include "setup.h"		/* include/asm-arm/setup.h */

#define AIPS1_BASE_ADDR		0x53F00000
#define AIPS1_MPROT			0x53F00004
#define AIPS2_BASE_ADDR		0x63F00000
#define AIPS2_MPROT         0x63F00004
#define CCM_BASE_ADDR       (AIPS1_BASE_ADDR + 0x000D4000)
#define ARM_BASE_ADDR       (AIPS2_BASE_ADDR + 0x000A0014)

#define PLL1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00080000)
#define PLL3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00088000)

#define W_DP_OP_800		((8 << 4) + ((1 - 1)  << 0))
#define W_DP_MFD_800	(3 - 1)
#define W_DP_MFN_800	1
#define W_DP_OP_216		((6 << 4) + ((3 - 1)  << 0))
#define W_DP_MFD_216	(4 - 1)
#define W_DP_MFN_216	3

struct clkctl {
	volatile u32	ccr;
	volatile u32	ccdr;
	volatile u32	csr;
	volatile u32	ccsr;
	volatile u32	cacrr;
	volatile u32	cbcdr;
	volatile u32	cbcmr;
	volatile u32	cscmr1;
	volatile u32	cscmr2;
	volatile u32	cscdr1;
	volatile u32	cs1cdr;
	volatile u32	cs2cdr;
	volatile u32	cdcdr;
	volatile u32	chsccdr;
	volatile u32	cscdr2;
	volatile u32	cscdr3;
	volatile u32	cscdr4;
	volatile u32	cwdr;
	volatile u32	cdhipr;
	volatile u32	cdcr;
	volatile u32	ctor;
	volatile u32	clpcr;
	volatile u32	cisr;
	volatile u32	cimr;
	volatile u32	ccosr;
	volatile u32	cgpr;
	volatile u32	ccgr0;
	volatile u32	ccgr1;
	volatile u32	ccgr2;
	volatile u32	ccgr3;
	volatile u32	ccgr4;
	volatile u32	ccgr5;
	volatile u32	ccgr6;
	volatile u32	ccgr7;
	volatile u32	cmeor;
};

struct dpll {
	volatile u32	dp_ctl;
	volatile u32	dp_config;
	volatile u32	dp_op;
	volatile u32	dp_mfd;
	volatile u32	dp_mfn;
	volatile u32	dp_mfn_minus;
	volatile u32	dp_mfn_plus;
	volatile u32	dp_hfs_op;
	volatile u32	dp_hfs_mfd;
	volatile u32	dp_hfs_mfn;
	volatile u32	dp_mfn_togc;
	volatile u32	dp_destat;
};

