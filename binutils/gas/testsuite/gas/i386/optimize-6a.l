.*: Assembler messages:
.*:5: Error: .*
.*:6: Error: .*
.*:8: Error: .*
.*:9: Error: .*
.*:11: Error: .*
.*:12: Error: .*
.*:14: Error: .*
.*:15: Error: .*
.*:17: Error: .*
.*:18: Error: .*
.*:20: Error: .*
.*:21: Error: .*
.*:23: Error: .*
.*:24: Error: .*
.*:26: Error: .*
.*:27: Error: .*
.*:29: Error: .*
.*:30: Error: .*
.*:32: Error: .*
.*:33: Error: .*
.*:35: Error: .*
.*:36: Error: .*
.*:38: Error: .*
.*:39: Error: .*
.*:41: Error: .*
.*:42: Error: .*
.*:44: Error: .*
.*:45: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\# Check instructions with optimized encoding
[ 	]*2[ 	]+
[ 	]*3[ 	]+\.include "optimize-6\.s"
[ 	]*1[ 	]+\# Check instructions with optimized encoding
[ 	]*2[ 	]+
[ 	]*3[ 	]+\.text
[ 	]*4[ 	]+_start:
[ 	]*5[ 	]+vandnpd %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*6[ 	]+vandnpd %zmm1, %zmm1, %zmm5
[ 	]*7[ 	]+
[ 	]*8[ 	]+vandnps %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*9[ 	]+vandnps %zmm1, %zmm1, %zmm5
[ 	]*10[ 	]+
[ 	]*11[ 	]+vpandnd %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*12[ 	]+vpandnd %zmm1, %zmm1, %zmm5
[ 	]*13[ 	]+
[ 	]*14[ 	]+vpandnq %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*15[ 	]+vpandnq %zmm1, %zmm1, %zmm5
[ 	]*16[ 	]+
[ 	]*17[ 	]+vxorpd %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*18[ 	]+vxorpd %zmm1, %zmm1, %zmm5
[ 	]*19[ 	]+
[ 	]*20[ 	]+vxorps %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*21[ 	]+vxorps %zmm1, %zmm1, %zmm5
[ 	]*22[ 	]+
[ 	]*23[ 	]+vpxord %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*24[ 	]+vpxord %zmm1, %zmm1, %zmm5
[ 	]*25[ 	]+
[ 	]*26[ 	]+vpxorq %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*27[ 	]+vpxorq %zmm1, %zmm1, %zmm5
[ 	]*28[ 	]+
[ 	]*29[ 	]+vpsubb %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*30[ 	]+vpsubb %zmm1, %zmm1, %zmm5
[ 	]*31[ 	]+
[ 	]*32[ 	]+vpsubw %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*33[ 	]+vpsubw %zmm1, %zmm1, %zmm5
[ 	]*34[ 	]+
[ 	]*35[ 	]+vpsubd %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*36[ 	]+vpsubd %zmm1, %zmm1, %zmm5
[ 	]*37[ 	]+
[ 	]*38[ 	]+vpsubq %zmm1, %zmm1, %zmm5\{%k7\}
[ 	]*39[ 	]+vpsubq %zmm1, %zmm1, %zmm5
[ 	]*40[ 	]+
[ 	]*41[ 	]+kxord %k1, %k1, %k5
[ 	]*42[ 	]+kxorq %k1, %k1, %k5
[ 	]*43[ 	]+
[ 	]*44[ 	]+kandnd %k1, %k1, %k5
[ 	]*45[ 	]+kandnq %k1, %k1, %k5
#pass
