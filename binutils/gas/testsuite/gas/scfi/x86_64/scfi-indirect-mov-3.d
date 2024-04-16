#as: --scfi=experimental -W
#as:
#objdump: -Wf
#name: Synthesize CFI for indirect mem op to stack 3
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

0+0018 0+002c 0+001c FDE cie=00000000 pc=0+0000..0+0028
  DW_CFA_advance_loc: 1 to 0+0001
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r6 \(rbp\) at cfa-16
  DW_CFA_advance_loc: 3 to 0+0004
  DW_CFA_def_cfa_register: r6 \(rbp\)
  DW_CFA_advance_loc: 8 to 0+000c
  DW_CFA_offset: r3 \(rbx\) at cfa-32
  DW_CFA_advance_loc: 5 to 0+0011
  DW_CFA_offset: r12 \(r12\) at cfa-24
  DW_CFA_advance_loc: 9 to 0+001a
  DW_CFA_restore: r3 \(rbx\)
  DW_CFA_advance_loc: 5 to 0+001f
  DW_CFA_restore: r12 \(r12\)
  DW_CFA_advance_loc: 7 to 0+0026
  DW_CFA_def_cfa_register: r7 \(rsp\)
  DW_CFA_advance_loc: 1 to 0+0027
  DW_CFA_restore: r6 \(rbp\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
