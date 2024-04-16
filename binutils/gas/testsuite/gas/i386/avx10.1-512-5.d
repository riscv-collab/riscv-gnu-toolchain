#as: -I${srcdir}/$subdir
#objdump: -dw
#name: AVX10.1/512 (part 5)

.*: +file format .*


Disassembly of section \.text:

0+ <bitalg>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8f ec[ 	]*vpshufbitqmb %zmm4,%zmm5,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 8f ec[ 	]*vpshufbitqmb %zmm4,%zmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8f ac f4 c0 1d fe ff[ 	]*vpshufbitqmb -0x1e240\(%esp,%esi,8\),%zmm5,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8f 6a 7f[ 	]*vpshufbitqmb 0x1fc0\(%edx\),%zmm5,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 54 f5[ 	]*vpopcntb %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 54 f5[ 	]*vpopcntb %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 54 f5[ 	]*vpopcntb %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 54 b4 f4 c0 1d fe ff[ 	]*vpopcntb -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 54 72 7f[ 	]*vpopcntb 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 54 f5[ 	]*vpopcntw %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 54 f5[ 	]*vpopcntw %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 54 f5[ 	]*vpopcntw %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 54 b4 f4 c0 1d fe ff[ 	]*vpopcntw -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 54 72 7f[ 	]*vpopcntw 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 f5[ 	]*vpopcntd %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 72 7f[ 	]*vpopcntd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 f5[ 	]*vpopcntq %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 72 7f[ 	]*vpopcntq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8f ec[ 	]*vpshufbitqmb %zmm4,%zmm5,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 8f ec[ 	]*vpshufbitqmb %zmm4,%zmm5,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8f ac f4 c0 1d fe ff[ 	]*vpshufbitqmb -0x1e240\(%esp,%esi,8\),%zmm5,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8f 6a 7f[ 	]*vpshufbitqmb 0x1fc0\(%edx\),%zmm5,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 54 f5[ 	]*vpopcntb %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 54 f5[ 	]*vpopcntb %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 54 f5[ 	]*vpopcntb %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 54 b4 f4 c0 1d fe ff[ 	]*vpopcntb -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 54 72 7f[ 	]*vpopcntb 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 54 f5[ 	]*vpopcntw %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 54 f5[ 	]*vpopcntw %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 54 f5[ 	]*vpopcntw %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 54 b4 f4 c0 1d fe ff[ 	]*vpopcntw -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 54 72 7f[ 	]*vpopcntw 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 f5[ 	]*vpopcntd %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 72 7f[ 	]*vpopcntd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 f5[ 	]*vpopcntq %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 72 7f[ 	]*vpopcntq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to8\},%zmm6

