# check that basic insns work.
# mach: ft32

.include "testutils.inc"

	start

	ldk	  $r0,__PMSIZE
	EXPECT    $r0,0x00040000
	ldk       $r0,__RAMSIZE
	EXPECT    $r0,0x00010000

	ldk     $r4,10
	add     $r4,$r4,23
	EXPECT  $r4,33

# lda, sta
	.data
tmp:    .long     0
	.text

	xor.l     $r0,$r0,$r0
	EXPECT    $r0,0x00000000
	xor.l     $r0,$r0,$r0
	add.l     $r0,$r0,1
	EXPECT    $r0,0x00000001

	ldk.l     $r0,0x4567
	EXPECT    $r0,0x00004567

	lpm.l     $r0,k_12345678
	EXPECT    $r0,0x12345678

	sta.l     tmp,$r0
	lda.l     $r1,tmp
	EXPECT    $r1,0x12345678

	lda.b     $r1,tmp
	EXPECT    $r1,0x00000078

	lda.b     $r1,tmp+1
	EXPECT    $r1,0x00000056

	lda.b     $r1,tmp+2
	EXPECT    $r1,0x00000034

	lda.b     $r1,tmp+3
	EXPECT    $r1,0x00000012

	sta.b     tmp+1,$r0
	lda.l     $r1,tmp+0
	EXPECT    $r1,0x12347878

# immediate
	ldk.l     $r1,12
	add.l     $r1,$r1,4
	EXPECT    $r1,0x00000010
	add.l     $r1,$r1,0x1ff
	EXPECT    $r1,0x0000020f
	add.l     $r1,$r1,-0x200
	EXPECT    $r1,0x0000000f

# addk
	xor.l     $r1,$r0,$r0
	add.l     $r2,$r1,127
	EXPECT    $r2,0x0000007f

	add.l     $r2,$r2,127
	EXPECT    $r2,0x000000fe

	add.l     $r2,$r2,-127
	EXPECT    $r2,0x0000007f

	add.l     $r2,$r2,-128
	EXPECT    $r2,0xffffffff

	add.l     $r2,$r2,1
	EXPECT    $r2,0x00000000

# mul
	ldk.l     $r1,100
	ldk.l     $r2,77
	mul.l     $r3,$r1,$r2
	EXPECT    $r3,0x00001e14

	# 0x12345678 ** 2 = 0x14b66dc1df4d840L
	mul.l     $r3,$r0,$r0
	EXPECT    $r3,0x1df4d840
	muluh.l   $r3,$r0,$r0
	EXPECT    $r3,0x014b66dc

# push and pop
	push.l    $r0
	EXPECT    $sp,0x0000fffc
	ldi.l     $r3,$sp,0
	EXPECT    $r3,0x12345678

	pop.l     $r4
	EXPECT    $sp,0x00000000
	EXPECT    $r4,0x12345678

	ldk.l     $r1,0x1111
	push.l    $r1
	ldk.l     $r1,0x2222
	push.l    $r1
	ldk.l     $r1,0x3333
	push.l    $r1
	ldk.l     $r1,0x4444
	push.l    $r1
	EXPECT    $sp,0x0000fff0
	pop.l     $r1
	EXPECT    $r1,0x00004444
	pop.l     $r1
	EXPECT    $r1,0x00003333
	pop.l     $r1
	EXPECT    $r1,0x00002222
	pop.l     $r1
	EXPECT    $r1,0x00001111

# push and pop with $sp changes
	ldk.l     $r1,0xa111
	push.l    $r1
	sub.l     $sp,$sp,4
	ldk.l     $r1,0xa222
	push.l    $r1
	add.l     $sp,$sp,-36
	add.l     $sp,$sp,36
	pop.l     $r1
	EXPECT    $r1,0x0000a222
	add.l     $sp,$sp,4
	pop.l     $r1
	EXPECT    $r1,0x0000a111

