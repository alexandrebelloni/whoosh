#if !defined (__CONFIG_H__)
#    define   __CONFIG_H__

	/* CREATE_ATAGS indicates that the shim ought to build the
	   ATAGS list.  If it is not defined, the ATAGS may be passed
	   by another piece of code, e.g. a boot loader. */
#define CREATE_ATAGS		/* Construct ATAGS from fields herein */

#define PHYS_PARAMS		0x10000100 /* Address for the parameter list */

#define RAM_BANK0_START		0x10000000
#define RAM_BANK0_LENGTH	(1u * 1024 * 1024 * 1024)

	/* GUARANTEE_ATAG_CMDLINE makes sure that a command line is
	   present in the ATAGS list.  If none is found, the
	   COMMANDLINE below is added appended to the end of the
	   ATAGS. */
#define GUARANTEE_ATAG_CMDLINE/* TODO: set lpj */
#define COMMANDLINE "console=ttymxc1,115200 video=mxcfb0:dev=ldb,LDB-XGA,if=RGB565"

/* For hdmi output : */
//#define COMMANDLINE "console=ttymxc1,115200 video=mxcfb0:dev=hdmi,800x600M@30"

#define MACH_TYPE		   3769

#include "mx6.h"

#endif  /* __CONFIG_H__ */
