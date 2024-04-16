#as:
#objdump: -dw
#name: x86-64 APX old evex insn use gpr32 with extend-evex prefix
#source: x86-64-apx-evex-egpr.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
\s*[a-f0-9]+:\s*62 fb 79 48 19 04 08 01[	 ]+vextractf32x4 \$0x1,%zmm0,\(%r16,%r17,1\)
\s*[a-f0-9]+:\s*62 fa 79 48 5a 04 1a[	 ]+vbroadcasti32x4 \(%r18,%r19,1\),%zmm0
\s*[a-f0-9]+:\s*62 eb 7d 08 17 c4 01[	 ]+vextractps \$0x1,%xmm16,%r20d
\s*[a-f0-9]+:\s*62 69 97 00 2a f5[	 ]+vcvtsi2sd %r21,%xmm29,%xmm30
\s*[a-f0-9]+:\s*67 62 fe 55 58 96 36[	 ]+vfmaddsub132ph \(%r22d\)\{1to32\},%zmm5,%zmm6
\s*[a-f0-9]+:\s*62 81 fe 18 78 fe[	 ]+vcvttss2usi \{sae\},%xmm30,%r23
\s*[a-f0-9]+:\s*62 25 10 47 58 b4 c5 00 00 00 10[	 ]+vaddph 0x10000000\(%rbp,%r24,8\),%zmm29,%zmm30\{%k7\}
\s*[a-f0-9]+:\s*62 4d 7c 08 2f 71 7f[	 ]+vcomish 0xfe\(%r25\),%xmm30
\s*[a-f0-9]+:\s*62 d9 fd 08 6e 52 01[	 ]+vmovq  0x8\(%r26\),%xmm2
#pass
