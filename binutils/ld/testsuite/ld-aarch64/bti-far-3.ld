OUTPUT_ARCH(aarch64)
SECTIONS
{
  . = 0x00010000;
  .rela.plt       : { *(.rela.plt) *(.rela.iplt) }
  . = 0x00020000;
  .plt            : { *(.plt) *(.iplt) }
  . = 0x00030000;
  .text           : { *(.text) }
  . = 0x20000000;
  .got            : { *(.got) *(.got.plt) }
}
