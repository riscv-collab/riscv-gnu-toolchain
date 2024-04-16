#as: --divide -I${srcdir}/$subdir
#objdump: -dw
#name: AVX10.1/256 (part 5)

.*: +file format .*


Disassembly of section \.text:

0+ <bitalg>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8f ec[ 	]*vpshufbitqmb %xmm4,%xmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8f ac f4 c0 1d fe ff[ 	]*vpshufbitqmb -0x1e240\(%esp,%esi,8\),%xmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8f 6a 7f[ 	]*vpshufbitqmb 0x7f0\(%edx\),%xmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8f ec[ 	]*vpshufbitqmb %ymm4,%ymm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8f ac f4 c0 1d fe ff[ 	]*vpshufbitqmb -0x1e240\(%esp,%esi,8\),%ymm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8f 6a 7f[ 	]*vpshufbitqmb 0xfe0\(%edx\),%ymm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 54 f5[ 	]*vpopcntb %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 54 f5[ 	]*vpopcntb %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntb -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 54 72 7f[ 	]*vpopcntb 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 54 f5[ 	]*vpopcntb %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 54 f5[ 	]*vpopcntb %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntb -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 54 72 7f[ 	]*vpopcntb 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 54 f5[ 	]*vpopcntw %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 54 f5[ 	]*vpopcntw %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntw -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 54 72 7f[ 	]*vpopcntw 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 54 f5[ 	]*vpopcntw %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 54 f5[ 	]*vpopcntw %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntw -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 54 72 7f[ 	]*vpopcntw 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 f5[ 	]*vpopcntd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 55 f5[ 	]*vpopcntd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 72 7f[ 	]*vpopcntd 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 f5[ 	]*vpopcntd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 55 f5[ 	]*vpopcntd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 72 7f[ 	]*vpopcntd 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 f5[ 	]*vpopcntq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 55 f5[ 	]*vpopcntq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 72 7f[ 	]*vpopcntq 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 f5[ 	]*vpopcntq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 55 f5[ 	]*vpopcntq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 72 7f[ 	]*vpopcntq 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8f ec[ 	]*vpshufbitqmb %xmm4,%xmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8f ac f4 c0 1d fe ff[ 	]*vpshufbitqmb -0x1e240\(%esp,%esi,8\),%xmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8f 6a 7f[ 	]*vpshufbitqmb 0x7f0\(%edx\),%xmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8f ec[ 	]*vpshufbitqmb %ymm4,%ymm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8f ac f4 c0 1d fe ff[ 	]*vpshufbitqmb -0x1e240\(%esp,%esi,8\),%ymm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8f 6a 7f[ 	]*vpshufbitqmb 0xfe0\(%edx\),%ymm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 54 f5[ 	]*vpopcntb %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 54 f5[ 	]*vpopcntb %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntb -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 54 72 7f[ 	]*vpopcntb 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 54 f5[ 	]*vpopcntb %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 54 f5[ 	]*vpopcntb %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntb -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 54 72 7f[ 	]*vpopcntb 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 54 f5[ 	]*vpopcntw %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 54 f5[ 	]*vpopcntw %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntw -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 54 72 7f[ 	]*vpopcntw 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 54 f5[ 	]*vpopcntw %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 54 f5[ 	]*vpopcntw %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 54 b4 f4 c0 1d fe ff[ 	]*vpopcntw -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 54 72 7f[ 	]*vpopcntw 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 f5[ 	]*vpopcntd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 55 f5[ 	]*vpopcntd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 72 7f[ 	]*vpopcntd 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 55 32[ 	]*vpopcntd \(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 f5[ 	]*vpopcntd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 55 f5[ 	]*vpopcntd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 72 7f[ 	]*vpopcntd 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 55 32[ 	]*vpopcntd \(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 f5[ 	]*vpopcntq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 55 f5[ 	]*vpopcntq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 72 7f[ 	]*vpopcntq 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 55 32[ 	]*vpopcntq \(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 f5[ 	]*vpopcntq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 55 f5[ 	]*vpopcntq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 72 7f[ 	]*vpopcntq 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 55 32[ 	]*vpopcntq \(%edx\)\{1to4\},%ymm6\{%k7\}

0+[a-f0-9]+ <cd>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 f5[ 	]*vpconflictd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f c4 f5[ 	]*vpconflictd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 31[ 	]*vpconflictd \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictd -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 30[ 	]*vpconflictd \(%eax\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 72 7f[ 	]*vpconflictd 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 b2 00 08 00 00[ 	]*vpconflictd 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 72 80[ 	]*vpconflictd -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 b2 f0 f7 ff ff[ 	]*vpconflictd -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 72 7f[ 	]*vpconflictd 0x1fc\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 b2 00 02 00 00[ 	]*vpconflictd 0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 72 80[ 	]*vpconflictd -0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 b2 fc fd ff ff[ 	]*vpconflictd -0x204\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 f5[ 	]*vpconflictd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af c4 f5[ 	]*vpconflictd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 31[ 	]*vpconflictd \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictd -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 30[ 	]*vpconflictd \(%eax\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 72 7f[ 	]*vpconflictd 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 b2 00 10 00 00[ 	]*vpconflictd 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 72 80[ 	]*vpconflictd -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 b2 e0 ef ff ff[ 	]*vpconflictd -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 72 7f[ 	]*vpconflictd 0x1fc\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 b2 00 02 00 00[ 	]*vpconflictd 0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 72 80[ 	]*vpconflictd -0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 b2 fc fd ff ff[ 	]*vpconflictd -0x204\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 f5[ 	]*vpconflictq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f c4 f5[ 	]*vpconflictq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 31[ 	]*vpconflictq \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictq -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 30[ 	]*vpconflictq \(%eax\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 72 7f[ 	]*vpconflictq 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 b2 00 08 00 00[ 	]*vpconflictq 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 72 80[ 	]*vpconflictq -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 b2 f0 f7 ff ff[ 	]*vpconflictq -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 72 7f[ 	]*vpconflictq 0x3f8\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 b2 00 04 00 00[ 	]*vpconflictq 0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 72 80[ 	]*vpconflictq -0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 b2 f8 fb ff ff[ 	]*vpconflictq -0x408\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 f5[ 	]*vpconflictq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af c4 f5[ 	]*vpconflictq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 31[ 	]*vpconflictq \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictq -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 30[ 	]*vpconflictq \(%eax\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 72 7f[ 	]*vpconflictq 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 b2 00 10 00 00[ 	]*vpconflictq 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 72 80[ 	]*vpconflictq -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 b2 e0 ef ff ff[ 	]*vpconflictq -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 72 7f[ 	]*vpconflictq 0x3f8\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 b2 00 04 00 00[ 	]*vpconflictq 0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 72 80[ 	]*vpconflictq -0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 b2 f8 fb ff ff[ 	]*vpconflictq -0x408\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 f5[ 	]*vplzcntd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 44 f5[ 	]*vplzcntd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 31[ 	]*vplzcntd \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntd -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 30[ 	]*vplzcntd \(%eax\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 72 7f[ 	]*vplzcntd 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 b2 00 08 00 00[ 	]*vplzcntd 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 72 80[ 	]*vplzcntd -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 b2 f0 f7 ff ff[ 	]*vplzcntd -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 72 7f[ 	]*vplzcntd 0x1fc\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 b2 00 02 00 00[ 	]*vplzcntd 0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 72 80[ 	]*vplzcntd -0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 b2 fc fd ff ff[ 	]*vplzcntd -0x204\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 f5[ 	]*vplzcntd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 44 f5[ 	]*vplzcntd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 31[ 	]*vplzcntd \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntd -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 30[ 	]*vplzcntd \(%eax\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 72 7f[ 	]*vplzcntd 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 b2 00 10 00 00[ 	]*vplzcntd 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 72 80[ 	]*vplzcntd -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 b2 e0 ef ff ff[ 	]*vplzcntd -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 72 7f[ 	]*vplzcntd 0x1fc\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 b2 00 02 00 00[ 	]*vplzcntd 0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 72 80[ 	]*vplzcntd -0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 b2 fc fd ff ff[ 	]*vplzcntd -0x204\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 f5[ 	]*vplzcntq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 44 f5[ 	]*vplzcntq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 31[ 	]*vplzcntq \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntq -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 30[ 	]*vplzcntq \(%eax\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 72 7f[ 	]*vplzcntq 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 b2 00 08 00 00[ 	]*vplzcntq 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 72 80[ 	]*vplzcntq -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 b2 f0 f7 ff ff[ 	]*vplzcntq -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 72 7f[ 	]*vplzcntq 0x3f8\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 b2 00 04 00 00[ 	]*vplzcntq 0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 72 80[ 	]*vplzcntq -0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 b2 f8 fb ff ff[ 	]*vplzcntq -0x408\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 f5[ 	]*vplzcntq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 44 f5[ 	]*vplzcntq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 31[ 	]*vplzcntq \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntq -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 30[ 	]*vplzcntq \(%eax\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 72 7f[ 	]*vplzcntq 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 b2 00 10 00 00[ 	]*vplzcntq 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 72 80[ 	]*vplzcntq -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 b2 e0 ef ff ff[ 	]*vplzcntq -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 72 7f[ 	]*vplzcntq 0x3f8\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 b2 00 04 00 00[ 	]*vplzcntq 0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 72 80[ 	]*vplzcntq -0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 b2 f8 fb ff ff[ 	]*vplzcntq -0x408\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7e 08 3a f6[ 	]*vpbroadcastmw2d %k6,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7e 28 3a f6[ 	]*vpbroadcastmw2d %k6,%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fe 08 2a f6[ 	]*vpbroadcastmb2q %k6,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fe 28 2a f6[ 	]*vpbroadcastmb2q %k6,%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 f5[ 	]*vpconflictd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f c4 f5[ 	]*vpconflictd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 31[ 	]*vpconflictd \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictd -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 30[ 	]*vpconflictd \(%eax\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 72 7f[ 	]*vpconflictd 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 b2 00 08 00 00[ 	]*vpconflictd 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 72 80[ 	]*vpconflictd -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f c4 b2 f0 f7 ff ff[ 	]*vpconflictd -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 72 7f[ 	]*vpconflictd 0x1fc\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 b2 00 02 00 00[ 	]*vpconflictd 0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 72 80[ 	]*vpconflictd -0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f c4 b2 fc fd ff ff[ 	]*vpconflictd -0x204\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 f5[ 	]*vpconflictd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af c4 f5[ 	]*vpconflictd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 31[ 	]*vpconflictd \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictd -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 30[ 	]*vpconflictd \(%eax\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 72 7f[ 	]*vpconflictd 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 b2 00 10 00 00[ 	]*vpconflictd 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 72 80[ 	]*vpconflictd -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f c4 b2 e0 ef ff ff[ 	]*vpconflictd -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 72 7f[ 	]*vpconflictd 0x1fc\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 b2 00 02 00 00[ 	]*vpconflictd 0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 72 80[ 	]*vpconflictd -0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f c4 b2 fc fd ff ff[ 	]*vpconflictd -0x204\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 f5[ 	]*vpconflictq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f c4 f5[ 	]*vpconflictq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 31[ 	]*vpconflictq \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictq -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 30[ 	]*vpconflictq \(%eax\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 72 7f[ 	]*vpconflictq 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 b2 00 08 00 00[ 	]*vpconflictq 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 72 80[ 	]*vpconflictq -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f c4 b2 f0 f7 ff ff[ 	]*vpconflictq -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 72 7f[ 	]*vpconflictq 0x3f8\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 b2 00 04 00 00[ 	]*vpconflictq 0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 72 80[ 	]*vpconflictq -0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f c4 b2 f8 fb ff ff[ 	]*vpconflictq -0x408\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 f5[ 	]*vpconflictq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af c4 f5[ 	]*vpconflictq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 31[ 	]*vpconflictq \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 b4 f4 c0 1d fe ff[ 	]*vpconflictq -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 30[ 	]*vpconflictq \(%eax\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 72 7f[ 	]*vpconflictq 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 b2 00 10 00 00[ 	]*vpconflictq 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 72 80[ 	]*vpconflictq -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f c4 b2 e0 ef ff ff[ 	]*vpconflictq -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 72 7f[ 	]*vpconflictq 0x3f8\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 b2 00 04 00 00[ 	]*vpconflictq 0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 72 80[ 	]*vpconflictq -0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f c4 b2 f8 fb ff ff[ 	]*vpconflictq -0x408\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 f5[ 	]*vplzcntd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 44 f5[ 	]*vplzcntd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 31[ 	]*vplzcntd \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntd -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 30[ 	]*vplzcntd \(%eax\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 72 7f[ 	]*vplzcntd 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 b2 00 08 00 00[ 	]*vplzcntd 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 72 80[ 	]*vplzcntd -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 44 b2 f0 f7 ff ff[ 	]*vplzcntd -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 72 7f[ 	]*vplzcntd 0x1fc\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 b2 00 02 00 00[ 	]*vplzcntd 0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 72 80[ 	]*vplzcntd -0x200\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 1f 44 b2 fc fd ff ff[ 	]*vplzcntd -0x204\(%edx\)\{1to4\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 f5[ 	]*vplzcntd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 44 f5[ 	]*vplzcntd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 31[ 	]*vplzcntd \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntd -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 30[ 	]*vplzcntd \(%eax\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 72 7f[ 	]*vplzcntd 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 b2 00 10 00 00[ 	]*vplzcntd 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 72 80[ 	]*vplzcntd -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 44 b2 e0 ef ff ff[ 	]*vplzcntd -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 72 7f[ 	]*vplzcntd 0x1fc\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 b2 00 02 00 00[ 	]*vplzcntd 0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 72 80[ 	]*vplzcntd -0x200\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 3f 44 b2 fc fd ff ff[ 	]*vplzcntd -0x204\(%edx\)\{1to8\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 f5[ 	]*vplzcntq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 44 f5[ 	]*vplzcntq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 31[ 	]*vplzcntq \(%ecx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntq -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 30[ 	]*vplzcntq \(%eax\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 72 7f[ 	]*vplzcntq 0x7f0\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 b2 00 08 00 00[ 	]*vplzcntq 0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 72 80[ 	]*vplzcntq -0x800\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 44 b2 f0 f7 ff ff[ 	]*vplzcntq -0x810\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 72 7f[ 	]*vplzcntq 0x3f8\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 b2 00 04 00 00[ 	]*vplzcntq 0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 72 80[ 	]*vplzcntq -0x400\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 1f 44 b2 f8 fb ff ff[ 	]*vplzcntq -0x408\(%edx\)\{1to2\},%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 f5[ 	]*vplzcntq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 44 f5[ 	]*vplzcntq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 31[ 	]*vplzcntq \(%ecx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 b4 f4 c0 1d fe ff[ 	]*vplzcntq -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 30[ 	]*vplzcntq \(%eax\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 72 7f[ 	]*vplzcntq 0xfe0\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 b2 00 10 00 00[ 	]*vplzcntq 0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 72 80[ 	]*vplzcntq -0x1000\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 44 b2 e0 ef ff ff[ 	]*vplzcntq -0x1020\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 72 7f[ 	]*vplzcntq 0x3f8\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 b2 00 04 00 00[ 	]*vplzcntq 0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 72 80[ 	]*vplzcntq -0x400\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 3f 44 b2 f8 fb ff ff[ 	]*vplzcntq -0x408\(%edx\)\{1to4\},%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7e 08 3a f6[ 	]*vpbroadcastmw2d %k6,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7e 28 3a f6[ 	]*vpbroadcastmw2d %k6,%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fe 08 2a f6[ 	]*vpbroadcastmb2q %k6,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fe 28 2a f6[ 	]*vpbroadcastmb2q %k6,%ymm6

0+[a-f0-9]+ <ifma>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 f4[ 	]*vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f b4 f4[ 	]*vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 31[ 	]*vpmadd52luq \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 b4 f4 c0 1d fe ff[ 	]*vpmadd52luq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 30[ 	]*vpmadd52luq \(%eax\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 72 7f[ 	]*vpmadd52luq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 b2 00 08 00 00[ 	]*vpmadd52luq 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 72 80[ 	]*vpmadd52luq -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 b2 f0 f7 ff ff[ 	]*vpmadd52luq -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 72 7f[ 	]*vpmadd52luq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 b2 00 04 00 00[ 	]*vpmadd52luq 0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 72 80[ 	]*vpmadd52luq -0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 b2 f8 fb ff ff[ 	]*vpmadd52luq -0x408\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 f4[ 	]*vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af b4 f4[ 	]*vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 31[ 	]*vpmadd52luq \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 b4 f4 c0 1d fe ff[ 	]*vpmadd52luq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 30[ 	]*vpmadd52luq \(%eax\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 72 7f[ 	]*vpmadd52luq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 b2 00 10 00 00[ 	]*vpmadd52luq 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 72 80[ 	]*vpmadd52luq -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 b2 e0 ef ff ff[ 	]*vpmadd52luq -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 72 7f[ 	]*vpmadd52luq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 b2 00 04 00 00[ 	]*vpmadd52luq 0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 72 80[ 	]*vpmadd52luq -0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 b2 f8 fb ff ff[ 	]*vpmadd52luq -0x408\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 f4[ 	]*vpmadd52huq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f b5 f4[ 	]*vpmadd52huq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 31[ 	]*vpmadd52huq \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 b4 f4 c0 1d fe ff[ 	]*vpmadd52huq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 30[ 	]*vpmadd52huq \(%eax\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 72 7f[ 	]*vpmadd52huq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 b2 00 08 00 00[ 	]*vpmadd52huq 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 72 80[ 	]*vpmadd52huq -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 b2 f0 f7 ff ff[ 	]*vpmadd52huq -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 72 7f[ 	]*vpmadd52huq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 b2 00 04 00 00[ 	]*vpmadd52huq 0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 72 80[ 	]*vpmadd52huq -0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 b2 f8 fb ff ff[ 	]*vpmadd52huq -0x408\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 f4[ 	]*vpmadd52huq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af b5 f4[ 	]*vpmadd52huq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 31[ 	]*vpmadd52huq \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 b4 f4 c0 1d fe ff[ 	]*vpmadd52huq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 30[ 	]*vpmadd52huq \(%eax\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 72 7f[ 	]*vpmadd52huq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 b2 00 10 00 00[ 	]*vpmadd52huq 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 72 80[ 	]*vpmadd52huq -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 b2 e0 ef ff ff[ 	]*vpmadd52huq -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 72 7f[ 	]*vpmadd52huq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 b2 00 04 00 00[ 	]*vpmadd52huq 0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 72 80[ 	]*vpmadd52huq -0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 b2 f8 fb ff ff[ 	]*vpmadd52huq -0x408\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 f4[ 	]*vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f b4 f4[ 	]*vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 31[ 	]*vpmadd52luq \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 b4 f4 c0 1d fe ff[ 	]*vpmadd52luq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 30[ 	]*vpmadd52luq \(%eax\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 72 7f[ 	]*vpmadd52luq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 b2 00 08 00 00[ 	]*vpmadd52luq 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 72 80[ 	]*vpmadd52luq -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b4 b2 f0 f7 ff ff[ 	]*vpmadd52luq -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 72 7f[ 	]*vpmadd52luq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 b2 00 04 00 00[ 	]*vpmadd52luq 0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 72 80[ 	]*vpmadd52luq -0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b4 b2 f8 fb ff ff[ 	]*vpmadd52luq -0x408\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 f4[ 	]*vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af b4 f4[ 	]*vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 31[ 	]*vpmadd52luq \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 b4 f4 c0 1d fe ff[ 	]*vpmadd52luq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 30[ 	]*vpmadd52luq \(%eax\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 72 7f[ 	]*vpmadd52luq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 b2 00 10 00 00[ 	]*vpmadd52luq 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 72 80[ 	]*vpmadd52luq -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b4 b2 e0 ef ff ff[ 	]*vpmadd52luq -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 72 7f[ 	]*vpmadd52luq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 b2 00 04 00 00[ 	]*vpmadd52luq 0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 72 80[ 	]*vpmadd52luq -0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b4 b2 f8 fb ff ff[ 	]*vpmadd52luq -0x408\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 f4[ 	]*vpmadd52huq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f b5 f4[ 	]*vpmadd52huq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 31[ 	]*vpmadd52huq \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 b4 f4 c0 1d fe ff[ 	]*vpmadd52huq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 30[ 	]*vpmadd52huq \(%eax\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 72 7f[ 	]*vpmadd52huq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 b2 00 08 00 00[ 	]*vpmadd52huq 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 72 80[ 	]*vpmadd52huq -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f b5 b2 f0 f7 ff ff[ 	]*vpmadd52huq -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 72 7f[ 	]*vpmadd52huq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 b2 00 04 00 00[ 	]*vpmadd52huq 0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 72 80[ 	]*vpmadd52huq -0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f b5 b2 f8 fb ff ff[ 	]*vpmadd52huq -0x408\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 f4[ 	]*vpmadd52huq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af b5 f4[ 	]*vpmadd52huq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 31[ 	]*vpmadd52huq \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 b4 f4 c0 1d fe ff[ 	]*vpmadd52huq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 30[ 	]*vpmadd52huq \(%eax\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 72 7f[ 	]*vpmadd52huq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 b2 00 10 00 00[ 	]*vpmadd52huq 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 72 80[ 	]*vpmadd52huq -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f b5 b2 e0 ef ff ff[ 	]*vpmadd52huq -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 72 7f[ 	]*vpmadd52huq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 b2 00 04 00 00[ 	]*vpmadd52huq 0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 72 80[ 	]*vpmadd52huq -0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f b5 b2 f8 fb ff ff[ 	]*vpmadd52huq -0x408\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}

0+[a-f0-9]+ <vbmi>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d f4[ 	]*vpermb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 8d f4[ 	]*vpermb %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d 31[ 	]*vpermb \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d b4 f4 c0 1d fe ff[ 	]*vpermb -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d 72 7f[ 	]*vpermb 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d b2 00 08 00 00[ 	]*vpermb 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d 72 80[ 	]*vpermb -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d b2 f0 f7 ff ff[ 	]*vpermb -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d f4[ 	]*vpermb %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 8d f4[ 	]*vpermb %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d 31[ 	]*vpermb \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d b4 f4 c0 1d fe ff[ 	]*vpermb -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d 72 7f[ 	]*vpermb 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d b2 00 10 00 00[ 	]*vpermb 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d 72 80[ 	]*vpermb -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d b2 e0 ef ff ff[ 	]*vpermb -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 f4[ 	]*vpermi2b %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 75 f4[ 	]*vpermi2b %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 31[ 	]*vpermi2b \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 b4 f4 c0 1d fe ff[ 	]*vpermi2b -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 72 7f[ 	]*vpermi2b 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 b2 00 08 00 00[ 	]*vpermi2b 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 72 80[ 	]*vpermi2b -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 b2 f0 f7 ff ff[ 	]*vpermi2b -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 f4[ 	]*vpermi2b %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 75 f4[ 	]*vpermi2b %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 31[ 	]*vpermi2b \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 b4 f4 c0 1d fe ff[ 	]*vpermi2b -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 72 7f[ 	]*vpermi2b 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 b2 00 10 00 00[ 	]*vpermi2b 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 72 80[ 	]*vpermi2b -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 b2 e0 ef ff ff[ 	]*vpermi2b -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d f4[ 	]*vpermt2b %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 7d f4[ 	]*vpermt2b %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d 31[ 	]*vpermt2b \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d b4 f4 c0 1d fe ff[ 	]*vpermt2b -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d 72 7f[ 	]*vpermt2b 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d b2 00 08 00 00[ 	]*vpermt2b 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d 72 80[ 	]*vpermt2b -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d b2 f0 f7 ff ff[ 	]*vpermt2b -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d f4[ 	]*vpermt2b %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 7d f4[ 	]*vpermt2b %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d 31[ 	]*vpermt2b \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d b4 f4 c0 1d fe ff[ 	]*vpermt2b -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d 72 7f[ 	]*vpermt2b 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d b2 00 10 00 00[ 	]*vpermt2b 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d 72 80[ 	]*vpermt2b -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d b2 e0 ef ff ff[ 	]*vpermt2b -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 f4[ 	]*vpmultishiftqb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 83 f4[ 	]*vpmultishiftqb %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 31[ 	]*vpmultishiftqb \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 b4 f4 c0 1d fe ff[ 	]*vpmultishiftqb -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 30[ 	]*vpmultishiftqb \(%eax\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 72 7f[ 	]*vpmultishiftqb 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 b2 00 08 00 00[ 	]*vpmultishiftqb 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 72 80[ 	]*vpmultishiftqb -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 b2 f0 f7 ff ff[ 	]*vpmultishiftqb -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 72 7f[ 	]*vpmultishiftqb 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 b2 00 04 00 00[ 	]*vpmultishiftqb 0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 72 80[ 	]*vpmultishiftqb -0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 b2 f8 fb ff ff[ 	]*vpmultishiftqb -0x408\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 f4[ 	]*vpmultishiftqb %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 83 f4[ 	]*vpmultishiftqb %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 31[ 	]*vpmultishiftqb \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 b4 f4 c0 1d fe ff[ 	]*vpmultishiftqb -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 30[ 	]*vpmultishiftqb \(%eax\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 72 7f[ 	]*vpmultishiftqb 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 b2 00 10 00 00[ 	]*vpmultishiftqb 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 72 80[ 	]*vpmultishiftqb -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 b2 e0 ef ff ff[ 	]*vpmultishiftqb -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 72 7f[ 	]*vpmultishiftqb 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 b2 00 04 00 00[ 	]*vpmultishiftqb 0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 72 80[ 	]*vpmultishiftqb -0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 b2 f8 fb ff ff[ 	]*vpmultishiftqb -0x408\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d f4[ 	]*vpermb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 8d f4[ 	]*vpermb %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d 31[ 	]*vpermb \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d b4 f4 c0 1d fe ff[ 	]*vpermb -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d 72 7f[ 	]*vpermb 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d b2 00 08 00 00[ 	]*vpermb 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d 72 80[ 	]*vpermb -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 8d b2 f0 f7 ff ff[ 	]*vpermb -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d f4[ 	]*vpermb %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 8d f4[ 	]*vpermb %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d 31[ 	]*vpermb \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d b4 f4 c0 1d fe ff[ 	]*vpermb -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d 72 7f[ 	]*vpermb 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d b2 00 10 00 00[ 	]*vpermb 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d 72 80[ 	]*vpermb -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 8d b2 e0 ef ff ff[ 	]*vpermb -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 f4[ 	]*vpermi2b %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 75 f4[ 	]*vpermi2b %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 31[ 	]*vpermi2b \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 b4 f4 c0 1d fe ff[ 	]*vpermi2b -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 72 7f[ 	]*vpermi2b 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 b2 00 08 00 00[ 	]*vpermi2b 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 72 80[ 	]*vpermi2b -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 75 b2 f0 f7 ff ff[ 	]*vpermi2b -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 f4[ 	]*vpermi2b %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 75 f4[ 	]*vpermi2b %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 31[ 	]*vpermi2b \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 b4 f4 c0 1d fe ff[ 	]*vpermi2b -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 72 7f[ 	]*vpermi2b 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 b2 00 10 00 00[ 	]*vpermi2b 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 72 80[ 	]*vpermi2b -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 75 b2 e0 ef ff ff[ 	]*vpermi2b -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d f4[ 	]*vpermt2b %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 7d f4[ 	]*vpermt2b %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d 31[ 	]*vpermt2b \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d b4 f4 c0 1d fe ff[ 	]*vpermt2b -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d 72 7f[ 	]*vpermt2b 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d b2 00 08 00 00[ 	]*vpermt2b 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d 72 80[ 	]*vpermt2b -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 7d b2 f0 f7 ff ff[ 	]*vpermt2b -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d f4[ 	]*vpermt2b %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 7d f4[ 	]*vpermt2b %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d 31[ 	]*vpermt2b \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d b4 f4 c0 1d fe ff[ 	]*vpermt2b -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d 72 7f[ 	]*vpermt2b 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d b2 00 10 00 00[ 	]*vpermt2b 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d 72 80[ 	]*vpermt2b -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 7d b2 e0 ef ff ff[ 	]*vpermt2b -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 f4[ 	]*vpmultishiftqb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 83 f4[ 	]*vpmultishiftqb %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 31[ 	]*vpmultishiftqb \(%ecx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 b4 f4 c0 1d fe ff[ 	]*vpmultishiftqb -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 30[ 	]*vpmultishiftqb \(%eax\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 72 7f[ 	]*vpmultishiftqb 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 b2 00 08 00 00[ 	]*vpmultishiftqb 0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 72 80[ 	]*vpmultishiftqb -0x800\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 83 b2 f0 f7 ff ff[ 	]*vpmultishiftqb -0x810\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 72 7f[ 	]*vpmultishiftqb 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 b2 00 04 00 00[ 	]*vpmultishiftqb 0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 72 80[ 	]*vpmultishiftqb -0x400\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 83 b2 f8 fb ff ff[ 	]*vpmultishiftqb -0x408\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 f4[ 	]*vpmultishiftqb %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 83 f4[ 	]*vpmultishiftqb %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 31[ 	]*vpmultishiftqb \(%ecx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 b4 f4 c0 1d fe ff[ 	]*vpmultishiftqb -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 30[ 	]*vpmultishiftqb \(%eax\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 72 7f[ 	]*vpmultishiftqb 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 b2 00 10 00 00[ 	]*vpmultishiftqb 0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 72 80[ 	]*vpmultishiftqb -0x1000\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 83 b2 e0 ef ff ff[ 	]*vpmultishiftqb -0x1020\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 72 7f[ 	]*vpmultishiftqb 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 b2 00 04 00 00[ 	]*vpmultishiftqb 0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 72 80[ 	]*vpmultishiftqb -0x400\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 83 b2 f8 fb ff ff[ 	]*vpmultishiftqb -0x408\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}

0+[a-f0-9]+ <vbmi2>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressb %xmm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 63 72 7e[ 	]*vpcompressb %xmm6,0x7e\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressb %ymm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 63 72 7e[ 	]*vpcompressb %ymm6,0x7e\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 63 ee[ 	]*vpcompressb %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 63 ee[ 	]*vpcompressb %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 63 ee[ 	]*vpcompressb %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 63 ee[ 	]*vpcompressb %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressw %xmm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 63 72 40[ 	]*vpcompressw %xmm6,0x80\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressw %ymm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 63 72 40[ 	]*vpcompressw %ymm6,0x80\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 63 ee[ 	]*vpcompressw %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 63 ee[ 	]*vpcompressw %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 63 ee[ 	]*vpcompressw %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 63 ee[ 	]*vpcompressw %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 62 31[ 	]*vpexpandb \(%ecx\),%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandb -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 62 72 7e[ 	]*vpexpandb 0x7e\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 62 31[ 	]*vpexpandb \(%ecx\),%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandb -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 62 72 7e[ 	]*vpexpandb 0x7e\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 62 f5[ 	]*vpexpandb %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 62 f5[ 	]*vpexpandb %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 62 f5[ 	]*vpexpandb %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 62 f5[ 	]*vpexpandb %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 62 31[ 	]*vpexpandw \(%ecx\),%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandw -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 62 72 40[ 	]*vpexpandw 0x80\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 62 31[ 	]*vpexpandw \(%ecx\),%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandw -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 62 72 40[ 	]*vpexpandw 0x80\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 62 f5[ 	]*vpexpandw %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 62 f5[ 	]*vpexpandw %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 62 f5[ 	]*vpexpandw %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 62 f5[ 	]*vpexpandw %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 70 f4[ 	]*vpshldvw %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 70 f4[ 	]*vpshldvw %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 70 b4 f4 c0 1d fe ff[ 	]*vpshldvw -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 70 72 7f[ 	]*vpshldvw 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 70 f4[ 	]*vpshldvw %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 70 f4[ 	]*vpshldvw %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 70 b4 f4 c0 1d fe ff[ 	]*vpshldvw -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 70 72 7f[ 	]*vpshldvw 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 71 f4[ 	]*vpshldvd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 71 f4[ 	]*vpshldvd %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvd -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 71 72 7f[ 	]*vpshldvd 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 1f 71 72 7f[ 	]*vpshldvd 0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 71 f4[ 	]*vpshldvd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 71 f4[ 	]*vpshldvd %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvd -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 71 72 7f[ 	]*vpshldvd 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 3f 71 72 7f[ 	]*vpshldvd 0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 71 f4[ 	]*vpshldvq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 71 f4[ 	]*vpshldvq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 71 72 7f[ 	]*vpshldvq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 71 72 7f[ 	]*vpshldvq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 71 f4[ 	]*vpshldvq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 71 f4[ 	]*vpshldvq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 71 72 7f[ 	]*vpshldvq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 71 72 7f[ 	]*vpshldvq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 72 f4[ 	]*vpshrdvw %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 72 f4[ 	]*vpshrdvw %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 72 b4 f4 c0 1d fe ff[ 	]*vpshrdvw -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 72 72 7f[ 	]*vpshrdvw 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 72 f4[ 	]*vpshrdvw %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 72 f4[ 	]*vpshrdvw %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 72 b4 f4 c0 1d fe ff[ 	]*vpshrdvw -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 72 72 7f[ 	]*vpshrdvw 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 73 f4[ 	]*vpshrdvd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 73 f4[ 	]*vpshrdvd %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvd -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 73 72 7f[ 	]*vpshrdvd 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 1f 73 72 7f[ 	]*vpshrdvd 0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 73 f4[ 	]*vpshrdvd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 73 f4[ 	]*vpshrdvd %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvd -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 73 72 7f[ 	]*vpshrdvd 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 3f 73 72 7f[ 	]*vpshrdvd 0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 73 f4[ 	]*vpshrdvq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 73 f4[ 	]*vpshrdvq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 73 72 7f[ 	]*vpshrdvq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 73 72 7f[ 	]*vpshrdvq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 73 f4[ 	]*vpshrdvq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 73 f4[ 	]*vpshrdvq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 73 72 7f[ 	]*vpshrdvq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 73 72 7f[ 	]*vpshrdvq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 70 f4 ab[ 	]*vpshldw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 70 f4 ab[ 	]*vpshldw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 70 b4 f4 c0 1d fe ff 7b[ 	]*vpshldw \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 70 72 7f 7b[ 	]*vpshldw \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 70 f4 ab[ 	]*vpshldw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 70 f4 ab[ 	]*vpshldw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 70 b4 f4 c0 1d fe ff 7b[ 	]*vpshldw \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 70 72 7f 7b[ 	]*vpshldw \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 71 f4 ab[ 	]*vpshldd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 8f 71 f4 ab[ 	]*vpshldd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldd \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 1f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 71 f4 ab[ 	]*vpshldd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 af 71 f4 ab[ 	]*vpshldd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldd \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 3f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 71 f4 ab[ 	]*vpshldq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 71 f4 ab[ 	]*vpshldq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 1f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 71 f4 ab[ 	]*vpshldq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 71 f4 ab[ 	]*vpshldq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 3f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 72 f4 ab[ 	]*vpshrdw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 72 f4 ab[ 	]*vpshrdw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 72 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdw \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 72 72 7f 7b[ 	]*vpshrdw \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 72 f4 ab[ 	]*vpshrdw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 72 f4 ab[ 	]*vpshrdw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 72 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdw \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 72 72 7f 7b[ 	]*vpshrdw \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 73 f4 ab[ 	]*vpshrdd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 8f 73 f4 ab[ 	]*vpshrdd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdd \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 1f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 73 f4 ab[ 	]*vpshrdd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 af 73 f4 ab[ 	]*vpshrdd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdd \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 3f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 73 f4 ab[ 	]*vpshrdq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 73 f4 ab[ 	]*vpshrdq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 1f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 73 f4 ab[ 	]*vpshrdq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 73 f4 ab[ 	]*vpshrdq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 3f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressb %xmm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 63 72 7e[ 	]*vpcompressb %xmm6,0x7e\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressb %ymm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 63 72 7e[ 	]*vpcompressb %ymm6,0x7e\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 63 ee[ 	]*vpcompressb %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 63 ee[ 	]*vpcompressb %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 63 ee[ 	]*vpcompressb %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 63 ee[ 	]*vpcompressb %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressw %xmm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 63 72 40[ 	]*vpcompressw %xmm6,0x80\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 63 b4 f4 c0 1d fe ff[ 	]*vpcompressw %ymm6,-0x1e240\(%esp,%esi,8\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 63 72 40[ 	]*vpcompressw %ymm6,0x80\(%edx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 63 ee[ 	]*vpcompressw %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 63 ee[ 	]*vpcompressw %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 63 ee[ 	]*vpcompressw %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 63 ee[ 	]*vpcompressw %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 62 31[ 	]*vpexpandb \(%ecx\),%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandb -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 62 72 7e[ 	]*vpexpandb 0x7e\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 62 31[ 	]*vpexpandb \(%ecx\),%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandb -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 62 72 7e[ 	]*vpexpandb 0x7e\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 62 f5[ 	]*vpexpandb %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 62 f5[ 	]*vpexpandb %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 62 f5[ 	]*vpexpandb %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 62 f5[ 	]*vpexpandb %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 62 31[ 	]*vpexpandw \(%ecx\),%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandw -0x1e240\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 62 72 40[ 	]*vpexpandw 0x80\(%edx\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 62 31[ 	]*vpexpandw \(%ecx\),%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 62 b4 f4 c0 1d fe ff[ 	]*vpexpandw -0x1e240\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 62 72 40[ 	]*vpexpandw 0x80\(%edx\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 62 f5[ 	]*vpexpandw %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 62 f5[ 	]*vpexpandw %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 62 f5[ 	]*vpexpandw %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 62 f5[ 	]*vpexpandw %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 70 f4[ 	]*vpshldvw %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 70 f4[ 	]*vpshldvw %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 70 b4 f4 c0 1d fe ff[ 	]*vpshldvw -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 70 72 7f[ 	]*vpshldvw 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 70 f4[ 	]*vpshldvw %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 70 f4[ 	]*vpshldvw %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 70 b4 f4 c0 1d fe ff[ 	]*vpshldvw -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 70 72 7f[ 	]*vpshldvw 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 71 f4[ 	]*vpshldvd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 71 f4[ 	]*vpshldvd %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvd -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 71 72 7f[ 	]*vpshldvd 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 1f 71 72 7f[ 	]*vpshldvd 0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 71 f4[ 	]*vpshldvd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 71 f4[ 	]*vpshldvd %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvd -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 71 72 7f[ 	]*vpshldvd 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 3f 71 72 7f[ 	]*vpshldvd 0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 71 f4[ 	]*vpshldvq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 71 f4[ 	]*vpshldvq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 71 72 7f[ 	]*vpshldvq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 71 72 7f[ 	]*vpshldvq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 71 f4[ 	]*vpshldvq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 71 f4[ 	]*vpshldvq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 71 b4 f4 c0 1d fe ff[ 	]*vpshldvq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 71 72 7f[ 	]*vpshldvq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 71 72 7f[ 	]*vpshldvq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 72 f4[ 	]*vpshrdvw %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 72 f4[ 	]*vpshrdvw %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 72 b4 f4 c0 1d fe ff[ 	]*vpshrdvw -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 72 72 7f[ 	]*vpshrdvw 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 72 f4[ 	]*vpshrdvw %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 72 f4[ 	]*vpshrdvw %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 72 b4 f4 c0 1d fe ff[ 	]*vpshrdvw -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 72 72 7f[ 	]*vpshrdvw 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 73 f4[ 	]*vpshrdvd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 8f 73 f4[ 	]*vpshrdvd %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvd -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 0f 73 72 7f[ 	]*vpshrdvd 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 1f 73 72 7f[ 	]*vpshrdvd 0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 73 f4[ 	]*vpshrdvd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 af 73 f4[ 	]*vpshrdvd %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvd -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 2f 73 72 7f[ 	]*vpshrdvd 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 3f 73 72 7f[ 	]*vpshrdvd 0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 73 f4[ 	]*vpshrdvq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 8f 73 f4[ 	]*vpshrdvq %xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvq -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 0f 73 72 7f[ 	]*vpshrdvq 0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 1f 73 72 7f[ 	]*vpshrdvq 0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 73 f4[ 	]*vpshrdvq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 af 73 f4[ 	]*vpshrdvq %ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvq -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 2f 73 72 7f[ 	]*vpshrdvq 0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 3f 73 72 7f[ 	]*vpshrdvq 0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 70 f4 ab[ 	]*vpshldw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 70 f4 ab[ 	]*vpshldw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 70 b4 f4 c0 1d fe ff 7b[ 	]*vpshldw \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 70 72 7f 7b[ 	]*vpshldw \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 70 f4 ab[ 	]*vpshldw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 70 f4 ab[ 	]*vpshldw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 70 b4 f4 c0 1d fe ff 7b[ 	]*vpshldw \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 70 72 7f 7b[ 	]*vpshldw \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 71 f4 ab[ 	]*vpshldd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 8f 71 f4 ab[ 	]*vpshldd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldd \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 1f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 71 f4 ab[ 	]*vpshldd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 af 71 f4 ab[ 	]*vpshldd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldd \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 3f 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 71 f4 ab[ 	]*vpshldq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 71 f4 ab[ 	]*vpshldq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 1f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 71 f4 ab[ 	]*vpshldq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 71 f4 ab[ 	]*vpshldq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 3f 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 72 f4 ab[ 	]*vpshrdw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 72 f4 ab[ 	]*vpshrdw \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 72 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdw \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 72 72 7f 7b[ 	]*vpshrdw \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 72 f4 ab[ 	]*vpshrdw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 72 f4 ab[ 	]*vpshrdw \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 72 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdw \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 72 72 7f 7b[ 	]*vpshrdw \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 73 f4 ab[ 	]*vpshrdd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 8f 73 f4 ab[ 	]*vpshrdd \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdd \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 0f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 1f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x1fc\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 73 f4 ab[ 	]*vpshrdd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 af 73 f4 ab[ 	]*vpshrdd \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdd \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 2f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 3f 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x1fc\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 73 f4 ab[ 	]*vpshrdq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 8f 73 f4 ab[ 	]*vpshrdq \$0xab,%xmm4,%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 0f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x7f0\(%edx\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 1f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x3f8\(%edx\)\{1to2\},%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 73 f4 ab[ 	]*vpshrdq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 af 73 f4 ab[ 	]*vpshrdq \$0xab,%ymm4,%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 2f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0xfe0\(%edx\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 3f 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x3f8\(%edx\)\{1to4\},%ymm5,%ymm6\{%k7\}

0+[a-f0-9]+ <vnni>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0b 52 d2[ 	]*vpdpwssd %xmm2,%xmm4,%xmm2\{%k3\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 8b 52 d2[ 	]*vpdpwssd %xmm2,%xmm4,%xmm2\{%k3\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 09 52 94 f4 c0 1d fe ff[ 	]*vpdpwssd -0x1e240\(%esp,%esi,8\),%xmm4,%xmm2\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 09 52 52 7f[ 	]*vpdpwssd 0x7f0\(%edx\),%xmm4,%xmm2\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 19 52 52 7f[ 	]*vpdpwssd 0x1fc\(%edx\)\{1to4\},%xmm4,%xmm2\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 29 52 d9[ 	]*vpdpwssd %ymm1,%ymm3,%ymm3\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 a9 52 d9[ 	]*vpdpwssd %ymm1,%ymm3,%ymm3\{%k1\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 2c 52 9c f4 c0 1d fe ff[ 	]*vpdpwssd -0x1e240\(%esp,%esi,8\),%ymm3,%ymm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 2c 52 5a 7f[ 	]*vpdpwssd 0xfe0\(%edx\),%ymm3,%ymm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 3c 52 5a 7f[ 	]*vpdpwssd 0x1fc\(%edx\)\{1to8\},%ymm3,%ymm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 09 53 d1[ 	]*vpdpwssds %xmm1,%xmm4,%xmm2\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 89 53 d1[ 	]*vpdpwssds %xmm1,%xmm4,%xmm2\{%k1\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0c 53 94 f4 c0 1d fe ff[ 	]*vpdpwssds -0x1e240\(%esp,%esi,8\),%xmm4,%xmm2\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0c 53 52 7f[ 	]*vpdpwssds 0x7f0\(%edx\),%xmm4,%xmm2\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 1c 53 52 7f[ 	]*vpdpwssds 0x1fc\(%edx\)\{1to4\},%xmm4,%xmm2\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 2f 53 e4[ 	]*vpdpwssds %ymm4,%ymm1,%ymm4\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 af 53 e4[ 	]*vpdpwssds %ymm4,%ymm1,%ymm4\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 2b 53 a4 f4 c0 1d fe ff[ 	]*vpdpwssds -0x1e240\(%esp,%esi,8\),%ymm1,%ymm4\{%k3\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 2b 53 62 7f[ 	]*vpdpwssds 0xfe0\(%edx\),%ymm1,%ymm4\{%k3\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 3b 53 62 7f[ 	]*vpdpwssds 0x1fc\(%edx\)\{1to8\},%ymm1,%ymm4\{%k3\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 0c 50 d1[ 	]*vpdpbusd %xmm1,%xmm3,%xmm2\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 8c 50 d1[ 	]*vpdpbusd %xmm1,%xmm3,%xmm2\{%k4\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 0a 50 94 f4 c0 1d fe ff[ 	]*vpdpbusd -0x1e240\(%esp,%esi,8\),%xmm3,%xmm2\{%k2\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 0a 50 52 7f[ 	]*vpdpbusd 0x7f0\(%edx\),%xmm3,%xmm2\{%k2\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 1a 50 52 7f[ 	]*vpdpbusd 0x1fc\(%edx\)\{1to4\},%xmm3,%xmm2\{%k2\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2d 50 d2[ 	]*vpdpbusd %ymm2,%ymm2,%ymm2\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d ad 50 d2[ 	]*vpdpbusd %ymm2,%ymm2,%ymm2\{%k5\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2f 50 94 f4 c0 1d fe ff[ 	]*vpdpbusd -0x1e240\(%esp,%esi,8\),%ymm2,%ymm2\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2f 50 52 7f[ 	]*vpdpbusd 0xfe0\(%edx\),%ymm2,%ymm2\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 3f 50 52 7f[ 	]*vpdpbusd 0x1fc\(%edx\)\{1to8\},%ymm2,%ymm2\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 0e 51 f4[ 	]*vpdpbusds %xmm4,%xmm2,%xmm6\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 8e 51 f4[ 	]*vpdpbusds %xmm4,%xmm2,%xmm6\{%k6\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 0c 51 b4 f4 c0 1d fe ff[ 	]*vpdpbusds -0x1e240\(%esp,%esi,8\),%xmm2,%xmm6\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 0c 51 72 7f[ 	]*vpdpbusds 0x7f0\(%edx\),%xmm2,%xmm6\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 1c 51 72 7f[ 	]*vpdpbusds 0x1fc\(%edx\)\{1to4\},%xmm2,%xmm6\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 2f 51 e1[ 	]*vpdpbusds %ymm1,%ymm3,%ymm4\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 af 51 e1[ 	]*vpdpbusds %ymm1,%ymm3,%ymm4\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 29 51 a4 f4 c0 1d fe ff[ 	]*vpdpbusds -0x1e240\(%esp,%esi,8\),%ymm3,%ymm4\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 29 51 62 7f[ 	]*vpdpbusds 0xfe0\(%edx\),%ymm3,%ymm4\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 39 51 62 7f[ 	]*vpdpbusds 0x1fc\(%edx\)\{1to8\},%ymm3,%ymm4\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 09 52 ea[ 	]*vpdpwssd %xmm2,%xmm2,%xmm5\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 89 52 ea[ 	]*vpdpwssd %xmm2,%xmm2,%xmm5\{%k1\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 0e 52 ac f4 c0 1d fe ff[ 	]*vpdpwssd -0x1e240\(%esp,%esi,8\),%xmm2,%xmm5\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 0e 52 6a 7f[ 	]*vpdpwssd 0x7f0\(%edx\),%xmm2,%xmm5\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 1e 52 6a 7f[ 	]*vpdpwssd 0x1fc\(%edx\)\{1to4\},%xmm2,%xmm5\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2f 52 cc[ 	]*vpdpwssd %ymm4,%ymm2,%ymm1\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d af 52 cc[ 	]*vpdpwssd %ymm4,%ymm2,%ymm1\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2e 52 8c f4 c0 1d fe ff[ 	]*vpdpwssd -0x1e240\(%esp,%esi,8\),%ymm2,%ymm1\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2e 52 4a 7f[ 	]*vpdpwssd 0xfe0\(%edx\),%ymm2,%ymm1\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 3e 52 4a 7f[ 	]*vpdpwssd 0x1fc\(%edx\)\{1to8\},%ymm2,%ymm1\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0a 53 c9[ 	]*vpdpwssds %xmm1,%xmm4,%xmm1\{%k2\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 8a 53 c9[ 	]*vpdpwssds %xmm1,%xmm4,%xmm1\{%k2\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0e 53 8c f4 c0 1d fe ff[ 	]*vpdpwssds -0x1e240\(%esp,%esi,8\),%xmm4,%xmm1\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0e 53 4a 7f[ 	]*vpdpwssds 0x7f0\(%edx\),%xmm4,%xmm1\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 1e 53 4a 7f[ 	]*vpdpwssds 0x1fc\(%edx\)\{1to4\},%xmm4,%xmm1\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2c 53 dc[ 	]*vpdpwssds %ymm4,%ymm2,%ymm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d ac 53 dc[ 	]*vpdpwssds %ymm4,%ymm2,%ymm3\{%k4\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2d 53 9c f4 c0 1d fe ff[ 	]*vpdpwssds -0x1e240\(%esp,%esi,8\),%ymm2,%ymm3\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2d 53 5a 7f[ 	]*vpdpwssds 0xfe0\(%edx\),%ymm2,%ymm3\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 3d 53 5a 7f[ 	]*vpdpwssds 0x1fc\(%edx\)\{1to8\},%ymm2,%ymm3\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 0f 50 dc[ 	]*vpdpbusd %xmm4,%xmm4,%xmm3\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 8f 50 dc[ 	]*vpdpbusd %xmm4,%xmm4,%xmm3\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 09 50 9c f4 c0 1d fe ff[ 	]*vpdpbusd -0x1e240\(%esp,%esi,8\),%xmm4,%xmm3\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 09 50 5a 7f[ 	]*vpdpbusd 0x7f0\(%edx\),%xmm4,%xmm3\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 19 50 5a 7f[ 	]*vpdpbusd 0x1fc\(%edx\)\{1to4\},%xmm4,%xmm3\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2d 50 f4[ 	]*vpdpbusd %ymm4,%ymm2,%ymm6\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d ad 50 f4[ 	]*vpdpbusd %ymm4,%ymm2,%ymm6\{%k5\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2d 50 b4 f4 c0 1d fe ff[ 	]*vpdpbusd -0x1e240\(%esp,%esi,8\),%ymm2,%ymm6\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 2d 50 72 7f[ 	]*vpdpbusd 0xfe0\(%edx\),%ymm2,%ymm6\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 3d 50 72 7f[ 	]*vpdpbusd 0x1fc\(%edx\)\{1to8\},%ymm2,%ymm6\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 0d 51 dc[ 	]*vpdpbusds %xmm4,%xmm3,%xmm3\{%k5\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 8d 51 dc[ 	]*vpdpbusds %xmm4,%xmm3,%xmm3\{%k5\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 0c 51 9c f4 c0 1d fe ff[ 	]*vpdpbusds -0x1e240\(%esp,%esi,8\),%xmm3,%xmm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 0c 51 5a 7f[ 	]*vpdpbusds 0x7f0\(%edx\),%xmm3,%xmm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 1c 51 5a 7f[ 	]*vpdpbusds 0x1fc\(%edx\)\{1to4\},%xmm3,%xmm3\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 2c 51 d4[ 	]*vpdpbusds %ymm4,%ymm3,%ymm2\{%k4\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 ac 51 d4[ 	]*vpdpbusds %ymm4,%ymm3,%ymm2\{%k4\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 29 51 94 f4 c0 1d fe ff[ 	]*vpdpbusds -0x1e240\(%esp,%esi,8\),%ymm3,%ymm2\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 29 51 52 7f[ 	]*vpdpbusds 0xfe0\(%edx\),%ymm3,%ymm2\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 39 51 52 7f[ 	]*vpdpbusds 0x1fc\(%edx\)\{1to8\},%ymm3,%ymm2\{%k1\}

0+[a-f0-9]+ <bf16>:
[ 	]*[a-f0-9]+:	62 f2 57 28 72 f4    	vcvtne2ps2bf16 %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 57 08 72 f4    	vcvtne2ps2bf16 %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 57 2f 72 b4 f4 00 00 00 10 	vcvtne2ps2bf16 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 57 38 72 31    	vcvtne2ps2bf16 \(%ecx\)\{1to8\},%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 57 28 72 71 7f 	vcvtne2ps2bf16 0xfe0\(%ecx\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 57 bf 72 b2 00 f0 ff ff 	vcvtne2ps2bf16 -0x1000\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 57 0f 72 b4 f4 00 00 00 10 	vcvtne2ps2bf16 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 57 18 72 31    	vcvtne2ps2bf16 \(%ecx\)\{1to4\},%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 57 08 72 71 7f 	vcvtne2ps2bf16 0x7f0\(%ecx\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 57 9f 72 b2 00 f8 ff ff 	vcvtne2ps2bf16 -0x800\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7e 08 72 f5    	vcvtneps2bf16 %xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 28 72 f5    	vcvtneps2bf16 %ymm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 0f 72 b4 f4 00 00 00 10 	vcvtneps2bf16x 0x10000000\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7e 18 72 31    	vcvtneps2bf16 \(%ecx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 18 72 31    	vcvtneps2bf16 \(%ecx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 08 72 71 7f 	vcvtneps2bf16x 0x7f0\(%ecx\),%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 9f 72 b2 00 f8 ff ff 	vcvtneps2bf16 -0x800\(%edx\)\{1to4\},%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7e 38 72 31    	vcvtneps2bf16 \(%ecx\)\{1to8\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 38 72 31    	vcvtneps2bf16 \(%ecx\)\{1to8\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 28 72 71 7f 	vcvtneps2bf16y 0xfe0\(%ecx\),%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e bf 72 b2 00 f0 ff ff 	vcvtneps2bf16 -0x1000\(%edx\)\{1to8\},%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 56 28 52 f4    	vdpbf16ps %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 56 08 52 f4    	vdpbf16ps %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 56 2f 52 b4 f4 00 00 00 10 	vdpbf16ps 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 56 38 52 31    	vdpbf16ps \(%ecx\)\{1to8\},%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 56 28 52 71 7f 	vdpbf16ps 0xfe0\(%ecx\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 56 bf 52 b2 00 f0 ff ff 	vdpbf16ps -0x1000\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 56 0f 52 b4 f4 00 00 00 10 	vdpbf16ps 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 56 18 52 31    	vdpbf16ps \(%ecx\)\{1to4\},%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 56 08 52 71 7f 	vdpbf16ps 0x7f0\(%ecx\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 56 9f 52 b2 00 f8 ff ff 	vdpbf16ps -0x800\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 57 28 72 f4    	vcvtne2ps2bf16 %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 57 08 72 f4    	vcvtne2ps2bf16 %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 57 2f 72 b4 f4 00 00 00 10 	vcvtne2ps2bf16 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 57 38 72 31    	vcvtne2ps2bf16 \(%ecx\)\{1to8\},%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 57 28 72 71 7f 	vcvtne2ps2bf16 0xfe0\(%ecx\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 57 bf 72 b2 00 f0 ff ff 	vcvtne2ps2bf16 -0x1000\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 57 0f 72 b4 f4 00 00 00 10 	vcvtne2ps2bf16 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 57 18 72 31    	vcvtne2ps2bf16 \(%ecx\)\{1to4\},%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 57 08 72 71 7f 	vcvtne2ps2bf16 0x7f0\(%ecx\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 57 9f 72 b2 00 f8 ff ff 	vcvtne2ps2bf16 -0x800\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7e 08 72 f5    	vcvtneps2bf16 %xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 28 72 f5    	vcvtneps2bf16 %ymm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 0f 72 b4 f4 00 00 00 10 	vcvtneps2bf16x 0x10000000\(%esp,%esi,8\),%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7e 18 72 31    	vcvtneps2bf16 \(%ecx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 18 72 31    	vcvtneps2bf16 \(%ecx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 08 72 71 7f 	vcvtneps2bf16x 0x7f0\(%ecx\),%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 9f 72 b2 00 f8 ff ff 	vcvtneps2bf16 -0x800\(%edx\)\{1to4\},%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7e 38 72 31    	vcvtneps2bf16 \(%ecx\)\{1to8\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 38 72 31    	vcvtneps2bf16 \(%ecx\)\{1to8\},%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e 28 72 71 7f 	vcvtneps2bf16y 0xfe0\(%ecx\),%xmm6
[ 	]*[a-f0-9]+:	62 f2 7e bf 72 b2 00 f0 ff ff 	vcvtneps2bf16 -0x1000\(%edx\)\{1to8\},%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 56 28 52 f4    	vdpbf16ps %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 56 08 52 f4    	vdpbf16ps %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 56 2f 52 b4 f4 00 00 00 10 	vdpbf16ps 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 56 38 52 31    	vdpbf16ps \(%ecx\)\{1to8\},%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 56 28 52 71 7f 	vdpbf16ps 0xfe0\(%ecx\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 56 bf 52 b2 00 f0 ff ff 	vdpbf16ps -0x1000\(%edx\)\{1to8\},%ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 56 0f 52 b4 f4 00 00 00 10 	vdpbf16ps 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 56 18 52 31    	vdpbf16ps \(%ecx\)\{1to4\},%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 56 08 52 71 7f 	vdpbf16ps 0x7f0\(%ecx\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:	62 f2 56 9f 52 b2 00 f8 ff ff 	vdpbf16ps -0x800\(%edx\)\{1to4\},%xmm5,%xmm6\{%k7\}\{z\}

0+[a-f0-9]+ <vpopcnt>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 f5[ 	]*vpopcntd %ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 2f 55 f5[ 	]*vpopcntd %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d af 55 f5[ 	]*vpopcntd %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 31[ 	]*vpopcntd \(%ecx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 38 55 30[ 	]*vpopcntd \(%eax\)\{1to8\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 72 7f[ 	]*vpopcntd 0xfe0\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 b2 00 10 00 00[ 	]*vpopcntd 0x1000\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 72 80[ 	]*vpopcntd -0x1000\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 28 55 b2 e0 ef ff ff[ 	]*vpopcntd -0x1020\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 38 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to8\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 38 55 b2 00 02 00 00[ 	]*vpopcntd 0x200\(%edx\)\{1to8\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 38 55 72 80[ 	]*vpopcntd -0x200\(%edx\)\{1to8\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 38 55 b2 fc fd ff ff[ 	]*vpopcntd -0x204\(%edx\)\{1to8\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 f5[ 	]*vpopcntq %ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 2f 55 f5[ 	]*vpopcntq %ymm5,%ymm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd af 55 f5[ 	]*vpopcntq %ymm5,%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 31[ 	]*vpopcntq \(%ecx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 38 55 30[ 	]*vpopcntq \(%eax\)\{1to4\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 72 7f[ 	]*vpopcntq 0xfe0\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 b2 00 10 00 00[ 	]*vpopcntq 0x1000\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 72 80[ 	]*vpopcntq -0x1000\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 28 55 b2 e0 ef ff ff[ 	]*vpopcntq -0x1020\(%edx\),%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 38 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to4\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 38 55 b2 00 04 00 00[ 	]*vpopcntq 0x400\(%edx\)\{1to4\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 38 55 72 80[ 	]*vpopcntq -0x400\(%edx\)\{1to4\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 38 55 b2 f8 fb ff ff[ 	]*vpopcntq -0x408\(%edx\)\{1to4\},%ymm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 f5[ 	]*vpopcntd %xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 0f 55 f5[ 	]*vpopcntd %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 8f 55 f5[ 	]*vpopcntd %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 31[ 	]*vpopcntd \(%ecx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 18 55 30[ 	]*vpopcntd \(%eax\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 18 55 30[ 	]*vpopcntd \(%eax\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 72 7f[ 	]*vpopcntd 0x7f0\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 b2 00 08 00 00[ 	]*vpopcntd 0x800\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 72 80[ 	]*vpopcntd -0x800\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 08 55 b2 f0 f7 ff ff[ 	]*vpopcntd -0x810\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 18 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 18 55 b2 00 02 00 00[ 	]*vpopcntd 0x200\(%edx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 18 55 72 80[ 	]*vpopcntd -0x200\(%edx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 18 55 b2 fc fd ff ff[ 	]*vpopcntd -0x204\(%edx\)\{1to4\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 f5[ 	]*vpopcntq %xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 0f 55 f5[ 	]*vpopcntq %xmm5,%xmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 8f 55 f5[ 	]*vpopcntq %xmm5,%xmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 31[ 	]*vpopcntq \(%ecx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 18 55 30[ 	]*vpopcntq \(%eax\)\{1to2\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 18 55 30[ 	]*vpopcntq \(%eax\)\{1to2\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 72 7f[ 	]*vpopcntq 0x7f0\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 b2 00 08 00 00[ 	]*vpopcntq 0x800\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 72 80[ 	]*vpopcntq -0x800\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 08 55 b2 f0 f7 ff ff[ 	]*vpopcntq -0x810\(%edx\),%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 18 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to2\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 18 55 b2 00 04 00 00[ 	]*vpopcntq 0x400\(%edx\)\{1to2\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 18 55 72 80[ 	]*vpopcntq -0x400\(%edx\)\{1to2\},%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 18 55 b2 f8 fb ff ff[ 	]*vpopcntq -0x408\(%edx\)\{1to2\},%xmm6
#pass
