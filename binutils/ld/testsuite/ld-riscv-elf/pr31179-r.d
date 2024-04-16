#source: pr31179.s
#as:
#readelf: -Wr

Relocation section '.rela.text' at .*
[ 	]+Offset[ 	]+Info[ 	]+Type[ 	]+.*
[0-9a-f]+[ 	]+[0-9a-f]+[ 	]+R_RISCV_SET_ULEB128[ 	]+[0-9a-f]+[ 	]+bar \+ 1
[0-9a-f]+[ 	]+[0-9a-f]+[ 	]+R_RISCV_SUB_ULEB128[ 	]+[0-9a-f]+[ 	]+foo \+ 0
[0-9a-f]+[ 	]+[0-9a-f]+[ 	]+R_RISCV_SET_ULEB128[ 	]+[0-9a-f]+[ 	]+bar \+ 1
[0-9a-f]+[ 	]+[0-9a-f]+[ 	]+R_RISCV_SUB_ULEB128[ 	]+[0-9a-f]+[ 	]+foo \+ 1
