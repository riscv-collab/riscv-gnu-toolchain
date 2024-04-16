#as: --64
#objdump: -dw
#name: illegal decoding of APX_F jmpabs insns
#source: x86-64-apx-jmpabs-inval.s

.*: +file format .*

Disassembly of section \.text:

0+ <.text>:
\s*[a-f0-9]+:	66 d5 00 a1[  	]+\(bad\)
\s*[a-f0-9]+:	01 00[  	]+add    %eax,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	67 d5 00 a1[  	]+\(bad\)
\s*[a-f0-9]+:	01 00[  	]+add    %eax,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	f2 d5 00 a1[  	]+\(bad\)
\s*[a-f0-9]+:	01 00[  	]+add    %eax,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	f3 d5 00 a1[  	]+\(bad\)
\s*[a-f0-9]+:	01 00[  	]+add    %eax,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	f0 d5 00 a1[  	]+\(bad\)
\s*[a-f0-9]+:	01 00[  	]+add    %eax,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	d5 08 a1[  	]+\(bad\)
\s*[a-f0-9]+:	01 00[  	]+add    %eax,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
\s*[a-f0-9]+:	00 00[  	]+add    %al,\(%rax\)
#pass