# sti
	ldk.l     $r2,80
	EXPECT    $r2,0x00000050
	sti.l     $r2,0,$r0
	lda.l     $r1,80
	EXPECT    $r1,0x12345678

	ldk.l     $r3,0xF0
	sti.b     $r2,0,$r3
	lda.l     $r1,80
	EXPECT    $r1,0x123456f0

	add.l     $r2,$r2,1
	sti.l     $r2,0,$r0
	sti.b     $r2,0,$r3
	lda.l     $r1,80
	EXPECT    $r1,0x1234f078

	add.l     $r2,$r2,1
	sti.l     $r2,0,$r0
	sti.b     $r2,0,$r3
	lda.l     $r1,80
	EXPECT    $r1,0x12f05678

	add.l     $r2,$r2,1
	sti.l     $r2,0,$r0
	sti.b     $r2,0,$r3
	lda.l     $r1,80
	EXPECT    $r1,0xf0345678

	ldk.l     $r2,80
	sti.l     $r2,0,$r0
	ldk.s     $r3,0xbeef
	sti.s     $r2,0,$r3
	lda.l     $r1,80
	EXPECT    $r1,0x1234beef
	add.l     $r2,$r2,2
	sti.s     $r2,0,$r3
	lda.l     $r1,80
	EXPECT    $r1,0xbeefbeef

# lpmi

	ldk.l     $r1,k_12345678
	lpmi.l    $r2,$r1,0
	EXPECT    $r2,0x12345678

	lpmi.b    $r2,$r1,0
	EXPECT    $r2,0x00000078

	add.l     $r1,$r1,1
	lpmi.b    $r2,$r1,0
	EXPECT    $r2,0x00000056

	add.l     $r1,$r1,1
	lpmi.b    $r2,$r1,0
	EXPECT    $r2,0x00000034

	add.l     $r1,$r1,1
	lpmi.b    $r2,$r1,0
	EXPECT    $r2,0x00000012

	lpmi.l    $r2,$r1,4
	EXPECT    $r2,0xabcdef01

	lpmi.l    $r2,$r1,-4
	EXPECT    $r2,0x10111213

	lpmi.b    $r2,$r1,-4
	EXPECT    $r2,0x00000010

	ldk.l     $r1,k_12345678
	lpmi.s    $r2,$r1,0
	EXPECT    $r2,0x00005678
	lpmi.s    $r2,$r1,2
	EXPECT    $r2,0x00001234
	lpmi.b    $r2,$r1,6
	EXPECT    $r2,0x000000cd
	lpmi.b    $r2,$r1,7
	EXPECT    $r2,0x000000ab
	lpmi.b    $r2,$r1,-1
	EXPECT    $r2,0x00000010
	lpmi.s    $r2,$r1,-2
	EXPECT    $r2,0x00001011

	ldk.l     $r1,k_12345678-127
	lpmi.b    $r2,$r1,127
	EXPECT    $r2,0x00000078

	ldk.l     $r1,k_12345678+128
	lpmi.b    $r2,$r1,-128
	EXPECT    $r2,0x00000078

# shifts

	lpm.l     $r0,k_12345678
	ldk.l     $r2,4
	ashl.l    $r1,$r0,$r2
	EXPECT    $r1,0x23456780
	lshr.l    $r1,$r0,$r2
	EXPECT    $r1,0x01234567
	ashr.l    $r1,$r0,$r2
	EXPECT    $r1,0x01234567

	lpm.l     $r0,k_abcdef01
	ashl.l    $r1,$r0,$r2
	EXPECT    $r1,0xbcdef010
	lshr.l    $r1,$r0,$r2
	EXPECT    $r1,0x0abcdef0
	ashr.l    $r1,$r0,$r2
	EXPECT    $r1,0xfabcdef0

# rotate right

	lpm.l     $r0,k_12345678
	ror.l     $r1,$r0,0
	EXPECT    $r1,0x12345678
	ror.l     $r1,$r0,12
	EXPECT    $r1,0x67812345
	ror.l     $r1,$r0,-4
	EXPECT    $r1,0x23456781

