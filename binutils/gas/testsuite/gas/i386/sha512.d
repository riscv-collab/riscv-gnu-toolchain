#objdump: -dw
#name: i386 SHA512 insns
#source: sha512.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 7f cc f5\s+vsha512msg1 %xmm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 7f cd f5\s+vsha512msg2 %ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 57 cb f4\s+vsha512rnds2 %xmm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 7f cc f5\s+vsha512msg1 %xmm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 7f cd f5\s+vsha512msg2 %ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 57 cb f4\s+vsha512rnds2 %xmm4,%ymm5,%ymm6
#pass
