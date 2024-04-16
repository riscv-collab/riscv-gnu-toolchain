#objdump: -dw
#name: i386 AVX512F insns with nondefault values in ignored / reserved bits

.*: +file format .*


Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	62 f3 d5 1f 0b f4 7b 	vrndscalesd \$0x7b,\{sae\},%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f3 d5 5f 0b f4 7b 	vrndscalesd \$0x7b,\{sae\},%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 55 4f 3b f4    	vpminud %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 c2 55 4f 3b f4    	vpminud %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 55 1f 3b f4    	vpminud \{rn-bad\},%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7e 48 31 72 7f 	vpmovdb %zmm6,0x7f0\(%edx\)
[ 	]*[a-f0-9]+:	62 f2 7e 58 31 72 7f 	vpmovdb %zmm6,0x7f0\(%edx\)\{bad\}
[ 	]*[a-f0-9]+:	62 f1 7c 88 58 c3    	(\{evex\} )?vaddps %xmm3,%xmm0,%xmm0\{bad\}
[ 	]*[a-f0-9]+:	62 f2 7d 4f 92 01    	vgatherdps \(bad\),%zmm0\{%k7\}
[ 	]*[a-f0-9]+:	67 62 f2 7d 4f 92 01 	addr16 vgatherdps \(bad\),%zmm0\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7d cf 92 04 08 	vgatherdps \(%eax,%zmm1(,1)?\),%zmm0\{%k7\}\{z\}/\(bad\)
[ 	]*[a-f0-9]+:	62 f2 7d 48 92 04 08 	vgatherdps \(%eax,%zmm1(,1)?\),%zmm0/\(bad\)
[ 	]*[a-f0-9]+:	62 f1 7c cf c2 c0 00 	vcmpeqps %zmm0,%zmm0,%k0\{%k7\}\{z\}/\(bad\)
[ 	]*[a-f0-9]+:	62 f1 7c cf 29 00    	vmovaps %zmm0,\(%eax\)\{%k7\}\{z\}/\(bad\)
[ 	]*[a-f0-9]+:	62 f1 7d 0a c5 c8 00 	vpextrw \$(0x)?0,%xmm0,%ecx\{%k2\}/\(bad\)
[ 	]*[a-f0-9]+:	62 f3 7d 0a 16 01 00 	vpextrd \$(0x)?0,%xmm0,\(%ecx\)\{%k2\}/\(bad\)
[ 	]*[a-f0-9]+:	62 f2 7d 4a 2a 01    	vmovntdqa \(%ecx\),%zmm0\{%k2\}/\(bad\)
#pass