# jmpx
	ldk       $r28,0xaaaaa
	jmpx      0,$r28,1,failcase
	jmpx      1,$r28,0,failcase
	jmpx      2,$r28,1,failcase
	jmpx      3,$r28,0,failcase
	jmpx      4,$r28,1,failcase
	jmpx      5,$r28,0,failcase
	jmpx      6,$r28,1,failcase
	jmpx      7,$r28,0,failcase
	jmpx      8,$r28,1,failcase
	jmpx      9,$r28,0,failcase
	jmpx      10,$r28,1,failcase
	jmpx      11,$r28,0,failcase
	jmpx      12,$r28,1,failcase
	jmpx      13,$r28,0,failcase
	jmpx      14,$r28,1,failcase
	jmpx      15,$r28,0,failcase
	jmpx      16,$r28,1,failcase
	jmpx      17,$r28,0,failcase
	jmpx      18,$r28,1,failcase
	jmpx      19,$r28,0,failcase

	move      $r29,$r28
	ldk       $r28,0
	jmpx      0,$r29,1,failcase
	jmpx      1,$r29,0,failcase
	jmpx      2,$r29,1,failcase
	jmpx      3,$r29,0,failcase
	jmpx      4,$r29,1,failcase
	jmpx      5,$r29,0,failcase
	jmpx      6,$r29,1,failcase
	jmpx      7,$r29,0,failcase
	jmpx      8,$r29,1,failcase
	jmpx      9,$r29,0,failcase
	jmpx      10,$r29,1,failcase
	jmpx      11,$r29,0,failcase
	jmpx      12,$r29,1,failcase
	jmpx      13,$r29,0,failcase
	jmpx      14,$r29,1,failcase
	jmpx      15,$r29,0,failcase
	jmpx      16,$r29,1,failcase
	jmpx      17,$r29,0,failcase
	jmpx      18,$r29,1,failcase
	jmpx      19,$r29,0,failcase

	move      $r30,$r29
	ldk       $r29,0
	jmpx      0,$r30,1,failcase
	jmpx      1,$r30,0,failcase
	jmpx      2,$r30,1,failcase
	jmpx      3,$r30,0,failcase
	jmpx      4,$r30,1,failcase
	jmpx      5,$r30,0,failcase
	jmpx      6,$r30,1,failcase
	jmpx      7,$r30,0,failcase
	jmpx      8,$r30,1,failcase
	jmpx      9,$r30,0,failcase
	jmpx      10,$r30,1,failcase
	jmpx      11,$r30,0,failcase
	jmpx      12,$r30,1,failcase
	jmpx      13,$r30,0,failcase
	jmpx      14,$r30,1,failcase
	jmpx      15,$r30,0,failcase
	jmpx      16,$r30,1,failcase
	jmpx      17,$r30,0,failcase
	jmpx      18,$r30,1,failcase
	jmpx      19,$r30,0,failcase

# callx
	ldk       $r30,0xaaaaa
	callx     0,$r30,0,skip1
	jmp       failcase
	callx     1,$r30,1,skip1
	jmp       failcase
	callx     2,$r30,0,skip1
	jmp       failcase
	callx     3,$r30,1,skip1
	jmp       failcase

	callx     0,$r30,1,skip1
	ldk       $r30,0x123
	EXPECT    $r30,0x123

#define BIT(N,M)  ((((N) & 15) << 5) | (M))
# bextu
	bextu.l   $r1,$r0,(0<<5)|0
	EXPECT    $r1,0x00005678
	bextu.l   $r1,$r0,(4<<5)|0
	EXPECT    $r1,0x00000008
	bextu.l   $r1,$r0,(4<<5)|4
	EXPECT    $r1,0x00000007
	bextu.l   $r1,$r0,(4<<5)|28
	EXPECT    $r1,0x00000001
	bextu.l   $r1,$r0,(8<<5)|16
	EXPECT    $r1,0x00000034
	ldk.l     $r2,-1
	bextu.l   $r1,$r2,(6<<5)|(3)
	EXPECT    $r1,0x0000003f

