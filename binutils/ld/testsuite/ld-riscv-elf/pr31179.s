.globl _start
_start:

foo:
.2byte 0
bar:

.uleb128 bar - foo + 1

reloc:
.reloc reloc, R_RISCV_SET_ULEB128, bar + 1
.reloc reloc, R_RISCV_SUB_ULEB128, foo + 1
.byte 0x0
