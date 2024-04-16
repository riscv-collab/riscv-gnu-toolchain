#as:
#ld: -shared -z combreloc
#objdump: -R
#skip: loongarch*-elf

.*: +file format .*

DYNAMIC RELOCATION RECORDS
OFFSET +TYPE +VALUE
[[:xdigit:]]+ R_LARCH_64 +test
[[:xdigit:]]+ R_LARCH_IRELATIVE +\*ABS\*\+0x[[:xdigit:]]+
