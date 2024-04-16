#name: s390x opcode
#objdump: -drw

.*: +file format .*

Disassembly of section .text:

.* <foo>:
.*:	eb d6 65 b3 01 6a [	 ]*asi	5555\(%r6\),-42
.*:	eb d6 65 b3 01 7a [	 ]*agsi	5555\(%r6\),-42
.*:	eb d6 65 b3 01 6e [	 ]*alsi	5555\(%r6\),-42
.*:	eb d6 65 b3 01 7e [	 ]*algsi	5555\(%r6\),-42
 *([\da-f]+):	c6 6d 00 00 00 00 [	 ]*crl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 68 00 00 00 00 [	 ]*cgrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 6c 00 00 00 00 [	 ]*cgfrl	%r6,\1 <foo\+0x\1>
.*:	ec 67 84 57 a0 f6 [	 ]*crbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 f6 [	 ]*crbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 f6 [	 ]*crbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 f6 [	 ]*crbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 f6 [	 ]*crbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 f6 [	 ]*crbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 f6 [	 ]*crbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 f6 [	 ]*crbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 f6 [	 ]*crbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 f6 [	 ]*crbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 f6 [	 ]*crbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 f6 [	 ]*crbnh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 f6 [	 ]*crbnh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 e4 [	 ]*cgrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 e4 [	 ]*cgrbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 e4 [	 ]*cgrbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 e4 [	 ]*cgrbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 e4 [	 ]*cgrbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 e4 [	 ]*cgrbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 e4 [	 ]*cgrbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 e4 [	 ]*cgrbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 e4 [	 ]*cgrbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 e4 [	 ]*cgrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 e4 [	 ]*cgrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 e4 [	 ]*cgrbnh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 e4 [	 ]*cgrbnh	%r6,%r7,1111\(%r8\)
 *([\da-f]+):	ec 67 00 00 a0 76 [	 ]*crjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 76 [	 ]*crjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 76 [	 ]*crjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 76 [	 ]*crjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 76 [	 ]*crjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 76 [	 ]*crjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 76 [	 ]*crjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 76 [	 ]*crje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 76 [	 ]*crje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 76 [	 ]*crjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 76 [	 ]*crjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 76 [	 ]*crjnh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 76 [	 ]*crjnh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 64 [	 ]*cgrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 64 [	 ]*cgrjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 64 [	 ]*cgrjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 64 [	 ]*cgrjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 64 [	 ]*cgrjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 64 [	 ]*cgrjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 64 [	 ]*cgrjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 64 [	 ]*cgrje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 64 [	 ]*cgrje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 64 [	 ]*cgrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 64 [	 ]*cgrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 64 [	 ]*cgrjnh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 64 [	 ]*cgrjnh	%r6,%r7,\1 <foo\+0x\1>
