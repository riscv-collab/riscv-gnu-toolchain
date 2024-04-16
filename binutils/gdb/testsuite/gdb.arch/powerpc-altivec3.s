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
	.long  0x117e0001    /* vmul10cuq v11,v30          */
	.long  0x13c1b807    /* vcmpneb v30,v1,v23         */
	.long  0x13d3f77b    /* vpermr  v30,v19,v30,v29    */
	.long  0x12948841    /* vmul10ecuq v20,v20,v17     */
	.long  0x1373f847    /* vcmpneh v27,v19,v31        */
	.long  0x10c9b885    /* vrlwmi  v6,v9,v23          */
	.long  0x12da0887    /* vcmpnew v22,v26,v1         */
	.long  0x131ec8c5    /* vrldmi  v24,v30,v25        */
	.long  0x127db107    /* vcmpnezb v19,v29,v22       */
	.long  0x11179947    /* vcmpnezh v8,v23,v19        */
	.long  0x13785985    /* vrlwnm  v27,v24,v11        */
	.long  0x12ad5187    /* vcmpnezw v21,v13,v10       */
	.long  0x10b4e9c5    /* vrldnm  v5,v20,v29         */
	.long  0x13d30201    /* vmul10uq v30,v19           */
	.long  0x130caa0d    /* vextractub v24,v21,12      */
	.long  0x1013e241    /* vmul10euq v0,v19,v28       */
	.long  0x114c1a4d    /* vextractuh v10,v3,12       */
	.long  0x1387628d    /* vextractuw v28,v12,7       */
	.long  0x13c1dacd    /* vextractd v30,v27,1        */
	.long  0x1324fb0d    /* vinsertb v25,v31,4         */
	.long  0x12aef341    /* bcdcpsgn. v21,v14,v30      */
	.long  0x12c5934d    /* vinserth v22,v18,5         */
	.long  0x13a1b38d    /* vinsertw v29,v22,1         */
	.long  0x13a76bcd    /* vinsertd v29,v13,7         */
	.long  0x12d94407    /* vcmpneb. v22,v25,v8        */
	.long  0x120fac47    /* vcmpneh. v16,v15,v21       */
	.long  0x12d5fc81    /* bcdus.  v22,v21,v31        */
	.long  0x102c6487    /* vcmpnew. v1,v12,v12        */
	.long  0x10a346c1    /* bcds.   v5,v3,v8,1         */
	.long  0x13760d01    /* bcdtrunc. v27,v22,v1,0     */
	.long  0x105a0507    /* vcmpnezb. v2,v26,v0        */
	.long  0x134e3d41    /* bcdutrunc. v26,v14,v7      */
	.long  0x12056547    /* vcmpnezh. v16,v5,v12       */
	.long  0x13002d81    /* bcdctsq. v24,v5            */
	.long  0x10e20581    /* bcdcfsq. v7,v0,0           */
	.long  0x13c46781    /* bcdctz. v30,v12,1          */
	.long  0x1225bd81    /* bcdctn. v17,v23            */
	.long  0x10867f81    /* bcdcfz. v4,v15,1           */
	.long  0x13a72f81    /* bcdcfn. v29,v5,1           */
	.long  0x137f6581    /* bcdsetsgn. v27,v12,0       */
	.long  0x11dccd87    /* vcmpnezw. v14,v28,v25      */
	.long  0x104237c1    /* bcdsr.  v2,v2,v6,1         */
	.long  0x13202dcc    /* vbpermd v25,v0,v5          */
	.long  0x1380ce02    /* vclzlsbb r28,v25           */
	.long  0x1041c602    /* vctzlsbb r2,v24            */
	.long  0x12a65e02    /* vnegw   v21,v11            */
	.long  0x1227de02    /* vnegd   v17,v27            */
	.long  0x13e8be02    /* vprtybw v31,v23            */
	.long  0x12a9be02    /* vprtybd v21,v23            */
	.long  0x12aa9602    /* vprtybq v21,v18            */
	.long  0x13d02602    /* vextsb2w v30,v4            */
	.long  0x1071d602    /* vextsh2w v3,v26            */
	.long  0x11788e02    /* vextsb2d v11,v17           */
	.long  0x10b95602    /* vextsh2d v5,v10            */
	.long  0x11bace02    /* vextsw2d v13,v25           */
	.long  0x133c1602    /* vctzb   v25,v2             */
	.long  0x101d1e02    /* vctzh   v0,v3              */
	.long  0x12de3602    /* vctzw   v22,v6             */
	.long  0x135fc602    /* vctzd   v26,v24            */
	.long  0x10df160d    /* vextublx r6,r31,v2         */
	.long  0x11a0964d    /* vextuhlx r13,r0,v18        */
	.long  0x11defe8d    /* vextuwlx r14,r30,v31       */
	.long  0x11ec7704    /* vsrv    v15,v12,v14        */
	.long  0x128af70d    /* vextubrx r20,r10,v30       */
	.long  0x12b51744    /* vslv    v21,v21,v2         */
	.long  0x11e90f4d    /* vextuhrx r15,r9,v1         */
	.long  0x12b1878d    /* vextuwrx r21,r17,v16       */
	.long  0x1295b5e3    /* vmsumudm v20,v21,v22,v23   */
	.section	.note.GNU-stack,"",@progbits
