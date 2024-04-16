# mach: crisv32

 .include "testutils.inc"

; Check for various non-arithmetic insns that C and V are not affected
; on v32 (where they were on v10), as the generic tests don't cover
; that; they are cleared before testing.

; First, a macro testing that VC are unaffected, not counting previous
; register contents.
 .macro nonvc0 insn op
 move.d $r0,$r3
 setf vc
 .ifnc \insn,swapnwbr
 \insn \op,$r3
 .else
 \insn $r3
 .endif
 bcc 9f
 nop
 bvc 9f
 nop
 move.d $r0,$r3
 clearf vc
 .ifnc \insn,swapnwbr
 \insn \op,$r3
 .else
 \insn $r3
 .endif
 bcs 9f
 nop
 bvc 8f
 nop
9:
 fail
8:
 .endm

; Use the above, but initialize the non-parameter operand to a value.
 .macro nonvc1 insn val op
 move.d \val,$r0
 nonvc0 \insn,\op
 .endm

; Use the above, iterating over various values.
 .macro nonvc2 insn op
 .irp p,0,1,2,31,32,63,64,127,128,255,256,32767,32768,65535,65536,0x7fffffff,0x80000000
 nonvc1 \insn,\p,\op
 nonvc1 \insn,-\p,\op
 .endr
 .endm

 .macro nonvc2q insn op min=-63 max=63
 .if (\op >= \min) && (\op <= \max)
 nonvc2 \insn,\op
 .endif
 .endm

; The above, for each .b .w .d insn variant.
 .macro nonvcbwd insn op
 .irp s,.b,.w,.d
 nonvc2 \insn\s,\op
 .endr
 .endm

; For various insns with register, dword constant and memory operands.
 .macro nonvcitermcd op=[$r4]
 nonvc2 and.d,\op
 nonvc2 move.d,\op
 nonvc2 or.d,\op
 .endm

; Similar, for various insns with register, word constant and memory operands.
 .macro nonvcitermcw op=[$r4]
 nonvcitermcd \op
 nonvc2 and.w,\op
 nonvc2 move.w,\op
 nonvc2 or.w,\op
 nonvc2 movs.w,\op
 nonvc2 movu.w,\op
 .endm

; Similar, for various insns with register, byte constant and memory operands.
 .macro nonvcitermcb op=[$r4]
 nonvcitermcw \op
 nonvc2 and.b,\op
 nonvc2 move.b,\op
 nonvc2 or.b,\op
 nonvc2 movs.b,\op
 nonvc2 movu.b,\op
 .endm

; Similar, for insns with quick constant operands.
 .macro nonvciterq op
 nonvcitermcb \op
 nonvc2 bound.b,\op
 nonvc2q andq,\op,min=-32,max=31
 nonvc2q asrq,\op,min=0,max=31
 nonvc2q lsrq,\op,min=0,max=31
 nonvc2q orq,\op,min=-32,max=31
 nonvc2q moveq,\op,min=-32,max=31
 .endm

; Similar, for insns with register operands.
 .macro nonvciterr op
 nonvcitermcb \op
 nonvcbwd bound,\op
 nonvc2 abs,\op
 nonvcbwd asr,\op
 nonvc2 dstep,\op
 nonvcbwd lsr,\op
 nonvcbwd lsl,\op
 nonvc2 lz,\op
 nonvc2 swapnwbr
 nonvc2 xor,\op
 .endm

; Test all applicable constant, register and memory variants of a value.
 .macro tst op
; Constants
 .if (\op <= 31) && (\op >= -32)
 nonvciterq \op
 .elseif (\op <= 255) && (\op >= -128)
 nonvcitermcb \op
 nonvcbwd bound,\op
 .elseif (\op <= 65535) && (\op >= -32767)
 nonvcitermcw \op
 nonvc2 bound.w,\op
 nonvc2 bound.d,\op
 .else
 nonvcitermcd \op
 nonvc2 bound.d,\op
 .endif
; Registers
 move.d \op,$r4
 nonvciterr $r4
; Memory
 nonvcitermcb [$r5]
 addq 4,$r5
 .section .rodata
 .dword \op
 .previous
 .endm

; As above but negation too.
 .macro tstpm op
 tst \op
 tst -\op
 .endm


; Set up for the actual test.

 start
 move.d c0,$r5

 .section .rodata
c0:
 .previous

; Finally, test.

 .irp x,0,1,2,31,32,63,64,127,128,255,256,32767,32768,65535,65536,0x7fffffff,0x80000000
 tstpm \x
 .endr 

 pass
