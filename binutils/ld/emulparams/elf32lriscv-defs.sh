# This is an ELF platform.
SCRIPT_NAME=elf

# Handle both little-ended 32-bit RISC-V objects.
ARCH=riscv
OUTPUT_FORMAT="elf32-littleriscv"

TEMPLATE_NAME=elf32
EXTRA_EM_FILE=riscvelf

case "$EMULATION_NAME" in
elf32*) ELFSIZE=32; LIBPATH_SUFFIX=32 ;;
elf64*) ELFSIZE=64; LIBPATH_SUFFIX=   ;;
*) echo $0: unhandled emulation $EMULATION_NAME >&2; exit 1 ;;
esac

if test `echo "$host" | sed -e s/64//` = `echo "$target" | sed -e s/64//`; then
  case " $EMULATION_LIBPATH " in
    *" ${EMULATION_NAME} "*)
      NATIVE=yes
      ;;
  esac
fi

GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes

TEXT_START_ADDR=0x10000000
SHLIB_TEXT_START_ADDR=0x1000000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
ENTRY=_start

# Unlike most targets, the RISC-V backend puts all dynamic relocations
# in a single dynobj section, which it also calls ".rel.dyn".  It does
# this so that it can easily sort all dynamic relocations before the
# output section has been populated.
OTHER_GOT_RELOC_SECTIONS="
  .rel.dyn      ${RELOCATING-0} : { *(.rel.dyn) }
"
GOT=".got          ${RELOCATING-0} : { *(.got) }"
unset OTHER_READWRITE_SECTIONS
unset OTHER_RELRO_SECTIONS

# Magic symbols.
TEXT_START_SYMBOLS='_ftext = . ;'
DATA_START_SYMBOLS='_fdata = . ;'
OTHER_BSS_SYMBOLS='_fbss = .;'

INITIAL_READONLY_SECTIONS=".interp       ${RELOCATING-0} : { *(.interp) }"
SDATA_START_SYMBOLS="_gp = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)"
if test -n "${CREATE_SHLIB}"; then
  INITIAL_READONLY_SECTIONS=
  SDATA_START_SYMBOLS=
  OTHER_READONLY_SECTIONS=".srodata      ${RELOCATING-0} : { *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*) }"
  unset GOT
fi

TEXT_DYNAMIC=
