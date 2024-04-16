.*: Assembler messages:
.*:8: Error: .*bad register name `%zmm.*
.*:11: Error: .*bad register name `%zmm.*
.*:14: Error: .*bad register name `%zmm.*
.*:17: Error: .*bad register name `%zmm.*
.*:18: Error: .*bad register name `%zmm.*
.*:21: Error: .*bad register name `%zmm.*
.*:24: Error: .*vector size.*
.*:25: Error: .*bad register name `%zmm.*
.*:8: Error: .*bad register name `%zmm.*
.*:10: Error: .*bad register name `%ymm.*
.*:11: Error: .*bad register name `%zmm.*
.*:13: Error: .*bad register name `%ymm.*
.*:14: Error: .*bad register name `%zmm.*
.*:16: Error: .*bad register name `%ymm.*
.*:17: Error: .*bad register name `%zmm.*
.*:18: Error: .*bad register name `%zmm.*
.*:20: Error: .*bad register name `%ymm.*
.*:21: Error: .*bad register name `%zmm.*
.*:23: Error: .*bad register name `%ymm.*
.*:24: Error: .*
.*:25: Error: .*bad register name `%zmm.*
.*:27: Error: .*bad register name `%ymm.*
.*:9: Error: .*unsupported masking.*
.*:10: Error: .*unsupported masking.*
.*:12: Error: .*operand size mismatch.*
.*:13: Error: .*operand size mismatch.*
.*:15: Error: .*operand size mismatch.*
.*:16: Error: .*operand size mismatch.*
.*:19: Error: .*unsupported masking.*
.*:20: Error: .*unsupported masking.*
.*:22: Error: .*operand size mismatch.*
.*:23: Error: .*operand size mismatch.*
.*:26: Error: .*operand size mismatch.*
.*:27: Error: .*operand size mismatch.*
#...
[ 	]*[0-9]+[ 	]+\# Test \.arch \.noavx512vl
[ 	]*[0-9]+[ 	]+\.text
[ 	]*[0-9]+[ 	]*
[ 	]*[0-9]+[ 	]+\.irp isa, default, .*
#...
[ 	]*[0-9]+[ 	]+\.endr
#...
[ 	]*[0-9]+[ 	]+>  \.arch default
[ 	]*[0-9]+[ 	]+>  \.arch default
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D4F 	>  vpabsb %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D0F 	>  vpabsb %xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D2F 	>  vpabsb %ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D48 	>  vpconflictd %zmm5,%zmm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D08 	>  vpconflictd %xmm5,%xmm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D28 	>  vpconflictd %ymm5,%ymm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD4F 	>  vcvtpd2qq \(%ecx\),%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD0F 	>  vcvtpd2qq \(%ecx\),%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD2F 	>  vcvtpd2qq \(%ecx\),%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D4F 	>  vexp2ps %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+C8F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D54F 	>  vaddpd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D50F 	>  vaddpd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D52F 	>  vaddpd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D54F 	>  vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D50F 	>  vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D52F 	>  vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2FD49 	>  vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}
[ 	]*[0-9]+[ 	]+C68CFD17 *
[ 	]*[0-9]+[ 	]+000000
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2554F 	>  vpermb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2550F 	>  vpermb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2552F 	>  vpermb %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
#...
[ 	]*[0-9]+[ 	]+>  \.arch default
[ 	]*[0-9]+[ 	]+>  \.arch \.avx10\.1/256
[ 	]*[0-9]+[ 	]+>  vpabsb %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D0F 	>  vpabsb %xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D2F 	>  vpabsb %ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+>  vpconflictd %zmm5,%zmm6
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D08 	>  vpconflictd %xmm5,%xmm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D28 	>  vpconflictd %ymm5,%ymm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+>  vcvtpd2qq \(%ecx\),%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD0F 	>  vcvtpd2qq \(%ecx\),%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD2F 	>  vcvtpd2qq \(%ecx\),%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+>  vexp2ps %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vaddpd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D50F 	>  vaddpd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D52F 	>  vaddpd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+>  vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D50F 	>  vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D52F 	>  vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+>  vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}
[ 	]*[0-9]+[ 	]+>  vpermb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2550F 	>  vpermb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2552F 	>  vpermb %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
#...
[ 	]*[0-9]+[ 	]+>  \.arch default
[ 	]*[0-9]+[ 	]+>  \.arch \.avx10\.1/128
[ 	]*[0-9]+[ 	]+>  vpabsb %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D0F 	>  vpabsb %xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+>  vpabsb %ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vpconflictd %zmm5,%zmm6
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D08 	>  vpconflictd %xmm5,%xmm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+>  vpconflictd %ymm5,%ymm6
[ 	]*[0-9]+[ 	]+>  vcvtpd2qq \(%ecx\),%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD0F 	>  vcvtpd2qq \(%ecx\),%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+>  vcvtpd2qq \(%ecx\),%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vexp2ps %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vaddpd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D50F 	>  vaddpd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+>  vaddpd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D50F 	>  vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+>  vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}
[ 	]*[0-9]+[ 	]+>  vpermb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2550F 	>  vpermb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
[ 	]*[0-9]+[ 	]+>  vpermb %ymm4,%ymm5,%ymm6\{%k7\}
#...
[ 	]*[0-9]+[ 	]+>  \.arch \.noavx512vl
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D4F 	>  vpabsb %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+1CF5
[ 	]*[0-9]+[ 	]+>  vpabsb %xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vpabsb %ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D48 	>  vpconflictd %zmm5,%zmm6
[ 	]*[0-9]+[ 	]+C4F5
[ 	]*[0-9]+[ 	]+>  vpconflictd %xmm5,%xmm6
[ 	]*[0-9]+[ 	]+>  vpconflictd %ymm5,%ymm6
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1FD4F 	>  vcvtpd2qq \(%ecx\),%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+7B31
[ 	]*[0-9]+[ 	]+>  vcvtpd2qq \(%ecx\),%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vcvtpd2qq \(%ecx\),%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F27D4F 	>  vexp2ps %zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+C8F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F1D54F 	>  vaddpd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+58F4
[ 	]*[0-9]+[ 	]+>  vaddpd %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vaddpd %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2D54F 	>  vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+B4F4
[ 	]*[0-9]+[ 	]+>  vpmadd52luq %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vpmadd52luq %ymm4,%ymm5,%ymm6\{%k7\}
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2FD49 	>  vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}
[ 	]*[0-9]+[ 	]+C68CFD17 *
[ 	]*[0-9]+[ 	]+000000
[ 	]*[0-9]+[ 	]+\?\?\?\? 62F2554F 	>  vpermb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[0-9]+[ 	]+8DF4
[ 	]*[0-9]+[ 	]+>  vpermb %xmm4,%xmm5,%xmm6\{%k7\}
[ 	]*[0-9]+[ 	]+>  vpermb %ymm4,%ymm5,%ymm6\{%k7\}
#...
[ 	]*[0-9]+[ 	]+\?\?\?\? C4E2791C 		vpabsb %xmm5, %xmm6
[ 	]*[0-9]+[ 	]+F5
[ 	]*[0-9]+[ 	]+\?\?\?\? C4E27D1C 		vpabsb %ymm5, %ymm6
[ 	]*[0-9]+[ 	]+F5
[ 	]*[0-9]+[ 	]+\?\?\?\? C5D158F4 		vaddpd %xmm4, %xmm5, %xmm6
[ 	]*[0-9]+[ 	]+\?\?\?\? C5D558F4 		vaddpd %ymm4, %ymm5, %ymm6
[ 	]*[0-9]+[ 	]+\?\?\?\? 660F381C 		pabsb %xmm5, %xmm6
[ 	]*[0-9]+[ 	]+F5
[ 	]*[0-9]+[ 	]+\?\?\?\? 660F58F4 		addpd %xmm4, %xmm6
[ 	]*[0-9]+[ 	]+
[ 	]*[1-9][0-9]*[ 	]+\.intel_syntax noprefix
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 62F3FD48 		vfpclasspd k0, \[eax], 0
[ 	]*[1-9][0-9]*[ 	]+660000
[ 	]*[1-9][0-9]*[ 	]+\?\?\?\? 62F37D48 		vfpclassps k0, \[eax\+0x80], 0
[ 	]*[1-9][0-9]*[ 	]+66400200 ?
[ 	]*[1-9][0-9]*[ 	]+
#pass
