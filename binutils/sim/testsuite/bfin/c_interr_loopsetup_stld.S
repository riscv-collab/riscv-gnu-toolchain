//Original:/proj/frio/dv/testcases/core/c_interr_loopsetup_stld/c_interr_loopsetup_stld.dsp
// Spec Reference: interrupt loopsetup_ldst
# mach: bfin

#include "test.h"
.include "testutils.inc"
start

    A0 = 0;         // reset accumulators
    A1 = 0;

P1 = 3;
P2 = 4;

LD32(r0, 0x00200005);
LD32(r1, 0x00300010);
LD32(r2, 0x00500012);
LD32(r3, 0x00600024);
LD32(r4, 0x00700016);
LD32(r5, 0x00900028);
LD32(r6, 0x0a000030);
LD32(r7, 0x00b00044);

loadsym I0, DATA0;
loadsym I1, DATA1;
R0 = [ I0 ++ ];
R1 = [ I1 ++ ];
LSETUP ( start1 , end1 ) LC0 = P1;
start1:  R0 += 1;
         R1 += 2;
         A1 += R0.H * R1.H, A0 += R0.L * R1.L || R0 = [ I0 ++ ] || R1 = [ I1 ++ ]; // dsp32mac dual
      // a1 += h*h, a0 += l*l (r0,r1) ; r0 = [i0++]; r1 = [i1++]; // dsp32mac
         R2 = ( R2 + R5 ) << 1;   // alu2op
DIVQ ( R5 , R3 );
         R1 <<= R5;
         R1 >>>= R1;
         R6 = ~ R0;
         //MY_GEN_INT(10, 1)
DIVQ ( R5 , R2 );
         R0 = R3.B (X);
DIVS ( R7 , R0 );
end1:    R2 += 3;
         R3 = ( A0 += A1 );
CHECKREG(r0, 0x00000024);
CHECKREG(r1, 0x00000000);
CHECKREG(r2, 0x0670098D);
CHECKREG(r3, 0x000015EC);
CHECKREG(r4, 0x00700016);
CHECKREG(r5, 0x0B240A39);
CHECKREG(r6, 0xFFF2FFFC);
CHECKREG(r7, 0x05800220);

A0 = 0;
A1 = 0;
LSETUP ( start2 , end2 ) LC0 = P2;
start2:  R4 += 4;
       //a1 += h*h, a0 += l*l (r0,r1), r0 = [i0--], r1 = [i1--];
         A1 += R0.H * R1.H, A0 += R0.L * R1.L; R0 = [ I0 -- ]; R1 = [ I1 -- ];
         R1 <<= R5;
         R6 = R7.B (Z);
         R2 = - R6;
         R3 = R4.L (Z);
DIVS ( R1 , R1 );
         R6 = - R0;
         R0 >>= R0;
DIVS ( R4 , R7 );
         //MY_GEN_INT(13, 1)
         R1 = R2.L (Z);
end2:    R5 += -5;
         R6 = ( A0 += A1 );
CHECKREG(r0, 0x00000000);
CHECKREG(r1, 0x0000FFE0);
CHECKREG(r2, 0xFFFFFFE0);
CHECKREG(r3, 0x000000EC);
CHECKREG(r4, 0x070001D8);
CHECKREG(r5, 0x0B240A25);
CHECKREG(r6, 0x00000000);
CHECKREG(r7, 0x05800220);
LD32(r0, 0x01200805);
LD32(r1, 0x02300710);
LD32(r2, 0x03500612);
LD32(r3, 0x04600524);
LD32(r4, 0x05700416);
LD32(r5, 0x06900328);
LD32(r6, 0x0a700230);
LD32(r7, 0x08b00044);

loadsym I2, DATA0;
loadsym I3, DATA1;
[ I2 ++ ] = R0;
[ I3 ++ ] = R1;
LSETUP ( start3 , end3 ) LC0 = P1;
start3:
         [ I2 ++ ] = R2;
         [ I3 ++ ] = R3;
         R2 += 1;
end3:
         R3 += 1;

A0 = 0;
A1 = 0;
LSETUP ( start4 , end4 ) LC0 = P2;
R0 = [ I0 -- ];
R1 = [ I1 -- ];
start4:
      // a1 += h*h, a0 += l*l (r0,r1), r0 = [i2--], r1 = [i3--];
         A1 += R0.H * R1.H, A0 += R0.L * R1.L; R0 = [ I2 -- ]; R1 = [ I3 -- ];
         R4 = R4 + R0;   // comp3op
         R5 = R7.L (Z);
         R4 >>>= R5;
         R0 = R7.B (X);
DIVQ ( R6 , R6 );
         //MY_GEN_INT(7, 1)
end4:    R5 = R5 + R1;
         R6 = ( A0 += A1 );
         R7 = ( A0 += A1 );
CHECKREG(r0, 0x00000044);
CHECKREG(r1, 0x04600524);
CHECKREG(r2, 0x03500615);
CHECKREG(r3, 0x04600527);
CHECKREG(r4, 0x00000000);
CHECKREG(r5, 0x04600568);
CHECKREG(r6, 0x007C3498);
CHECKREG(r7, 0x00812098);


pass;            // End the test

//
// Data Segment
//



.data

DATA0:
.dd 0x000a0000
.dd 0x000b0001
.dd 0x000c0002
.dd 0x000d0003
.dd 0x000e0004
.dd 0x000f0005
.dd 0x00100006
.dd 0x00200007
.dd 0x00300008
.dd 0x00400009
.dd 0x0050000a
.dd 0x0060000b
.dd 0x0070000c
.dd 0x0080000d
.dd 0x0090000e
.dd 0x0100000f
.dd 0x02000010
.dd 0x03000011
.dd 0x04000012
.dd 0x05000013
.dd 0x06000014
.dd 0x001a0000
.dd 0x001b0001
.dd 0x001c0002
.dd 0x001d0003
.dd 0x00010004
.dd 0x00010005
.dd 0x02100006
.dd 0x02200007
.dd 0x02300008
.dd 0x02200009
.dd 0x0250000a
.dd 0x0260000b
.dd 0x0270000c
.dd 0x0280000d
.dd 0x0290000e
.dd 0x2100000f
.dd 0x22000010
.dd 0x22000011
.dd 0x24000012
.dd 0x25000013
.dd 0x26000014

DATA1:
.dd 0x00f00100
.dd 0x00e00101
.dd 0x00d00102
.dd 0x00c00103
.dd 0x00b00104
.dd 0x00a00105
.dd 0x00900106
.dd 0x00800107
.dd 0x00100108
.dd 0x00200109
.dd 0x0030010a
.dd 0x0040010b
.dd 0x0050011c
.dd 0x0060010d
.dd 0x0070010e
.dd 0x0080010f
.dd 0x00900110
.dd 0x01000111
.dd 0x02000112
.dd 0x03000113
.dd 0x04000114
.dd 0x05000115
.dd 0x03f00100
.dd 0x03e00101
.dd 0x03d00102
.dd 0x03c00103
.dd 0x03b00104
.dd 0x03a00105
.dd 0x03900106
.dd 0x03800107
.dd 0x03100108
.dd 0x03200109
.dd 0x0330010a
.dd 0x0330010b
.dd 0x0350011c
.dd 0x0360010d
.dd 0x0370010e
.dd 0x0380010f
.dd 0x03900110
.dd 0x31000111
.dd 0x32000112
.dd 0x33000113
.dd 0x34000114
