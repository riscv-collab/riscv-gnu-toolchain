#objdump: -dw
#name: i386 SM3 insns
#source: sm3.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 50 da f4\s+vsm3msg1 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 da b4 f4 00 00 00 10\s+vsm3msg1 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 da 31\s+vsm3msg1 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 da f4\s+vsm3msg2 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 da b4 f4 00 00 00 10\s+vsm3msg2 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 da 31\s+vsm3msg2 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e3 51 de f4 7b\s+vsm3rnds2 \$0x7b,%xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e3 51 de b4 f4 00 00 00 10 7b\s+vsm3rnds2 \$0x7b,0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e3 51 de 31 7b\s+vsm3rnds2 \$0x7b,\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 da f4\s+vsm3msg1 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 da b4 f4 00 00 00 10\s+vsm3msg1 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 da 31\s+vsm3msg1 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 da f4\s+vsm3msg2 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 da b4 f4 00 00 00 10\s+vsm3msg2 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 da 31\s+vsm3msg2 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e3 51 de f4 7b\s+vsm3rnds2 \$0x7b,%xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e3 51 de b4 f4 00 00 00 10 7b\s+vsm3rnds2 \$0x7b,0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e3 51 de 31 7b\s+vsm3rnds2 \$0x7b,\(%ecx\),%xmm5,%xmm6
#pass