.*:	ec 6a 74 57 d6 fe [	 ]*cibnl	%r6,-42,1111\(%r7\)
.*:	ec 62 74 57 d6 fe [	 ]*cibh	%r6,-42,1111\(%r7\)
.*:	ec 62 74 57 d6 fe [	 ]*cibh	%r6,-42,1111\(%r7\)
.*:	ec 64 74 57 d6 fe [	 ]*cibl	%r6,-42,1111\(%r7\)
.*:	ec 64 74 57 d6 fe [	 ]*cibl	%r6,-42,1111\(%r7\)
.*:	ec 66 74 57 d6 fe [	 ]*cibne	%r6,-42,1111\(%r7\)
.*:	ec 66 74 57 d6 fe [	 ]*cibne	%r6,-42,1111\(%r7\)
.*:	ec 68 74 57 d6 fe [	 ]*cibe	%r6,-42,1111\(%r7\)
.*:	ec 68 74 57 d6 fe [	 ]*cibe	%r6,-42,1111\(%r7\)
.*:	ec 6a 74 57 d6 fe [	 ]*cibnl	%r6,-42,1111\(%r7\)
.*:	ec 6a 74 57 d6 fe [	 ]*cibnl	%r6,-42,1111\(%r7\)
.*:	ec 6c 74 57 d6 fe [	 ]*cibnh	%r6,-42,1111\(%r7\)
.*:	ec 6c 74 57 d6 fe [	 ]*cibnh	%r6,-42,1111\(%r7\)
.*:	ec 6a 74 57 d6 fc [	 ]*cgibnl	%r6,-42,1111\(%r7\)
.*:	ec 62 74 57 d6 fc [	 ]*cgibh	%r6,-42,1111\(%r7\)
.*:	ec 62 74 57 d6 fc [	 ]*cgibh	%r6,-42,1111\(%r7\)
.*:	ec 64 74 57 d6 fc [	 ]*cgibl	%r6,-42,1111\(%r7\)
.*:	ec 64 74 57 d6 fc [	 ]*cgibl	%r6,-42,1111\(%r7\)
.*:	ec 66 74 57 d6 fc [	 ]*cgibne	%r6,-42,1111\(%r7\)
.*:	ec 66 74 57 d6 fc [	 ]*cgibne	%r6,-42,1111\(%r7\)
.*:	ec 68 74 57 d6 fc [	 ]*cgibe	%r6,-42,1111\(%r7\)
.*:	ec 68 74 57 d6 fc [	 ]*cgibe	%r6,-42,1111\(%r7\)
.*:	ec 6a 74 57 d6 fc [	 ]*cgibnl	%r6,-42,1111\(%r7\)
.*:	ec 6a 74 57 d6 fc [	 ]*cgibnl	%r6,-42,1111\(%r7\)
.*:	ec 6c 74 57 d6 fc [	 ]*cgibnh	%r6,-42,1111\(%r7\)
.*:	ec 6c 74 57 d6 fc [	 ]*cgibnh	%r6,-42,1111\(%r7\)
 *([\da-f]+):	ec 6a 00 00 d6 7e [	 ]*cijnl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 d6 7e [	 ]*cijh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 d6 7e [	 ]*cijh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 d6 7e [	 ]*cijl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 d6 7e [	 ]*cijl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 d6 7e [	 ]*cijne	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 d6 7e [	 ]*cijne	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 d6 7e [	 ]*cije	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 d6 7e [	 ]*cije	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 d6 7e [	 ]*cijnl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 d6 7e [	 ]*cijnl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 d6 7e [	 ]*cijnh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 d6 7e [	 ]*cijnh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 d6 7c [	 ]*cgijnl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 d6 7c [	 ]*cgijh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 d6 7c [	 ]*cgijh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 d6 7c [	 ]*cgijl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 d6 7c [	 ]*cgijl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 d6 7c [	 ]*cgijne	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 d6 7c [	 ]*cgijne	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 d6 7c [	 ]*cgije	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 d6 7c [	 ]*cgije	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 d6 7c [	 ]*cgijnl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 d6 7c [	 ]*cgijnl	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 d6 7c [	 ]*cgijnh	%r6,-42,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 d6 7c [	 ]*cgijnh	%r6,-42,\1 <foo\+0x\1>
