#objdump: -dw
#name: i386 SM4 insns
#source: sm4.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 56 da f4\s+vsm4key4 %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 da f4\s+vsm4key4 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 da b4 f4 00 00 00 10\s+vsm4key4 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 da 31\s+vsm4key4 \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 da b4 f4 00 00 00 10\s+vsm4key4 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 da 31\s+vsm4key4 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 57 da f4\s+vsm4rnds4 %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 53 da f4\s+vsm4rnds4 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 57 da b4 f4 00 00 00 10\s+vsm4rnds4 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 57 da 31\s+vsm4rnds4 \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 53 da b4 f4 00 00 00 10\s+vsm4rnds4 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 53 da 31\s+vsm4rnds4 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 da f4\s+vsm4key4 %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 da f4\s+vsm4key4 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 da b4 f4 00 00 00 10\s+vsm4key4 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 da 31\s+vsm4key4 \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 da b4 f4 00 00 00 10\s+vsm4key4 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 da 31\s+vsm4key4 \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 57 da f4\s+vsm4rnds4 %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 53 da f4\s+vsm4rnds4 %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 57 da b4 f4 00 00 00 10\s+vsm4rnds4 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 57 da 31\s+vsm4rnds4 \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 53 da b4 f4 00 00 00 10\s+vsm4rnds4 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 53 da 31\s+vsm4rnds4 \(%ecx\),%xmm5,%xmm6
