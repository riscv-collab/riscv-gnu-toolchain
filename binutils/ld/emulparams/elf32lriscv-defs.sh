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

TEXT_START_ADDR=0x10000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"

SDATA_START_SYMBOLS="_gp = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)"

# Place the data section before text section.  This enables more compact
# global variable access for RVC code via linker relaxation.
INITIAL_READONLY_SECTIONS="
  .data           : { *(.data) *(.data.*) *(.gnu.linkonce.d.*) }
  .rodata         : { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r.*) }
  .srodata        : { ${SDATA_START_SYMBOLS} }
  .sdata          : { *(.sdata .sdata.* .gnu.linkonce.s.*) }
  .sbss           : { *(.dynsbss) *(.sbss .sbss.* .gnu.linkonce.sb.*) }
  .bss            : { *(.dynbss) *(.bss .bss.* .gnu.linkonce.b.*) *(COMMON) }
  . = ALIGN(${SEGMENT_SIZE}) + (. & (${MAXPAGESIZE} - 1));"
INITIAL_READONLY_SECTIONS=".interp         : { *(.interp) } ${CREATE_PIE-${INITIAL_READONLY_SECTIONS}}"
INITIAL_READONLY_SECTIONS="${RELOCATING+${CREATE_SHLIB-${INITIAL_READONLY_SECTIONS}}}"

SDATA_START_SYMBOLS="${CREATE_PIE+${SDATA_START_SYMBOLS}}"
