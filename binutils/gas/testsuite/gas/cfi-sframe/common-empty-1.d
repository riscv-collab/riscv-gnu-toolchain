#as: --gsframe
#warning: skipping SFrame FDE due to DWARF CFI op 0xa
#objdump: --sframe=.sframe
#name: Uninteresting cfi directives generate an empty SFrame section
#...
Contents of the SFrame section .sframe:

  Header :

    Version: SFRAME_VERSION_2
    Flags: NONE
    Num FDEs: 0
    Num FREs: 0

#pass
