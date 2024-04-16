#name: Test of Guarded Control Stack Instructions.
#as: -march=armv9.3-a+gcs
#objdump: -dr

[^:]+:     file format .*


[^:]+:

[^:]+:
.*:	d508779f 	gcspushx
.*:	d50877bf 	gcspopcx
.*:	d50877df 	gcspopx
.*:	d52b773f 	gcspopm
.*:	d503227f 	gcsb	dsync
.*:	d50b7700 	gcspushm	x0
.*:	d50b770f 	gcspushm	x15
.*:	d50b771e 	gcspushm	x30
.*:	d50b771f 	gcspushm	xzr
.*:	d50b7740 	gcsss1	x0
.*:	d50b774f 	gcsss1	x15
.*:	d50b775e 	gcsss1	x30
.*:	d50b775f 	gcsss1	xzr
.*:	d52b7760 	gcsss2	x0
.*:	d52b776f 	gcsss2	x15
.*:	d52b777e 	gcsss2	x30
.*:	d52b777f 	gcsss2	xzr
.*:	d52b7720 	gcspopm	x0
.*:	d52b772f 	gcspopm	x15
.*:	d52b773e 	gcspopm	x30
.*:	d52b773f 	gcspopm
.*:	d91f0c20 	gcsstr	x0, x1
.*:	d91f0e00 	gcsstr	x0, x16
.*:	d91f0fe0 	gcsstr	x0, sp
.*:	d91f0c2f 	gcsstr	x15, x1
.*:	d91f0e0f 	gcsstr	x15, x16
.*:	d91f0fef 	gcsstr	x15, sp
.*:	d91f0c3e 	gcsstr	x30, x1
.*:	d91f0e1e 	gcsstr	x30, x16
.*:	d91f0ffe 	gcsstr	x30, sp
.*:	d91f0c3f 	gcsstr	xzr, x1
.*:	d91f0e1f 	gcsstr	xzr, x16
.*:	d91f0fff 	gcsstr	xzr, sp
.*:	d91f1c20 	gcssttr	x0, x1
.*:	d91f1e00 	gcssttr	x0, x16
.*:	d91f1fe0 	gcssttr	x0, sp
.*:	d91f1c2f 	gcssttr	x15, x1
.*:	d91f1e0f 	gcssttr	x15, x16
.*:	d91f1fef 	gcssttr	x15, sp
.*:	d91f1c3e 	gcssttr	x30, x1
.*:	d91f1e1e 	gcssttr	x30, x16
.*:	d91f1ffe 	gcssttr	x30, sp
.*:	d91f1c3f 	gcssttr	xzr, x1
.*:	d91f1e1f 	gcssttr	xzr, x16
.*:	d91f1fff 	gcssttr	xzr, sp
