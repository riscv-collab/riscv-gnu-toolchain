.*: Assembler messages:
.*:6: Error: invalid .* `addss' after `repe'
.*:7: Error: invalid .* `addss' after `repne'
.*:8: Error: invalid .* `vaddss' after `repe'
.*:9: Error: invalid .* `vaddss' after `repne'
.*:11: Error: .*disp16.* is not supported .*
.*:16: Error: same type of prefix .*
.*:17: Error: same type of prefix .*
.*:20: Error: data size .* `addps'
.*:21: Error: data size .* `addpd'
.*:22: Error: data size .* `vaddps'
.*:23: Error: data size .* `vaddpd'
GAS LISTING .*
#...
[ 	]*1[ 	]+\.text
[ 	]*2[ 	]+prefix:
[ 	]*3[ 	]+\?\?\?\? 67E203[ 	]+addr32 loop	\.Lrep_ret
[ 	]*4[ 	]+\?\?\?\? 67E304[ 	]+addr32 jrcxz	\.Ldata16
[ 	]*5[ 	]*
[ 	]*6[ 	]+repe addss	%xmm0, %xmm0
[ 	]*7[ 	]+repne addss	%xmm0, %xmm0
[ 	]*8[ 	]+repe vaddss	%xmm0, %xmm0, %xmm0
[ 	]*9[ 	]+repne vaddss	%xmm0, %xmm0, %xmm0
[ 	]*10[ 	]*
[ 	]*[0-9]+[ 	]+\{disp16\} nop
[ 	]*[0-9]+[ 	]*
[ 	]*[0-9]+[ 	]+\.Lrep_ret:
[ 	]*[0-9]+[ 	]+\?\?\?\? F2C3[ 	]+bnd ret
[ 	]*[0-9]+[ 	]+\?\?\?\? F3C3[ 	]+rep ret
[ 	]*[0-9]+[ 	]+bnd rep ret
[ 	]*[0-9]+[ 	]+rep bnd ret
[ 	]*[0-9]+[ 	]*
[ 	]*[0-9]+[ 	]+\.Ldata16:
[ 	]*[0-9]+[ 	]+data16 addps	%xmm0, %xmm0
[ 	]*[0-9]+[ 	]+data16 addpd	%xmm0, %xmm0
[ 	]*[0-9]+[ 	]+data16 vaddps	%xmm0, %xmm0, %xmm0
[ 	]*[0-9]+[ 	]+data16 vaddpd	%xmm0, %xmm0, %xmm0
[ 	]*[0-9]+[ 	]*
#pass
