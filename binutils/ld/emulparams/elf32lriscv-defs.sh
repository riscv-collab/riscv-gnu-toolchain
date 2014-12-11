# This is an ELF platform.
SCRIPT_NAME=elf
ARCH=riscv
OUTPUT_FORMAT="elf32-littleriscv"
NO_REL_RELOCS=yes

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

TEXT_START_ADDR=0x800000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"

INITIAL_READONLY_SECTIONS=".interp       ${RELOCATING-0} : { *(.interp) }"
SDATA_START_SYMBOLS="_gp = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)"
if test -n "${CREATE_SHLIB}"; then
  INITIAL_READONLY_SECTIONS=
  SDATA_START_SYMBOLS=
  OTHER_READONLY_SECTIONS=".srodata      ${RELOCATING-0} : { *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*) }"
  unset GOT
fi
