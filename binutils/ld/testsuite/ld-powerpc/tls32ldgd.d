#as: -a32
#ld: -shared -melf32ppc
#readelf: -rW

Relocation section '\.rela\.dyn' at offset .* contains 3 entries:
 Offset +Info +Type +Sym\. Value +Symbol's Name \+ Addend
.* +00000044 R_PPC_DTPMOD32 +0
.* +0000004e R_PPC_DTPREL32 +0
.* +00000044 R_PPC_DTPMOD32 +0

Relocation section '\.rela\.plt' at offset .* contains 1 entry:
 Offset +Info +Type +Sym\. Value +Symbol's Name \+ Addend
.* +00000215 R_PPC_JMP_SLOT +00000000 +__tls_get_addr \+ 0
