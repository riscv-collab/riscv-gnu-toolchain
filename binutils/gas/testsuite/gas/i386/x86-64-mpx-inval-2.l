.*: Assembler messages:
.*:5: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:6: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:7: Error: `\(%rip\)' cannot be used here
.*:8: Error: .*
.*:11: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:12: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:14: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:15: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:18: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:19: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:20: Error: .* `bndcl'
.*:21: Error: .* `bndcl'
.*:24: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:25: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:26: Error: .* `bndcu'
.*:27: Error: .* `bndcu'
.*:30: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:31: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:32: Error: .* `bndcn'
.*:33: Error: .* `bndcn'
.*:36: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:37: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:38: Warning: register scaling is being ignored here
.*:39: Error: `base\(%rip\)' cannot be used here
.*:40: Error: .*
.*:43: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:44: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:45: Warning: register scaling is being ignored here
.*:46: Error: `base\(%rip\)' cannot be used here
.*:47: Error: .*
.*:50: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:51: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:52: Error: `\[rip\]' cannot be used here
.*:53: Error: .*
.*:54: Error: `\[rax\+rsp\]' is not a valid base/index expression
.*:57: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:58: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:60: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:61: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:64: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:65: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:66: Error: .* `bndcl'
.*:67: Error: .* `bndcl'
.*:70: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:71: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:72: Error: .* `bndcu'
.*:73: Error: .* `bndcu'
.*:76: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:77: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:78: Error: .* `bndcn'
.*:79: Error: .* `bndcn'
.*:82: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:83: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:84: Warning: register scaling is being ignored here
.*:85: Error: `\[rip\+base\]' cannot be used here
.*:86: Error: .*
.*:87: Error: `\[rax\+rsp\]' is not a valid base/index expression
.*:90: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:91: Error: 32-bit address isn't allowed in 64-bit MPX instructions.
.*:92: Warning: register scaling is being ignored here
.*:93: Error: `\[rip\+base\]' cannot be used here
.*:94: Error: .*
.*:95: Error: `\[rax\+rsp\]' is not a valid base/index expression
GAS LISTING .*


[ 	]*1[ 	]+\# MPX instructions
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndmk
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1B 		bndmk \(%eax\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1B 		bndmk 0x3\(%ecx,%ebx,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C1903
[ 	]*[1-9][0-9]*[ 	]+bndmk \(%rip\), %bnd3
[ 	]*[1-9][0-9]*[ 	]+bndmk \(%eip\), %bnd2
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndmov
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 6766410F 		bndmov \(%r8d\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1A08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 6766410F 		bndmov 0x3\(%r9d,%edx,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1A4C1103 
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67660F1B 		bndmov %bnd1, \(%eax\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67660F1B 		bndmov %bnd1, 0x3\(%ecx,%eax,1\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0103
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndcl
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1A 		bndcl \(%ecx\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+09
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1A 		bndcl 0x3\(%ecx,%eax,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0103
[ 	]*[1-9][0-9]*[ 	]+bndcl %ecx, %bnd1
[ 	]*[1-9][0-9]*[ 	]+bndcl %cx, %bnd1
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndcu
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1A 		bndcu \(%ecx\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+09
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1A 		bndcu 0x3\(%ecx,%eax,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0103
[ 	]*[1-9][0-9]*[ 	]+bndcu %ecx, %bnd1
[ 	]*[1-9][0-9]*[ 	]+bndcu %cx, %bnd1
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndcn
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1B 		bndcn \(%ecx\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+09
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1B 		bndcn 0x3\(%ecx,%eax,1\), %bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0103
[ 	]*[1-9][0-9]*[ 	]+bndcn %ecx, %bnd1
[ 	]*[1-9][0-9]*[ 	]+bndcn %cx, %bnd1
GAS LISTING .*


[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndstx
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1B44 		bndstx %bnd0, 0x3\(%eax,%ebx,1\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1803
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1B53 		bndstx %bnd2, 3\(%ebx,1\)
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+03
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 410F1B0C 		bndstx %bnd1, \(%r15,%rax,2\)
.*  Warning: register scaling is being ignored here
[ 	]*[1-9][0-9]*[ 	]+47
[ 	]*[1-9][0-9]*[ 	]+bndstx %bnd3, base\(%rip\)
[ 	]*[1-9][0-9]*[ 	]+bndstx %bnd1, base\(%eip\)
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndldx
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1A44 		bndldx 0x3\(%eax,%ebx,1\), %bnd0
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1803
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1A53 		bndldx 3\(%ebx,1\), %bnd2
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+03
[ 	]*[1-9][0-9]*[ 	]*\?\?\?\? 420F1A1C 		bndldx \(%rax,%r15,4\), %bnd3
.*  Warning: register scaling is being ignored here
[ 	]*[1-9][0-9]*[ 	]*B8
[ 	]*[1-9][0-9]*[ 	]*bndldx base\(%rip\), %bnd1
[ 	]*[1-9][0-9]*[ 	]*bndldx base\(%eip\), %bnd3
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\.intel_syntax noprefix
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1B 		bndmk bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1B 		bndmk bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0203
[ 	]*[1-9][0-9]*[ 	]*bndmk bnd3, \[rip\]
[ 	]*[1-9][0-9]*[ 	]*bndmk bnd2, \[eip\]
[ 	]*[1-9][0-9]*[ 	]+bndmk bnd2, \[rax\+rsp\]
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndmov
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67660F1A 		bndmov bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67660F1A 		bndmov bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0203
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67660F1B 		bndmov \[eax\], bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67660F1B 		bndmov \[edx\+1\*eax\+0x3\], bnd1
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0203
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndcl
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1A 		bndcl bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
GAS LISTING .*


[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F30F1A 		bndcl bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0203
[ 	]*[1-9][0-9]*[ 	]+bndcl bnd1, eax
[ 	]*[1-9][0-9]*[ 	]+bndcl bnd1, dx
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndcu
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1A 		bndcu bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1A 		bndcu bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0203
[ 	]*[1-9][0-9]*[ 	]+bndcu bnd1, eax
[ 	]*[1-9][0-9]*[ 	]+bndcu bnd1, dx
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndcn
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1B 		bndcn bnd1, \[eax\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+08
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 67F20F1B 		bndcn bnd1, \[edx\+1\*eax\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+4C0203
[ 	]*[1-9][0-9]*[ 	]+bndcn bnd1, eax
[ 	]*[1-9][0-9]*[ 	]+bndcn bnd1, dx
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndstx
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1B44 		bndstx \[eax\+ebx\*1\+0x3\], bnd0
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1803
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1B14 		bndstx \[1\*ebx\+3\], bnd2
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1D030000 
[ 	]*[1-9][0-9]*[ 	]+00
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 410F1B14 		bndstx \[r8\+rdi\*4\], bnd2
.*  Warning: register scaling is being ignored here
[ 	]*[1-9][0-9]*[ 	]+B8
[ 	]*[1-9][0-9]*[ 	]+bndstx \[rip\+base\], bnd1
[ 	]*[1-9][0-9]*[ 	]+bndstx \[eip\+base\], bnd3
[ 	]*[1-9][0-9]*[ 	]+bndstx \[rax\+rsp\], bnd3
[ 	]*[1-9][0-9]*[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\#\#\# bndldx
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1A44 		bndldx bnd0, \[eax\+ebx\*1\+0x3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1803
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 670F1A14 		bndldx bnd2, \[1\*ebx\+3\]
.*  Error: 32-bit address isn't allowed in 64-bit MPX instructions\.
[ 	]*[1-9][0-9]*[ 	]+1D030000 
[ 	]*[1-9][0-9]*[ 	]+00
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 420F1A14 		bndldx bnd2, \[rdi\+r8\*8\]
.*  Warning: register scaling is being ignored here
[ 	]*[1-9][0-9]*[ 	]+C7
[ 	]*[1-9][0-9]*[ 	]+bndldx bnd1, \[rip\+base\]
[ 	]*[1-9][0-9]*[ 	]+bndldx bnd3, \[eip\+base\]
[ 	]*[1-9][0-9]*[ 	]+bndldx bnd3, \[rax\+rsp\]
#pass
