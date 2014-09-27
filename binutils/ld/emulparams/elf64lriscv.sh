. ${srcdir}/emulparams/elf64lriscv-defs.sh
OUTPUT_FORMAT="elf64-littleriscv"

# Magic sections.
OTHER_SECTIONS='
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
'
