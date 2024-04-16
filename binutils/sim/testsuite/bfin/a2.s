# mach: bfin

.include "testutils.inc"
	start

	loadsym P0, middle;

	R0 = [ P0 + 0 ];	DBGA ( R0.L , 50 );
	R0 = [ P0 + 4 ];	DBGA ( R0.L , 51 );
	R0 = [ P0 + 8 ];	DBGA ( R0.L , 52 );
	R0 = [ P0 + 12 ];	DBGA ( R0.L , 53 );
	R0 = [ P0 + 16 ];	DBGA ( R0.L , 54 );
	R0 = [ P0 + 20 ];	DBGA ( R0.L , 55 );
	R0 = [ P0 + 24 ];	DBGA ( R0.L , 56 );
	R0 = [ P0 + 28 ];	DBGA ( R0.L , 57 );

	R0 = [ P0 + -4 ];	DBGA ( R0.L , 49 );
	R0 = [ P0 + -8 ];	DBGA ( R0.L , 48 );
	R0 = [ P0 + -12 ];	DBGA ( R0.L , 47 );
	R0 = [ P0 + -16 ];	DBGA ( R0.L , 46 );
	R0 = [ P0 + -20 ];	DBGA ( R0.L , 45 );
	R0 = [ P0 + -24 ];	DBGA ( R0.L , 44 );
	R0 = [ P0 + -28 ];	DBGA ( R0.L , 43 );
	R0 = [ P0 + -32 ];	DBGA ( R0.L , 42 );

	FP = P0;

	R0 = [ FP + 0 ];	DBGA ( R0.L , 50 );
	R0 = [ FP + 4 ];	DBGA ( R0.L , 51 );
	R0 = [ FP + 8 ];	DBGA ( R0.L , 52 );
	R0 = [ FP + 12 ];	DBGA ( R0.L , 53 );
	R0 = [ FP + 16 ];	DBGA ( R0.L , 54 );
	R0 = [ FP + 20 ];	DBGA ( R0.L , 55 );
	R0 = [ FP + 24 ];	DBGA ( R0.L , 56 );
	R0 = [ FP + 28 ];	DBGA ( R0.L , 57 );
	R0 = [ FP + 32 ];	DBGA ( R0.L , 58 );
	R0 = [ FP + 36 ];	DBGA ( R0.L , 59 );
	R0 = [ FP + 40 ];	DBGA ( R0.L , 60 );
	R0 = [ FP + 44 ];	DBGA ( R0.L , 61 );
	R0 = [ FP + 48 ];	DBGA ( R0.L , 62 );
	R0 = [ FP + 52 ];	DBGA ( R0.L , 63 );
	R0 = [ FP + 56 ];	DBGA ( R0.L , 64 );
	R0 = [ FP + 60 ];	DBGA ( R0.L , 65 );

	R0 = [ FP + -4 ];	DBGA ( R0.L , 49 );
	R0 = [ FP + -8 ];	DBGA ( R0.L , 48 );
	R0 = [ FP + -12 ];	DBGA ( R0.L , 47 );
	R0 = [ FP + -16 ];	DBGA ( R0.L , 46 );
	R0 = [ FP + -20 ];	DBGA ( R0.L , 45 );
	R0 = [ FP + -24 ];	DBGA ( R0.L , 44 );
	R0 = [ FP + -28 ];	DBGA ( R0.L , 43 );
	R0 = [ FP + -32 ];	DBGA ( R0.L , 42 );
	R0 = [ FP + -36 ];	DBGA ( R0.L , 41 );
	R0 = [ FP + -40 ];	DBGA ( R0.L , 40 );
	R0 = [ FP + -44 ];	DBGA ( R0.L , 39 );
	R0 = [ FP + -48 ];	DBGA ( R0.L , 38 );
	R0 = [ FP + -52 ];	DBGA ( R0.L , 37 );
	R0 = [ FP + -56 ];	DBGA ( R0.L , 36 );
	R0 = [ FP + -60 ];	DBGA ( R0.L , 35 );
	R0 = [ FP + -64 ];	DBGA ( R0.L , 34 );
	R0 = [ FP + -68 ];	DBGA ( R0.L , 33 );
	R0 = [ FP + -72 ];	DBGA ( R0.L , 32 );
	R0 = [ FP + -76 ];	DBGA ( R0.L , 31 );
	R0 = [ FP + -80 ];	DBGA ( R0.L , 30 );
	R0 = [ FP + -84 ];	DBGA ( R0.L , 29 );
	R0 = [ FP + -88 ];	DBGA ( R0.L , 28 );
	R0 = [ FP + -92 ];	DBGA ( R0.L , 27 );
	R0 = [ FP + -96 ];	DBGA ( R0.L , 26 );
	R0 = [ FP + -100 ];	DBGA ( R0.L , 25 );
	R0 = [ FP + -104 ];	DBGA ( R0.L , 24 );
	R0 = [ FP + -108 ];	DBGA ( R0.L , 23 );
	R0 = [ FP + -112 ];	DBGA ( R0.L , 22 );
	R0 = [ FP + -116 ];	DBGA ( R0.L , 21 );

	pass

	.data
base:
	.dd 0
	.dd 1
	.dd 2
	.dd 3
	.dd 4
	.dd 5
	.dd 6
	.dd 7
	.dd 8
	.dd 9
	.dd 10
	.dd 11
	.dd 12
	.dd 13
	.dd 14
	.dd 15
	.dd 16
	.dd 17
	.dd 18
	.dd 19
	.dd 20
	.dd 21
	.dd 22
	.dd 23
	.dd 24
	.dd 25
	.dd 26
	.dd 27
	.dd 28
	.dd 29
	.dd 30
	.dd 31
	.dd 32
	.dd 33
	.dd 34
	.dd 35
	.dd 36
	.dd 37
	.dd 38
	.dd 39
	.dd 40
	.dd 41
	.dd 42
	.dd 43
	.dd 44
	.dd 45
	.dd 46
	.dd 47
	.dd 48
	.dd 49
middle:
	.dd 50
	.dd 51
	.dd 52
	.dd 53
	.dd 54
	.dd 55
	.dd 56
	.dd 57
	.dd 58
	.dd 59
	.dd 60
	.dd 61
	.dd 62
	.dd 63
	.dd 64
	.dd 65
	.dd 66
	.dd 67
	.dd 68
	.dd 69
	.dd 70
	.dd 71
	.dd 72
	.dd 73
	.dd 74
	.dd 75
	.dd 76
	.dd 77
	.dd 78
	.dd 79
	.dd 80
	.dd 81
	.dd 82
	.dd 83
	.dd 84
	.dd 85
	.dd 86
	.dd 87
	.dd 88
	.dd 89
	.dd 90
	.dd 91
	.dd 92
	.dd 93
	.dd 94
	.dd 95
	.dd 96
	.dd 97
	.dd 98
	.dd 99
