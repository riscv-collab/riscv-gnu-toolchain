source_sh ${srcdir}/emulparams/elf32lriscv-defs.sh
ELFSIZE=64
SEPARATE_GOTPLT="SIZEOF (.got.plt) >= 16 ? 16 : 0"
