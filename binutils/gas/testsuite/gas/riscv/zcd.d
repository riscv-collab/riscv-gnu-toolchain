#as: -march=rv64id_zcd
#objdump: -d -Mno-aliases

.*:[ 	]+file format .*

Disassembly of section .text:

0+000 <target>:
[ 	]+[0-9a-f]+:[ 	]+2108[ 	]+c.fld[ 	]+fa0,0\(a0\)
[ 	]+[0-9a-f]+:[ 	]+200c[ 	]+c.fld[ 	]+fa1,0\(s0\)
[ 	]+[0-9a-f]+:[ 	]+2502[ 	]+c.fldsp[ 	]+fa0,0\(sp\)
[ 	]+[0-9a-f]+:[ 	]+2582[ 	]+c.fldsp[ 	]+fa1,0\(sp\)
[ 	]+[0-9a-f]+:[ 	]+a108[ 	]+c.fsd[ 	]+fa0,0\(a0\)
[ 	]+[0-9a-f]+:[ 	]+a00c[ 	]+c.fsd[ 	]+fa1,0\(s0\)
[ 	]+[0-9a-f]+:[ 	]+a02a[ 	]+c.fsdsp[ 	]+fa0,0\(sp\)
[ 	]+[0-9a-f]+:[ 	]+a02e[ 	]+c.fsdsp[ 	]+fa1,0\(sp\)
