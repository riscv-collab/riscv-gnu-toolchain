	##-----------------------------------------------------------
	## Generating test.s from MDS
	## (c) Copyright 2010-2018 Kalray SA.
	##-----------------------------------------------------------

#	Option: '-m32'

##	target-core:	kv3-2

	.section .text

	.align 8
	.proc	main
	.global	main
main:
	abdbo $r0 = $r0r1.lo, $r0r1r2r3.x
	;;
	abdbo $r1 = $r0r1.hi, 536870911
	;;
	abdd $r0r1r2r3.y = $r2, 2305843009213693951
	;;
	abdd $r2r3.lo = $r0r1r2r3.z, $r3
	;;
	abdd $r2r3.hi = $r0r1r2r3.t, -64
	;;
	abdd $r4 = $r4r5.lo, -8589934592
	;;
	abdd.@ $r4r5r6r7.x = $r5, 536870911
	;;
	abdhq $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	abdhq.@ $r6r7.lo = $r4r5r6r7.z, 536870911
	;;
	abdsbo $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	abdsbo $r8 = $r8r9.lo, 536870911
	;;
	abdsd $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	abdsd.@ $r8r9r10r11.y = $r10, 536870911
	;;
	abdshq $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	abdshq $r10r11.hi = $r8r9r10r11.t, 536870911
	;;
	abdswp $r12 = $sp, $r13
	;;
	abdswp.@ $tp = $r14, 536870911
	;;
	abdsw $fp = $r15, $rp
	;;
	abdsw $r16 = $r16r17.lo, 536870911
	;;
	abdubo $r16r17r18r19.x = $r17, $r16r17.hi
	;;
	abdubo $r16r17r18r19.y = $r18, 536870911
	;;
	abdud $r18r19.lo = $r16r17r18r19.z, $r19
	;;
	abdud.@ $r18r19.hi = $r16r17r18r19.t, 536870911
	;;
	abduhq $r20 = $r20r21.lo, $r20r21r22r23.x
	;;
	abduhq $r21 = $r20r21.hi, 536870911
	;;
	abduwp $r20r21r22r23.y = $r22, $r22r23.lo
	;;
	abduwp.@ $r20r21r22r23.z = $r23, 536870911
	;;
	abduw $r22r23.hi = $r20r21r22r23.t, $r24
	;;
	abduw $r24r25.lo = $r24r25r26r27.x, 536870911
	;;
	abdwp $r25 = $r24r25.hi, $r24r25r26r27.y
	;;
	abdwp $r26 = $r26r27.lo, 536870911
	;;
	abdw $r24r25r26r27.z = $r27, $r26r27.hi
	;;
	abdw $r24r25r26r27.t = $r28, -64
	;;
	abdw $r28r29.lo = $r28r29r30r31.x, -8589934592
	;;
	absbo $r29 = $r28r29.hi
	;;
	absd $r28r29r30r31.y = $r30
	;;
	abshq $r30r31.lo = $r28r29r30r31.z
	;;
	abssbo $r31 = $r30r31.hi
	;;
	abssd $r28r29r30r31.t = $r32
	;;
	absshq $r32r33.lo = $r32r33r34r35.x
	;;
	absswp $r33 = $r32r33.hi
	;;
	abssw $r32r33r34r35.y = $r34
	;;
	abswp $r34r35.lo = $r32r33r34r35.z
	;;
	absw $r35 = $r34r35.hi
	;;
	acswapd.v $r32r33r34r35.t, -1125899906842624[$r36] = $r0r1
	;;
	acswapd.g $r36r37.lo, -8388608[$r36r37r38r39.x] = $r0r1r2r3.lo
	;;
	acswapd.v.s $r37, [$r36r37.hi] = $r2r3
	;;
	acswapq $r0r1r2r3.hi, -1125899906842624[$r36r37r38r39.y] = $r0r1r2r3
	;;
	acswapq.v.g $r4r5, -8388608[$r38] = $r4r5r6r7
	;;
	acswapq.s $r4r5r6r7.lo, [$r38r39.lo] = $r8r9r10r11
	;;
	acswapw.v $r36r37r38r39.z, -1125899906842624[$r39] = $r6r7
	;;
	acswapw.g $r38r39.hi, -8388608[$r36r37r38r39.t] = $r4r5r6r7.hi
	;;
	acswapw.v.s $r40, [$r40r41.lo] = $r8r9
	;;
	addbo $r40r41r42r43.x = $r41, $r40r41.hi
	;;
	addbo.@ $r40r41r42r43.y = $r42, 536870911
	;;
	addcd.i $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	addcd.i $r42r43.hi = $r40r41r42r43.t, 536870911
	;;
	addcd $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	addcd $r45 = $r44r45.hi, 536870911
	;;
	addd $r44r45r46r47.y = $r46, 2305843009213693951
	;;
	addd $r46r47.lo = $r44r45r46r47.z, $r47
	;;
	addd $r46r47.hi = $r44r45r46r47.t, -64
	;;
	addd $r48 = $r48r49.lo, -8589934592
	;;
	addd.@ $r48r49r50r51.x = $r49, 536870911
	;;
	addhq $r48r49.hi = $r48r49r50r51.y, $r50
	;;
	addhq $r50r51.lo = $r48r49r50r51.z, 536870911
	;;
	addrbod $r51 = $r50r51.hi
	;;
	addrhqd $r48r49r50r51.t = $r52
	;;
	addrwpd $r52r53.lo = $r52r53r54r55.x
	;;
	addsbo $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	addsbo.@ $r54 = $r54r55.lo, 536870911
	;;
	addsd $r52r53r54r55.z = $r55, $r54r55.hi
	;;
	addsd $r52r53r54r55.t = $r56, 536870911
	;;
	addshq $r56r57.lo = $r56r57r58r59.x, $r57
	;;
	addshq.@ $r56r57.hi = $r56r57r58r59.y, 536870911
	;;
	addswp $r58 = $r58r59.lo, $r56r57r58r59.z
	;;
	addswp $r59 = $r58r59.hi, 536870911
	;;
	addsw $r56r57r58r59.t = $r60, $r60r61.lo
	;;
	addsw $r60r61r62r63.x = $r61, 536870911
	;;
	addurbod $r60r61.hi = $r60r61r62r63.y
	;;
	addurhqd $r62 = $r62r63.lo
	;;
	addurwpd $r60r61r62r63.z = $r63
	;;
	addusbo $r62r63.hi = $r60r61r62r63.t, $r0
	;;
	addusbo.@ $r0r1.lo = $r0r1r2r3.x, 536870911
	;;
	addusd $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	addusd $r2 = $r2r3.lo, 536870911
	;;
	addushq $r0r1r2r3.z = $r3, $r2r3.hi
	;;
	addushq.@ $r0r1r2r3.t = $r4, 536870911
	;;
	adduswp $r4r5.lo = $r4r5r6r7.x, $r5
	;;
	adduswp $r4r5.hi = $r4r5r6r7.y, 536870911
	;;
	addusw $r6 = $r6r7.lo, $r4r5r6r7.z
	;;
	addusw $r7 = $r6r7.hi, 536870911
	;;
	adduwd $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	adduwd $r8r9r10r11.x = $r9, 536870911
	;;
	addwd $r8r9.hi = $r8r9r10r11.y, $r10
	;;
	addwd $r10r11.lo = $r8r9r10r11.z, 536870911
	;;
	addwp $r11 = $r10r11.hi, $r8r9r10r11.t
	;;
	addwp.@ $r12 = $sp, 536870911
	;;
	addw $r13 = $tp, $r14
	;;
	addw $fp = $r15, -64
	;;
	addw $rp = $r16, -8589934592
	;;
	addx16bo $r16r17.lo = $r16r17r18r19.x, $r17
	;;
	addx16bo $r16r17.hi = $r16r17r18r19.y, 536870911
	;;
	addx16d $r18 = $r18r19.lo, $r16r17r18r19.z
	;;
	addx16d.@ $r19 = $r18r19.hi, 536870911
	;;
	addx16hq $r16r17r18r19.t = $r20, $r20r21.lo
	;;
	addx16hq $r20r21r22r23.x = $r21, 536870911
	;;
	addx16uwd $r20r21.hi = $r20r21r22r23.y, $r22
	;;
	addx16uwd $r22r23.lo = $r20r21r22r23.z, 536870911
	;;
	addx16wd $r23 = $r22r23.hi, $r20r21r22r23.t
	;;
	addx16wd $r24 = $r24r25.lo, 536870911
	;;
	addx16wp $r24r25r26r27.x = $r25, $r24r25.hi
	;;
	addx16wp.@ $r24r25r26r27.y = $r26, 536870911
	;;
	addx16w $r26r27.lo = $r24r25r26r27.z, $r27
	;;
	addx16w $r26r27.hi = $r24r25r26r27.t, 536870911
	;;
	addx2bo $r28 = $r28r29.lo, $r28r29r30r31.x
	;;
	addx2bo $r29 = $r28r29.hi, 536870911
	;;
	addx2d $r28r29r30r31.y = $r30, $r30r31.lo
	;;
	addx2d.@ $r28r29r30r31.z = $r31, 536870911
	;;
	addx2hq $r30r31.hi = $r28r29r30r31.t, $r32
	;;
	addx2hq $r32r33.lo = $r32r33r34r35.x, 536870911
	;;
	addx2uwd $r33 = $r32r33.hi, $r32r33r34r35.y
	;;
	addx2uwd $r34 = $r34r35.lo, 536870911
	;;
	addx2wd $r32r33r34r35.z = $r35, $r34r35.hi
	;;
	addx2wd $r32r33r34r35.t = $r36, 536870911
	;;
	addx2wp $r36r37.lo = $r36r37r38r39.x, $r37
	;;
	addx2wp.@ $r36r37.hi = $r36r37r38r39.y, 536870911
	;;
	addx2w $r38 = $r38r39.lo, $r36r37r38r39.z
	;;
	addx2w $r39 = $r38r39.hi, 536870911
	;;
	addx32d $r36r37r38r39.t = $r40, $r40r41.lo
	;;
	addx32d $r40r41r42r43.x = $r41, 536870911
	;;
	addx32uwd $r40r41.hi = $r40r41r42r43.y, $r42
	;;
	addx32uwd $r42r43.lo = $r40r41r42r43.z, 536870911
	;;
	addx32wd $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	addx32wd $r44 = $r44r45.lo, 536870911
	;;
	addx32w $r44r45r46r47.x = $r45, $r44r45.hi
	;;
	addx32w $r44r45r46r47.y = $r46, 536870911
	;;
	addx4bo $r46r47.lo = $r44r45r46r47.z, $r47
	;;
	addx4bo.@ $r46r47.hi = $r44r45r46r47.t, 536870911
	;;
	addx4d $r48 = $r48r49.lo, $r48r49r50r51.x
	;;
	addx4d $r49 = $r48r49.hi, 536870911
	;;
	addx4hq $r48r49r50r51.y = $r50, $r50r51.lo
	;;
	addx4hq.@ $r48r49r50r51.z = $r51, 536870911
	;;
	addx4uwd $r50r51.hi = $r48r49r50r51.t, $r52
	;;
	addx4uwd $r52r53.lo = $r52r53r54r55.x, 536870911
	;;
	addx4wd $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	addx4wd $r54 = $r54r55.lo, 536870911
	;;
	addx4wp $r52r53r54r55.z = $r55, $r54r55.hi
	;;
	addx4wp $r52r53r54r55.t = $r56, 536870911
	;;
	addx4w $r56r57.lo = $r56r57r58r59.x, $r57
	;;
	addx4w $r56r57.hi = $r56r57r58r59.y, 536870911
	;;
	addx64d $r58 = $r58r59.lo, $r56r57r58r59.z
	;;
	addx64d.@ $r59 = $r58r59.hi, 536870911
	;;
	addx64uwd $r56r57r58r59.t = $r60, $r60r61.lo
	;;
	addx64uwd $r60r61r62r63.x = $r61, 536870911
	;;
	addx64wd $r60r61.hi = $r60r61r62r63.y, $r62
	;;
	addx64wd $r62r63.lo = $r60r61r62r63.z, 536870911
	;;
	addx64w $r63 = $r62r63.hi, $r60r61r62r63.t
	;;
	addx64w $r0 = $r0r1.lo, 536870911
	;;
	addx8bo $r0r1r2r3.x = $r1, $r0r1.hi
	;;
	addx8bo $r0r1r2r3.y = $r2, 536870911
	;;
	addx8d $r2r3.lo = $r0r1r2r3.z, $r3
	;;
	addx8d.@ $r2r3.hi = $r0r1r2r3.t, 536870911
	;;
	addx8hq $r4 = $r4r5.lo, $r4r5r6r7.x
	;;
	addx8hq $r5 = $r4r5.hi, 536870911
	;;
	addx8uwd $r4r5r6r7.y = $r6, $r6r7.lo
	;;
	addx8uwd $r4r5r6r7.z = $r7, 536870911
	;;
	addx8wd $r6r7.hi = $r4r5r6r7.t, $r8
	;;
	addx8wd $r8r9.lo = $r8r9r10r11.x, 536870911
	;;
	addx8wp $r9 = $r8r9.hi, $r8r9r10r11.y
	;;
	addx8wp.@ $r10 = $r10r11.lo, 536870911
	;;
	addx8w $r8r9r10r11.z = $r11, $r10r11.hi
	;;
	addx8w $r8r9r10r11.t = $r12, 536870911
	;;
	aladdd -1125899906842624[$sp] = $r13
	;;
	aladdd.g -8388608[$tp] = $r14
	;;
	aladdd.s [$fp] = $r15
	;;
	aladdw -1125899906842624[$rp] = $r16
	;;
	aladdw.g -8388608[$r16r17.lo] = $r16r17r18r19.x
	;;
	aladdw.s [$r17] = $r16r17.hi
	;;
	alclrd $r16r17r18r19.y = -1125899906842624[$r18]
	;;
	alclrd.g $r18r19.lo = -8388608[$r16r17r18r19.z]
	;;
	alclrd.s $r19 = [$r18r19.hi]
	;;
	alclrw $r16r17r18r19.t = -1125899906842624[$r20]
	;;
	alclrw.g $r20r21.lo = -8388608[$r20r21r22r23.x]
	;;
	alclrw.s $r21 = [$r20r21.hi]
	;;
	ald $r20r21r22r23.y = -1125899906842624[$r22]
	;;
	ald.g $r22r23.lo = -8388608[$r20r21r22r23.z]
	;;
	ald.s $r23 = [$r22r23.hi]
	;;
	alw $r20r21r22r23.t = -1125899906842624[$r24]
	;;
	alw.g $r24r25.lo = -8388608[$r24r25r26r27.x]
	;;
	alw.s $r25 = [$r24r25.hi]
	;;
	andd $r24r25r26r27.y = $r26, 2305843009213693951
	;;
	andd $r26r27.lo = $r24r25r26r27.z, $r27
	;;
	andd $r26r27.hi = $r24r25r26r27.t, -64
	;;
	andd $r28 = $r28r29.lo, -8589934592
	;;
	andd.@ $r28r29r30r31.x = $r29, 536870911
	;;
	andnd $r28r29.hi = $r28r29r30r31.y, 2305843009213693951
	;;
	andnd $r30 = $r30r31.lo, $r28r29r30r31.z
	;;
	andnd $r31 = $r30r31.hi, -64
	;;
	andnd $r28r29r30r31.t = $r32, -8589934592
	;;
	andnd.@ $r32r33.lo = $r32r33r34r35.x, 536870911
	;;
	andnw $r33 = $r32r33.hi, $r32r33r34r35.y
	;;
	andnw $r34 = $r34r35.lo, -64
	;;
	andnw $r32r33r34r35.z = $r35, -8589934592
	;;
	andrbod $r34r35.hi = $r32r33r34r35.t
	;;
	andrhqd $r36 = $r36r37.lo
	;;
	andrwpd $r36r37r38r39.x = $r37
	;;
	andw $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	andw $r38r39.lo = $r36r37r38r39.z, -64
	;;
	andw $r39 = $r38r39.hi, -8589934592
	;;
	asd -1125899906842624[$r36r37r38r39.t] = $r40
	;;
	asd.g -8388608[$r40r41.lo] = $r40r41r42r43.x
	;;
	asd.s [$r41] = $r40r41.hi
	;;
	asw -1125899906842624[$r40r41r42r43.y] = $r42
	;;
	asw.g -8388608[$r42r43.lo] = $r40r41r42r43.z
	;;
	asw.s [$r43] = $r42r43.hi
	;;
	avgbo $r40r41r42r43.t = $r44, $r44r45.lo
	;;
	avgbo $r44r45r46r47.x = $r45, 536870911
	;;
	avghq $r44r45.hi = $r44r45r46r47.y, $r46
	;;
	avgrbo $r46r47.lo = $r44r45r46r47.z, $r47
	;;
	avgrbo.@ $r46r47.hi = $r44r45r46r47.t, 536870911
	;;
	avgrhq $r48 = $r48r49.lo, $r48r49r50r51.x
	;;
	avgrubo $r49 = $r48r49.hi, $r48r49r50r51.y
	;;
	avgrubo $r50 = $r50r51.lo, 536870911
	;;
	avgruhq $r48r49r50r51.z = $r51, $r50r51.hi
	;;
	avgruwp $r48r49r50r51.t = $r52, $r52r53.lo
	;;
	avgruwp.@ $r52r53r54r55.x = $r53, 536870911
	;;
	avgruw $r52r53.hi = $r52r53r54r55.y, $r54
	;;
	avgruw $r54r55.lo = $r52r53r54r55.z, 536870911
	;;
	avgrwp $r55 = $r54r55.hi, $r52r53r54r55.t
	;;
	avgrwp $r56 = $r56r57.lo, 536870911
	;;
	avgrw $r56r57r58r59.x = $r57, $r56r57.hi
	;;
	avgrw $r56r57r58r59.y = $r58, 536870911
	;;
	avgubo $r58r59.lo = $r56r57r58r59.z, $r59
	;;
	avgubo.@ $r58r59.hi = $r56r57r58r59.t, 536870911
	;;
	avguhq $r60 = $r60r61.lo, $r60r61r62r63.x
	;;
	avguwp $r61 = $r60r61.hi, $r60r61r62r63.y
	;;
	avguwp $r62 = $r62r63.lo, 536870911
	;;
	avguw $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	avguw $r60r61r62r63.t = $r0, 536870911
	;;
	avgwp $r0r1.lo = $r0r1r2r3.x, $r1
	;;
	avgwp.@ $r0r1.hi = $r0r1r2r3.y, 536870911
	;;
	avgw $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	avgw $r3 = $r2r3.hi, 536870911
	;;
	await
	;;
	barrier
	;;
	break 0
	;;
	call -33554432
	;;
	cbsd $r0r1r2r3.t = $r4
	;;
	cbswp $r4r5.lo = $r4r5r6r7.x
	;;
	cbsw $r5 = $r4r5.hi
	;;
	cb.dnez $r4r5r6r7.y? -32768
	;;
	clrf $r6 = $r6r7.lo, 7, 7
	;;
	clsd $r4r5r6r7.z = $r7
	;;
	clswp $r6r7.hi = $r4r5r6r7.t
	;;
	clsw $r8 = $r8r9.lo
	;;
	clzd $r8r9r10r11.x = $r9
	;;
	clzwp $r8r9.hi = $r8r9r10r11.y
	;;
	clzw $r10 = $r10r11.lo
	;;
	cmovebo.nez $r8r9r10r11.z? $r11 = $r10r11.hi
	;;
	cmoved.deqz $r8r9r10r11.t? $r12 = 2305843009213693951
	;;
	cmoved.dltz $sp? $r13 = $tp
	;;
	cmoved.dgez $r14? $fp = -64
	;;
	cmoved.dlez $r15? $rp = -8589934592
	;;
	cmovehq.eqz $r16? $r16r17.lo = $r16r17r18r19.x
	;;
	cmovewp.ltz $r17? $r16r17.hi = $r16r17r18r19.y
	;;
	cmuldt $r8r9r10r11.lo = $r18, $r18r19.lo
	;;
	cmulghxdt $r10r11 = $r16r17r18r19.z, $r19
	;;
	cmulglxdt $r8r9r10r11.hi = $r18r19.hi, $r16r17r18r19.t
	;;
	cmulgmxdt $r12r13 = $r20, $r20r21.lo
	;;
	cmulxdt $r12r13r14r15.lo = $r20r21r22r23.x, $r21
	;;
	compd.ne $r20r21.hi = $r20r21r22r23.y, 2305843009213693951
	;;
	compd.eq $r22 = $r22r23.lo, $r20r21r22r23.z
	;;
	compd.lt $r23 = $r22r23.hi, -64
	;;
	compd.ge $r20r21r22r23.t = $r24, -8589934592
	;;
	compnbo.le $r24r25.lo = $r24r25r26r27.x, $r25
	;;
	compnbo.gt $r24r25.hi = $r24r25r26r27.y, 536870911
	;;
	compnd.ltu $r26 = $r26r27.lo, $r24r25r26r27.z
	;;
	compnd.geu $r27 = $r26r27.hi, 536870911
	;;
	compnhq.leu $r24r25r26r27.t = $r28, $r28r29.lo
	;;
	compnhq.gtu.@ $r28r29r30r31.x = $r29, 536870911
	;;
	compnwp.all $r28r29.hi = $r28r29r30r31.y, $r30
	;;
	compnwp.nall $r30r31.lo = $r28r29r30r31.z, 536870911
	;;
	compnw.any $r31 = $r30r31.hi, $r28r29r30r31.t
	;;
	compnw.none $r32 = $r32r33.lo, 536870911
	;;
	compuwd.ne $r32r33r34r35.x = $r33, $r32r33.hi
	;;
	compuwd.eq $r32r33r34r35.y = $r34, 536870911
	;;
	compwd.lt $r34r35.lo = $r32r33r34r35.z, $r35
	;;
	compwd.ge $r34r35.hi = $r32r33r34r35.t, 536870911
	;;
	compw.le $r36 = $r36r37.lo, $r36r37r38r39.x
	;;
	compw.gt $r37 = $r36r37.hi, 536870911
	;;
	copyd $r36r37r38r39.y = $r38
	;;
	copyo $r12r13r14r15 = $r16r17r18r19
	;;
	copyq $r14r15 = $r38r39.lo, $r36r37r38r39.z
	;;
	copyw $r39 = $r38r39.hi
	;;
	crcbellw $r36r37r38r39.t = $r40, $r40r41.lo
	;;
	crcbellw $r40r41r42r43.x = $r41, 536870911
	;;
	crcbelmw $r40r41.hi = $r40r41r42r43.y, $r42
	;;
	crcbelmw $r42r43.lo = $r40r41r42r43.z, 536870911
	;;
	crclellw $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	crclellw $r44 = $r44r45.lo, 536870911
	;;
	crclelmw $r44r45r46r47.x = $r45, $r44r45.hi
	;;
	crclelmw $r44r45r46r47.y = $r46, 536870911
	;;
	ctzd $r46r47.lo = $r44r45r46r47.z
	;;
	ctzwp $r47 = $r46r47.hi
	;;
	ctzw $r44r45r46r47.t = $r48
	;;
	d1inval
	;;
	dflushl $r48r49.lo[$r48r49r50r51.x]
	;;
	dflushl 2305843009213693951[$r49]
	;;
	dflushl -64[$r48r49.hi]
	;;
	dflushl -8589934592[$r48r49r50r51.y]
	;;
	dflushsw.l1 $r50, $r50r51.lo
	;;
	dinvall.xs $r48r49r50r51.z[$r51]
	;;
	dinvall 2305843009213693951[$r50r51.hi]
	;;
	dinvall -64[$r48r49r50r51.t]
	;;
	dinvall -8589934592[$r52]
	;;
	dinvalsw.l2 $r52r53.lo, $r52r53r54r55.x
	;;
	dot2suwdp $r12r13r14r15.hi = $r16r17, $r16r17r18r19.lo
	;;
	dot2suwd $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	dot2uwdp $r18r19 = $r16r17r18r19.hi, $r20r21
	;;
	dot2uwd $r54 = $r54r55.lo, $r52r53r54r55.z
	;;
	dot2wdp $r20r21r22r23.lo = $r22r23, $r20r21r22r23.hi
	;;
	dot2wd $r55 = $r54r55.hi, $r52r53r54r55.t
	;;
	dot2wzp $r24r25 = $r24r25r26r27.lo, $r26r27
	;;
	dot2w $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	dpurgel $r57[$r56r57.hi]
	;;
	dpurgel 2305843009213693951[$r56r57r58r59.y]
	;;
	dpurgel -64[$r58]
	;;
	dpurgel -8589934592[$r58r59.lo]
	;;
	dpurgesw.l1 $r56r57r58r59.z, $r59
	;;
	dtouchl.xs $r58r59.hi[$r56r57r58r59.t]
	;;
	dtouchl 2305843009213693951[$r60]
	;;
	dtouchl -64[$r60r61.lo]
	;;
	dtouchl -8589934592[$r60r61r62r63.x]
	;;
	errop
	;;
	extfs $r61 = $r60r61.hi, 7, 7
	;;
	extfz $r60r61r62r63.y = $r62, 7, 7
	;;
	fabsd $r62r63.lo = $r60r61r62r63.z
	;;
	fabshq $r63 = $r62r63.hi
	;;
	fabswp $r60r61r62r63.t = $r0
	;;
	fabsw $r0r1.lo = $r0r1r2r3.x
	;;
	fadddc.c.rn $r24r25r26r27.hi = $r28r29, $r28r29r30r31.lo
	;;
	fadddc.ru.s $r30r31 = $r28r29r30r31.hi, $r32r33
	;;
	fadddp.rd $r32r33r34r35.lo = $r34r35, $r32r33r34r35.hi
	;;
	faddd.rz.s $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	faddho.rna $r36r37 = $r36r37r38r39.lo, $r38r39
	;;
	faddhq.rnz.s $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	faddwc.c.ro $r3 = $r2r3.hi, $r0r1r2r3.t
	;;
	faddwcp.c.s $r36r37r38r39.hi = $r40r41, $r40r41r42r43.lo
	;;
	faddwcp.rn $r42r43 = $r40r41r42r43.hi, $r44r45
	;;
	faddwc.ru.s $r4 = $r4r5.lo, $r4r5r6r7.x
	;;
	faddwp.rd $r5 = $r4r5.hi, $r4r5r6r7.y
	;;
	faddwq.rz.s $r44r45r46r47.lo = $r46r47, $r44r45r46r47.hi
	;;
	faddw.rna $r6 = $r6r7.lo, $r4r5r6r7.z
	;;
	fcdivd.s $r7 = $r48r49
	;;
	fcdivwp $r6r7.hi = $r48r49r50r51.lo
	;;
	fcdivw.s $r4r5r6r7.t = $r50r51
	;;
	fcompd.one $r8 = $r8r9.lo, $r8r9r10r11.x
	;;
	fcompd.ueq $r9 = $r8r9.hi, 536870911
	;;
	fcompnd.oeq $r8r9r10r11.y = $r10, $r10r11.lo
	;;
	fcompnd.une $r8r9r10r11.z = $r11, 536870911
	;;
	fcompnhq.olt $r10r11.hi = $r8r9r10r11.t, $r12
	;;
	fcompnhq.uge.@ $sp = $r13, 536870911
	;;
	fcompnwp.oge $tp = $r14, $fp
	;;
	fcompnwp.ult $r15 = $rp, 536870911
	;;
	fcompnw.one $r16 = $r16r17.lo, $r16r17r18r19.x
	;;
	fcompnw.ueq $r17 = $r16r17.hi, 536870911
	;;
	fcompw.oeq $r16r17r18r19.y = $r18, $r18r19.lo
	;;
	fcompw.une $r16r17r18r19.z = $r19, 536870911
	;;
	fdot2wdp.rnz $r48r49r50r51.hi = $r52r53, $r52r53r54r55.lo
	;;
	fdot2wd.ro.s $r18r19.hi = $r16r17r18r19.t, $r20
	;;
	fdot2wzp $r54r55 = $r52r53r54r55.hi, $r56r57
	;;
	fdot2w.rn.s $r20r21.lo = $r20r21r22r23.x, $r21
	;;
	fence
	;;
	ffdmaswp.ru $r20r21.hi = $r56r57r58r59.lo, $r58r59
	;;
	ffdmaswq.rd.s $r56r57r58r59.hi = $r20r21r22r23, $r24r25r26r27
	;;
	ffdmasw.rz $r20r21r22r23.y = $r22, $r22r23.lo
	;;
	ffdmawp.rna.s $r20r21r22r23.z = $r60r61, $r60r61r62r63.lo
	;;
	ffdmawq.rnz $r62r63 = $r28r29r30r31, $r32r33r34r35
	;;
	ffdmaw.ro.s $r23 = $r22r23.hi, $r20r21r22r23.t
	;;
	ffdmdawp $r24 = $r60r61r62r63.hi, $r0r1
	;;
	ffdmdawq.rn.s $r0r1r2r3.lo = $r36r37r38r39, $r40r41r42r43
	;;
	ffdmdaw.ru $r24r25.lo = $r24r25r26r27.x, $r25
	;;
	ffdmdswp.rd.s $r24r25.hi = $r2r3, $r0r1r2r3.hi
	;;
	ffdmdswq.rz $r4r5 = $r44r45r46r47, $r48r49r50r51
	;;
	ffdmdsw.rna.s $r24r25r26r27.y = $r26, $r26r27.lo
	;;
	ffdmsawp.rnz $r24r25r26r27.z = $r4r5r6r7.lo, $r6r7
	;;
	ffdmsawq.ro.s $r4r5r6r7.hi = $r52r53r54r55, $r56r57r58r59
	;;
	ffdmsaw $r27 = $r26r27.hi, $r24r25r26r27.t
	;;
	ffdmswp.rn.s $r28 = $r8r9, $r8r9r10r11.lo
	;;
	ffdmswq.ru $r10r11 = $r60r61r62r63, $r0r1r2r3
	;;
	ffdmsw.rd.s $r28r29.lo = $r28r29r30r31.x, $r29
	;;
	ffmad.rz $r28r29.hi = $r28r29r30r31.y, $r30
	;;
	ffmaho.rna.s $r8r9r10r11.hi = $r12r13, $r12r13r14r15.lo
	;;
	ffmahq.rnz $r30r31.lo = $r28r29r30r31.z, $r31
	;;
	ffmahwq.ro.s $r14r15 = $r30r31.hi, $r28r29r30r31.t
	;;
	ffmahw $r32 = $r32r33.lo, $r32r33r34r35.x
	;;
	ffmawcp.rn.s $r12r13r14r15.hi = $r16r17, $r16r17r18r19.lo
	;;
	ffmawc.c.ru $r33 = $r32r33.hi, $r32r33r34r35.y
	;;
	ffmawdp.rd.s $r18r19 = $r34, $r34r35.lo
	;;
	ffmawd.rz $r32r33r34r35.z = $r35, $r34r35.hi
	;;
	ffmawp.rna.s $r32r33r34r35.t = $r36, $r36r37.lo
	;;
	ffmawq.rnz $r16r17r18r19.hi = $r20r21, $r20r21r22r23.lo
	;;
	ffmaw.ro.s $r36r37r38r39.x = $r37, $r36r37.hi
	;;
	ffmsd $r36r37r38r39.y = $r38, $r38r39.lo
	;;
	ffmsho.rn.s $r22r23 = $r20r21r22r23.hi, $r24r25
	;;
	ffmshq.ru $r36r37r38r39.z = $r39, $r38r39.hi
	;;
	ffmshwq.rd.s $r24r25r26r27.lo = $r36r37r38r39.t, $r40
	;;
	ffmshw.rz $r40r41.lo = $r40r41r42r43.x, $r41
	;;
	ffmswcp.rna.s $r26r27 = $r24r25r26r27.hi, $r28r29
	;;
	ffmswc.c.rnz $r40r41.hi = $r40r41r42r43.y, $r42
	;;
	ffmswdp.ro.s $r28r29r30r31.lo = $r42r43.lo, $r40r41r42r43.z
	;;
	ffmswd $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	ffmswp.rn.s $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	ffmswq.ru $r30r31 = $r28r29r30r31.hi, $r32r33
	;;
	ffmsw.rd.s $r45 = $r44r45.hi, $r44r45r46r47.y
	;;
	fixedd.rz $r46 = $r46r47.lo, 7
	;;
	fixedud.rna.s $r44r45r46r47.z = $r47, 7
	;;
	fixeduwp.rnz $r46r47.hi = $r44r45r46r47.t, 7
	;;
	fixeduw.ro.s $r48 = $r48r49.lo, 7
	;;
	fixedwp $r48r49r50r51.x = $r49, 7
	;;
	fixedw.rn.s $r48r49.hi = $r48r49r50r51.y, 7
	;;
	floatd.ru $r50 = $r50r51.lo, 7
	;;
	floatud.rd.s $r48r49r50r51.z = $r51, 7
	;;
	floatuwp.rz $r50r51.hi = $r48r49r50r51.t, 7
	;;
	floatuw.rna.s $r52 = $r52r53.lo, 7
	;;
	floatwp.rnz $r52r53r54r55.x = $r53, 7
	;;
	floatw.ro.s $r52r53.hi = $r52r53r54r55.y, 7
	;;
	fmaxd $r54 = $r54r55.lo, $r52r53r54r55.z
	;;
	fmaxhq $r55 = $r54r55.hi, $r52r53r54r55.t
	;;
	fmaxwp $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	fmaxw $r57 = $r56r57.hi, $r56r57r58r59.y
	;;
	fmind $r58 = $r58r59.lo, $r56r57r58r59.z
	;;
	fminhq $r59 = $r58r59.hi, $r56r57r58r59.t
	;;
	fminwp $r60 = $r60r61.lo, $r60r61r62r63.x
	;;
	fminw $r61 = $r60r61.hi, $r60r61r62r63.y
	;;
	fmm212w $r32r33r34r35.lo = $r62, $r62r63.lo
	;;
	fmm222w.rn.s $r34r35 = $r32r33r34r35.hi, $r36r37
	;;
	fmma212w.ru $r36r37r38r39.lo = $r60r61r62r63.z, $r63
	;;
	fmma222w.tn.rd.s $r38r39 = $r36r37r38r39.hi, $r40r41
	;;
	fmms212w.rz $r40r41r42r43.lo = $r62r63.hi, $r60r61r62r63.t
	;;
	fmms222w.nt.rna.s $r42r43 = $r40r41r42r43.hi, $r44r45
	;;
	fmuld.rnz $r0 = $r0r1.lo, $r0r1r2r3.x
	;;
	fmulho.ro.s $r44r45r46r47.lo = $r46r47, $r44r45r46r47.hi
	;;
	fmulhq $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	fmulhwq.rn.s $r48r49 = $r2, $r2r3.lo
	;;
	fmulhw.ru $r0r1r2r3.z = $r3, $r2r3.hi
	;;
	fmulwcp.rd.s $r48r49r50r51.lo = $r50r51, $r48r49r50r51.hi
	;;
	fmulwc.c.rz $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	fmulwdp.rna.s $r52r53 = $r4r5r6r7.x, $r5
	;;
	fmulwd.rnz $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	fmulwp.ro.s $r6r7.lo = $r4r5r6r7.z, $r7
	;;
	fmulwq $r52r53r54r55.lo = $r54r55, $r52r53r54r55.hi
	;;
	fmulw.rn.s $r6r7.hi = $r4r5r6r7.t, $r8
	;;
	fnarrowdwp.ru $r8r9.lo = $r56r57
	;;
	fnarrowdw.rd.s $r8r9r10r11.x = $r9
	;;
	fnarrowwhq.rz $r8r9.hi = $r56r57r58r59.lo
	;;
	fnarrowwh.rna.s $r8r9r10r11.y = $r10
	;;
	fnegd $r10r11.lo = $r8r9r10r11.z
	;;
	fneghq $r11 = $r10r11.hi
	;;
	fnegwp $r8r9r10r11.t = $r12
	;;
	fnegw $sp = $r13
	;;
	frecw.rnz $tp = $r14
	;;
	frsrw.ro.s $fp = $r15
	;;
	fsbfdc.c $r58r59 = $r56r57r58r59.hi, $r60r61
	;;
	fsbfdc.rn.s $r60r61r62r63.lo = $r62r63, $r60r61r62r63.hi
	;;
	fsbfdp.ru $r0r1 = $r0r1r2r3.lo, $r2r3
	;;
	fsbfd.rd.s $rp = $r16, $r16r17.lo
	;;
	fsbfho.rz $r0r1r2r3.hi = $r4r5, $r4r5r6r7.lo
	;;
	fsbfhq.rna.s $r16r17r18r19.x = $r17, $r16r17.hi
	;;
	fsbfwc.c.rnz $r16r17r18r19.y = $r18, $r18r19.lo
	;;
	fsbfwcp.c.ro.s $r6r7 = $r4r5r6r7.hi, $r8r9
	;;
	fsbfwcp $r8r9r10r11.lo = $r10r11, $r8r9r10r11.hi
	;;
	fsbfwc.rn.s $r16r17r18r19.z = $r19, $r18r19.hi
	;;
	fsbfwp.ru $r16r17r18r19.t = $r20, $r20r21.lo
	;;
	fsbfwq.rd.s $r12r13 = $r12r13r14r15.lo, $r14r15
	;;
	fsbfw.rz $r20r21r22r23.x = $r21, $r20r21.hi
	;;
	fsdivd.s $r20r21r22r23.y = $r12r13r14r15.hi
	;;
	fsdivwp $r22 = $r16r17
	;;
	fsdivw.s $r22r23.lo = $r16r17r18r19.lo
	;;
	fsrecd $r20r21r22r23.z = $r23
	;;
	fsrecwp.s $r22r23.hi = $r20r21r22r23.t
	;;
	fsrecw $r24 = $r24r25.lo
	;;
	fsrsrd $r24r25r26r27.x = $r25
	;;
	fsrsrwp $r24r25.hi = $r24r25r26r27.y
	;;
	fsrsrw $r26 = $r26r27.lo
	;;
	fwidenlhwp.s $r24r25r26r27.z = $r27
	;;
	fwidenlhw $r26r27.hi = $r24r25r26r27.t
	;;
	fwidenlwd.s $r28 = $r28r29.lo
	;;
	fwidenmhwp $r28r29r30r31.x = $r29
	;;
	fwidenmhw.s $r28r29.hi = $r28r29r30r31.y
	;;
	fwidenmwd $r30 = $r30r31.lo
	;;
	get $r28r29r30r31.z = $pc
	;;
	get $r31 = $pc
	;;
	goto -33554432
	;;
	i1invals $r30r31.hi[$r28r29r30r31.t]
	;;
	i1invals 2305843009213693951[$r32]
	;;
	i1invals -64[$r32r33.lo]
	;;
	i1invals -8589934592[$r32r33r34r35.x]
	;;
	i1inval
	;;
	icall $r33
	;;
	iget $r32r33.hi
	;;
	igoto $r32r33r34r35.y
	;;
	insf $r34 = $r34r35.lo, 7, 7
	;;
	landd $r32r33r34r35.z = $r35, $r34r35.hi
	;;
	landw $r32r33r34r35.t = $r36, $r36r37.lo
	;;
	landw $r36r37r38r39.x = $r37, 536870911
	;;
	lbs.xs $r36r37.hi = $r36r37r38r39.y[$r38]
	;;
	lbs.s.dgtz $r38r39.lo? $r36r37r38r39.z = -1125899906842624[$r39]
	;;
	lbs.u.odd $r38r39.hi? $r36r37r38r39.t = -8388608[$r40]
	;;
	lbs.us.even $r40r41.lo? $r40r41r42r43.x = [$r41]
	;;
	lbs $r40r41.hi = 2305843009213693951[$r40r41r42r43.y]
	;;
	lbs.s $r42 = -64[$r42r43.lo]
	;;
	lbs.u $r40r41r42r43.z = -8589934592[$r43]
	;;
	lbz.us $r42r43.hi = $r40r41r42r43.t[$r44]
	;;
	lbz.wnez $r44r45.lo? $r44r45r46r47.x = -1125899906842624[$r45]
	;;
	lbz.s.weqz $r44r45.hi? $r44r45r46r47.y = -8388608[$r46]
	;;
	lbz.u.wltz $r46r47.lo? $r44r45r46r47.z = [$r47]
	;;
	lbz.us $r46r47.hi = 2305843009213693951[$r44r45r46r47.t]
	;;
	lbz $r48 = -64[$r48r49.lo]
	;;
	lbz.s $r48r49r50r51.x = -8589934592[$r49]
	;;
	ld.u.xs $r48r49.hi = $r48r49r50r51.y[$r50]
	;;
	ld.us.wgez $r50r51.lo? $r48r49r50r51.z = -1125899906842624[$r51]
	;;
	ld.wlez $r50r51.hi? $r48r49r50r51.t = -8388608[$r52]
	;;
	ld.s.wgtz $r52r53.lo? $r52r53r54r55.x = [$r53]
	;;
	ld.u $r52r53.hi = 2305843009213693951[$r52r53r54r55.y]
	;;
	ld.us $r54 = -64[$r54r55.lo]
	;;
	ld $r52r53r54r55.z = -8589934592[$r55]
	;;
	lhs.s $r54r55.hi = $r52r53r54r55.t[$r56]
	;;
	lhs.u.dnez $r56r57.lo? $r56r57r58r59.x = -1125899906842624[$r57]
	;;
	lhs.us.deqz $r56r57.hi? $r56r57r58r59.y = -8388608[$r58]
	;;
	lhs.dltz $r58r59.lo? $r56r57r58r59.z = [$r59]
	;;
	lhs.s $r58r59.hi = 2305843009213693951[$r56r57r58r59.t]
	;;
	lhs.u $r60 = -64[$r60r61.lo]
	;;
	lhs.us $r60r61r62r63.x = -8589934592[$r61]
	;;
	lhz.xs $r60r61.hi = $r60r61r62r63.y[$r62]
	;;
	lhz.s.dgez $r62r63.lo? $r60r61r62r63.z = -1125899906842624[$r63]
	;;
	lhz.u.dlez $r62r63.hi? $r60r61r62r63.t = -8388608[$r0]
	;;
	lhz.us.dgtz $r0r1.lo? $r0r1r2r3.x = [$r1]
	;;
	lhz $r0r1.hi = 2305843009213693951[$r0r1r2r3.y]
	;;
	lhz.s $r2 = -64[$r2r3.lo]
	;;
	lhz.u $r0r1r2r3.z = -8589934592[$r3]
	;;
	lnandd $r2r3.hi = $r0r1r2r3.t, $r4
	;;
	lnandw $r4r5.lo = $r4r5r6r7.x, $r5
	;;
	lnandw $r4r5.hi = $r4r5r6r7.y, 536870911
	;;
	lnord $r6 = $r6r7.lo, $r4r5r6r7.z
	;;
	lnorw $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	lnorw $r8 = $r8r9.lo, 536870911
	;;
	loopdo $r8r9r10r11.x, -32768
	;;
	lord $r9 = $r8r9.hi, $r8r9r10r11.y
	;;
	lorw $r10 = $r10r11.lo, $r8r9r10r11.z
	;;
	lorw $r11 = $r10r11.hi, 536870911
	;;
	lo.us $r4r5r6r7 = $r8r9r10r11.t[$r12]
	;;
	lo.u0 $sp? $r8r9r10r11 = -1125899906842624[$r13]
	;;
	lo.s.u1 $tp? $r12r13r14r15 = -8388608[$r14]
	;;
	lo.u.u2 $fp? $r16r17r18r19 = [$r15]
	;;
	lo.us.odd $rp? $r20r21r22r23 = -1125899906842624[$r16]
	;;
	lo.even $r16r17.lo? $r24r25r26r27 = -8388608[$r16r17r18r19.x]
	;;
	lo.s.wnez $r17? $r28r29r30r31 = [$r16r17.hi]
	;;
	lo.u $r32r33r34r35 = 2305843009213693951[$r16r17r18r19.y]
	;;
	lo.us $r36r37r38r39 = -64[$r18]
	;;
	lo $r40r41r42r43 = -8589934592[$r18r19.lo]
	;;
	lq.s.xs $r18r19 = $r16r17r18r19.z[$r19]
	;;
	lq.u.weqz $r18r19.hi? $r16r17r18r19.hi = -1125899906842624[$r16r17r18r19.t]
	;;
	lq.us.wltz $r20? $r20r21 = -8388608[$r20r21.lo]
	;;
	lq.wgez $r20r21r22r23.x? $r20r21r22r23.lo = [$r21]
	;;
	lq.s $r22r23 = 2305843009213693951[$r20r21.hi]
	;;
	lq.u $r20r21r22r23.hi = -64[$r20r21r22r23.y]
	;;
	lq.us $r24r25 = -8589934592[$r22]
	;;
	lws $r22r23.lo = $r20r21r22r23.z[$r23]
	;;
	lws.s.wlez $r22r23.hi? $r20r21r22r23.t = -1125899906842624[$r24]
	;;
	lws.u.wgtz $r24r25.lo? $r24r25r26r27.x = -8388608[$r25]
	;;
	lws.us.dnez $r24r25.hi? $r24r25r26r27.y = [$r26]
	;;
	lws $r26r27.lo = 2305843009213693951[$r24r25r26r27.z]
	;;
	lws.s $r27 = -64[$r26r27.hi]
	;;
	lws.u $r24r25r26r27.t = -8589934592[$r28]
	;;
	lwz.us.xs $r28r29.lo = $r28r29r30r31.x[$r29]
	;;
	lwz.deqz $r28r29.hi? $r28r29r30r31.y = -1125899906842624[$r30]
	;;
	lwz.s.dltz $r30r31.lo? $r28r29r30r31.z = -8388608[$r31]
	;;
	lwz.u.dgez $r30r31.hi? $r28r29r30r31.t = [$r32]
	;;
	lwz.us $r32r33.lo = 2305843009213693951[$r32r33r34r35.x]
	;;
	lwz $r33 = -64[$r32r33.hi]
	;;
	lwz.s $r32r33r34r35.y = -8589934592[$r34]
	;;
	madddt $r24r25r26r27.lo = $r34r35.lo, $r32r33r34r35.z
	;;
	maddd $r35 = $r34r35.hi, $r32r33r34r35.t
	;;
	maddd $r36 = $r36r37.lo, 536870911
	;;
	maddhq $r36r37r38r39.x = $r37, $r36r37.hi
	;;
	maddhq $r36r37r38r39.y = $r38, 536870911
	;;
	maddhwq $r26r27 = $r38r39.lo, $r36r37r38r39.z
	;;
	maddmwq $r24r25r26r27.hi = $r28r29, $r28r29r30r31.lo
	;;
	maddsudt $r30r31 = $r39, $r38r39.hi
	;;
	maddsuhwq $r28r29r30r31.hi = $r36r37r38r39.t, $r40
	;;
	maddsumwq $r32r33 = $r32r33r34r35.lo, $r34r35
	;;
	maddsuwdp $r32r33r34r35.hi = $r40r41.lo, $r40r41r42r43.x
	;;
	maddsuwd $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	maddsuwd $r42 = $r42r43.lo, 536870911
	;;
	maddudt $r36r37 = $r40r41r42r43.z, $r43
	;;
	madduhwq $r36r37r38r39.lo = $r42r43.hi, $r40r41r42r43.t
	;;
	maddumwq $r38r39 = $r36r37r38r39.hi, $r40r41
	;;
	madduwdp $r40r41r42r43.lo = $r44, $r44r45.lo
	;;
	madduwd $r44r45r46r47.x = $r45, $r44r45.hi
	;;
	madduwd $r44r45r46r47.y = $r46, 536870911
	;;
	madduzdt $r42r43 = $r46r47.lo, $r44r45r46r47.z
	;;
	maddwdp $r40r41r42r43.hi = $r47, $r46r47.hi
	;;
	maddwd $r44r45r46r47.t = $r48, $r48r49.lo
	;;
	maddwd $r48r49r50r51.x = $r49, 536870911
	;;
	maddwp $r48r49.hi = $r48r49r50r51.y, $r50
	;;
	maddwp $r50r51.lo = $r48r49r50r51.z, 536870911
	;;
	maddwq $r44r45 = $r44r45r46r47.lo, $r46r47
	;;
	maddw $r51 = $r50r51.hi, $r48r49r50r51.t
	;;
	maddw $r52 = $r52r53.lo, 536870911
	;;
	make $r52r53r54r55.x = 2305843009213693951
	;;
	make $r53 = -549755813888
	;;
	make $r52r53.hi = -4096
	;;
	maxbo $r52r53r54r55.y = $r54, $r54r55.lo
	;;
	maxbo.@ $r52r53r54r55.z = $r55, 536870911
	;;
	maxd $r54r55.hi = $r52r53r54r55.t, 2305843009213693951
	;;
	maxd $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	maxd $r57 = $r56r57.hi, -64
	;;
	maxd $r56r57r58r59.y = $r58, -8589934592
	;;
	maxd.@ $r58r59.lo = $r56r57r58r59.z, 536870911
	;;
	maxhq $r59 = $r58r59.hi, $r56r57r58r59.t
	;;
	maxhq $r60 = $r60r61.lo, 536870911
	;;
	maxrbod $r60r61r62r63.x = $r61
	;;
	maxrhqd $r60r61.hi = $r60r61r62r63.y
	;;
	maxrwpd $r62 = $r62r63.lo
	;;
	maxubo $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	maxubo.@ $r60r61r62r63.t = $r0, 536870911
	;;
	maxud $r0r1.lo = $r0r1r2r3.x, 2305843009213693951
	;;
	maxud $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	maxud $r2 = $r2r3.lo, -64
	;;
	maxud $r0r1r2r3.z = $r3, -8589934592
	;;
	maxud.@ $r2r3.hi = $r0r1r2r3.t, 536870911
	;;
	maxuhq $r4 = $r4r5.lo, $r4r5r6r7.x
	;;
	maxuhq $r5 = $r4r5.hi, 536870911
	;;
	maxurbod $r4r5r6r7.y = $r6
	;;
	maxurhqd $r6r7.lo = $r4r5r6r7.z
	;;
	maxurwpd $r7 = $r6r7.hi
	;;
	maxuwp $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	maxuwp.@ $r8r9r10r11.x = $r9, 536870911
	;;
	maxuw $r8r9.hi = $r8r9r10r11.y, $r10
	;;
	maxuw $r10r11.lo = $r8r9r10r11.z, -64
	;;
	maxuw $r11 = $r10r11.hi, -8589934592
	;;
	maxwp $r8r9r10r11.t = $r12, $sp
	;;
	maxwp $r13 = $tp, 536870911
	;;
	maxw $r14 = $fp, $r15
	;;
	maxw $rp = $r16, -64
	;;
	maxw $r16r17.lo = $r16r17r18r19.x, -8589934592
	;;
	minbo $r17 = $r16r17.hi, $r16r17r18r19.y
	;;
	minbo.@ $r18 = $r18r19.lo, 536870911
	;;
	mind $r16r17r18r19.z = $r19, 2305843009213693951
	;;
	mind $r18r19.hi = $r16r17r18r19.t, $r20
	;;
	mind $r20r21.lo = $r20r21r22r23.x, -64
	;;
	mind $r21 = $r20r21.hi, -8589934592
	;;
	mind.@ $r20r21r22r23.y = $r22, 536870911
	;;
	minhq $r22r23.lo = $r20r21r22r23.z, $r23
	;;
	minhq $r22r23.hi = $r20r21r22r23.t, 536870911
	;;
	minrbod $r24 = $r24r25.lo
	;;
	minrhqd $r24r25r26r27.x = $r25
	;;
	minrwpd $r24r25.hi = $r24r25r26r27.y
	;;
	minubo $r26 = $r26r27.lo, $r24r25r26r27.z
	;;
	minubo.@ $r27 = $r26r27.hi, 536870911
	;;
	minud $r24r25r26r27.t = $r28, 2305843009213693951
	;;
	minud $r28r29.lo = $r28r29r30r31.x, $r29
	;;
	minud $r28r29.hi = $r28r29r30r31.y, -64
	;;
	minud $r30 = $r30r31.lo, -8589934592
	;;
	minud.@ $r28r29r30r31.z = $r31, 536870911
	;;
	minuhq $r30r31.hi = $r28r29r30r31.t, $r32
	;;
	minuhq $r32r33.lo = $r32r33r34r35.x, 536870911
	;;
	minurbod $r33 = $r32r33.hi
	;;
	minurhqd $r32r33r34r35.y = $r34
	;;
	minurwpd $r34r35.lo = $r32r33r34r35.z
	;;
	minuwp $r35 = $r34r35.hi, $r32r33r34r35.t
	;;
	minuwp.@ $r36 = $r36r37.lo, 536870911
	;;
	minuw $r36r37r38r39.x = $r37, $r36r37.hi
	;;
	minuw $r36r37r38r39.y = $r38, -64
	;;
	minuw $r38r39.lo = $r36r37r38r39.z, -8589934592
	;;
	minwp $r39 = $r38r39.hi, $r36r37r38r39.t
	;;
	minwp $r40 = $r40r41.lo, 536870911
	;;
	minw $r40r41r42r43.x = $r41, $r40r41.hi
	;;
	minw $r40r41r42r43.y = $r42, -64
	;;
	minw $r42r43.lo = $r40r41r42r43.z, -8589934592
	;;
	mm212w $r44r45r46r47.hi = $r43, $r42r43.hi
	;;
	mma212w $r48r49 = $r40r41r42r43.t, $r44
	;;
	mms212w $r48r49r50r51.lo = $r44r45.lo, $r44r45r46r47.x
	;;
	msbfdt $r50r51 = $r45, $r44r45.hi
	;;
	msbfd $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	msbfhq $r44r45r46r47.z = $r47, $r46r47.hi
	;;
	msbfhwq $r48r49r50r51.hi = $r44r45r46r47.t, $r48
	;;
	msbfmwq $r52r53 = $r52r53r54r55.lo, $r54r55
	;;
	msbfsudt $r52r53r54r55.hi = $r48r49.lo, $r48r49r50r51.x
	;;
	msbfsuhwq $r56r57 = $r49, $r48r49.hi
	;;
	msbfsumwq $r56r57r58r59.lo = $r58r59, $r56r57r58r59.hi
	;;
	msbfsuwdp $r60r61 = $r48r49r50r51.y, $r50
	;;
	msbfsuwd $r50r51.lo = $r48r49r50r51.z, $r51
	;;
	msbfsuwd $r50r51.hi = $r48r49r50r51.t, 536870911
	;;
	msbfudt $r60r61r62r63.lo = $r52, $r52r53.lo
	;;
	msbfuhwq $r62r63 = $r52r53r54r55.x, $r53
	;;
	msbfumwq $r60r61r62r63.hi = $r0r1, $r0r1r2r3.lo
	;;
	msbfuwdp $r2r3 = $r52r53.hi, $r52r53r54r55.y
	;;
	msbfuwd $r54 = $r54r55.lo, $r52r53r54r55.z
	;;
	msbfuwd $r55 = $r54r55.hi, 536870911
	;;
	msbfuzdt $r0r1r2r3.hi = $r52r53r54r55.t, $r56
	;;
	msbfwdp $r4r5 = $r56r57.lo, $r56r57r58r59.x
	;;
	msbfwd $r57 = $r56r57.hi, $r56r57r58r59.y
	;;
	msbfwd $r58 = $r58r59.lo, 536870911
	;;
	msbfwp $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	msbfwq $r4r5r6r7.lo = $r6r7, $r4r5r6r7.hi
	;;
	msbfw $r56r57r58r59.t = $r60, $r60r61.lo
	;;
	msbfw $r60r61r62r63.x = $r61, 536870911
	;;
	muldt $r8r9 = $r60r61.hi, $r60r61r62r63.y
	;;
	muld $r62 = $r62r63.lo, $r60r61r62r63.z
	;;
	muld $r63 = $r62r63.hi, 536870911
	;;
	mulhq $r60r61r62r63.t = $r0, $r0r1.lo
	;;
	mulhq $r0r1r2r3.x = $r1, 536870911
	;;
	mulhwq $r8r9r10r11.lo = $r0r1.hi, $r0r1r2r3.y
	;;
	mulmwq $r10r11 = $r8r9r10r11.hi, $r12r13
	;;
	mulsudt $r12r13r14r15.lo = $r2, $r2r3.lo
	;;
	mulsuhwq $r14r15 = $r0r1r2r3.z, $r3
	;;
	mulsumwq $r12r13r14r15.hi = $r16r17, $r16r17r18r19.lo
	;;
	mulsuwdp $r18r19 = $r2r3.hi, $r0r1r2r3.t
	;;
	mulsuwd $r4 = $r4r5.lo, $r4r5r6r7.x
	;;
	mulsuwd $r5 = $r4r5.hi, 536870911
	;;
	muludt $r16r17r18r19.hi = $r4r5r6r7.y, $r6
	;;
	muluhwq $r20r21 = $r6r7.lo, $r4r5r6r7.z
	;;
	mulumwq $r20r21r22r23.lo = $r22r23, $r20r21r22r23.hi
	;;
	muluwdp $r24r25 = $r7, $r6r7.hi
	;;
	muluwd $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	muluwd $r8r9r10r11.x = $r9, 536870911
	;;
	mulwdp $r24r25r26r27.lo = $r8r9.hi, $r8r9r10r11.y
	;;
	mulwd $r10 = $r10r11.lo, $r8r9r10r11.z
	;;
	mulwd $r11 = $r10r11.hi, 536870911
	;;
	mulwp $r8r9r10r11.t = $r12, $sp
	;;
	mulwp $r13 = $tp, 536870911
	;;
	mulwq $r26r27 = $r24r25r26r27.hi, $r28r29
	;;
	mulw $r14 = $fp, $r15
	;;
	mulw $rp = $r16, 536870911
	;;
	nandd $r16r17.lo = $r16r17r18r19.x, 2305843009213693951
	;;
	nandd $r17 = $r16r17.hi, $r16r17r18r19.y
	;;
	nandd $r18 = $r18r19.lo, -64
	;;
	nandd $r16r17r18r19.z = $r19, -8589934592
	;;
	nandd.@ $r18r19.hi = $r16r17r18r19.t, 536870911
	;;
	nandw $r20 = $r20r21.lo, $r20r21r22r23.x
	;;
	nandw $r21 = $r20r21.hi, -64
	;;
	nandw $r20r21r22r23.y = $r22, -8589934592
	;;
	negbo $r22r23.lo = $r20r21r22r23.z
	;;
	negd $r23 = $r22r23.hi
	;;
	neghq $r20r21r22r23.t = $r24
	;;
	negsbo $r24r25.lo = $r24r25r26r27.x
	;;
	negsd $r25 = $r24r25.hi
	;;
	negshq $r24r25r26r27.y = $r26
	;;
	negswp $r26r27.lo = $r24r25r26r27.z
	;;
	negsw $r27 = $r26r27.hi
	;;
	negwp $r24r25r26r27.t = $r28
	;;
	negw $r28r29.lo = $r28r29r30r31.x
	;;
	nop
	;;
	nord $r29 = $r28r29.hi, 2305843009213693951
	;;
	nord $r28r29r30r31.y = $r30, $r30r31.lo
	;;
	nord $r28r29r30r31.z = $r31, -64
	;;
	nord $r30r31.hi = $r28r29r30r31.t, -8589934592
	;;
	nord.@ $r32 = $r32r33.lo, 536870911
	;;
	norw $r32r33r34r35.x = $r33, $r32r33.hi
	;;
	norw $r32r33r34r35.y = $r34, -64
	;;
	norw $r34r35.lo = $r32r33r34r35.z, -8589934592
	;;
	notd $r35 = $r34r35.hi
	;;
	notw $r32r33r34r35.t = $r36
	;;
	nxord $r36r37.lo = $r36r37r38r39.x, 2305843009213693951
	;;
	nxord $r37 = $r36r37.hi, $r36r37r38r39.y
	;;
	nxord $r38 = $r38r39.lo, -64
	;;
	nxord $r36r37r38r39.z = $r39, -8589934592
	;;
	nxord.@ $r38r39.hi = $r36r37r38r39.t, 536870911
	;;
	nxorw $r40 = $r40r41.lo, $r40r41r42r43.x
	;;
	nxorw $r41 = $r40r41.hi, -64
	;;
	nxorw $r40r41r42r43.y = $r42, -8589934592
	;;
	ord $r42r43.lo = $r40r41r42r43.z, 2305843009213693951
	;;
	ord $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	ord $r44 = $r44r45.lo, -64
	;;
	ord $r44r45r46r47.x = $r45, -8589934592
	;;
	ord.@ $r44r45.hi = $r44r45r46r47.y, 536870911
	;;
	ornd $r46 = $r46r47.lo, 2305843009213693951
	;;
	ornd $r44r45r46r47.z = $r47, $r46r47.hi
	;;
	ornd $r44r45r46r47.t = $r48, -64
	;;
	ornd $r48r49.lo = $r48r49r50r51.x, -8589934592
	;;
	ornd.@ $r49 = $r48r49.hi, 536870911
	;;
	ornw $r48r49r50r51.y = $r50, $r50r51.lo
	;;
	ornw $r48r49r50r51.z = $r51, -64
	;;
	ornw $r50r51.hi = $r48r49r50r51.t, -8589934592
	;;
	orrbod $r52 = $r52r53.lo
	;;
	orrhqd $r52r53r54r55.x = $r53
	;;
	orrwpd $r52r53.hi = $r52r53r54r55.y
	;;
	orw $r54 = $r54r55.lo, $r52r53r54r55.z
	;;
	orw $r55 = $r54r55.hi, -64
	;;
	orw $r52r53r54r55.t = $r56, -8589934592
	;;
	pcrel $r56r57.lo = 2305843009213693951
	;;
	pcrel $r56r57r58r59.x = -549755813888
	;;
	pcrel $r57 = -4096
	;;
	ret
	;;
	rfe
	;;
	rolwps $r56r57.hi = $r56r57r58r59.y, $r58
	;;
	rolwps $r58r59.lo = $r56r57r58r59.z, 7
	;;
	rolw $r59 = $r58r59.hi, $r56r57r58r59.t
	;;
	rolw $r60 = $r60r61.lo, 7
	;;
	rorwps $r60r61r62r63.x = $r61, $r60r61.hi
	;;
	rorwps $r60r61r62r63.y = $r62, 7
	;;
	rorw $r62r63.lo = $r60r61r62r63.z, $r63
	;;
	rorw $r62r63.hi = $r60r61r62r63.t, 7
	;;
	rswap $r0 = $mmc
	;;
	rswap $r0r1.lo = $s0
	;;
	rswap $r0r1r2r3.x = $pc
	;;
	sbfbo $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	sbfbo.@ $r2 = $r2r3.lo, 536870911
	;;
	sbfcd.i $r0r1r2r3.z = $r3, $r2r3.hi
	;;
	sbfcd.i $r0r1r2r3.t = $r4, 536870911
	;;
	sbfcd $r4r5.lo = $r4r5r6r7.x, $r5
	;;
	sbfcd $r4r5.hi = $r4r5r6r7.y, 536870911
	;;
	sbfd $r6 = $r6r7.lo, 2305843009213693951
	;;
	sbfd $r4r5r6r7.z = $r7, $r6r7.hi
	;;
	sbfd $r4r5r6r7.t = $r8, -64
	;;
	sbfd $r8r9.lo = $r8r9r10r11.x, -8589934592
	;;
	sbfd.@ $r9 = $r8r9.hi, 536870911
	;;
	sbfhq $r8r9r10r11.y = $r10, $r10r11.lo
	;;
	sbfhq $r8r9r10r11.z = $r11, 536870911
	;;
	sbfsbo $r10r11.hi = $r8r9r10r11.t, $r12
	;;
	sbfsbo.@ $sp = $r13, 536870911
	;;
	sbfsd $tp = $r14, $fp
	;;
	sbfsd $r15 = $rp, 536870911
	;;
	sbfshq $r16 = $r16r17.lo, $r16r17r18r19.x
	;;
	sbfshq.@ $r17 = $r16r17.hi, 536870911
	;;
	sbfswp $r16r17r18r19.y = $r18, $r18r19.lo
	;;
	sbfswp $r16r17r18r19.z = $r19, 536870911
	;;
	sbfsw $r18r19.hi = $r16r17r18r19.t, $r20
	;;
	sbfsw $r20r21.lo = $r20r21r22r23.x, 536870911
	;;
	sbfusbo $r21 = $r20r21.hi, $r20r21r22r23.y
	;;
	sbfusbo.@ $r22 = $r22r23.lo, 536870911
	;;
	sbfusd $r20r21r22r23.z = $r23, $r22r23.hi
	;;
	sbfusd $r20r21r22r23.t = $r24, 536870911
	;;
	sbfushq $r24r25.lo = $r24r25r26r27.x, $r25
	;;
	sbfushq.@ $r24r25.hi = $r24r25r26r27.y, 536870911
	;;
	sbfuswp $r26 = $r26r27.lo, $r24r25r26r27.z
	;;
	sbfuswp $r27 = $r26r27.hi, 536870911
	;;
	sbfusw $r24r25r26r27.t = $r28, $r28r29.lo
	;;
	sbfusw $r28r29r30r31.x = $r29, 536870911
	;;
	sbfuwd $r28r29.hi = $r28r29r30r31.y, $r30
	;;
	sbfuwd $r30r31.lo = $r28r29r30r31.z, 536870911
	;;
	sbfwd $r31 = $r30r31.hi, $r28r29r30r31.t
	;;
	sbfwd $r32 = $r32r33.lo, 536870911
	;;
	sbfwp $r32r33r34r35.x = $r33, $r32r33.hi
	;;
	sbfwp.@ $r32r33r34r35.y = $r34, 536870911
	;;
	sbfw $r34r35.lo = $r32r33r34r35.z, $r35
	;;
	sbfw $r34r35.hi = $r32r33r34r35.t, -64
	;;
	sbfw $r36 = $r36r37.lo, -8589934592
	;;
	sbfx16bo $r36r37r38r39.x = $r37, $r36r37.hi
	;;
	sbfx16bo $r36r37r38r39.y = $r38, 536870911
	;;
	sbfx16d $r38r39.lo = $r36r37r38r39.z, $r39
	;;
	sbfx16d.@ $r38r39.hi = $r36r37r38r39.t, 536870911
	;;
	sbfx16hq $r40 = $r40r41.lo, $r40r41r42r43.x
	;;
	sbfx16hq $r41 = $r40r41.hi, 536870911
	;;
	sbfx16uwd $r40r41r42r43.y = $r42, $r42r43.lo
	;;
	sbfx16uwd $r40r41r42r43.z = $r43, 536870911
	;;
	sbfx16wd $r42r43.hi = $r40r41r42r43.t, $r44
	;;
	sbfx16wd $r44r45.lo = $r44r45r46r47.x, 536870911
	;;
	sbfx16wp $r45 = $r44r45.hi, $r44r45r46r47.y
	;;
	sbfx16wp.@ $r46 = $r46r47.lo, 536870911
	;;
	sbfx16w $r44r45r46r47.z = $r47, $r46r47.hi
	;;
	sbfx16w $r44r45r46r47.t = $r48, 536870911
	;;
	sbfx2bo $r48r49.lo = $r48r49r50r51.x, $r49
	;;
	sbfx2bo $r48r49.hi = $r48r49r50r51.y, 536870911
	;;
	sbfx2d $r50 = $r50r51.lo, $r48r49r50r51.z
	;;
	sbfx2d.@ $r51 = $r50r51.hi, 536870911
	;;
	sbfx2hq $r48r49r50r51.t = $r52, $r52r53.lo
	;;
	sbfx2hq $r52r53r54r55.x = $r53, 536870911
	;;
	sbfx2uwd $r52r53.hi = $r52r53r54r55.y, $r54
	;;
	sbfx2uwd $r54r55.lo = $r52r53r54r55.z, 536870911
	;;
	sbfx2wd $r55 = $r54r55.hi, $r52r53r54r55.t
	;;
	sbfx2wd $r56 = $r56r57.lo, 536870911
	;;
	sbfx2wp $r56r57r58r59.x = $r57, $r56r57.hi
	;;
	sbfx2wp.@ $r56r57r58r59.y = $r58, 536870911
	;;
	sbfx2w $r58r59.lo = $r56r57r58r59.z, $r59
	;;
	sbfx2w $r58r59.hi = $r56r57r58r59.t, 536870911
	;;
	sbfx32d $r60 = $r60r61.lo, $r60r61r62r63.x
	;;
	sbfx32d $r61 = $r60r61.hi, 536870911
	;;
	sbfx32uwd $r60r61r62r63.y = $r62, $r62r63.lo
	;;
	sbfx32uwd $r60r61r62r63.z = $r63, 536870911
	;;
	sbfx32wd $r62r63.hi = $r60r61r62r63.t, $r0
	;;
	sbfx32wd $r0r1.lo = $r0r1r2r3.x, 536870911
	;;
	sbfx32w $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	sbfx32w $r2 = $r2r3.lo, 536870911
	;;
	sbfx4bo $r0r1r2r3.z = $r3, $r2r3.hi
	;;
	sbfx4bo.@ $r0r1r2r3.t = $r4, 536870911
	;;
	sbfx4d $r4r5.lo = $r4r5r6r7.x, $r5
	;;
	sbfx4d $r4r5.hi = $r4r5r6r7.y, 536870911
	;;
	sbfx4hq $r6 = $r6r7.lo, $r4r5r6r7.z
	;;
	sbfx4hq.@ $r7 = $r6r7.hi, 536870911
	;;
	sbfx4uwd $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	sbfx4uwd $r8r9r10r11.x = $r9, 536870911
	;;
	sbfx4wd $r8r9.hi = $r8r9r10r11.y, $r10
	;;
	sbfx4wd $r10r11.lo = $r8r9r10r11.z, 536870911
	;;
	sbfx4wp $r11 = $r10r11.hi, $r8r9r10r11.t
	;;
	sbfx4wp $r12 = $sp, 536870911
	;;
	sbfx4w $r13 = $tp, $r14
	;;
	sbfx4w $fp = $r15, 536870911
	;;
	sbfx64d $rp = $r16, $r16r17.lo
	;;
	sbfx64d.@ $r16r17r18r19.x = $r17, 536870911
	;;
	sbfx64uwd $r16r17.hi = $r16r17r18r19.y, $r18
	;;
	sbfx64uwd $r18r19.lo = $r16r17r18r19.z, 536870911
	;;
	sbfx64wd $r19 = $r18r19.hi, $r16r17r18r19.t
	;;
	sbfx64wd $r20 = $r20r21.lo, 536870911
	;;
	sbfx64w $r20r21r22r23.x = $r21, $r20r21.hi
	;;
	sbfx64w $r20r21r22r23.y = $r22, 536870911
	;;
	sbfx8bo $r22r23.lo = $r20r21r22r23.z, $r23
	;;
	sbfx8bo $r22r23.hi = $r20r21r22r23.t, 536870911
	;;
	sbfx8d $r24 = $r24r25.lo, $r24r25r26r27.x
	;;
	sbfx8d.@ $r25 = $r24r25.hi, 536870911
	;;
	sbfx8hq $r24r25r26r27.y = $r26, $r26r27.lo
	;;
	sbfx8hq $r24r25r26r27.z = $r27, 536870911
	;;
	sbfx8uwd $r26r27.hi = $r24r25r26r27.t, $r28
	;;
	sbfx8uwd $r28r29.lo = $r28r29r30r31.x, 536870911
	;;
	sbfx8wd $r29 = $r28r29.hi, $r28r29r30r31.y
	;;
	sbfx8wd $r30 = $r30r31.lo, 536870911
	;;
	sbfx8wp $r28r29r30r31.z = $r31, $r30r31.hi
	;;
	sbfx8wp.@ $r28r29r30r31.t = $r32, 536870911
	;;
	sbfx8w $r32r33.lo = $r32r33r34r35.x, $r33
	;;
	sbfx8w $r32r33.hi = $r32r33r34r35.y, 536870911
	;;
	sbmm8 $r34 = $r34r35.lo, 2305843009213693951
	;;
	sbmm8 $r32r33r34r35.z = $r35, $r34r35.hi
	;;
	sbmm8 $r32r33r34r35.t = $r36, -64
	;;
	sbmm8 $r36r37.lo = $r36r37r38r39.x, -8589934592
	;;
	sbmm8.@ $r37 = $r36r37.hi, 536870911
	;;
	sbmmt8 $r36r37r38r39.y = $r38, 2305843009213693951
	;;
	sbmmt8 $r38r39.lo = $r36r37r38r39.z, $r39
	;;
	sbmmt8 $r38r39.hi = $r36r37r38r39.t, -64
	;;
	sbmmt8 $r40 = $r40r41.lo, -8589934592
	;;
	sbmmt8.@ $r40r41r42r43.x = $r41, 536870911
	;;
	sb $r40r41.hi[$r40r41r42r43.y] = $r42
	;;
	sb 2305843009213693951[$r42r43.lo] = $r40r41r42r43.z
	;;
	sb.dlez $r43? -1125899906842624[$r42r43.hi] = $r40r41r42r43.t
	;;
	sb.dgtz $r44? -8388608[$r44r45.lo] = $r44r45r46r47.x
	;;
	sb.odd $r45? [$r44r45.hi] = $r44r45r46r47.y
	;;
	sb -64[$r46] = $r46r47.lo
	;;
	sb -8589934592[$r44r45r46r47.z] = $r47
	;;
	scall $r46r47.hi
	;;
	scall 511
	;;
	sd.xs $r44r45r46r47.t[$r48] = $r48r49.lo
	;;
	sd 2305843009213693951[$r48r49r50r51.x] = $r49
	;;
	sd.even $r48r49.hi? -1125899906842624[$r48r49r50r51.y] = $r50
	;;
	sd.wnez $r50r51.lo? -8388608[$r48r49r50r51.z] = $r51
	;;
	sd.weqz $r50r51.hi? [$r48r49r50r51.t] = $r52
	;;
	sd -64[$r52r53.lo] = $r52r53r54r55.x
	;;
	sd -8589934592[$r53] = $r52r53.hi
	;;
	set $s28 = $r52r53r54r55.y
	;;
	set $ra = $r54
	;;
	set $ps = $r54r55.lo
	;;
	set $ps = $r52r53r54r55.z
	;;
	sh $r55[$r54r55.hi] = $r52r53r54r55.t
	;;
	sh 2305843009213693951[$r56] = $r56r57.lo
	;;
	sh.wltz $r56r57r58r59.x? -1125899906842624[$r57] = $r56r57.hi
	;;
	sh.wgez $r56r57r58r59.y? -8388608[$r58] = $r58r59.lo
	;;
	sh.wlez $r56r57r58r59.z? [$r59] = $r58r59.hi
	;;
	sh -64[$r56r57r58r59.t] = $r60
	;;
	sh -8589934592[$r60r61.lo] = $r60r61r62r63.x
	;;
	sleep
	;;
	sllbos $r61 = $r60r61.hi, $r60r61r62r63.y
	;;
	sllbos $r62 = $r62r63.lo, 7
	;;
	slld $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	slld $r60r61r62r63.t = $r0, 7
	;;
	sllhqs $r0r1.lo = $r0r1r2r3.x, $r1
	;;
	sllhqs $r0r1.hi = $r0r1r2r3.y, 7
	;;
	sllwps $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	sllwps $r3 = $r2r3.hi, 7
	;;
	sllw $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	sllw $r4r5r6r7.x = $r5, 7
	;;
	slsbos $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	slsbos $r6r7.lo = $r4r5r6r7.z, 7
	;;
	slsd $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	slsd $r8 = $r8r9.lo, 7
	;;
	slshqs $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	slshqs $r8r9r10r11.y = $r10, 7
	;;
	slswps $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	slswps $r10r11.hi = $r8r9r10r11.t, 7
	;;
	slsw $r12 = $sp, $r13
	;;
	slsw $tp = $r14, 7
	;;
	slusbos $fp = $r15, $rp
	;;
	slusbos $r16 = $r16r17.lo, 7
	;;
	slusd $r16r17r18r19.x = $r17, $r16r17.hi
	;;
	slusd $r16r17r18r19.y = $r18, 7
	;;
	slushqs $r18r19.lo = $r16r17r18r19.z, $r19
	;;
	slushqs $r18r19.hi = $r16r17r18r19.t, 7
	;;
	sluswps $r20 = $r20r21.lo, $r20r21r22r23.x
	;;
	sluswps $r21 = $r20r21.hi, 7
	;;
	slusw $r20r21r22r23.y = $r22, $r22r23.lo
	;;
	slusw $r20r21r22r23.z = $r23, 7
	;;
	so.xs $r22r23.hi[$r20r21r22r23.t] = $r44r45r46r47
	;;
	so 2305843009213693951[$r24] = $r48r49r50r51
	;;
	so.u3 $r24r25.lo? -1125899906842624[$r24r25r26r27.x] = $r52r53r54r55
	;;
	so.mt $r25? -8388608[$r24r25.hi] = $r56r57r58r59
	;;
	so.mf $r24r25r26r27.y? [$r26] = $r60r61r62r63
	;;
	so.wgtz $r26r27.lo? -1125899906842624[$r24r25r26r27.z] = $r0r1r2r3
	;;
	so.dnez $r27? -8388608[$r26r27.hi] = $r4r5r6r7
	;;
	so.deqz $r24r25r26r27.t? [$r28] = $r8r9r10r11
	;;
	so -64[$r28r29.lo] = $r12r13r14r15
	;;
	so -8589934592[$r28r29r30r31.x] = $r16r17r18r19
	;;
	sq $r29[$r28r29.hi] = $r28r29r30r31.lo
	;;
	sq 2305843009213693951[$r28r29r30r31.y] = $r30r31
	;;
	sq.dltz $r30? -1125899906842624[$r30r31.lo] = $r28r29r30r31.hi
	;;
	sq.dgez $r28r29r30r31.z? -8388608[$r31] = $r32r33
	;;
	sq.dlez $r30r31.hi? [$r28r29r30r31.t] = $r32r33r34r35.lo
	;;
	sq -64[$r32] = $r34r35
	;;
	sq -8589934592[$r32r33.lo] = $r32r33r34r35.hi
	;;
	srabos $r32r33r34r35.x = $r33, $r32r33.hi
	;;
	srabos $r32r33r34r35.y = $r34, 7
	;;
	srad $r34r35.lo = $r32r33r34r35.z, $r35
	;;
	srad $r34r35.hi = $r32r33r34r35.t, 7
	;;
	srahqs $r36 = $r36r37.lo, $r36r37r38r39.x
	;;
	srahqs $r37 = $r36r37.hi, 7
	;;
	srawps $r36r37r38r39.y = $r38, $r38r39.lo
	;;
	srawps $r36r37r38r39.z = $r39, 7
	;;
	sraw $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	sraw $r40r41.lo = $r40r41r42r43.x, 7
	;;
	srlbos $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	srlbos $r42 = $r42r43.lo, 7
	;;
	srld $r40r41r42r43.z = $r43, $r42r43.hi
	;;
	srld $r40r41r42r43.t = $r44, 7
	;;
	srlhqs $r44r45.lo = $r44r45r46r47.x, $r45
	;;
	srlhqs $r44r45.hi = $r44r45r46r47.y, 7
	;;
	srlwps $r46 = $r46r47.lo, $r44r45r46r47.z
	;;
	srlwps $r47 = $r46r47.hi, 7
	;;
	srlw $r44r45r46r47.t = $r48, $r48r49.lo
	;;
	srlw $r48r49r50r51.x = $r49, 7
	;;
	srsbos $r48r49.hi = $r48r49r50r51.y, $r50
	;;
	srsbos $r50r51.lo = $r48r49r50r51.z, 7
	;;
	srsd $r51 = $r50r51.hi, $r48r49r50r51.t
	;;
	srsd $r52 = $r52r53.lo, 7
	;;
	srshqs $r52r53r54r55.x = $r53, $r52r53.hi
	;;
	srshqs $r52r53r54r55.y = $r54, 7
	;;
	srswps $r54r55.lo = $r52r53r54r55.z, $r55
	;;
	srswps $r54r55.hi = $r52r53r54r55.t, 7
	;;
	srsw $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	srsw $r57 = $r56r57.hi, 7
	;;
	stop
	;;
	stsud $r56r57r58r59.y = $r58, $r58r59.lo
	;;
	stsud $r56r57r58r59.z = $r59, 536870911
	;;
	stsuhq $r58r59.hi = $r56r57r58r59.t, $r60
	;;
	stsuhq.@ $r60r61.lo = $r60r61r62r63.x, 536870911
	;;
	stsuwp $r61 = $r60r61.hi, $r60r61r62r63.y
	;;
	stsuwp $r62 = $r62r63.lo, 536870911
	;;
	stsuw $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	stsuw $r60r61r62r63.t = $r0, 536870911
	;;
	sw.xs $r0r1.lo[$r0r1r2r3.x] = $r1
	;;
	sw 2305843009213693951[$r0r1.hi] = $r0r1r2r3.y
	;;
	sw.dgtz $r2? -1125899906842624[$r2r3.lo] = $r0r1r2r3.z
	;;
	sw.odd $r3? -8388608[$r2r3.hi] = $r0r1r2r3.t
	;;
	sw.even $r4? [$r4r5.lo] = $r4r5r6r7.x
	;;
	sw -64[$r5] = $r4r5.hi
	;;
	sw -8589934592[$r4r5r6r7.y] = $r6
	;;
	sxbd $r6r7.lo = $r4r5r6r7.z
	;;
	sxhd $r7 = $r6r7.hi
	;;
	sxlbhq $r4r5r6r7.t = $r8
	;;
	sxlhwp $r8r9.lo = $r8r9r10r11.x
	;;
	sxmbhq $r9 = $r8r9.hi
	;;
	sxmhwp $r8r9r10r11.y = $r10
	;;
	sxwd $r10r11.lo = $r8r9r10r11.z
	;;
	syncgroup $r11
	;;
	tlbdinval
	;;
	tlbiinval
	;;
	tlbprobe
	;;
	tlbread
	;;
	tlbwrite
	;;
	waitit $r10r11.hi
	;;
	wfxl $ps, $r8r9r10r11.t
	;;
	wfxl $pcr, $r12
	;;
	wfxl $s1, $sp
	;;
	wfxm $s1, $r13
	;;
	wfxm $s2, $tp
	;;
	wfxm $pcr, $r14
	;;
	xaccesso $r20r21r22r23 = $a0..a1, $fp
	;;
	xaccesso $r24r25r26r27 = $a0..a3, $r15
	;;
	xaccesso $r28r29r30r31 = $a0..a7, $rp
	;;
	xaccesso $r32r33r34r35 = $a0..a15, $r16
	;;
	xaccesso $r36r37r38r39 = $a0..a31, $r16r17.lo
	;;
	xaccesso $r40r41r42r43 = $a0..a63, $r16r17r18r19.x
	;;
	xaligno $a0 = $a2..a3, $r17
	;;
	xaligno $a0a1.lo = $a4..a7, $r16r17.hi
	;;
	xaligno $a0a1a2a3.x = $a8..a15, $r16r17r18r19.y
	;;
	xaligno $a1 = $a16..a31, $r18
	;;
	xaligno $a0a1.hi = $a32..a63, $r18r19.lo
	;;
	xaligno $a0a1a2a3.y = $a0..a63, $r16r17r18r19.z
	;;
	xandno $a2 = $a2a3.lo, $a0a1a2a3.z
	;;
	xando $a3 = $a2a3.hi, $a0a1a2a3.t
	;;
	xclampwo $a4 = $a4a5.lo, $a4a5a6a7.x
	;;
	xcopyo $a5 = $a4a5.hi
	;;
	xcopyv $a0a1a2a3 = $a4a5a6a7
	;;
	xcopyx $a0a1 = $a0a1a2a3.lo
	;;
	xffma44hw.rna.s $a2a3 = $a4a5a6a7.y, $a6
	;;
	xfmaxhx $a6a7.lo = $a4a5a6a7.z, $a7
	;;
	xfminhx $a6a7.hi = $a4a5a6a7.t, $a8
	;;
	xfmma484hw.rnz $a0a1a2a3.hi = $a4a5, $a4a5a6a7.lo
	;;
	xfnarrow44wh.ro.s $a8a9.lo = $a6a7
	;;
	xfscalewo $a8a9a10a11.x = $a9, $r19
	;;
	xlo.u.q0 $a8a9a10a11 = $r18r19.hi[$r16r17r18r19.t]
	;;
	xlo.us.xs $a8a9.hi = $r20[$r20r21.lo]
	;;
	xlo.dnez.q1 $r20r21r22r23.x? $a12a13a14a15 = -1125899906842624[$r21]
	;;
	xlo.s.deqz.q2 $r20r21.hi? $a16a17a18a19 = -8388608[$r20r21r22r23.y]
	;;
	xlo.u.wnez.q3 $r22? $a20a21a22a23 = [$r22r23.lo]
	;;
	xlo.us.weqz $r20r21r22r23.z? $a8a9a10a11.y = -1125899906842624[$r23]
	;;
	xlo.mt $r22r23.hi? $a10 = -8388608[$r20r21r22r23.t]
	;;
	xlo.s.mf $r24? $a10a11.lo = [$r24r25.lo]
	;;
	xlo.u $a4..a5, $r24r25r26r27.x = -1125899906842624[$r25]
	;;
	xlo.us.q $a6..a7, $r24r25.hi = -8388608[$r24r25r26r27.y]
	;;
	xlo.d $a8..a9, $r26 = [$r26r27.lo]
	;;
	xlo.s.w $a8..a11, $r24r25r26r27.z = -1125899906842624[$r27]
	;;
	xlo.u.h $a12..a15, $r26r27.hi = -8388608[$r24r25r26r27.t]
	;;
	xlo.us.b $a16..a19, $r28 = [$r28r29.lo]
	;;
	xlo $a16..a23, $r28r29r30r31.x = -1125899906842624[$r29]
	;;
	xlo.s.q $a24..a31, $r28r29.hi = -8388608[$r28r29r30r31.y]
	;;
	xlo.u.d $a32..a39, $r30 = [$r30r31.lo]
	;;
	xlo.us.w $a32..a47, $r28r29r30r31.z = -1125899906842624[$r31]
	;;
	xlo.h $a48..a63, $r30r31.hi = -8388608[$r28r29r30r31.t]
	;;
	xlo.s.b $a0..a15, $r32 = [$r32r33.lo]
	;;
	xlo.u $a0..a31, $r32r33r34r35.x = -1125899906842624[$r33]
	;;
	xlo.us.q $a32..a63, $r32r33.hi = -8388608[$r32r33r34r35.y]
	;;
	xlo.d $a0..a31, $r34 = [$r34r35.lo]
	;;
	xlo.s.w $a0..a63, $r32r33r34r35.z = -1125899906842624[$r35]
	;;
	xlo.u.h $a0..a63, $r34r35.hi = -8388608[$r32r33r34r35.t]
	;;
	xlo.us.b $a0..a63, $r36 = [$r36r37.lo]
	;;
	xlo.q0 $a24a25a26a27 = 2305843009213693951[$r36r37r38r39.x]
	;;
	xlo.s.q1 $a28a29a30a31 = -64[$r37]
	;;
	xlo.u.q2 $a32a33a34a35 = -8589934592[$r36r37.hi]
	;;
	xlo.us $a8a9a10a11.z = 2305843009213693951[$r36r37r38r39.y]
	;;
	xlo $a11 = -64[$r38]
	;;
	xlo.s $a10a11.hi = -8589934592[$r38r39.lo]
	;;
	xmadd44bw0 $a4a5a6a7.hi = $a8a9a10a11.t, $a12
	;;
	xmadd44bw1 $a8a9 = $a12a13.lo, $a12a13a14a15.x
	;;
	xmaddifwo.rn.s $a13 = $a12a13.hi, $a12a13a14a15.y
	;;
	xmaddsu44bw0 $a8a9a10a11.lo = $a14, $a14a15.lo
	;;
	xmaddsu44bw1 $a10a11 = $a12a13a14a15.z, $a15
	;;
	xmaddu44bw0 $a8a9a10a11.hi = $a14a15.hi, $a12a13a14a15.t
	;;
	xmaddu44bw1 $a12a13 = $a16, $a16a17.lo
	;;
	xmma4164bw $a12a13a14a15.lo = $a14a15, $a12a13a14a15.hi
	;;
	xmma484bw $a16a17 = $a16a17a18a19.x, $a17
	;;
	xmmasu4164bw $a16a17a18a19.lo = $a18a19, $a16a17a18a19.hi
	;;
	xmmasu484bw $a20a21 = $a16a17.hi, $a16a17a18a19.y
	;;
	xmmau4164bw $a20a21a22a23.lo = $a22a23, $a20a21a22a23.hi
	;;
	xmmau484bw $a24a25 = $a18, $a18a19.lo
	;;
	xmmaus4164bw $a24a25a26a27.lo = $a26a27, $a24a25a26a27.hi
	;;
	xmmaus484bw $a28a29 = $a16a17a18a19.z, $a19
	;;
	xmovefd $r36r37r38r39.z = $a0_x
	;;
	xmovefo $r44r45r46r47 = $a18a19.hi
	;;
	xmovefq $r36r37 = $a0_lo
	;;
	xmovetd $a0_t = $r39
	;;
	xmovetd $a0_x = $r38r39.hi
	;;
	xmovetd $a0_y = $r36r37r38r39.t
	;;
	xmovetd $a0_z = $r40
	;;
	xmovetq $a0_lo = $r40r41.lo, $r40r41r42r43.x
	;;
	xmovetq $a0_hi = $r41, $r40r41.hi
	;;
	xmsbfifwo.ru $a16a17a18a19.t = $a20, $a20a21.lo
	;;
	xmt44d $a36a37a38a39 = $a40a41a42a43
	;;
	xnando $a20a21a22a23.x = $a21, $a20a21.hi
	;;
	xnoro $a20a21a22a23.y = $a22, $a22a23.lo
	;;
	xnxoro $a20a21a22a23.z = $a23, $a22a23.hi
	;;
	xord $r40r41r42r43.y = $r42, 2305843009213693951
	;;
	xord $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	xord $r42r43.hi = $r40r41r42r43.t, -64
	;;
	xord $r44 = $r44r45.lo, -8589934592
	;;
	xord.@ $r44r45r46r47.x = $r45, 536870911
	;;
	xorno $a20a21a22a23.t = $a24, $a24a25.lo
	;;
	xoro $a24a25a26a27.x = $a25, $a24a25.hi
	;;
	xorrbod $r44r45.hi = $r44r45r46r47.y
	;;
	xorrhqd $r46 = $r46r47.lo
	;;
	xorrwpd $r44r45r46r47.z = $r47
	;;
	xorw $r46r47.hi = $r44r45r46r47.t, $r48
	;;
	xorw $r48r49.lo = $r48r49r50r51.x, -64
	;;
	xorw $r49 = $r48r49.hi, -8589934592
	;;
	xrecvo.f $a24a25a26a27.y
	;;
	xsbmm8dq $a26 = $a26a27.lo, $a24a25a26a27.z
	;;
	xsbmmt8dq $a27 = $a26a27.hi, $a24a25a26a27.t
	;;
	xsendo.b $a28
	;;
	xsendrecvo.f.b $a28a29.lo, $a28a29a30a31.x
	;;
	xso $r48r49r50r51.y[$r50] = $a29
	;;
	xso 2305843009213693951[$r50r51.lo] = $a28a29.hi
	;;
	xso.mtc $r48r49r50r51.z? -1125899906842624[$r51] = $a28a29a30a31.y
	;;
	xso.mfc $r50r51.hi? -8388608[$r48r49r50r51.t] = $a30
	;;
	xso.dnez $r52? [$r52r53.lo] = $a30a31.lo
	;;
	xso -64[$r52r53r54r55.x] = $a28a29a30a31.z
	;;
	xso -8589934592[$r53] = $a31
	;;
	xsplatdo $a30a31.hi = 2305843009213693951
	;;
	xsplatdo $a28a29a30a31.t = -549755813888
	;;
	xsplatdo $a32 = -4096
	;;
	xsplatov.td $a44a45a46a47 = $a32a33.lo
	;;
	xsplatox.zd $a28a29a30a31.lo = $a32a33a34a35.x
	;;
	xsx48bw $a48a49a50a51 = $a33
	;;
	xtrunc48wb $a32a33.hi = $a52a53a54a55
	;;
	xxoro $a32a33a34a35.y = $a34, $a34a35.lo
	;;
	xzx48bw $a56a57a58a59 = $a32a33a34a35.z
	;;
	zxbd $r52r53.hi = $r52r53r54r55.y
	;;
	zxhd $r54 = $r54r55.lo
	;;
	zxlbhq $r52r53r54r55.z = $r55
	;;
	zxlhwp $r54r55.hi = $r52r53r54r55.t
	;;
	zxmbhq $r56 = $r56r57.lo
	;;
	zxmhwp $r56r57r58r59.x = $r57
	;;
	zxwd $r56r57.hi = $r56r57r58r59.y
	;;
	.endp	main
	.section .text
