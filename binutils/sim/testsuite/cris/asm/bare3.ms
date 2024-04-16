# mach: crisv32
# ld: -N --oformat binary --section-start=.text=0x10000000
# sim: --architecture crisv32 --target binary --cris-program-offset=0x10000000 --cris-start-address=0x10000040 --cris-naked --memory-region 0x10000000,0x1000
 .include "testutils.inc"

; Test that we can load a binary program at a non-zero address.
; Also serves to exercise the --cris-program-offset and
; --cris-start-address options.

; Make sure starting at the first address does fail.
 fail

; ...and that we know an offset we can jump for it to work, and all we
; have to assume is that "fail" takes no more than 64 bytes.
 .p2align 6
 ba _start
 nop

; 
 start
x:

; Make sure we're loaded at the linked address.  Since we're re-used
; in other tests, we have to provide for non-v32 as well.
 .if ..asm.arch.cris.v32
 lapcq .,$r0
 .else
 move.d $pc,$r0
 subq .-x,$r0
 .endif

 cmp.d x,$r0
 bne y
 nop
 pass
y:
 fail

; Make sure we have enough contents for the mapping.
 .data
 .fill 4096,1,0