# bexts
	bexts.l   $r1,$r0,(8<<5)|0
	EXPECT    $r1,0x00000078
	bexts.l   $r1,$r0,(0<<5)|16
	EXPECT    $r1,0x00001234
	bexts.l   $r1,$r0,(4<<5)|0
	EXPECT    $r1,0xfffffff8
	# extract the '5' digit in widths 4-1
	bexts.l   $r1,$r0,(4<<5)|12
	EXPECT    $r1,0x00000005
	bexts.l   $r1,$r0,(3<<5)|12
	EXPECT    $r1,0xfffffffd
	bexts.l   $r1,$r0,(2<<5)|12
	EXPECT    $r1,0x00000001
	bexts.l   $r1,$r0,(1<<5)|12
	EXPECT    $r1,0xffffffff

# btst
	# low four bits should be 0,0,0,1
	btst.l    $r0,(1<<5)|0
	jmpc      nz,failcase
	btst.l    $r0,(1<<5)|1
	jmpc      nz,failcase
	btst.l    $r0,(1<<5)|2
	jmpc      nz,failcase
	btst.l    $r0,(1<<5)|3
	jmpc      z,failcase

	# the 6 bit field starting at position 24 is positive
	btst.l    $r0,(6<<5)|24
	jmpc      s,failcase
	# the 5 bit field starting at position 24 is negative
	btst.l    $r0,(5<<5)|24
	jmpc      ns,failcase

	EXPECT    $r0,0x12345678

# bins
	bins.l    $r1,$r0,(8 << 5) | (0)
	EXPECT    $r1,0x12345600

	bins.l    $r1,$r0,(0 << 5) | (8)
	EXPECT    $r1,0x12000078

	ldk.l     $r1,(0xff << 10) | (8 << 5) | (8)
	bins.l    $r1,$r0,$r1
	EXPECT    $r1,0x1234ff78

	call      litr1
	.long     (0x8dd1 << 10) | (0 << 5) | (0)
	bins.l    $r1,$r0,$r1
	EXPECT    $r1,0x12348dd1

	call      litr1
	.long     (0x8dd1 << 10) | (0 << 5) | (16)
	bins.l    $r1,$r0,$r1
	EXPECT    $r1,0x8dd15678

	ldk.l     $r1,(0xde << 10) | (8 << 5) | (0)
	bins.l    $r1,$r0,$r1
	EXPECT    $r1,0x123456de

# ldl
	ldk.l     $r0,0
	ldl.l     $r3,$r0,0
	EXPECT    $r3,0x00000000
	ldk.l     $r0,-1
	ldl.l     $r3,$r0,-1
	EXPECT    $r3,0xffffffff
	ldk.l     $r0,(0x12345678 >> 10)
	ldl.l     $r3,$r0,(0x12345678 & 0x3ff)
	EXPECT    $r3,0x12345678
	ldk.l     $r0,(0xe2345678 >> 10)
	ldl.l     $r3,$r0,(0xe2345678 & 0x3ff)
	EXPECT    $r3,0xe2345678

# flip
	ldk.l     $r0,0x0000001
	flip.l    $r1,$r0,0
	EXPECT    $r1,0x00000001

	lpm.l     $r0,k_12345678
	flip.l    $r1,$r0,0
	EXPECT    $r1,0x12345678
	flip.l    $r1,$r0,24
	EXPECT    $r1,0x78563412
	flip.l    $r1,$r0,31
	EXPECT    $r1,0x1e6a2c48

# stack push pop

	EXPECT    $sp,0x00000000
	ldk.l     $r6,0x6666
	push.l    $r6
	or.l      $r0,$r0,$r0      # xxx
	EXPECT    $sp,0x0000fffc
	ldi.l     $r1,$sp,0
	EXPECT    $r1,0x00006666
	pop.l     $r1
	EXPECT    $r1,0x00006666
	EXPECT    $sp,0x00000000

# call/return
	call      fowia
	push.l    $r1
	call      fowia
	pop.l     $r2
	sub.l     $r1,$r1,$r2
	EXPECT    $r1,0x00000008

