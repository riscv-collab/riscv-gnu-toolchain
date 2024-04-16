#objdump: -dw
#name: x86_64 SHA512 insns
#source: x86-64-sha512.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 c2 7f cc f7\s+vsha512msg1 %xmm15,%ymm6
\s*[a-f0-9]+:\s*c4 62 7f cd fd\s+vsha512msg2 %ymm5,%ymm15
\s*[a-f0-9]+:\s*c4 62 57 cb f4\s+vsha512rnds2 %xmm4,%ymm5,%ymm14
\s*[a-f0-9]+:\s*c4 c2 7f cc f7\s+vsha512msg1 %xmm15,%ymm6
\s*[a-f0-9]+:\s*c4 62 7f cd fd\s+vsha512msg2 %ymm5,%ymm15
\s*[a-f0-9]+:\s*c4 62 57 cb f4\s+vsha512rnds2 %xmm4,%ymm5,%ymm14
#pass
