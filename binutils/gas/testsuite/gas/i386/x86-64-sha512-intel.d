#objdump: -dw -Mintel
#name: x86_64 SHA512 insns (Intel disassembly)
#source: x86-64-sha512.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 c2 7f cc f7\s+vsha512msg1 ymm6,xmm15
\s*[a-f0-9]+:\s*c4 62 7f cd fd\s+vsha512msg2 ymm15,ymm5
\s*[a-f0-9]+:\s*c4 62 57 cb f4\s+vsha512rnds2 ymm14,ymm5,xmm4
\s*[a-f0-9]+:\s*c4 c2 7f cc f7\s+vsha512msg1 ymm6,xmm15
\s*[a-f0-9]+:\s*c4 62 7f cd fd\s+vsha512msg2 ymm15,ymm5
\s*[a-f0-9]+:\s*c4 62 57 cb f4\s+vsha512rnds2 ymm14,ymm5,xmm4
#pass
