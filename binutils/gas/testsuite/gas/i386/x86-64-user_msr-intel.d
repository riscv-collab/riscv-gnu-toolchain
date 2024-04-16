#as:
#objdump: -dw -Mintel
#name: x86_64 USER_MSR insns (Intel disassembly)
#source: x86-64-user_msr.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
\s*[a-f0-9]+:\s*f2 45 0f 38 f8 f4\s+urdmsr r12,r14
\s*[a-f0-9]+:\s*f2 44 0f 38 f8 f0\s+urdmsr rax,r14
\s*[a-f0-9]+:\s*62 64 7f 08 f8 c0\s+urdmsr rax,r24
\s*[a-f0-9]+:\s*f2 41 0f 38 f8 d4\s+urdmsr r12,rdx
\s*[a-f0-9]+:\s*62 fc 7f 08 f8 d6\s+urdmsr r22,rdx
\s*[a-f0-9]+:\s*f2 0f 38 f8 d0\s+urdmsr rax,rdx
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 0f 0f 12 03\s+urdmsr r12,0x3120f0f
\s*[a-f0-9]+:\s*c4 e7 7b f8 c0 0f 0f 12 03\s+urdmsr rax,0x3120f0f
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 7f 00 00 00\s+urdmsr r12,0x7f
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 ff 7f 00 00\s+urdmsr r12,0x7fff
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 00 00 00 80\s+urdmsr r12,0x80000000
\s*[a-f0-9]+:\s*62 ff 7f 08 f8 c6 00 00 00 80\s+urdmsr r22,0x80000000
\s*[a-f0-9]+:\s*f3 45 0f 38 f8 f4\s+uwrmsr r14,r12
\s*[a-f0-9]+:\s*f3 44 0f 38 f8 f0\s+uwrmsr r14,rax
\s*[a-f0-9]+:\s*62 64 7e 08 f8 c0\s+uwrmsr r24,rax
\s*[a-f0-9]+:\s*f3 41 0f 38 f8 d4\s+uwrmsr rdx,r12
\s*[a-f0-9]+:\s*62 fc 7e 08 f8 d6\s+uwrmsr rdx,r22
\s*[a-f0-9]+:\s*f3 0f 38 f8 d0\s+uwrmsr rdx,rax
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 0f 0f 12 03\s+uwrmsr 0x3120f0f,r12
\s*[a-f0-9]+:\s*c4 e7 7a f8 c0 0f 0f 12 03\s+uwrmsr 0x3120f0f,rax
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 7f 00 00 00\s+uwrmsr 0x7f,r12
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 ff 7f 00 00\s+uwrmsr 0x7fff,r12
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 00 00 00 80\s+uwrmsr 0x80000000,r12
\s*[a-f0-9]+:\s*62 ff 7e 08 f8 c6 00 00 00 80\s+uwrmsr 0x80000000,r22
\s*[a-f0-9]+:\s*f2 45 0f 38 f8 f4\s+urdmsr r12,r14
\s*[a-f0-9]+:\s*f2 44 0f 38 f8 f0\s+urdmsr rax,r14
\s*[a-f0-9]+:\s*f2 41 0f 38 f8 d4\s+urdmsr r12,rdx
\s*[a-f0-9]+:\s*f2 0f 38 f8 d0\s+urdmsr rax,rdx
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 0f 0f 12 03\s+urdmsr r12,0x3120f0f
\s*[a-f0-9]+:\s*c4 e7 7b f8 c0 0f 0f 12 03\s+urdmsr rax,0x3120f0f
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 7f 00 00 00\s+urdmsr r12,0x7f
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 ff 7f 00 00\s+urdmsr r12,0x7fff
\s*[a-f0-9]+:\s*c4 c7 7b f8 c4 00 00 00 80\s+urdmsr r12,0x80000000
\s*[a-f0-9]+:\s*f3 45 0f 38 f8 f4\s+uwrmsr r14,r12
\s*[a-f0-9]+:\s*f3 44 0f 38 f8 f0\s+uwrmsr r14,rax
\s*[a-f0-9]+:\s*f3 41 0f 38 f8 d4\s+uwrmsr rdx,r12
\s*[a-f0-9]+:\s*f3 0f 38 f8 d0\s+uwrmsr rdx,rax
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 0f 0f 12 03\s+uwrmsr 0x3120f0f,r12
\s*[a-f0-9]+:\s*c4 e7 7a f8 c0 0f 0f 12 03\s+uwrmsr 0x3120f0f,rax
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 7f 00 00 00\s+uwrmsr 0x7f,r12
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 ff 7f 00 00\s+uwrmsr 0x7fff,r12
\s*[a-f0-9]+:\s*c4 c7 7a f8 c4 00 00 00 80\s+uwrmsr 0x80000000,r12
#pass
