# mach: crisv3 crisv8 crisv10 crisv32

; Check that the simulator and the program agrees on the
; value of endmem; the base point for further allocations.

 .include "testutils.inc"
 start

; First, get the end of memory by calling brk with 0 to get
; the current value.
 clear.d $r10
 movu.w 0x2d,$r9
 break 13

; Get one more 8192-byte page.
 addu.w 8192,$r10

; Save it in a (call- and) syscall-saved register.
 move.d $r10,$r0

; Call brk again to raise the value.
 movu.w 0x2d,$r9
 break 13

; If the simulator messed up internally, it'd have exited
; with an error at the brk call.  But, let's sanity-check
; the return-value: it should be the same as the input and
; the allocated memory should be accessible.
 cmp.d $r10,$r0
 beq 0f
 nop

1:
 fail

0:
; Check that we can write and read the last address on that
; page.
 subq 4,$r10
 move.d 0xbad1d101,$r12
 move.d $r12,[$r10]
 move.d [$r10],$r11
 cmp.d $r11,$r12
 bne 1b
 nop

 pass
