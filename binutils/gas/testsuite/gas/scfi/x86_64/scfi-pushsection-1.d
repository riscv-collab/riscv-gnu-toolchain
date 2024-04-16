#as: --scfi=experimental -W
#objdump: -Wf
#name: Synthesize CFI with pushsection 1
#...
Contents of the .eh_frame section:

00000000 0+0014 0+0000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -8
  Return address column: 16
  Augmentation data:     [01][abc]
  DW_CFA_def_cfa: r7 \(rsp\) ofs 8
  DW_CFA_offset: r16 \(rip\) at cfa-8
  DW_CFA_nop
  DW_CFA_nop

00000018 0+0010 0000001c FDE cie=00000000 pc=0+0000..0+0005
  DW_CFA_advance_loc: 4 to 0+0004
  DW_CFA_def_cfa_offset: 48

0000002c 0+0028 00000030 FDE cie=00000000 pc=0+0000..0+0014
  DW_CFA_advance_loc: 2 to 0+0002
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r12 \(r12\) at cfa-16
  DW_CFA_advance_loc: 2 to 0+0004
  DW_CFA_def_cfa_offset: 24
  DW_CFA_offset: r13 \(r13\) at cfa-24
  DW_CFA_advance_loc: 4 to 0+0008
  DW_CFA_def_cfa_offset: 32
  DW_CFA_advance_loc: 7 to 0+000f
  DW_CFA_def_cfa_offset: 24
  DW_CFA_advance_loc: 2 to 0+0011
  DW_CFA_restore: r13 \(r13\)
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 0+0013
  DW_CFA_restore: r12 \(r12\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