0+[a-f0-9]+ <cd>:
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 f5    	vpconflictd %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 4f c4 f5    	vpconflictd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7d cf c4 f5    	vpconflictd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 31    	vpconflictd \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 b4 f4 c0 1d fe ff 	vpconflictd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 30    	vpconflictd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 72 7f 	vpconflictd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 b2 00 20 00 00 	vpconflictd 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 72 80 	vpconflictd -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 b2 c0 df ff ff 	vpconflictd -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 72 7f 	vpconflictd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 b2 00 02 00 00 	vpconflictd 0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 72 80 	vpconflictd -0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 b2 fc fd ff ff 	vpconflictd -0x204\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 f5    	vpconflictq %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 4f c4 f5    	vpconflictq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf c4 f5    	vpconflictq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 31    	vpconflictq \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 b4 f4 c0 1d fe ff 	vpconflictq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 30    	vpconflictq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 72 7f 	vpconflictq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 b2 00 20 00 00 	vpconflictq 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 72 80 	vpconflictq -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 b2 c0 df ff ff 	vpconflictq -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 72 7f 	vpconflictq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 b2 00 04 00 00 	vpconflictq 0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 72 80 	vpconflictq -0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 b2 f8 fb ff ff 	vpconflictq -0x408\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 f5    	vplzcntd %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 4f 44 f5    	vplzcntd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7d cf 44 f5    	vplzcntd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 31    	vplzcntd \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 b4 f4 c0 1d fe ff 	vplzcntd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 30    	vplzcntd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 72 7f 	vplzcntd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 b2 00 20 00 00 	vplzcntd 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 72 80 	vplzcntd -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 b2 c0 df ff ff 	vplzcntd -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 72 7f 	vplzcntd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 b2 00 02 00 00 	vplzcntd 0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 72 80 	vplzcntd -0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 b2 fc fd ff ff 	vplzcntd -0x204\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 f5    	vplzcntq %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 4f 44 f5    	vplzcntq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 44 f5    	vplzcntq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 31    	vplzcntq \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 b4 f4 c0 1d fe ff 	vplzcntq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 30    	vplzcntq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 72 7f 	vplzcntq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 b2 00 20 00 00 	vplzcntq 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 72 80 	vplzcntq -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 b2 c0 df ff ff 	vplzcntq -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 72 7f 	vplzcntq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 b2 00 04 00 00 	vplzcntq 0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 72 80 	vplzcntq -0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 b2 f8 fb ff ff 	vplzcntq -0x408\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7e 48 3a f6    	vpbroadcastmw2d %k6,%zmm6
[ 	]*[a-f0-9]+:	62 f2 fe 48 2a f6    	vpbroadcastmb2q %k6,%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 f5    	vpconflictd %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 4f c4 f5    	vpconflictd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7d cf c4 f5    	vpconflictd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 31    	vpconflictd \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 b4 f4 c0 1d fe ff 	vpconflictd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 30    	vpconflictd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 72 7f 	vpconflictd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 b2 00 20 00 00 	vpconflictd 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 72 80 	vpconflictd -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 c4 b2 c0 df ff ff 	vpconflictd -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 72 7f 	vpconflictd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 b2 00 02 00 00 	vpconflictd 0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 72 80 	vpconflictd -0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 c4 b2 fc fd ff ff 	vpconflictd -0x204\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 f5    	vpconflictq %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 4f c4 f5    	vpconflictq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf c4 f5    	vpconflictq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 31    	vpconflictq \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 b4 f4 c0 1d fe ff 	vpconflictq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 30    	vpconflictq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 72 7f 	vpconflictq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 b2 00 20 00 00 	vpconflictq 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 72 80 	vpconflictq -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 c4 b2 c0 df ff ff 	vpconflictq -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 72 7f 	vpconflictq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 b2 00 04 00 00 	vpconflictq 0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 72 80 	vpconflictq -0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 c4 b2 f8 fb ff ff 	vpconflictq -0x408\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 f5    	vplzcntd %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 4f 44 f5    	vplzcntd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7d cf 44 f5    	vplzcntd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 31    	vplzcntd \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 b4 f4 c0 1d fe ff 	vplzcntd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 30    	vplzcntd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 72 7f 	vplzcntd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 b2 00 20 00 00 	vplzcntd 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 72 80 	vplzcntd -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 48 44 b2 c0 df ff ff 	vplzcntd -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 72 7f 	vplzcntd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 b2 00 02 00 00 	vplzcntd 0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 72 80 	vplzcntd -0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7d 58 44 b2 fc fd ff ff 	vplzcntd -0x204\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 f5    	vplzcntq %zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 4f 44 f5    	vplzcntq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 fd cf 44 f5    	vplzcntq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 31    	vplzcntq \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 b4 f4 c0 1d fe ff 	vplzcntq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 30    	vplzcntq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 72 7f 	vplzcntq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 b2 00 20 00 00 	vplzcntq 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 72 80 	vplzcntq -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 48 44 b2 c0 df ff ff 	vplzcntq -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 72 7f 	vplzcntq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 b2 00 04 00 00 	vplzcntq 0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 72 80 	vplzcntq -0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 fd 58 44 b2 f8 fb ff ff 	vplzcntq -0x408\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:	62 f2 7e 48 3a f6    	vpbroadcastmw2d %k6,%zmm6
[ 	]*[a-f0-9]+:	62 f2 fe 48 2a f6    	vpbroadcastmb2q %k6,%zmm6

