/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

	.text
	.globl func
func:
	.long  0x7c60e20e    /* lvepxl  v3,0,r28           */
	.long  0x7e64920e    /* lvepxl  v19,r4,r18         */
	.long  0x7f609a4e    /* lvepx   v27,0,r19          */
	.long  0x7c39924e    /* lvepx   v1,r25,r18         */
	.long  0x7fe0da0a    /* lvexbx  v31,0,r27          */
	.long  0x7f81620a    /* lvexbx  v28,r1,r12         */
	.long  0x7fe0724a    /* lvexhx  v31,0,r14          */
	.long  0x7e30fa4a    /* lvexhx  v17,r16,r31        */
	.long  0x7ec0ea8a    /* lvexwx  v22,0,r29          */
	.long  0x7ef92a8a    /* lvexwx  v23,r25,r5         */
	.long  0x7c60660a    /* lvsm    v3,0,r12           */
	.long  0x7f7d0e0a    /* lvsm    v27,r29,r1         */
	.long  0x7ce036ca    /* lvswxl  v7,0,r6            */
	.long  0x7cf046ca    /* lvswxl  v7,r16,r8          */
	.long  0x7dc094ca    /* lvswx   v14,0,r18          */
	.long  0x7f9c84ca    /* lvswx   v28,r28,r16        */
	.long  0x7f60668a    /* lvtlxl  v27,0,r12          */
	.long  0x7f7c068a    /* lvtlxl  v27,r28,r0         */
	.long  0x7ee0cc8a    /* lvtlx   v23,0,r25          */
	.long  0x7c39748a    /* lvtlx   v1,r25,r14         */
	.long  0x7e80c64a    /* lvtrxl  v20,0,r24          */
	.long  0x7eddc64a    /* lvtrxl  v22,r29,r24        */
	.long  0x7f00444a    /* lvtrx   v24,0,r8           */
	.long  0x7db7e44a    /* lvtrx   v13,r23,r28        */
	.long  0x7d9c60dc    /* mvidsplt v12,r28,r12       */
	.long  0x7d5b005c    /* mviwsplt v10,r27,r0        */
	.long  0x7f606e0e    /* stvepxl v27,0,r13          */
	.long  0x7c42fe0e    /* stvepxl v2,r2,r31          */
	.long  0x7c60564e    /* stvepx  v3,0,r10           */
	.long  0x7f7c064e    /* stvepx  v27,r28,r0         */
	.long  0x7da0330a    /* stvexbx v13,0,r6           */
	.long  0x7db91b0a    /* stvexbx v13,r25,r3         */
	.long  0x7ec00b4a    /* stvexhx v22,0,r1           */
	.long  0x7e2e534a    /* stvexhx v17,r14,r10        */
	.long  0x7ea0db8a    /* stvexwx v21,0,r27          */
	.long  0x7ff20b8a    /* stvexwx v31,r18,r1         */
	.long  0x7f406f8a    /* stvflxl v26,0,r13          */
	.long  0x7ecdaf8a    /* stvflxl v22,r13,r21        */
	.long  0x7ca04d8a    /* stvflx  v5,0,r9            */
	.long  0x7eb80d8a    /* stvflx  v21,r24,r1         */
	.long  0x7da0574a    /* stvfrxl v13,0,r10          */
	.long  0x7db1cf4a    /* stvfrxl v13,r17,r25        */
	.long  0x7e20554a    /* stvfrx  v17,0,r10          */
	.long  0x7d0cfd4a    /* stvfrx  v8,r12,r31         */
	.long  0x7e40efca    /* stvswxl v18,0,r29          */
	.long  0x7f4e47ca    /* stvswxl v26,r14,r8         */
	.long  0x7c007dca    /* stvswx  v0,0,r15           */
	.long  0x7db73dca    /* stvswx  v13,r23,r7         */
	.long  0x10d18403    /* vabsdub v6,v17,v16         */
	.long  0x12b22443    /* vabsduh v21,v18,v4         */
	.long  0x13344c83    /* vabsduw v25,v20,v9         */
	.long  0x10d1a6ad    /* vpermxor v6,v17,v20,v26    */
	.long  0x13ba7f3c    /* vaddeuqm v29,v26,v15,v28   */
	.long  0x11e83e3d    /* vaddecuq v15,v8,v7,v24     */
	.long  0x1046a87e    /* vsubeuqm v2,v6,v21,v1      */
	.long  0x13a6013f    /* vsubecuq v29,v6,v0,v4      */
	.long  0x11c91888    /* vmulouw v14,v9,v3          */
	.long  0x13109089    /* vmuluwm v24,v16,v18        */
	.long  0x115188c0    /* vaddudm v10,v17,v17        */
	.long  0x13d920c2    /* vmaxud  v30,v25,v4         */
	.long  0x1146e0c4    /* vrld    v10,v6,v28         */
	.long  0x136738c7    /* vcmpequd v27,v7,v7         */
	.long  0x12d0c900    /* vadduqm v22,v16,v25        */
	.long  0x1035e940    /* vaddcuq v1,v21,v29         */
	.long  0x128b9988    /* vmulosw v20,v11,v19        */
	.long  0x131309c2    /* vmaxsd  v24,v19,v1         */
	.long  0x11bbf288    /* vmuleuw v13,v27,v30        */
	.long  0x11388ac2    /* vminud  v9,v24,v17         */
	.long  0x1152e2c7    /* vcmpgtud v10,v18,v28       */
	.long  0x101db388    /* vmulesw v0,v29,v22         */
	.long  0x11bc0bc2    /* vminsd  v13,v28,v1         */
	.long  0x11542bc4    /* vsrad   v10,v20,v5         */
	.long  0x13752bc7    /* vcmpgtsd v27,v21,v5        */
	.long  0x1017f601    /* bcdadd. v0,v23,v30,1       */
	.long  0x1338d408    /* vpmsumb v25,v24,v26        */
	.long  0x11042641    /* bcdsub. v8,v4,v4,1         */
	.long  0x120ed448    /* vpmsumh v16,v14,v26        */
	.long  0x1362d44e    /* vpkudum v27,v2,v26         */
	.long  0x10d78c88    /* vpmsumw v6,v23,v17         */
	.long  0x1286ccc8    /* vpmsumd v20,v6,v25         */
	.long  0x137684ce    /* vpkudus v27,v22,v16        */
	.long  0x12b494c0    /* vsubudm v21,v20,v18        */
	.long  0x12b49500    /* vsubuqm v21,v20,v18        */
	.long  0x13bd3508    /* vcipher v29,v29,v6         */
	.long  0x104da509    /* vcipherlast v2,v13,v20     */
	.long  0x1280950c    /* vgbbd   v20,v18            */
	.long  0x1268cd40    /* vsubcuq v19,v8,v25         */
	.long  0x113aed44    /* vorc    v9,v26,v29         */
	.long  0x12946d48    /* vncipher v20,v20,v13       */
	.long  0x11e5dd49    /* vncipherlast v15,v5,v27    */
	.long  0x1073354c    /* vbpermq v3,v19,v6          */
	.long  0x13c4e54e    /* vpksdus v30,v4,v28         */
	.long  0x10047584    /* vnand   v0,v4,v14          */
	.long  0x1228edc4    /* vsld    v17,v8,v29         */
	.long  0x13b405c8    /* vsbox   v29,v20            */
	.long  0x11675dce    /* vpksdss v11,v7,v11         */
	.long  0x107384c7    /* vcmpequd. v3,v19,v16       */
	.long  0x12408e4e    /* vupkhsw v18,v17            */
	.long  0x13a86e82    /* vshasigmaw v29,v8,0,13     */
	.long  0x12fcd684    /* veqv    v23,v28,v26        */
	.long  0x13a0178c    /* vmrgew  v29,v0,v2          */
	.long  0x13a0168c    /* vmrgow  v29,v0,v2          */
	.long  0x137306c2    /* vshasigmad v27,v19,0,0     */
	.long  0x129ce6c4    /* vsrd    v20,v28,v28        */
	.long  0x1240aece    /* vupklsw v18,v21            */
	.long  0x13c03f02    /* vclzb   v30,v7             */
	.long  0x13a0af03    /* vpopcntb v29,v21           */
	.long  0x1320af42    /* vclzh   v25,v21            */
	.long  0x1200f743    /* vpopcnth v16,v30           */
	.long  0x13801f82    /* vclzw   v28,v3             */
	.long  0x11404f83    /* vpopcntw v10,v9            */
	.long  0x12c04fc2    /* vclzd   v22,v9             */
	.long  0x11e0f7c3    /* vpopcntd v15,v30           */
	.long  0x105f36c7    /* vcmpgtud. v2,v31,v6        */
	.long  0x128f17c7    /* vcmpgtsd. v20,v15,v2       */
	.section	.note.GNU-stack,"",@progbits
