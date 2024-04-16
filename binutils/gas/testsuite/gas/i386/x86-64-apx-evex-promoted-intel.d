#as:
#objdump: -dw -Mintel
#name: x86_64 APX_F EVEX-Promoted insns (Intel disassembly)
#source: x86-64-apx-evex-promoted.s

.*: +file format .*

Disassembly of section \.text:

0+ <_start>:
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 fc 8c 87 23 01 00 00[	 ]+aadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 fc bc 87 23 01 00 00[	 ]+aadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7d 08 fc 8c 87 23 01 00 00[	 ]+aand[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fd 08 fc bc 87 23 01 00 00[	 ]+aand[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 dd b4 87 23 01 00 00[	 ]+aesdec128kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 df b4 87 23 01 00 00[	 ]+aesdec256kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 8c 87 23 01 00 00[	 ]+aesdecwide128kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 9c 87 23 01 00 00[	 ]+aesdecwide256kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 dc b4 87 23 01 00 00[	 ]+aesenc128kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 de b4 87 23 01 00 00[	 ]+aesenc256kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 84 87 23 01 00 00[	 ]+aesencwide128kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 94 87 23 01 00 00[	 ]+aesencwide256kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7f 08 fc 8c 87 23 01 00 00[	 ]+aor[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c ff 08 fc bc 87 23 01 00 00[	 ]+aor[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7e 08 fc 8c 87 23 01 00 00[	 ]+axor[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 fc bc 87 23 01 00 00[	 ]+axor[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 72 34 00 f7 d2[	 ]+bextr[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f7 94 87 23 01 00 00[	 ]+bextr[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 84 00 f7 df[	 ]+bextr[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 84 00 f7 bc 87 23 01 00 00[	 ]+bextr[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 da 6c 08 f3 d9[	 ]+blsi[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 84 08 f3 df[	 ]+blsi[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f3 9c 87 23 01 00 00[	 ]+blsi[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 84 00 f3 9c 87 23 01 00 00[	 ]+blsi[	 ]+r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 6c 08 f3 d1[	 ]+blsmsk[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 84 08 f3 d7[	 ]+blsmsk[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f3 94 87 23 01 00 00[	 ]+blsmsk[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 84 00 f3 94 87 23 01 00 00[	 ]+blsmsk[	 ]+r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 6c 08 f3 c9[	 ]+blsr[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 84 08 f3 cf[	 ]+blsr[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f3 8c 87 23 01 00 00[	 ]+blsr[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 84 00 f3 8c 87 23 01 00 00[	 ]+blsr[	 ]+r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 72 34 00 f5 d2[	 ]+bzhi[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f5 94 87 23 01 00 00[	 ]+bzhi[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 84 00 f5 df[	 ]+bzhi[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 84 00 f5 bc 87 23 01 00 00[	 ]+bzhi[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e6 94 87 23 01 00 00[	 ]+cmpbexadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e6 bc 87 23 01 00 00[	 ]+cmpbexadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e2 94 87 23 01 00 00[	 ]+cmpbxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e2 bc 87 23 01 00 00[	 ]+cmpbxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ec 94 87 23 01 00 00[	 ]+cmplxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ec bc 87 23 01 00 00[	 ]+cmplxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e7 94 87 23 01 00 00[	 ]+cmpnbexadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e7 bc 87 23 01 00 00[	 ]+cmpnbexadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e3 94 87 23 01 00 00[	 ]+cmpnbxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e3 bc 87 23 01 00 00[	 ]+cmpnbxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ef 94 87 23 01 00 00[	 ]+cmpnlexadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ef bc 87 23 01 00 00[	 ]+cmpnlexadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ed 94 87 23 01 00 00[	 ]+cmpnlxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ed bc 87 23 01 00 00[	 ]+cmpnlxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e1 94 87 23 01 00 00[	 ]+cmpnoxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e1 bc 87 23 01 00 00[	 ]+cmpnoxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 eb 94 87 23 01 00 00[	 ]+cmpnpxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 eb bc 87 23 01 00 00[	 ]+cmpnpxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e9 94 87 23 01 00 00[	 ]+cmpnsxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e9 bc 87 23 01 00 00[	 ]+cmpnsxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e5 94 87 23 01 00 00[	 ]+cmpnzxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e5 bc 87 23 01 00 00[	 ]+cmpnzxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e0 94 87 23 01 00 00[	 ]+cmpoxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e0 bc 87 23 01 00 00[	 ]+cmpoxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ea 94 87 23 01 00 00[	 ]+cmppxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ea bc 87 23 01 00 00[	 ]+cmppxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e8 94 87 23 01 00 00[	 ]+cmpsxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e8 bc 87 23 01 00 00[	 ]+cmpsxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e4 94 87 23 01 00 00[	 ]+cmpzxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e4 bc 87 23 01 00 00[	 ]+cmpzxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 cc fc 08 f1 f7[	 ]+crc32[	 ]+r22,r31
[	 ]*[a-f0-9]+:[	 ]*62 cc fc 08 f1 37[	 ]+crc32[	 ]+r22,QWORD PTR \[r31\]
[	 ]*[a-f0-9]+:[	 ]*62 ec fc 08 f0 cb[	 ]+crc32[	 ]+r17,r19b
[	 ]*[a-f0-9]+:[	 ]*62 ec 7c 08 f0 eb[	 ]+crc32[	 ]+r21d,r19b
[	 ]*[a-f0-9]+:[	 ]*62 fc 7c 08 f0 1b[	 ]+crc32[	 ]+ebx,BYTE PTR \[r19\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 f1 ff[	 ]+crc32[	 ]+r23d,r31d
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 f1 3f[	 ]+crc32[	 ]+r23d,DWORD PTR \[r31\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 f1 ef[	 ]+crc32[	 ]+r21d,r31w
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 f1 2f[	 ]+crc32[	 ]+r21d,WORD PTR \[r31\]
[	 ]*[a-f0-9]+:[	 ]*62 e4 fc 08 f1 d0[	 ]+crc32[	 ]+r18,rax
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 da d1[	 ]+encodekey128[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 db d1[	 ]+encodekey256[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*67 62 4c 7f 08 f8 8c 87 23 01 00 00[	 ]+enqcmd[	 ]+r25d,\[r31d\+eax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7f 08 f8 bc 87 23 01 00 00[	 ]+enqcmd[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*67 62 4c 7e 08 f8 8c 87 23 01 00 00[	 ]+enqcmds[	 ]+r25d,\[r31d\+eax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7e 08 f8 bc 87 23 01 00 00[	 ]+enqcmds[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 f0 bc 87 23 01 00 00[	 ]+invept[	 ]+r31,OWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 f2 bc 87 23 01 00 00[	 ]+invpcid[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 f1 bc 87 23 01 00 00[	 ]+invvpid[	 ]+r31,OWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 7d 08 93 cd[	 ]+kmovb[	 ]+r25d,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7d 08 91 ac 87 23 01 00 00[	 ]+kmovb[	 ]+BYTE PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7d 08 92 e9[	 ]+kmovb[	 ]+k5,r25d
[	 ]*[a-f0-9]+:[	 ]*62 d9 7d 08 90 ac 87 23 01 00 00[	 ]+kmovb[	 ]+k5,BYTE PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 7f 08 93 cd[	 ]+kmovd[	 ]+r25d,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 fd 08 91 ac 87 23 01 00 00[	 ]+kmovd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7f 08 92 e9[	 ]+kmovd[	 ]+k5,r25d
[	 ]*[a-f0-9]+:[	 ]*62 d9 fd 08 90 ac 87 23 01 00 00[	 ]+kmovd[	 ]+k5,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 ff 08 93 fd[	 ]+kmovq[	 ]+r31,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 fc 08 91 ac 87 23 01 00 00[	 ]+kmovq[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 ff 08 92 ef[	 ]+kmovq[	 ]+k5,r31
[	 ]*[a-f0-9]+:[	 ]*62 d9 fc 08 90 ac 87 23 01 00 00[	 ]+kmovq[	 ]+k5,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 7c 08 93 cd[	 ]+kmovw[	 ]+r25d,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7c 08 91 ac 87 23 01 00 00[	 ]+kmovw[	 ]+WORD PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7c 08 92 e9[	 ]+kmovw[	 ]+k5,r25d
[	 ]*[a-f0-9]+:[	 ]*62 d9 7c 08 90 ac 87 23 01 00 00[	 ]+kmovw[	 ]+k5,WORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7c 08 49 84 87 23 01 00 00[	 ]+ldtilecfg[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 fc 7d 08 60 c2[	 ]+movbe[	 ]+ax,r18w
[	 ]*[a-f0-9]+:[	 ]*62 ec 7d 08 61 94 80 23 01 00 00[	 ]+movbe[	 ]+WORD PTR \[r16\+rax\*4\+0x123\],r18w
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 61 94 87 23 01 00 00[	 ]+movbe[	 ]+WORD PTR \[r31\+rax\*4\+0x123\],r18w
[	 ]*[a-f0-9]+:[	 ]*62 dc 7c 08 60 d1[	 ]+movbe[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 6c 7c 08 61 8c 80 23 01 00 00[	 ]+movbe[	 ]+DWORD PTR \[r16\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 5c fc 08 60 ff[	 ]+movbe[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 6c fc 08 61 bc 80 23 01 00 00[	 ]+movbe[	 ]+QWORD PTR \[r16\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 61 bc 87 23 01 00 00[	 ]+movbe[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 6c fc 08 60 bc 80 23 01 00 00[	 ]+movbe[	 ]+r31,QWORD PTR \[r16\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 60 94 87 23 01 00 00[	 ]+movbe[	 ]+r18w,WORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 60 8c 87 23 01 00 00[	 ]+movbe[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*67 62 4c 7d 08 f8 8c 87 23 01 00 00[	 ]+movdir64b[	 ]+r25d,\[r31d\+eax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7d 08 f8 bc 87 23 01 00 00[	 ]+movdir64b[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 f9 8c 87 23 01 00 00[	 ]+movdiri[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 f9 bc 87 23 01 00 00[	 ]+movdiri[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 6f 08 f5 d1[	 ]+pdep[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 87 08 f5 df[	 ]+pdep[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 37 00 f5 94 87 23 01 00 00[	 ]+pdep[	 ]+edx,r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5a 87 00 f5 bc 87 23 01 00 00[	 ]+pdep[	 ]+r15,r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5a 6e 08 f5 d1[	 ]+pext[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 86 08 f5 df[	 ]+pext[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 36 00 f5 94 87 23 01 00 00[	 ]+pext[	 ]+edx,r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5a 86 00 f5 bc 87 23 01 00 00[	 ]+pext[	 ]+r15,r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 d9 f7[	 ]+sha1msg1 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 d9 b4 87 23 01 00 00[	 ]+sha1msg1 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 da f7[	 ]+sha1msg2 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 da b4 87 23 01 00 00[	 ]+sha1msg2 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 d8 f7[	 ]+sha1nexte xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 d8 b4 87 23 01 00 00[	 ]+sha1nexte xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 d4 f7 7b[	 ]+sha1rnds4 xmm22,xmm23,0x7b
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 d4 b4 87 23 01 00 00 7b[	 ]+sha1rnds4 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\],0x7b
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 dc f7[	 ]+sha256msg1 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 dc b4 87 23 01 00 00[	 ]+sha256msg1 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 dd f7[	 ]+sha256msg2 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 dd b4 87 23 01 00 00[	 ]+sha256msg2 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5c 7c 08 db a4 87 23 01 00 00[	 ]+sha256rnds2 xmm12,XMMWORD PTR \[r31\+rax\*4\+0x123\],xmm0
[	 ]*[a-f0-9]+:[	 ]*62 72 35 00 f7 d2[	 ]+shlx[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 f7 94 87 23 01 00 00[	 ]+shlx[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 85 00 f7 df[	 ]+shlx[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 f7 bc 87 23 01 00 00[	 ]+shlx[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 72 37 00 f7 d2[	 ]+shrx[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 37 00 f7 94 87 23 01 00 00[	 ]+shrx[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 87 00 f7 df[	 ]+shrx[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 87 00 f7 bc 87 23 01 00 00[	 ]+shrx[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 da 7d 08 49 84 87 23 01 00 00[	 ]+sttilecfg[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7f 08 4b b4 87 23 01 00 00[	 ]+tileloadd tmm6,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7d 08 4b b4 87 23 01 00 00[	 ]+tileloaddt1 tmm6,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7e 08 4b b4 87 23 01 00 00[	 ]+tilestored[	 ]+\[r31\+rax\*4\+0x123\],tmm6
[	 ]*[a-f0-9]+:[	 ]*62 db fd 08 09 30 01[	 ]+vrndscalepd xmm6,XMMWORD PTR \[r24\],(0x)?1
[	 ]*[a-f0-9]+:[	 ]*62 db 7d 08 08 30 02[	 ]+vrndscaleps xmm6,XMMWORD PTR \[r24\],(0x)?2
[	 ]*[a-f0-9]+:[	 ]*62 db cd 08 0b 18 03[	 ]+vrndscalesd xmm3,xmm6,QWORD PTR \[r24\],(0x)?3
[	 ]*[a-f0-9]+:[	 ]*62 db 4d 08 0a 18 04[	 ]+vrndscaless xmm3,xmm6,DWORD PTR \[r24\],(0x)?4
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 66 8c 87 23 01 00 00[	 ]+wrssd[	 ]+\[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 66 bc 87 23 01 00 00[	 ]+wrssq[	 ]+\[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7d 08 65 8c 87 23 01 00 00[	 ]+wrussd[	 ]+\[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fd 08 65 bc 87 23 01 00 00[	 ]+wrussq[	 ]+\[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 fc 8c 87 23 01 00 00[	 ]+aadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 fc bc 87 23 01 00 00[	 ]+aadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7d 08 fc 8c 87 23 01 00 00[	 ]+aand[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fd 08 fc bc 87 23 01 00 00[	 ]+aand[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 dd b4 87 23 01 00 00[	 ]+aesdec128kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 df b4 87 23 01 00 00[	 ]+aesdec256kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 8c 87 23 01 00 00[	 ]+aesdecwide128kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 9c 87 23 01 00 00[	 ]+aesdecwide256kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 dc b4 87 23 01 00 00[	 ]+aesenc128kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7e 08 de b4 87 23 01 00 00[	 ]+aesenc256kl xmm22,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 84 87 23 01 00 00[	 ]+aesencwide128kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 d8 94 87 23 01 00 00[	 ]+aesencwide256kl[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7f 08 fc 8c 87 23 01 00 00[	 ]+aor[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c ff 08 fc bc 87 23 01 00 00[	 ]+aor[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7e 08 fc 8c 87 23 01 00 00[	 ]+axor[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 fc bc 87 23 01 00 00[	 ]+axor[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 72 34 00 f7 d2[	 ]+bextr[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f7 94 87 23 01 00 00[	 ]+bextr[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 84 00 f7 df[	 ]+bextr[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 84 00 f7 bc 87 23 01 00 00[	 ]+bextr[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 da 6c 08 f3 d9[	 ]+blsi[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 84 08 f3 df[	 ]+blsi[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f3 9c 87 23 01 00 00[	 ]+blsi[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 84 00 f3 9c 87 23 01 00 00[	 ]+blsi[	 ]+r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 6c 08 f3 d1[	 ]+blsmsk[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 84 08 f3 d7[	 ]+blsmsk[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f3 94 87 23 01 00 00[	 ]+blsmsk[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 84 00 f3 94 87 23 01 00 00[	 ]+blsmsk[	 ]+r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 6c 08 f3 c9[	 ]+blsr[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 84 08 f3 cf[	 ]+blsr[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f3 8c 87 23 01 00 00[	 ]+blsr[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 84 00 f3 8c 87 23 01 00 00[	 ]+blsr[	 ]+r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 72 34 00 f5 d2[	 ]+bzhi[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 34 00 f5 94 87 23 01 00 00[	 ]+bzhi[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 84 00 f5 df[	 ]+bzhi[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 84 00 f5 bc 87 23 01 00 00[	 ]+bzhi[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e6 94 87 23 01 00 00[	 ]+cmpbexadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e6 bc 87 23 01 00 00[	 ]+cmpbexadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e2 94 87 23 01 00 00[	 ]+cmpbxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e2 bc 87 23 01 00 00[	 ]+cmpbxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ec 94 87 23 01 00 00[	 ]+cmplxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ec bc 87 23 01 00 00[	 ]+cmplxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e7 94 87 23 01 00 00[	 ]+cmpnbexadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e7 bc 87 23 01 00 00[	 ]+cmpnbexadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e3 94 87 23 01 00 00[	 ]+cmpnbxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e3 bc 87 23 01 00 00[	 ]+cmpnbxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ef 94 87 23 01 00 00[	 ]+cmpnlexadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ef bc 87 23 01 00 00[	 ]+cmpnlexadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ed 94 87 23 01 00 00[	 ]+cmpnlxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ed bc 87 23 01 00 00[	 ]+cmpnlxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e1 94 87 23 01 00 00[	 ]+cmpnoxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e1 bc 87 23 01 00 00[	 ]+cmpnoxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 eb 94 87 23 01 00 00[	 ]+cmpnpxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 eb bc 87 23 01 00 00[	 ]+cmpnpxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e9 94 87 23 01 00 00[	 ]+cmpnsxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e9 bc 87 23 01 00 00[	 ]+cmpnsxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e5 94 87 23 01 00 00[	 ]+cmpnzxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e5 bc 87 23 01 00 00[	 ]+cmpnzxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e0 94 87 23 01 00 00[	 ]+cmpoxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e0 bc 87 23 01 00 00[	 ]+cmpoxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 ea 94 87 23 01 00 00[	 ]+cmppxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 ea bc 87 23 01 00 00[	 ]+cmppxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e8 94 87 23 01 00 00[	 ]+cmpsxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e8 bc 87 23 01 00 00[	 ]+cmpsxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 e4 94 87 23 01 00 00[	 ]+cmpzxadd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 e4 bc 87 23 01 00 00[	 ]+cmpzxadd[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 cc fc 08 f1 f7[	 ]+crc32[	 ]+r22,r31
[	 ]*[a-f0-9]+:[	 ]*62 cc fc 08 f1 37[	 ]+crc32[	 ]+r22,QWORD PTR \[r31\]
[	 ]*[a-f0-9]+:[	 ]*62 ec fc 08 f0 cb[	 ]+crc32[	 ]+r17,r19b
[	 ]*[a-f0-9]+:[	 ]*62 ec 7c 08 f0 eb[	 ]+crc32[	 ]+r21d,r19b
[	 ]*[a-f0-9]+:[	 ]*62 fc 7c 08 f0 1b[	 ]+crc32[	 ]+ebx,BYTE PTR \[r19\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 f1 ff[	 ]+crc32[	 ]+r23d,r31d
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 f1 3f[	 ]+crc32[	 ]+r23d,DWORD PTR \[r31\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 f1 ef[	 ]+crc32[	 ]+r21d,r31w
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 f1 2f[	 ]+crc32[	 ]+r21d,WORD PTR \[r31\]
[	 ]*[a-f0-9]+:[	 ]*62 e4 fc 08 f1 d0[	 ]+crc32[	 ]+r18,rax
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 da d1[	 ]+encodekey128[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 dc 7e 08 db d1[	 ]+encodekey256[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*67 62 4c 7f 08 f8 8c 87 23 01 00 00[	 ]+enqcmd[	 ]+r25d,\[r31d\+eax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7f 08 f8 bc 87 23 01 00 00[	 ]+enqcmd[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*67 62 4c 7e 08 f8 8c 87 23 01 00 00[	 ]+enqcmds[	 ]+r25d,\[r31d\+eax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7e 08 f8 bc 87 23 01 00 00[	 ]+enqcmds[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 f0 bc 87 23 01 00 00[	 ]+invept[	 ]+r31,OWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 f2 bc 87 23 01 00 00[	 ]+invpcid[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c fe 08 f1 bc 87 23 01 00 00[	 ]+invvpid[	 ]+r31,OWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 7d 08 93 cd[	 ]+kmovb[	 ]+r25d,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7d 08 91 ac 87 23 01 00 00[	 ]+kmovb[	 ]+BYTE PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7d 08 92 e9[	 ]+kmovb[	 ]+k5,r25d
[	 ]*[a-f0-9]+:[	 ]*62 d9 7d 08 90 ac 87 23 01 00 00[	 ]+kmovb[	 ]+k5,BYTE PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 7f 08 93 cd[	 ]+kmovd[	 ]+r25d,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 fd 08 91 ac 87 23 01 00 00[	 ]+kmovd[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7f 08 92 e9[	 ]+kmovd[	 ]+k5,r25d
[	 ]*[a-f0-9]+:[	 ]*62 d9 fd 08 90 ac 87 23 01 00 00[	 ]+kmovd[	 ]+k5,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 ff 08 93 fd[	 ]+kmovq[	 ]+r31,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 fc 08 91 ac 87 23 01 00 00[	 ]+kmovq[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 ff 08 92 ef[	 ]+kmovq[	 ]+k5,r31
[	 ]*[a-f0-9]+:[	 ]*62 d9 fc 08 90 ac 87 23 01 00 00[	 ]+kmovq[	 ]+k5,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 61 7c 08 93 cd[	 ]+kmovw[	 ]+r25d,k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7c 08 91 ac 87 23 01 00 00[	 ]+kmovw[	 ]+WORD PTR \[r31\+rax\*4\+0x123\],k5
[	 ]*[a-f0-9]+:[	 ]*62 d9 7c 08 92 e9[	 ]+kmovw[	 ]+k5,r25d
[	 ]*[a-f0-9]+:[	 ]*62 d9 7c 08 90 ac 87 23 01 00 00[	 ]+kmovw[	 ]+k5,WORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7c 08 49 84 87 23 01 00 00[	 ]+ldtilecfg[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 fc 7d 08 60 c2[	 ]+movbe[	 ]+ax,r18w
[	 ]*[a-f0-9]+:[	 ]*62 ec 7d 08 61 94 80 23 01 00 00[	 ]+movbe[	 ]+WORD PTR \[r16\+rax\*4\+0x123\],r18w
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 61 94 87 23 01 00 00[	 ]+movbe[	 ]+WORD PTR \[r31\+rax\*4\+0x123\],r18w
[	 ]*[a-f0-9]+:[	 ]*62 dc 7c 08 60 d1[	 ]+movbe[	 ]+edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 6c 7c 08 61 8c 80 23 01 00 00[	 ]+movbe[	 ]+DWORD PTR \[r16\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 5c fc 08 60 ff[	 ]+movbe[	 ]+r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 6c fc 08 61 bc 80 23 01 00 00[	 ]+movbe[	 ]+QWORD PTR \[r16\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 61 bc 87 23 01 00 00[	 ]+movbe[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 6c fc 08 60 bc 80 23 01 00 00[	 ]+movbe[	 ]+r31,QWORD PTR \[r16\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 cc 7d 08 60 94 87 23 01 00 00[	 ]+movbe[	 ]+r18w,WORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 60 8c 87 23 01 00 00[	 ]+movbe[	 ]+r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*67 62 4c 7d 08 f8 8c 87 23 01 00 00[	 ]+movdir64b[	 ]+r25d,\[r31d\+eax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7d 08 f8 bc 87 23 01 00 00[	 ]+movdir64b[	 ]+r31,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 f9 8c 87 23 01 00 00[	 ]+movdiri[	 ]+DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 f9 bc 87 23 01 00 00[	 ]+movdiri[	 ]+QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 6f 08 f5 d1[	 ]+pdep[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 87 08 f5 df[	 ]+pdep[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 37 00 f5 94 87 23 01 00 00[	 ]+pdep[	 ]+edx,r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5a 87 00 f5 bc 87 23 01 00 00[	 ]+pdep[	 ]+r15,r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5a 6e 08 f5 d1[	 ]+pext[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 5a 86 08 f5 df[	 ]+pext[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 da 36 00 f5 94 87 23 01 00 00[	 ]+pext[	 ]+edx,r25d,DWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5a 86 00 f5 bc 87 23 01 00 00[	 ]+pext[	 ]+r15,r31,QWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 d9 f7[	 ]+sha1msg1 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 d9 b4 87 23 01 00 00[	 ]+sha1msg1 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 da f7[	 ]+sha1msg2 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 da b4 87 23 01 00 00[	 ]+sha1msg2 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 d8 f7[	 ]+sha1nexte xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 d8 b4 87 23 01 00 00[	 ]+sha1nexte xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 d4 f7 7b[	 ]+sha1rnds4 xmm22,xmm23,0x7b
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 d4 b4 87 23 01 00 00 7b[	 ]+sha1rnds4 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\],0x7b
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 dc f7[	 ]+sha256msg1 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 dc b4 87 23 01 00 00[	 ]+sha256msg1 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 a4 7c 08 dd f7[	 ]+sha256msg2 xmm22,xmm23
[	 ]*[a-f0-9]+:[	 ]*62 cc 7c 08 dd b4 87 23 01 00 00[	 ]+sha256msg2 xmm22,XMMWORD PTR \[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 5c 7c 08 db a4 87 23 01 00 00[	 ]+sha256rnds2 xmm12,XMMWORD PTR \[r31\+rax\*4\+0x123\],xmm0
[	 ]*[a-f0-9]+:[	 ]*62 72 35 00 f7 d2[	 ]+shlx[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 35 00 f7 94 87 23 01 00 00[	 ]+shlx[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 85 00 f7 df[	 ]+shlx[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 85 00 f7 bc 87 23 01 00 00[	 ]+shlx[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 72 37 00 f7 d2[	 ]+shrx[	 ]+r10d,edx,r25d
[	 ]*[a-f0-9]+:[	 ]*62 da 37 00 f7 94 87 23 01 00 00[	 ]+shrx[	 ]+edx,DWORD PTR \[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 52 87 00 f7 df[	 ]+shrx[	 ]+r11,r15,r31
[	 ]*[a-f0-9]+:[	 ]*62 5a 87 00 f7 bc 87 23 01 00 00[	 ]+shrx[	 ]+r15,QWORD PTR \[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 da 7d 08 49 84 87 23 01 00 00[	 ]+sttilecfg[	 ]+\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7f 08 4b b4 87 23 01 00 00[	 ]+tileloadd tmm6,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7d 08 4b b4 87 23 01 00 00[	 ]+tileloaddt1 tmm6,\[r31\+rax\*4\+0x123\]
[	 ]*[a-f0-9]+:[	 ]*62 da 7e 08 4b b4 87 23 01 00 00[	 ]+tilestored[	 ]+\[r31\+rax\*4\+0x123\],tmm6
[	 ]*[a-f0-9]+:[	 ]*62 4c 7c 08 66 8c 87 23 01 00 00[	 ]+wrssd[	 ]+\[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fc 08 66 bc 87 23 01 00 00[	 ]+wrssq[	 ]+\[r31\+rax\*4\+0x123\],r31
[	 ]*[a-f0-9]+:[	 ]*62 4c 7d 08 65 8c 87 23 01 00 00[	 ]+wrussd[	 ]+\[r31\+rax\*4\+0x123\],r25d
[	 ]*[a-f0-9]+:[	 ]*62 4c fd 08 65 bc 87 23 01 00 00[	 ]+wrussq[	 ]+\[r31\+rax\*4\+0x123\],r31
#pass
