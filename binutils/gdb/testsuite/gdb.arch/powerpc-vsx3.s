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
	.long  0x7c46ca19    /* lxvx    vs34,r6,r25        */
	.long  0x7e805218    /* lxvx    vs20,0,r10         */
	.long  0x7e98521a    /* lxvl    vs20,r24,r10       */
	.long  0x7ec0ea1b    /* lxvl    vs54,0,r29         */
	.long  0x7f149a5a    /* lxvll   vs24,r20,r19       */
	.long  0x7c40725b    /* lxvll   vs34,0,r14         */
	.long  0x7ec20266    /* mfvsrld r2,vs22            */
	.long  0x7f5acad9    /* lxvwsx  vs58,r26,r25       */
	.long  0x7ee0ead9    /* lxvwsx  vs55,0,r29         */
	.long  0x7dd52318    /* stxvx   vs14,r21,r4        */
	.long  0x7fc0b318    /* stxvx   vs30,0,r22         */
	.long  0x7c1a231a    /* stxvl   vs0,r26,r4         */
	.long  0x7ca0b31b    /* stxvl   vs37,0,r22         */
	.long  0x7f0a0326    /* mtvsrws vs24,r10           */
	.long  0x7fd57b5a    /* stxvll  vs30,r21,r15       */
	.long  0x7ce0735b    /* stxvll  vs39,0,r14         */
	.long  0x7d862b66    /* mtvsrdd vs12,r6,r5         */
	.long  0x7cc0ab67    /* mtvsrdd vs38,0,r21         */
	.long  0x7f7c361b    /* lxsibzx vs59,r28,r6        */
	.long  0x7fc0461a    /* lxsibzx vs30,0,r8          */
	.long  0x7d578e59    /* lxvh8x  vs42,r23,r17       */
	.long  0x7c802e59    /* lxvh8x  vs36,0,r5          */
	.long  0x7d895e5a    /* lxsihzx vs12,r9,r11        */
	.long  0x7e206e5b    /* lxsihzx vs49,0,r13         */
	.long  0x7ca39ed9    /* lxvb16x vs37,r3,r19        */
	.long  0x7c00f6d8    /* lxvb16x vs0,0,r30          */
	.long  0x7c5e371a    /* stxsibx vs2,r30,r6         */
	.long  0x7d806f1a    /* stxsibx vs12,0,r13         */
	.long  0x7e1d4758    /* stxvh8x vs16,r29,r8        */
	.long  0x7ee05759    /* stxvh8x vs55,0,r10         */
	.long  0x7c42bf5b    /* stxsihx vs34,r2,r23        */
	.long  0x7f80bf5b    /* stxsihx vs60,0,r23         */
	.long  0x7eee67d8    /* stxvb16x vs23,r14,r12      */
	.long  0x7e602fd8    /* stxvb16x vs19,0,r5         */
	.long  0xe7000002    /* lxsd    v24,0(0)           */
	.long  0xe5f50012    /* lxsd    v15,16(r21)        */
	.long  0xe4c00003    /* lxssp   v6,0(0)            */
	.long  0xe6e90013    /* lxssp   v23,16(r9)         */
	.long  0xf253081e    /* xscmpeqdp vs18,vs51,vs33   */
	.long  0xf05a105a    /* xscmpgtdp vs2,vs26,vs34    */
	.long  0xf0baa098    /* xscmpgedp vs5,vs26,vs20    */
	.long  0xf18a58d3    /* xxperm  vs44,vs10,vs43     */
	.long  0xf13429d1    /* xxpermr vs41,vs20,vs5      */
	.long  0xf212b9da    /* xscmpexpdp cr4,vs18,vs55   */
	.long  0xf2e32a96    /* xxextractuw vs23,vs37,3    */
	.long  0xf2c75ad1    /* xxspltib vs54,235          */
	.long  0xf1e4f2d4    /* xxinsertw vs15,vs30,4      */
	.long  0xf18b3c00    /* xsmaxcdp vs12,vs11,vs7     */
	.long  0xf019c441    /* xsmincdp vs32,vs25,vs24    */
	.long  0xf3356484    /* xsmaxjdp vs25,vs53,vs12    */
	.long  0xf17f24aa    /* xststdcsp cr2,vs36,127     */
	.long  0xf0156cc3    /* xsminjdp vs32,vs21,vs45    */
	.long  0xf220956e    /* xsxexpdp r17,vs50          */
	.long  0xf0e1456e    /* xsxsigdp r7,vs40           */
	.long  0xf2d0156f    /* xscvhpdp vs54,vs34         */
	.long  0xf351b56f    /* xscvdphp vs58,vs54         */
	.long  0xf07f35aa    /* xststdcdp cr0,vs38,127     */
	.long  0xf31faeef    /* xvtstdcsp vs56,vs53,127    */
	.long  0xf2d4a6c3    /* xviexpsp vs54,vs20,vs52    */
	.long  0xf33cef2d    /* xsiexpdp vs57,r28,r29      */
	.long  0xf020a76c    /* xvxexpdp vs1,vs20          */
	.long  0xf2c1df6f    /* xvxsigdp vs54,vs59         */
	.long  0xf2472f6e    /* xxbrh   vs18,vs37          */
	.long  0xf1c80f6c    /* xvxexpsp vs14,vs1          */
	.long  0xf2896f6d    /* xvxsigsp vs52,vs13         */
	.long  0xf26f2f6c    /* xxbrw   vs19,vs5           */
	.long  0xf277bf6f    /* xxbrd   vs51,vs55          */
	.long  0xf0788f6d    /* xvcvhpsp vs35,vs17         */
	.long  0xf1f96f6e    /* xvcvsphp vs15,vs45         */
	.long  0xf23fff6c    /* xxbrq   vs17,vs31          */
	.long  0xf21f67ec    /* xvtstdcdp vs16,vs12,127    */
	.long  0xf36947c0    /* xviexpdp vs27,vs9,vs8      */
	.long  0xf4800001    /* lxv     vs4,0(0)           */
	.long  0xf5140019    /* lxv     vs40,16(r20)       */
	.long  0xf640000d    /* stxv    vs50,0(0)          */
	.long  0xf5100015    /* stxv    vs8,16(r16)        */
	.long  0xf4600002    /* stxsd   v3,0(0)            */
	.long  0xf6220012    /* stxsd   v17,16(r2)         */
	.long  0xf5a00003    /* stxssp  v13,0(0)           */
	.long  0xf62d0013    /* stxssp  v17,16(r13)        */
	.long  0xfd0a9008    /* xsaddqp v8,v10,v18         */
	.long  0xfca1e809    /* xsaddqpo v5,v1,v29         */
	.long  0xfd80960a    /* xsrqpi  0,v12,v18,3        */
	.long  0xffe1980b    /* xsrqpix 1,v31,v19,0        */
	.long  0xfdc13048    /* xsmulqp v14,v1,v6          */
	.long  0xfe27d849    /* xsmulqpo v17,v7,v27        */
	.long  0xfc80584a    /* xsrqpxp 0,v4,v11,0         */
	.long  0xffb7e0c8    /* xscpsgnqp v29,v23,v28      */
	.long  0xff8dd908    /* xscmpoqp cr7,v13,v27       */
	.long  0xfe953148    /* xscmpexpqp cr5,v21,v6      */
	.long  0xfc532308    /* xsmaddqp v2,v19,v4         */
	.long  0xffc78309    /* xsmaddqpo v30,v7,v16       */
	.long  0xfebe7b48    /* xsmsubqp v21,v30,v15       */
	.long  0xfd91f349    /* xsmsubqpo v12,v17,v30      */
	.long  0xfcde6388    /* xsnmaddqp v6,v30,v12       */
	.long  0xfd966389    /* xsnmaddqpo v12,v22,v12     */
	.long  0xfd5ddbc8    /* xsnmsubqp v10,v29,v27      */
	.long  0xffbd6bc9    /* xsnmsubqpo v29,v29,v13     */
	.long  0xfe7b2408    /* xssubqp v19,v27,v4         */
	.long  0xfda80c09    /* xssubqpo v13,v8,v1         */
	.long  0xfd03dc48    /* xsdivqp v8,v3,v27          */
	.long  0xff14dc49    /* xsdivqpo v24,v20,v27       */
	.long  0xff8e2508    /* xscmpuqp cr7,v14,v4        */
	.long  0xfe7f1588    /* xststdcqp cr4,v2,127       */
	.long  0xffe0b648    /* xsabsqp v31,v22            */
	.long  0xff221e48    /* xsxexpqp v25,v3            */
	.long  0xfd48e648    /* xsnabsqp v10,v28           */
	.long  0xfe70fe48    /* xsnegqp v19,v31            */
	.long  0xfd726e48    /* xsxsigqp v11,v13           */
	.long  0xfdbb7648    /* xssqrtqp v13,v14           */
	.long  0xfc3bde49    /* xssqrtqpo v1,v27           */
	.long  0xfc613e88    /* xscvqpuwz v3,v7            */
	.long  0xfe829688    /* xscvudqp v20,v18           */
	.long  0xffa9ee88    /* xscvqpswz v29,v29          */
	.long  0xfc4ae688    /* xscvsdqp v2,v28            */
	.long  0xfef12688    /* xscvqpudz v23,v4           */
	.long  0xfc74a688    /* xscvqpdp v3,v20            */
	.long  0xfc341e89    /* xscvqpdpo v1,v3            */
	.long  0xfe766688    /* xscvdpqp v19,v12           */
	.long  0xfdb92688    /* xscvqpsdz v13,v4           */
	.long  0xfcf83ec8    /* xsiexpqp v7,v24,v7         */
	.section	.note.GNU-stack,"",@progbits
