# If you change this file, please also look at files which source this one:
# elf64ltsmip.sh

. ${srcdir}/emulparams/elf64lriscv-defs.sh
OUTPUT_FORMAT="elf64-littleriscv"

# Magic sections.
OTHER_SECTIONS='
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
'
