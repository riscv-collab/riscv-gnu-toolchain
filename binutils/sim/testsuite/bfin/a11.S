//  Test ALU   RND RND12 RND20
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start


	R7 = 0;
	ASTAT = R7;

//    7ffffff0
// +  00008000
// -> 7fff0000
	R0 = 0xfff0 (Z);
	R0.H = 0x7fff;
	R7.L = R0 (RND);
	R0 = ASTAT;
	CHECKREG R7, 0x7fff;
	CHECKREG R0, (_VS|_V|_V_COPY);

//    7ffffff0
// +  00008000
// -> 7fff0000
	R0.L = 0xfff0;
	R0.H = 0x7fff;
	R7.H = R0 (RND);
	R0 = ASTAT;
	CHECKREG R7, 0x7fff7fff;
	CHECKREG R0, (_VS|_V|_V_COPY);

//    7ff0fff0
// +  00008000
// -> 7ff10000
	R0.L = 0xfff0;
	R0.H = 0x7ff0;
	R7.L = R0 (RND);
	R0 = ASTAT;
	CHECKREG R7, 0x7fff7ff1
	CHECKREG R0, (_VS);

//    7ff0fff0
// +  00008000
// -> 7ff10000
// 7ff0fff0
// +   8000
// -> 7ff1
	R0.L = 0xfff0;
	R0.H = 0x7ff0;
	R7.H = R0 (RND);
	R0 = ASTAT;
	CHECKREG R7, 0x7ff17ff1
	CHECKREG R0, (_VS);

//    fffffff0
// +  00008000
// -> 00000000
	R0.L = 0xfff0;
	R0.H = 0xffff;
	R7.L = R0 (RND);
	R0 = ASTAT;
	CHECKREG R7, 0x7ff10000;
	CHECKREG R0, (_VS|_AZ);

//    fffffff0
// +  00008000
// -> 00000000
	R0.L = 0xfff0;
	R0.H = 0xffff;
	R7.H = R0 (RND);
	R0 = ASTAT;
	DBGA ( R7.H , 0 );
	CHECKREG R0, (_VS|_AZ);

//    00fffff0
// +  00008000
// -> 0100
	R0.L = 0xfff0;
	R0.H = 0x00ff;
	R7.L = R0 (RND);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0100 );
	CHECKREG R0, (_VS);

// RND12

//    07ffe000
// +  00000000
// =  07ffe000
// +  00000800
// ->  7ffe
	R0.L = 0xe000;
	R0.H = 0x07ff;
	R1 = 0x0000 (Z);
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x7ffe );
	CHECKREG R0, (_VS);

//    07ffff00
// +  00000000
// =  07ffff00
// +  00000800
// ->  7fff
	R0.L = 0xff00;
	R0.H = 0x07ff;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x7fff );
	CHECKREG R0, (_VS|_V|_V_COPY);

//    07fffc00
// +  00000f00
// =  08000b00
// +  00000800
// ->  7fff
	R0.L = 0xfc00;
	R0.H = 0x07ff;
	R1.L = 0x0f00;
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x7fff );
	CHECKREG R0, (_VS|_V|_V_COPY);

//    07ff c000
// +  0000 1000
// =  07ff d000
// +  0000 0800
// ->  7ff d
	R0.L = 0xc000;
	R0.H = 0x07ff;
	R1.L = 0x1000;
	R1.H = 0x0000;
	_DBG ASTAT;
	R7.L = R0 + R1 (RND12);
	_DBG ASTAT;
	R0 = ASTAT;
	_DBG R0;
	DBGA ( R7.L , 0x7ffd );
	CHECKREG R0, (_VS);

//    ffff ffea
// +  07ff fe00
// = 107ff fdea
// +  0000 0800
// ->  7ff f
	R0.L = 0xffea;
	R0.H = 0xffff;
	R1.L = 0xfe00;
	R1.H = 0x07ff;
	_DBG ASTAT;
	R7.L = R0 + R1 (RND12);
	_DBG ASTAT;
	R0 = ASTAT;
	_DBG R0;
	DBGA ( R7.L , 0x7fff );
	CHECKREG R0, (_VS|_V|_V_COPY);

// Small negative plus small negative should give zero
//    ffff ffff
// +  ffff ffff
// +  0000 0800
// ->  000 0
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0xffff;
	R1.H = 0xffff;
	_DBG ASTAT;
	R7.L = R0 + R1 (RND12);
	R0 = ASTAT;
	_DBG R0;
	DBGA ( R7.L , 0x0000 );
	CHECKREG R0, (_VS|_AZ);

