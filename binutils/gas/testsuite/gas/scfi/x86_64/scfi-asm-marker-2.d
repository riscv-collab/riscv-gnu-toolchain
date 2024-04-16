#as: --scfi=experimental -W
#as:
#objdump: -Wf
#name: Synthesize CFI for demarcated code blocks 2
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

00000018 0+0014 0000001c FDE cie=00000000 pc=0+0000..0+000f
  DW_CFA_nop
  DW_CFA_nop
#...

#pass
