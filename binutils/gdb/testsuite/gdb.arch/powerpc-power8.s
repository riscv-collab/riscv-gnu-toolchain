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
	.long  0x7c05071d    /* tabort. r5                 */
	.long  0x7ce8861d    /* tabortwc. 7,r8,r16         */
	.long  0x7e8b565d    /* tabortdc. 20,r11,r10       */
	.long  0x7e2a9e9d    /* tabortwci. 17,r10,-13      */
	.long  0x7fa3dedd    /* tabortdci. 29,r3,-5        */
	.long  0x7c00051d    /* tbegin.                    */
	.long  0x7f80059c    /* tcheck  cr7                */
	.long  0x7c00055d    /* tend.                      */
	.long  0x7e00055d    /* tendall.                   */
	.long  0x7c18075d    /* treclaim. r24              */
	.long  0x7c0007dd    /* trechkpt.                  */
	.long  0x7c0005dd    /* tsuspend.                  */
	.long  0x7c2005dd    /* tresume.                   */
	.long  0x60420000    /* ori     r2,r2,0            */
	.long  0x60000000    /* nop                        */
	.long  0x4c000124    /* rfebb   0                  */
	.long  0x4c000924    /* rfebb                      */
	.long  0x4d950460    /* bgttar  cr5                */
	.long  0x4c870461    /* bnstarl cr1                */
	.long  0x4dec0460    /* blttar+ cr3                */
	.long  0x4ce20461    /* bnetarl+                   */
	.long  0x4c880c60    /* bgetar  cr2,1              */
	.long  0x4c871461    /* bnstarl cr1,2              */
	.long  0x7c00003c    /* waitasec                   */
	.long  0x7c00411c    /* msgsndp r8                 */
	.long  0x7c200126    /* mtsle   1                  */
	.long  0x7c00d95c    /* msgclrp r27                */
	.long  0x7d4a616d    /* stqcx.  r10,r10,r12        */
	.long  0x7f80396d    /* stqcx.  r28,0,r7           */
	.long  0x7f135a28    /* lqarx   r24,r19,r11        */
	.long  0x7ec05a28    /* lqarx   r22,0,r11          */
	.long  0x7e80325c    /* mfbhrbe r20,6              */
	.long  0x7fb18329    /* pbt.    r29,r17,r16        */
	.long  0x7dc03b29    /* pbt.    r14,0,r7           */
	.long  0x7c00035c    /* clrbhrb                    */
	.long  0x116a05ed    /* vpermxor v11,v10,v0,v23    */
	.long  0x1302393c    /* vaddeuqm v24,v2,v7,v4      */
	.long  0x114a40bd    /* vaddecuq v10,v10,v8,v2     */
	.long  0x10af44fe    /* vsubeuqm v5,v15,v8,v19     */
	.long  0x119f877f    /* vsubecuq v12,v31,v16,v29   */
	.long  0x129d6888    /* vmulouw v20,v29,v13        */
	.long  0x13a0d089    /* vmuluwm v29,v0,v26         */
	.long  0x1115e0c0    /* vaddudm v8,v21,v28         */
	.long  0x103a08c2    /* vmaxud  v1,v26,v1          */
	.long  0x128308c4    /* vrld    v20,v3,v1          */
	.long  0x109358c7    /* vcmpequd v4,v19,v11        */
	.long  0x12eef100    /* vadduqm v23,v14,v30        */
	.long  0x11086940    /* vaddcuq v8,v8,v13          */
	.long  0x139b2188    /* vmulosw v28,v27,v4         */
	.long  0x106421c2    /* vmaxsd  v3,v4,v4           */
	.long  0x1013aa88    /* vmuleuw v0,v19,v21         */
	.long  0x13149ac2    /* vminud  v24,v20,v19        */
	.long  0x101c7ac7    /* vcmpgtud v0,v28,v15        */
	.long  0x12a01388    /* vmulesw v21,v0,v2          */
	.long  0x113a4bc2    /* vminsd  v9,v26,v9          */
	.long  0x133d5bc4    /* vsrad   v25,v29,v11        */
	.long  0x117c5bc7    /* vcmpgtsd v11,v28,v11       */
	.long  0x10a8d601    /* bcdadd. v5,v8,v26,1        */
	.long  0x10836408    /* vpmsumb v4,v3,v12          */
	.long  0x135fae41    /* bcdsub. v26,v31,v21,1      */
	.long  0x10b18448    /* vpmsumh v5,v17,v16         */
	.long  0x12f1a44e    /* vpkudum v23,v17,v20        */
	.long  0x1315ec88    /* vpmsumw v24,v21,v29        */
	.long  0x11366cc8    /* vpmsumd v9,v22,v13         */
	.long  0x125394ce    /* vpkudus v18,v19,v18        */
	.long  0x13d0b500    /* vsubuqm v30,v16,v22        */
	.long  0x11cb3d08    /* vcipher v14,v11,v7         */
	.long  0x1142b509    /* vcipherlast v10,v2,v22     */
	.long  0x12e06d0c    /* vgbbd   v23,v13            */
	.long  0x12198540    /* vsubcuq v16,v25,v16        */
	.long  0x13e12d44    /* vorc    v31,v1,v5          */
	.long  0x1091fd48    /* vncipher v4,v17,v31        */
	.long  0x1302dd49    /* vncipherlast v24,v2,v27    */
	.long  0x12f5bd4c    /* vbpermq v23,v21,v23        */
	.long  0x13724d4e    /* vpksdus v27,v18,v9         */
	.long  0x137ddd84    /* vnand   v27,v29,v27        */
	.long  0x1273c5c4    /* vsld    v19,v19,v24        */
	.long  0x10ad05c8    /* vsbox   v5,v13             */
	.long  0x13233dce    /* vpksdss v25,v3,v7          */
	.long  0x138804c7    /* vcmpequd. v28,v8,v0        */
	.long  0x1340d64e    /* vupkhsw v26,v26            */
	.long  0x10a73682    /* vshasigmaw v5,v7,0,6       */
	.long  0x13957684    /* veqv    v28,v21,v14        */
	.long  0x10289e8c    /* vmrgow  v1,v8,v19          */
	.long  0x100a56c2    /* vshasigmad v0,v10,0,10     */
	.long  0x10bb76c4    /* vsrd    v5,v27,v14         */
	.long  0x11606ece    /* vupklsw v11,v13            */
	.long  0x11c08702    /* vclzb   v14,v16            */
	.long  0x1280df03    /* vpopcntb v20,v27           */
	.long  0x13805f42    /* vclzh   v28,v11            */
	.long  0x13004f43    /* vpopcnth v24,v9            */
	.long  0x1360ff82    /* vclzw   v27,v31            */
	.long  0x12209f83    /* vpopcntw v17,v19           */
	.long  0x1180efc2    /* vclzd   v12,v29            */
	.long  0x12e0b7c3    /* vpopcntd v23,v22           */
	.long  0x1314eec7    /* vcmpgtud. v24,v20,v29      */
	.long  0x1126dfc7    /* vcmpgtsd. v9,v6,v27        */
	.long  0x7fced019    /* lxsiwzx vs62,r14,r26       */
	.long  0x7d00c819    /* lxsiwzx vs40,0,r25         */
	.long  0x7f20d098    /* lxsiwax vs25,0,r26         */
	.long  0x7c601898    /* lxsiwax vs3,0,r3           */
	.long  0x7fcc0067    /* mfvsrd  r12,vs62           */
	.long  0x7d9400e6    /* mffprwz r20,f12            */
	.long  0x7dc97118    /* stxsiwx vs14,r9,r14        */
	.long  0x7ea04118    /* stxsiwx vs21,0,r8          */
	.long  0x7e0b0167    /* mtvsrd  vs48,r11           */
	.long  0x7ff701a7    /* mtvrwa  v31,r23            */
	.long  0x7e1a01e6    /* mtfprwz f16,r26            */
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
	.long  0x7c00719c    /* msgsnd  r14                */
	.long  0x7c00b9dc    /* msgclr  r23                */
	.long  0x7d002e99    /* lxvd2x  vs40,0,r5          */
	.long  0x7d543698    /* lxvd2x  vs10,r20,r6        */
	.long  0x7d203f99    /* stxvd2x vs41,0,r7          */
	.long  0x7d754798    /* stxvd2x vs11,r21,r8        */
	.long  0x7e803868    /* lbarx   r20,0,r7           */
	.long  0x7e803869    /* lbarx   r20,0,r7,1         */
	.long  0x7e813868    /* lbarx   r20,r1,r7          */
	.long  0x7e813869    /* lbarx   r20,r1,r7,1        */
	.long  0x7ea040a8    /* ldarx   r21,0,r8           */
	.long  0x7ea040a9    /* ldarx   r21,0,r8,1         */
	.long  0x7ea140a8    /* ldarx   r21,r1,r8          */
	.long  0x7ea140a9    /* ldarx   r21,r1,r8,1        */
	.long  0x7ec048e8    /* lharx   r22,0,r9           */
	.long  0x7ec048e9    /* lharx   r22,0,r9,1         */
	.long  0x7ec148e8    /* lharx   r22,r1,r9          */
	.long  0x7ec148e9    /* lharx   r22,r1,r9,1        */
	.long  0x7ee05028    /* lwarx   r23,0,r10          */
	.long  0x7ee05029    /* lwarx   r23,0,r10,1        */
	.long  0x7ee15028    /* lwarx   r23,r1,r10         */
	.long  0x7ee15029    /* lwarx   r23,r1,r10,1       */
	.long  0x7d403d6d    /* stbcx.  r10,0,r7           */
	.long  0x7d413d6d    /* stbcx.  r10,r1,r7          */
	.long  0x7d6045ad    /* sthcx.  r11,0,r8           */
	.long  0x7d6145ad    /* sthcx.  r11,r1,r8          */
	.long  0x7d80492d    /* stwcx.  r12,0,r9           */
	.long  0x7d81492d    /* stwcx.  r12,r1,r9          */
	.long  0x7da051ad    /* stdcx.  r13,0,r10          */
	.long  0x7da151ad    /* stdcx.  r13,r1,r10         */
	.section	.note.GNU-stack,"",@progbits