# add,carry

	ldk.l     $r0,0
	ldk.l     $r1,0
	call      add64
	EXPECT    $r1,0x00000000
	EXPECT    $r0,0x00000000

	lpm.l     $r0,k_abcdef01
	lpm.l     $r1,k_abcdef01
	call      add64
	EXPECT    $r1,0x00000001
	EXPECT    $r0,0x579bde02

	ldk.l     $r0,4
	ldk.l     $r1,-5
	call      add64
	EXPECT    $r1,0x00000000
	EXPECT    $r0,0xffffffff

	ldk.l     $r0,5
	ldk.l     $r1,-5
	call      add64
	EXPECT    $r1,0x00000001
	EXPECT    $r0,0x00000000

	lpm.l     $r0,k_12345678
	ldk.l     $r1,-1
	call      add64
	EXPECT    $r1,0x00000001
	EXPECT    $r0,0x12345677

	ldk.l     $r0,-1
	ldk.l     $r1,-1
	call      add64
	EXPECT    $r1,0x00000001
	EXPECT    $r0,0xfffffffe

# inline literal
	call      lit
	.long     0xdecafbad
	EXPECT    $r0,0xdecafbad

	ldk.l     $r1,0xee
	call      lit
	ldk.l     $r1,0xfe
	EXPECT    $r1,0x000000ee

	call      lit
	.long     0x01020304
	EXPECT    $r0,0x01020304

	call      lit
	.long     lit
	calli     $r0
	.long     0xffaa55aa
	EXPECT    $r0,0xffaa55aa

# comparisons
	ldk.l     $r0,-100
	ldk.l     $r1,100
	cmp.l     $r0,$r1

	ldk.l     $r2,0
	jmpc      lt,.c1
	ldk.l     $r2,1
.c1:
	EXPECT    $r2,0x00000000

	ldk.l     $r2,0
	jmpc      gt,.c2
	ldk.l     $r2,1
.c2:
	EXPECT    $r2,0x00000001

	ldk.l     $r2,0
	jmpc      a,.c3
	ldk.l     $r2,1
.c3:
	EXPECT    $r2,0x00000000

	ldk.l     $r2,0
	jmpc      b,.c4
	ldk.l     $r2,1
.c4:
	EXPECT    $r2,0x00000001

	ldk.l     $r2,0
	jmpc      be,.c5
	ldk.l     $r2,1
.c5:
	EXPECT    $r2,0x00000001

# 8-bit comparisons
	ldk.l     $r0,0x8fe
	ldk.l     $r1,0x708
	cmp.b     $r0,$r1

	ldk.l     $r2,0
	jmpc      lt,.8c1
	ldk.l     $r2,1
.8c1:
	EXPECT    $r2,0x00000000

	ldk.l     $r2,0
	jmpc      gt,.8c2
	ldk.l     $r2,1
.8c2:
	EXPECT    $r2,0x00000001

	ldk.l     $r2,0
	jmpc      a,.8c3
	ldk.l     $r2,1
.8c3:
	EXPECT    $r2,0x00000000

	ldk.l     $r2,0
	jmpc      b,.8c4
	ldk.l     $r2,1
.8c4:
	EXPECT    $r2,0x00000001

	ldk.l     $r2,0
	jmpc      be,.8c5
	ldk.l     $r2,1
.8c5:
	EXPECT    $r2,0x00000001

	ldk.l     $r0,0x8aa
	ldk.l     $r1,0x7aa
	cmp.b     $r0,$r1

	ldk.l     $r2,0
	jmpc      z,.8c6
	ldk.l     $r2,1
.8c6:
	EXPECT    $r2,0x00000000

	ldk.b     $r0,1
	ldk.b     $r2,0xe0
	cmp.b     $r2,0x1c0
	jmpc      a,.8c7
	ldk.b     $r0,0
.8c7:
	EXPECT    $r0,0x00000001

# conditional call
	cmp.l     $r0,$r0
	callc     z,lit
	.long     0xccddeeff
	callc     nz,zr0
	EXPECT    $r0,0xccddeeff

