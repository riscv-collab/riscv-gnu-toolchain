#as: -march=rv32if_zcf
#objdump: -d -Mno-aliases

.*:[ 	]+file format .*

Disassembly of section .text:

0+000 <target>:
[ 	]+[0-9a-f]+:[ 	]+6108[ 	]+c.flw[ 	]+fa0,0\(a0\)
[ 	]+[0-9a-f]+:[ 	]+600c[ 	]+c.flw[ 	]+fa1,0\(s0\)
[ 	]+[0-9a-f]+:[ 	]+6502[ 	]+c.flwsp[ 	]+fa0,0\(sp\)
[ 	]+[0-9a-f]+:[ 	]+6582[ 	]+c.flwsp[ 	]+fa1,0\(sp\)
[ 	]+[0-9a-f]+:[ 	]+e108[ 	]+c.fsw[ 	]+fa0,0\(a0\)
[ 	]+[0-9a-f]+:[ 	]+e00c[ 	]+c.fsw[ 	]+fa1,0\(s0\)
[ 	]+[0-9a-f]+:[ 	]+e02a[ 	]+c.fswsp[ 	]+fa0,0\(sp\)
[ 	]+[0-9a-f]+:[ 	]+e02e[ 	]+c.fswsp[ 	]+fa1,0\(sp\)
