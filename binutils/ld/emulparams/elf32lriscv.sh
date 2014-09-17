. ${srcdir}/emulparams/elf32lriscv-defs.sh
OUTPUT_FORMAT="elf32-littleriscv"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"

# Magic sections.
OTHER_SECTIONS='
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
'
