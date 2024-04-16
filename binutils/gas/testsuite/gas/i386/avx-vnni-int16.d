#objdump: -dw
#name: i386 AVX-VNNI-INT16 insns
#source: avx-vnni-int16.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 56 d2 f4\s+vpdpwsud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 f4\s+vpdpwsud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b4 f4 00 00 00 10\s+vpdpwsud 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 31\s+vpdpwsud \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b1 e0 0f 00 00\s+vpdpwsud 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b2 00 f0 ff ff\s+vpdpwsud -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b4 f4 00 00 00 10\s+vpdpwsud 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 31\s+vpdpwsud \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b1 f0 07 00 00\s+vpdpwsud 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b2 00 f8 ff ff\s+vpdpwsud -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 f4\s+vpdpwsuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 f4\s+vpdpwsuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b4 f4 00 00 00 10\s+vpdpwsuds 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 31\s+vpdpwsuds \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b1 e0 0f 00 00\s+vpdpwsuds 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b2 00 f0 ff ff\s+vpdpwsuds -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b4 f4 00 00 00 10\s+vpdpwsuds 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 31\s+vpdpwsuds \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b1 f0 07 00 00\s+vpdpwsuds 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b2 00 f8 ff ff\s+vpdpwsuds -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 f4\s+vpdpwusd %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 f4\s+vpdpwusd %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b4 f4 00 00 00 10\s+vpdpwusd 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 31\s+vpdpwusd \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b1 e0 0f 00 00\s+vpdpwusd 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b2 00 f0 ff ff\s+vpdpwusd -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b4 f4 00 00 00 10\s+vpdpwusd 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 31\s+vpdpwusd \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b1 f0 07 00 00\s+vpdpwusd 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b2 00 f8 ff ff\s+vpdpwusd -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 f4\s+vpdpwusds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 f4\s+vpdpwusds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b4 f4 00 00 00 10\s+vpdpwusds 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 31\s+vpdpwusds \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b1 e0 0f 00 00\s+vpdpwusds 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b2 00 f0 ff ff\s+vpdpwusds -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b4 f4 00 00 00 10\s+vpdpwusds 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 31\s+vpdpwusds \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b1 f0 07 00 00\s+vpdpwusds 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b2 00 f8 ff ff\s+vpdpwusds -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 f4\s+vpdpwuud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 f4\s+vpdpwuud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b4 f4 00 00 00 10\s+vpdpwuud 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 31\s+vpdpwuud \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b1 e0 0f 00 00\s+vpdpwuud 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b2 00 f0 ff ff\s+vpdpwuud -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b4 f4 00 00 00 10\s+vpdpwuud 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 31\s+vpdpwuud \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b1 f0 07 00 00\s+vpdpwuud 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b2 00 f8 ff ff\s+vpdpwuud -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 f4\s+vpdpwuuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 f4\s+vpdpwuuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b4 f4 00 00 00 10\s+vpdpwuuds 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 31\s+vpdpwuuds \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b1 e0 0f 00 00\s+vpdpwuuds 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b2 00 f0 ff ff\s+vpdpwuuds -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b4 f4 00 00 00 10\s+vpdpwuuds 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 31\s+vpdpwuuds \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b1 f0 07 00 00\s+vpdpwuuds 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b2 00 f8 ff ff\s+vpdpwuuds -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 f4\s+vpdpwsud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 f4\s+vpdpwsud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b4 f4 00 00 00 10\s+vpdpwsud 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 31\s+vpdpwsud \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b1 e0 0f 00 00\s+vpdpwsud 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b2 00 f0 ff ff\s+vpdpwsud -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b4 f4 00 00 00 10\s+vpdpwsud 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 31\s+vpdpwsud \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b1 f0 07 00 00\s+vpdpwsud 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b2 00 f8 ff ff\s+vpdpwsud -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 f4\s+vpdpwsuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 f4\s+vpdpwsuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b4 f4 00 00 00 10\s+vpdpwsuds 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 31\s+vpdpwsuds \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b1 e0 0f 00 00\s+vpdpwsuds 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b2 00 f0 ff ff\s+vpdpwsuds -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b4 f4 00 00 00 10\s+vpdpwsuds 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 31\s+vpdpwsuds \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b1 f0 07 00 00\s+vpdpwsuds 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b2 00 f8 ff ff\s+vpdpwsuds -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 f4\s+vpdpwusd %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 f4\s+vpdpwusd %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b4 f4 00 00 00 10\s+vpdpwusd 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 31\s+vpdpwusd \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b1 e0 0f 00 00\s+vpdpwusd 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b2 00 f0 ff ff\s+vpdpwusd -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b4 f4 00 00 00 10\s+vpdpwusd 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 31\s+vpdpwusd \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b1 f0 07 00 00\s+vpdpwusd 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b2 00 f8 ff ff\s+vpdpwusd -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 f4\s+vpdpwusds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 f4\s+vpdpwusds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b4 f4 00 00 00 10\s+vpdpwusds 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 31\s+vpdpwusds \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b1 e0 0f 00 00\s+vpdpwusds 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b2 00 f0 ff ff\s+vpdpwusds -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b4 f4 00 00 00 10\s+vpdpwusds 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 31\s+vpdpwusds \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b1 f0 07 00 00\s+vpdpwusds 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b2 00 f8 ff ff\s+vpdpwusds -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 f4\s+vpdpwuud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 f4\s+vpdpwuud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b4 f4 00 00 00 10\s+vpdpwuud 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 31\s+vpdpwuud \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b1 e0 0f 00 00\s+vpdpwuud 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b2 00 f0 ff ff\s+vpdpwuud -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b4 f4 00 00 00 10\s+vpdpwuud 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 31\s+vpdpwuud \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b1 f0 07 00 00\s+vpdpwuud 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b2 00 f8 ff ff\s+vpdpwuud -0x800\(%edx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 f4\s+vpdpwuuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 f4\s+vpdpwuuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b4 f4 00 00 00 10\s+vpdpwuuds 0x10000000\(%esp,%esi,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 31\s+vpdpwuuds \(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b1 e0 0f 00 00\s+vpdpwuuds 0xfe0\(%ecx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b2 00 f0 ff ff\s+vpdpwuuds -0x1000\(%edx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b4 f4 00 00 00 10\s+vpdpwuuds 0x10000000\(%esp,%esi,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 31\s+vpdpwuuds \(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b1 f0 07 00 00\s+vpdpwuuds 0x7f0\(%ecx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b2 00 f8 ff ff\s+vpdpwuuds -0x800\(%edx\),%xmm5,%xmm6
