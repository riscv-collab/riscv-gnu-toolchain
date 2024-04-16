# The .align may cause overflow because deleting nops.
  .text		      # 0x120004000
  .align 3
  la.local $r12, .L1

#  .fill 0x1f7ffc # max fill without overflow, .data address is 0x120200000
  .fill 0x1f8000 # min fill with overflow, .data address is 0x120204000
#  .fill 0x1fbff4 # max fill with overflow, .data address is 0x120204000

  .data
.L1:
  .byte 2
