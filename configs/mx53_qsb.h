#if !defined (__CONFIG_H__)
#    define   __CONFIG_H__

	/* CREATE_ATAGS indicates that the shim ought to build the
	   ATAGS list.  If it is not defined, the ATAGS may be passed
	   by another piece of code, e.g. a boot loader. */
#define CREATE_ATAGS		/* Construct ATAGS from fields herein */

	/* GUARANTEE_ATAG_CMDLINE makes sure that a command line is
	   present in the ATAGS list.  If none is found, the
	   COMMANDLINE below is added appended to the end of the
	   ATAGS. */
#define GUARANTEE_ATAG_CMDLINE

#define PHYS_PARAMS			0x70000100 /* Address for the parameter list */

#define RAM_BANK0_START		0x70000000
#define RAM_BANK0_LENGTH	(512 * 1024 * 1024)

#define COMMANDLINE			"console=ttymxc0,115200 lpj=4997120"

#define MACH_TYPE			3273

#include "mx53.h"

#endif  /* __CONFIG_H__ */