0+[a-f0-9]+ <ifma>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 f4[ 	]*vpmadd52luq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f b4 f4[ 	]*vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf b4 f4[ 	]*vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 31[ 	]*vpmadd52luq \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 b4 f4 c0 1d fe ff[ 	]*vpmadd52luq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 30[ 	]*vpmadd52luq \(%eax\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 72 7f[ 	]*vpmadd52luq 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 b2 00 20 00 00[ 	]*vpmadd52luq 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 72 80[ 	]*vpmadd52luq -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 b2 c0 df ff ff[ 	]*vpmadd52luq -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 72 7f[ 	]*vpmadd52luq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 b2 00 04 00 00[ 	]*vpmadd52luq 0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 72 80[ 	]*vpmadd52luq -0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 b2 f8 fb ff ff[ 	]*vpmadd52luq -0x408\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 f4[ 	]*vpmadd52huq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f b5 f4[ 	]*vpmadd52huq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf b5 f4[ 	]*vpmadd52huq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 31[ 	]*vpmadd52huq \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 b4 f4 c0 1d fe ff[ 	]*vpmadd52huq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 30[ 	]*vpmadd52huq \(%eax\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 72 7f[ 	]*vpmadd52huq 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 b2 00 20 00 00[ 	]*vpmadd52huq 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 72 80[ 	]*vpmadd52huq -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 b2 c0 df ff ff[ 	]*vpmadd52huq -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 72 7f[ 	]*vpmadd52huq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 b2 00 04 00 00[ 	]*vpmadd52huq 0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 72 80[ 	]*vpmadd52huq -0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 b2 f8 fb ff ff[ 	]*vpmadd52huq -0x408\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 f4[ 	]*vpmadd52luq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f b4 f4[ 	]*vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf b4 f4[ 	]*vpmadd52luq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 31[ 	]*vpmadd52luq \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 b4 f4 c0 1d fe ff[ 	]*vpmadd52luq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 30[ 	]*vpmadd52luq \(%eax\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 72 7f[ 	]*vpmadd52luq 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 b2 00 20 00 00[ 	]*vpmadd52luq 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 72 80[ 	]*vpmadd52luq -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b4 b2 c0 df ff ff[ 	]*vpmadd52luq -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 72 7f[ 	]*vpmadd52luq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 b2 00 04 00 00[ 	]*vpmadd52luq 0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 72 80[ 	]*vpmadd52luq -0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b4 b2 f8 fb ff ff[ 	]*vpmadd52luq -0x408\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 f4[ 	]*vpmadd52huq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f b5 f4[ 	]*vpmadd52huq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf b5 f4[ 	]*vpmadd52huq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 31[ 	]*vpmadd52huq \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 b4 f4 c0 1d fe ff[ 	]*vpmadd52huq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 30[ 	]*vpmadd52huq \(%eax\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 72 7f[ 	]*vpmadd52huq 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 b2 00 20 00 00[ 	]*vpmadd52huq 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 72 80[ 	]*vpmadd52huq -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 b5 b2 c0 df ff ff[ 	]*vpmadd52huq -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 72 7f[ 	]*vpmadd52huq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 b2 00 04 00 00[ 	]*vpmadd52huq 0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 72 80[ 	]*vpmadd52huq -0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 b5 b2 f8 fb ff ff[ 	]*vpmadd52huq -0x408\(%edx\)\{1to8\},%zmm5,%zmm6

0+[a-f0-9]+ <vbmi>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d f4[ 	]*vpermb %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 8d f4[ 	]*vpermb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 8d f4[ 	]*vpermb %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d 31[ 	]*vpermb \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d b4 f4 c0 1d fe ff[ 	]*vpermb -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d 72 7f[ 	]*vpermb 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d b2 00 20 00 00[ 	]*vpermb 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d 72 80[ 	]*vpermb -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d b2 c0 df ff ff[ 	]*vpermb -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 f4[ 	]*vpermi2b %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 75 f4[ 	]*vpermi2b %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 75 f4[ 	]*vpermi2b %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 31[ 	]*vpermi2b \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 b4 f4 c0 1d fe ff[ 	]*vpermi2b -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 72 7f[ 	]*vpermi2b 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 b2 00 20 00 00[ 	]*vpermi2b 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 72 80[ 	]*vpermi2b -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 b2 c0 df ff ff[ 	]*vpermi2b -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d f4[ 	]*vpermt2b %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 7d f4[ 	]*vpermt2b %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 7d f4[ 	]*vpermt2b %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d 31[ 	]*vpermt2b \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d b4 f4 c0 1d fe ff[ 	]*vpermt2b -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d 72 7f[ 	]*vpermt2b 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d b2 00 20 00 00[ 	]*vpermt2b 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d 72 80[ 	]*vpermt2b -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d b2 c0 df ff ff[ 	]*vpermt2b -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 f4[ 	]*vpmultishiftqb %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 83 f4[ 	]*vpmultishiftqb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 83 f4[ 	]*vpmultishiftqb %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 31[ 	]*vpmultishiftqb \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 b4 f4 c0 1d fe ff[ 	]*vpmultishiftqb -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 30[ 	]*vpmultishiftqb \(%eax\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 72 7f[ 	]*vpmultishiftqb 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 b2 00 20 00 00[ 	]*vpmultishiftqb 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 72 80[ 	]*vpmultishiftqb -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 b2 c0 df ff ff[ 	]*vpmultishiftqb -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 72 7f[ 	]*vpmultishiftqb 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 b2 00 04 00 00[ 	]*vpmultishiftqb 0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 72 80[ 	]*vpmultishiftqb -0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 b2 f8 fb ff ff[ 	]*vpmultishiftqb -0x408\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d f4[ 	]*vpermb %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 8d f4[ 	]*vpermb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 8d f4[ 	]*vpermb %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d 31[ 	]*vpermb \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d b4 f4 c0 1d fe ff[ 	]*vpermb -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d 72 7f[ 	]*vpermb 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d b2 00 20 00 00[ 	]*vpermb 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d 72 80[ 	]*vpermb -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 8d b2 c0 df ff ff[ 	]*vpermb -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 f4[ 	]*vpermi2b %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 75 f4[ 	]*vpermi2b %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 75 f4[ 	]*vpermi2b %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 31[ 	]*vpermi2b \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 b4 f4 c0 1d fe ff[ 	]*vpermi2b -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 72 7f[ 	]*vpermi2b 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 b2 00 20 00 00[ 	]*vpermi2b 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 72 80[ 	]*vpermi2b -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 75 b2 c0 df ff ff[ 	]*vpermi2b -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d f4[ 	]*vpermt2b %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 7d f4[ 	]*vpermt2b %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 7d f4[ 	]*vpermt2b %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d 31[ 	]*vpermt2b \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d b4 f4 c0 1d fe ff[ 	]*vpermt2b -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d 72 7f[ 	]*vpermt2b 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d b2 00 20 00 00[ 	]*vpermt2b 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d 72 80[ 	]*vpermt2b -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 7d b2 c0 df ff ff[ 	]*vpermt2b -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 f4[ 	]*vpmultishiftqb %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 83 f4[ 	]*vpmultishiftqb %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 83 f4[ 	]*vpmultishiftqb %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 31[ 	]*vpmultishiftqb \(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 b4 f4 c0 1d fe ff[ 	]*vpmultishiftqb -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 30[ 	]*vpmultishiftqb \(%eax\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 72 7f[ 	]*vpmultishiftqb 0x1fc0\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 b2 00 20 00 00[ 	]*vpmultishiftqb 0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 72 80[ 	]*vpmultishiftqb -0x2000\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 83 b2 c0 df ff ff[ 	]*vpmultishiftqb -0x2040\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 72 7f[ 	]*vpmultishiftqb 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 b2 00 04 00 00[ 	]*vpmultishiftqb 0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 72 80[ 	]*vpmultishiftqb -0x400\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 83 b2 f8 fb ff ff[ 	]*vpmultishiftqb -0x408\(%edx\)\{1to8\},%zmm5,%zmm6

0+[a-f0-9]+ <vbmi2>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 63 31[ 	]*vpcompressb %zmm6,\(%ecx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 63 b4 f4 c0 1d fe ff[ 	]*vpcompressb %zmm6,-0x1e240\(%esp,%esi,8\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 63 72 7e[ 	]*vpcompressb %zmm6,0x7e\(%edx\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 63 ee[ 	]*vpcompressb %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 63 ee[ 	]*vpcompressb %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 63 ee[ 	]*vpcompressb %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 63 31[ 	]*vpcompressw %zmm6,\(%ecx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 63 b4 f4 c0 1d fe ff[ 	]*vpcompressw %zmm6,-0x1e240\(%esp,%esi,8\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 63 72 40[ 	]*vpcompressw %zmm6,0x80\(%edx\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 63 ee[ 	]*vpcompressw %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 63 ee[ 	]*vpcompressw %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 63 ee[ 	]*vpcompressw %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 62 31[ 	]*vpexpandb \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 62 31[ 	]*vpexpandb \(%ecx\),%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 62 b4 f4 c0 1d fe ff[ 	]*vpexpandb -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 62 72 7e[ 	]*vpexpandb 0x7e\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 62 f5[ 	]*vpexpandb %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 62 f5[ 	]*vpexpandb %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 62 f5[ 	]*vpexpandb %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 62 31[ 	]*vpexpandw \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 62 31[ 	]*vpexpandw \(%ecx\),%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 62 b4 f4 c0 1d fe ff[ 	]*vpexpandw -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 62 72 40[ 	]*vpexpandw 0x80\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 62 f5[ 	]*vpexpandw %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 62 f5[ 	]*vpexpandw %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 62 f5[ 	]*vpexpandw %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 70 f4[ 	]*vpshldvw %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 70 f4[ 	]*vpshldvw %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 70 f4[ 	]*vpshldvw %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 70 b4 f4 c0 1d fe ff[ 	]*vpshldvw -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 70 72 02[ 	]*vpshldvw 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 71 f4[ 	]*vpshldvd %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 71 f4[ 	]*vpshldvd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 71 f4[ 	]*vpshldvd %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 71 b4 f4 c0 1d fe ff[ 	]*vpshldvd -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 71 72 02[ 	]*vpshldvd 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 58 71 72 7f[ 	]*vpshldvd 0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 71 f4[ 	]*vpshldvq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 71 f4[ 	]*vpshldvq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 71 f4[ 	]*vpshldvq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 71 b4 f4 c0 1d fe ff[ 	]*vpshldvq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 71 72 02[ 	]*vpshldvq 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 71 72 7f[ 	]*vpshldvq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 72 f4[ 	]*vpshrdvw %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 72 f4[ 	]*vpshrdvw %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 72 f4[ 	]*vpshrdvw %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 72 b4 f4 c0 1d fe ff[ 	]*vpshrdvw -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 72 72 02[ 	]*vpshrdvw 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 73 f4[ 	]*vpshrdvd %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 73 f4[ 	]*vpshrdvd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 73 f4[ 	]*vpshrdvd %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvd -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 73 72 02[ 	]*vpshrdvd 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 58 73 72 7f[ 	]*vpshrdvd 0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 73 f4[ 	]*vpshrdvq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 73 f4[ 	]*vpshrdvq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 73 f4[ 	]*vpshrdvq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 73 72 02[ 	]*vpshrdvq 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 73 72 7f[ 	]*vpshrdvq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 70 f4 ab[ 	]*vpshldw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 70 f4 ab[ 	]*vpshldw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 70 f4 7b[ 	]*vpshldw \$0x7b,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 70 b4 f4 c0 1d fe ff 7b[ 	]*vpshldw \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 70 72 02 7b[ 	]*vpshldw \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 4f 71 f4 ab[ 	]*vpshldd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 cf 71 f4 ab[ 	]*vpshldd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 71 f4 7b[ 	]*vpshldd \$0x7b,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldd \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 71 72 02 7b[ 	]*vpshldd \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 58 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 71 f4 ab[ 	]*vpshldq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 71 f4 ab[ 	]*vpshldq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldq \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 71 72 02 7b[ 	]*vpshldq \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 58 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 72 f4 ab[ 	]*vpshrdw \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 72 f4 ab[ 	]*vpshrdw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 72 f4 ab[ 	]*vpshrdw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 72 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdw \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 72 72 02 7b[ 	]*vpshrdw \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 73 f4 ab[ 	]*vpshrdd \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 4f 73 f4 ab[ 	]*vpshrdd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 cf 73 f4 ab[ 	]*vpshrdd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdd \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 73 72 02 7b[ 	]*vpshrdd \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 58 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 73 f4 ab[ 	]*vpshrdq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 73 f4 ab[ 	]*vpshrdq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 73 f4 7b[ 	]*vpshrdq \$0x7b,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdq \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 73 72 02 7b[ 	]*vpshrdq \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 58 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 63 31[ 	]*vpcompressb %zmm6,\(%ecx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 63 b4 f4 c0 1d fe ff[ 	]*vpcompressb %zmm6,-0x1e240\(%esp,%esi,8\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 63 72 7e[ 	]*vpcompressb %zmm6,0x7e\(%edx\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 63 ee[ 	]*vpcompressb %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 63 ee[ 	]*vpcompressb %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 63 ee[ 	]*vpcompressb %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 63 31[ 	]*vpcompressw %zmm6,\(%ecx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 63 b4 f4 c0 1d fe ff[ 	]*vpcompressw %zmm6,-0x1e240\(%esp,%esi,8\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 63 72 40[ 	]*vpcompressw %zmm6,0x80\(%edx\)
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 63 ee[ 	]*vpcompressw %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 63 ee[ 	]*vpcompressw %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 63 ee[ 	]*vpcompressw %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 62 31[ 	]*vpexpandb \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 62 31[ 	]*vpexpandb \(%ecx\),%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 62 b4 f4 c0 1d fe ff[ 	]*vpexpandb -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 62 72 7e[ 	]*vpexpandb 0x7e\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 62 f5[ 	]*vpexpandb %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 62 f5[ 	]*vpexpandb %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 62 f5[ 	]*vpexpandb %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 62 31[ 	]*vpexpandw \(%ecx\),%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 62 31[ 	]*vpexpandw \(%ecx\),%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 62 b4 f4 c0 1d fe ff[ 	]*vpexpandw -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 62 72 40[ 	]*vpexpandw 0x80\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 62 f5[ 	]*vpexpandw %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 62 f5[ 	]*vpexpandw %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 62 f5[ 	]*vpexpandw %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 70 f4[ 	]*vpshldvw %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 70 f4[ 	]*vpshldvw %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 70 f4[ 	]*vpshldvw %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 70 b4 f4 c0 1d fe ff[ 	]*vpshldvw -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 70 72 02[ 	]*vpshldvw 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 71 f4[ 	]*vpshldvd %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 71 f4[ 	]*vpshldvd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 71 f4[ 	]*vpshldvd %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 71 b4 f4 c0 1d fe ff[ 	]*vpshldvd -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 71 72 02[ 	]*vpshldvd 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 58 71 72 7f[ 	]*vpshldvd 0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 71 f4[ 	]*vpshldvq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 71 f4[ 	]*vpshldvq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 71 f4[ 	]*vpshldvq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 71 b4 f4 c0 1d fe ff[ 	]*vpshldvq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 71 72 02[ 	]*vpshldvq 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 71 72 7f[ 	]*vpshldvq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 72 f4[ 	]*vpshrdvw %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 72 f4[ 	]*vpshrdvw %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 72 f4[ 	]*vpshrdvw %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 72 b4 f4 c0 1d fe ff[ 	]*vpshrdvw -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 72 72 02[ 	]*vpshrdvw 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 73 f4[ 	]*vpshrdvd %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4f 73 f4[ 	]*vpshrdvd %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 cf 73 f4[ 	]*vpshrdvd %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvd -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 73 72 02[ 	]*vpshrdvd 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 58 73 72 7f[ 	]*vpshrdvd 0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 73 f4[ 	]*vpshrdvq %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 4f 73 f4[ 	]*vpshrdvq %zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 cf 73 f4[ 	]*vpshrdvq %zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 73 b4 f4 c0 1d fe ff[ 	]*vpshrdvq -0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 48 73 72 02[ 	]*vpshrdvq 0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 d5 58 73 72 7f[ 	]*vpshrdvq 0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 70 f4 ab[ 	]*vpshldw \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 70 f4 ab[ 	]*vpshldw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 70 f4 ab[ 	]*vpshldw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 70 b4 f4 c0 1d fe ff 7b[ 	]*vpshldw \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 70 72 02 7b[ 	]*vpshldw \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 71 f4 ab[ 	]*vpshldd \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 4f 71 f4 ab[ 	]*vpshldd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 cf 71 f4 ab[ 	]*vpshldd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldd \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 71 72 02 7b[ 	]*vpshldd \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 58 71 72 7f 7b[ 	]*vpshldd \$0x7b,0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 71 f4 ab[ 	]*vpshldq \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 71 f4 ab[ 	]*vpshldq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 71 f4 ab[ 	]*vpshldq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 71 b4 f4 c0 1d fe ff 7b[ 	]*vpshldq \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 71 72 02 7b[ 	]*vpshldq \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 58 71 72 7f 7b[ 	]*vpshldq \$0x7b,0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 72 f4 ab[ 	]*vpshrdw \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 72 f4 ab[ 	]*vpshrdw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 72 f4 ab[ 	]*vpshrdw \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 72 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdw \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 72 72 02 7b[ 	]*vpshrdw \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 73 f4 ab[ 	]*vpshrdd \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 4f 73 f4 ab[ 	]*vpshrdd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 cf 73 f4 ab[ 	]*vpshrdd \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdd \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 48 73 72 02 7b[ 	]*vpshrdd \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 55 58 73 72 7f 7b[ 	]*vpshrdd \$0x7b,0x1fc\(%edx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 73 f4 ab[ 	]*vpshrdq \$0xab,%zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 4f 73 f4 ab[ 	]*vpshrdq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 cf 73 f4 ab[ 	]*vpshrdq \$0xab,%zmm4,%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 73 b4 f4 c0 1d fe ff 7b[ 	]*vpshrdq \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 48 73 72 02 7b[ 	]*vpshrdq \$0x7b,0x80\(%edx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 58 73 72 7f 7b[ 	]*vpshrdq \$0x7b,0x3f8\(%edx\)\{1to8\},%zmm5,%zmm6

0+[a-f0-9]+ <vnni>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 52 e3[ 	]*vpdpwssd %zmm3,%zmm1,%zmm4
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 49 52 e3[ 	]*vpdpwssd %zmm3,%zmm1,%zmm4\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 c9 52 e3[ 	]*vpdpwssd %zmm3,%zmm1,%zmm4\{%k1\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 52 a4 f4 c0 1d fe ff[ 	]*vpdpwssd -0x1e240\(%esp,%esi,8\),%zmm1,%zmm4
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 52 62 7f[ 	]*vpdpwssd 0x1fc0\(%edx\),%zmm1,%zmm4
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 58 52 62 7f[ 	]*vpdpwssd 0x1fc\(%edx\)\{1to16\},%zmm1,%zmm4
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 53 d4[ 	]*vpdpwssds %zmm4,%zmm5,%zmm2
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 4e 53 d4[ 	]*vpdpwssds %zmm4,%zmm5,%zmm2\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 ce 53 d4[ 	]*vpdpwssds %zmm4,%zmm5,%zmm2\{%k6\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 53 94 f4 c0 1d fe ff[ 	]*vpdpwssds -0x1e240\(%esp,%esi,8\),%zmm5,%zmm2
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 48 53 52 7f[ 	]*vpdpwssds 0x1fc0\(%edx\),%zmm5,%zmm2
[ 	]*[a-f0-9]+:[ 	]*62 f2 55 58 53 52 7f[ 	]*vpdpwssds 0x1fc\(%edx\)\{1to16\},%zmm5,%zmm2
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 48 50 eb[ 	]*vpdpbusd %zmm3,%zmm2,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 49 50 eb[ 	]*vpdpbusd %zmm3,%zmm2,%zmm5\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d c9 50 eb[ 	]*vpdpbusd %zmm3,%zmm2,%zmm5\{%k1\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 48 50 ac f4 c0 1d fe ff[ 	]*vpdpbusd -0x1e240\(%esp,%esi,8\),%zmm2,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 48 50 6a 7f[ 	]*vpdpbusd 0x1fc0\(%edx\),%zmm2,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 6d 58 50 6a 7f[ 	]*vpdpbusd 0x1fc\(%edx\)\{1to16\},%zmm2,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 48 51 e9[ 	]*vpdpbusds %zmm1,%zmm3,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 4a 51 e9[ 	]*vpdpbusds %zmm1,%zmm3,%zmm5\{%k2\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 ca 51 e9[ 	]*vpdpbusds %zmm1,%zmm3,%zmm5\{%k2\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 48 51 ac f4 c0 1d fe ff[ 	]*vpdpbusds -0x1e240\(%esp,%esi,8\),%zmm3,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 48 51 6a 7f[ 	]*vpdpbusds 0x1fc0\(%edx\),%zmm3,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 65 58 51 6a 7f[ 	]*vpdpbusds 0x1fc\(%edx\)\{1to16\},%zmm3,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 48 52 d9[ 	]*vpdpwssd %zmm1,%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 4b 52 d9[ 	]*vpdpwssd %zmm1,%zmm4,%zmm3\{%k3\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d cb 52 d9[ 	]*vpdpwssd %zmm1,%zmm4,%zmm3\{%k3\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 48 52 9c f4 c0 1d fe ff[ 	]*vpdpwssd -0x1e240\(%esp,%esi,8\),%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 48 52 5a 7f[ 	]*vpdpwssd 0x1fc0\(%edx\),%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 58 52 5a 7f[ 	]*vpdpwssd 0x1fc\(%edx\)\{1to16\},%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 53 da[ 	]*vpdpwssds %zmm2,%zmm1,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 4f 53 da[ 	]*vpdpwssds %zmm2,%zmm1,%zmm3\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 cf 53 da[ 	]*vpdpwssds %zmm2,%zmm1,%zmm3\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 53 9c f4 c0 1d fe ff[ 	]*vpdpwssds -0x1e240\(%esp,%esi,8\),%zmm1,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 53 5a 7f[ 	]*vpdpwssds 0x1fc0\(%edx\),%zmm1,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 58 53 5a 7f[ 	]*vpdpwssds 0x1fc\(%edx\)\{1to16\},%zmm1,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 48 50 d9[ 	]*vpdpbusd %zmm1,%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 4e 50 d9[ 	]*vpdpbusd %zmm1,%zmm4,%zmm3\{%k6\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d ce 50 d9[ 	]*vpdpbusd %zmm1,%zmm4,%zmm3\{%k6\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 48 50 9c f4 c0 1d fe ff[ 	]*vpdpbusd -0x1e240\(%esp,%esi,8\),%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 48 50 5a 7f[ 	]*vpdpbusd 0x1fc0\(%edx\),%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 5d 58 50 5a 7f[ 	]*vpdpbusd 0x1fc\(%edx\)\{1to16\},%zmm4,%zmm3
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 51 c9[ 	]*vpdpbusds %zmm1,%zmm1,%zmm1
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 49 51 c9[ 	]*vpdpbusds %zmm1,%zmm1,%zmm1\{%k1\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 c9 51 c9[ 	]*vpdpbusds %zmm1,%zmm1,%zmm1\{%k1\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 51 8c f4 c0 1d fe ff[ 	]*vpdpbusds -0x1e240\(%esp,%esi,8\),%zmm1,%zmm1
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 48 51 4a 7f[ 	]*vpdpbusds 0x1fc0\(%edx\),%zmm1,%zmm1
[ 	]*[a-f0-9]+:[ 	]*62 f2 75 58 51 4a 7f[ 	]*vpdpbusds 0x1fc\(%edx\)\{1to16\},%zmm1,%zmm1

0+[a-f0-9]+ <bf16>:
[ 	]*[a-f0-9]+:	62 f2 57 48 72 f4    	vcvtne2ps2bf16 %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 57 4f 72 b4 f4 00 00 00 10 	vcvtne2ps2bf16 0x10000000\(%esp,%esi,8\),%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 57 58 72 31    	vcvtne2ps2bf16 \(%ecx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 57 48 72 71 7f 	vcvtne2ps2bf16 0x1fc0\(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 57 df 72 b2 00 e0 ff ff 	vcvtne2ps2bf16 -0x2000\(%edx\)\{1to16\},%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7e 48 72 f5    	vcvtneps2bf16 %zmm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 7e 4f 72 b4 f4 00 00 00 10 	vcvtneps2bf16 0x10000000\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7e 58 72 31    	vcvtneps2bf16 \(%ecx\)\{1to16\},%ymm6
[ 	]*[a-f0-9]+:	62 f2 7e 48 72 71 7f 	vcvtneps2bf16 0x1fc0\(%ecx\),%ymm6
[ 	]*[a-f0-9]+:	62 f2 7e df 72 b2 00 e0 ff ff 	vcvtneps2bf16 -0x2000\(%edx\)\{1to16\},%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 56 48 52 f4    	vdpbf16ps %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 56 4f 52 b4 f4 00 00 00 10 	vdpbf16ps 0x10000000\(%esp,%esi,8\),%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 56 58 52 31    	vdpbf16ps \(%ecx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 56 48 52 71 7f 	vdpbf16ps 0x1fc0\(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 56 df 52 b2 00 e0 ff ff 	vdpbf16ps -0x2000\(%edx\)\{1to16\},%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 57 48 72 f4    	vcvtne2ps2bf16 %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 57 4f 72 b4 f4 00 00 00 10 	vcvtne2ps2bf16 0x10000000\(%esp,%esi,8\),%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 57 58 72 31    	vcvtne2ps2bf16 \(%ecx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 57 48 72 71 7f 	vcvtne2ps2bf16 0x1fc0\(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 57 df 72 b2 00 e0 ff ff 	vcvtne2ps2bf16 -0x2000\(%edx\)\{1to16\},%zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 7e 48 72 f5    	vcvtneps2bf16 %zmm5,%ymm6
[ 	]*[a-f0-9]+:	62 f2 7e 4f 72 b4 f4 00 00 00 10 	vcvtneps2bf16 0x10000000\(%esp,%esi,8\),%ymm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 7e 58 72 31    	vcvtneps2bf16 \(%ecx\)\{1to16\},%ymm6
[ 	]*[a-f0-9]+:	62 f2 7e 48 72 71 7f 	vcvtneps2bf16 0x1fc0\(%ecx\),%ymm6
[ 	]*[a-f0-9]+:	62 f2 7e df 72 b2 00 e0 ff ff 	vcvtneps2bf16 -0x2000\(%edx\)\{1to16\},%ymm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:	62 f2 56 48 52 f4    	vdpbf16ps %zmm4,%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 56 4f 52 b4 f4 00 00 00 10 	vdpbf16ps 0x10000000\(%esp,%esi,8\),%zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:	62 f2 56 58 52 31    	vdpbf16ps \(%ecx\)\{1to16\},%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 56 48 52 71 7f 	vdpbf16ps 0x1fc0\(%ecx\),%zmm5,%zmm6
[ 	]*[a-f0-9]+:	62 f2 56 df 52 b2 00 e0 ff ff 	vdpbf16ps -0x2000\(%edx\)\{1to16\},%zmm5,%zmm6\{%k7\}\{z\}

0+[a-f0-9]+ <vpopcnt>:
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 f5[ 	]*vpopcntd %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 31[ 	]*vpopcntd \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 30[ 	]*vpopcntd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 72 7f[ 	]*vpopcntd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b2 00 20 00 00[ 	]*vpopcntd 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 72 80[ 	]*vpopcntd -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b2 c0 df ff ff[ 	]*vpopcntd -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 b2 00 02 00 00[ 	]*vpopcntd 0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 72 80[ 	]*vpopcntd -0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 b2 fc fd ff ff[ 	]*vpopcntd -0x204\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 f5[ 	]*vpopcntq %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 31[ 	]*vpopcntq \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 30[ 	]*vpopcntq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 72 7f[ 	]*vpopcntq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b2 00 20 00 00[ 	]*vpopcntq 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 72 80[ 	]*vpopcntq -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b2 c0 df ff ff[ 	]*vpopcntq -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 b2 00 04 00 00[ 	]*vpopcntq 0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 72 80[ 	]*vpopcntq -0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 b2 f8 fb ff ff[ 	]*vpopcntq -0x408\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 f5[ 	]*vpopcntd %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 4f 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d cf 55 f5[ 	]*vpopcntd %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 31[ 	]*vpopcntd \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntd -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 30[ 	]*vpopcntd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 30[ 	]*vpopcntd \(%eax\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 72 7f[ 	]*vpopcntd 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b2 00 20 00 00[ 	]*vpopcntd 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 72 80[ 	]*vpopcntd -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 48 55 b2 c0 df ff ff[ 	]*vpopcntd -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 72 7f[ 	]*vpopcntd 0x1fc\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 b2 00 02 00 00[ 	]*vpopcntd 0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 72 80[ 	]*vpopcntd -0x200\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 7d 58 55 b2 fc fd ff ff[ 	]*vpopcntd -0x204\(%edx\)\{1to16\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 f5[ 	]*vpopcntq %zmm5,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 4f 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd cf 55 f5[ 	]*vpopcntq %zmm5,%zmm6\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 31[ 	]*vpopcntq \(%ecx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b4 f4 c0 1d fe ff[ 	]*vpopcntq -0x1e240\(%esp,%esi,8\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 30[ 	]*vpopcntq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 30[ 	]*vpopcntq \(%eax\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 72 7f[ 	]*vpopcntq 0x1fc0\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b2 00 20 00 00[ 	]*vpopcntq 0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 72 80[ 	]*vpopcntq -0x2000\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 48 55 b2 c0 df ff ff[ 	]*vpopcntq -0x2040\(%edx\),%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 72 7f[ 	]*vpopcntq 0x3f8\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 b2 00 04 00 00[ 	]*vpopcntq 0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 72 80[ 	]*vpopcntq -0x400\(%edx\)\{1to8\},%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f2 fd 58 55 b2 f8 fb ff ff[ 	]*vpopcntq -0x408\(%edx\)\{1to8\},%zmm6
#pass
