#objdump: -dw -Mintel
#name: x86_64 SM4 insns (Intel disassembly)
#source: x86-64-sm4.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 c2 56 da f6\s+vsm4key4 ymm6,ymm5,ymm14
\s*[a-f0-9]+:\s*c4 c2 52 da f6\s+vsm4key4 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 06 da b4 f5 00 00 00 10\s+vsm4key4 ymm6,ymm15,YMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 06 da 31\s+vsm4key4 ymm6,ymm15,YMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 a2 02 da b4 f5 00 00 00 10\s+vsm4key4 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 02 da 31\s+vsm4key4 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c2 57 da f6\s+vsm4rnds4 ymm6,ymm5,ymm14
\s*[a-f0-9]+:\s*c4 c2 53 da f6\s+vsm4rnds4 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 07 da b4 f5 00 00 00 10\s+vsm4rnds4 ymm6,ymm15,YMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 07 da 31\s+vsm4rnds4 ymm6,ymm15,YMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 a2 03 da b4 f5 00 00 00 10\s+vsm4rnds4 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 03 da 31\s+vsm4rnds4 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c2 56 da f6\s+vsm4key4 ymm6,ymm5,ymm14
\s*[a-f0-9]+:\s*c4 c2 52 da f6\s+vsm4key4 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 06 da b4 f5 00 00 00 10\s+vsm4key4 ymm6,ymm15,YMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 06 da 31\s+vsm4key4 ymm6,ymm15,YMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 a2 02 da b4 f5 00 00 00 10\s+vsm4key4 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 02 da 31\s+vsm4key4 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c2 57 da f6\s+vsm4rnds4 ymm6,ymm5,ymm14
\s*[a-f0-9]+:\s*c4 c2 53 da f6\s+vsm4rnds4 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 07 da b4 f5 00 00 00 10\s+vsm4rnds4 ymm6,ymm15,YMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 07 da 31\s+vsm4rnds4 ymm6,ymm15,YMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 a2 03 da b4 f5 00 00 00 10\s+vsm4rnds4 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 03 da 31\s+vsm4rnds4 xmm6,xmm15,XMMWORD PTR \[r9\]
