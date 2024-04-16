#as: --gsframe
#warning: skipping SFrame FDE due to DWARF CFI op 0xe
#objdump: --sframe=.sframe
#name: SFrame supports only FP/SP based CFA
#...
Contents of the SFrame section .sframe:

  Header :

    Version: SFRAME_VERSION_2
    Flags: NONE
    Num FDEs: 0
    Num FREs: 0

#pass
