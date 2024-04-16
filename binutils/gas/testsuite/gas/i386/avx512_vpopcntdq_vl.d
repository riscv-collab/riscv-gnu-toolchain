#objdump: -dw
#name: i386 AVX512VL+VPOPCNTDQ insns
#source: avx512_vpopcntdq_vl.s

.*: +file format .*


Disassembly of section \.text:

00000000 <vpopcnt>:
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
