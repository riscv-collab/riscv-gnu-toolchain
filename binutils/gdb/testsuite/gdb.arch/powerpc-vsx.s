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
	.long  0x7d0aa499    /* lxsdx   vs40,r10,r20       */
	.long  0x7d0aa699    /* lxvd2x  vs40,r10,r20       */
	.long  0x7d0aa299    /* lxvdsx  vs40,r10,r20       */
	.long  0x7d0aa619    /* lxvw4x  vs40,r10,r20       */
	.long  0x7d0aa599    /* stxsdx  vs40,r10,r20       */
	.long  0x7d0aa799    /* stxvd2x vs40,r10,r20       */
	.long  0x7d0aa719    /* stxvw4x vs40,r10,r20       */
	.long  0xf100e567    /* xsabsdp vs40,vs60          */
	.long  0xf112e107    /* xsadddp vs40,vs50,vs60     */
	.long  0xf092e15e    /* xscmpodp cr1,vs50,vs60     */
	.long  0xf092e11e    /* xscmpudp cr1,vs50,vs60     */
	.long  0xf112e587    /* xscpsgndp vs40,vs50,vs60   */
	.long  0xf100e427    /* xscvdpsp vs40,vs60         */
	.long  0xf100e563    /* xscvdpsxds vs40,vs60       */
	.long  0xf100e163    /* xscvdpsxws vs40,vs60       */
	.long  0xf100e523    /* xscvdpuxds vs40,vs60       */
	.long  0xf100e123    /* xscvdpuxws vs40,vs60       */
	.long  0xf100e527    /* xscvspdp vs40,vs60         */
	.long  0xf100e5e3    /* xscvsxddp vs40,vs60        */
	.long  0xf100e5a3    /* xscvuxddp vs40,vs60        */
	.long  0xf112e1c7    /* xsdivdp vs40,vs50,vs60     */
	.long  0xf112e10f    /* xsmaddadp vs40,vs50,vs60   */
	.long  0xf112e14f    /* xsmaddmdp vs40,vs50,vs60   */
	.long  0xf112e507    /* xsmaxdp vs40,vs50,vs60     */
	.long  0xf112e547    /* xsmindp vs40,vs50,vs60     */
	.long  0xf112e18f    /* xsmsubadp vs40,vs50,vs60   */
	.long  0xf112e1cf    /* xsmsubmdp vs40,vs50,vs60   */
	.long  0xf112e187    /* xsmuldp vs40,vs50,vs60     */
	.long  0xf100e5a7    /* xsnabsdp vs40,vs60         */
	.long  0xf100e5e7    /* xsnegdp vs40,vs60          */
	.long  0xf112e50f    /* xsnmaddadp vs40,vs50,vs60  */
	.long  0xf112e54f    /* xsnmaddmdp vs40,vs50,vs60  */
	.long  0xf112e58f    /* xsnmsubadp vs40,vs50,vs60  */
	.long  0xf112e5cf    /* xsnmsubmdp vs40,vs50,vs60  */
	.long  0xf100e127    /* xsrdpi  vs40,vs60          */
	.long  0xf100e1af    /* xsrdpic vs40,vs60          */
	.long  0xf100e1e7    /* xsrdpim vs40,vs60          */
	.long  0xf100e1a7    /* xsrdpip vs40,vs60          */
	.long  0xf100e167    /* xsrdpiz vs40,vs60          */
	.long  0xf100e16b    /* xsredp  vs40,vs60          */
	.long  0xf100e12b    /* xsrsqrtedp vs40,vs60       */
	.long  0xf100e12f    /* xssqrtdp vs40,vs60         */
	.long  0xf112e147    /* xssubdp vs40,vs50,vs60     */
	.long  0xf092e1ee    /* xstdivdp cr1,vs50,vs60     */
	.long  0xf080e1aa    /* xstsqrtdp cr1,vs60         */
	.long  0xf100e767    /* xvabsdp vs40,vs60          */
	.long  0xf100e667    /* xvabssp vs40,vs60          */
	.long  0xf112e307    /* xvadddp vs40,vs50,vs60     */
	.long  0xf112e207    /* xvaddsp vs40,vs50,vs60     */
	.long  0xf112e31f    /* xvcmpeqdp vs40,vs50,vs60   */
	.long  0xf112e71f    /* xvcmpeqdp. vs40,vs50,vs60  */
	.long  0xf112e21f    /* xvcmpeqsp vs40,vs50,vs60   */
	.long  0xf112e61f    /* xvcmpeqsp. vs40,vs50,vs60  */
	.long  0xf112e39f    /* xvcmpgedp vs40,vs50,vs60   */
	.long  0xf112e79f    /* xvcmpgedp. vs40,vs50,vs60  */
	.long  0xf112e29f    /* xvcmpgesp vs40,vs50,vs60   */
	.long  0xf112e69f    /* xvcmpgesp. vs40,vs50,vs60  */
	.long  0xf112e35f    /* xvcmpgtdp vs40,vs50,vs60   */
	.long  0xf112e75f    /* xvcmpgtdp. vs40,vs50,vs60  */
	.long  0xf112e25f    /* xvcmpgtsp vs40,vs50,vs60   */
	.long  0xf112e65f    /* xvcmpgtsp. vs40,vs50,vs60  */
	.long  0xf112e787    /* xvcpsgndp vs40,vs50,vs60   */
	.long  0xf11ce787    /* xvmovdp vs40,vs60          */
	.long  0xf112e687    /* xvcpsgnsp vs40,vs50,vs60   */
	.long  0xf11ce687    /* xvmovsp vs40,vs60          */
	.long  0xf100e627    /* xvcvdpsp vs40,vs60         */
	.long  0xf100e763    /* xvcvdpsxds vs40,vs60       */
	.long  0xf100e363    /* xvcvdpsxws vs40,vs60       */
	.long  0xf100e723    /* xvcvdpuxds vs40,vs60       */
	.long  0xf100e323    /* xvcvdpuxws vs40,vs60       */
	.long  0xf100e727    /* xvcvspdp vs40,vs60         */
	.long  0xf100e663    /* xvcvspsxds vs40,vs60       */
	.long  0xf100e263    /* xvcvspsxws vs40,vs60       */
	.long  0xf100e623    /* xvcvspuxds vs40,vs60       */
	.long  0xf100e223    /* xvcvspuxws vs40,vs60       */
	.long  0xf100e7e3    /* xvcvsxddp vs40,vs60        */
	.long  0xf100e6e3    /* xvcvsxdsp vs40,vs60        */
	.long  0xf100e3e3    /* xvcvsxwdp vs40,vs60        */
	.long  0xf100e2e3    /* xvcvsxwsp vs40,vs60        */
	.long  0xf100e7a3    /* xvcvuxddp vs40,vs60        */
	.long  0xf100e6a3    /* xvcvuxdsp vs40,vs60        */
	.long  0xf100e3a3    /* xvcvuxwdp vs40,vs60        */
	.long  0xf100e2a3    /* xvcvuxwsp vs40,vs60        */
	.long  0xf112e3c7    /* xvdivdp vs40,vs50,vs60     */
	.long  0xf112e2c7    /* xvdivsp vs40,vs50,vs60     */
	.long  0xf112e30f    /* xvmaddadp vs40,vs50,vs60   */
	.long  0xf112e34f    /* xvmaddmdp vs40,vs50,vs60   */
	.long  0xf112e20f    /* xvmaddasp vs40,vs50,vs60   */
	.long  0xf112e24f    /* xvmaddmsp vs40,vs50,vs60   */
	.long  0xf112e707    /* xvmaxdp vs40,vs50,vs60     */
	.long  0xf112e607    /* xvmaxsp vs40,vs50,vs60     */
	.long  0xf112e747    /* xvmindp vs40,vs50,vs60     */
	.long  0xf112e647    /* xvminsp vs40,vs50,vs60     */
	.long  0xf112e38f    /* xvmsubadp vs40,vs50,vs60   */
	.long  0xf112e3cf    /* xvmsubmdp vs40,vs50,vs60   */
	.long  0xf112e28f    /* xvmsubasp vs40,vs50,vs60   */
	.long  0xf112e2cf    /* xvmsubmsp vs40,vs50,vs60   */
	.long  0xf112e387    /* xvmuldp vs40,vs50,vs60     */
	.long  0xf112e287    /* xvmulsp vs40,vs50,vs60     */
	.long  0xf100e7a7    /* xvnabsdp vs40,vs60         */
	.long  0xf100e6a7    /* xvnabssp vs40,vs60         */
	.long  0xf100e7e7    /* xvnegdp vs40,vs60          */
	.long  0xf100e6e7    /* xvnegsp vs40,vs60          */
	.long  0xf112e70f    /* xvnmaddadp vs40,vs50,vs60  */
	.long  0xf112e74f    /* xvnmaddmdp vs40,vs50,vs60  */
	.long  0xf112e60f    /* xvnmaddasp vs40,vs50,vs60  */
	.long  0xf112e64f    /* xvnmaddmsp vs40,vs50,vs60  */
	.long  0xf112e78f    /* xvnmsubadp vs40,vs50,vs60  */
	.long  0xf112e7cf    /* xvnmsubmdp vs40,vs50,vs60  */
	.long  0xf112e68f    /* xvnmsubasp vs40,vs50,vs60  */
	.long  0xf112e6cf    /* xvnmsubmsp vs40,vs50,vs60  */
	.long  0xf100e327    /* xvrdpi  vs40,vs60          */
	.long  0xf100e3af    /* xvrdpic vs40,vs60          */
	.long  0xf100e3e7    /* xvrdpim vs40,vs60          */
	.long  0xf100e3a7    /* xvrdpip vs40,vs60          */
	.long  0xf100e367    /* xvrdpiz vs40,vs60          */
	.long  0xf100e36b    /* xvredp  vs40,vs60          */
	.long  0xf100e26b    /* xvresp  vs40,vs60          */
	.long  0xf100e227    /* xvrspi  vs40,vs60          */
	.long  0xf100e2af    /* xvrspic vs40,vs60          */
	.long  0xf100e2e7    /* xvrspim vs40,vs60          */
	.long  0xf100e2a7    /* xvrspip vs40,vs60          */
	.long  0xf100e267    /* xvrspiz vs40,vs60          */
	.long  0xf100e32b    /* xvrsqrtedp vs40,vs60       */
	.long  0xf100e22b    /* xvrsqrtesp vs40,vs60       */
	.long  0xf100e32f    /* xvsqrtdp vs40,vs60         */
	.long  0xf100e22f    /* xvsqrtsp vs40,vs60         */
	.long  0xf112e347    /* xvsubdp vs40,vs50,vs60     */
	.long  0xf112e247    /* xvsubsp vs40,vs50,vs60     */
	.long  0xf092e3ee    /* xvtdivdp cr1,vs50,vs60     */
	.long  0xf092e2ee    /* xvtdivsp cr1,vs50,vs60     */
	.long  0xf080e3aa    /* xvtsqrtdp cr1,vs60         */
	.long  0xf080e2aa    /* xvtsqrtsp cr1,vs60         */
	.long  0xf112e417    /* xxland  vs40,vs50,vs60     */
	.long  0xf112e457    /* xxlandc vs40,vs50,vs60     */
	.long  0xf112e517    /* xxlnor  vs40,vs50,vs60     */
	.long  0xf112e497    /* xxlor   vs40,vs50,vs60     */
	.long  0xf112e4d7    /* xxlxor  vs40,vs50,vs60     */
	.long  0xf112e097    /* xxmrghw vs40,vs50,vs60     */
	.long  0xf112e197    /* xxmrglw vs40,vs50,vs60     */
	.long  0xf112e057    /* xxmrghd vs40,vs50,vs60     */
	.long  0xf112e157    /* xxpermdi vs40,vs50,vs60,1  */
	.long  0xf112e257    /* xxpermdi vs40,vs50,vs60,2  */
	.long  0xf112e357    /* xxmrgld vs40,vs50,vs60     */
	.long  0xf1129057    /* xxspltd vs40,vs50,0        */
	.long  0xf1129357    /* xxspltd vs40,vs50,1        */
	.long  0xf1129257    /* xxswapd vs40,vs50          */
	.long  0xf112e7bf    /* xxsel   vs40,vs50,vs60,vs62 */
	.long  0xf112e217    /* xxsldwi vs40,vs50,vs60,2   */
	.long  0xf102e293    /* xxspltw vs40,vs60,2        */
	.long  0x7d00a699    /* lxvd2x  vs40,0,r20         */
	.long  0x7d00a799    /* stxvd2x vs40,0,r20         */
	.section	.note.GNU-stack,"",@progbits
