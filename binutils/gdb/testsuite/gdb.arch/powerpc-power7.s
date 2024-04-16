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
	.long  0x7c642e98    /* lxvd2x  vs3,r4,r5          */
	.long  0x7d642e99    /* lxvd2x  vs43,r4,r5         */
	.long  0x7c642f98    /* stxvd2x vs3,r4,r5          */
	.long  0x7d642f99    /* stxvd2x vs43,r4,r5         */
	.long  0xf0642850    /* xxmrghd vs3,vs4,vs5        */
	.long  0xf16c6857    /* xxmrghd vs43,vs44,vs45     */
	.long  0xf0642b50    /* xxmrgld vs3,vs4,vs5        */
	.long  0xf16c6b57    /* xxmrgld vs43,vs44,vs45     */
	.long  0xf0642950    /* xxpermdi vs3,vs4,vs5,1     */
	.long  0xf16c6957    /* xxpermdi vs43,vs44,vs45,1  */
	.long  0xf0642a50    /* xxpermdi vs3,vs4,vs5,2     */
	.long  0xf16c6a57    /* xxpermdi vs43,vs44,vs45,2  */
	.long  0xf0642780    /* xvmovdp vs3,vs4            */
	.long  0xf16c6787    /* xvmovdp vs43,vs44          */
	.long  0xf0642f80    /* xvcpsgndp vs3,vs4,vs5      */
	.long  0xf16c6f87    /* xvcpsgndp vs43,vs44,vs45   */
	.long  0x4c000324    /* doze                       */
	.long  0x4c000364    /* nap                        */
	.long  0x4c0003a4    /* sleep                      */
	.long  0x4c0003e4    /* rvwinkle                   */
	.long  0x7c830134    /* prtyw   r3,r4              */
	.long  0x7dcd0174    /* prtyd   r13,r14            */
	.long  0x7d5c02a6    /* mfcfar  r10                */
	.long  0x7d7c03a6    /* mtcfar  r11                */
	.long  0x7c832bf8    /* cmpb    r3,r4,r5           */
	.long  0x7d4b662a    /* lwzcix  r10,r11,r12        */
	.long  0xee119004    /* dadd    f16,f17,f18        */
	.long  0xfe96c004    /* daddq   f20,f22,f24        */
	.long  0x7c60066c    /* dss     3                  */
	.long  0x7e00066c    /* dssall                     */
	.long  0x7c2522ac    /* dst     r5,r4,1            */
	.long  0x7e083aac    /* dstt    r8,r7,0            */
	.long  0x7c6532ec    /* dstst   r5,r6,3            */
	.long  0x7e442aec    /* dststt  r4,r5,2            */
	.long  0x7d4b6356    /* divwe   r10,r11,r12        */
	.long  0x7d6c6b57    /* divwe.  r11,r12,r13        */
	.long  0x7d8d7756    /* divweo  r12,r13,r14        */
	.long  0x7dae7f57    /* divweo. r13,r14,r15        */
	.long  0x7d4b6316    /* divweu  r10,r11,r12        */
	.long  0x7d6c6b17    /* divweu. r11,r12,r13        */
	.long  0x7d8d7716    /* divweuo r12,r13,r14        */
	.long  0x7dae7f17    /* divweuo. r13,r14,r15       */
	.long  0x7e27d9f8    /* bpermd  r7,r17,r27         */
	.long  0x7e8a02f4    /* popcntw r10,r20            */
	.long  0x7e8a03f4    /* popcntd r10,r20            */
	.long  0x7e95b428    /* ldbrx   r20,r21,r22        */
	.long  0x7e95b528    /* stdbrx  r20,r21,r22        */
	.long  0x7d4056ee    /* lfiwzx  f10,0,r10          */
	.long  0x7d4956ee    /* lfiwzx  f10,r9,r10         */
	.long  0xec802e9c    /* fcfids  f4,f5              */
	.long  0xec802e9d    /* fcfids. f4,f5              */
	.long  0xec802f9c    /* fcfidus f4,f5              */
	.long  0xec802f9d    /* fcfidus. f4,f5             */
	.long  0xfc80291c    /* fctiwu  f4,f5              */
	.long  0xfc80291d    /* fctiwu. f4,f5              */
	.long  0xfc80291e    /* fctiwuz f4,f5              */
	.long  0xfc80291f    /* fctiwuz. f4,f5             */
	.long  0xfc802f5c    /* fctidu  f4,f5              */
	.long  0xfc802f5d    /* fctidu. f4,f5              */
	.long  0xfc802f5e    /* fctiduz f4,f5              */
	.long  0xfc802f5f    /* fctiduz. f4,f5             */
	.long  0xfc802f9c    /* fcfidu  f4,f5              */
	.long  0xfc802f9d    /* fcfidu. f4,f5              */
	.long  0xfc0a5900    /* ftdiv   cr0,f10,f11        */
	.long  0xff8a5900    /* ftdiv   cr7,f10,f11        */
	.long  0xfc005140    /* ftsqrt  cr0,f10            */
	.long  0xff805140    /* ftsqrt  cr7,f10            */
	.long  0x7e084a2c    /* dcbtt   r8,r9              */
	.long  0x7e0849ec    /* dcbtstt r8,r9              */
	.long  0xed406644    /* dcffix  f10,f12            */
	.long  0xee80b645    /* dcffix. f20,f22            */
	.long  0xfdc07830    /* fre     f14,f15            */
	.long  0xfdc07831    /* fre.    f14,f15            */
	.long  0xedc07830    /* fres    f14,f15            */
	.long  0xedc07831    /* fres.   f14,f15            */
	.long  0xfdc07834    /* frsqrte f14,f15            */
	.long  0xfdc07835    /* frsqrte. f14,f15           */
	.long  0xedc07834    /* frsqrtes f14,f15           */
	.long  0xedc07835    /* frsqrtes. f14,f15          */
	.long  0x7c43271e    /* isel    r2,r3,r4,28        */
	.long  0x7f7bdb78    /* yield                      */
	.long  0x60420000    /* ori     r2,r2,0            */
	.long  0x60000000    /* nop                        */
	.long  0x7fbdeb78    /* mdoio                      */
	.long  0x7fdef378    /* mdoom                      */
	.long  0x7d40e2a6    /* mfppr   r10                */
	.long  0x7d62e2a6    /* mfppr32 r11                */
	.long  0x7d80e3a6    /* mtppr   r12                */
	.long  0x7da2e3a6    /* mtppr32 r13                */
	.long  0x7d605264    /* tlbie   r10,r11            */
	.section	.note.GNU-stack,"",@progbits
