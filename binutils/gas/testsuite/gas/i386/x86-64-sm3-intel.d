#objdump: -dw -Mintel
#name: x86_64 SM3 insns (Intel disassembly)
#source: x86-64-sm3.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 c2 50 da f6\s+vsm3msg1 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 00 da b4 f5 00 00 00 10\s+vsm3msg1 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 00 da 31\s+vsm3msg1 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c2 51 da f6\s+vsm3msg2 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 01 da b4 f5 00 00 00 10\s+vsm3msg2 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 01 da 31\s+vsm3msg2 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c3 51 de f6 7b\s+vsm3rnds2 xmm6,xmm5,xmm14,0x7b
\s*[a-f0-9]+:\s*c4 a3 01 de b4 f5 00 00 00 10 7b\s+vsm3rnds2 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\],0x7b
\s*[a-f0-9]+:\s*c4 c3 01 de 31 7b\s+vsm3rnds2 xmm6,xmm15,XMMWORD PTR \[r9\],0x7b
\s*[a-f0-9]+:\s*c4 c2 50 da f6\s+vsm3msg1 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 00 da b4 f5 00 00 00 10\s+vsm3msg1 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 00 da 31\s+vsm3msg1 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c2 51 da f6\s+vsm3msg2 xmm6,xmm5,xmm14
\s*[a-f0-9]+:\s*c4 a2 01 da b4 f5 00 00 00 10\s+vsm3msg2 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\]
\s*[a-f0-9]+:\s*c4 c2 01 da 31\s+vsm3msg2 xmm6,xmm15,XMMWORD PTR \[r9\]
\s*[a-f0-9]+:\s*c4 c3 51 de f6 7b\s+vsm3rnds2 xmm6,xmm5,xmm14,0x7b
\s*[a-f0-9]+:\s*c4 a3 01 de b4 f5 00 00 00 10 7b\s+vsm3rnds2 xmm6,xmm15,XMMWORD PTR \[rbp\+r14\*8\+0x10000000\],0x7b
\s*[a-f0-9]+:\s*c4 c3 01 de 31 7b\s+vsm3rnds2 xmm6,xmm15,XMMWORD PTR \[r9\],0x7b
#pass
