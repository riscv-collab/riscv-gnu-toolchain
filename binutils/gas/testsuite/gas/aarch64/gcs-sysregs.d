#name: Test of Guarded Control Stack system registers
#as: -march=armv8.8-a+gcs
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
.*:	d5182500 	msr	gcscr_el1, x0
.*:	d538251e 	mrs	x30, gcscr_el1
.*:	d5182520 	msr	gcspr_el1, x0
.*:	d538253e 	mrs	x30, gcspr_el1
.*:	d51c2500 	msr	gcscr_el2, x0
.*:	d53c251e 	mrs	x30, gcscr_el2
.*:	d51c2520 	msr	gcspr_el2, x0
.*:	d53c253e 	mrs	x30, gcspr_el2
.*:	d51e2500 	msr	gcscr_el3, x0
.*:	d53e251e 	mrs	x30, gcscr_el3
.*:	d51e2520 	msr	gcspr_el3, x0
.*:	d53e253e 	mrs	x30, gcspr_el3
.*:	d51b2520 	msr	gcspr_el0, x0
.*:	d53b253e 	mrs	x30, gcspr_el0
.*:	d51d2520 	msr	gcspr_el12, x0
.*:	d53d253e 	mrs	x30, gcspr_el12
.*:	d51d2500 	msr	gcscr_el12, x0
.*:	d53d251e 	mrs	x30, gcscr_el12
.*:	d5182540 	msr	gcscre0_el1, x0
.*:	d538255e 	mrs	x30, gcscre0_el1
