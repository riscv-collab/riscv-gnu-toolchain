# Blackfin testcase for the accumulator and compares
# mach: bfin

	.include "testutils.inc"

	start

r7=0;
astat=r7;
r7.l=0x80;
A1.x=r7.l;
r0 = 0;
A1.w=r0;
r1.l = 0xffff;
r1.h =0xffff;
A0.w=r1;
r7.l=0x7f;
A0.x=r7.l;
#dbg A0;
#dbg A1;
#dbg astat;
cc = A0==A1;
#dbg astat;
r7=astat;
dbga (r7.h, 0x0);
dbga (r7.l, 0x0);
astat=r0;
#dbg astat;
r7.l=0x80;
A0.x=r7.l;
r0 = 0;
A0.w=r0;
r1.l = 0xffff;
r1.h =0xffff;
A1.w=r1;
r7.l=0x7f;
A1.x=r7.l;
cc = A0<A1;
#dbg astat;
r7=astat;
dbga (r7.h, 0x0);
dbga (r7.l, 0x1026);
astat=r0;
cc = A0<=A1;
#dbg astat;
r7=astat;
dbga (r7.h, 0x0);
dbga (r7.l, 0x1026);

pass