.*:	b9 72 a0 67 [	 ]*crtnl	%r6,%r7
.*:	b9 72 20 67 [	 ]*crth	%r6,%r7
.*:	b9 72 20 67 [	 ]*crth	%r6,%r7
.*:	b9 72 40 67 [	 ]*crtl	%r6,%r7
.*:	b9 72 40 67 [	 ]*crtl	%r6,%r7
.*:	b9 72 60 67 [	 ]*crtne	%r6,%r7
.*:	b9 72 60 67 [	 ]*crtne	%r6,%r7
.*:	b9 72 80 67 [	 ]*crte	%r6,%r7
.*:	b9 72 80 67 [	 ]*crte	%r6,%r7
.*:	b9 72 a0 67 [	 ]*crtnl	%r6,%r7
.*:	b9 72 a0 67 [	 ]*crtnl	%r6,%r7
.*:	b9 72 c0 67 [	 ]*crtnh	%r6,%r7
.*:	b9 72 c0 67 [	 ]*crtnh	%r6,%r7
.*:	b9 60 a0 67 [	 ]*cgrtnl	%r6,%r7
.*:	b9 60 20 67 [	 ]*cgrth	%r6,%r7
.*:	b9 60 20 67 [	 ]*cgrth	%r6,%r7
.*:	b9 60 40 67 [	 ]*cgrtl	%r6,%r7
.*:	b9 60 40 67 [	 ]*cgrtl	%r6,%r7
.*:	b9 60 60 67 [	 ]*cgrtne	%r6,%r7
.*:	b9 60 60 67 [	 ]*cgrtne	%r6,%r7
.*:	b9 60 80 67 [	 ]*cgrte	%r6,%r7
.*:	b9 60 80 67 [	 ]*cgrte	%r6,%r7
.*:	b9 60 a0 67 [	 ]*cgrtnl	%r6,%r7
.*:	b9 60 a0 67 [	 ]*cgrtnl	%r6,%r7
.*:	b9 60 c0 67 [	 ]*cgrtnh	%r6,%r7
.*:	b9 60 c0 67 [	 ]*cgrtnh	%r6,%r7
.*:	ec 60 8a d0 a0 72 [	 ]*citnl	%r6,-30000
.*:	ec 60 8a d0 20 72 [	 ]*cith	%r6,-30000
.*:	ec 60 8a d0 20 72 [	 ]*cith	%r6,-30000
.*:	ec 60 8a d0 40 72 [	 ]*citl	%r6,-30000
.*:	ec 60 8a d0 40 72 [	 ]*citl	%r6,-30000
.*:	ec 60 8a d0 60 72 [	 ]*citne	%r6,-30000
.*:	ec 60 8a d0 60 72 [	 ]*citne	%r6,-30000
.*:	ec 60 8a d0 80 72 [	 ]*cite	%r6,-30000
.*:	ec 60 8a d0 80 72 [	 ]*cite	%r6,-30000
.*:	ec 60 8a d0 a0 72 [	 ]*citnl	%r6,-30000
.*:	ec 60 8a d0 a0 72 [	 ]*citnl	%r6,-30000
.*:	ec 60 8a d0 c0 72 [	 ]*citnh	%r6,-30000
.*:	ec 60 8a d0 c0 72 [	 ]*citnh	%r6,-30000
.*:	ec 60 8a d0 a0 70 [	 ]*cgitnl	%r6,-30000
.*:	ec 60 8a d0 20 70 [	 ]*cgith	%r6,-30000
.*:	ec 60 8a d0 20 70 [	 ]*cgith	%r6,-30000
.*:	ec 60 8a d0 40 70 [	 ]*cgitl	%r6,-30000
.*:	ec 60 8a d0 40 70 [	 ]*cgitl	%r6,-30000
.*:	ec 60 8a d0 60 70 [	 ]*cgitne	%r6,-30000
.*:	ec 60 8a d0 60 70 [	 ]*cgitne	%r6,-30000
.*:	ec 60 8a d0 80 70 [	 ]*cgite	%r6,-30000
.*:	ec 60 8a d0 80 70 [	 ]*cgite	%r6,-30000
.*:	ec 60 8a d0 a0 70 [	 ]*cgitnl	%r6,-30000
.*:	ec 60 8a d0 a0 70 [	 ]*cgitnl	%r6,-30000
.*:	ec 60 8a d0 c0 70 [	 ]*cgitnh	%r6,-30000
.*:	ec 60 8a d0 c0 70 [	 ]*cgitnh	%r6,-30000
.*:	e3 67 85 b3 01 34 [	 ]*cgh	%r6,5555\(%r7,%r8\)
.*:	e5 54 64 57 8a d0 [	 ]*chhsi	1111\(%r6\),-30000
.*:	e5 5c 64 57 8a d0 [	 ]*chsi	1111\(%r6\),-30000
.*:	e5 58 64 57 8a d0 [	 ]*cghsi	1111\(%r6\),-30000
 *([\da-f]+):	c6 65 00 00 00 00 [	 ]*chrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 64 00 00 00 00 [	 ]*cghrl	%r6,\1 <foo\+0x\1>
.*:	e5 55 64 57 9c 40 [	 ]*clhhsi	1111\(%r6\),40000
.*:	e5 5d 64 57 9c 40 [	 ]*clfhsi	1111\(%r6\),40000
.*:	e5 59 64 57 9c 40 [	 ]*clghsi	1111\(%r6\),40000
 *([\da-f]+):	c6 6f 00 00 00 00 [	 ]*clrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 6a 00 00 00 00 [	 ]*clgrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 6e 00 00 00 00 [	 ]*clgfrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 67 00 00 00 00 [	 ]*clhrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 66 00 00 00 00 [	 ]*clghrl	%r6,\1 <foo\+0x\1>