# modify return address
	ldk.l     $r0,0x66
	call      skip1
	ldk.l     $r0,0xAA
	EXPECT    $r0,0x00000066

	ldk.l     $r0,0x77
	call      skip2
	ldk.l     $r0,0xBB
	EXPECT    $r0,0x00000077

# simple recursive function
	ldk.l     $r0,1
	call      factorial
	EXPECT    $r0,0x00000001
	ldk.l     $r0,2
	call      factorial
	EXPECT    $r0,0x00000002
	ldk.l     $r0,3
	call      factorial
	EXPECT    $r0,0x00000006
	ldk.l     $r0,4
	call      factorial
	EXPECT    $r0,0x00000018
	ldk.l     $r0,5
	call      factorial
	EXPECT    $r0,0x00000078
	ldk.l     $r0,6
	call      factorial
	EXPECT    $r0,0x000002d0
	ldk.l     $r0,7
	call      factorial
	EXPECT    $r0,0x000013b0
	ldk.l     $r0,12
	call      factorial
	EXPECT    $r0,0x1c8cfc00

# read sp after a call
	call      nullfunc
	EXPECT    $sp,0x00000000

# CALLI->RETURN
	ldk.l     $r4,nullfunc
	calli     $r4
	EXPECT    $sp,0x00000000

# Link/unlink
	ldk.l     $r14,0x17566

	link      $r14,48
	EXPECT    $r14,0x0000fffc
	sub.l     $sp,$sp,200
	unlink    $r14
	EXPECT    $r14,0x00017566

# LINK->UNLINK
	link      $r14,48
	unlink    $r14
	EXPECT    $r14,0x00017566

# LINK->JUMPI
	ldk.l     $r3,.here
	link      $r14,48
	jmpi      $r3
	jmp       failcase
.here:
	unlink    $r14
	EXPECT    $r14,0x00017566

# LINK->RETURN
# (This is a nonsense combination, but can still exericse it by
# using a negative parameter for the link.  "link $r14,-4" leaves
# $sp exactly unchanged.)
	ldk.l     $r0,.returnhere
	push.l    $r0
	link      $r14,0xfffc
	return
.returnhere:
	EXPECT    $sp,0x00000000

# LPMI->CALLI
	ldk.l     $r0,k_abcdef01
	ldk.l     $r1,increment
	lpmi.l    $r0,$r0,0
	calli     $r1
	EXPECT    $r0,0xabcdef02

# STRLen
	lpm.l     $r4,str3
	sta.l     tmp,$r4
	ldk.l     $r0,tmp
	strlen.b  $r1,$r0
	EXPECT    $r1,0x00000003
	strlen.s  $r1,$r0
	EXPECT    $r1,0x00000003
	strlen.l  $r1,$r0
	EXPECT    $r1,0x00000003

	ldk.l     $r4,0
	sta.b     4,$r4
	strlen.l  $r1,$r0
	EXPECT    $r1,0x00000000

	ldk.l     $r4,-1
	sta.l     4,$r4
	lpm.l     $r4,str3
	sta.l     8,$r4
	strlen.l  $r1,$r0
	EXPECT    $r1,0x00000007

# MEMSet
	ldk.l     $r0,4
	ldk.l     $r1,0xaa
	memset.s  $r0,$r1,8
	ldk.l     $r1,0x55
	memset.b  $r0,$r1,5
	lda.l     $r0,4
	EXPECT    $r0,0x55555555
	lda.l     $r0,8
	EXPECT    $r0,0xaaaaaa55

# first cycle after mispredict
	ldk.l     $r0,3
	cmp.l     $r0,$r0
	jmpc      nz,failcase
	add.l     $r0,$r0,7
	EXPECT    $r0,0x0000000a
	jmpc      nz,failcase
	push.l    $r0
	EXPECT    $sp,0x0000fffc
	pop.l     $r0

# $sp access after stall
	lpm.l     $r13,0
	push.l    $r0
	EXPECT    $sp,0x0000fffc
	pop.l     $r0

	push.l    $r0
	add.l     $sp,$sp,-484
	EXPECT    $sp,0x0000fe18
	EXPECT    $sp,0x0000fe18
	EXPECT    $sp,0x0000fe18
	add.l     $sp,$sp,484
	EXPECT    $sp,0x0000fffc
	pop.l     $r0

