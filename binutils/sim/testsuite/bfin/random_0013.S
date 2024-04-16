# Ensure that dsp insns with IH modifiers saturate first, then round
# mach: bfin
#include "test.h"
.include "testutils.inc"

	start

	dmm32 ASTAT, (0x24304400 | _VS | _AV1S | _AV0 | _AC1 | _AQ | _AC0_COPY | _AN | _AZ);
	dmm32 A0.w, 0x3883de11;
	dmm32 A0.x, 0x00000025;
	imm32 R2, 0xeb641947;
	imm32 R3, 0x66d10863;
	imm32 R5, 0x00d44f5a;
	R5.L = (A0 += R3.L * R2.L) (IH);
	checkreg R5, 0x00d47fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x24304400 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x04b04e10 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0x1e069e1a;
	dmm32 A0.x, 0xfffffff5;
	imm32 R3, 0xffff0001;
	R3.L = A0 (IH);
	checkreg R3, 0xffff8000;
	checkreg ASTAT, (0x04b04e10 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x14f08600 | _VS | _V | _AV1S | _AV0 | _AC1 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x766c79cc;
	dmm32 A0.x, 0xffffffd9;
	imm32 R4, 0x14801bff;
	R4.L = A0 (IH);
	checkreg R4, 0x14808000;
	checkreg ASTAT, (0x14f08600 | _VS | _V | _AV1S | _AV0 | _AC1 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x6060c600 | _VS | _AV1S | _AV0S | _AC1 | _V_COPY | _AN | _AZ);
	dmm32 A0.w, 0x1e7461de;
	dmm32 A0.x, 0xffffff91;
	imm32 R6, 0x1ba08a9e;
	R6.L = A0 (IH);
	checkreg R6, 0x1ba08000;
	checkreg ASTAT, (0x6060c600 | _VS | _V | _AV1S | _AV0S | _AC1 | _V_COPY | _AN);

	dmm32 ASTAT, (0x28700e10 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN | _AZ);
	dmm32 A0.w, 0xfb5acc4e;
	dmm32 A0.x, 0xfffffffe;
	imm32 R4, 0x15baf604;
	R4.L = A0 (IH);
	checkreg R4, 0x15ba8000;
	checkreg ASTAT, (0x28700e10 | _VS | _V | _AV1S | _AC1 | _AC0 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x24708610 | _VS | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0x0de70c92;
	dmm32 A0.x, 0xffffffde;
	imm32 R3, 0x0f323c4c;
	R3.L = A0 (IH);
	checkreg R3, 0x0f328000;
	checkreg ASTAT, (0x24708610 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x6800c880 | _AV1 | _AV0 | _AQ | _AZ);
	dmm32 A0.w, 0x482bfb59;
	dmm32 A0.x, 0x0000005e;
	imm32 R6, 0x4616e4ad;
	imm32 R7, 0x4a88b2b1;
	R6.L = (A0 += R6.H * R7.L) (IH);
	checkreg R6, 0x46167fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x6800c880 | _VS | _V | _AV1 | _AV0S | _AV0 | _AQ | _V_COPY | _AZ);

	dmm32 ASTAT, (0x44d08280 | _VS | _V | _AQ | _V_COPY | _AZ);
	dmm32 A0.w, 0xf29e3a4c;
	dmm32 A0.x, 0x0000003b;
	imm32 R2, 0x004027d0;
	imm32 R4, 0x44761fd1;
	imm32 R7, 0x7fff0001;
	R7.L = (A0 -= R4.H * R2.H) (IH);
	checkreg R7, 0x7fff7fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x44d08280 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY | _AZ);

	dmm32 ASTAT, (0x18a00680 | _VS | _AV1S | _AQ | _CC);
	dmm32 A0.w, 0x174c203a;
	dmm32 A0.x, 0x00000060;
	imm32 R3, 0x1f100000;
	R3.L = A0 (IH);
	checkreg R3, 0x1f107fff;
	checkreg ASTAT, (0x18a00680 | _VS | _V | _AV1S | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x38e04090 | _VS | _AV0S | _AQ | _AN | _AZ);
	dmm32 A0.w, 0x5db9b913;
	dmm32 A0.x, 0x00000048;
	imm32 R0, 0xd513ffff;
	imm32 R2, 0xfcee02ff;
	R0.L = (A0 -= R2.H * R0.H) (IH);
	checkreg R0, 0xd5137fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x38e04090 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x2030c680 | _VS | _V | _AV0S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x113de06e;
	dmm32 A0.x, 0x00000006;
	imm32 R3, 0x3de9b335;
	R3.L = A0 (IH);
	checkreg R3, 0x3de97fff;
	checkreg ASTAT, (0x2030c680 | _VS | _V | _AV0S | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x14300210 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x3219dde5;
	dmm32 A0.x, 0xfffffffe;
	imm32 R2, 0x8000ffde;
	R2.L = A0 (IH);
	checkreg R2, 0x80008000;
	checkreg ASTAT, (0x14300210 | _VS | _V | _AV1S | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x5c304e10 | _VS | _V | _AV1S | _AC1 | _AC0 | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x500d8a96;
	dmm32 A0.x, 0x00000071;
	imm32 R2, 0x47bc6a2d;
	R2.L = A0 (IH);
	checkreg R2, 0x47bc7fff;
	checkreg ASTAT, (0x5c304e10 | _VS | _V | _AV1S | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x40d04410 | _VS | _V | _AV1S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0xed76198b;
	dmm32 A0.x, 0xffffffdd;
	imm32 R4, 0x485f8000;
	R4.L = A0 (IH);
	checkreg ASTAT, (0x40d04410 | _VS | _V | _AV1S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x34f00290 | _VS | _V | _AV1S | _AV0S | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0xc0000000;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x80008000;
	imm32 R3, 0x2cb77eda;
	R0.L = (A0 += R3.H * R3.H) (IH);
	checkreg R0, 0x80007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x34f00290 | _VS | _V | _AV1S | _AV0S | _AV0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x2490c610 | _VS | _V | _V_COPY | _AN);
	dmm32 A0.w, 0xc2375c00;
	dmm32 A0.x, 0x00000000;
	imm32 R0, 0x8000ffff;
	imm32 R1, 0xac86b35f;
	imm32 R6, 0x3cb137de;
	R0.L = (A0 -= R6.H * R1.H) (IH);
	checkreg R0, 0x80007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x2490c610 | _VS | _V | _AV0S | _AV0 | _V_COPY | _AN);

	dmm32 ASTAT, (0x3000c810 | _VS | _AC0 | _AQ | _CC | _AN);
	dmm32 A0.w, 0x44fe7a9d;
	dmm32 A0.x, 0x0000006e;
	imm32 R2, 0xbb4f8000;
	imm32 R4, 0xfe2d7fff;
	imm32 R7, 0x5da7ea43;
	R7.L = (A0 += R4.L * R2.L) (IH);
	checkreg R7, 0x5da77fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x3000c810 | _VS | _V | _AV0S | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x1c708000 | _VS | _V | _AV1S | _AV0 | _AC1 | _AQ | _V_COPY | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x6ad001aa;
	dmm32 A0.x, 0x0000002a;
	imm32 R6, 0x7fff65d9;
	R6.L = A0 (IH);
	checkreg R6, 0x7fff7fff;
	checkreg ASTAT, (0x1c708000 | _VS | _V | _AV1S | _AV0 | _AC1 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x1430cc80 | _AV0S | _AC0 | _AQ | _AN | _AZ);
	dmm32 A0.w, 0x5c04c87a;
	dmm32 A0.x, 0x00000002;
	imm32 R1, 0x6752c24c;
	imm32 R7, 0x21f7c24f;
	R1.L = (A0 -= R1.H * R7.H) (IH);
	checkreg R1, 0x67527fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x1430cc80 | _VS | _V | _AV0S | _AV0 | _AC0 | _AQ | _V_COPY | _AN | _AZ);

	dmm32 ASTAT, (0x44500c80 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0x603980cf;
	dmm32 A0.x, 0xffffffff;
	imm32 R3, 0xffffffff;
	R3.L = A0 (IH);
	checkreg R3, 0xffff8000;
	checkreg ASTAT, (0x44500c80 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x70508c90 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _V_COPY);
	dmm32 A0.w, 0x097b558d;
	dmm32 A0.x, 0x00000005;
	imm32 R1, 0x80002c0a;
	R1.L = A0 (IH);
	checkreg R1, 0x80007fff;
	checkreg ASTAT, (0x70508c90 | _VS | _V | _AV1S | _AV0S | _AC0 | _AQ | _V_COPY);

	dmm32 ASTAT, (0x1820c410 | _AC1 | _AC0 | _AQ | _CC | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x69470e6b;
	dmm32 A0.x, 0x0000005a;
	imm32 R1, 0x3a0e82ef;
	imm32 R4, 0x2c0af024;
	imm32 R6, 0x5a301523;
	R1.L = (A0 += R6.L * R4.L) (IH);
	checkreg R1, 0x3a0e7fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x1820c410 | _VS | _V | _AV0S | _AV0 | _AC1 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x14a04e10 | _VS | _V | _AV0S | _AC0 | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0xaaa829c8;
	dmm32 A0.x, 0x0000000f;
	imm32 R3, 0x901b7fff;
	imm32 R4, 0xf8d50755;
	imm32 R6, 0x0a98c742;
	R4.L = (A0 += R3.L * R6.L) (IH);
	checkreg R4, 0xf8d57fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x14a04e10 | _VS | _V | _AV0S | _AV0 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x7c70c800 | _VS | _V | _AV1S | _AC0 | _AQ | _CC | _V_COPY);
	dmm32 A0.w, 0x3875c265;
	dmm32 A0.x, 0x0000000e;
	imm32 R0, 0x8000af00;
	imm32 R3, 0x071fe97d;
	imm32 R5, 0x72d82b4b;
	R0.L = (A0 += R3.H * R5.H) (IH);
	checkreg R0, 0x80007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x7c70c800 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x04508a80 | _VS | _V | _AC0 | _AQ | _CC | _V_COPY);
	dmm32 A0.w, 0x5055d0b1;
	dmm32 A0.x, 0x00000009;
	imm32 R2, 0x7b9b1a96;
	imm32 R4, 0x56a17f45;
	R4.L = (A0 -= R4.L * R2.L) (IH);
	checkreg R4, 0x56a17fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x04508a80 | _VS | _V | _AV0S | _AV0 | _AC0 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x60408c90 | _VS | _AV1 | _CC | _AC0_COPY);
	dmm32 A1.w, 0x4d722bbd;
	dmm32 A1.x, 0x0000000a;
	imm32 R1, 0x31c46841;
	imm32 R4, 0xe31521b2;
	imm32 R6, 0x49d747d4;
	R6.H = (A1 -= R1.L * R4.L) (M, IH);
	checkreg R6, 0x7fff47d4;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x60408c90 | _VS | _V | _AV1S | _AV1 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x7c808690 | _VS | _AV1S | _AC1 | _AC0 | _AC0_COPY);
	dmm32 A0.w, 0x48379e0d;
	dmm32 A0.x, 0x00000061;
	imm32 R0, 0x272c8000;
	imm32 R4, 0x7fff7fff;
	R0.L = (A0 += R4.L * R4.H) (IH);
	checkreg R0, 0x272c7fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x7c808690 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _AC0 | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x10308800 | _VS | _AC1 | _AC0 | _AQ | _AN);
	dmm32 A1.w, 0x9ddbf339;
	dmm32 A1.x, 0x00000010;
	imm32 R1, 0x00679160;
	imm32 R5, 0x1fa0ffff;
	imm32 R6, 0x4312c2cd;
	R6.H = (A1 -= R1.L * R5.H) (IH);
	checkreg R6, 0x7fffc2cd;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x10308800 | _VS | _V | _AV1S | _AV1 | _AC1 | _AC0 | _AQ | _V_COPY | _AN);

	dmm32 ASTAT, (0x3040ca90 | _VS | _AV1S | _AV0S | _AC1 | _AQ | _CC | _AN | _AZ);
	dmm32 A0.w, 0x2d631ab7;
	dmm32 A0.x, 0x00000066;
	imm32 R5, 0x325c8000;
	R5.L = A0 (IH);
	checkreg R5, 0x325c7fff;
	checkreg ASTAT, (0x3040ca90 | _VS | _V | _AV1S | _AV0S | _AC1 | _AQ | _CC | _V_COPY);

	dmm32 ASTAT, (0x5ca08c90 | _AV1S | _AV0S | _AC0 | _AQ | _CC | _AC0_COPY);
	dmm32 A0.w, 0x86fce74b;
	dmm32 A0.x, 0x0000007f;
	imm32 R1, 0x3e9e0014;
	imm32 R7, 0x6d73d06c;
	R7.L = (A0 += R1.L * R7.H) (IH);
	checkreg R7, 0x6d737fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x5ca08c90 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC0 | _AQ | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x50e0c880 | _VS | _AC1);
	dmm32 A0.w, 0x9e40a194;
	dmm32 A0.x, 0x00000000;
	imm32 R5, 0x6ba7ac29;
	imm32 R6, 0x50a97ffe;
	R5.L = (A0 += R6.L * R5.H) (IH);
	checkreg R5, 0x6ba77fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x50e0c880 | _VS | _V | _AV0S | _AV0 | _AC1 | _V_COPY);

	dmm32 ASTAT, (0x3ce0c810 | _VS | _V | _AV1S | _CC | _V_COPY | _AC0_COPY | _AN);
	dmm32 A0.w, 0x9abe32ae;
	dmm32 A0.x, 0xffffffc2;
	imm32 R2, 0x8000e9a0;
	R2.L = A0 (IH);
	checkreg R2, 0x80008000;
	checkreg ASTAT, (0x3ce0c810 | _VS | _V | _AV1S | _CC | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x6090c010 | _VS | _AV0S | _AC0 | _CC | _AC0_COPY);
	dmm32 A0.w, 0x53e97a53;
	dmm32 A0.x, 0x0000004d;
	imm32 R1, 0x289e2e4e;
	R1.L = A0 (IH);
	checkreg R1, 0x289e7fff;
	checkreg ASTAT, (0x6090c010 | _VS | _V | _AV0S | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x34708800 | _VS | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _AC0_COPY | _AN);
	dmm32 A0.w, 0x1035b3fa;
	dmm32 A0.x, 0x00000001;
	imm32 R1, 0xec227fff;
	R1.L = A0 (IH);
	checkreg ASTAT, (0x34708800 | _VS | _V | _AV1S | _AV0S | _AC1 | _AC0 | _CC | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x30200c00 | _VS | _V | _AV1S | _AV0S | _AV0 | _AC1 | _V_COPY);
	imm32 R1, 0x30d07fff;
	imm32 R2, 0x007f1105;
	imm32 R4, 0x7fffffff;
	R1.H = R2.L * R4.L (M, IH);
	checkreg R1, 0x11057fff;
	checkreg ASTAT, (0x30200c00 | _VS | _AV1S | _AV0S | _AV0 | _AC1);

	dmm32 ASTAT, (0x1c008200 | _VS | _V | _AV1S | _AV0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);
	dmm32 A0.w, 0x46ccaead;
	dmm32 A0.x, 0x0000006b;
	imm32 R4, 0x80003753;
	imm32 R5, 0x128216a3;
	imm32 R6, 0x7c3455c4;
	R4.L = (A0 += R5.L * R6.H) (IH);
	checkreg R4, 0x80007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x1c008200 | _VS | _V | _AV1S | _AV0S | _AV0 | _AQ | _CC | _V_COPY | _AC0_COPY | _AZ);

	dmm32 ASTAT, (0x14304e10 | _VS | _AV0S | _AV0 | _AC0);
	dmm32 A0.w, 0x7fc17d70;
	dmm32 A0.x, 0x0000000f;
	imm32 R3, 0x5cb72991;
	imm32 R4, 0x3a823142;
	imm32 R7, 0xde5bf5a2;
	R7.L = (A0 += R4.H * R3.H) (IH);
	checkreg R7, 0xde5b7fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x14304e10 | _VS | _V | _AV0S | _AV0 | _AC0 | _V_COPY);

	dmm32 ASTAT, (0x10900290 | _VS | _V | _AQ | _V_COPY | _AC0_COPY);
	dmm32 A0.w, 0x7fb16a1d;
	dmm32 A0.x, 0x00000052;
	imm32 R0, 0x1e4a7fff;
	imm32 R2, 0x62b886f4;
	imm32 R3, 0x80004104;
	R3.L = (A0 -= R2.H * R0.H) (IH);
	checkreg R3, 0x80007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x10900290 | _VS | _V | _AV0S | _AV0 | _AQ | _V_COPY | _AC0_COPY);

	dmm32 ASTAT, (0x18608400 | _VS | _AV1S | _AQ | _AC0_COPY | _AN);
	dmm32 A1.w, 0x62fcbde0;
	dmm32 A1.x, 0x0000006a;
	imm32 R2, 0x60339fcc;
	imm32 R3, 0x5fa9f612;
	imm32 R4, 0x6f006000;
	R2.H = (A1 += R3.L * R4.H) (IH);
	checkreg R2, 0x7fff9fcc;
	checkreg A1.w, 0x7fffffff;
	checkreg A1.x, 0x00000000;
	checkreg ASTAT, (0x18608400 | _VS | _V | _AV1S | _AV1 | _AQ | _V_COPY | _AC0_COPY | _AN);

	dmm32 ASTAT, (0x60100210 | _VS | _V | _CC | _V_COPY | _AN);
	dmm32 A0.w, 0x52a9b75e;
	dmm32 A0.x, 0x00000003;
	imm32 R0, 0xffff349c;
	imm32 R6, 0x0084550f;
	R0.L = (A0 += R6.L * R0.H) (IH);
	checkreg R0, 0xffff7fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x60100210 | _VS | _V | _AV0S | _AV0 | _CC | _V_COPY | _AN);

	dmm32 ASTAT, (0x7840cc10 | _VS | _V | _AV0 | _AC1 | _V_COPY | _AN | _AZ);
	dmm32 A0.w, 0x22aa6b49;
	dmm32 A0.x, 0x0000006a;
	imm32 R1, 0x17528642;
	imm32 R5, 0x8000a49b;
	imm32 R6, 0x03ec4bb6;
	R5.L = (A0 -= R1.H * R6.H) (IH);
	checkreg R5, 0x80007fff;
	checkreg A0.w, 0x7fffffff;
	checkreg A0.x, 0x00000000;
	checkreg ASTAT, (0x7840cc10 | _VS | _V | _AV0S | _AV0 | _AC1 | _V_COPY | _AN | _AZ);

	pass
