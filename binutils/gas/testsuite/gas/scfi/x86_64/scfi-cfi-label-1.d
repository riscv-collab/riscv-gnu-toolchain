#as: --scfi=experimental -W
#as:
#objdump: -tWf
#name: SCFI no ignore .cfi_label
#...
.*\.o:     file format elf.*

SYMBOL TABLE:
0+0000 l    d  \.text	0+0000 \.text
0+002b l       \.eh_frame	0+0000 cfi2
0+0000 g     F \.text	0+0008 foo
0+002a g       \.eh_frame	0+0000 cfi1


Contents of the .eh_frame section:


00000000 0+0014 00000000 CIE
  Version:               1
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -8
  Return address column: 16
  Augmentation data:     1b
  DW_CFA_def_cfa: r7 \(rsp\) ofs 8
  DW_CFA_offset: r16 \(rip\) at cfa-8
  DW_CFA_nop
  DW_CFA_nop

00000018 0+0014 0000001c FDE cie=00000000 pc=0+0000..0+0008
  DW_CFA_advance_loc: 1 to 0+0001
  DW_CFA_advance_loc: 1 to 0+0002
  DW_CFA_advance_loc: 1 to 0+0003
  DW_CFA_advance_loc: 4 to 0+0007
  DW_CFA_def_cfa_offset: 0
  DW_CFA_nop

#pass