.*:	ec 67 84 57 a0 f7 [	 ]*clrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 f7 [	 ]*clrbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 f7 [	 ]*clrbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 f7 [	 ]*clrbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 f7 [	 ]*clrbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 f7 [	 ]*clrbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 f7 [	 ]*clrbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 f7 [	 ]*clrbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 f7 [	 ]*clrbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 f7 [	 ]*clrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 f7 [	 ]*clrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 f7 [	 ]*clrbnh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 f7 [	 ]*clrbnh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 e5 [	 ]*clgrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 e5 [	 ]*clgrbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 20 e5 [	 ]*clgrbh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 e5 [	 ]*clgrbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 40 e5 [	 ]*clgrbl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 e5 [	 ]*clgrbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 60 e5 [	 ]*clgrbne	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 e5 [	 ]*clgrbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 80 e5 [	 ]*clgrbe	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 e5 [	 ]*clgrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 a0 e5 [	 ]*clgrbnl	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 e5 [	 ]*clgrbnh	%r6,%r7,1111\(%r8\)
.*:	ec 67 84 57 c0 e5 [	 ]*clgrbnh	%r6,%r7,1111\(%r8\)
 *([\da-f]+):	ec 67 00 00 a0 77 [	 ]*clrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 77 [	 ]*clrjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 77 [	 ]*clrjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 77 [	 ]*clrjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 77 [	 ]*clrjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 77 [	 ]*clrjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 77 [	 ]*clrjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 77 [	 ]*clrje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 77 [	 ]*clrje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 77 [	 ]*clrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 77 [	 ]*clrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 77 [	 ]*clrjnh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 77 [	 ]*clrjnh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 65 [	 ]*clgrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 65 [	 ]*clgrjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 20 65 [	 ]*clgrjh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 65 [	 ]*clgrjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 40 65 [	 ]*clgrjl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 65 [	 ]*clgrjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 60 65 [	 ]*clgrjne	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 65 [	 ]*clgrje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 80 65 [	 ]*clgrje	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 65 [	 ]*clgrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 a0 65 [	 ]*clgrjnl	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 65 [	 ]*clgrjnh	%r6,%r7,\1 <foo\+0x\1>
 *([\da-f]+):	ec 67 00 00 c0 65 [	 ]*clgrjnh	%r6,%r7,\1 <foo\+0x\1>
.*:	ec 6a 74 57 c8 ff [	 ]*clibnl	%r6,200,1111\(%r7\)
.*:	ec 62 74 57 c8 ff [	 ]*clibh	%r6,200,1111\(%r7\)
.*:	ec 62 74 57 c8 ff [	 ]*clibh	%r6,200,1111\(%r7\)
.*:	ec 64 74 57 c8 ff [	 ]*clibl	%r6,200,1111\(%r7\)
.*:	ec 64 74 57 c8 ff [	 ]*clibl	%r6,200,1111\(%r7\)
.*:	ec 66 74 57 c8 ff [	 ]*clibne	%r6,200,1111\(%r7\)
.*:	ec 66 74 57 c8 ff [	 ]*clibne	%r6,200,1111\(%r7\)
.*:	ec 68 74 57 c8 ff [	 ]*clibe	%r6,200,1111\(%r7\)
.*:	ec 68 74 57 c8 ff [	 ]*clibe	%r6,200,1111\(%r7\)
.*:	ec 6a 74 57 c8 ff [	 ]*clibnl	%r6,200,1111\(%r7\)
.*:	ec 6a 74 57 c8 ff [	 ]*clibnl	%r6,200,1111\(%r7\)
.*:	ec 6c 74 57 c8 ff [	 ]*clibnh	%r6,200,1111\(%r7\)
.*:	ec 6c 74 57 c8 ff [	 ]*clibnh	%r6,200,1111\(%r7\)
.*:	ec 6a 74 57 c8 fd [	 ]*clgibnl	%r6,200,1111\(%r7\)
.*:	ec 62 74 57 c8 fd [	 ]*clgibh	%r6,200,1111\(%r7\)
.*:	ec 62 74 57 c8 fd [	 ]*clgibh	%r6,200,1111\(%r7\)
.*:	ec 64 74 57 c8 fd [	 ]*clgibl	%r6,200,1111\(%r7\)
.*:	ec 64 74 57 c8 fd [	 ]*clgibl	%r6,200,1111\(%r7\)
.*:	ec 66 74 57 c8 fd [	 ]*clgibne	%r6,200,1111\(%r7\)
.*:	ec 66 74 57 c8 fd [	 ]*clgibne	%r6,200,1111\(%r7\)
.*:	ec 68 74 57 c8 fd [	 ]*clgibe	%r6,200,1111\(%r7\)
.*:	ec 68 74 57 c8 fd [	 ]*clgibe	%r6,200,1111\(%r7\)
.*:	ec 6a 74 57 c8 fd [	 ]*clgibnl	%r6,200,1111\(%r7\)
.*:	ec 6a 74 57 c8 fd [	 ]*clgibnl	%r6,200,1111\(%r7\)
.*:	ec 6c 74 57 c8 fd [	 ]*clgibnh	%r6,200,1111\(%r7\)
.*:	ec 6c 74 57 c8 fd [	 ]*clgibnh	%r6,200,1111\(%r7\)
 *([\da-f]+):	ec 6a 00 00 c8 7f [	 ]*clijnl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 c8 7f [	 ]*clijh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 c8 7f [	 ]*clijh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 c8 7f [	 ]*clijl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 c8 7f [	 ]*clijl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 c8 7f [	 ]*clijne	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 c8 7f [	 ]*clijne	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 c8 7f [	 ]*clije	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 c8 7f [	 ]*clije	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 c8 7f [	 ]*clijnl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 c8 7f [	 ]*clijnl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 c8 7f [	 ]*clijnh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 c8 7f [	 ]*clijnh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 c8 7d [	 ]*clgijnl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 c8 7d [	 ]*clgijh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 62 00 00 c8 7d [	 ]*clgijh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 c8 7d [	 ]*clgijl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 64 00 00 c8 7d [	 ]*clgijl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 c8 7d [	 ]*clgijne	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 66 00 00 c8 7d [	 ]*clgijne	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 c8 7d [	 ]*clgije	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 68 00 00 c8 7d [	 ]*clgije	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 c8 7d [	 ]*clgijnl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6a 00 00 c8 7d [	 ]*clgijnl	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 c8 7d [	 ]*clgijnh	%r6,200,\1 <foo\+0x\1>
 *([\da-f]+):	ec 6c 00 00 c8 7d [	 ]*clgijnh	%r6,200,\1 <foo\+0x\1>
