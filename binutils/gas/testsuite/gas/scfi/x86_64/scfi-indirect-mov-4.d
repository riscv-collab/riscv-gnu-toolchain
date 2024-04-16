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

00000018 0+004c 0000001c FDE cie=00000000 pc=0+0000..0+003d
  DW_CFA_advance_loc: 2 to 0+0002
  DW_CFA_def_cfa_offset: 16
  DW_CFA_offset: r15 \(r15\) at cfa-16
  DW_CFA_advance_loc: 2 to 0+0004
  DW_CFA_def_cfa_offset: 24
  DW_CFA_offset: r14 \(r14\) at cfa-24
  DW_CFA_advance_loc: 5 to 0+0009
  DW_CFA_def_cfa_offset: 32
  DW_CFA_offset: r13 \(r13\) at cfa-32
  DW_CFA_advance_loc: 5 to 0+000e
  DW_CFA_def_cfa_offset: 40
  DW_CFA_offset: r12 \(r12\) at cfa-40
  DW_CFA_advance_loc: 4 to 0+0012
  DW_CFA_def_cfa_offset: 48
  DW_CFA_offset: r6 \(rbp\) at cfa-48
  DW_CFA_advance_loc: 4 to 0+0016
  DW_CFA_def_cfa_offset: 56
  DW_CFA_offset: r3 \(rbx\) at cfa-56
  DW_CFA_advance_loc: 7 to 0+001d
  DW_CFA_def_cfa_offset: 96
  DW_CFA_advance_loc: 21 to 0+0032
  DW_CFA_def_cfa_offset: 56
  DW_CFA_advance_loc: 1 to 0+0033
  DW_CFA_restore: r3 \(rbx\)
  DW_CFA_def_cfa_offset: 48
  DW_CFA_advance_loc: 1 to 0+0034
  DW_CFA_restore: r6 \(rbp\)
  DW_CFA_def_cfa_offset: 40
  DW_CFA_advance_loc: 2 to 0+0036
  DW_CFA_restore: r12 \(r12\)
  DW_CFA_def_cfa_offset: 32
  DW_CFA_advance_loc: 2 to 0+0038
  DW_CFA_restore: r13 \(r13\)
  DW_CFA_def_cfa_offset: 24
  DW_CFA_advance_loc: 2 to 0+003a
  DW_CFA_restore: r14 \(r14\)
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 0+003c
  DW_CFA_restore: r15 \(r15\)
  DW_CFA_def_cfa_offset: 8
  DW_CFA_nop
#...

#pass
