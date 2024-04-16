MEMORY
{
  L1_CODE : ORIGIN = 0xFFA00000, LENGTH = 0x8000
  L1_DATA : ORIGIN = 0xFF800000, LENGTH = 0x8000
  SDRAM : ORIGIN = 0x4000, LENGTH = 0x4000000
}

OUTPUT_ARCH(bfin)
ENTRY(__start)

SECTIONS
{
	.text     : { *(.text)     } >L1_CODE
	.text.usr : { *(.text.usr) } >SDRAM
	.data     : { *(.data)     } >SDRAM
}
