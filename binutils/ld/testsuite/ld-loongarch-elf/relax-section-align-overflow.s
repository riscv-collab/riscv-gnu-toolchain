# relocation overflow because .align
.text
  addi.d $t0, $t1, 0
  addi.d $t0, $t1, 0
  # Add one NOP instruction
  .align 3
  addi.d $t0, $t1, 0

.section ".t.a", "ax"
  addi.d $t0, $t1, 0
  # In one try:
  # first pass, la.local can be relaxed (0x120200010 - 0x120000014 = 0x1ffffc)
  # second pass, the NOP addend by .align be deleted and pc decrease 4,
  # but .L1 not decrease because section alignment.
  # (0x120200010 - 0x120000010 = 0x200000)
  la.local $t0, .L1
  .fill 0x1ffff0

.section ".t.b", "ax"
.L1:
  addi.d $t0, $t1, 0
  # To make section address not change when first .align 3 delete NOP
  .align 4
  addi.d $t0, $t1, 0

