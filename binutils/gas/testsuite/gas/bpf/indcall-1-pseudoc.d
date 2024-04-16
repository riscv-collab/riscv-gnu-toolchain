#as: -EL -mdialect=pseudoc -misa-spec=xbpf
#objdump: -M xbpf,pseudoc,dec -dr
#source: indcall-1-pseudoc.s
#name: BPF indirect call 1, pseudoc syntax

.*: +file format .*bpf.*

Disassembly of section \.text:

0000000000000000 <main>:
   0:	b7 00 00 00 01 00 00 00 	r0=1
   8:	b7 01 00 00 01 00 00 00 	r1=1
  10:	b7 02 00 00 02 00 00 00 	r2=2
  18:	18 06 00 00 38 00 00 00 	r6=56 ll
  20:	00 00 00 00 00 00 00 00[    ]*
			18: R_BPF_64_64	.text
  28:	8d 06 00 00 00 00 00 00 	callx r6
  30:	95 00 00 00 00 00 00 00 	exit

0000000000000038 <bar>:
  38:	b7 00 00 00 00 00 00 00 	r0=0
  40:	95 00 00 00 00 00 00 00 	exit
#pass
