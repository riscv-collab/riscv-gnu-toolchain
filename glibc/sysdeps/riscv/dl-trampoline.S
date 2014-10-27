#include <sysdep.h>
#include <sys/asm.h>

/* Assembler veneer called from the PLT header code for lazy loading.
   The PLT header passes its own args in t0-t2. */

ENTRY(_dl_runtime_resolve)
  # Save arguments to stack.
  addi sp, sp, -10*SZREG
  REG_S ra, 9*SZREG(sp)
  REG_S a0, 1*SZREG(sp)
  REG_S a1, 2*SZREG(sp)
  REG_S a2, 3*SZREG(sp)
  REG_S a3, 4*SZREG(sp)
  REG_S a4, 5*SZREG(sp)
  REG_S a5, 6*SZREG(sp)
  REG_S a6, 7*SZREG(sp)
  REG_S a7, 8*SZREG(sp)

  # Update .got.plt and obtain runtime address of callee.
  slli a1, t1, 1
  mv a0, t0       # link map
  add a1, a1, t1  # reloc offset (== thrice the .got.plt offset)
  jal _dl_fixup
  move t0, a0

  # Restore arguments from stack.
  REG_L ra, 9*SZREG(sp)
  REG_L a0, 1*SZREG(sp)
  REG_L a1, 2*SZREG(sp)
  REG_L a2, 3*SZREG(sp)
  REG_L a3, 4*SZREG(sp)
  REG_L a4, 5*SZREG(sp)
  REG_L a5, 6*SZREG(sp)
  REG_L a6, 7*SZREG(sp)
  REG_L a7, 8*SZREG(sp)
  addi sp, sp, 10*SZREG

  # Invoke the callee.
  jr t0
END(_dl_runtime_resolve)