.*:	b9 73 a0 67 [	 ]*clrtnl	%r6,%r7
.*:	b9 73 20 67 [	 ]*clrth	%r6,%r7
.*:	b9 73 20 67 [	 ]*clrth	%r6,%r7
.*:	b9 73 40 67 [	 ]*clrtl	%r6,%r7
.*:	b9 73 40 67 [	 ]*clrtl	%r6,%r7
.*:	b9 73 60 67 [	 ]*clrtne	%r6,%r7
.*:	b9 73 60 67 [	 ]*clrtne	%r6,%r7
.*:	b9 73 80 67 [	 ]*clrte	%r6,%r7
.*:	b9 73 80 67 [	 ]*clrte	%r6,%r7
.*:	b9 73 a0 67 [	 ]*clrtnl	%r6,%r7
.*:	b9 73 a0 67 [	 ]*clrtnl	%r6,%r7
.*:	b9 73 c0 67 [	 ]*clrtnh	%r6,%r7
.*:	b9 73 c0 67 [	 ]*clrtnh	%r6,%r7
.*:	b9 61 a0 67 [	 ]*clgrtnl	%r6,%r7
.*:	b9 61 20 67 [	 ]*clgrth	%r6,%r7
.*:	b9 61 20 67 [	 ]*clgrth	%r6,%r7
.*:	b9 61 40 67 [	 ]*clgrtl	%r6,%r7
.*:	b9 61 40 67 [	 ]*clgrtl	%r6,%r7
.*:	b9 61 60 67 [	 ]*clgrtne	%r6,%r7
.*:	b9 61 60 67 [	 ]*clgrtne	%r6,%r7
.*:	b9 61 80 67 [	 ]*clgrte	%r6,%r7
.*:	b9 61 80 67 [	 ]*clgrte	%r6,%r7
.*:	b9 61 a0 67 [	 ]*clgrtnl	%r6,%r7
.*:	b9 61 a0 67 [	 ]*clgrtnl	%r6,%r7
.*:	b9 61 c0 67 [	 ]*clgrtnh	%r6,%r7
.*:	b9 61 c0 67 [	 ]*clgrtnh	%r6,%r7
.*:	ec 60 75 30 a0 73 [	 ]*clfitnl	%r6,30000
.*:	ec 60 75 30 20 73 [	 ]*clfith	%r6,30000
.*:	ec 60 75 30 20 73 [	 ]*clfith	%r6,30000
.*:	ec 60 75 30 40 73 [	 ]*clfitl	%r6,30000
.*:	ec 60 75 30 40 73 [	 ]*clfitl	%r6,30000
.*:	ec 60 75 30 60 73 [	 ]*clfitne	%r6,30000
.*:	ec 60 75 30 60 73 [	 ]*clfitne	%r6,30000
.*:	ec 60 75 30 80 73 [	 ]*clfite	%r6,30000
.*:	ec 60 75 30 80 73 [	 ]*clfite	%r6,30000
.*:	ec 60 75 30 a0 73 [	 ]*clfitnl	%r6,30000
.*:	ec 60 75 30 a0 73 [	 ]*clfitnl	%r6,30000
.*:	ec 60 75 30 c0 73 [	 ]*clfitnh	%r6,30000
.*:	ec 60 75 30 c0 73 [	 ]*clfitnh	%r6,30000
.*:	ec 60 75 30 a0 71 [	 ]*clgitnl	%r6,30000
.*:	ec 60 75 30 20 71 [	 ]*clgith	%r6,30000
.*:	ec 60 75 30 20 71 [	 ]*clgith	%r6,30000
.*:	ec 60 75 30 40 71 [	 ]*clgitl	%r6,30000
.*:	ec 60 75 30 40 71 [	 ]*clgitl	%r6,30000
.*:	ec 60 75 30 60 71 [	 ]*clgitne	%r6,30000
.*:	ec 60 75 30 60 71 [	 ]*clgitne	%r6,30000
.*:	ec 60 75 30 80 71 [	 ]*clgite	%r6,30000
.*:	ec 60 75 30 80 71 [	 ]*clgite	%r6,30000
.*:	ec 60 75 30 a0 71 [	 ]*clgitnl	%r6,30000
.*:	ec 60 75 30 a0 71 [	 ]*clgitnl	%r6,30000
.*:	ec 60 75 30 c0 71 [	 ]*clgitnh	%r6,30000
.*:	ec 60 75 30 c0 71 [	 ]*clgitnh	%r6,30000
.*:	eb 67 84 57 00 4c [	 ]*ecag	%r6,%r7,1111\(%r8\)
 *([\da-f]+):	c4 6d 00 00 00 00 [	 ]*lrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 68 00 00 00 00 [	 ]*lgrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 6c 00 00 00 00 [	 ]*lgfrl	%r6,\1 <foo\+0x\1>
