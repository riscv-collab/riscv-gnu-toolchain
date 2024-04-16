#name: Test of FEAT_THE quadword Instructions.
#as: -march=armv9.4-a+the+d128
#objdump: -dr

[^:]+:     file format .*


[^:]+:

[^:]+:
.*:	19200e04 	rcwcasp	x0, x1, x4, x5, \[x16\]
.*:	19a00e04 	rcwcaspa	x0, x1, x4, x5, \[x16\]
.*:	19e00e04 	rcwcaspal	x0, x1, x4, x5, \[x16\]
.*:	19600e04 	rcwcaspl	x0, x1, x4, x5, \[x16\]
.*:	59200e04 	rcwscasp	x0, x1, x4, x5, \[x16\]
.*:	59a00e04 	rcwscaspa	x0, x1, x4, x5, \[x16\]
.*:	59e00e04 	rcwscaspal	x0, x1, x4, x5, \[x16\]
.*:	59600e04 	rcwscaspl	x0, x1, x4, x5, \[x16\]
.*:	19259200 	rcwclrp	x0, x5, \[x16\]
.*:	19a59200 	rcwclrpa	x0, x5, \[x16\]
.*:	19e59200 	rcwclrpal	x0, x5, \[x16\]
.*:	19659200 	rcwclrpl	x0, x5, \[x16\]
.*:	59259200 	rcwsclrp	x0, x5, \[x16\]
.*:	59a59200 	rcwsclrpa	x0, x5, \[x16\]
.*:	59e59200 	rcwsclrpal	x0, x5, \[x16\]
.*:	59659200 	rcwsclrpl	x0, x5, \[x16\]
.*:	1925a200 	rcwswpp	x0, x5, \[x16\]
.*:	19a5a200 	rcwswppa	x0, x5, \[x16\]
.*:	19e5a200 	rcwswppal	x0, x5, \[x16\]
.*:	1965a200 	rcwswppl	x0, x5, \[x16\]
.*:	5925a200 	rcwsswpp	x0, x5, \[x16\]
.*:	59a5a200 	rcwsswppa	x0, x5, \[x16\]
.*:	59e5a200 	rcwsswppal	x0, x5, \[x16\]
.*:	5965a200 	rcwsswppl	x0, x5, \[x16\]
.*:	1925b200 	rcwsetp	x0, x5, \[x16\]
.*:	19a5b200 	rcwsetpa	x0, x5, \[x16\]
.*:	19e5b200 	rcwsetpal	x0, x5, \[x16\]
.*:	1965b200 	rcwsetpl	x0, x5, \[x16\]
.*:	5925b200 	rcwssetp	x0, x5, \[x16\]
.*:	59a5b200 	rcwssetpa	x0, x5, \[x16\]
.*:	59e5b200 	rcwssetpal	x0, x5, \[x16\]
.*:	5965b200 	rcwssetpl	x0, x5, \[x16\]
