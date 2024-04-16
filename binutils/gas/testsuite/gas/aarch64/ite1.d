#as: -march=armv9.4-a+ite
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
.*:	d50b72e1 	trcit	x1
.*:	d5381261 	mrs	x1, trcitecr_el1
.*:	d53d1263 	mrs	x3, trcitecr_el12
.*:	d53c1265 	mrs	x5, trcitecr_el2
.*:	d5310227 	mrs	x7, trciteedcr
.*:	d5181261 	msr	trcitecr_el1, x1
.*:	d51d1263 	msr	trcitecr_el12, x3
.*:	d51c1265 	msr	trcitecr_el2, x5
.*:	d5110227 	msr	trciteedcr, x7
