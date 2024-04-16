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
	.long  0x7fced019    /* lxsiwzx vs62,r14,r26       */
	.long  0x7d00c819    /* lxsiwzx vs40,0,r25         */
	.long  0x7f20d098    /* lxsiwax vs25,0,r26         */
	.long  0x7c601898    /* lxsiwax vs3,0,r3           */
	.long  0x7fcc0066    /* mfvsrd  r12,vs30           */
	.long  0x7fcc0067    /* mfvsrd  r12,vs62           */
	.long  0x7d9400e6    /* mffprwz r20,f12            */
	.long  0x7d9500e7    /* mfvrwz  r21,v12            */
	.long  0x7dc97118    /* stxsiwx vs14,r9,r14        */
	.long  0x7ea04118    /* stxsiwx vs21,0,r8          */
	.long  0x7d7c0166    /* mtvsrd  vs11,r28           */
	.long  0x7d7d0167    /* mtvsrd  vs43,r29           */
	.long  0x7f1601a6    /* mtfprwa f24,r22            */
	.long  0x7f3701a7    /* mtvrwa  v25,r23            */
	.long  0x7f5b01e6    /* mtfprwz f26,r27            */
	.long  0x7f7c01e7    /* mtvrwz  v27,r28            */
	.long  0x7db36c18    /* lxsspx  vs13,r19,r13       */
	.long  0x7e406c18    /* lxsspx  vs18,0,r13         */
	.long  0x7d622519    /* stxsspx vs43,r2,r4         */
	.long  0x7ee05d19    /* stxsspx vs55,0,r11         */
	.long  0xf2d0c805    /* xsaddsp vs54,vs48,vs25     */
	.long  0xf1d2080c    /* xsmaddasp vs14,vs50,vs1    */
	.long  0xf3565042    /* xssubsp vs26,vs22,vs42     */
	.long  0xf375a04e    /* xsmaddmsp vs27,vs53,vs52   */
	.long  0xf100d82a    /* xsrsqrtesp vs8,vs59        */
	.long  0xf180482e    /* xssqrtsp vs12,vs41         */
	.long  0xf32b0083    /* xsmulsp vs57,vs11,vs32     */
	.long  0xf0d4d089    /* xsmsubasp vs38,vs20,vs26   */
	.long  0xf35330c0    /* xsdivsp vs26,vs19,vs6      */
	.long  0xf065b8cf    /* xsmsubmsp vs35,vs37,vs55   */
	.long  0xf3604069    /* xsresp  vs59,vs8           */
	.long  0xf1810c0f    /* xsnmaddasp vs44,vs33,vs33  */
	.long  0xf23ef44c    /* xsnmaddmsp vs17,vs62,vs30  */
	.long  0xf2d4fc8d    /* xsnmsubasp vs54,vs52,vs31  */
	.long  0xf0a5d4cb    /* xsnmsubmsp vs37,vs5,vs58   */
	.long  0xf3d66556    /* xxlorc  vs30,vs54,vs44     */
	.long  0xf22eed91    /* xxlnand vs49,vs14,vs29     */
	.long  0xf3d6f5d1    /* xxleqv  vs62,vs22,vs30     */
	.long  0xf380b42f    /* xscvdpspn vs60,vs54        */
	.long  0xf2c06c66    /* xsrsp   vs22,vs45          */
	.long  0xf340dca2    /* xscvuxdsp vs26,vs59        */
	.long  0xf0c08ce3    /* xscvsxdsp vs38,vs49        */
	.long  0xf360d52d    /* xscvspdpn vs59,vs26        */
	.long  0xff0e168c    /* fmrgow  f24,f14,f2         */
	.long  0xfec72f8c    /* fmrgew  f22,f7,f5          */
	.section	.note.GNU-stack,"",@progbits
