#as: --scfi=experimental -W
#as:
#objdump: -Wf
#name: Synthesize CFI in presence of control flow 1
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

0+0018 0+0024 0000001c FDE cie=00000000 pc=0+0000..0+003a
  DW_CFA_advance_loc: 1 to 0+0001
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r3 \(rbx\) at cfa-16
  DW_CFA_advance_loc: 37 to 0+0026
  DW_CFA_remember_state
  DW_CFA_advance_loc: 1 to 0+0027
  DW_CFA_restore: r3 \(rbx\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_advance_loc: 1 to 0+0028
  DW_CFA_restore_state
  DW_CFA_advance_loc: 9 to 0+0031
  DW_CFA_restore: r3 \(rbx\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
