#name: Test of SME2.1 movaz instructions.
#as: -march=armv9.4-a+sme2p1
#objdump: -dr

[^:]+:     file format .*


[^:]+:

[^:]+:
.*:	c006c260 	movaz	{z0.b-z1.b}, za0v.b \[w14, 6:7\]
.*:	c046c260 	movaz	{z0.h-z1.h}, za0v.h \[w14, 6:7\]
.*:	c086c220 	movaz	{z0.s-z1.s}, za0v.s \[w14, 2:3\]
.*:	c0c6c200 	movaz	{z0.d-z1.d}, za0v.d \[w14, 0:1\]
.*:	c00602e0 	movaz	{z0.b-z1.b}, za0h.b \[w12, 14:15\]
.*:	c0462260 	movaz	{z0.h-z1.h}, za0h.h \[w13, 6:7\]
.*:	c0864220 	movaz	{z0.s-z1.s}, za0h.s \[w14, 2:3\]
.*:	c0c66200 	movaz	{z0.d-z1.d}, za0h.d \[w15, 0:1\]
.*:	c006c260 	movaz	{z0.b-z1.b}, za0v.b \[w14, 6:7\]
.*:	c046c2e0 	movaz	{z0.h-z1.h}, za1v.h \[w14, 6:7\]
.*:	c086c2a0 	movaz	{z0.s-z1.s}, za2v.s \[w14, 2:3\]
.*:	c0c6c260 	movaz	{z0.d-z1.d}, za3v.d \[w14, 0:1\]
.*:	c00602e0 	movaz	{z0.b-z1.b}, za0h.b \[w12, 14:15\]
.*:	c04622e0 	movaz	{z0.h-z1.h}, za1h.h \[w13, 6:7\]
.*:	c08642a0 	movaz	{z0.s-z1.s}, za2h.s \[w14, 2:3\]
.*:	c0c66260 	movaz	{z0.d-z1.d}, za3h.d \[w15, 0:1\]
.*:	c006c660 	movaz	{z0.b-z3.b}, za0v.b \[w14, 12:15\]
.*:	c046c620 	movaz	{z0.h-z3.h}, za0v.h \[w14, 4:7\]
.*:	c086c600 	movaz	{z0.s-z3.s}, za0v.s \[w14, 0:3\]
.*:	c0c6c600 	movaz	{z0.d-z3.d}, za0v.d \[w14, 0:3\]
.*:	c0060660 	movaz	{z0.b-z3.b}, za0h.b \[w12, 12:15\]
.*:	c0462620 	movaz	{z0.h-z3.h}, za0h.h \[w13, 4:7\]
.*:	c0864600 	movaz	{z0.s-z3.s}, za0h.s \[w14, 0:3\]
.*:	c0c66600 	movaz	{z0.d-z3.d}, za0h.d \[w15, 0:3\]
.*:	c006c640 	movaz	{z0.b-z3.b}, za0v.b \[w14, 8:11\]
.*:	c046c660 	movaz	{z0.h-z3.h}, za1v.h \[w14, 4:7\]
.*:	c086c640 	movaz	{z0.s-z3.s}, za2v.s \[w14, 0:3\]
.*:	c0c6c660 	movaz	{z0.d-z3.d}, za3v.d \[w14, 0:3\]
.*:	c0060660 	movaz	{z0.b-z3.b}, za0h.b \[w12, 12:15\]
.*:	c0462660 	movaz	{z0.h-z3.h}, za1h.h \[w13, 4:7\]
.*:	c0864640 	movaz	{z0.s-z3.s}, za2h.s \[w14, 0:3\]
.*:	c0c66660 	movaz	{z0.d-z3.d}, za3h.d \[w15, 0:3\]
