/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

	.text
	.globl func
func:
/* 32-bit long instructions */
	.long 0x7c200176	/* brd r0,r1 */
	.long 0x7c2001b6	/* brh r0,r1 */
	.long 0x7c200136	/* brw r0,r1 */
	.long 0x7c2011b8	/* cfuged r0,r1,r2 */
	.long 0x7c201076	/* cntlzdm r0,r1,r2 */
	.long 0x7c201476	/* cnttzdm r0,r1,r2 */
	.long 0xff8007c4	/* dcffixqq f28,v0 */
	.long 0xfc01d7c4	/* dctfixqq v0,f26 */
	.long 0xf01f0ad0	/* lxvkq vs0,1 */
	.long 0xf01f82d0	/* lxvkq vs0,16 */
	.long 0xf01f8ad0	/* lxvkq vs0,17 */
	.long 0xf01f92d0	/* lxvkq vs0,18 */
	.long 0xf01f9ad0	/* lxvkq vs0,19 */
	.long 0xf01f12d0	/* lxvkq vs0,2 */
	.long 0xf01fa2d0	/* lxvkq vs0,20 */
	.long 0xf01faad0	/* lxvkq vs0,21 */
	.long 0xf01fb2d0	/* lxvkq vs0,22 */
	.long 0xf01fbad0	/* lxvkq vs0,23 */
	.long 0xf01fc2d0	/* lxvkq vs0,24 */
	.long 0xf01f1ad0	/* lxvkq vs0,3 */
	.long 0xf01f22d0	/* lxvkq vs0,4 */
	.long 0xf01f2ad0	/* lxvkq vs0,5 */
	.long 0xf01f32d0	/* lxvkq vs0,6 */
	.long 0xf01f3ad0	/* lxvkq vs0,7 */
	.long 0xf01f42d0	/* lxvkq vs0,8 */
	.long 0xf01f4ad0	/* lxvkq vs0,9 */
	.long 0x1a800000	/* lxvp vs20,0(0) */
	.long 0x1a800010	/* lxvp vs20,16(0) */
	.long 0x1a800020	/* lxvp vs20,32(0) */
	.long 0x7e800a9a	/* lxvpx vs20,0,r1 */
	.long 0x7c01101a	/* lxvrbx vs0,r1,r2 */
	.long 0x7c0110da	/* lxvrdx vs0,r1,r2 */
	.long 0x7c01105a	/* lxvrhx vs0,r1,r2 */
	.long 0x7c01109a	/* lxvrwx vs0,r1,r2 */
	.long 0x7f400066	/* mffprd r0,f26 */
	.long 0x7f600066	/* mffprd r0,f27 */
	.long 0x7f800066	/* mffprd r0,f28 */
	.long 0x7fa00066	/* mffprd r0,f29 */
	.long 0x7f400266	/* mfvsrld r0,vs26*/
	.long 0x7f600266	/* mfvsrld r0,vs27*/
	.long 0x7f800266	/* mfvsrld r0,vs28*/
	.long 0x7fa00266	/* mfvsrld r0,vs29 */
	.long 0x10000014	/* mtvsrbmi v0,0 */
	.long 0x10010015	/* mtvsrbmi v0,3 */
	.long 0x10030015	/* mtvsrbmi v0,7 */
	.long 0x10100e42	/* mtvsrbm v0,r1 */
	.long 0x10130e42	/* mtvsrdm v0,r1 */
	.long 0x10110e42	/* mtvsrhm v0,r1 */
	.long 0x10140e42	/* mtvsrqm v0,r1 */
	.long 0x10120e42	/* mtvsrwm v0,r1 */
	.long 0x60000000	/* nop */
	.long 0x7f470300	/* setbc r26,4*cr1+so */
	.long 0x7f480300	/* setbc r26,4*cr2+lt */
	.long 0x7f5f0300	/* setbc r26,4*cr7+so */
	.long 0x7f420300	/* setbc r26,eq */
	.long 0x7f410300	/* setbc r26,gt */
	.long 0x7f400300	/* setbc r26,lt */
	.long 0x7f430300	/* setbc r26,so */
	.long 0x7f470340	/* setbcr r26,4*cr1+so */
	.long 0x7f480340	/* setbcr r26,4*cr2+lt */
	.long 0x7f5f0340	/* setbcr r26,4*cr7+so */
	.long 0x7f470380	/* setnbc r26,4*cr1+so */
	.long 0x7f480380	/* setnbc r26,4*cr2+lt */
	.long 0x7f5f0380	/* setnbc r26,4*cr7+so */
	.long 0x7f420380	/* setnbc r26,eq */
	.long 0x7f410380	/* setnbc r26,gt */
	.long 0x7f400380	/* setnbc r26,lt */
	.long 0x7f430380	/* setnbc r26,so */
	.long 0x7f4703c0	/* setnbcr r26,4*cr1+so */
	.long 0x7f4803c0	/* setnbcr r26,4*cr2+lt */
	.long 0x7f5f03c0	/* setnbcr r26,4*cr7+so */
	.long 0x7f4003c0	/* setnbcr r26,lt */
	.long 0x7f4303c0	/* setnbcr r26,so */
	.long 0x1a800001	/* stxvp vs20,0(0) */
	.long 0x1a800011	/* stxvp vs20,16(0) */
	.long 0x1a800021	/* stxvp vs20,32(0) */
	.long 0x1a800031	/* stxvp vs20,48(0) */
	.long 0x7e800b9a	/* stxvpx vs20,0,r1 */
	.long 0x7c01111a	/* stxvrbx vs0,r1,r2 */
	.long 0x7c0111da	/* stxvrdx vs0,r1,r2 */
	.long 0x7c01115a	/* stxvrhx vs0,r1,r2 */
	.long 0x7c01119a	/* stxvrwx vs0,r1,r2 */
	.long 0x1001154d	/* vcfuged v0,v1,v2 */
	.long 0x1001118d	/* vclrlb v0,v1,r2 */
	.long 0x100111cd	/* vclrrb v0,v1,r2 */
	.long 0x10011784	/* vclzdm v0,v1,v2 */
	.long 0x100111c7	/* vcmpequq v0,v1,v2 */
	.long 0x10011387	/* vcmpgtsq v0,v1,v2 */
	.long 0x10011287	/* vcmpgtuq v0,v1,v2 */
	.long 0x11800941	/* vcmpsq cr3,v0,v1 */
	.long 0x11800901	/* vcmpuq cr3,v0,v1 */
	.long 0x10180e42	/* vcntmbb r0,v1,0 */
	.long 0x10190e42	/* vcntmbb r0,v1,1 */
	.long 0x101e0e42	/* vcntmbd r0,v1,0 */
	.long 0x101f0e42	/* vcntmbd r0,v1,1 */
	.long 0x101a0e42	/* vcntmbh r0,v1,0 */
	.long 0x101b0e42	/* vcntmbh r0,v1,1 */
	.long 0x101c0e42	/* vcntmbw r0,v1,0 */
	.long 0x101d0e42	/* vcntmbw r0,v1,1 */
	.long 0x100117c4	/* vctzdm v0,v1,v2 */
	.long 0x100113cb	/* vdivesd v0,v1,v2 */
	.long 0x1001130b	/* vdivesq v0,v1,v2 */
	.long 0x1001138b	/* vdivesw v0,v1,v2 */
	.long 0x100112cb	/* vdiveud v0,v1,v2 */
	.long 0x1001120b	/* vdiveuq v0,v1,v2 */
	.long 0x1001128b	/* vdiveuw v0,v1,v2 */
	.long 0x100111cb	/* vdivsd v0,v1,v2 */
	.long 0x1001110b	/* vdivsq v0,v1,v2 */
	.long 0x1001118b	/* vdivsw v0,v1,v2 */
	.long 0x100110cb	/* vdivud v0,v1,v2 */
	.long 0x1001100b	/* vdivuq v0,v1,v2 */
	.long 0x1001108b	/* vdivuw v0,v1,v2 */
	.long 0x10000e42	/* vexpandbm v0,v1 */
	.long 0x10030e42	/* vexpanddm v0,v1 */
	.long 0x10010e42	/* vexpandhm v0,v1 */
	.long 0x10040e42	/* vexpandqm v0,v1 */
	.long 0x10020e42	/* vexpandwm v0,v1 */
	.long 0x100110de	/* vextddvlx v0,v1,v2,r3 */
	.long 0x100110df	/* vextddvrx v0,v1,v2,r3 */
	.long 0x100110d8	/* vextdubvlx v0,v1,v2,r3 */
	.long 0x100110d9	/* vextdubvrx v0,v1,v2,r3 */
	.long 0x100110da	/* vextduhvlx v0,v1,v2,r3 */
	.long 0x100110db	/* vextduhvrx v0,v1,v2,r3 */
	.long 0x100110dc	/* vextduwvlx v0,v1,v2,r3 */
	.long 0x100110dd	/* vextduwvrx v0,v1,v2,r3 */
	.long 0x10080e42	/* vextractbm r0,v1 */
	.long 0x100b0e42	/* vextractdm r0,v1 */
	.long 0x10090e42	/* vextracthm r0,v1 */
	.long 0x100c0e42	/* vextractqm r0,v1 */
	.long 0x100a0e42	/* vextractwm r0,v1 */
	.long 0x101b0e02	/* vextsd2q v0,v1 */
	.long 0x10020ccc	/* vgnb r0,v1,2 */
	.long 0x10030ccc	/* vgnb r0,v1,3 */
	.long 0x10040ccc	/* vgnb r0,v1,4 */
	.long 0x10050ccc	/* vgnb r0,v1,5 */
	.long 0x10060ccc	/* vgnb r0,v1,6 */
	.long 0x10070ccc	/* vgnb r0,v1,7 */
	.long 0x1001120f	/* vinsblx v0,r1,r2 */
	.long 0x1001130f	/* vinsbrx v0,r1,r2 */
	.long 0x1001100f	/* vinsbvlx v0,r1,v2 */
	.long 0x1001110f	/* vinsbvrx v0,r1,v2 */
	.long 0x100112cf	/* vinsdlx v0,r1,r2 */
	.long 0x100113cf	/* vinsdrx v0,r1,r2 */
	.long 0x100309cf	/* vinsd v0,r1,3 */
	.long 0x100709cf	/* vinsd v0,r1,7 */
	.long 0x1001124f	/* vinshlx v0,r1,r2 */
	.long 0x1001134f	/* vinshrx v0,r1,r2 */
	.long 0x1001104f	/* vinshvlx v0,r1,v2 */
	.long 0x1001114f	/* vinshvrx v0,r1,v2 */
	.long 0x1001128f	/* vinswlx v0,r1,r2 */
	.long 0x1001138f	/* vinswrx v0,r1,r2 */
	.long 0x100308cf	/* vinsw v0,r1,3 */
	.long 0x100708cf	/* vinsw v0,r1,7 */
	.long 0x1001108f	/* vinswvlx v0,r1,v2 */
	.long 0x1001118f	/* vinswvrx v0,r1,v2 */
	.long 0x100117cb	/* vmodsd v0,v1,v2 */
	.long 0x1001170b	/* vmodsq v0,v1,v2 */
	.long 0x1001178b	/* vmodsw v0,v1,v2 */
	.long 0x100116cb	/* vmodud v0,v1,v2 */
	.long 0x1001160b	/* vmoduq v0,v1,v2 */
	.long 0x1001168b	/* vmoduw v0,v1,v2 */
	.long 0x100110d7	/* vmsumcud v0,v1,v2,v3 */
	.long 0x100113c8	/* vmulesd v0,v1,v2 */
	.long 0x100112c8	/* vmuleud v0,v1,v2 */
	.long 0x100113c9	/* vmulhsd v0,v1,v2 */
	.long 0x10011389	/* vmulhsw v0,v1,v2 */
	.long 0x100112c9	/* vmulhud v0,v1,v2 */
	.long 0x10011289	/* vmulhuw v0,v1,v2 */
	.long 0x100111c9	/* vmulld v0,v1,v2 */
	.long 0x100111c8	/* vmulosd v0,v1,v2 */
	.long 0x100110c8	/* vmuloud v0,v1,v2 */
	.long 0x100115cd	/* vpdepd v0,v1,v2 */
	.long 0x1001158d	/* vpextd v0,v1,v2 */
	.long 0x10011045	/* vrlqmi v0,v1,v2 */
	.long 0x10011145	/* vrlqnm v0,v1,v2 */
	.long 0x10011005	/* vrlq v0,v1,v2 */
	.long 0x10011016	/* vsldbi v0,v1,v2,0 */
	.long 0x10011116	/* vsldbi v0,v1,v2,4 */
	.long 0x10011105	/* vslq v0,v1,v2 */
	.long 0x10011305	/* vsraq v0,v1,v2 */
	.long 0x10011216	/* vsrdbi v0,v1,v2,0 */
	.long 0x10011316	/* vsrdbi v0,v1,v2,4 */
	.long 0x10011205	/* vsrq v0,v1,v2 */
	.long 0x1000080d	/* vstribl v0,v1 */
	.long 0x10000c0d	/* vstribl. v0,v1 */
	.long 0x1001080d	/* vstribr v0,v1 */
	.long 0x10010c0d	/* vstribr. v0,v1 */
	.long 0x1002080d	/* vstrihl v0,v1 */
	.long 0x10020c0d	/* vstrihl. v0,v1 */
	.long 0x1003080d	/* vstrihr v0,v1 */
	.long 0x10030c0d	/* vstrihr. v0,v1 */
	.long 0xfc011088	/* xscmpeqqp v0,v1,v2 */
	.long 0xfc011188	/* xscmpgeqp v0,v1,v2 */
	.long 0xfc0111c8	/* xscmpgtqp v0,v1,v2 */
	.long 0xfc080e88	/* xscvqpsqz v0,v1 */
	.long 0xfc000e88	/* xscvqpuqz v0,v1 */
	.long 0xfc0b0e88	/* xscvsqqp v0,v1 */
	.long 0xfc030e88	/* xscvuqqp v0,v1 */
	.long 0xfc011548	/* xsmaxcqp v0,v1,v2 */
	.long 0xfc0115c8	/* xsmincqp v0,v1,v2 */
	.long 0xee000998	/* xvbf16ger2 a4,vs0,vs1 */
	.long 0xee000f90	/* xvbf16ger2nn a4,vs0,vs1 */
	.long 0xee000b90	/* xvbf16ger2np a4,vs0,vs1 */
	.long 0xee000d90	/* xvbf16ger2pn a4,vs0,vs1 */
	.long 0xee000990	/* xvbf16ger2pp a4,vs0,vs1 */
	.long 0xf0100f6c	/* xvcvbf16spn vs0,vs1 */
	.long 0xf0110f6c	/* xvcvspbf16 vs0,vs1 */
	.long 0xee000898	/* xvf16ger2 a4,vs0,vs1 */
	.long 0xee000e90	/* xvf16ger2nn a4,vs0,vs1 */
	.long 0xee000a90	/* xvf16ger2np a4,vs0,vs1 */
	.long 0xee000c90	/* xvf16ger2pn a4,vs0,vs1 */
	.long 0xee000890	/* xvf16ger2pp a4,vs0,vs1 */
	.long 0xee0008d8	/* xvf32ger a4,vs0,vs1 */
	.long 0xee000ed0	/* xvf32gernn a4,vs0,vs1 */
	.long 0xee000ad0	/* xvf32gernp a4,vs0,vs1 */
	.long 0xee000cd0	/* xvf32gerpn a4,vs0,vs1 */
	.long 0xee0008d0	/* xvf32gerpp a4,vs0,vs1 */
	.long 0xee1601d8	/* xvf64ger a4,vs22,vs0 */
	.long 0xee1607d0	/* xvf64gernn a4,vs22,vs0 */
	.long 0xee1603d0	/* xvf64gernp a4,vs22,vs0 */
	.long 0xee1605d0	/* xvf64gerpn a4,vs22,vs0 */
	.long 0xee1601d0	/* xvf64gerpp a4,vs22,vs0 */
	.long 0xee000a58	/* xvi16ger2 a4,vs0,vs1 */
	.long 0xee000b58	/* xvi16ger2pp a4,vs0,vs1 */
	.long 0xee000958	/* xvi16ger2s a4,vs0,vs1 */
	.long 0xee000950	/* xvi16ger2spp a4,vs0,vs1 */
	.long 0xee000918	/* xvi4ger8 a4,vs0,vs1 */
	.long 0xee000910	/* xvi4ger8pp a4,vs0,vs1 */
	.long 0xee000818	/* xvi8ger4 a4,vs0,vs1 */
	.long 0xee000810	/* xvi8ger4pp a4,vs0,vs1 */
	.long 0xee000b18	/* xvi8ger4spp a4,vs0,vs1 */
	.long 0xf182076c	/* xvtlsbb cr3,vs0 */
	.long 0xf0000f28	/* xxgenpcvbm vs0,v1,0 */
	.long 0xf0010f28	/* xxgenpcvbm vs0,v1,1 */
	.long 0xf0020f28	/* xxgenpcvbm vs0,v1,2 */
	.long 0xf0030f28	/* xxgenpcvbm vs0,v1,3 */
	.long 0xf0000f6a	/* xxgenpcvdm vs0,v1,0 */
	.long 0xf0010f6a	/* xxgenpcvdm vs0,v1,1 */
	.long 0xf0020f6a	/* xxgenpcvdm vs0,v1,2 */
	.long 0xf0030f6a	/* xxgenpcvdm vs0,v1,3 */
	.long 0xf0000f2a	/* xxgenpcvhm vs0,v1,0 */
	.long 0xf0010f2a	/* xxgenpcvhm vs0,v1,1 */
	.long 0xf0020f2a	/* xxgenpcvhm vs0,v1,2 */
	.long 0xf0030f2a	/* xxgenpcvhm vs0,v1,3 */
	.long 0xf0000f68	/* xxgenpcvwm vs0,v1,0 */
	.long 0xf0010f68	/* xxgenpcvwm vs0,v1,1 */
	.long 0xf0020f68	/* xxgenpcvwm vs0,v1,2 */
	.long 0xf0030f68	/* xxgenpcvwm vs0,v1,3 */
	.long 0x7e000162	/* xxmfacc a4 */
	.long 0x7e010162	/* xxmtacc a4 */
	.long 0x7e030162	/* xxsetaccz a4 */

