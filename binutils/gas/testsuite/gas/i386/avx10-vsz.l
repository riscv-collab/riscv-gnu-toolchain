.*: Assembler messages:
.*:7: Warning: default
.*:25: Error: ambiguous operand size for `vcvtpd2ph'
.*:26: Error: ambiguous operand size for `vcvtpd2ps'
.*:27: Error: ambiguous operand size for `vfpclassps'
.*:7: Warning: \.avx10\.1/256
.*:11: Error: vector size .* `vcvtpd2phz'
.*:13: Error: unsupported broadcast for `vcvtpd2ph'
.*:16: Error: vector size .* for `vcvtpd2ps'
.*:22: Error: vector size .* `vfpclasspsz'
.*:25: Error: ambiguous operand size for `vcvtpd2ph'
.*:26: Error: ambiguous operand size for `vcvtpd2ps'
.*:27: Error: ambiguous operand size for `vfpclassps'
.*:7: Warning: \.avx10\.1/128
.*:11: Error: vector size .* `vcvtpd2phz'
.*:12: Error: vector size .* `vcvtpd2phy'
.*:13: Error: unsupported broadcast for `vcvtpd2ph'
.*:14: Error: unsupported broadcast for `vcvtpd2ph'
.*:16: Error: .*
.*:17: Error: vector size .* `vcvtpd2psy'
.*:18: Error: vector size .* `vcvtpd2psy'
.*:19: Error: unsupported broadcast for `vcvtpd2ps'
.*:21: Error: vector size .* `vfpclasspsy'
.*:22: Error: vector size .* `vfpclasspsz'
.*:7: Warning: \.avx10\.1
.*:25: Error: ambiguous operand size for `vcvtpd2ph'
.*:26: Error: ambiguous operand size for `vcvtpd2ps'
.*:27: Error: ambiguous operand size for `vfpclassps'
#...
[ 	]*[0-9]+[ 	]+>  \.arch generic32
[ 	]*[0-9]+[ 	]+>  \.arch default
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F5FD48 5A00[ 	]+>  vcvtpd2phz \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD28 5A00[ 	]+>  vcvtpd2phy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD58 5A00[ 	]+>  vcvtpd2ph \(%eax\)\{1to8\},%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD38 5A00[ 	]+>  vcvtpd2ph \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F1FD48 5A00[ 	]+>  vcvtpd2ps \(%eax\),%ymm0
[ 	]*[0-9]+[ 	]+\?+ C5FD5A00[ 	]+>  vcvtpd2psy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F1FD29 5A00[ 	]+>  vcvtpd2psy \(%eax\),%xmm0\{%k1\}
[ 	]*[0-9]+[ 	]+\?+ 62F1FD38 5A00[ 	]+>  vcvtpd2ps \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F37D28 660000[ 	]+>  vfpclasspsy \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+\?+ 62F37D48 660000[ 	]+>  vfpclasspsz \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  \.intel_syntax noprefix
[ 	]*[0-9]+[ 	]+>  vcvtpd2ph xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+>  vcvtpd2ps xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+>  vfpclassps k0,\[eax\],0
#...
[ 	]*[0-9]+[ 	]+>  \.arch generic32
[ 	]*[0-9]+[ 	]+>  \.arch \.avx10\.1/256
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  vcvtpd2phz \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD28 5A00[ 	]+>  vcvtpd2phy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+>  vcvtpd2ph \(%eax\)\{1to8\},%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD38 5A00[ 	]+>  vcvtpd2ph \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  vcvtpd2ps \(%eax\),%ymm0
[ 	]*[0-9]+[ 	]+\?+ C5FD5A00[ 	]+>  vcvtpd2psy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F1FD29 5A00[ 	]+>  vcvtpd2psy \(%eax\),%xmm0\{%k1\}
[ 	]*[0-9]+[ 	]+\?+ 62F1FD38 5A00[ 	]+>  vcvtpd2ps \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F37D28 660000[ 	]+>  vfpclasspsy \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+>  vfpclasspsz \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  \.intel_syntax noprefix
[ 	]*[0-9]+[ 	]+>  vcvtpd2ph xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+>  vcvtpd2ps xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+>  vfpclassps k0,\[eax\],0
#...
[ 	]*[0-9]+[ 	]+>  \.arch generic32
[ 	]*[0-9]+[ 	]+>  \.arch \.avx10\.1/128
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  vcvtpd2phz \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+>  vcvtpd2phy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+>  vcvtpd2ph \(%eax\)\{1to8\},%xmm0
[ 	]*[0-9]+[ 	]+>  vcvtpd2ph \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  vcvtpd2ps \(%eax\),%ymm0
[ 	]*[0-9]+[ 	]+>  vcvtpd2psy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+>  vcvtpd2psy \(%eax\),%xmm0\{%k1\}
[ 	]*[0-9]+[ 	]+>  vcvtpd2ps \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  vfpclasspsy \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+>  vfpclasspsz \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  \.intel_syntax noprefix
[ 	]*[0-9]+[ 	]+\?+ 62F5FD08 5A00[ 	]+>  vcvtpd2ph xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+\?+ C5F95A00[ 	]+>  vcvtpd2ps xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+\?+ 62F37D08 660000[ 	]+>  vfpclassps k0,\[eax\],0
#...
[ 	]*[0-9]+[ 	]+>  \.arch generic32
[ 	]*[0-9]+[ 	]+>  \.arch \.avx10\.1
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F5FD48 5A00[ 	]+>  vcvtpd2phz \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD28 5A00[ 	]+>  vcvtpd2phy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD58 5A00[ 	]+>  vcvtpd2ph \(%eax\)\{1to8\},%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F5FD38 5A00[ 	]+>  vcvtpd2ph \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F1FD48 5A00[ 	]+>  vcvtpd2ps \(%eax\),%ymm0
[ 	]*[0-9]+[ 	]+\?+ C5FD5A00[ 	]+>  vcvtpd2psy \(%eax\),%xmm0
[ 	]*[0-9]+[ 	]+\?+ 62F1FD29 5A00[ 	]+>  vcvtpd2psy \(%eax\),%xmm0\{%k1\}
[ 	]*[0-9]+[ 	]+\?+ 62F1FD38 5A00[ 	]+>  vcvtpd2ps \(%eax\)\{1to4\},%xmm0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+\?+ 62F37D28 660000[ 	]+>  vfpclasspsy \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+\?+ 62F37D48 660000[ 	]+>  vfpclasspsz \$0,\(%eax\),%k0
[ 	]*[0-9]+[ 	]+> *
[ 	]*[0-9]+[ 	]+>  \.intel_syntax noprefix
[ 	]*[0-9]+[ 	]+>  vcvtpd2ph xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+>  vcvtpd2ps xmm0,\[eax\]
[ 	]*[0-9]+[ 	]+>  vfpclassps k0,\[eax\],0
#pass
