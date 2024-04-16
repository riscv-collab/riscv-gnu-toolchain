#as: --scfi=experimental -W
#as:
#objdump: -Wf
#name: SCFI for callee-saved registers 2
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

00000018 0+002c 0000001c FDE cie=00000000 pc=0+0000..0+0017
  DW_CFA_advance_loc: 2 to 0+0002
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r12 \(r12\) at cfa-16
  DW_CFA_advance_loc: 2 to 0+0004
  DW_CFA_def_cfa_offset: 24
  DW_CFA_offset: r13 \(r13\) at cfa-24
  DW_CFA_advance_loc: 9 to 0+000d
  DW_CFA_def_cfa_offset: 32
  DW_CFA_advance_loc: 1 to 0+000e
  DW_CFA_def_cfa_offset: 40
  DW_CFA_advance_loc: 4 to 0+0012
  DW_CFA_def_cfa_offset: 24
  DW_CFA_advance_loc: 2 to 0+0014
  DW_CFA_restore: r13 \(r13\)
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 0+0016
  DW_CFA_restore: r12 \(r12\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
