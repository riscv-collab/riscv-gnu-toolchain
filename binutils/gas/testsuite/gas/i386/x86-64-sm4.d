#objdump: -dw
#name: x86_64 SM4 insns
#source: x86-64-sm4.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 c2 56 da f6\s+vsm4key4 %ymm14,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 52 da f6\s+vsm4key4 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 06 da b4 f5 00 00 00 10\s+vsm4key4 0x10000000\(%rbp,%r14,8\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 c2 06 da 31\s+vsm4key4 \(%r9\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 a2 02 da b4 f5 00 00 00 10\s+vsm4key4 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 02 da 31\s+vsm4key4 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 57 da f6\s+vsm4rnds4 %ymm14,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 53 da f6\s+vsm4rnds4 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 07 da b4 f5 00 00 00 10\s+vsm4rnds4 0x10000000\(%rbp,%r14,8\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 c2 07 da 31\s+vsm4rnds4 \(%r9\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 a2 03 da b4 f5 00 00 00 10\s+vsm4rnds4 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 03 da 31\s+vsm4rnds4 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 56 da f6\s+vsm4key4 %ymm14,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 52 da f6\s+vsm4key4 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 06 da b4 f5 00 00 00 10\s+vsm4key4 0x10000000\(%rbp,%r14,8\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 c2 06 da 31\s+vsm4key4 \(%r9\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 a2 02 da b4 f5 00 00 00 10\s+vsm4key4 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 02 da 31\s+vsm4key4 \(%r9\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 57 da f6\s+vsm4rnds4 %ymm14,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 53 da f6\s+vsm4rnds4 %xmm14,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 07 da b4 f5 00 00 00 10\s+vsm4rnds4 0x10000000\(%rbp,%r14,8\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 c2 07 da 31\s+vsm4rnds4 \(%r9\),%ymm15,%ymm6
\s*[a-f0-9]+:\s*c4 a2 03 da b4 f5 00 00 00 10\s+vsm4rnds4 0x10000000\(%rbp,%r14,8\),%xmm15,%xmm6
\s*[a-f0-9]+:\s*c4 c2 03 da 31\s+vsm4rnds4 \(%r9\),%xmm15,%xmm6