.*:	e3 67 85 b3 01 75 [	 ]*laey	%r6,5555\(%r7,%r8\)
.*:	e3 67 85 b3 01 32 [	 ]*ltgf	%r6,5555\(%r7,%r8\)
 *([\da-f]+):	c4 65 00 00 00 00 [	 ]*lhrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 64 00 00 00 00 [	 ]*lghrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 6e 00 00 00 00 [	 ]*llgfrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 62 00 00 00 00 [	 ]*llhrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 66 00 00 00 00 [	 ]*llghrl	%r6,\1 <foo\+0x\1>
.*:	e5 44 64 57 8a d0 [	 ]*mvhhi	1111\(%r6\),-30000
.*:	e5 4c 64 57 8a d0 [	 ]*mvhi	1111\(%r6\),-30000
.*:	e5 48 64 57 8a d0 [	 ]*mvghi	1111\(%r6\),-30000
.*:	e3 67 85 b3 01 5c [	 ]*mfy	%r6,5555\(%r7,%r8\)
.*:	e3 67 85 b3 01 7c [	 ]*mhy	%r6,5555\(%r7,%r8\)
.*:	c2 61 ff fe 79 60 [	 ]*msfi	%r6,-100000
.*:	c2 60 ff fe 79 60 [	 ]*msgfi	%r6,-100000
.*:	e3 a6 75 b3 01 36 [	 ]*pfd	10,5555\(%r6,%r7\)
 *([\da-f]+):	c6 a2 00 00 00 00 [	 ]*pfdrl	10,\1 <foo\+0x\1>
