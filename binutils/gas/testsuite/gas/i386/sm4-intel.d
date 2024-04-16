#objdump: -dw -Mintel
#name: i386 SM4 insns (Intel disassembly)
#source: sm4.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 56 da f4\s+vsm4key4 ymm6,ymm5,ymm4
\s*[a-f0-9]+:\s*c4 e2 52 da f4\s+vsm4key4 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 56 da b4 f4 00 00 00 10\s+vsm4key4 ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 56 da 31\s+vsm4key4 ymm6,ymm5,YMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 52 da b4 f4 00 00 00 10\s+vsm4key4 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 52 da 31\s+vsm4key4 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 57 da f4\s+vsm4rnds4 ymm6,ymm5,ymm4
\s*[a-f0-9]+:\s*c4 e2 53 da f4\s+vsm4rnds4 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 57 da b4 f4 00 00 00 10\s+vsm4rnds4 ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 57 da 31\s+vsm4rnds4 ymm6,ymm5,YMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 53 da b4 f4 00 00 00 10\s+vsm4rnds4 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 53 da 31\s+vsm4rnds4 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 56 da f4\s+vsm4key4 ymm6,ymm5,ymm4
\s*[a-f0-9]+:\s*c4 e2 52 da f4\s+vsm4key4 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 56 da b4 f4 00 00 00 10\s+vsm4key4 ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 56 da 31\s+vsm4key4 ymm6,ymm5,YMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 52 da b4 f4 00 00 00 10\s+vsm4key4 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 52 da 31\s+vsm4key4 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 57 da f4\s+vsm4rnds4 ymm6,ymm5,ymm4
\s*[a-f0-9]+:\s*c4 e2 53 da f4\s+vsm4rnds4 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 57 da b4 f4 00 00 00 10\s+vsm4rnds4 ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 57 da 31\s+vsm4rnds4 ymm6,ymm5,YMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 53 da b4 f4 00 00 00 10\s+vsm4rnds4 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 53 da 31\s+vsm4rnds4 xmm6,xmm5,XMMWORD PTR \[ecx\]
