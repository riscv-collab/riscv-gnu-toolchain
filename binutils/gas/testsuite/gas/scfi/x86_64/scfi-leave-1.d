#as: --scfi=experimental -W
#as:
#objdump: -Wf
#name: Synthesize CFI for leave insn
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

0+0018 0+0024 0+001c FDE cie=0+0000 pc=0+0000..0+000a
  DW_CFA_advance_loc: 1 to 0+0001
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r6 \(rbp\) at cfa-16
  DW_CFA_advance_loc: 3 to 0+0004
  DW_CFA_def_cfa_register: r6 \(rbp\)
  DW_CFA_advance_loc: 1 to 0+0005
  DW_CFA_offset: r3 \(rbx\) at cfa-24
  DW_CFA_advance_loc: 3 to 0+0008
  DW_CFA_restore: r3 \(rbx\)
  DW_CFA_advance_loc: 1 to 0+0009
  DW_CFA_def_cfa_register: r7 \(rsp\)
  DW_CFA_restore: r6 \(rbp\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
