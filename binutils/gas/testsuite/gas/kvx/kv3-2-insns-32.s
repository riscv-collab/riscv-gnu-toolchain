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
	avghq.@ $r46r47.lo = $r44r45r46r47.z, 536870911
	;;
	avgrbo $r47 = $r46r47.hi, $r44r45r46r47.t
	;;
	avgrbo $r48 = $r48r49.lo, 536870911
	;;
	avgrhq $r48r49r50r51.x = $r49, $r48r49.hi
	;;
	avgrhq.@ $r48r49r50r51.y = $r50, 536870911
	;;
	avgrubo $r50r51.lo = $r48r49r50r51.z, $r51
	;;
	avgrubo $r50r51.hi = $r48r49r50r51.t, 536870911
	;;
	avgruhq $r52 = $r52r53.lo, $r52r53r54r55.x
	;;
	avgruhq.@ $r53 = $r52r53.hi, 536870911
	;;
	avgruwp $r52r53r54r55.y = $r54, $r54r55.lo
	;;
	avgruwp $r52r53r54r55.z = $r55, 536870911
	;;
	avgruw $r54r55.hi = $r52r53r54r55.t, $r56
	;;
	avgruw $r56r57.lo = $r56r57r58r59.x, 536870911
	;;
	avgrwp $r57 = $r56r57.hi, $r56r57r58r59.y
	;;
	avgrwp.@ $r58 = $r58r59.lo, 536870911
	;;
	avgrw $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	avgrw $r56r57r58r59.t = $r60, 536870911
	;;
	avgubo $r60r61.lo = $r60r61r62r63.x, $r61
	;;
	avgubo $r60r61.hi = $r60r61r62r63.y, 536870911
	;;
	avguhq $r62 = $r62r63.lo, $r60r61r62r63.z
	;;
	avguhq.@ $r63 = $r62r63.hi, 536870911
	;;
	avguwp $r60r61r62r63.t = $r0, $r0r1.lo
	;;
	avguwp $r0r1r2r3.x = $r1, 536870911
	;;
	avguw $r0r1.hi = $r0r1r2r3.y, $r2
	;;
	avguw $r2r3.lo = $r0r1r2r3.z, 536870911
	;;
	avgwp $r3 = $r2r3.hi, $r0r1r2r3.t
	;;
	avgwp.@ $r4 = $r4r5.lo, 536870911
	;;
	avgw $r4r5r6r7.x = $r5, $r4r5.hi
	;;
	avgw $r4r5r6r7.y = $r6, 536870911
	;;
	await
	;;
	barrier
	;;
	break 0
	;;
	call -33554432
	;;
	cbsd $r6r7.lo = $r4r5r6r7.z
	;;
	cbswp $r7 = $r6r7.hi
	;;
	cbsw $r4r5r6r7.t = $r8
	;;
	cb.dnez $r8r9.lo? -32768
	;;
	clrf $r8r9r10r11.x = $r9, 7, 7
	;;
	clsd $r8r9.hi = $r8r9r10r11.y
	;;
	clswp $r10 = $r10r11.lo
	;;
	clsw $r8r9r10r11.z = $r11
	;;
	clzd $r10r11.hi = $r8r9r10r11.t
	;;
	clzwp $r12 = $sp
	;;
	clzw $r13 = $tp
	;;
	cmovebo.nez $r14? $fp = $r15
	;;
	cmoved.deqz $rp? $r16 = 2305843009213693951
	;;
	cmoved.dltz $r16r17.lo? $r16r17r18r19.x = $r17
	;;
	cmoved.dgez $r16r17.hi? $r16r17r18r19.y = -64
	;;
	cmoved.dlez $r18? $r18r19.lo = -8589934592
	;;
	cmovehq.eqz $r16r17r18r19.z? $r19 = $r18r19.hi
	;;
	cmovewp.ltz $r16r17r18r19.t? $r20 = $r20r21.lo
	;;
	cmuldt $r8r9r10r11.lo = $r20r21r22r23.x, $r21
	;;
	cmulghxdt $r10r11 = $r20r21.hi, $r20r21r22r23.y
	;;
	cmulglxdt $r8r9r10r11.hi = $r22, $r22r23.lo
	;;
	cmulgmxdt $r12r13 = $r20r21r22r23.z, $r23
	;;
	cmulxdt $r12r13r14r15.lo = $r22r23.hi, $r20r21r22r23.t
	;;
	compd.ne $r24 = $r24r25.lo, 2305843009213693951
	;;
	compd.eq $r24r25r26r27.x = $r25, $r24r25.hi
	;;
	compd.lt $r24r25r26r27.y = $r26, -64
	;;
	compd.ge $r26r27.lo = $r24r25r26r27.z, -8589934592
	;;
	compnbo.le $r27 = $r26r27.hi, $r24r25r26r27.t
	;;
	compnbo.gt $r28 = $r28r29.lo, 536870911
	;;
	compnd.ltu $r28r29r30r31.x = $r29, $r28r29.hi
	;;
	compnd.geu $r28r29r30r31.y = $r30, 536870911
	;;
	compnhq.leu $r30r31.lo = $r28r29r30r31.z, $r31
	;;
	compnhq.gtu.@ $r30r31.hi = $r28r29r30r31.t, 536870911
	;;
	compnwp.all $r32 = $r32r33.lo, $r32r33r34r35.x
	;;
	compnwp.nall $r33 = $r32r33.hi, 536870911
	;;
	compnw.any $r32r33r34r35.y = $r34, $r34r35.lo
	;;
	compnw.none $r32r33r34r35.z = $r35, 536870911
	;;
	compuwd.ne $r34r35.hi = $r32r33r34r35.t, $r36
	;;
	compuwd.eq $r36r37.lo = $r36r37r38r39.x, 536870911
	;;
	compwd.lt $r37 = $r36r37.hi, $r36r37r38r39.y
	;;
	compwd.ge $r38 = $r38r39.lo, 536870911
	;;
	compw.le $r36r37r38r39.z = $r39, $r38r39.hi
	;;
	compw.gt $r36r37r38r39.t = $r40, 536870911
	;;
	copyd $r40r41.lo = $r40r41r42r43.x
	;;
	copyo $r12r13r14r15 = $r16r17r18r19
	;;
	copyq $r14r15 = $r41, $r40r41.hi
	;;
	copyw $r40r41r42r43.y = $r42
	;;
	crcbellw $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	crcbellw $r42r43.hi = $r40r41r42r43.t, 536870911
	;;
	crcbelmw $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	crcbelmw $r45 = $r44r45.hi, 536870911
	;;
	crclellw $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	crclellw $r44r45r46r47.z = $r47, 536870911
	;;
	crclelmw $r46r47.hi = $r44r45r46r47.t, $r48
	;;
	crclelmw $r48r49.lo = $r48r49r50r51.x, 536870911
	;;
	ctzd $r49 = $r48r49.hi
	;;
	ctzwp $r48r49r50r51.y = $r50
	;;
	ctzw $r50r51.lo = $r48r49r50r51.z
	;;
	d1inval
	;;
	dflushl $r51[$r50r51.hi]
	;;
	dflushl 2305843009213693951[$r48r49r50r51.t]
	;;
	dflushl -64[$r52]
	;;
	dflushl -8589934592[$r52r53.lo]
	;;
	dflushsw.l1 $r52r53r54r55.x, $r53
	;;
	dinvall.xs $r52r53.hi[$r52r53r54r55.y]
	;;
	dinvall 2305843009213693951[$r54]
	;;
	dinvall -64[$r54r55.lo]
	;;
	dinvall -8589934592[$r52r53r54r55.z]
	;;
	dinvalsw.l2 $r55, $r54r55.hi
	;;
	dot2suwdp $r12r13r14r15.hi = $r16r17, $r16r17r18r19.lo
	;;
	dot2suwd $r52r53r54r55.t = $r56, $r56r57.lo
	;;
	dot2uwdp $r18r19 = $r16r17r18r19.hi, $r20r21
	;;
	dot2uwd $r56r57r58r59.x = $r57, $r56r57.hi
	;;
	dot2wdp $r20r21r22r23.lo = $r22r23, $r20r21r22r23.hi
	;;
	dot2wd $r56r57r58r59.y = $r58, $r58r59.lo
	;;
	dot2wzp $r24r25 = $r24r25r26r27.lo, $r26r27
	;;
	dot2w $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	dpurgel $r56r57r58r59.t[$r60]
	;;
	dpurgel 2305843009213693951[$r60r61.lo]
	;;
	dpurgel -64[$r60r61r62r63.x]
	;;
	dpurgel -8589934592[$r61]
	;;
	dpurgesw.l1 $r60r61.hi, $r60r61r62r63.y
	;;
	dtouchl.xs $r62[$r62r63.lo]
	;;
	dtouchl 2305843009213693951[$r60r61r62r63.z]
	;;
	dtouchl -64[$r63]
	;;
	dtouchl -8589934592[$r62r63.hi]
	;;
	errop
	;;
	extfs $r60r61r62r63.t = $r0, 7, 7
	;;
	extfz $r0r1.lo = $r0r1r2r3.x, 7, 7
	;;
	fabsd $r1 = $r0r1.hi
	;;
	fabshq $r0r1r2r3.y = $r2
	;;
	fabswp $r2r3.lo = $r0r1r2r3.z
	;;
	fabsw $r3 = $r2r3.hi
	;;
	fadddc.c.rn $r24r25r26r27.hi = $r28r29, $r28r29r30r31.lo
	;;
	fadddc.ru.s $r30r31 = $r28r29r30r31.hi, $r32r33
	;;
	fadddp.rd $r32r33r34r35.lo = $r34r35, $r32r33r34r35.hi
	;;
	faddd.rz.s $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	faddho.rna $r36r37 = $r36r37r38r39.lo, $r38r39
	;;
	faddhq.rnz.s $r4r5r6r7.x = $r5, $r4r5.hi
	;;
	faddwc.c.ro $r4r5r6r7.y = $r6, $r6r7.lo
	;;
	faddwcp.c.s $r36r37r38r39.hi = $r40r41, $r40r41r42r43.lo
	;;
	faddwcp.rn $r42r43 = $r40r41r42r43.hi, $r44r45
	;;
	faddwc.ru.s $r4r5r6r7.z = $r7, $r6r7.hi
	;;
	faddwp.rd $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	faddwq.rz.s $r44r45r46r47.lo = $r46r47, $r44r45r46r47.hi
	;;
	faddw.rna $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	fcdivd.s $r8r9r10r11.y = $r48r49
	;;
	fcdivwp $r10 = $r48r49r50r51.lo
	;;
	fcdivw.s $r10r11.lo = $r50r51
	;;
	fcompd.one $r8r9r10r11.z = $r11, $r10r11.hi
	;;
	fcompd.ueq $r8r9r10r11.t = $r12, 536870911
	;;
	fcompnd.oeq $sp = $r13, $tp
	;;
	fcompnd.une $r14 = $fp, 536870911
	;;
	fcompnhq.olt $r15 = $rp, $r16
	;;
	fcompnhq.uge.@ $r16r17.lo = $r16r17r18r19.x, 536870911
	;;
	fcompnwp.oge $r17 = $r16r17.hi, $r16r17r18r19.y
	;;
	fcompnwp.ult $r18 = $r18r19.lo, 536870911
	;;
	fcompnw.one $r16r17r18r19.z = $r19, $r18r19.hi
	;;
	fcompnw.ueq $r16r17r18r19.t = $r20, 536870911
	;;
	fcompw.oeq $r20r21.lo = $r20r21r22r23.x, $r21
	;;
	fcompw.une $r20r21.hi = $r20r21r22r23.y, 536870911
	;;
	fdot2wdp.rnz $r48r49r50r51.hi = $r52r53, $r52r53r54r55.lo
	;;
	fdot2wd.ro.s $r22 = $r22r23.lo, $r20r21r22r23.z
	;;
	fdot2wzp $r54r55 = $r52r53r54r55.hi, $r56r57
	;;
	fdot2w.rn.s $r23 = $r22r23.hi, $r20r21r22r23.t
	;;
	fence
	;;
	ffdmaswp.ru $r24 = $r56r57r58r59.lo, $r58r59
	;;
	ffdmaswq.rd.s $r56r57r58r59.hi = $r20r21r22r23, $r24r25r26r27
	;;
	ffdmasw.rz $r24r25.lo = $r24r25r26r27.x, $r25
	;;
	ffdmawp.rna.s $r24r25.hi = $r60r61, $r60r61r62r63.lo
	;;
	ffdmawq.rnz $r62r63 = $r28r29r30r31, $r32r33r34r35
	;;
	ffdmaw.ro.s $r24r25r26r27.y = $r26, $r26r27.lo
	;;
	ffdmdawp $r24r25r26r27.z = $r60r61r62r63.hi, $r0r1
	;;
	ffdmdawq.rn.s $r0r1r2r3.lo = $r36r37r38r39, $r40r41r42r43
	;;
	ffdmdaw.ru $r27 = $r26r27.hi, $r24r25r26r27.t
	;;
	ffdmdswp.rd.s $r28 = $r2r3, $r0r1r2r3.hi
	;;
	ffdmdswq.rz $r4r5 = $r44r45r46r47, $r48r49r50r51
	;;
	ffdmdsw.rna.s $r28r29.lo = $r28r29r30r31.x, $r29
	;;
	ffdmsawp.rnz $r28r29.hi = $r4r5r6r7.lo, $r6r7
	;;
	ffdmsawq.ro.s $r4r5r6r7.hi = $r52r53r54r55, $r56r57r58r59
	;;
	ffdmsaw $r28r29r30r31.y = $r30, $r30r31.lo
	;;
	ffdmswp.rn.s $r28r29r30r31.z = $r8r9, $r8r9r10r11.lo
	;;
	ffdmswq.ru $r10r11 = $r60r61r62r63, $r0r1r2r3
	;;
	ffdmsw.rd.s $r31 = $r30r31.hi, $r28r29r30r31.t
	;;
	ffmad.rz $r32 = $r32r33.lo, $r32r33r34r35.x
	;;
	ffmaho.rna.s $r8r9r10r11.hi = $r12r13, $r12r13r14r15.lo
	;;
	ffmahq.rnz $r33 = $r32r33.hi, $r32r33r34r35.y
	;;
	ffmahwq.ro.s $r14r15 = $r34, $r34r35.lo
	;;
	ffmahw $r32r33r34r35.z = $r35, $r34r35.hi
	;;
	ffmawcp.rn.s $r12r13r14r15.hi = $r16r17, $r16r17r18r19.lo
	;;
	ffmawc.c.ru $r32r33r34r35.t = $r36, $r36r37.lo
	;;
	ffmawdp.rd.s $r18r19 = $r36r37r38r39.x, $r37
	;;
	ffmawd.rz $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	ffmawp.rna.s $r38r39.lo = $r36r37r38r39.z, $r39
	;;
	ffmawq.rnz $r16r17r18r19.hi = $r20r21, $r20r21r22r23.lo
	;;
	ffmaw.ro.s $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	ffmsd $r40r41.lo = $r40r41r42r43.x, $r41
	;;
	ffmsho.rn.s $r22r23 = $r20r21r22r23.hi, $r24r25
	;;
	ffmshq.ru $r40r41.hi = $r40r41r42r43.y, $r42
	;;
	ffmshwq.rd.s $r24r25r26r27.lo = $r42r43.lo, $r40r41r42r43.z
	;;
	ffmshw.rz $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	ffmswcp.rna.s $r26r27 = $r24r25r26r27.hi, $r28r29
	;;
	ffmswc.c.rnz $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	ffmswdp.ro.s $r28r29r30r31.lo = $r45, $r44r45.hi
	;;
	ffmswd $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	ffmswp.rn.s $r44r45r46r47.z = $r47, $r46r47.hi
	;;
	ffmswq.ru $r30r31 = $r28r29r30r31.hi, $r32r33
	;;
	ffmsw.rd.s $r44r45r46r47.t = $r48, $r48r49.lo
	;;
	fixedd.rz $r48r49r50r51.x = $r49, 7
	;;
	fixedud.rna.s $r48r49.hi = $r48r49r50r51.y, 7
	;;
	fixeduwp.rnz $r50 = $r50r51.lo, 7
	;;
	fixeduw.ro.s $r48r49r50r51.z = $r51, 7
	;;
	fixedwp $r50r51.hi = $r48r49r50r51.t, 7
	;;
	fixedw.rn.s $r52 = $r52r53.lo, 7
	;;
	floatd.ru $r52r53r54r55.x = $r53, 7
	;;
	floatud.rd.s $r52r53.hi = $r52r53r54r55.y, 7
	;;
	floatuwp.rz $r54 = $r54r55.lo, 7
	;;
	floatuw.rna.s $r52r53r54r55.z = $r55, 7
	;;
	floatwp.rnz $r54r55.hi = $r52r53r54r55.t, 7
	;;
	floatw.ro.s $r56 = $r56r57.lo, 7
	;;
	fmaxd $r56r57r58r59.x = $r57, $r56r57.hi
	;;
	fmaxhq $r56r57r58r59.y = $r58, $r58r59.lo
	;;
	fmaxwp $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	fmaxw $r56r57r58r59.t = $r60, $r60r61.lo
	;;
	fmind $r60r61r62r63.x = $r61, $r60r61.hi
	;;
	fminhq $r60r61r62r63.y = $r62, $r62r63.lo
	;;
	fminwp $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	fminw $r60r61r62r63.t = $r0, $r0r1.lo
	;;
	fmm212w $r32r33r34r35.lo = $r0r1r2r3.x, $r1
	;;
	fmm222w.rn.s $r34r35 = $r32r33r34r35.hi, $r36r37
	;;
	fmma212w.ru $r36r37r38r39.lo = $r0r1.hi, $r0r1r2r3.y
	;;
	fmma222w.tn.rd.s $r38r39 = $r36r37r38r39.hi, $r40r41
	;;
	fmms212w.rz $r40r41r42r43.lo = $r2, $r2r3.lo
	;;
	fmms222w.nt.rna.s $r42r43 = $r40r41r42r43.hi, $r44r45
	;;
	fmuld.rnz $r0r1r2r3.z = $r3, $r2r3.hi
	;;
	fmulho.ro.s $r44r45r46r47.lo = $r46r47, $r44r45r46r47.hi
	;;
	fmulhq $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	fmulhwq.rn.s $r48r49 = $r4r5r6r7.x, $r5
	;;
	fmulhw.ru $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	fmulwcp.rd.s $r48r49r50r51.lo = $r50r51, $r48r49r50r51.hi
	;;
	fmulwc.c.rz $r6r7.lo = $r4r5r6r7.z, $r7
	;;
	fmulwdp.rna.s $r52r53 = $r6r7.hi, $r4r5r6r7.t
	;;
	fmulwd.rnz $r8 = $r8r9.lo, $r8r9r10r11.x
	;;
	fmulwp.ro.s $r9 = $r8r9.hi, $r8r9r10r11.y
	;;
	fmulwq $r52r53r54r55.lo = $r54r55, $r52r53r54r55.hi
	;;
	fmulw.rn.s $r10 = $r10r11.lo, $r8r9r10r11.z
	;;
	fnarrowdwp.ru $r11 = $r56r57
	;;
	fnarrowdw.rd.s $r10r11.hi = $r8r9r10r11.t
	;;
	fnarrowwhq.rz $r12 = $r56r57r58r59.lo
	;;
	fnarrowwh.rna.s $sp = $r13
	;;
	fnegd $tp = $r14
	;;
	fneghq $fp = $r15
	;;
	fnegwp $rp = $r16
	;;
	fnegw $r16r17.lo = $r16r17r18r19.x
	;;
	frecw.rnz $r17 = $r16r17.hi
	;;
	frsrw.ro.s $r16r17r18r19.y = $r18
	;;
	fsbfdc.c $r58r59 = $r56r57r58r59.hi, $r60r61
	;;
	fsbfdc.rn.s $r60r61r62r63.lo = $r62r63, $r60r61r62r63.hi
	;;
	fsbfdp.ru $r0r1 = $r0r1r2r3.lo, $r2r3
	;;
	fsbfd.rd.s $r18r19.lo = $r16r17r18r19.z, $r19
	;;
	fsbfho.rz $r0r1r2r3.hi = $r4r5, $r4r5r6r7.lo
	;;
	fsbfhq.rna.s $r18r19.hi = $r16r17r18r19.t, $r20
	;;
	fsbfwc.c.rnz $r20r21.lo = $r20r21r22r23.x, $r21
	;;
	fsbfwcp.c.ro.s $r6r7 = $r4r5r6r7.hi, $r8r9
	;;
	fsbfwcp $r8r9r10r11.lo = $r10r11, $r8r9r10r11.hi
	;;
	fsbfwc.rn.s $r20r21.hi = $r20r21r22r23.y, $r22
	;;
	fsbfwp.ru $r22r23.lo = $r20r21r22r23.z, $r23
	;;
	fsbfwq.rd.s $r12r13 = $r12r13r14r15.lo, $r14r15
	;;
	fsbfw.rz $r22r23.hi = $r20r21r22r23.t, $r24
	;;
	fsdivd.s $r24r25.lo = $r12r13r14r15.hi
	;;
	fsdivwp $r24r25r26r27.x = $r16r17
	;;
	fsdivw.s $r25 = $r16r17r18r19.lo
	;;
	fsrecd $r24r25.hi = $r24r25r26r27.y
	;;
	fsrecwp.s $r26 = $r26r27.lo
	;;
	fsrecw $r24r25r26r27.z = $r27
	;;
	fsrsrd $r26r27.hi = $r24r25r26r27.t
	;;
	fsrsrwp $r28 = $r28r29.lo
	;;
	fsrsrw $r28r29r30r31.x = $r29
	;;
	fwidenlhwp.s $r28r29.hi = $r28r29r30r31.y
	;;
	fwidenlhw $r30 = $r30r31.lo
	;;
	fwidenlwd.s $r28r29r30r31.z = $r31
	;;
	fwidenmhwp $r30r31.hi = $r28r29r30r31.t
	;;
	fwidenmhw.s $r32 = $r32r33.lo
	;;
	fwidenmwd $r32r33r34r35.x = $r33
	;;
	get $r32r33.hi = $pc
	;;
	get $r32r33r34r35.y = $pc
	;;
	goto -33554432
	;;
	i1invals $r34[$r34r35.lo]
	;;
	i1invals 2305843009213693951[$r32r33r34r35.z]
	;;
	i1invals -64[$r35]
	;;
	i1invals -8589934592[$r34r35.hi]
	;;
	i1inval
	;;
	icall $r32r33r34r35.t
	;;
	iget $r36
	;;
	igoto $r36r37.lo
	;;
	insf $r36r37r38r39.x = $r37, 7, 7
	;;
	landd $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	landw $r38r39.lo = $r36r37r38r39.z, $r39
	;;
	landw $r38r39.hi = $r36r37r38r39.t, 536870911
	;;
	lbs.xs $r40 = $r40r41.lo[$r40r41r42r43.x]
	;;
	lbs.s.dgtz $r41? $r40r41.hi = -1125899906842624[$r40r41r42r43.y]
	;;
	lbs.u.odd $r42? $r42r43.lo = -8388608[$r40r41r42r43.z]
	;;
	lbs.us.even $r43? $r42r43.hi = [$r40r41r42r43.t]
	;;
	lbs $r44 = 2305843009213693951[$r44r45.lo]
	;;
	lbs.s $r44r45r46r47.x = -64[$r45]
	;;
	lbs.u $r44r45.hi = -8589934592[$r44r45r46r47.y]
	;;
	lbz.us $r46 = $r46r47.lo[$r44r45r46r47.z]
	;;
	lbz.wnez $r47? $r46r47.hi = -1125899906842624[$r44r45r46r47.t]
	;;
	lbz.s.weqz $r48? $r48r49.lo = -8388608[$r48r49r50r51.x]
	;;
	lbz.u.wltz $r49? $r48r49.hi = [$r48r49r50r51.y]
	;;
	lbz.us $r50 = 2305843009213693951[$r50r51.lo]
	;;
	lbz $r48r49r50r51.z = -64[$r51]
	;;
	lbz.s $r50r51.hi = -8589934592[$r48r49r50r51.t]
	;;
	ld.u.xs $r52 = $r52r53.lo[$r52r53r54r55.x]
	;;
	ld.us.wgez $r53? $r52r53.hi = -1125899906842624[$r52r53r54r55.y]
	;;
	ld.wlez $r54? $r54r55.lo = -8388608[$r52r53r54r55.z]
	;;
	ld.s.wgtz $r55? $r54r55.hi = [$r52r53r54r55.t]
	;;
	ld.u $r56 = 2305843009213693951[$r56r57.lo]
	;;
	ld.us $r56r57r58r59.x = -64[$r57]
	;;
	ld $r56r57.hi = -8589934592[$r56r57r58r59.y]
	;;
	lhs.s $r58 = $r58r59.lo[$r56r57r58r59.z]
	;;
	lhs.u.dnez $r59? $r58r59.hi = -1125899906842624[$r56r57r58r59.t]
	;;
	lhs.us.deqz $r60? $r60r61.lo = -8388608[$r60r61r62r63.x]
	;;
	lhs.dltz $r61? $r60r61.hi = [$r60r61r62r63.y]
	;;
	lhs.s $r62 = 2305843009213693951[$r62r63.lo]
	;;
	lhs.u $r60r61r62r63.z = -64[$r63]
	;;
	lhs.us $r62r63.hi = -8589934592[$r60r61r62r63.t]
	;;
	lhz.xs $r0 = $r0r1.lo[$r0r1r2r3.x]
	;;
	lhz.s.dgez $r1? $r0r1.hi = -1125899906842624[$r0r1r2r3.y]
	;;
	lhz.u.dlez $r2? $r2r3.lo = -8388608[$r0r1r2r3.z]
	;;
	lhz.us.dgtz $r3? $r2r3.hi = [$r0r1r2r3.t]
	;;
	lhz $r4 = 2305843009213693951[$r4r5.lo]
	;;
	lhz.s $r4r5r6r7.x = -64[$r5]
	;;
	lhz.u $r4r5.hi = -8589934592[$r4r5r6r7.y]
	;;
	lnandd $r6 = $r6r7.lo, $r4r5r6r7.z
	;;
	lnandw $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	lnandw $r8 = $r8r9.lo, 536870911
	;;
	lnord $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	lnorw $r8r9r10r11.y = $r10, $r10r11.lo
	;;
	lnorw $r8r9r10r11.z = $r11, 536870911
	;;
	loopdo $r10r11.hi, -32768
	;;
	lord $r8r9r10r11.t = $r12, $sp
	;;
	lorw $r13 = $tp, $r14
	;;
	lorw $fp = $r15, 536870911
	;;
	lo.us $r4r5r6r7 = $rp[$r16]
	;;
	lo.u0 $r16r17.lo? $r8r9r10r11 = -1125899906842624[$r16r17r18r19.x]
	;;
	lo.s.u1 $r17? $r12r13r14r15 = -8388608[$r16r17.hi]
	;;
	lo.u.u2 $r16r17r18r19.y? $r16r17r18r19 = [$r18]
	;;
	lo.us.odd $r18r19.lo? $r20r21r22r23 = -1125899906842624[$r16r17r18r19.z]
	;;
	lo.even $r19? $r24r25r26r27 = -8388608[$r18r19.hi]
	;;
	lo.s.wnez $r16r17r18r19.t? $r28r29r30r31 = [$r20]
	;;
	lo.u $r32r33r34r35 = 2305843009213693951[$r20r21.lo]
	;;
	lo.us $r36r37r38r39 = -64[$r20r21r22r23.x]
	;;
	lo $r40r41r42r43 = -8589934592[$r21]
	;;
	lq.s.xs $r18r19 = $r20r21.hi[$r20r21r22r23.y]
	;;
	lq.u.weqz $r22? $r16r17r18r19.hi = -1125899906842624[$r22r23.lo]
	;;
	lq.us.wltz $r20r21r22r23.z? $r20r21 = -8388608[$r23]
	;;
	lq.wgez $r22r23.hi? $r20r21r22r23.lo = [$r20r21r22r23.t]
	;;
	lq.s $r22r23 = 2305843009213693951[$r24]
	;;
	lq.u $r20r21r22r23.hi = -64[$r24r25.lo]
	;;
	lq.us $r24r25 = -8589934592[$r24r25r26r27.x]
	;;
	lws $r25 = $r24r25.hi[$r24r25r26r27.y]
	;;
	lws.s.wlez $r26? $r26r27.lo = -1125899906842624[$r24r25r26r27.z]
	;;
	lws.u.wgtz $r27? $r26r27.hi = -8388608[$r24r25r26r27.t]
	;;
	lws.us.dnez $r28? $r28r29.lo = [$r28r29r30r31.x]
	;;
	lws $r29 = 2305843009213693951[$r28r29.hi]
	;;
	lws.s $r28r29r30r31.y = -64[$r30]
	;;
	lws.u $r30r31.lo = -8589934592[$r28r29r30r31.z]
	;;
	lwz.us.xs $r31 = $r30r31.hi[$r28r29r30r31.t]
	;;
	lwz.deqz $r32? $r32r33.lo = -1125899906842624[$r32r33r34r35.x]
	;;
	lwz.s.dltz $r33? $r32r33.hi = -8388608[$r32r33r34r35.y]
	;;
	lwz.u.dgez $r34? $r34r35.lo = [$r32r33r34r35.z]
	;;
	lwz.us $r35 = 2305843009213693951[$r34r35.hi]
	;;
	lwz $r32r33r34r35.t = -64[$r36]
	;;
	lwz.s $r36r37.lo = -8589934592[$r36r37r38r39.x]
	;;
	madddt $r24r25r26r27.lo = $r37, $r36r37.hi
	;;
	maddd $r36r37r38r39.y = $r38, $r38r39.lo
	;;
	maddd $r36r37r38r39.z = $r39, 536870911
	;;
	maddhq $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	maddhq $r40r41.lo = $r40r41r42r43.x, 536870911
	;;
	maddhwq $r26r27 = $r41, $r40r41.hi
	;;
	maddmwq $r24r25r26r27.hi = $r28r29, $r28r29r30r31.lo
	;;
	maddsudt $r30r31 = $r40r41r42r43.y, $r42
	;;
	maddsuhwq $r28r29r30r31.hi = $r42r43.lo, $r40r41r42r43.z
	;;
	maddsumwq $r32r33 = $r32r33r34r35.lo, $r34r35
	;;
	maddsuwdp $r32r33r34r35.hi = $r43, $r42r43.hi
	;;
	maddsuwd $r40r41r42r43.t = $r44, $r44r45.lo
	;;
	maddsuwd $r44r45r46r47.x = $r45, 536870911
	;;
	maddudt $r36r37 = $r44r45.hi, $r44r45r46r47.y
	;;
	madduhwq $r36r37r38r39.lo = $r46, $r46r47.lo
	;;
	maddumwq $r38r39 = $r36r37r38r39.hi, $r40r41
	;;
	madduwdp $r40r41r42r43.lo = $r44r45r46r47.z, $r47
	;;
	madduwd $r46r47.hi = $r44r45r46r47.t, $r48
	;;
	madduwd $r48r49.lo = $r48r49r50r51.x, 536870911
	;;
	madduzdt $r42r43 = $r49, $r48r49.hi
	;;
	maddwdp $r40r41r42r43.hi = $r48r49r50r51.y, $r50
	;;
	maddwd $r50r51.lo = $r48r49r50r51.z, $r51
	;;
	maddwd $r50r51.hi = $r48r49r50r51.t, 536870911
	;;
	maddwp $r52 = $r52r53.lo, $r52r53r54r55.x
	;;
	maddwp $r53 = $r52r53.hi, 536870911
	;;
	maddwq $r44r45 = $r44r45r46r47.lo, $r46r47
	;;
	maddw $r52r53r54r55.y = $r54, $r54r55.lo
	;;
	maddw $r52r53r54r55.z = $r55, 536870911
	;;
	make $r54r55.hi = 2305843009213693951
	;;
	make $r52r53r54r55.t = -549755813888
	;;
	make $r56 = -4096
	;;
	maxbo $r56r57.lo = $r56r57r58r59.x, $r57
	;;
	maxbo.@ $r56r57.hi = $r56r57r58r59.y, 536870911
	;;
	maxd $r58 = $r58r59.lo, 2305843009213693951
	;;
	maxd $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	maxd $r56r57r58r59.t = $r60, -64
	;;
	maxd $r60r61.lo = $r60r61r62r63.x, -8589934592
	;;
	maxd.@ $r61 = $r60r61.hi, 536870911
	;;
	maxhq $r60r61r62r63.y = $r62, $r62r63.lo
	;;
	maxhq $r60r61r62r63.z = $r63, 536870911
	;;
	maxrbod $r62r63.hi = $r60r61r62r63.t
	;;
	maxrhqd $r0 = $r0r1.lo
	;;
	maxrwpd $r0r1r2r3.x = $r1
	;;
	maxubo $r0r1.hi = $r0r1r2r3.y, $r2
	;;
	maxubo.@ $r2r3.lo = $r0r1r2r3.z, 536870911
	;;
	maxud $r3 = $r2r3.hi, 2305843009213693951
	;;
	maxud $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	maxud $r4r5r6r7.x = $r5, -64
	;;
	maxud $r4r5.hi = $r4r5r6r7.y, -8589934592
	;;
	maxud.@ $r6 = $r6r7.lo, 536870911
	;;
	maxuhq $r4r5r6r7.z = $r7, $r6r7.hi
	;;
	maxuhq $r4r5r6r7.t = $r8, 536870911
	;;
	maxurbod $r8r9.lo = $r8r9r10r11.x
	;;
	maxurhqd $r9 = $r8r9.hi
	;;
	maxurwpd $r8r9r10r11.y = $r10
	;;
	maxuwp $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	maxuwp.@ $r10r11.hi = $r8r9r10r11.t, 536870911
	;;
	maxuw $r12 = $sp, $r13
	;;
	maxuw $tp = $r14, -64
	;;
	maxuw $fp = $r15, -8589934592
	;;
	maxwp $rp = $r16, $r16r17.lo
	;;
	maxwp $r16r17r18r19.x = $r17, 536870911
	;;
	maxw $r16r17.hi = $r16r17r18r19.y, $r18
	;;
	maxw $r18r19.lo = $r16r17r18r19.z, -64
	;;
	maxw $r19 = $r18r19.hi, -8589934592
	;;
	minbo $r16r17r18r19.t = $r20, $r20r21.lo
	;;
	minbo.@ $r20r21r22r23.x = $r21, 536870911
	;;
	mind $r20r21.hi = $r20r21r22r23.y, 2305843009213693951
	;;
	mind $r22 = $r22r23.lo, $r20r21r22r23.z
	;;
	mind $r23 = $r22r23.hi, -64
	;;
	mind $r20r21r22r23.t = $r24, -8589934592
	;;
	mind.@ $r24r25.lo = $r24r25r26r27.x, 536870911
	;;
	minhq $r25 = $r24r25.hi, $r24r25r26r27.y
	;;
	minhq $r26 = $r26r27.lo, 536870911
	;;
	minrbod $r24r25r26r27.z = $r27
	;;
	minrhqd $r26r27.hi = $r24r25r26r27.t
	;;
	minrwpd $r28 = $r28r29.lo
	;;
	minubo $r28r29r30r31.x = $r29, $r28r29.hi
	;;
	minubo.@ $r28r29r30r31.y = $r30, 536870911
	;;
	minud $r30r31.lo = $r28r29r30r31.z, 2305843009213693951
	;;
	minud $r31 = $r30r31.hi, $r28r29r30r31.t
	;;
	minud $r32 = $r32r33.lo, -64
	;;
	minud $r32r33r34r35.x = $r33, -8589934592
	;;
	minud.@ $r32r33.hi = $r32r33r34r35.y, 536870911
	;;
	minuhq $r34 = $r34r35.lo, $r32r33r34r35.z
	;;
	minuhq $r35 = $r34r35.hi, 536870911
	;;
	minurbod $r32r33r34r35.t = $r36
	;;
	minurhqd $r36r37.lo = $r36r37r38r39.x
	;;
	minurwpd $r37 = $r36r37.hi
	;;
	minuwp $r36r37r38r39.y = $r38, $r38r39.lo
	;;
	minuwp.@ $r36r37r38r39.z = $r39, 536870911
	;;
	minuw $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	minuw $r40r41.lo = $r40r41r42r43.x, -64
	;;
	minuw $r41 = $r40r41.hi, -8589934592
	;;
	minwp $r40r41r42r43.y = $r42, $r42r43.lo
	;;
	minwp $r40r41r42r43.z = $r43, 536870911
	;;
	minw $r42r43.hi = $r40r41r42r43.t, $r44
	;;
	minw $r44r45.lo = $r44r45r46r47.x, -64
	;;
	minw $r45 = $r44r45.hi, -8589934592
	;;
	mm212w $r44r45r46r47.hi = $r44r45r46r47.y, $r46
	;;
	mma212w $r48r49 = $r46r47.lo, $r44r45r46r47.z
	;;
	mms212w $r48r49r50r51.lo = $r47, $r46r47.hi
	;;
	msbfdt $r50r51 = $r44r45r46r47.t, $r48
	;;
	msbfd $r48r49.lo = $r48r49r50r51.x, $r49
	;;
	msbfhq $r48r49.hi = $r48r49r50r51.y, $r50
	;;
	msbfhwq $r48r49r50r51.hi = $r50r51.lo, $r48r49r50r51.z
	;;
	msbfmwq $r52r53 = $r52r53r54r55.lo, $r54r55
	;;
	msbfsudt $r52r53r54r55.hi = $r51, $r50r51.hi
	;;
	msbfsuhwq $r56r57 = $r48r49r50r51.t, $r52
	;;
	msbfsumwq $r56r57r58r59.lo = $r58r59, $r56r57r58r59.hi
	;;
	msbfsuwdp $r60r61 = $r52r53.lo, $r52r53r54r55.x
	;;
	msbfsuwd $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	msbfsuwd $r54 = $r54r55.lo, 536870911
	;;
	msbfudt $r60r61r62r63.lo = $r52r53r54r55.z, $r55
	;;
	msbfuhwq $r62r63 = $r54r55.hi, $r52r53r54r55.t
	;;
	msbfumwq $r60r61r62r63.hi = $r0r1, $r0r1r2r3.lo
	;;
	msbfuwdp $r2r3 = $r56, $r56r57.lo
	;;
	msbfuwd $r56r57r58r59.x = $r57, $r56r57.hi
	;;
	msbfuwd $r56r57r58r59.y = $r58, 536870911
	;;
	msbfuzdt $r0r1r2r3.hi = $r58r59.lo, $r56r57r58r59.z
	;;
	msbfwdp $r4r5 = $r59, $r58r59.hi
	;;
	msbfwd $r56r57r58r59.t = $r60, $r60r61.lo
	;;
	msbfwd $r60r61r62r63.x = $r61, 536870911
	;;
	msbfwp $r60r61.hi = $r60r61r62r63.y, $r62
	;;
	msbfwq $r4r5r6r7.lo = $r6r7, $r4r5r6r7.hi
	;;
	msbfw $r62r63.lo = $r60r61r62r63.z, $r63
	;;
	msbfw $r62r63.hi = $r60r61r62r63.t, 536870911
	;;
	muldt $r8r9 = $r0, $r0r1.lo
	;;
	muld $r0r1r2r3.x = $r1, $r0r1.hi
	;;
	muld $r0r1r2r3.y = $r2, 536870911
	;;
	mulhq $r2r3.lo = $r0r1r2r3.z, $r3
	;;
	mulhq $r2r3.hi = $r0r1r2r3.t, 536870911
	;;
	mulhwq $r8r9r10r11.lo = $r4, $r4r5.lo
	;;
	mulmwq $r10r11 = $r8r9r10r11.hi, $r12r13
	;;
	mulsudt $r12r13r14r15.lo = $r4r5r6r7.x, $r5
	;;
	mulsuhwq $r14r15 = $r4r5.hi, $r4r5r6r7.y
	;;
	mulsumwq $r12r13r14r15.hi = $r16r17, $r16r17r18r19.lo
	;;
	mulsuwdp $r18r19 = $r6, $r6r7.lo
	;;
	mulsuwd $r4r5r6r7.z = $r7, $r6r7.hi
	;;
	mulsuwd $r4r5r6r7.t = $r8, 536870911
	;;
	muludt $r16r17r18r19.hi = $r8r9.lo, $r8r9r10r11.x
	;;
	muluhwq $r20r21 = $r9, $r8r9.hi
	;;
	mulumwq $r20r21r22r23.lo = $r22r23, $r20r21r22r23.hi
	;;
	muluwdp $r24r25 = $r8r9r10r11.y, $r10
	;;
	muluwd $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	muluwd $r10r11.hi = $r8r9r10r11.t, 536870911
	;;
	mulwdp $r24r25r26r27.lo = $r12, $sp
	;;
	mulwd $r13 = $tp, $r14
	;;
	mulwd $fp = $r15, 536870911
	;;
	mulwp $rp = $r16, $r16r17.lo
	;;
	mulwp $r16r17r18r19.x = $r17, 536870911
	;;
	mulwq $r26r27 = $r24r25r26r27.hi, $r28r29
	;;
	mulw $r16r17.hi = $r16r17r18r19.y, $r18
	;;
	mulw $r18r19.lo = $r16r17r18r19.z, 536870911
	;;
	nandd $r19 = $r18r19.hi, 2305843009213693951
	;;
	nandd $r16r17r18r19.t = $r20, $r20r21.lo
	;;
	nandd $r20r21r22r23.x = $r21, -64
	;;
	nandd $r20r21.hi = $r20r21r22r23.y, -8589934592
	;;
	nandd.@ $r22 = $r22r23.lo, 536870911
	;;
	nandw $r20r21r22r23.z = $r23, $r22r23.hi
	;;
	nandw $r20r21r22r23.t = $r24, -64
	;;
	nandw $r24r25.lo = $r24r25r26r27.x, -8589934592
	;;
	negbo $r25 = $r24r25.hi
	;;
	negd $r24r25r26r27.y = $r26
	;;
	neghq $r26r27.lo = $r24r25r26r27.z
	;;
	negsbo $r27 = $r26r27.hi
	;;
	negsd $r24r25r26r27.t = $r28
	;;
	negshq $r28r29.lo = $r28r29r30r31.x
	;;
	negswp $r29 = $r28r29.hi
	;;
	negsw $r28r29r30r31.y = $r30
	;;
	negwp $r30r31.lo = $r28r29r30r31.z
	;;
	negw $r31 = $r30r31.hi
	;;
	nop
	;;
	nord $r28r29r30r31.t = $r32, 2305843009213693951
	;;
	nord $r32r33.lo = $r32r33r34r35.x, $r33
	;;
	nord $r32r33.hi = $r32r33r34r35.y, -64
	;;
	nord $r34 = $r34r35.lo, -8589934592
	;;
	nord.@ $r32r33r34r35.z = $r35, 536870911
	;;
	norw $r34r35.hi = $r32r33r34r35.t, $r36
	;;
	norw $r36r37.lo = $r36r37r38r39.x, -64
	;;
	norw $r37 = $r36r37.hi, -8589934592
	;;
	notd $r36r37r38r39.y = $r38
	;;
	notw $r38r39.lo = $r36r37r38r39.z
	;;
	nxord $r39 = $r38r39.hi, 2305843009213693951
	;;
	nxord $r36r37r38r39.t = $r40, $r40r41.lo
	;;
	nxord $r40r41r42r43.x = $r41, -64
	;;
	nxord $r40r41.hi = $r40r41r42r43.y, -8589934592
	;;
	nxord.@ $r42 = $r42r43.lo, 536870911
	;;
	nxorw $r40r41r42r43.z = $r43, $r42r43.hi
	;;
	nxorw $r40r41r42r43.t = $r44, -64
	;;
	nxorw $r44r45.lo = $r44r45r46r47.x, -8589934592
	;;
	ord $r45 = $r44r45.hi, 2305843009213693951
	;;
	ord $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	ord $r44r45r46r47.z = $r47, -64
	;;
	ord $r46r47.hi = $r44r45r46r47.t, -8589934592
	;;
	ord.@ $r48 = $r48r49.lo, 536870911
	;;
	ornd $r48r49r50r51.x = $r49, 2305843009213693951
	;;
	ornd $r48r49.hi = $r48r49r50r51.y, $r50
	;;
	ornd $r50r51.lo = $r48r49r50r51.z, -64
	;;
	ornd $r51 = $r50r51.hi, -8589934592
	;;
	ornd.@ $r48r49r50r51.t = $r52, 536870911
	;;
	ornw $r52r53.lo = $r52r53r54r55.x, $r53
	;;
	ornw $r52r53.hi = $r52r53r54r55.y, -64
	;;
	ornw $r54 = $r54r55.lo, -8589934592
	;;
	orrbod $r52r53r54r55.z = $r55
	;;
	orrhqd $r54r55.hi = $r52r53r54r55.t
	;;
	orrwpd $r56 = $r56r57.lo
	;;
	orw $r56r57r58r59.x = $r57, $r56r57.hi
	;;
	orw $r56r57r58r59.y = $r58, -64
	;;
	orw $r58r59.lo = $r56r57r58r59.z, -8589934592
	;;
	pcrel $r59 = 2305843009213693951
	;;
	pcrel $r58r59.hi = -549755813888
	;;
	pcrel $r56r57r58r59.t = -4096
	;;
	ret
	;;
	rfe
	;;
	rolwps $r60 = $r60r61.lo, $r60r61r62r63.x
	;;
	rolwps $r61 = $r60r61.hi, 7
	;;
	rolw $r60r61r62r63.y = $r62, $r62r63.lo
	;;
	rolw $r60r61r62r63.z = $r63, 7
	;;
	rorwps $r62r63.hi = $r60r61r62r63.t, $r0
	;;
	rorwps $r0r1.lo = $r0r1r2r3.x, 7
	;;
	rorw $r1 = $r0r1.hi, $r0r1r2r3.y
	;;
	rorw $r2 = $r2r3.lo, 7
	;;
	rswap $r0r1r2r3.z = $mmc
	;;
	rswap $r3 = $s0
	;;
	rswap $r2r3.hi = $pc
	;;
	sbfbo $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	sbfbo.@ $r4r5r6r7.x = $r5, 536870911
	;;
	sbfcd.i $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	sbfcd.i $r6r7.lo = $r4r5r6r7.z, 536870911
	;;
	sbfcd $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	sbfcd $r8 = $r8r9.lo, 536870911
	;;
	sbfd $r8r9r10r11.x = $r9, 2305843009213693951
	;;
	sbfd $r8r9.hi = $r8r9r10r11.y, $r10
	;;
	sbfd $r10r11.lo = $r8r9r10r11.z, -64
	;;
	sbfd $r11 = $r10r11.hi, -8589934592
	;;
	sbfd.@ $r8r9r10r11.t = $r12, 536870911
	;;
	sbfhq $sp = $r13, $tp
	;;
	sbfhq $r14 = $fp, 536870911
	;;
	sbfsbo $r15 = $rp, $r16
	;;
	sbfsbo.@ $r16r17.lo = $r16r17r18r19.x, 536870911
	;;
	sbfsd $r17 = $r16r17.hi, $r16r17r18r19.y
	;;
	sbfsd $r18 = $r18r19.lo, 536870911
	;;
	sbfshq $r16r17r18r19.z = $r19, $r18r19.hi
	;;
	sbfshq.@ $r16r17r18r19.t = $r20, 536870911
	;;
	sbfswp $r20r21.lo = $r20r21r22r23.x, $r21
	;;
	sbfswp $r20r21.hi = $r20r21r22r23.y, 536870911
	;;
	sbfsw $r22 = $r22r23.lo, $r20r21r22r23.z
	;;
	sbfsw $r23 = $r22r23.hi, 536870911
	;;
	sbfusbo $r20r21r22r23.t = $r24, $r24r25.lo
	;;
	sbfusbo.@ $r24r25r26r27.x = $r25, 536870911
	;;
	sbfusd $r24r25.hi = $r24r25r26r27.y, $r26
	;;
	sbfusd $r26r27.lo = $r24r25r26r27.z, 536870911
	;;
	sbfushq $r27 = $r26r27.hi, $r24r25r26r27.t
	;;
	sbfushq.@ $r28 = $r28r29.lo, 536870911
	;;
	sbfuswp $r28r29r30r31.x = $r29, $r28r29.hi
	;;
	sbfuswp $r28r29r30r31.y = $r30, 536870911
	;;
	sbfusw $r30r31.lo = $r28r29r30r31.z, $r31
	;;
	sbfusw $r30r31.hi = $r28r29r30r31.t, 536870911
	;;
	sbfuwd $r32 = $r32r33.lo, $r32r33r34r35.x
	;;
	sbfuwd $r33 = $r32r33.hi, 536870911
	;;
	sbfwd $r32r33r34r35.y = $r34, $r34r35.lo
	;;
	sbfwd $r32r33r34r35.z = $r35, 536870911
	;;
	sbfwp $r34r35.hi = $r32r33r34r35.t, $r36
	;;
	sbfwp.@ $r36r37.lo = $r36r37r38r39.x, 536870911
	;;
	sbfw $r37 = $r36r37.hi, $r36r37r38r39.y
	;;
	sbfw $r38 = $r38r39.lo, -64
	;;
	sbfw $r36r37r38r39.z = $r39, -8589934592
	;;
	sbfx16bo $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	sbfx16bo $r40r41.lo = $r40r41r42r43.x, 536870911
	;;
	sbfx16d $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	sbfx16d.@ $r42 = $r42r43.lo, 536870911
	;;
	sbfx16hq $r40r41r42r43.z = $r43, $r42r43.hi
	;;
	sbfx16hq $r40r41r42r43.t = $r44, 536870911
	;;
	sbfx16uwd $r44r45.lo = $r44r45r46r47.x, $r45
	;;
	sbfx16uwd $r44r45.hi = $r44r45r46r47.y, 536870911
	;;
	sbfx16wd $r46 = $r46r47.lo, $r44r45r46r47.z
	;;
	sbfx16wd $r47 = $r46r47.hi, 536870911
	;;
	sbfx16wp $r44r45r46r47.t = $r48, $r48r49.lo
	;;
	sbfx16wp.@ $r48r49r50r51.x = $r49, 536870911
	;;
	sbfx16w $r48r49.hi = $r48r49r50r51.y, $r50
	;;
	sbfx16w $r50r51.lo = $r48r49r50r51.z, 536870911
	;;
	sbfx2bo $r51 = $r50r51.hi, $r48r49r50r51.t
	;;
	sbfx2bo $r52 = $r52r53.lo, 536870911
	;;
	sbfx2d $r52r53r54r55.x = $r53, $r52r53.hi
	;;
	sbfx2d.@ $r52r53r54r55.y = $r54, 536870911
	;;
	sbfx2hq $r54r55.lo = $r52r53r54r55.z, $r55
	;;
	sbfx2hq $r54r55.hi = $r52r53r54r55.t, 536870911
	;;
	sbfx2uwd $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	sbfx2uwd $r57 = $r56r57.hi, 536870911
	;;
	sbfx2wd $r56r57r58r59.y = $r58, $r58r59.lo
	;;
	sbfx2wd $r56r57r58r59.z = $r59, 536870911
	;;
	sbfx2wp $r58r59.hi = $r56r57r58r59.t, $r60
	;;
	sbfx2wp.@ $r60r61.lo = $r60r61r62r63.x, 536870911
	;;
	sbfx2w $r61 = $r60r61.hi, $r60r61r62r63.y
	;;
	sbfx2w $r62 = $r62r63.lo, 536870911
	;;
	sbfx32d $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	sbfx32d $r60r61r62r63.t = $r0, 536870911
	;;
	sbfx32uwd $r0r1.lo = $r0r1r2r3.x, $r1
	;;
	sbfx32uwd $r0r1.hi = $r0r1r2r3.y, 536870911
	;;
	sbfx32wd $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	sbfx32wd $r3 = $r2r3.hi, 536870911
	;;
	sbfx32w $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	sbfx32w $r4r5r6r7.x = $r5, 536870911
	;;
	sbfx4bo $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	sbfx4bo.@ $r6r7.lo = $r4r5r6r7.z, 536870911
	;;
	sbfx4d $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	sbfx4d $r8 = $r8r9.lo, 536870911
	;;
	sbfx4hq $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	sbfx4hq.@ $r8r9r10r11.y = $r10, 536870911
	;;
	sbfx4uwd $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	sbfx4uwd $r10r11.hi = $r8r9r10r11.t, 536870911
	;;
	sbfx4wd $r12 = $sp, $r13
	;;
	sbfx4wd $tp = $r14, 536870911
	;;
	sbfx4wp $fp = $r15, $rp
	;;
	sbfx4wp $r16 = $r16r17.lo, 536870911
	;;
	sbfx4w $r16r17r18r19.x = $r17, $r16r17.hi
	;;
	sbfx4w $r16r17r18r19.y = $r18, 536870911
	;;
	sbfx64d $r18r19.lo = $r16r17r18r19.z, $r19
	;;
	sbfx64d.@ $r18r19.hi = $r16r17r18r19.t, 536870911
	;;
	sbfx64uwd $r20 = $r20r21.lo, $r20r21r22r23.x
	;;
	sbfx64uwd $r21 = $r20r21.hi, 536870911
	;;
	sbfx64wd $r20r21r22r23.y = $r22, $r22r23.lo
	;;
	sbfx64wd $r20r21r22r23.z = $r23, 536870911
	;;
	sbfx64w $r22r23.hi = $r20r21r22r23.t, $r24
	;;
	sbfx64w $r24r25.lo = $r24r25r26r27.x, 536870911
	;;
	sbfx8bo $r25 = $r24r25.hi, $r24r25r26r27.y
	;;
	sbfx8bo $r26 = $r26r27.lo, 536870911
	;;
	sbfx8d $r24r25r26r27.z = $r27, $r26r27.hi
	;;
	sbfx8d.@ $r24r25r26r27.t = $r28, 536870911
	;;
	sbfx8hq $r28r29.lo = $r28r29r30r31.x, $r29
	;;
	sbfx8hq $r28r29.hi = $r28r29r30r31.y, 536870911
	;;
	sbfx8uwd $r30 = $r30r31.lo, $r28r29r30r31.z
	;;
	sbfx8uwd $r31 = $r30r31.hi, 536870911
	;;
	sbfx8wd $r28r29r30r31.t = $r32, $r32r33.lo
	;;
	sbfx8wd $r32r33r34r35.x = $r33, 536870911
	;;
	sbfx8wp $r32r33.hi = $r32r33r34r35.y, $r34
	;;
	sbfx8wp.@ $r34r35.lo = $r32r33r34r35.z, 536870911
	;;
	sbfx8w $r35 = $r34r35.hi, $r32r33r34r35.t
	;;
	sbfx8w $r36 = $r36r37.lo, 536870911
	;;
	sbmm8 $r36r37r38r39.x = $r37, 2305843009213693951
	;;
	sbmm8 $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	sbmm8 $r38r39.lo = $r36r37r38r39.z, -64
	;;
	sbmm8 $r39 = $r38r39.hi, -8589934592
	;;
	sbmm8.@ $r36r37r38r39.t = $r40, 536870911
	;;
	sbmmt8 $r40r41.lo = $r40r41r42r43.x, 2305843009213693951
	;;
	sbmmt8 $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	sbmmt8 $r42 = $r42r43.lo, -64
	;;
	sbmmt8 $r40r41r42r43.z = $r43, -8589934592
	;;
	sbmmt8.@ $r42r43.hi = $r40r41r42r43.t, 536870911
	;;
	sb $r44[$r44r45.lo] = $r44r45r46r47.x
	;;
	sb 2305843009213693951[$r45] = $r44r45.hi
	;;
	sb.dlez $r44r45r46r47.y? -1125899906842624[$r46] = $r46r47.lo
	;;
	sb.dgtz $r44r45r46r47.z? -8388608[$r47] = $r46r47.hi
	;;
	sb.odd $r44r45r46r47.t? [$r48] = $r48r49.lo
	;;
	sb -64[$r48r49r50r51.x] = $r49
	;;
	sb -8589934592[$r48r49.hi] = $r48r49r50r51.y
	;;
	scall $r50
	;;
	scall 511
	;;
	sd.xs $r50r51.lo[$r48r49r50r51.z] = $r51
	;;
	sd 2305843009213693951[$r50r51.hi] = $r48r49r50r51.t
	;;
	sd.even $r52? -1125899906842624[$r52r53.lo] = $r52r53r54r55.x
	;;
	sd.wnez $r53? -8388608[$r52r53.hi] = $r52r53r54r55.y
	;;
	sd.weqz $r54? [$r54r55.lo] = $r52r53r54r55.z
	;;
	sd -64[$r55] = $r54r55.hi
	;;
	sd -8589934592[$r52r53r54r55.t] = $r56
	;;
	set $s28 = $r56r57.lo
	;;
	set $ra = $r56r57r58r59.x
	;;
	set $ps = $r57
	;;
	set $ps = $r56r57.hi
	;;
	sh $r56r57r58r59.y[$r58] = $r58r59.lo
	;;
	sh 2305843009213693951[$r56r57r58r59.z] = $r59
	;;
	sh.wltz $r58r59.hi? -1125899906842624[$r56r57r58r59.t] = $r60
	;;
	sh.wgez $r60r61.lo? -8388608[$r60r61r62r63.x] = $r61
	;;
	sh.wlez $r60r61.hi? [$r60r61r62r63.y] = $r62
	;;
	sh -64[$r62r63.lo] = $r60r61r62r63.z
	;;
	sh -8589934592[$r63] = $r62r63.hi
	;;
	sleep
	;;
	sllbos $r60r61r62r63.t = $r0, $r0r1.lo
	;;
	sllbos $r0r1r2r3.x = $r1, 7
	;;
	slld $r0r1.hi = $r0r1r2r3.y, $r2
	;;
	slld $r2r3.lo = $r0r1r2r3.z, 7
	;;
	sllhqs $r3 = $r2r3.hi, $r0r1r2r3.t
	;;
	sllhqs $r4 = $r4r5.lo, 7
	;;
	sllwps $r4r5r6r7.x = $r5, $r4r5.hi
	;;
	sllwps $r4r5r6r7.y = $r6, 7
	;;
	sllw $r6r7.lo = $r4r5r6r7.z, $r7
	;;
	sllw $r6r7.hi = $r4r5r6r7.t, 7
	;;
	slsbos $r8 = $r8r9.lo, $r8r9r10r11.x
	;;
	slsbos $r9 = $r8r9.hi, 7
	;;
	slsd $r8r9r10r11.y = $r10, $r10r11.lo
	;;
	slsd $r8r9r10r11.z = $r11, 7
	;;
	slshqs $r10r11.hi = $r8r9r10r11.t, $r12
	;;
	slshqs $sp = $r13, 7
	;;
	slswps $tp = $r14, $fp
	;;
	slswps $r15 = $rp, 7
	;;
	slsw $r16 = $r16r17.lo, $r16r17r18r19.x
	;;
	slsw $r17 = $r16r17.hi, 7
	;;
	slusbos $r16r17r18r19.y = $r18, $r18r19.lo
	;;
	slusbos $r16r17r18r19.z = $r19, 7
	;;
	slusd $r18r19.hi = $r16r17r18r19.t, $r20
	;;
	slusd $r20r21.lo = $r20r21r22r23.x, 7
	;;
	slushqs $r21 = $r20r21.hi, $r20r21r22r23.y
	;;
	slushqs $r22 = $r22r23.lo, 7
	;;
	sluswps $r20r21r22r23.z = $r23, $r22r23.hi
	;;
	sluswps $r20r21r22r23.t = $r24, 7
	;;
	slusw $r24r25.lo = $r24r25r26r27.x, $r25
	;;
	slusw $r24r25.hi = $r24r25r26r27.y, 7
	;;
	so.xs $r26[$r26r27.lo] = $r44r45r46r47
	;;
	so 2305843009213693951[$r24r25r26r27.z] = $r48r49r50r51
	;;
	so.u3 $r27? -1125899906842624[$r26r27.hi] = $r52r53r54r55
	;;
	so.mt $r24r25r26r27.t? -8388608[$r28] = $r56r57r58r59
	;;
	so.mf $r28r29.lo? [$r28r29r30r31.x] = $r60r61r62r63
	;;
	so.wgtz $r29? -1125899906842624[$r28r29.hi] = $r0r1r2r3
	;;
	so.dnez $r28r29r30r31.y? -8388608[$r30] = $r4r5r6r7
	;;
	so.deqz $r30r31.lo? [$r28r29r30r31.z] = $r8r9r10r11
	;;
	so -64[$r31] = $r12r13r14r15
	;;
	so -8589934592[$r30r31.hi] = $r16r17r18r19
	;;
	sq $r28r29r30r31.t[$r32] = $r28r29r30r31.lo
	;;
	sq 2305843009213693951[$r32r33.lo] = $r30r31
	;;
	sq.dltz $r32r33r34r35.x? -1125899906842624[$r33] = $r28r29r30r31.hi
	;;
	sq.dgez $r32r33.hi? -8388608[$r32r33r34r35.y] = $r32r33
	;;
	sq.dlez $r34? [$r34r35.lo] = $r32r33r34r35.lo
	;;
	sq -64[$r32r33r34r35.z] = $r34r35
	;;
	sq -8589934592[$r35] = $r32r33r34r35.hi
	;;
	srabos $r34r35.hi = $r32r33r34r35.t, $r36
	;;
	srabos $r36r37.lo = $r36r37r38r39.x, 7
	;;
	srad $r37 = $r36r37.hi, $r36r37r38r39.y
	;;
	srad $r38 = $r38r39.lo, 7
	;;
	srahqs $r36r37r38r39.z = $r39, $r38r39.hi
	;;
	srahqs $r36r37r38r39.t = $r40, 7
	;;
	srawps $r40r41.lo = $r40r41r42r43.x, $r41
	;;
	srawps $r40r41.hi = $r40r41r42r43.y, 7
	;;
	sraw $r42 = $r42r43.lo, $r40r41r42r43.z
	;;
	sraw $r43 = $r42r43.hi, 7
	;;
	srlbos $r40r41r42r43.t = $r44, $r44r45.lo
	;;
	srlbos $r44r45r46r47.x = $r45, 7
	;;
	srld $r44r45.hi = $r44r45r46r47.y, $r46
	;;
	srld $r46r47.lo = $r44r45r46r47.z, 7
	;;
	srlhqs $r47 = $r46r47.hi, $r44r45r46r47.t
	;;
	srlhqs $r48 = $r48r49.lo, 7
	;;
	srlwps $r48r49r50r51.x = $r49, $r48r49.hi
	;;
	srlwps $r48r49r50r51.y = $r50, 7
	;;
	srlw $r50r51.lo = $r48r49r50r51.z, $r51
	;;
	srlw $r50r51.hi = $r48r49r50r51.t, 7
	;;
	srsbos $r52 = $r52r53.lo, $r52r53r54r55.x
	;;
	srsbos $r53 = $r52r53.hi, 7
	;;
	srsd $r52r53r54r55.y = $r54, $r54r55.lo
	;;
	srsd $r52r53r54r55.z = $r55, 7
	;;
	srshqs $r54r55.hi = $r52r53r54r55.t, $r56
	;;
	srshqs $r56r57.lo = $r56r57r58r59.x, 7
	;;
	srswps $r57 = $r56r57.hi, $r56r57r58r59.y
	;;
	srswps $r58 = $r58r59.lo, 7
	;;
	srsw $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	srsw $r56r57r58r59.t = $r60, 7
	;;
	stop
	;;
	stsud $r60r61.lo = $r60r61r62r63.x, $r61
	;;
	stsud $r60r61.hi = $r60r61r62r63.y, 536870911
	;;
	stsuhq $r62 = $r62r63.lo, $r60r61r62r63.z
	;;
	stsuhq.@ $r63 = $r62r63.hi, 536870911
	;;
	stsuwp $r60r61r62r63.t = $r0, $r0r1.lo
	;;
	stsuwp $r0r1r2r3.x = $r1, 536870911
	;;
	stsuw $r0r1.hi = $r0r1r2r3.y, $r2
	;;
	stsuw $r2r3.lo = $r0r1r2r3.z, 536870911
	;;
	sw.xs $r3[$r2r3.hi] = $r0r1r2r3.t
	;;
	sw 2305843009213693951[$r4] = $r4r5.lo
	;;
	sw.dgtz $r4r5r6r7.x? -1125899906842624[$r5] = $r4r5.hi
	;;
	sw.odd $r4r5r6r7.y? -8388608[$r6] = $r6r7.lo
	;;
	sw.even $r4r5r6r7.z? [$r7] = $r6r7.hi
	;;
	sw -64[$r4r5r6r7.t] = $r8
	;;
	sw -8589934592[$r8r9.lo] = $r8r9r10r11.x
	;;
	sxbd $r9 = $r8r9.hi
	;;
	sxhd $r8r9r10r11.y = $r10
	;;
	sxlbhq $r10r11.lo = $r8r9r10r11.z
	;;
	sxlhwp $r11 = $r10r11.hi
	;;
	sxmbhq $r8r9r10r11.t = $r12
	;;
	sxmhwp $sp = $r13
	;;
	sxwd $tp = $r14
	;;
	syncgroup $fp
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
	waitit $r15
	;;
	wfxl $ps, $rp
	;;
	wfxl $pcr, $r16
	;;
	wfxl $s1, $r16r17.lo
	;;
	wfxm $s1, $r16r17r18r19.x
	;;
	wfxm $s2, $r17
	;;
	wfxm $pcr, $r16r17.hi
	;;
	xaccesso $r20r21r22r23 = $a0..a1, $r16r17r18r19.y
	;;
	xaccesso $r24r25r26r27 = $a0..a3, $r18
	;;
	xaccesso $r28r29r30r31 = $a0..a7, $r18r19.lo
	;;
	xaccesso $r32r33r34r35 = $a0..a15, $r16r17r18r19.z
	;;
	xaccesso $r36r37r38r39 = $a0..a31, $r19
	;;
	xaccesso $r40r41r42r43 = $a0..a63, $r18r19.hi
	;;
	xaligno $a0 = $a2..a3, $r16r17r18r19.t
	;;
	xaligno $a0a1.lo = $a4..a7, $r20
	;;
	xaligno $a0a1a2a3.x = $a8..a15, $r20r21.lo
	;;
	xaligno $a1 = $a16..a31, $r20r21r22r23.x
	;;
	xaligno $a0a1.hi = $a32..a63, $r21
	;;
	xaligno $a0a1a2a3.y = $a0..a63, $r20r21.hi
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
	xfscalewo $a8a9a10a11.x = $a9, $r20r21r22r23.y
	;;
	xlo.u.q0 $a8a9a10a11 = $r22[$r22r23.lo]
	;;
	xlo.us.xs $a8a9.hi = $r20r21r22r23.z[$r23]
	;;
	xlo.dnez.q1 $r22r23.hi? $a12a13a14a15 = -1125899906842624[$r20r21r22r23.t]
	;;
	xlo.s.deqz.q2 $r24? $a16a17a18a19 = -8388608[$r24r25.lo]
	;;
	xlo.u.wnez.q3 $r24r25r26r27.x? $a20a21a22a23 = [$r25]
	;;
	xlo.us.weqz $r24r25.hi? $a8a9a10a11.y = -1125899906842624[$r24r25r26r27.y]
	;;
	xlo.mt $r26? $a10 = -8388608[$r26r27.lo]
	;;
	xlo.s.mf $r24r25r26r27.z? $a10a11.lo = [$r27]
	;;
	xlo.u $a4..a5, $r26r27.hi = -1125899906842624[$r24r25r26r27.t]
	;;
	xlo.us.q $a6..a7, $r28 = -8388608[$r28r29.lo]
	;;
	xlo.d $a8..a9, $r28r29r30r31.x = [$r29]
	;;
	xlo.s.w $a8..a11, $r28r29.hi = -1125899906842624[$r28r29r30r31.y]
	;;
	xlo.u.h $a12..a15, $r30 = -8388608[$r30r31.lo]
	;;
	xlo.us.b $a16..a19, $r28r29r30r31.z = [$r31]
	;;
	xlo $a16..a23, $r30r31.hi = -1125899906842624[$r28r29r30r31.t]
	;;
	xlo.s.q $a24..a31, $r32 = -8388608[$r32r33.lo]
	;;
	xlo.u.d $a32..a39, $r32r33r34r35.x = [$r33]
	;;
	xlo.us.w $a32..a47, $r32r33.hi = -1125899906842624[$r32r33r34r35.y]
	;;
	xlo.h $a48..a63, $r34 = -8388608[$r34r35.lo]
	;;
	xlo.s.b $a0..a15, $r32r33r34r35.z = [$r35]
	;;
	xlo.u $a0..a31, $r34r35.hi = -1125899906842624[$r32r33r34r35.t]
	;;
	xlo.us.q $a32..a63, $r36 = -8388608[$r36r37.lo]
	;;
	xlo.d $a0..a31, $r36r37r38r39.x = [$r37]
	;;
	xlo.s.w $a0..a63, $r36r37.hi = -1125899906842624[$r36r37r38r39.y]
	;;
	xlo.u.h $a0..a63, $r38 = -8388608[$r38r39.lo]
	;;
	xlo.us.b $a0..a63, $r36r37r38r39.z = [$r39]
	;;
	xlo.q0 $a24a25a26a27 = 2305843009213693951[$r38r39.hi]
	;;
	xlo.s.q1 $a28a29a30a31 = -64[$r36r37r38r39.t]
	;;
	xlo.u.q2 $a32a33a34a35 = -8589934592[$r40]
	;;
	xlo.us $a8a9a10a11.z = 2305843009213693951[$r40r41.lo]
	;;
	xlo $a11 = -64[$r40r41r42r43.x]
	;;
	xlo.s $a10a11.hi = -8589934592[$r41]
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
	xmovefd $r40r41.hi = $a0_x
	;;
	xmovefo $r44r45r46r47 = $a18a19.hi
	;;
	xmovefq $r36r37 = $a0_lo
	;;
	xmovetd $a0_t = $r40r41r42r43.y
	;;
	xmovetd $a0_x = $r42
	;;
	xmovetd $a0_y = $r42r43.lo
	;;
	xmovetd $a0_z = $r40r41r42r43.z
	;;
	xmovetq $a0_lo = $r43, $r42r43.hi
	;;
	xmovetq $a0_hi = $r40r41r42r43.t, $r44
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
	xord $r44r45.lo = $r44r45r46r47.x, 2305843009213693951
	;;
	xord $r45 = $r44r45.hi, $r44r45r46r47.y
	;;
	xord $r46 = $r46r47.lo, -64
	;;
	xord $r44r45r46r47.z = $r47, -8589934592
	;;
	xord.@ $r46r47.hi = $r44r45r46r47.t, 536870911
	;;
	xorno $a20a21a22a23.t = $a24, $a24a25.lo
	;;
	xoro $a24a25a26a27.x = $a25, $a24a25.hi
	;;
	xorrbod $r48 = $r48r49.lo
	;;
	xorrhqd $r48r49r50r51.x = $r49
	;;
	xorrwpd $r48r49.hi = $r48r49r50r51.y
	;;
	xorw $r50 = $r50r51.lo, $r48r49r50r51.z
	;;
	xorw $r51 = $r50r51.hi, -64
	;;
	xorw $r48r49r50r51.t = $r52, -8589934592
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
	xso $r52r53.lo[$r52r53r54r55.x] = $a29
	;;
	xso 2305843009213693951[$r53] = $a28a29.hi
	;;
	xso.mtc $r52r53.hi? -1125899906842624[$r52r53r54r55.y] = $a28a29a30a31.y
	;;
	xso.mfc $r54? -8388608[$r54r55.lo] = $a30
	;;
	xso.dnez $r52r53r54r55.z? [$r55] = $a30a31.lo
	;;
	xso -64[$r54r55.hi] = $a28a29a30a31.z
	;;
	xso -8589934592[$r52r53r54r55.t] = $a31
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
	zxbd $r56 = $r56r57.lo
	;;
	zxhd $r56r57r58r59.x = $r57
	;;
	zxlbhq $r56r57.hi = $r56r57r58r59.y
	;;
	zxlhwp $r58 = $r58r59.lo
	;;
	zxmbhq $r56r57r58r59.z = $r59
	;;
	zxmhwp $r58r59.hi = $r56r57r58r59.t
	;;
	zxwd $r60 = $r60r61.lo
	;;
	.endp	main
	.section .text