/* Prefixed instructions are 64-bits long.  Use multiple .long statements to
   represent the 64-bit instructions so the instructions are properly
   represented in the machines native byte order when words are fetched 32-bits
   at a time.  The first .long is for the 32-bit prefix word and the second
   .long is for the 32-bit suffix word thus making the word order independent
   of the machine Endianes.  The use of the .quad statement on Big Endian
   results in the disassembler fetching the 32-bit suffix before the 32-bit
   prefix word.  */
	.long 0x06000000	/* paddi r0,r1,0 */
	.long 0x38010000
	.long 0x06000000	/* paddi r0,r1,12 */
	.long 0x3801000c
	.long 0x06000000	/* paddi r0,r1,48 */
	.long 0x38010030
	.long 0x06000000	/* paddi r0,r1,98 */
	.long 0x38010062
	.long 0x7c201138	/* pdepd r0,r1,r2 */
	.long 0x7c201178
	.long 0x06000000	/* plbz r0,0(r1) */
	.long 0x88010000
	.long 0x06000000	/* plbz r0,16(r1) */
	.long 0x88010010
	.long 0x06000000	/* plbz r0,32(r1) */
	.long 0x88010020
	.long 0x06000000	/* plbz r0,64(r1) */
	.long 0x88010040
	.long 0x06000000	/* plbz r0,8(r1) */
	.long 0x88010008
	.long 0x04000000	/* pld r0,0(r1) */
	.long 0xe4010000
	.long 0x04000000	/* pld r0,16(r1) */
	.long 0xe4010010
	.long 0x04000000	/* pld r0,32(r1) */
	.long 0xe4010020
	.long 0x04000000	/* pld r0,64(r1) */
	.long 0xe4010040
	.long 0x04000000	/* pld r0,8(r1) */
	.long 0xe4010008
	.long 0x06000000	/* plfd f28,0(0) */
	.long 0xcb800000
	.long 0x06000000	/* plfd f28,16(0) */
	.long 0xcb800010
	.long 0x06000000	/* plfd f28,32(0) */
	.long 0xcb800020
	.long 0x06000000	/* plfd f28,4(0) */
	.long 0xcb800004
	.long 0x06000000	/* plfd f28,64(0) */
	.long 0xcb800040
	.long 0x06000000	/* plfd f28,8(0) */
	.long 0xcb800008
	.long 0x06000000	/* plfs f28,0(0) */
	.long 0xc3800000
	.long 0x06000000	/* plfs f28,16(0) */
	.long 0xc3800010
	.long 0x06000000	/* plfs f28,32(0) */
	.long 0xc3800020
	.long 0x06000000	/* plfs f28,4(0) */
	.long 0xc3800004
	.long 0x06000000	/* plfs f28,64(0) */
	.long 0xc3800040
	.long 0x06000000	/* plfs f28,8(0) */
	.long 0xc3800008
	.long 0x06000000	/* plha r0,0(r1) */
	.long 0xa8010000
	.long 0x06000000	/* plha r0,16(r1) */
	.long 0xa8010010
	.long 0x06000000	/* plha r0,32(r1) */
	.long 0xa8010020
	.long 0x06000000	/* plha r0,64(r1) */
	.long 0xa8010040
	.long 0x06000000	/* plha r0,8(r1) */
	.long 0xa8010008
	.long 0x06000000	/* plhz r0,0(r1) */
	.long 0xa0010000
	.long 0x06000000	/* plhz r0,16(r1) */
	.long 0xa0010010
	.long 0x06000000	/* plhz r0,32(r1) */
	.long 0xa0010020
	.long 0x06000000	/* plhz r0,64(r1) */
	.long 0xa0010040
	.long 0x06000000	/* plhz r0,8(r1) */
	.long 0xa0010008
	.long 0x04000000	/* plq r26,0(0) */
	.long 0xe3400000
	.long 0x04000000	/* plq r26,16(0) */
	.long 0xe3400010
	.long 0x04000000	/* plq r26,32(0) */
	.long 0xe3400020
	.long 0x04000000	/* plq r26,48(0) */
	.long 0xe3400030
	.long 0x04000000	/* plq r26,64(0) */
	.long 0xe3400040
	.long 0x04000000	/* plq r26,8(0) */
	.long 0xe3400008
	.long 0x04000000	/* plwa r0,0(r1) */
	.long 0xa4010000
	.long 0x04000000	/* plwa r0,16(r1) */
	.long 0xa4010010
	.long 0x04000000	/* plwa r0,32(r1) */
	.long 0xa4010020
	.long 0x04000000	/* plwa r0,64(r1) */
	.long 0xa4010040
	.long 0x04000000	/* plwa r0,8(r1) */
	.long 0xa4010008
	.long 0x06000000	/* plwz r0,0(r1) */
	.long 0x80010000
	.long 0x06000000	/* plwz r0,16(r1) */
	.long 0x80010010
	.long 0x06000000	/* plwz r0,32(r1) */
	.long 0x80010020
	.long 0x06000000	/* plwz r0,64(r1) */
	.long 0x80010040
	.long 0x06000000	/* plwz r0,8(r1) */
	.long 0x80010008
	.long 0x04000000	/* plxsd v0,0(r1) */
	.long 0xa8010000
	.long 0x04000000	/* plxsd v0,16(r1) */
	.long 0xa8010010
	.long 0x04000000	/* plxsd v0,32(r1) */
	.long 0xa8010020
	.long 0x04000000	/* plxsd v0,4(r1) */
	.long 0xa8010004
	.long 0x04000000	/* plxsd v0,64(r1) */
	.long 0xa8010040
	.long 0x04000000	/* plxsd v0,8(r1) */
	.long 0xa8010008
	.long 0x04000000	/* plxssp v0,0(r1) */
	.long 0xac010000
	.long 0x04000000	/* plxssp v0,16(r1) */
	.long 0xac010010
	.long 0x04000000	/* plxssp v0,32(r1) */
	.long 0xac010020
	.long 0x04000000	/* plxssp v0,4(r1) */
	.long 0xac010004
	.long 0x04000000	/* plxssp v0,64(r1) */
	.long 0xac010040
	.long 0x04000000	/* plxssp v0,8(r1) */
	.long 0xac010008
	.long 0x04000000	/* plxvp vs20,0(0) */
	.long 0xea800000
	.long 0x04000000	/* plxvp vs20,16(0) */
	.long 0xea800010
	.long 0x04000000	/* plxvp vs20,24(0) */
	.long 0xea800018
	.long 0x04000000	/* plxvp vs20,32(0) */
	.long 0xea800020
	.long 0x04000000	/* plxvp vs20,8(0) */
	.long 0xea800008
	.long 0x04000000	/* plxv vs0,0(r1) */
	.long 0xc8010000
	.long 0x04000000	/* plxv vs0,16(r1) */
	.long 0xc8010010
	.long 0x04000000	/* plxv vs0,4(r1) */
	.long 0xc8010004
	.long 0x04000000	/* plxv vs0,8(r1) */
	.long 0xc8010008
	.long 0x07900000	/* pmdmxvbf16ger2 a4,vs0,vs1,0,0,0 */
	.long 0xee000998
	.long 0x07904000	/* pmdmxvbf16ger2 a4,vs0,vs1,0,0,1 */
	.long 0xee000998
	.long 0x0790000d	/* pmdmxvbf16ger2 a4,vs0,vs1,0,13,0 */
	.long 0xee000998
	.long 0x0790400d	/* pmdmxvbf16ger2 a4,vs0,vs1,0,13,1 */
	.long 0xee000998
	.long 0x079000b0	/* pmdmxvbf16ger2 a4,vs0,vs1,11,0,0 */
	.long 0xee000998
	.long 0x079040b0	/* pmdmxvbf16ger2 a4,vs0,vs1,11,0,1 */
	.long 0xee000998
	.long 0x079000bd	/* pmdmxvbf16ger2 a4,vs0,vs1,11,13,0 */
	.long 0xee000998
	.long 0x079040bd	/* pmdmxvbf16ger2 a4,vs0,vs1,11,13,1 */
	.long 0xee000998
	.long 0x07900000	/* pmdmxvbf16ger2nn a4,vs0,vs1,0,0,0 */
	.long 0xee000f90
	.long 0x07904000	/* pmdmxvbf16ger2nn a4,vs0,vs1,0,0,1 */
	.long 0xee000f90
	.long 0x0790000d	/* pmdmxvbf16ger2nn a4,vs0,vs1,0,13,0 */
	.long 0xee000f90
	.long 0x0790400d	/* pmdmxvbf16ger2nn a4,vs0,vs1,0,13,1 */
	.long 0xee000f90
	.long 0x079000b0	/* pmdmxvbf16ger2nn a4,vs0,vs1,11,0,0 */
	.long 0xee000f90
	.long 0x079040b0	/* pmdmxvbf16ger2nn a4,vs0,vs1,11,0,1 */
	.long 0xee000f90
	.long 0x079000bd	/* pmdmxvbf16ger2nn a4,vs0,vs1,11,13,0 */
	.long 0xee000f90
	.long 0x079040bd	/* pmdmxvbf16ger2nn a4,vs0,vs1,11,13,1 */
	.long 0xee000f90
	.long 0x07900000	/* pmdmxvbf16ger2np a4,vs0,vs1,0,0,0 */
	.long 0xee000b90
	.long 0x07904000	/* pmdmxvbf16ger2np a4,vs0,vs1,0,0,1 */
	.long 0xee000b90
	.long 0x0790000d	/* pmdmxvbf16ger2np a4,vs0,vs1,0,13,0 */
	.long 0xee000b90
	.long 0x0790400d	/* pmdmxvbf16ger2np a4,vs0,vs1,0,13,1 */
	.long 0xee000b90
	.long 0x079000b0	/* pmdmxvbf16ger2np a4,vs0,vs1,11,0,0 */
	.long 0xee000b90
	.long 0x079040b0	/* pmdmxvbf16ger2np a4,vs0,vs1,11,0,1 */
	.long 0xee000b90
	.long 0x079000bd	/* pmdmxvbf16ger2np a4,vs0,vs1,11,13,0 */
	.long 0xee000b90
	.long 0x079040bd	/* pmdmxvbf16ger2np a4,vs0,vs1,11,13,1 */
	.long 0xee000b90
	.long 0x07900000	/* pmdmxvbf16ger2pn a4,vs0,vs1,0,0,0 */
	.long 0xee000d90
	.long 0x07904000	/* pmdmxvbf16ger2pn a4,vs0,vs1,0,0,1 */
	.long 0xee000d90
	.long 0x0790000d	/* pmdmxvbf16ger2pn a4,vs0,vs1,0,13,0 */
	.long 0xee000d90
	.long 0x0790400d	/* pmdmxvbf16ger2pn a4,vs0,vs1,0,13,1 */
	.long 0xee000d90
	.long 0x079000b0	/* pmdmxvbf16ger2pn a4,vs0,vs1,11,0,0 */
	.long 0xee000d90
	.long 0x079040b0	/* pmdmxvbf16ger2pn a4,vs0,vs1,11,0,1 */
	.long 0xee000d90
	.long 0x079000bd	/* pmdmxvbf16ger2pn a4,vs0,vs1,11,13,0 */
	.long 0xee000d90
	.long 0x079040bd	/* pmdmxvbf16ger2pn a4,vs0,vs1,11,13,1 */
	.long 0xee000d90
	.long 0x07900000	/* pmdmxvbf16ger2pp a4,vs0,vs1,0,0,0 */
	.long 0xee000990
	.long 0x07904000	/* pmdmxvbf16ger2pp a4,vs0,vs1,0,0,1 */
	.long 0xee000990
	.long 0x0790000d	/* pmdmxvbf16ger2pp a4,vs0,vs1,0,13,0 */
	.long 0xee000990
	.long 0x0790400d	/* pmdmxvbf16ger2pp a4,vs0,vs1,0,13,1 */
	.long 0xee000990
	.long 0x079000b0	/* pmdmxvbf16ger2pp a4,vs0,vs1,11,0,0 */
	.long 0xee000990
	.long 0x079040b0	/* pmdmxvbf16ger2pp a4,vs0,vs1,11,0,1 */
	.long 0xee000990
	.long 0x079000bd	/* pmdmxvbf16ger2pp a4,vs0,vs1,11,13,0 */
	.long 0xee000990
	.long 0x079040bd	/* pmdmxvbf16ger2pp a4,vs0,vs1,11,13,1 */
	.long 0xee000990
	.long 0x07900000	/* pmdmxvf16ger2 a4,vs0,vs1,0,0,0 */
	.long 0xee000898
	.long 0x07904000	/* pmdmxvf16ger2 a4,vs0,vs1,0,0,1 */
	.long 0xee000898
	.long 0x0790000d	/* pmdmxvf16ger2 a4,vs0,vs1,0,13,0 */
	.long 0xee000898
	.long 0x0790400d	/* pmdmxvf16ger2 a4,vs0,vs1,0,13,1 */
	.long 0xee000898
	.long 0x079000b0	/* pmdmxvf16ger2 a4,vs0,vs1,11,0,0 */
	.long 0xee000898
	.long 0x079040b0	/* pmdmxvf16ger2 a4,vs0,vs1,11,0,1 */
	.long 0xee000898
	.long 0x079000bd	/* pmdmxvf16ger2 a4,vs0,vs1,11,13,0 */
	.long 0xee000898
	.long 0x079040bd	/* pmdmxvf16ger2 a4,vs0,vs1,11,13,1 */
	.long 0xee000898
	.long 0x07900000	/* pmdmxvf16ger2nn a4,vs0,vs1,0,0,0 */
	.long 0xee000e90
	.long 0x07904000	/* pmdmxvf16ger2nn a4,vs0,vs1,0,0,1 */
	.long 0xee000e90
	.long 0x0790000d	/* pmdmxvf16ger2nn a4,vs0,vs1,0,13,0 */
	.long 0xee000e90
	.long 0x0790400d	/* pmdmxvf16ger2nn a4,vs0,vs1,0,13,1 */
	.long 0xee000e90
	.long 0x079000b0	/* pmdmxvf16ger2nn a4,vs0,vs1,11,0,0 */
	.long 0xee000e90
	.long 0x079040b0	/* pmdmxvf16ger2nn a4,vs0,vs1,11,0,1 */
	.long 0xee000e90
	.long 0x079000bd	/* pmdmxvf16ger2nn a4,vs0,vs1,11,13,0 */
	.long 0xee000e90
	.long 0x079040bd	/* pmdmxvf16ger2nn a4,vs0,vs1,11,13,1 */
	.long 0xee000e90
	.long 0x07900000	/* pmdmxvf16ger2np a4,vs0,vs1,0,0,0 */
	.long 0xee000a90
	.long 0x07904000	/* pmdmxvf16ger2np a4,vs0,vs1,0,0,1 */
	.long 0xee000a90
	.long 0x0790000d	/* pmdmxvf16ger2np a4,vs0,vs1,0,13,0 */
	.long 0xee000a90
	.long 0x0790400d	/* pmdmxvf16ger2np a4,vs0,vs1,0,13,1 */
	.long 0xee000a90
	.long 0x079000b0	/* pmdmxvf16ger2np a4,vs0,vs1,11,0,0 */
	.long 0xee000a90
	.long 0x079040b0	/* pmdmxvf16ger2np a4,vs0,vs1,11,0,1 */
	.long 0xee000a90
	.long 0x079000bd	/* pmdmxvf16ger2np a4,vs0,vs1,11,13,0 */
	.long 0xee000a90
	.long 0x079040bd	/* pmdmxvf16ger2np a4,vs0,vs1,11,13,1 */
	.long 0xee000a90
	.long 0x07900000	/* pmdmxvf16ger2pn a4,vs0,vs1,0,0,0 */
	.long 0xee000c90
	.long 0x07904000	/* pmdmxvf16ger2pn a4,vs0,vs1,0,0,1 */
	.long 0xee000c90
	.long 0x0790000d	/* pmdmxvf16ger2pn a4,vs0,vs1,0,13,0 */
	.long 0xee000c90
	.long 0x0790400d	/* pmdmxvf16ger2pn a4,vs0,vs1,0,13,1 */
	.long 0xee000c90
	.long 0x079000b0	/* pmdmxvf16ger2pn a4,vs0,vs1,11,0,0 */
	.long 0xee000c90
	.long 0x079040b0	/* pmdmxvf16ger2pn a4,vs0,vs1,11,0,1 */
	.long 0xee000c90
	.long 0x079000bd	/* pmdmxvf16ger2pn a4,vs0,vs1,11,13,0 */
	.long 0xee000c90
	.long 0x079040bd	/* pmdmxvf16ger2pn a4,vs0,vs1,11,13,1 */
	.long 0xee000c90
	.long 0x07900000	/* pmdmxvf16ger2pp a4,vs0,vs1,0,0,0 */
	.long 0xee000890
	.long 0x07904000	/* pmdmxvf16ger2pp a4,vs0,vs1,0,0,1 */
	.long 0xee000890
	.long 0x0790000d	/* pmdmxvf16ger2pp a4,vs0,vs1,0,13,0 */
	.long 0xee000890
	.long 0x0790400d	/* pmdmxvf16ger2pp a4,vs0,vs1,0,13,1 */
	.long 0xee000890
	.long 0x079000b0	/* pmdmxvf16ger2pp a4,vs0,vs1,11,0,0 */
	.long 0xee000890
	.long 0x079040b0	/* pmdmxvf16ger2pp a4,vs0,vs1,11,0,1 */
	.long 0xee000890
	.long 0x079000bd	/* pmdmxvf16ger2pp a4,vs0,vs1,11,13,0 */
	.long 0xee000890
	.long 0x079040bd	/* pmdmxvf16ger2pp a4,vs0,vs1,11,13,1 */
	.long 0xee000890
	.long 0x07900000	/* pmdmxvf32ger a4,vs0,vs1,0,0 */
	.long 0xee0008d8
	.long 0x0790000d	/* pmdmxvf32ger a4,vs0,vs1,0,13 */
	.long 0xee0008d8
	.long 0x079000b0	/* pmdmxvf32ger a4,vs0,vs1,11,0 */
	.long 0xee0008d8
	.long 0x079000bd	/* pmdmxvf32ger a4,vs0,vs1,11,13 */
	.long 0xee0008d8
	.long 0x07900000	/* pmdmxvf32gernn a4,vs0,vs1,0,0 */
	.long 0xee000ed0
	.long 0x0790000d	/* pmdmxvf32gernn a4,vs0,vs1,0,13 */
	.long 0xee000ed0
	.long 0x079000b0	/* pmdmxvf32gernn a4,vs0,vs1,11,0 */
	.long 0xee000ed0
	.long 0x079000bd	/* pmdmxvf32gernn a4,vs0,vs1,11,13 */
	.long 0xee000ed0
	.long 0x07900000	/* pmdmxvf32gernp a4,vs0,vs1,0,0 */
	.long 0xee000ad0
	.long 0x0790000d	/* pmdmxvf32gernp a4,vs0,vs1,0,13 */
	.long 0xee000ad0
	.long 0x079000b0	/* pmdmxvf32gernp a4,vs0,vs1,11,0 */
	.long 0xee000ad0
	.long 0x079000bd	/* pmdmxvf32gernp a4,vs0,vs1,11,13 */
	.long 0xee000ad0
	.long 0x07900000	/* pmdmxvf32gerpn a4,vs0,vs1,0,0 */
	.long 0xee000cd0
	.long 0x0790000d	/* pmdmxvf32gerpn a4,vs0,vs1,0,13 */
	.long 0xee000cd0
	.long 0x079000b0	/* pmdmxvf32gerpn a4,vs0,vs1,11,0 */
	.long 0xee000cd0
	.long 0x079000bd	/* pmdmxvf32gerpn a4,vs0,vs1,11,13 */
	.long 0xee000cd0
	.long 0x07900000	/* pmdmxvf32gerpp a4,vs0,vs1,0,0 */
	.long 0xee0008d0
	.long 0x0790000d	/* pmdmxvf32gerpp a4,vs0,vs1,0,13 */
	.long 0xee0008d0
	.long 0x079000b0	/* pmdmxvf32gerpp a4,vs0,vs1,11,0 */
	.long 0xee0008d0
	.long 0x079000bd	/* pmdmxvf32gerpp a4,vs0,vs1,11,13 */
	.long 0xee0008d0
	.long 0x07900000	/* pmdmxvf64ger a4,vs22,vs0,0,0 */
	.long 0xee1601d8
	.long 0x07900004	/* pmdmxvf64ger a4,vs22,vs0,0,1 */
	.long 0xee1601d8
	.long 0x079000b0	/* pmdmxvf64ger a4,vs22,vs0,11,0 */
	.long 0xee1601d8
	.long 0x079000b4	/* pmdmxvf64ger a4,vs22,vs0,11,1 */
	.long 0xee1601d8
	.long 0x07900000	/* pmdmxvf64gernn a4,vs22,vs0,0,0 */
	.long 0xee1607d0
	.long 0x07900004	/* pmdmxvf64gernn a4,vs22,vs0,0,1 */
	.long 0xee1607d0
	.long 0x079000b0	/* pmdmxvf64gernn a4,vs22,vs0,11,0 */
	.long 0xee1607d0
	.long 0x079000b4	/* pmdmxvf64gernn a4,vs22,vs0,11,1 */
	.long 0xee1607d0
	.long 0x07900000	/* pmdmxvf64gernp a4,vs22,vs0,0,0 */
	.long 0xee1603d0
	.long 0x07900004	/* pmdmxvf64gernp a4,vs22,vs0,0,1 */
	.long 0xee1603d0
	.long 0x079000b0	/* pmdmxvf64gernp a4,vs22,vs0,11,0 */
	.long 0xee1603d0
	.long 0x079000b4	/* pmdmxvf64gernp a4,vs22,vs0,11,1 */
	.long 0xee1603d0
	.long 0x07900000	/* pmdmxvf64gerpn a4,vs22,vs0,0,0 */
	.long 0xee1605d0
	.long 0x07900004	/* pmdmxvf64gerpn a4,vs22,vs0,0,1 */
	.long 0xee1605d0
	.long 0x079000b0	/* pmdmxvf64gerpn a4,vs22,vs0,11,0 */
	.long 0xee1605d0
	.long 0x079000b4	/* pmdmxvf64gerpn a4,vs22,vs0,11,1 */
	.long 0xee1605d0
	.long 0x07900000	/* pmdmxvf64gerpp a4,vs22,vs0,0,0 */
	.long 0xee1601d0
	.long 0x07900004	/* pmdmxvf64gerpp a4,vs22,vs0,0,1 */
	.long 0xee1601d0
	.long 0x079000b0	/* pmdmxvf64gerpp a4,vs22,vs0,11,0 */
	.long 0xee1601d0
	.long 0x079000b4	/* pmdmxvf64gerpp a4,vs22,vs0,11,1 */
	.long 0xee1601d0
	.long 0x07900000	/* pmdmxvi16ger2 a4,vs0,vs1,0,0,0 */
	.long 0xee000a58
	.long 0x07904000	/* pmdmxvi16ger2 a4,vs0,vs1,0,0,1 */
	.long 0xee000a58
	.long 0x0790000d	/* pmdmxvi16ger2 a4,vs0,vs1,0,13,0 */
	.long 0xee000a58
	.long 0x0790400d	/* pmdmxvi16ger2 a4,vs0,vs1,0,13,1 */
	.long 0xee000a58
	.long 0x079000b0	/* pmdmxvi16ger2 a4,vs0,vs1,11,0,0 */
	.long 0xee000a58
	.long 0x079040b0	/* pmdmxvi16ger2 a4,vs0,vs1,11,0,1 */
	.long 0xee000a58
	.long 0x079000bd	/* pmdmxvi16ger2 a4,vs0,vs1,11,13,0 */
	.long 0xee000a58
	.long 0x079040bd	/* pmdmxvi16ger2 a4,vs0,vs1,11,13,1 */
	.long 0xee000a58
	.long 0x07900000	/* pmdmxvi16ger2pp a4,vs0,vs1,0,0,0 */
	.long 0xee000b58
	.long 0x07904000	/* pmdmxvi16ger2pp a4,vs0,vs1,0,0,1 */
	.long 0xee000b58
	.long 0x0790000d	/* pmdmxvi16ger2pp a4,vs0,vs1,0,13,0 */
	.long 0xee000b58
	.long 0x0790400d	/* pmdmxvi16ger2pp a4,vs0,vs1,0,13,1 */
	.long 0xee000b58
	.long 0x079000b0	/* pmdmxvi16ger2pp a4,vs0,vs1,11,0,0 */
	.long 0xee000b58
	.long 0x079040b0	/* pmdmxvi16ger2pp a4,vs0,vs1,11,0,1 */
	.long 0xee000b58
	.long 0x079000bd	/* pmdmxvi16ger2pp a4,vs0,vs1,11,13,0 */
	.long 0xee000b58
	.long 0x079040bd	/* pmdmxvi16ger2pp a4,vs0,vs1,11,13,1 */
	.long 0xee000b58
	.long 0x07900000	/* pmdmxvi16ger2s a4,vs0,vs1,0,0,0 */
	.long 0xee000958
	.long 0x07904000	/* pmdmxvi16ger2s a4,vs0,vs1,0,0,1 */
	.long 0xee000958
	.long 0x0790000d	/* pmdmxvi16ger2s a4,vs0,vs1,0,13,0 */
	.long 0xee000958
	.long 0x0790400d	/* pmdmxvi16ger2s a4,vs0,vs1,0,13,1 */
	.long 0xee000958
	.long 0x079000b0	/* pmdmxvi16ger2s a4,vs0,vs1,11,0,0 */
	.long 0xee000958
	.long 0x079040b0	/* pmdmxvi16ger2s a4,vs0,vs1,11,0,1 */
	.long 0xee000958
	.long 0x079000bd	/* pmdmxvi16ger2s a4,vs0,vs1,11,13,0 */
	.long 0xee000958
	.long 0x079040bd	/* pmdmxvi16ger2s a4,vs0,vs1,11,13,1 */
	.long 0xee000958
	.long 0x07900000	/* pmdmxvi16ger2spp a4,vs0,vs1,0,0,0 */
	.long 0xee000950
	.long 0x07904000	/* pmdmxvi16ger2spp a4,vs0,vs1,0,0,1 */
	.long 0xee000950
	.long 0x0790000d	/* pmdmxvi16ger2spp a4,vs0,vs1,0,13,0 */
	.long 0xee000950
	.long 0x0790400d	/* pmdmxvi16ger2spp a4,vs0,vs1,0,13,1 */
	.long 0xee000950
	.long 0x079000b0	/* pmdmxvi16ger2spp a4,vs0,vs1,11,0,0 */
	.long 0xee000950
	.long 0x079040b0	/* pmdmxvi16ger2spp a4,vs0,vs1,11,0,1 */
	.long 0xee000950
	.long 0x079000bd	/* pmdmxvi16ger2spp a4,vs0,vs1,11,13,0 */
	.long 0xee000950
	.long 0x079040bd	/* pmdmxvi16ger2spp a4,vs0,vs1,11,13,1 */
	.long 0xee000950
	.long 0x07900000	/* pmdmxvi4ger8 a4,vs0,vs1,0,0,0 */
	.long 0xee000918
	.long 0x07902d00	/* pmdmxvi4ger8 a4,vs0,vs1,0,0,45 */
	.long 0xee000918
	.long 0x07900001	/* pmdmxvi4ger8 a4,vs0,vs1,0,1,0 */
	.long 0xee000918
	.long 0x07902d01	/* pmdmxvi4ger8 a4,vs0,vs1,0,1,45 */
	.long 0xee000918
	.long 0x079000b0	/* pmdmxvi4ger8 a4,vs0,vs1,11,0,0 */
	.long 0xee000918
	.long 0x07902db0	/* pmdmxvi4ger8 a4,vs0,vs1,11,0,45 */
	.long 0xee000918
	.long 0x079000b1	/* pmdmxvi4ger8 a4,vs0,vs1,11,1,0 */
	.long 0xee000918
	.long 0x07902db1	/* pmdmxvi4ger8 a4,vs0,vs1,11,1,45 */
	.long 0xee000918
	.long 0x07900000	/* pmdmxvi4ger8pp a4,vs0,vs1,0,0,0 */
	.long 0xee000910
	.long 0x07902d00	/* pmdmxvi4ger8pp a4,vs0,vs1,0,0,45 */
	.long 0xee000910
	.long 0x07900001	/* pmdmxvi4ger8pp a4,vs0,vs1,0,1,0 */
	.long 0xee000910
	.long 0x07902d01	/* pmdmxvi4ger8pp a4,vs0,vs1,0,1,45 */
	.long 0xee000910
	.long 0x079000b0	/* pmdmxvi4ger8pp a4,vs0,vs1,11,0,0 */
	.long 0xee000910
	.long 0x07902db0	/* pmdmxvi4ger8pp a4,vs0,vs1,11,0,45 */
	.long 0xee000910
	.long 0x079000b1	/* pmdmxvi4ger8pp a4,vs0,vs1,11,1,0 */
	.long 0xee000910
	.long 0x07902db1	/* pmdmxvi4ger8pp a4,vs0,vs1,11,1,45 */
	.long 0xee000910
	.long 0x07900000	/* pmdmxvi8ger4 a4,vs0,vs1,0,0,0 */
	.long 0xee000818
	.long 0x07905000	/* pmdmxvi8ger4 a4,vs0,vs1,0,0,5 */
	.long 0xee000818
	.long 0x0790000d	/* pmdmxvi8ger4 a4,vs0,vs1,0,13,0 */
	.long 0xee000818
	.long 0x0790500d	/* pmdmxvi8ger4 a4,vs0,vs1,0,13,5 */
	.long 0xee000818
	.long 0x079000b0	/* pmdmxvi8ger4 a4,vs0,vs1,11,0,0 */
	.long 0xee000818
	.long 0x079050b0	/* pmdmxvi8ger4 a4,vs0,vs1,11,0,5 */
	.long 0xee000818
	.long 0x079000bd	/* pmdmxvi8ger4 a4,vs0,vs1,11,13,0 */
	.long 0xee000818
	.long 0x079050bd	/* pmdmxvi8ger4 a4,vs0,vs1,11,13,5 */
	.long 0xee000818
	.long 0x07900000	/* pmdmxvi8ger4pp a4,vs0,vs1,0,0,0 */
	.long 0xee000810
	.long 0x07905000	/* pmdmxvi8ger4pp a4,vs0,vs1,0,0,5 */
	.long 0xee000810
	.long 0x0790000d	/* pmdmxvi8ger4pp a4,vs0,vs1,0,13,0 */
	.long 0xee000810
	.long 0x0790500d	/* pmdmxvi8ger4pp a4,vs0,vs1,0,13,5 */
	.long 0xee000810
	.long 0x079000b0	/* pmdmxvi8ger4pp a4,vs0,vs1,11,0,0 */
	.long 0xee000810
	.long 0x079050b0	/* pmdmxvi8ger4pp a4,vs0,vs1,11,0,5 */
	.long 0xee000810
	.long 0x079000bd	/* pmdmxvi8ger4pp a4,vs0,vs1,11,13,0 */
	.long 0xee000810
	.long 0x079050bd	/* pmdmxvi8ger4pp a4,vs0,vs1,11,13,5 */
	.long 0xee000810
	.long 0x07900000	/* pmdmxvi8ger4spp a4,vs0,vs1,0,0,0 */
	.long 0xee000b18
	.long 0x07905000	/* pmdmxvi8ger4spp a4,vs0,vs1,0,0,5 */
	.long 0xee000b18
	.long 0x0790000d	/* pmdmxvi8ger4spp a4,vs0,vs1,0,13,0 */
	.long 0xee000b18
	.long 0x0790500d	/* pmdmxvi8ger4spp a4,vs0,vs1,0,13,5 */
	.long 0xee000b18
	.long 0x079000b0	/* pmdmxvi8ger4spp a4,vs0,vs1,11,0,0 */
	.long 0xee000b18
	.long 0x079050b0	/* pmdmxvi8ger4spp a4,vs0,vs1,11,0,5 */
	.long 0xee000b18
	.long 0x079000bd	/* pmdmxvi8ger4spp a4,vs0,vs1,11,13,0 */
	.long 0xee000b18
	.long 0x079050bd	/* pmdmxvi8ger4spp a4,vs0,vs1,11,13,5 */
	.long 0xee000b18
	.long 0x06000000	/* pstb r0,0(r1) */
	.long 0x98010000
	.long 0x06000000	/* pstb r0,16(r1) */
	.long 0x98010010
	.long 0x06000000	/* pstb r0,32(r1) */
	.long 0x98010020
	.long 0x06000000	/* pstb r0,8(r1) */
	.long 0x98010008
	.long 0x04000000	/* pstd r0,0(r1) */
	.long 0xf4010000
	.long 0x04000000	/* pstd r0,16(r1) */
	.long 0xf4010010
	.long 0x04000000	/* pstd r0,32(r1) */
	.long 0xf4010020
	.long 0x04000000	/* pstd r0,8(r1) */
	.long 0xf4010008
	.long 0x06000000	/* pstfd f26,0(0) */
	.long 0xdb400000
	.long 0x06000000	/* pstfd f26,16(0) */
	.long 0xdb400010
	.long 0x06000000	/* pstfd f26,32(0) */
	.long 0xdb400020
	.long 0x06000000	/* pstfd f26,4(0) */
	.long 0xdb400004
	.long 0x06000000	/* pstfd f26,8(0) */
	.long 0xdb400008
	.long 0x06000000	/* pstfs f26,0(0) */
	.long 0xd3400000
	.long 0x06000000	/* pstfs f26,16(0) */
	.long 0xd3400010
	.long 0x06000000	/* pstfs f26,32(0) */
	.long 0xd3400020
	.long 0x06000000	/* pstfs f26,4(0) */
	.long 0xd3400004
	.long 0x06000000	/* pstfs f26,8(0) */
	.long 0xd3400008
	.long 0x06000000	/* psth r0,0(r1) */
	.long 0xb0010000
	.long 0x06000000	/* psth r0,16(r1) */
	.long 0xb0010010
	.long 0x06000000	/* psth r0,32(r1) */
	.long 0xb0010020
	.long 0x06000000	/* psth r0,8(r1) */
	.long 0xb0010008
	.long 0x04000000	/* pstq r24,0(0) */
	.long 0xf3000000
	.long 0x04000000	/* pstq r24,16(0) */
	.long 0xf3000010
	.long 0x04000000	/* pstq r24,32(0) */
	.long 0xf3000020
	.long 0x04000000	/* pstq r24,64(0) */
	.long 0xf3000040
	.long 0x04000000	/* pstq r24,8(0) */
	.long 0xf3000008
	.long 0x06000000	/* pstw r0,0(r1) */
	.long 0x90010000
	.long 0x06000000	/* pstw r0,16(r1) */
	.long 0x90010010
	.long 0x06000000	/* pstw r0,32(r1) */
	.long 0x90010020
	.long 0x06000000	/* pstw r0,8(r1) */
	.long 0x90010008
	.long 0x04000000	/* pstxsd v22,0(0) */
	.long 0xbac00000
	.long 0x04000000	/* pstxsd v22,16(0) */
	.long 0xbac00010
	.long 0x04000000	/* pstxsd v22,32(0) */
	.long 0xbac00020
	.long 0x04000000	/* pstxsd v22,4(0) */
	.long 0xbac00004
	.long 0x04000000	/* pstxsd v22,64(0) */
	.long 0xbac00040
	.long 0x04000000	/* pstxsd v22,8(0) */
	.long 0xbac00008
	.long 0x04000000	/* pstxssp v22,0(0) */
	.long 0xbec00000
	.long 0x04000000	/* pstxssp v22,16(0) */
	.long 0xbec00010
	.long 0x04000000	/* pstxssp v22,32(0) */
	.long 0xbec00020
	.long 0x04000000	/* pstxssp v22,4(0) */
	.long 0xbec00004
	.long 0x04000000	/* pstxssp v22,64(0) */
	.long 0xbec00040
	.long 0x04000000	/* pstxssp v22,8(0) */
	.long 0xbec00008
	.long 0x04000000	/* pstxvp vs20,0(0) */
	.long 0xfa800000
	.long 0x04000000	/* pstxvp vs20,16(0) */
	.long 0xfa800010
	.long 0x04000000	/* pstxvp vs20,32(0) */
	.long 0xfa800020
	.long 0x04000000	/* pstxvp vs20,48(0) */
	.long 0xfa800030
	.long 0x04000000	/* pstxv vs0,0(r1) */
	.long 0xd8010000
	.long 0x04000000	/* pstxv vs0,16(r1) */
	.long 0xd8010010
	.long 0x04000000	/* pstxv vs0,4(r1) */
	.long 0xd8010004
	.long 0x04000000	/* pstxv vs0,8(r1) */
	.long 0xd8010008
	.long 0x05000000	/* xxblendvb vs0,vs1,vs2,vs3 */
	.long 0x840110c0
	.long 0x05000000	/* xxblendvd vs0,vs1,vs2,vs3 */
	.long 0x840110f0
	.long 0x05000000	/* xxblendvh vs0,vs1,vs2,vs3 */
	.long 0x840110d0
	.long 0x05000000	/* xxblendvw vs0,vs1,vs2,vs3 */
	.long 0x840110e0
	.long 0x05000000	/* xxeval vs0,vs1,vs2,vs3,0 */
	.long 0x880110d0
	.long 0x05000003	/* xxeval vs0,vs1,vs2,vs3,3 */
	.long 0x880110d0
	.long 0x05000000	/* xxpermx vs0,vs1,vs2,vs3,0 */
	.long 0x880110c0
	.long 0x05000003	/* xxpermx vs0,vs1,vs2,vs3,3 */
	.long 0x880110c0
	.long 0x05000000	/* xxsplti32dx vs0,0,127 */
	.long 0x8000007f
	.long 0x05000000	/* xxsplti32dx vs0,0,15 */
	.long 0x8000000f
	.long 0x0500a5a5	/* xxsplti32dx vs0,0,2779096485 */
	.long 0x8000a5a5
	.long 0x05000000	/* xxsplti32dx vs0,0,3 */
	.long 0x80000003
	.long 0x05000000	/* xxsplti32dx vs0,0,31 */
	.long 0x8000001f
	.long 0x05000000	/* xxsplti32dx vs0,0,32768 */
	.long 0x80008000
	.long 0x0500ffff	/* xxsplti32dx vs0,0,4294967295 */
	.long 0x8000ffff
	.long 0x05000000	/* xxsplti32dx vs0,0,63 */
	.long 0x8000003f
	.long 0x05000001	/* xxsplti32dx vs0,0,66535 */
	.long 0x800003e7
	.long 0x05000000	/* xxsplti32dx vs0,0,7 */
	.long 0x80000007
	.long 0x05000000	/* xxsplti32dx vs0,1,127 */
	.long 0x8002007f
	.long 0x05000000	/* xxsplti32dx vs0,1,15 */
	.long 0x8002000f
	.long 0x0500a5a5	/* xxsplti32dx vs0,1,2779096485 */
	.long 0x8002a5a5
	.long 0x05000000	/* xxsplti32dx vs0,1,3 */
	.long 0x80020003
	.long 0x05000000	/* xxsplti32dx vs0,1,31 */
	.long 0x8002001f
	.long 0x05000000	/* xxsplti32dx vs0,1,32768 */
	.long 0x80028000
	.long 0x0500ffff	/* xxsplti32dx vs0,1,4294967295 */
	.long 0x8002ffff
	.long 0x05000000	/* xxsplti32dx vs0,1,63 */
	.long 0x8002003f
	.long 0x05000001	/* xxsplti32dx vs0,1,66535 */
	.long 0x800203e7
	.long 0x05000000	/* xxsplti32dx vs0,1,7 */
	.long 0x80020007
	.long 0x05000000	/* xxspltidp vs0,0 */
	.long 0x80040000
	.long 0x05000080	/* xxspltidp vs0,8388608 */
	.long 0x80040000
	.long 0x05000080	/* xxspltidp vs0,8388609 */
	.long 0x80040001
	.long 0x05000083	/* xxspltidp vs0,8594245 */
	.long 0x80042345
	.long 0x050000ff	/* xxspltidp vs0,16777215 */
	.long 0x8004ffff
	.long 0x05003200	/* xxspltidp vs0,838860800 */
	.long 0x80040000
	.long 0x05007f80	/* xxspltidp vs0,2139095040 */
	.long 0x80040000
	.long 0x05007f80	/* xxspltidp vs0,2139095041 */
	.long 0x80040001
	.long 0x05007f83	/* xxspltidp vs0,2139300677 */
	.long 0x80042345
	.long 0x05007fff	/* xxspltidp vs0,2147483647 */
	.long 0x8004ffff
	.long 0x05008000	/* xxspltidp vs0,2147483648 */
	.long 0x80040000
	.long 0x05008080	/* xxspltidp vs0,2155872256 */
	.long 0x80040000
	.long 0x05008080	/* xxspltidp vs0,2155872257 */
	.long 0x80040001
	.long 0x05008083	/* xxspltidp vs0,2156077893 */
	.long 0x80042345
	.long 0x050080ff	/* xxspltidp vs0,2164260863 */
	.long 0x8004ffff
	.long 0x0500ff80	/* xxspltidp vs0,4286578688 */
	.long 0x80040000
	.long 0x0500ff80	/* xxspltidp vs0,4286578689 */
	.long 0x80040001
	.long 0x0500ff83	/* xxspltidp vs0,4286784325 */
	.long 0x80042345
	.long 0x0500ffff	/* xxspltidp vs0,4294967295 */
	.long 0x8004ffff
	.long 0x05000000	/* xxspltiw vs0,0 */
	.long 0x80060000
	.long 0x05000000	/* xxspltiw vs0,0 */
	.long 0x80060001
	.long 0x05000000	/* xxspltiw vs0,3 */
	.long 0x80060003
	.long 0x05000000	/* xxspltiw vs0,8 */
	.long 0x80060008
	.section	.note.GNU-stack,"",@progbits
