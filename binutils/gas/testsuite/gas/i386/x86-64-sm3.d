#objdump: -dw
#name: x86_64 SM3 insns
#source: x86-64-sm3.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 c2 50 da f6\s+vsm3msg1 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 00 da b4 f5 00 00 00 10\s+vsm3msg1 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 00 da 31\s+vsm3msg1 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 51 da f6\s+vsm3msg2 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 01 da b4 f5 00 00 00 10\s+vsm3msg2 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 01 da 31\s+vsm3msg2 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c3 51 de f6 7b\s+vsm3rnds2 \$0x7b,%xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a3 01 de b4 f5 00 00 00 10 7b\s+vsm3rnds2 \$0x7b,0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c3 01 de 31 7b\s+vsm3rnds2 \$0x7b,\(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 50 da f6\s+vsm3msg1 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 00 da b4 f5 00 00 00 10\s+vsm3msg1 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 00 da 31\s+vsm3msg1 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 51 da f6\s+vsm3msg2 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 01 da b4 f5 00 00 00 10\s+vsm3msg2 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 01 da 31\s+vsm3msg2 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c3 51 de f6 7b\s+vsm3rnds2 \$0x7b,%xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a3 01 de b4 f5 00 00 00 10 7b\s+vsm3rnds2 \$0x7b,0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c3 01 de 31 7b\s+vsm3rnds2 \$0x7b,\(%r9\),%xmm15,%xmm6
#pass
