#objdump: -dw
#name: x86_64 AVX-VNNI-INT16 insns
#source: x86-64-avx-vnni-int16.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*c4 e2 56 d2 f4\s+vpdpwsud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 f4\s+vpdpwsud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 56 d2 b4 f5 00 00 00 10\s+vpdpwsud 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 56 d2 31\s+vpdpwsud \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b1 e0 0f 00 00\s+vpdpwsud 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b2 00 f0 ff ff\s+vpdpwsud -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 52 d2 b4 f5 00 00 00 10\s+vpdpwsud 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 52 d2 31\s+vpdpwsud \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b1 f0 07 00 00\s+vpdpwsud 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b2 00 f8 ff ff\s+vpdpwsud -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 f4\s+vpdpwsuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 f4\s+vpdpwsuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 56 d3 b4 f5 00 00 00 10\s+vpdpwsuds 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 56 d3 31\s+vpdpwsuds \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b1 e0 0f 00 00\s+vpdpwsuds 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b2 00 f0 ff ff\s+vpdpwsuds -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 52 d3 b4 f5 00 00 00 10\s+vpdpwsuds 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 52 d3 31\s+vpdpwsuds \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b1 f0 07 00 00\s+vpdpwsuds 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b2 00 f8 ff ff\s+vpdpwsuds -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 f4\s+vpdpwusd %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 f4\s+vpdpwusd %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 55 d2 b4 f5 00 00 00 10\s+vpdpwusd 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 55 d2 31\s+vpdpwusd \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b1 e0 0f 00 00\s+vpdpwusd 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b2 00 f0 ff ff\s+vpdpwusd -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 51 d2 b4 f5 00 00 00 10\s+vpdpwusd 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 51 d2 31\s+vpdpwusd \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b1 f0 07 00 00\s+vpdpwusd 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b2 00 f8 ff ff\s+vpdpwusd -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 f4\s+vpdpwusds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 f4\s+vpdpwusds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 55 d3 b4 f5 00 00 00 10\s+vpdpwusds 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 55 d3 31\s+vpdpwusds \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b1 e0 0f 00 00\s+vpdpwusds 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b2 00 f0 ff ff\s+vpdpwusds -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 51 d3 b4 f5 00 00 00 10\s+vpdpwusds 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 51 d3 31\s+vpdpwusds \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b1 f0 07 00 00\s+vpdpwusds 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b2 00 f8 ff ff\s+vpdpwusds -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 f4\s+vpdpwuud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 f4\s+vpdpwuud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 54 d2 b4 f5 00 00 00 10\s+vpdpwuud 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 54 d2 31\s+vpdpwuud \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b1 e0 0f 00 00\s+vpdpwuud 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b2 00 f0 ff ff\s+vpdpwuud -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 50 d2 b4 f5 00 00 00 10\s+vpdpwuud 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 50 d2 31\s+vpdpwuud \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b1 f0 07 00 00\s+vpdpwuud 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b2 00 f8 ff ff\s+vpdpwuud -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 f4\s+vpdpwuuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 f4\s+vpdpwuuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 54 d3 b4 f5 00 00 00 10\s+vpdpwuuds 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 54 d3 31\s+vpdpwuuds \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b1 e0 0f 00 00\s+vpdpwuuds 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b2 00 f0 ff ff\s+vpdpwuuds -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 50 d3 b4 f5 00 00 00 10\s+vpdpwuuds 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 50 d3 31\s+vpdpwuuds \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b1 f0 07 00 00\s+vpdpwuuds 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b2 00 f8 ff ff\s+vpdpwuuds -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 f4\s+vpdpwsud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 f4\s+vpdpwsud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 56 d2 b4 f5 00 00 00 10\s+vpdpwsud 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 56 d2 31\s+vpdpwsud \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b1 e0 0f 00 00\s+vpdpwsud 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d2 b2 00 f0 ff ff\s+vpdpwsud -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 52 d2 b4 f5 00 00 00 10\s+vpdpwsud 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 52 d2 31\s+vpdpwsud \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b1 f0 07 00 00\s+vpdpwsud 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d2 b2 00 f8 ff ff\s+vpdpwsud -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 f4\s+vpdpwsuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 f4\s+vpdpwsuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 56 d3 b4 f5 00 00 00 10\s+vpdpwsuds 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 56 d3 31\s+vpdpwsuds \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b1 e0 0f 00 00\s+vpdpwsuds 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 56 d3 b2 00 f0 ff ff\s+vpdpwsuds -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 52 d3 b4 f5 00 00 00 10\s+vpdpwsuds 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 52 d3 31\s+vpdpwsuds \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b1 f0 07 00 00\s+vpdpwsuds 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 52 d3 b2 00 f8 ff ff\s+vpdpwsuds -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 f4\s+vpdpwusd %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 f4\s+vpdpwusd %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 55 d2 b4 f5 00 00 00 10\s+vpdpwusd 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 55 d2 31\s+vpdpwusd \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b1 e0 0f 00 00\s+vpdpwusd 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d2 b2 00 f0 ff ff\s+vpdpwusd -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 51 d2 b4 f5 00 00 00 10\s+vpdpwusd 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 51 d2 31\s+vpdpwusd \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b1 f0 07 00 00\s+vpdpwusd 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d2 b2 00 f8 ff ff\s+vpdpwusd -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 f4\s+vpdpwusds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 f4\s+vpdpwusds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 55 d3 b4 f5 00 00 00 10\s+vpdpwusds 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 55 d3 31\s+vpdpwusds \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b1 e0 0f 00 00\s+vpdpwusds 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 55 d3 b2 00 f0 ff ff\s+vpdpwusds -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 51 d3 b4 f5 00 00 00 10\s+vpdpwusds 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 51 d3 31\s+vpdpwusds \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b1 f0 07 00 00\s+vpdpwusds 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 51 d3 b2 00 f8 ff ff\s+vpdpwusds -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 f4\s+vpdpwuud %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 f4\s+vpdpwuud %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 54 d2 b4 f5 00 00 00 10\s+vpdpwuud 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 54 d2 31\s+vpdpwuud \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b1 e0 0f 00 00\s+vpdpwuud 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d2 b2 00 f0 ff ff\s+vpdpwuud -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 50 d2 b4 f5 00 00 00 10\s+vpdpwuud 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 50 d2 31\s+vpdpwuud \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b1 f0 07 00 00\s+vpdpwuud 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d2 b2 00 f8 ff ff\s+vpdpwuud -0x800\(%rdx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 f4\s+vpdpwuuds %ymm4,%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 f4\s+vpdpwuuds %xmm4,%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 a2 54 d3 b4 f5 00 00 00 10\s+vpdpwuuds 0x10000000\(%rbp,%r14,8\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 c2 54 d3 31\s+vpdpwuuds \(%r9\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b1 e0 0f 00 00\s+vpdpwuuds 0xfe0\(%rcx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 e2 54 d3 b2 00 f0 ff ff\s+vpdpwuuds -0x1000\(%rdx\),%ymm5,%ymm6
\s*[a-f0-9]+:\s*c4 a2 50 d3 b4 f5 00 00 00 10\s+vpdpwuuds 0x10000000\(%rbp,%r14,8\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 c2 50 d3 31\s+vpdpwuuds \(%r9\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b1 f0 07 00 00\s+vpdpwuuds 0x7f0\(%rcx\),%xmm5,%xmm6
\s*[a-f0-9]+:\s*c4 e2 50 d3 b2 00 f8 ff ff\s+vpdpwuuds -0x800\(%rdx\),%xmm5,%xmm6