.*:	ec 67 d2 dc e6 54 [	 ]*rnsbg	%r6,%r7,210,220,230
.*:	ec 67 d2 dc 00 54 [	 ]*rnsbg	%r6,%r7,210,220
.*:	ec 67 92 dc e6 54 [	 ]*rnsbgt	%r6,%r7,18,220,230
.*:	ec 67 92 dc 00 54 [	 ]*rnsbgt	%r6,%r7,18,220
.*:	ec 67 92 1c 26 54 [	 ]*rnsbgt	%r6,%r7,18,28,38
.*:	ec 67 92 1c 00 54 [	 ]*rnsbgt	%r6,%r7,18,28
.*:	ec 67 d2 dc e6 57 [	 ]*rxsbg	%r6,%r7,210,220,230
.*:	ec 67 d2 dc 00 57 [	 ]*rxsbg	%r6,%r7,210,220
.*:	ec 67 92 dc e6 57 [	 ]*rxsbgt	%r6,%r7,18,220,230
.*:	ec 67 92 dc 00 57 [	 ]*rxsbgt	%r6,%r7,18,220
.*:	ec 67 92 1c 26 57 [	 ]*rxsbgt	%r6,%r7,18,28,38
.*:	ec 67 92 1c 00 57 [	 ]*rxsbgt	%r6,%r7,18,28
.*:	ec 67 d2 dc e6 56 [	 ]*rosbg	%r6,%r7,210,220,230
.*:	ec 67 d2 dc 00 56 [	 ]*rosbg	%r6,%r7,210,220
.*:	ec 67 92 dc e6 56 [	 ]*rosbgt	%r6,%r7,18,220,230
.*:	ec 67 92 dc 00 56 [	 ]*rosbgt	%r6,%r7,18,220
.*:	ec 67 92 1c 26 56 [	 ]*rosbgt	%r6,%r7,18,28,38
.*:	ec 67 92 1c 00 56 [	 ]*rosbgt	%r6,%r7,18,28
.*:	ec 67 d2 14 e6 55 [	 ]*risbg	%r6,%r7,210,20,230
.*:	ec 67 d2 14 00 55 [	 ]*risbg	%r6,%r7,210,20
.*:	ec 67 d2 bc e6 55 [	 ]*risbgz	%r6,%r7,210,60,230
.*:	ec 67 d2 bc 00 55 [	 ]*risbgz	%r6,%r7,210,60
.*:	ec 67 d2 94 e6 55 [	 ]*risbgz	%r6,%r7,210,20,230
.*:	ec 67 d2 94 00 55 [	 ]*risbgz	%r6,%r7,210,20
 *([\da-f]+):	c4 6f 00 00 00 00 [	 ]*strl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 6b 00 00 00 00 [	 ]*stgrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c4 67 00 00 00 00 [	 ]*sthrl	%r6,\1 <foo\+0x\1>
 *([\da-f]+):	c6 60 00 00 00 00 [	 ]*exrl	%r6,\1 <foo\+0x\1>
.*:	af ee 6d 05 [	 ]*mc	3333\(%r6\),238
.*:	b9 a2 00 60 [	 ]*ptf	%r6
.*:	b9 af 00 67 [	 ]*pfmf	%r6,%r7
.*:	b9 bf a0 67 [	 ]*trte	%r6,%r7,10
.*:	b9 bf 00 67 [	 ]*trte	%r6,%r7
.*:	b9 bd a0 67 [	 ]*trtre	%r6,%r7,10
.*:	b9 bd 00 67 [	 ]*trtre	%r6,%r7
.*:	b2 ed 00 67 [	 ]*ecpga	%r6,%r7
.*:	b2 e4 00 67 [	 ]*ecctr	%r6,%r7
.*:	b2 e5 00 67 [	 ]*epctr	%r6,%r7
.*:	b2 84 6d 05 [	 ]*lcctl	3333\(%r6\)
.*:	b2 85 6d 05 [	 ]*lpctl	3333\(%r6\)
.*:	b2 87 6d 05 [	 ]*lsctl	3333\(%r6\)
.*:	b2 8e 6d 05 [	 ]*qctri	3333\(%r6\)
.*:	b2 86 6d 05 [	 ]*qsi	3333\(%r6\)
.*:	b2 e0 00 67 [	 ]*scctr	%r6,%r7
.*:	b2 e1 00 67 [	 ]*spctr	%r6,%r7
.*:	b2 80 6d 05 [	 ]*lpp	3333\(%r6\)
.*:	07 07 [	 ]*nopr	%r7
