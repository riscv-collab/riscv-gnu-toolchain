#source: pr31179.s
#as:
#ld: --check-uleb128
#objdump: -sj .text
#warning: .*R_RISCV_SUB_ULEB128 with non-zero addend, please rebuild by binutils 2.42 or up

.*:[ 	]+file format .*

Contents of section .text:

[ 	]+[0-9a-f]+[ 	]+00000303[ 	]+.*
