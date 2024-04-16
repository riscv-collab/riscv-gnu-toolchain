#objdump: -dw -Mintel
#name: i386 SM3 insns (Intel disassembly)
#source: sm3.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 50 da f4\s+vsm3msg1 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 50 da b4 f4 00 00 00 10\s+vsm3msg1 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 50 da 31\s+vsm3msg1 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 51 da f4\s+vsm3msg2 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 51 da b4 f4 00 00 00 10\s+vsm3msg2 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 51 da 31\s+vsm3msg2 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e3 51 de f4 7b\s+vsm3rnds2 xmm6,xmm5,xmm4,0x7b
\s*[a-f0-9]+:\s*c4 e3 51 de b4 f4 00 00 00 10 7b\s+vsm3rnds2 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\],0x7b
\s*[a-f0-9]+:\s*c4 e3 51 de 31 7b\s+vsm3rnds2 xmm6,xmm5,XMMWORD PTR \[ecx\],0x7b
\s*[a-f0-9]+:\s*c4 e2 50 da f4\s+vsm3msg1 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 50 da b4 f4 00 00 00 10\s+vsm3msg1 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 50 da 31\s+vsm3msg1 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e2 51 da f4\s+vsm3msg2 xmm6,xmm5,xmm4
\s*[a-f0-9]+:\s*c4 e2 51 da b4 f4 00 00 00 10\s+vsm3msg2 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 e2 51 da 31\s+vsm3msg2 xmm6,xmm5,XMMWORD PTR \[ecx\]
\s*[a-f0-9]+:\s*c4 e3 51 de f4 7b\s+vsm3rnds2 xmm6,xmm5,xmm4,0x7b
\s*[a-f0-9]+:\s*c4 e3 51 de b4 f4 00 00 00 10 7b\s+vsm3rnds2 xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8\+0x10000000\],0x7b
\s*[a-f0-9]+:\s*c4 e3 51 de 31 7b\s+vsm3rnds2 xmm6,xmm5,XMMWORD PTR \[ecx\],0x7b
#pass
