.*: Assembler messages:
.*:5: Error: .*
.*:6: Error: .*
.*:7: Error: .*
.*:8: Error: .*
.*:10: Error: .*
.*:11: Error: .*
.*:12: Error: .*
.*:13: Error: .*
.*:15: Error: .*
.*:16: Error: .*
.*:17: Error: .*
.*:18: Error: .*
.*:20: Error: .*
.*:21: Error: .*
.*:22: Error: .*
.*:23: Error: .*
.*:25: Error: .*
.*:26: Error: .*
.*:27: Error: .*
.*:28: Error: .*
.*:30: Error: .*
.*:31: Error: .*
.*:32: Error: .*
.*:33: Error: .*
.*:35: Error: .*
.*:36: Error: .*
.*:37: Error: .*
.*:38: Error: .*
.*:40: Error: .*
.*:41: Error: .*
.*:42: Error: .*
.*:43: Error: .*
.*:45: Error: .*
.*:46: Error: .*
.*:47: Error: .*
.*:48: Error: .*
.*:50: Error: .*
.*:51: Error: .*
.*:52: Error: .*
.*:53: Error: .*
.*:55: Error: .*
.*:56: Error: .*
.*:57: Error: .*
.*:58: Error: .*
.*:60: Error: .*
.*:61: Error: .*
.*:62: Error: .*
.*:63: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\# Check 64bit instructions with optimized encoding
[ 	]*2[ 	]+
[ 	]*3[ 	]+\.include "x86-64-optimize-7\.s"
[ 	]*1[ 	]+\# Check 64bit instructions with optimized encoding
[ 	]*2[ 	]+
[ 	]*3[ 	]+\.text
[ 	]*4[ 	]+_start:
[ 	]*5[ 	]+vandnpd %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*6[ 	]+vandnpd %zmm1, %zmm1, %zmm15
[ 	]*7[ 	]+vandnpd %zmm1, %zmm1, %zmm16
[ 	]*8[ 	]+vandnpd %zmm17, %zmm17, %zmm1
[ 	]*9[ 	]+
[ 	]*10[ 	]+vandnps %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*11[ 	]+vandnps %zmm1, %zmm1, %zmm15
[ 	]*12[ 	]+vandnps %zmm1, %zmm1, %zmm16
[ 	]*13[ 	]+vandnps %zmm17, %zmm17, %zmm1
[ 	]*14[ 	]+
[ 	]*15[ 	]+vpandnd %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*16[ 	]+vpandnd %zmm1, %zmm1, %zmm15
[ 	]*17[ 	]+vpandnd %zmm1, %zmm1, %zmm16
[ 	]*18[ 	]+vpandnd %zmm17, %zmm17, %zmm1
[ 	]*19[ 	]+
[ 	]*20[ 	]+vpandnq %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*21[ 	]+vpandnq %zmm1, %zmm1, %zmm15
[ 	]*22[ 	]+vpandnq %zmm1, %zmm1, %zmm16
[ 	]*23[ 	]+vpandnq %zmm17, %zmm17, %zmm1
[ 	]*24[ 	]+
[ 	]*25[ 	]+vxorpd %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*26[ 	]+vxorpd %zmm1, %zmm1, %zmm15
[ 	]*27[ 	]+vxorpd %zmm1, %zmm1, %zmm16
[ 	]*28[ 	]+vxorpd %zmm17, %zmm17, %zmm1
[ 	]*29[ 	]+
[ 	]*30[ 	]+vxorps %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*31[ 	]+vxorps %zmm1, %zmm1, %zmm15
[ 	]*32[ 	]+vxorps %zmm1, %zmm1, %zmm16
[ 	]*33[ 	]+vxorps %zmm17, %zmm17, %zmm1
[ 	]*34[ 	]+
[ 	]*35[ 	]+vpxord %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*36[ 	]+vpxord %zmm1, %zmm1, %zmm15
[ 	]*37[ 	]+vpxord %zmm1, %zmm1, %zmm16
[ 	]*38[ 	]+vpxord %zmm17, %zmm17, %zmm1
[ 	]*39[ 	]+
[ 	]*40[ 	]+vpxorq %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*41[ 	]+vpxorq %zmm1, %zmm1, %zmm15
[ 	]*42[ 	]+vpxorq %zmm1, %zmm1, %zmm16
[ 	]*43[ 	]+vpxorq %zmm17, %zmm17, %zmm1
[ 	]*44[ 	]+
[ 	]*45[ 	]+vpsubb %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*46[ 	]+vpsubb %zmm1, %zmm1, %zmm15
[ 	]*47[ 	]+vpsubb %zmm1, %zmm1, %zmm16
[ 	]*48[ 	]+vpsubb %zmm17, %zmm17, %zmm1
[ 	]*49[ 	]+
[ 	]*50[ 	]+vpsubw %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*51[ 	]+vpsubw %zmm1, %zmm1, %zmm15
[ 	]*52[ 	]+vpsubw %zmm1, %zmm1, %zmm16
[ 	]*53[ 	]+vpsubw %zmm17, %zmm17, %zmm1
[ 	]*54[ 	]+
GAS LISTING .*


[ 	]*55[ 	]+vpsubd %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*56[ 	]+vpsubd %zmm1, %zmm1, %zmm15
[ 	]*57[ 	]+vpsubd %zmm1, %zmm1, %zmm16
[ 	]*58[ 	]+vpsubd %zmm17, %zmm17, %zmm1
[ 	]*59[ 	]+
[ 	]*60[ 	]+vpsubq %zmm1, %zmm1, %zmm15\{%k7\}
[ 	]*61[ 	]+vpsubq %zmm1, %zmm1, %zmm15
[ 	]*62[ 	]+vpsubq %zmm1, %zmm1, %zmm16
[ 	]*63[ 	]+vpsubq %zmm17, %zmm17, %zmm1
#pass
