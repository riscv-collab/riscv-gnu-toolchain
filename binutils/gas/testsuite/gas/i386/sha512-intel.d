#objdump: -dw -Mintel
#name: i386 SHA512 insns (Intel disassembly)
#source: sha512.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 7f cc f5\s+vsha512msg1 ymm6,xmm5
\s*[a-f0-9]+:\s*c4 e2 7f cd f5\s+vsha512msg2 ymm6,ymm5
\s*[a-f0-9]+:\s*c4 e2 57 cb f4\s+vsha512rnds2 ymm6,ymm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 7f cc f5\s+vsha512msg1 ymm6,xmm5
\s*[a-f0-9]+:\s*c4 e2 7f cd f5\s+vsha512msg2 ymm6,ymm5
\s*[a-f0-9]+:\s*c4 e2 57 cb f4\s+vsha512rnds2 ymm6,ymm5,xmm4
#pass
