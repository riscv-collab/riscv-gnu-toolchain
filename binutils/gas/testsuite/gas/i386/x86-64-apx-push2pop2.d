#as: --64
#objdump: -dw
#name: x86_64 APX-push2pop2 insns
#source: x86-64-apx-push2pop2.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*62 f4 7c 18 ff f3\s+push2\s+%rbx,%rax
\s*[a-f0-9]+:\s*62 fc 3c 18 ff f1\s+push2\s+%r17,%r8
\s*[a-f0-9]+:\s*62 d4 04 10 ff f1\s+push2\s+%r9,%r31
\s*[a-f0-9]+:\s*62 dc 3c 10 ff f7\s+push2\s+%r31,%r24
\s*[a-f0-9]+:\s*62 f4 fc 18 ff f3\s+push2p\s+%rbx,%rax
\s*[a-f0-9]+:\s*62 fc bc 18 ff f1\s+push2p\s+%r17,%r8
\s*[a-f0-9]+:\s*62 d4 84 10 ff f1\s+push2p\s+%r9,%r31
\s*[a-f0-9]+:\s*62 dc bc 10 ff f7\s+push2p\s+%r31,%r24
\s*[a-f0-9]+:\s*62 f4 64 18 8f c0\s+pop2\s+%rax,%rbx
\s*[a-f0-9]+:\s*62 d4 74 10 8f c0\s+pop2\s+%r8,%r17
\s*[a-f0-9]+:\s*62 dc 34 18 8f c7\s+pop2\s+%r31,%r9
\s*[a-f0-9]+:\s*62 dc 04 10 8f c0\s+pop2\s+%r24,%r31
\s*[a-f0-9]+:\s*62 f4 e4 18 8f c0\s+pop2p\s+%rax,%rbx
\s*[a-f0-9]+:\s*62 d4 f4 10 8f c0\s+pop2p\s+%r8,%r17
\s*[a-f0-9]+:\s*62 dc b4 18 8f c7\s+pop2p\s+%r31,%r9
\s*[a-f0-9]+:\s*62 dc 84 10 8f c0\s+pop2p\s+%r24,%r31
\s*[a-f0-9]+:\s*62 f4 7c 18 ff f3\s+push2\s+%rbx,%rax
\s*[a-f0-9]+:\s*62 fc 3c 18 ff f1\s+push2\s+%r17,%r8
\s*[a-f0-9]+:\s*62 d4 04 10 ff f1\s+push2\s+%r9,%r31
\s*[a-f0-9]+:\s*62 dc 3c 10 ff f7\s+push2\s+%r31,%r24
\s*[a-f0-9]+:\s*62 f4 fc 18 ff f3\s+push2p\s+%rbx,%rax
\s*[a-f0-9]+:\s*62 fc bc 18 ff f1\s+push2p\s+%r17,%r8
\s*[a-f0-9]+:\s*62 d4 84 10 ff f1\s+push2p\s+%r9,%r31
\s*[a-f0-9]+:\s*62 dc bc 10 ff f7\s+push2p\s+%r31,%r24
\s*[a-f0-9]+:\s*62 f4 64 18 8f c0\s+pop2\s+%rax,%rbx
\s*[a-f0-9]+:\s*62 d4 74 10 8f c0\s+pop2\s+%r8,%r17
\s*[a-f0-9]+:\s*62 dc 34 18 8f c7\s+pop2\s+%r31,%r9
\s*[a-f0-9]+:\s*62 dc 04 10 8f c0\s+pop2\s+%r24,%r31
\s*[a-f0-9]+:\s*62 f4 e4 18 8f c0\s+pop2p\s+%rax,%rbx
\s*[a-f0-9]+:\s*62 d4 f4 10 8f c0\s+pop2p\s+%r8,%r17
\s*[a-f0-9]+:\s*62 dc b4 18 8f c7\s+pop2p\s+%r31,%r9
\s*[a-f0-9]+:\s*62 dc 84 10 8f c0\s+pop2p\s+%r24,%r31
#pass