// Small negative minus small positive should give zero
//    ffff ffff
// +  0000 0001
// -  0000 0800
// ->  000 0
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7.L = R0 - R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0000 );
	CHECKREG R0, (_VS|_AZ);

// Large positive plus large positive should give maxpos
//    07ff ffff
// +  07ff ffff
// +  0000 0800
// ->  7ff f
	R0.L = 0xffff;
	R0.H = 0x07ff;
	R1.L = 0xffff;
	R1.H = 0x07ff;
	R7.L = R0 + R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x7fff );
	CHECKREG R0, (_VS|_V|_V_COPY);

// Large negative plus large negative should give maxneg
//    0800 0000
// +  0800 0000
// +  0000 0800
// ->  800 0
	R0.L = 0x0000;
	R0.H = 0x0800;
	R1.L = 0x0000;
	R1.H = 0x0800;
	R7.L = R0 + R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x7fff );
	CHECKREG R0, (_VS|_V|_V_COPY);

// Large positive minus large negative should give maxpos
//    07ff ffff
// -  0800 0000
// +  0000 0800
// ->  800 0
	R0.L = 0xffff;
	R0.H = 0x07ff;
	R1.L = 0x0000;
	R1.H = 0x0800;
	R7.L = R0 - R1 (RND12);
	R0 = ASTAT;
	_DBG ASTAT;
	DBGA ( R7.L , 0x0 );
	CHECKREG R0, (_VS|_AZ);

// Large negative minus large positive should give maxneg
//    0800 0000
// -  07ff ffff
// +  0000 0800
// ->  800 0
	R0.L = 0x0000;
	R0.H = 0x0800;
	R1.L = 0xffff;
	R1.H = 0x07ff;
	R7.L = R0 - R1 (RND12);
	R0 = ASTAT;
	_DBG ASTAT;
	DBGA ( R7.L , 0x0000 );
	CHECKREG R0, (_VS|_AZ);

//    cef4 3ed6
// -  56f4 417a
// +  0000 0800
// ->  800 0
	R0.L = 0x3ed6;
	R0.H = 0xcef4;
	R1.L = 0x417a;
	R1.H = 0x56f4;
	R7.L = R0 - R1 (RND12);
	R0 = ASTAT;
	DBGA ( R7.L , 0x8000 );
	CHECKREG R0, (_VS|_V|_V_COPY|_AN);

// RND20

//    00ff 0000
// +  0000 0000
// +  0008 0000
// ->0010
	R0.L = 0x0000;
	R0.H = 0x00ff;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0010 );
	CHECKREG R0, (_VS);

//    00f0 0000
// +  000f 0000
// +  0008 0000
// ->0010
	R0.L = 0x0000;
	R0.H = 0x00f0;
	R1.L = 0x0000;
	R1.H = 0x000f;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0010 );
	CHECKREG R0, (_VS);

//    7ff0 0000
// +  0000 0000
// +  0008 0000
// ->07ff
	R0.L = 0x0000;
	R0.H = 0x7ff0;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x07ff );
	CHECKREG R0, (_VS);

//    7fff 0000
// +  0000 0000
// +  0008 0000
// ->0800
	R0.L = 0x0000;
	R0.H = 0x7fff;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0800 );
	CHECKREG R0, (_VS);

//    ffff 0000
// +  0000 0000
// +  0008 0000
// ->0000
	R0.L = 0x0000;
	R0.H = 0xffff;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R0.H , 0x0200 );
	DBGA ( R0.L , 0x0001 );

//    ff00 0000
// +  0010 0000
// +  0008 0000
// ->fff1
	R0.L = 0x0000;
	R0.H = 0xff00;
	R1.L = 0x0000;
	R1.H = 0x0010;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0xfff1 );
	CHECKREG R0, (_VS|_AN);

//    ff00 0000
// +  0018 0000
// +  0008 0000
// ->fff2
	R0.L = 0x0000;
	R0.H = 0xff00;
	R1.L = 0x0000;
	R1.H = 0x0018;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0xfff2 );
	CHECKREG R0, (_VS|_AN);

// Small negative plus small negative should give zero
//    ffff ffff
// +  ffff ffff
// +  0008 0000
// ->0000
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R7.L = R0 + R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0000 );
	CHECKREG R0, (_VS|_AZ);

// Small negative minus small positive should give zero
//    ffff ffff
// +  0000 0010
// +  0008 0000
// ->0000
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0x0010;
	R1.H = 0x0000;
	R7.L = R0 - R1 (RND20);
	R0 = ASTAT;
	DBGA ( R7.L , 0x0000 );
	CHECKREG R0, (_VS|_AZ);

	pass
