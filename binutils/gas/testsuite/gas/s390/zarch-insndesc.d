#name: s390x insndesc
#objdump: -dr -M insndesc

.*: +file format .*

Disassembly of section .text:

.* <foo>:
.*:	b3 95 00 69 [	 ]*cdfbr	%f6,%r9	# convert from fixed 32 to long bfp
 *([\da-f]+):	84 69 00 00 [	 ]*brxh	%r6,%r9,\1 <foo\+0x\1>	# branch relative on index high
.*:	b2 99 5f ff [	 ]*srnm	4095\(%r5\)	# set rounding mode
.*:	b9 11 00 96 [	 ]*lngfr	%r9,%r6	# load negative 64<32
.*:	ec 67 92 1c 26 54 [	 ]*rnsbgt	%r6,%r7,18,28,38	# rotate then and selected bits and test results
.*:	ec 67 0c 8d 0e 5d [	 ]*risbhgz	%r6,%r7,12,13,14	# rotate then insert selected bits high and zero remaining bits
.*:	b3 96 37 59 [	 ]*cxfbra	%f5,3,%r9,7	# convert from 32 bit fixed to extended bfp with rounding mode
.*:	ec 67 0c 94 0e 59 [	 ]*risbgnz	%r6,%r7,12,20,14	# rotate then insert selected bits and zero remaining bits nocc
.*:	ec 6e 80 03 00 4e [	 ]*lochhino	%r6,-32765	# load halfword high immediate on condition on not overflow / if not ones
 *([\da-f]+):	ec 6a 00 00 d6 7c [	 ]*cgijnl	%r6,-42,\1 <foo\+0x\1>	# compare immediate and branch relative \(64<8\) on A not low
.*:	07 07 [	 ]*nopr	%r7	# no operation