# atomic exchange
	lpm.l     $r0,k_12345678
	lpm.l     $r1,k_abcdef01
	sta.l     100,$r1
	exa.l     $r0,100
	EXPECT    $r0,0xabcdef01
	lda.l     $r0,100
	EXPECT    $r0,0x12345678

	lpm.l     $r0,k_12345678
	lpm.l     $r1,k_abcdef01
	sta.l     144,$r1
	ldk.l     $r7,20
	exi.l     $r0,$r7,124
	EXPECT    $r0,0xabcdef01
	lda.l     $r0,144
	EXPECT    $r0,0x12345678

	lpm.l     $r0,k_12345678
	lpm.l     $r1,k_abcdef01
	push      $r1
	exi.l     $r0,$sp,0
	EXPECT    $r0,0xabcdef01
	pop.l     $r0
	EXPECT    $r0,0x12345678

# PM write port
	.equ    PM_UNLOCK,      0x1fc80
	.equ    PM_ADDR,        0x1fc84
	.equ    PM_DATA,        0x1fc88

	lpm.l     $r0,k_12345678
	lpm.l     $r1,k_abcdef01
	EXPECT    $r0,0x12345678
	EXPECT    $r1,0xabcdef01
	ldk.l     $r3,(0x1337f7d1 >> 10)
	ldl.l     $r3,$r3,(0x1337f7d1 & 0x3ff)
	EXPECT    $r3,0x1337f7d1
	ldk	  $r4,k_12345678
	sta.l     PM_ADDR,$r4

	# write while locked does nothing
	sta.l	  PM_DATA,$r1
	sta.l	  PM_DATA,$r0
	lpm.l     $r0,k_12345678
	lpm.l     $r1,k_abcdef01
	EXPECT    $r0,0x12345678
	EXPECT    $r1,0xabcdef01

	# write while unlocked modifies program memory
	sta.l	  PM_UNLOCK,$r3
	sta.l	  PM_DATA,$r1
	sta.l	  PM_DATA,$r0
	lpm.l     $r0,k_12345678
	lpm.l     $r1,k_abcdef01
	EXPECT    $r0,0xabcdef01
	EXPECT    $r1,0x12345678

# final stack check
	EXPECT    $sp,0x00000000

	PASS

# --------------------------------------------------

skip1:          # skip the instruction after the call
	pop.l     $r1
	add.l     $r1,$r1,4
	push.l    $r1
	return

skipparent:     # skip the instruction after the caller's call
	ldi.l     $r1,$sp,4
	add.l     $r1,$r1,4
	sti.l     $sp,4,$r1
	return
skip2:
	call      skipparent
	return

add64:
	addcc.l   $r0,$r1
	add.l     $r0,$r0,$r1
	ldk.l     $r1,0
	jmpc      nc,.done
	ldk.l     $r1,1
.done:
	return

fowia:  # find out where I'm at
	ldi.l     $r1,$sp,0
	return

lit:    # load literal to $r0
	pop.l     $r14
	lpmi.l    $r0,$r14,0
	add.l     $r14,$r14,4
	jmpi      $r14
zr0:
	ldk.l     $r0,0
	return
litr1:
	ldi.l     $r1,$sp,0
	add.l     $r1,$r1,4
	sti.l     $sp,0,$r1
	lpmi.l    $r1,$r1,-4
	return

factorial:
	ldk.l     $r1,1
	cmp.l     $r0,$r1
	jmpc      z,.factdone
	push.l    $r0
	add.l     $r0,$r0,-1
	call      factorial
	pop.l     $r1
	mul.l     $r0,$r0,$r1
.factdone:
	return

nullfunc:
	return

increment:
	add.l     $r0,$r0,1
	return

	.long   0x10111213
k_12345678:
	.long   0x12345678
k_abcdef01:
	.long   0xabcdef01
str3:
	.string   "abc"
