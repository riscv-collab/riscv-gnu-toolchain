# mach: crisv32
# output: 0\ncafebabe\nbaddbeef\necc0d00d\nc0ceface\npass\n

; Check that the PID register has the right size, 32 bits: check
; immediate, to/from register and memory.  (This has to be done in
; supervisor mode, so don't set u.)

 .include "testutils.inc"
 .macro dumpid
 move $pid,$r3
 dumpr3
 .endm

 start
 moveq -1,$r3
 move 0,$pid
 dumpid ; 0
 move 0xcafebabe,$pid
 dumpid ; cafebabe
 move.d 0xbaddbeef,$r2
 move $r2,$pid
 dumpid ; baddbeef
 move.d 0f,$r0
 move [$r0+],$pid
 cmp.d 0f+4,$r0
 beq 1f
 nop
dofail:
 fail
0:
 .dword 0xecc0d00d
0:
 .dword 0xc0ceface
1:
 dumpid ; ecc0d00d
 move.d 0b,$r1
 move 0xc0ceface,$pid
 move $pid,[$r1+]
 cmp.d 0b+4,$r1
 bne dofail
 subq 4,$r1
 nop
 move.d [$r1],$r3
 dumpr3 ; c0ceface
 pass
