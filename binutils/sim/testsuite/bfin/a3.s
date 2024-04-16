# mach: bfin

.include "testutils.inc"
	start

	loadsym P1, middle;

	R0 = W [ P1 + -2 ] (Z);		DBGA ( R0.L , 49 );
	R0 = W [ P1 + -4 ] (Z);		DBGA ( R0.L , 48 );
	R0 = W [ P1 + -6 ] (Z);		DBGA ( R0.L , 47 );
	R0 = W [ P1 + -8 ] (Z);		DBGA ( R0.L , 46 );
	R0 = W [ P1 + -10 ] (Z);	DBGA ( R0.L , 45 );
	R0 = W [ P1 + -12 ] (Z);	DBGA ( R0.L , 44 );
	R0 = W [ P1 + -14 ] (Z);	DBGA ( R0.L , 43 );
	R0 = W [ P1 + -16 ] (Z);	DBGA ( R0.L , 42 );
	R0 = W [ P1 + -18 ] (Z);	DBGA ( R0.L , 41 );
	R0 = W [ P1 + -20 ] (Z);	DBGA ( R0.L , 40 );
	R0 = W [ P1 + -22 ] (Z);	DBGA ( R0.L , 39 );
	R0 = W [ P1 + -24 ] (Z);	DBGA ( R0.L , 38 );
	R0 = W [ P1 + -26 ] (Z);	DBGA ( R0.L , 37 );
	R0 = W [ P1 + -28 ] (Z);	DBGA ( R0.L , 36 );
	R0 = W [ P1 + -30 ] (Z);	DBGA ( R0.L , 35 );
	R0 = W [ P1 + -32 ] (Z);	DBGA ( R0.L , 34 );
	R0 = W [ P1 + -34 ] (Z);	DBGA ( R0.L , 33 );
	R0 = W [ P1 + -36 ] (Z);	DBGA ( R0.L , 32 );
	R0 = W [ P1 + -38 ] (Z);	DBGA ( R0.L , 31 );
	R0 = W [ P1 + -40 ] (Z);	DBGA ( R0.L , 30 );
	R0 = W [ P1 + -42 ] (Z);	DBGA ( R0.L , 29 );
	R0 = W [ P1 + -44 ] (Z);	DBGA ( R0.L , 28 );
	R0 = W [ P1 + -46 ] (Z);	DBGA ( R0.L , 27 );
	R0 = W [ P1 + -48 ] (Z);	DBGA ( R0.L , 26 );
	R0 = W [ P1 + -50 ] (Z);	DBGA ( R0.L , 25 );
	R0 = W [ P1 + -52 ] (Z);	DBGA ( R0.L , 24 );
	R0 = W [ P1 + -54 ] (Z);	DBGA ( R0.L , 23 );
	R0 = W [ P1 + -56 ] (Z);	DBGA ( R0.L , 22 );
	R0 = W [ P1 + -58 ] (Z);	DBGA ( R0.L , 21 );
	R0 = W [ P1 + -60 ] (Z);	DBGA ( R0.L , 20 );
	R0 = W [ P1 + -62 ] (Z);	DBGA ( R0.L , 19 );
	R0 = W [ P1 + -64 ] (Z);	DBGA ( R0.L , 18 );
	R0 = W [ P1 + -66 ] (Z);	DBGA ( R0.L , 17 );
	R0 = W [ P1 + -68 ] (Z);	DBGA ( R0.L , 16 );
	R0 = W [ P1 + -70 ] (Z);	DBGA ( R0.L , 15 );
	R0 = W [ P1 + -72 ] (Z);	DBGA ( R0.L , 14 );
	R0 = W [ P1 + -74 ] (Z);	DBGA ( R0.L , 13 );
	R0 = W [ P1 + -76 ] (Z);	DBGA ( R0.L , 12 );
	R0 = W [ P1 + -78 ] (Z);	DBGA ( R0.L , 11 );
	R0 = W [ P1 + -80 ] (Z);	DBGA ( R0.L , 10 );
	R0 = W [ P1 + -82 ] (Z);	DBGA ( R0.L , 9 );
	R0 = W [ P1 + -84 ] (Z);	DBGA ( R0.L , 8 );
	R0 = W [ P1 + -86 ] (Z);	DBGA ( R0.L , 7 );
	R0 = W [ P1 + -88 ] (Z);	DBGA ( R0.L , 6 );
	R0 = W [ P1 + -90 ] (Z);	DBGA ( R0.L , 5 );
	R0 = W [ P1 + -92 ] (Z);	DBGA ( R0.L , 4 );
	R0 = W [ P1 + -94 ] (Z);	DBGA ( R0.L , 3 );
	R0 = W [ P1 + -96 ] (Z);	DBGA ( R0.L , 2 );
	R0 = W [ P1 + -98 ] (Z);	DBGA ( R0.L , 1 );
	R0 = W [ P1 + 0 ] (Z);		DBGA ( R0.L , 50 );
	R0 = W [ P1 + 2 ] (Z);		DBGA ( R0.L , 51 );
	R0 = W [ P1 + 4 ] (Z);		DBGA ( R0.L , 52 );
	R0 = W [ P1 + 6 ] (Z);		DBGA ( R0.L , 53 );
	R0 = W [ P1 + 8 ] (Z);		DBGA ( R0.L , 54 );
	R0 = W [ P1 + 10 ] (Z);		DBGA ( R0.L , 55 );
	R0 = W [ P1 + 12 ] (Z);		DBGA ( R0.L , 56 );
	R0 = W [ P1 + 14 ] (Z);		DBGA ( R0.L , 57 );
	R0 = W [ P1 + 16 ] (Z);		DBGA ( R0.L , 58 );
	R0 = W [ P1 + 18 ] (Z);		DBGA ( R0.L , 59 );
	R0 = W [ P1 + 20 ] (Z);		DBGA ( R0.L , 60 );
	R0 = W [ P1 + 22 ] (Z);		DBGA ( R0.L , 61 );
	R0 = W [ P1 + 24 ] (Z);		DBGA ( R0.L , 62 );
	R0 = W [ P1 + 26 ] (Z);		DBGA ( R0.L , 63 );
	R0 = W [ P1 + 28 ] (Z);		DBGA ( R0.L , 64 );
	R0 = W [ P1 + 30 ] (Z);		DBGA ( R0.L , 65 );
	R0 = W [ P1 + 32 ] (Z);		DBGA ( R0.L , 66 );
	R0 = W [ P1 + 34 ] (Z);		DBGA ( R0.L , 67 );
	R0 = W [ P1 + 36 ] (Z);		DBGA ( R0.L , 68 );
	R0 = W [ P1 + 38 ] (Z);		DBGA ( R0.L , 69 );
	R0 = W [ P1 + 40 ] (Z);		DBGA ( R0.L , 70 );
	R0 = W [ P1 + 42 ] (Z);		DBGA ( R0.L , 71 );
	R0 = W [ P1 + 44 ] (Z);		DBGA ( R0.L , 72 );
	R0 = W [ P1 + 46 ] (Z);		DBGA ( R0.L , 73 );
	R0 = W [ P1 + 48 ] (Z);		DBGA ( R0.L , 74 );
	R0 = W [ P1 + 50 ] (Z);		DBGA ( R0.L , 75 );
	R0 = W [ P1 + 52 ] (Z);		DBGA ( R0.L , 76 );
	R0 = W [ P1 + 54 ] (Z);		DBGA ( R0.L , 77 );
	R0 = W [ P1 + 56 ] (Z);		DBGA ( R0.L , 78 );
	R0 = W [ P1 + 58 ] (Z);		DBGA ( R0.L , 79 );
	R0 = W [ P1 + 60 ] (Z);		DBGA ( R0.L , 80 );
	R0 = W [ P1 + 62 ] (Z);		DBGA ( R0.L , 81 );
	R0 = W [ P1 + 64 ] (Z);		DBGA ( R0.L , 82 );
	R0 = W [ P1 + 66 ] (Z);		DBGA ( R0.L , 83 );
	R0 = W [ P1 + 68 ] (Z);		DBGA ( R0.L , 84 );
	R0 = W [ P1 + 70 ] (Z);		DBGA ( R0.L , 85 );
	R0 = W [ P1 + 72 ] (Z);		DBGA ( R0.L , 86 );
	R0 = W [ P1 + 74 ] (Z);		DBGA ( R0.L , 87 );
	R0 = W [ P1 + 76 ] (Z);		DBGA ( R0.L , 88 );
	R0 = W [ P1 + 78 ] (Z);		DBGA ( R0.L , 89 );
	R0 = W [ P1 + 80 ] (Z);		DBGA ( R0.L , 90 );
	R0 = W [ P1 + 82 ] (Z);		DBGA ( R0.L , 91 );
	R0 = W [ P1 + 84 ] (Z);		DBGA ( R0.L , 92 );
	R0 = W [ P1 + 86 ] (Z);		DBGA ( R0.L , 93 );
	R0 = W [ P1 + 88 ] (Z);		DBGA ( R0.L , 94 );
	R0 = W [ P1 + 90 ] (Z);		DBGA ( R0.L , 95 );
	R0 = W [ P1 + 92 ] (Z);		DBGA ( R0.L , 96 );
	R0 = W [ P1 + 94 ] (Z);		DBGA ( R0.L , 97 );
	R0 = W [ P1 + 96 ] (Z);		DBGA ( R0.L , 98 );
	R0 = W [ P1 + 98 ] (Z);		DBGA ( R0.L , 99 );

	FP = P1;

	R0 = W [ FP + -2 ] (Z);		DBGA ( R0.L , 49 );
	R0 = W [ FP + -4 ] (Z);		DBGA ( R0.L , 48 );
	R0 = W [ FP + -6 ] (Z);		DBGA ( R0.L , 47 );
	R0 = W [ FP + -8 ] (Z);		DBGA ( R0.L , 46 );
	R0 = W [ FP + -10 ] (Z);	DBGA ( R0.L , 45 );
	R0 = W [ FP + -12 ] (Z);	DBGA ( R0.L , 44 );
	R0 = W [ FP + -14 ] (Z);	DBGA ( R0.L , 43 );
	R0 = W [ FP + -16 ] (Z);	DBGA ( R0.L , 42 );
	R0 = W [ FP + -18 ] (Z);	DBGA ( R0.L , 41 );
	R0 = W [ FP + -20 ] (Z);	DBGA ( R0.L , 40 );
	R0 = W [ FP + -22 ] (Z);	DBGA ( R0.L , 39 );
	R0 = W [ FP + -24 ] (Z);	DBGA ( R0.L , 38 );
	R0 = W [ FP + -26 ] (Z);	DBGA ( R0.L , 37 );
	R0 = W [ FP + -28 ] (Z);	DBGA ( R0.L , 36 );
	R0 = W [ FP + -30 ] (Z);	DBGA ( R0.L , 35 );
	R0 = W [ FP + -32 ] (Z);	DBGA ( R0.L , 34 );
	R0 = W [ FP + -34 ] (Z);	DBGA ( R0.L , 33 );
	R0 = W [ FP + -36 ] (Z);	DBGA ( R0.L , 32 );
	R0 = W [ FP + -38 ] (Z);	DBGA ( R0.L , 31 );
	R0 = W [ FP + -40 ] (Z);	DBGA ( R0.L , 30 );
	R0 = W [ FP + -42 ] (Z);	DBGA ( R0.L , 29 );
	R0 = W [ FP + -44 ] (Z);	DBGA ( R0.L , 28 );
	R0 = W [ FP + -46 ] (Z);	DBGA ( R0.L , 27 );
	R0 = W [ FP + -48 ] (Z);	DBGA ( R0.L , 26 );
	R0 = W [ FP + -50 ] (Z);	DBGA ( R0.L , 25 );
	R0 = W [ FP + -52 ] (Z);	DBGA ( R0.L , 24 );
	R0 = W [ FP + -54 ] (Z);	DBGA ( R0.L , 23 );
	R0 = W [ FP + -56 ] (Z);	DBGA ( R0.L , 22 );
	R0 = W [ FP + -58 ] (Z);	DBGA ( R0.L , 21 );
	R0 = W [ FP + -60 ] (Z);	DBGA ( R0.L , 20 );
	R0 = W [ FP + -62 ] (Z);	DBGA ( R0.L , 19 );
	R0 = W [ FP + -64 ] (Z);	DBGA ( R0.L , 18 );
	R0 = W [ FP + -66 ] (Z);	DBGA ( R0.L , 17 );
	R0 = W [ FP + -68 ] (Z);	DBGA ( R0.L , 16 );
	R0 = W [ FP + -70 ] (Z);	DBGA ( R0.L , 15 );
	R0 = W [ FP + -72 ] (Z);	DBGA ( R0.L , 14 );
	R0 = W [ FP + -74 ] (Z);	DBGA ( R0.L , 13 );
	R0 = W [ FP + -76 ] (Z);	DBGA ( R0.L , 12 );
	R0 = W [ FP + -78 ] (Z);	DBGA ( R0.L , 11 );
	R0 = W [ FP + -80 ] (Z);	DBGA ( R0.L , 10 );
	R0 = W [ FP + -82 ] (Z);	DBGA ( R0.L , 9 );
	R0 = W [ FP + -84 ] (Z);	DBGA ( R0.L , 8 );
	R0 = W [ FP + -86 ] (Z);	DBGA ( R0.L , 7 );
	R0 = W [ FP + -88 ] (Z);	DBGA ( R0.L , 6 );
	R0 = W [ FP + -90 ] (Z);	DBGA ( R0.L , 5 );
	R0 = W [ FP + -92 ] (Z);	DBGA ( R0.L , 4 );
	R0 = W [ FP + -94 ] (Z);	DBGA ( R0.L , 3 );
	R0 = W [ FP + -96 ] (Z);	DBGA ( R0.L , 2 );
	R0 = W [ FP + -98 ] (Z);	DBGA ( R0.L , 1 );
	R0 = W [ FP + 0 ] (Z);		DBGA ( R0.L , 50 );
	R0 = W [ FP + 2 ] (Z);		DBGA ( R0.L , 51 );
	R0 = W [ FP + 4 ] (Z);		DBGA ( R0.L , 52 );
	R0 = W [ FP + 6 ] (Z);		DBGA ( R0.L , 53 );
	R0 = W [ FP + 8 ] (Z);		DBGA ( R0.L , 54 );
	R0 = W [ FP + 10 ] (Z);		DBGA ( R0.L , 55 );
	R0 = W [ FP + 12 ] (Z);		DBGA ( R0.L , 56 );
	R0 = W [ FP + 14 ] (Z);		DBGA ( R0.L , 57 );
	R0 = W [ FP + 16 ] (Z);		DBGA ( R0.L , 58 );
	R0 = W [ FP + 18 ] (Z);		DBGA ( R0.L , 59 );
	R0 = W [ FP + 20 ] (Z);		DBGA ( R0.L , 60 );
	R0 = W [ FP + 22 ] (Z);		DBGA ( R0.L , 61 );
	R0 = W [ FP + 24 ] (Z);		DBGA ( R0.L , 62 );
	R0 = W [ FP + 26 ] (Z);		DBGA ( R0.L , 63 );
	R0 = W [ FP + 28 ] (Z);		DBGA ( R0.L , 64 );
	R0 = W [ FP + 30 ] (Z);		DBGA ( R0.L , 65 );
	R0 = W [ FP + 32 ] (Z);		DBGA ( R0.L , 66 );
	R0 = W [ FP + 34 ] (Z);		DBGA ( R0.L , 67 );
	R0 = W [ FP + 36 ] (Z);		DBGA ( R0.L , 68 );
	R0 = W [ FP + 38 ] (Z);		DBGA ( R0.L , 69 );
	R0 = W [ FP + 40 ] (Z);		DBGA ( R0.L , 70 );
	R0 = W [ FP + 42 ] (Z);		DBGA ( R0.L , 71 );
	R0 = W [ FP + 44 ] (Z);		DBGA ( R0.L , 72 );
	R0 = W [ FP + 46 ] (Z);		DBGA ( R0.L , 73 );
	R0 = W [ FP + 48 ] (Z);		DBGA ( R0.L , 74 );
	R0 = W [ FP + 50 ] (Z);		DBGA ( R0.L , 75 );
	R0 = W [ FP + 52 ] (Z);		DBGA ( R0.L , 76 );
	R0 = W [ FP + 54 ] (Z);		DBGA ( R0.L , 77 );
	R0 = W [ FP + 56 ] (Z);		DBGA ( R0.L , 78 );
	R0 = W [ FP + 58 ] (Z);		DBGA ( R0.L , 79 );
	R0 = W [ FP + 60 ] (Z);		DBGA ( R0.L , 80 );
	R0 = W [ FP + 62 ] (Z);		DBGA ( R0.L , 81 );
	R0 = W [ FP + 64 ] (Z);		DBGA ( R0.L , 82 );
	R0 = W [ FP + 66 ] (Z);		DBGA ( R0.L , 83 );
	R0 = W [ FP + 68 ] (Z);		DBGA ( R0.L , 84 );
	R0 = W [ FP + 70 ] (Z);		DBGA ( R0.L , 85 );
	R0 = W [ FP + 72 ] (Z);		DBGA ( R0.L , 86 );
	R0 = W [ FP + 74 ] (Z);		DBGA ( R0.L , 87 );
	R0 = W [ FP + 76 ] (Z);		DBGA ( R0.L , 88 );
	R0 = W [ FP + 78 ] (Z);		DBGA ( R0.L , 89 );
	R0 = W [ FP + 80 ] (Z);		DBGA ( R0.L , 90 );
	R0 = W [ FP + 82 ] (Z);		DBGA ( R0.L , 91 );
	R0 = W [ FP + 84 ] (Z);		DBGA ( R0.L , 92 );
	R0 = W [ FP + 86 ] (Z);		DBGA ( R0.L , 93 );
	R0 = W [ FP + 88 ] (Z);		DBGA ( R0.L , 94 );
	R0 = W [ FP + 90 ] (Z);		DBGA ( R0.L , 95 );
	R0 = W [ FP + 92 ] (Z);		DBGA ( R0.L , 96 );
	R0 = W [ FP + 94 ] (Z);		DBGA ( R0.L , 97 );
	R0 = W [ FP + 96 ] (Z);		DBGA ( R0.L , 98 );
	R0 = W [ FP + 98 ] (Z);		DBGA ( R0.L , 99 );
	pass

	.data

	.dw 0
	.dw 1
	.dw 2
	.dw 3
	.dw 4
	.dw 5
	.dw 6
	.dw 7
	.dw 8
	.dw 9
	.dw 10
	.dw 11
	.dw 12
	.dw 13
	.dw 14
	.dw 15
	.dw 16
	.dw 17
	.dw 18
	.dw 19
	.dw 20
	.dw 21
	.dw 22
	.dw 23
	.dw 24
	.dw 25
	.dw 26
	.dw 27
	.dw 28
	.dw 29
	.dw 30
	.dw 31
	.dw 32
	.dw 33
	.dw 34
	.dw 35
	.dw 36
	.dw 37
	.dw 38
	.dw 39
	.dw 40
	.dw 41
	.dw 42
	.dw 43
	.dw 44
	.dw 45
	.dw 46
	.dw 47
	.dw 48
	.dw 49
middle:
	.dw 50
	.dw 51
	.dw 52
	.dw 53
	.dw 54
	.dw 55
	.dw 56
	.dw 57
	.dw 58
	.dw 59
	.dw 60
	.dw 61
	.dw 62
	.dw 63
	.dw 64
	.dw 65
	.dw 66
	.dw 67
	.dw 68
	.dw 69
	.dw 70
	.dw 71
	.dw 72
	.dw 73
	.dw 74
	.dw 75
	.dw 76
	.dw 77
	.dw 78
	.dw 79
	.dw 80
	.dw 81
	.dw 82
	.dw 83
	.dw 84
	.dw 85
	.dw 86
	.dw 87
	.dw 88
	.dw 89
	.dw 90
	.dw 91
	.dw 92
	.dw 93
	.dw 94
	.dw 95
	.dw 96
	.dw 97
	.dw 98
	.dw 99
