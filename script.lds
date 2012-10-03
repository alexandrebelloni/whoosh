/* Linking loader script for APEX Boot Loader */

OUTPUT_ARCH(arm)
EXTERN(start)
ENTRY(start)

SECTIONS
{
	. = 0x0;  /* Actually, this code must be PIC */
	.text : {
		*(.text)
		*(.rodata)
		*(.boot)
	}
	. = ALIGN (4);
	/* Segments we don't want */
	/DISCARD/ . : {
		*(.fixup)
		*(.glue_7)
		*(.glue_7t)
		*(.data)
		*(.bss)
	}
	. = ALIGN (4);
}
