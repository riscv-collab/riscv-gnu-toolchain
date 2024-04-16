#as: --scfi=experimental -W
#as:
#objdump: -Wf
#name: Synthesize CFI for indirect mem op to stack 1
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

0+0018 0+0034 0+001c FDE cie=00000000 pc=0+0000..0+005b
  DW_CFA_advance_loc: 4 to 0+0004
  DW_CFA_def_cfa_offset: 64
  DW_CFA_advance_loc: 4 to 0+0008
  DW_CFA_offset: r3 \(rbx\) at cfa-64
  DW_CFA_advance_loc: 5 to 0+000d
  DW_CFA_offset: r6 \(rbp\) at cfa-56
  DW_CFA_advance_loc: 5 to 0+0012
  DW_CFA_offset: r12 \(r12\) at cfa-48
  DW_CFA_advance_loc: 5 to 0+0017
  DW_CFA_offset: r13 \(r13\) at cfa-40
  DW_CFA_advance_loc: 5 to 0+001c
  DW_CFA_offset: r14 \(r14\) at cfa-32
  DW_CFA_advance_loc: 5 to 0+0021
  DW_CFA_offset: r15 \(r15\) at cfa-24
  DW_CFA_advance_loc: 29 to 0+003e
  DW_CFA_restore: r15 \(r15\)
  DW_CFA_advance_loc: 5 to 0+0043
  DW_CFA_restore: r14 \(r14\)
  DW_CFA_advance_loc: 5 to 0+0048
  DW_CFA_restore: r13 \(r13\)
  DW_CFA_advance_loc: 5 to 0+004d
  DW_CFA_restore: r12 \(r12\)
  DW_CFA_advance_loc: 5 to 0+0052
  DW_CFA_restore: r6 \(rbp\)
  DW_CFA_advance_loc: 4 to 0+0056
  DW_CFA_restore: r3 \(rbx\)
  DW_CFA_advance_loc: 4 to 0+005a
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
