	##-----------------------------------------------------------
	## Generating test.s from MDS
	## (c) Copyright 2010-2018 Kalray SA.
	##-----------------------------------------------------------

#	Option: ''

##	target-core:	kv3-1

	.section .text

	.align 8
	.proc	main
	.global	main
main:
	abdd $r0 = $r0r1.lo, 2305843009213693951
	;;
	abdd $r0r1r2r3.x = $r1, $r0r1.hi
	;;
	abdd $r0r1r2r3.y = $r2, -64
	;;
	abdd $r2r3.lo = $r0r1r2r3.z, -8589934592
	;;
	abdd.@ $r3 = $r2r3.hi, 536870911
	;;
	abdhq $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	abdhq $r4r5r6r7.x = $r5, 536870911
	;;
	abdwp $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	abdwp.@ $r6r7.lo = $r4r5r6r7.z, 536870911
	;;
	abdw $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	abdw $r8 = $r8r9.lo, -64
	;;
	abdw $r8r9r10r11.x = $r9, -8589934592
	;;
	absd $r8r9.hi = $r8r9r10r11.y
	;;
	abshq $r10 = $r10r11.lo
	;;
	abswp $r8r9r10r11.z = $r11
	;;
	absw $r10r11.hi = $r8r9r10r11.t
	;;
	acswapd $r12[$sp] = $r0r1
	;;
	acswapd 2305843009213693951[$r13] = $r0r1r2r3.lo
	;;
	acswapd.dnez $tp? -1125899906842624[$r14] = $r2r3
	;;
	acswapd.deqz $fp? -8388608[$r15] = $r0r1r2r3.hi
	;;
	acswapd.dltz $rp? [$r16] = $r4r5
	;;
	acswapd -64[$r16r17.lo] = $r4r5r6r7.lo
	;;
	acswapd -8589934592[$r16r17r18r19.x] = $r6r7
	;;
	acswapw.xs $r17[$r16r17.hi] = $r4r5r6r7.hi
	;;
	acswapw 2305843009213693951[$r16r17r18r19.y] = $r8r9
	;;
	acswapw.dgez $r18? -1125899906842624[$r18r19.lo] = $r8r9r10r11.lo
	;;
	acswapw.dlez $r16r17r18r19.z? -8388608[$r19] = $r10r11
	;;
	acswapw.dgtz $r18r19.hi? [$r16r17r18r19.t] = $r8r9r10r11.hi
	;;
	acswapw -64[$r20] = $r12r13
	;;
	acswapw -8589934592[$r20r21.lo] = $r12r13r14r15.lo
	;;
	addcd.i $r20r21r22r23.x = $r21, $r20r21.hi
	;;
	addcd.i $r20r21r22r23.y = $r22, 536870911
	;;
	addcd $r22r23.lo = $r20r21r22r23.z, $r23
	;;
	addcd $r22r23.hi = $r20r21r22r23.t, 536870911
	;;
	addd $r24 = $r24r25.lo, 2305843009213693951
	;;
	addd $r24r25r26r27.x = $r25, $r24r25.hi
	;;
	addd $r24r25r26r27.y = $r26, -64
	;;
	addd $r26r27.lo = $r24r25r26r27.z, -8589934592
	;;
	addd.@ $r27 = $r26r27.hi, 536870911
	;;
	addhcp.c $r24r25r26r27.t = $r28, $r28r29.lo
	;;
	addhcp.c $r28r29r30r31.x = $r29, 536870911
	;;
	addhq $r28r29.hi = $r28r29r30r31.y, $r30
	;;
	addhq.@ $r30r31.lo = $r28r29r30r31.z, 536870911
	;;
	addsd $r31 = $r30r31.hi, 2305843009213693951
	;;
	addsd $r28r29r30r31.t = $r32, $r32r33.lo
	;;
	addsd $r32r33r34r35.x = $r33, -64
	;;
	addsd $r32r33.hi = $r32r33r34r35.y, -8589934592
	;;
	addshq $r34 = $r34r35.lo, $r32r33r34r35.z
	;;
	addshq $r35 = $r34r35.hi, 536870911
	;;
	addswp $r32r33r34r35.t = $r36, $r36r37.lo
	;;
	addswp.@ $r36r37r38r39.x = $r37, 536870911
	;;
	addsw $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	addsw $r38r39.lo = $r36r37r38r39.z, 536870911
	;;
	adduwd $r39 = $r38r39.hi, $r36r37r38r39.t
	;;
	adduwd $r40 = $r40r41.lo, 536870911
	;;
	addwc.c $r40r41r42r43.x = $r41, $r40r41.hi
	;;
	addwc.c.@ $r40r41r42r43.y = $r42, 536870911
	;;
	addwd $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	addwd $r42r43.hi = $r40r41r42r43.t, 536870911
	;;
	addwp $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	addwp.@ $r45 = $r44r45.hi, 536870911
	;;
	addw $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	addw $r44r45r46r47.z = $r47, -64
	;;
	addw $r46r47.hi = $r44r45r46r47.t, -8589934592
	;;
	addx16d $r48 = $r48r49.lo, $r48r49r50r51.x
	;;
	addx16d $r49 = $r48r49.hi, 536870911
	;;
	addx16hq $r48r49r50r51.y = $r50, $r50r51.lo
	;;
	addx16hq.@ $r48r49r50r51.z = $r51, 536870911
	;;
	addx16uwd $r50r51.hi = $r48r49r50r51.t, $r52
	;;
	addx16uwd $r52r53.lo = $r52r53r54r55.x, 536870911
	;;
	addx16wd $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	addx16wd $r54 = $r54r55.lo, 536870911
	;;
	addx16wp $r52r53r54r55.z = $r55, $r54r55.hi
	;;
	addx16wp $r52r53r54r55.t = $r56, 536870911
	;;
	addx16w $r56r57.lo = $r56r57r58r59.x, $r57
	;;
	addx16w $r56r57.hi = $r56r57r58r59.y, 536870911
	;;
	addx2d $r58 = $r58r59.lo, $r56r57r58r59.z
	;;
	addx2d.@ $r59 = $r58r59.hi, 536870911
	;;
	addx2hq $r56r57r58r59.t = $r60, $r60r61.lo
	;;
	addx2hq $r60r61r62r63.x = $r61, 536870911
	;;
	addx2uwd $r60r61.hi = $r60r61r62r63.y, $r62
	;;
	addx2uwd $r62r63.lo = $r60r61r62r63.z, 536870911
	;;
	addx2wd $r63 = $r62r63.hi, $r60r61r62r63.t
	;;
	addx2wd $r0 = $r0r1.lo, 536870911
	;;
	addx2wp $r0r1r2r3.x = $r1, $r0r1.hi
	;;
	addx2wp.@ $r0r1r2r3.y = $r2, 536870911
	;;
	addx2w $r2r3.lo = $r0r1r2r3.z, $r3
	;;
	addx2w $r2r3.hi = $r0r1r2r3.t, 536870911
	;;
	addx4d $r4 = $r4r5.lo, $r4r5r6r7.x
	;;
	addx4d $r5 = $r4r5.hi, 536870911
	;;
	addx4hq $r4r5r6r7.y = $r6, $r6r7.lo
	;;
	addx4hq.@ $r4r5r6r7.z = $r7, 536870911
	;;
	addx4uwd $r6r7.hi = $r4r5r6r7.t, $r8
	;;
	addx4uwd $r8r9.lo = $r8r9r10r11.x, 536870911
	;;
	addx4wd $r9 = $r8r9.hi, $r8r9r10r11.y
	;;
	addx4wd $r10 = $r10r11.lo, 536870911
	;;
	addx4wp $r8r9r10r11.z = $r11, $r10r11.hi
	;;
	addx4wp $r8r9r10r11.t = $r12, 536870911
	;;
	addx4w $sp = $r13, $tp
	;;
	addx4w $r14 = $fp, 536870911
	;;
	addx8d $r15 = $rp, $r16
	;;
	addx8d.@ $r16r17.lo = $r16r17r18r19.x, 536870911
	;;
	addx8hq $r17 = $r16r17.hi, $r16r17r18r19.y
	;;
	addx8hq $r18 = $r18r19.lo, 536870911
	;;
	addx8uwd $r16r17r18r19.z = $r19, $r18r19.hi
	;;
	addx8uwd $r16r17r18r19.t = $r20, 536870911
	;;
	addx8wd $r20r21.lo = $r20r21r22r23.x, $r21
	;;
	addx8wd $r20r21.hi = $r20r21r22r23.y, 536870911
	;;
	addx8wp $r22 = $r22r23.lo, $r20r21r22r23.z
	;;
	addx8wp.@ $r23 = $r22r23.hi, 536870911
	;;
	addx8w $r20r21r22r23.t = $r24, $r24r25.lo
	;;
	addx8w $r24r25r26r27.x = $r25, 536870911
	;;
	aladdd $r24r25.hi[$r24r25r26r27.y] = $r26
	;;
	aladdd 2305843009213693951[$r26r27.lo] = $r24r25r26r27.z
	;;
	aladdd.odd $r27? -1125899906842624[$r26r27.hi] = $r24r25r26r27.t
	;;
	aladdd.even $r28? -8388608[$r28r29.lo] = $r28r29r30r31.x
	;;
	aladdd.wnez $r29? [$r28r29.hi] = $r28r29r30r31.y
	;;
	aladdd -64[$r30] = $r30r31.lo
	;;
	aladdd -8589934592[$r28r29r30r31.z] = $r31
	;;
	aladdw.xs $r30r31.hi[$r28r29r30r31.t] = $r32
	;;
	aladdw 2305843009213693951[$r32r33.lo] = $r32r33r34r35.x
	;;
	aladdw.weqz $r33? -1125899906842624[$r32r33.hi] = $r32r33r34r35.y
	;;
	aladdw.wltz $r34? -8388608[$r34r35.lo] = $r32r33r34r35.z
	;;
	aladdw.wgez $r35? [$r34r35.hi] = $r32r33r34r35.t
	;;
	aladdw -64[$r36] = $r36r37.lo
	;;
	aladdw -8589934592[$r36r37r38r39.x] = $r37
	;;
	alclrd $r36r37.hi = $r36r37r38r39.y[$r38]
	;;
	alclrd.wlez $r38r39.lo? $r36r37r38r39.z = -1125899906842624[$r39]
	;;
	alclrd.wgtz $r38r39.hi? $r36r37r38r39.t = -8388608[$r40]
	;;
	alclrd.dnez $r40r41.lo? $r40r41r42r43.x = [$r41]
	;;
	alclrd $r40r41.hi = 2305843009213693951[$r40r41r42r43.y]
	;;
	alclrd $r42 = -64[$r42r43.lo]
	;;
	alclrd $r40r41r42r43.z = -8589934592[$r43]
	;;
	alclrw.xs $r42r43.hi = $r40r41r42r43.t[$r44]
	;;
	alclrw.deqz $r44r45.lo? $r44r45r46r47.x = -1125899906842624[$r45]
	;;
	alclrw.dltz $r44r45.hi? $r44r45r46r47.y = -8388608[$r46]
	;;
	alclrw.dgez $r46r47.lo? $r44r45r46r47.z = [$r47]
	;;
	alclrw $r46r47.hi = 2305843009213693951[$r44r45r46r47.t]
	;;
	alclrw $r48 = -64[$r48r49.lo]
	;;
	alclrw $r48r49r50r51.x = -8589934592[$r49]
	;;
	aligno $r0r1r2r3 = $a0, $a1, 7
	;;
	aligno $r4r5r6r7 = $a0a1.lo, $a0a1.hi, $r48r49.hi
	;;
	aligno $r8r9r10r11 = $a0a1a2a3.y, $a0a1a2a3.x, 7
	;;
	aligno $r12r13r14r15 = $a3, $a2, $r48r49r50r51.y
	;;
	alignv $a0 = $a2a3.lo, $a2a3.hi, 7
	;;
	alignv $a0a1.lo = $a0a1a2a3.z, $a0a1a2a3.t, $r50
	;;
	alignv $a0a1a2a3.x = $a5, $a4, 7
	;;
	alignv $a1 = $a4a5.hi, $a4a5.lo, $r50r51.lo
	;;
	andd $r48r49r50r51.z = $r51, 2305843009213693951
	;;
	andd $r50r51.hi = $r48r49r50r51.t, $r52
	;;
	andd $r52r53.lo = $r52r53r54r55.x, -64
	;;
	andd $r53 = $r52r53.hi, -8589934592
	;;
	andd.@ $r52r53r54r55.y = $r54, 536870911
	;;
	andnd $r54r55.lo = $r52r53r54r55.z, 2305843009213693951
	;;
	andnd $r55 = $r54r55.hi, $r52r53r54r55.t
	;;
	andnd $r56 = $r56r57.lo, -64
	;;
	andnd $r56r57r58r59.x = $r57, -8589934592
	;;
	andnd.@ $r56r57.hi = $r56r57r58r59.y, 536870911
	;;
	andnw $r58 = $r58r59.lo, $r56r57r58r59.z
	;;
	andnw $r59 = $r58r59.hi, -64
	;;
	andnw $r56r57r58r59.t = $r60, -8589934592
	;;
	andw $r60r61.lo = $r60r61r62r63.x, $r61
	;;
	andw $r60r61.hi = $r60r61r62r63.y, -64
	;;
	andw $r62 = $r62r63.lo, -8589934592
	;;
	avghq $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	avghq $r60r61r62r63.t = $r0, 536870911
	;;
	avgrhq $r0r1.lo = $r0r1r2r3.x, $r1
	;;
	avgrhq.@ $r0r1.hi = $r0r1r2r3.y, 536870911
	;;
	avgruhq $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	avgruhq $r3 = $r2r3.hi, 536870911
	;;
	avgruwp $r0r1r2r3.t = $r4, $r4r5.lo
	;;
	avgruwp.@ $r4r5r6r7.x = $r5, 536870911
	;;
	avgruw $r4r5.hi = $r4r5r6r7.y, $r6
	;;
	avgruw $r6r7.lo = $r4r5r6r7.z, 536870911
	;;
	avgrwp $r7 = $r6r7.hi, $r4r5r6r7.t
	;;
	avgrwp $r8 = $r8r9.lo, 536870911
	;;
	avgrw $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	avgrw $r8r9r10r11.y = $r10, 536870911
	;;
	avguhq $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	avguhq.@ $r10r11.hi = $r8r9r10r11.t, 536870911
	;;
	avguwp $r12 = $sp, $r13
	;;
	avguwp $tp = $r14, 536870911
	;;
	avguw $fp = $r15, $rp
	;;
	avguw $r16 = $r16r17.lo, 536870911
	;;
	avgwp $r16r17r18r19.x = $r17, $r16r17.hi
	;;
	avgwp.@ $r16r17r18r19.y = $r18, 536870911
	;;
	avgw $r18r19.lo = $r16r17r18r19.z, $r19
	;;
	avgw $r18r19.hi = $r16r17r18r19.t, 536870911
	;;
	await
	;;
	barrier
	;;
	call -33554432
	;;
	cbsd $r20 = $r20r21.lo
	;;
	cbswp $r20r21r22r23.x = $r21
	;;
	cbsw $r20r21.hi = $r20r21r22r23.y
	;;
	cb.dlez $r22? -32768
	;;
	clrf $r22r23.lo = $r20r21r22r23.z, 7, 7
	;;
	clsd $r23 = $r22r23.hi
	;;
	clswp $r20r21r22r23.t = $r24
	;;
	clsw $r24r25.lo = $r24r25r26r27.x
	;;
	clzd $r25 = $r24r25.hi
	;;
	clzwp $r24r25r26r27.y = $r26
	;;
	clzw $r26r27.lo = $r24r25r26r27.z
	;;
	cmoved.dgtz $r27? $r26r27.hi = 2305843009213693951
	;;
	cmoved.odd $r24r25r26r27.t? $r28 = $r28r29.lo
	;;
	cmoved.even $r28r29r30r31.x? $r29 = -64
	;;
	cmoved.wnez $r28r29.hi? $r28r29r30r31.y = -8589934592
	;;
	cmovehq.nez $r30? $r30r31.lo = $r28r29r30r31.z
	;;
	cmovewp.eqz $r31? $r30r31.hi = $r28r29r30r31.t
	;;
	cmuldt $r14r15 = $r32, 2305843009213693951
	;;
	cmuldt $r12r13r14r15.hi = $r32r33.lo, $r32r33r34r35.x
	;;
	cmuldt $r16r17 = $r33, -64
	;;
	cmuldt $r16r17r18r19.lo = $r32r33.hi, -8589934592
	;;
	cmulghxdt $r18r19 = $r32r33r34r35.y, $r34
	;;
	cmulglxdt $r16r17r18r19.hi = $r34r35.lo, $r32r33r34r35.z
	;;
	cmulgmxdt $r20r21 = $r35, $r34r35.hi
	;;
	cmulxdt $r20r21r22r23.lo = $r32r33r34r35.t, $r36
	;;
	compd.ne $r36r37.lo = $r36r37r38r39.x, 2305843009213693951
	;;
	compd.eq $r37 = $r36r37.hi, $r36r37r38r39.y
	;;
	compd.lt $r38 = $r38r39.lo, -64
	;;
	compd.ge $r36r37r38r39.z = $r39, -8589934592
	;;
	compnhq.le $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	compnhq.gt $r40r41.lo = $r40r41r42r43.x, 536870911
	;;
	compnwp.ltu $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	compnwp.geu.@ $r42 = $r42r43.lo, 536870911
	;;
	compuwd.leu $r40r41r42r43.z = $r43, $r42r43.hi
	;;
	compuwd.gtu $r40r41r42r43.t = $r44, 536870911
	;;
	compwd.all $r44r45.lo = $r44r45r46r47.x, $r45
	;;
	compwd.nall $r44r45.hi = $r44r45r46r47.y, 536870911
	;;
	compw.any $r46 = $r46r47.lo, $r44r45r46r47.z
	;;
	compw.none $r47 = $r46r47.hi, 536870911
	;;
	convdhv0.rn.sat $a0_lo = $a0a1a2a3
	;;
	convdhv1.ru.satu $a0_hi = $a4a5a6a7
	;;
	convwbv0.rd.sat $a0_x = $a8a9a10a11
	;;
	convwbv1.rz.satu $a0_y = $a12a13a14a15
	;;
	convwbv2.rhu.sat $a0_z = $a16a17a18a19
	;;
	convwbv3.rn.satu $a0_t = $a20a21a22a23
	;;
	copyd $r44r45r46r47.t = $r48
	;;
	copyo $r16r17r18r19 = $r20r21r22r23
	;;
	copyq $r22r23 = $r48r49.lo, $r48r49r50r51.x
	;;
	copyw $r49 = $r48r49.hi
	;;
	crcbellw $r48r49r50r51.y = $r50, $r50r51.lo
	;;
	crcbellw $r48r49r50r51.z = $r51, 536870911
	;;
	crcbelmw $r50r51.hi = $r48r49r50r51.t, $r52
	;;
	crcbelmw $r52r53.lo = $r52r53r54r55.x, 536870911
	;;
	crclellw $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	crclellw $r54 = $r54r55.lo, 536870911
	;;
	crclelmw $r52r53r54r55.z = $r55, $r54r55.hi
	;;
	crclelmw $r52r53r54r55.t = $r56, 536870911
	;;
	ctzd $r56r57.lo = $r56r57r58r59.x
	;;
	ctzwp $r57 = $r56r57.hi
	;;
	ctzw $r56r57r58r59.y = $r58
	;;
	d1inval
	;;
	dinvall 2305843009213693951[$r58r59.lo]
	;;
	dinvall.weqz $r56r57r58r59.z? -1125899906842624[$r59]
	;;
	dinvall.wltz $r58r59.hi? -8388608[$r56r57r58r59.t]
	;;
	dinvall.wgez $r60? [$r60r61.lo]
	;;
	dinvall $r60r61r62r63.x[$r61]
	;;
	dinvall -64[$r60r61.hi]
	;;
	dinvall -8589934592[$r60r61r62r63.y]
	;;
	dot2suwdp $r20r21r22r23.hi = $r24r25, $r24r25r26r27.lo
	;;
	dot2suwd $r62 = $r62r63.lo, 2305843009213693951
	;;
	dot2suwd $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	dot2suwd $r60r61r62r63.t = $r0, -64
	;;
	dot2suwd $r0r1.lo = $r0r1r2r3.x, -8589934592
	;;
	dot2uwdp $r26r27 = $r24r25r26r27.hi, $r28r29
	;;
	dot2uwd $r1 = $r0r1.hi, 2305843009213693951
	;;
	dot2uwd $r0r1r2r3.y = $r2, $r2r3.lo
	;;
	dot2uwd $r0r1r2r3.z = $r3, -64
	;;
	dot2uwd $r2r3.hi = $r0r1r2r3.t, -8589934592
	;;
	dot2wdp $r28r29r30r31.lo = $r30r31, $r28r29r30r31.hi
	;;
	dot2wd $r4 = $r4r5.lo, 2305843009213693951
	;;
	dot2wd $r4r5r6r7.x = $r5, $r4r5.hi
	;;
	dot2wd $r4r5r6r7.y = $r6, -64
	;;
	dot2wd $r6r7.lo = $r4r5r6r7.z, -8589934592
	;;
	dot2wzp $r32r33 = $r32r33r34r35.lo, $r34r35
	;;
	dot2w $r7 = $r6r7.hi, 2305843009213693951
	;;
	dot2w $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	dot2w $r8r9r10r11.x = $r9, -64
	;;
	dot2w $r8r9.hi = $r8r9r10r11.y, -8589934592
	;;
	dtouchl 2305843009213693951[$r10]
	;;
	dtouchl.wlez $r10r11.lo? -1125899906842624[$r8r9r10r11.z]
	;;
	dtouchl.wgtz $r11? -8388608[$r10r11.hi]
	;;
	dtouchl.dnez $r8r9r10r11.t? [$r12]
	;;
	dtouchl $sp[$r13]
	;;
	dtouchl -64[$tp]
	;;
	dtouchl -8589934592[$r14]
	;;
	dzerol 2305843009213693951[$fp]
	;;
	dzerol.deqz $r15? -1125899906842624[$rp]
	;;
	dzerol.dltz $r16? -8388608[$r16r17.lo]
	;;
	dzerol.dgez $r16r17r18r19.x? [$r17]
	;;
	dzerol $r16r17.hi[$r16r17r18r19.y]
	;;
	dzerol -64[$r18]
	;;
	dzerol -8589934592[$r18r19.lo]
	;;
	errop
	;;
	extfs $r16r17r18r19.z = $r19, 7, 7
	;;
	extfz $r18r19.hi = $r16r17r18r19.t, 7, 7
	;;
	fabsd $r20 = $r20r21.lo
	;;
	fabshq $r20r21r22r23.x = $r21
	;;
	fabswp $r20r21.hi = $r20r21r22r23.y
	;;
	fabsw $r22 = $r22r23.lo
	;;
	fadddc.c.rn $r32r33r34r35.hi = $r36r37, $r36r37r38r39.lo
	;;
	fadddc.ru.s $r38r39 = $r36r37r38r39.hi, $r40r41
	;;
	fadddp.rd $r40r41r42r43.lo = $r42r43, $r40r41r42r43.hi
	;;
	faddd $r20r21r22r23.z = $r23, 2305843009213693951
	;;
	faddd $r22r23.hi = $r20r21r22r23.t, -64
	;;
	faddd $r24 = $r24r25.lo, -8589934592
	;;
	faddd.rz.s $r24r25r26r27.x = $r25, $r24r25.hi
	;;
	faddhq $r24r25r26r27.y = $r26, 2305843009213693951
	;;
	faddhq $r26r27.lo = $r24r25r26r27.z, -64
	;;
	faddhq $r27 = $r26r27.hi, -8589934592
	;;
	faddhq.rna $r24r25r26r27.t = $r28, $r28r29.lo
	;;
	faddwc.c $r28r29r30r31.x = $r29, 2305843009213693951
	;;
	faddwc.c $r28r29.hi = $r28r29r30r31.y, -64
	;;
	faddwc.c $r30 = $r30r31.lo, -8589934592
	;;
	faddwc.c.rnz.s $r28r29r30r31.z = $r31, $r30r31.hi
	;;
	faddwcp.c.ro $r44r45 = $r44r45r46r47.lo, $r46r47
	;;
	faddwcp.s $r44r45r46r47.hi = $r48r49, $r48r49r50r51.lo
	;;
	faddwc.rn $r28r29r30r31.t = $r32, $r32r33.lo
	;;
	faddwp $r32r33r34r35.x = $r33, 2305843009213693951
	;;
	faddwp $r32r33.hi = $r32r33r34r35.y, -64
	;;
	faddwp $r34 = $r34r35.lo, -8589934592
	;;
	faddwp.ru.s $r32r33r34r35.z = $r35, $r34r35.hi
	;;
	faddwq.rd $r50r51 = $r48r49r50r51.hi, $r52r53
	;;
	faddw $r32r33r34r35.t = $r36, 2305843009213693951
	;;
	faddw $r36r37.lo = $r36r37r38r39.x, -64
	;;
	faddw $r37 = $r36r37.hi, -8589934592
	;;
	faddw.rz.s $r36r37r38r39.y = $r38, $r38r39.lo
	;;
	fcdivd $r36r37r38r39.z = $r52r53r54r55.lo
	;;
	fcdivwp.s $r39 = $r54r55
	;;
	fcdivw $r38r39.hi = $r52r53r54r55.hi
	;;
	fcompd.one $r36r37r38r39.t = $r40, $r40r41.lo
	;;
	fcompd.ueq $r40r41r42r43.x = $r41, 536870911
	;;
	fcompnhq.oeq $r40r41.hi = $r40r41r42r43.y, $r42
	;;
	fcompnhq.une $r42r43.lo = $r40r41r42r43.z, 536870911
	;;
	fcompnwp.olt $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	fcompnwp.uge.@ $r44 = $r44r45.lo, 536870911
	;;
	fcompw.oge $r44r45r46r47.x = $r45, $r44r45.hi
	;;
	fcompw.ult $r44r45r46r47.y = $r46, 536870911
	;;
	fdot2wdp.rna.s $r56r57 = $r56r57r58r59.lo, $r58r59
	;;
	fdot2wd $r46r47.lo = $r44r45r46r47.z, 2305843009213693951
	;;
	fdot2wd $r47 = $r46r47.hi, -64
	;;
	fdot2wd $r44r45r46r47.t = $r48, -8589934592
	;;
	fdot2wd.rnz $r48r49.lo = $r48r49r50r51.x, $r49
	;;
	fdot2wzp.ro.s $r56r57r58r59.hi = $r60r61, $r60r61r62r63.lo
	;;
	fdot2w $r48r49.hi = $r48r49r50r51.y, 2305843009213693951
	;;
	fdot2w $r50 = $r50r51.lo, -64
	;;
	fdot2w $r48r49r50r51.z = $r51, -8589934592
	;;
	fdot2w $r50r51.hi = $r48r49r50r51.t, $r52
	;;
	fence
	;;
	ffmad $r52r53.lo = $r52r53r54r55.x, 2305843009213693951
	;;
	ffmad $r53 = $r52r53.hi, -64
	;;
	ffmad $r52r53r54r55.y = $r54, -8589934592
	;;
	ffmad.rn.s $r54r55.lo = $r52r53r54r55.z, $r55
	;;
	ffmahq $r54r55.hi = $r52r53r54r55.t, 2305843009213693951
	;;
	ffmahq $r56 = $r56r57.lo, -64
	;;
	ffmahq $r56r57r58r59.x = $r57, -8589934592
	;;
	ffmahq.ru $r56r57.hi = $r56r57r58r59.y, $r58
	;;
	ffmahwq $r62r63 = $r58r59.lo, 2305843009213693951
	;;
	ffmahwq $r60r61r62r63.hi = $r56r57r58r59.z, -64
	;;
	ffmahwq $r0r1 = $r59, -8589934592
	;;
	ffmahwq.rd.s $r0r1r2r3.lo = $r58r59.hi, $r56r57r58r59.t
	;;
	ffmahw $r60 = $r60r61.lo, 2305843009213693951
	;;
	ffmahw $r60r61r62r63.x = $r61, -64
	;;
	ffmahw $r60r61.hi = $r60r61r62r63.y, -8589934592
	;;
	ffmahw.rz $r62 = $r62r63.lo, $r60r61r62r63.z
	;;
	ffmawdp $r2r3 = $r63, 2305843009213693951
	;;
	ffmawdp $r0r1r2r3.hi = $r62r63.hi, -64
	;;
	ffmawdp $r4r5 = $r60r61r62r63.t, -8589934592
	;;
	ffmawdp.rna.s $r4r5r6r7.lo = $r0, $r0r1.lo
	;;
	ffmawd $r0r1r2r3.x = $r1, 2305843009213693951
	;;
	ffmawd $r0r1.hi = $r0r1r2r3.y, -64
	;;
	ffmawd $r2 = $r2r3.lo, -8589934592
	;;
	ffmawd.rnz $r0r1r2r3.z = $r3, $r2r3.hi
	;;
	ffmawp $r0r1r2r3.t = $r4, 2305843009213693951
	;;
	ffmawp $r4r5.lo = $r4r5r6r7.x, -64
	;;
	ffmawp $r5 = $r4r5.hi, -8589934592
	;;
	ffmawp.ro.s $r4r5r6r7.y = $r6, $r6r7.lo
	;;
	ffmaw $r4r5r6r7.z = $r7, 2305843009213693951
	;;
	ffmaw $r6r7.hi = $r4r5r6r7.t, -64
	;;
	ffmaw $r8 = $r8r9.lo, -8589934592
	;;
	ffmaw $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	ffmsd $r8r9r10r11.y = $r10, 2305843009213693951
	;;
	ffmsd $r10r11.lo = $r8r9r10r11.z, -64
	;;
	ffmsd $r11 = $r10r11.hi, -8589934592
	;;
	ffmsd.rn.s $r8r9r10r11.t = $r12, $sp
	;;
	ffmshq $r13 = $tp, 2305843009213693951
	;;
	ffmshq $r14 = $fp, -64
	;;
	ffmshq $r15 = $rp, -8589934592
	;;
	ffmshq.ru $r16 = $r16r17.lo, $r16r17r18r19.x
	;;
	ffmshwq $r6r7 = $r17, 2305843009213693951
	;;
	ffmshwq $r4r5r6r7.hi = $r16r17.hi, -64
	;;
	ffmshwq $r8r9 = $r16r17r18r19.y, -8589934592
	;;
	ffmshwq.rd.s $r8r9r10r11.lo = $r18, $r18r19.lo
	;;
	ffmshw $r16r17r18r19.z = $r19, 2305843009213693951
	;;
	ffmshw $r18r19.hi = $r16r17r18r19.t, -64
	;;
	ffmshw $r20 = $r20r21.lo, -8589934592
	;;
	ffmshw.rz $r20r21r22r23.x = $r21, $r20r21.hi
	;;
	ffmswdp $r10r11 = $r20r21r22r23.y, 2305843009213693951
	;;
	ffmswdp $r8r9r10r11.hi = $r22, -64
	;;
	ffmswdp $r12r13 = $r22r23.lo, -8589934592
	;;
	ffmswdp.rna.s $r12r13r14r15.lo = $r20r21r22r23.z, $r23
	;;
	ffmswd $r22r23.hi = $r20r21r22r23.t, 2305843009213693951
	;;
	ffmswd $r24 = $r24r25.lo, -64
	;;
	ffmswd $r24r25r26r27.x = $r25, -8589934592
	;;
	ffmswd.rnz $r24r25.hi = $r24r25r26r27.y, $r26
	;;
	ffmswp $r26r27.lo = $r24r25r26r27.z, 2305843009213693951
	;;
	ffmswp $r27 = $r26r27.hi, -64
	;;
	ffmswp $r24r25r26r27.t = $r28, -8589934592
	;;
	ffmswp.ro.s $r28r29.lo = $r28r29r30r31.x, $r29
	;;
	ffmsw $r28r29.hi = $r28r29r30r31.y, 2305843009213693951
	;;
	ffmsw $r30 = $r30r31.lo, -64
	;;
	ffmsw $r28r29r30r31.z = $r31, -8589934592
	;;
	ffmsw $r30r31.hi = $r28r29r30r31.t, $r32
	;;
	fixedd.rn.s $r32r33.lo = $r32r33r34r35.x, 7
	;;
	fixedud.ru $r33 = $r32r33.hi, 7
	;;
	fixeduwp.rd.s $r32r33r34r35.y = $r34, 7
	;;
	fixeduw.rz $r34r35.lo = $r32r33r34r35.z, 7
	;;
	fixedwp.rna.s $r35 = $r34r35.hi, 7
	;;
	fixedw.rnz $r32r33r34r35.t = $r36, 7
	;;
	floatd.ro.s $r36r37.lo = $r36r37r38r39.x, 7
	;;
	floatud $r37 = $r36r37.hi, 7
	;;
	floatuwp.rn.s $r36r37r38r39.y = $r38, 7
	;;
	floatuw.ru $r38r39.lo = $r36r37r38r39.z, 7
	;;
	floatwp.rd.s $r39 = $r38r39.hi, 7
	;;
	floatw.rz $r36r37r38r39.t = $r40, 7
	;;
	fmaxd $r40r41.lo = $r40r41r42r43.x, $r41
	;;
	fmaxhq $r40r41.hi = $r40r41r42r43.y, $r42
	;;
	fmaxwp $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	fmaxw $r42r43.hi = $r40r41r42r43.t, $r44
	;;
	fmind $r44r45.lo = $r44r45r46r47.x, $r45
	;;
	fminhq $r44r45.hi = $r44r45r46r47.y, $r46
	;;
	fminwp $r46r47.lo = $r44r45r46r47.z, $r47
	;;
	fminw $r46r47.hi = $r44r45r46r47.t, $r48
	;;
	fmm212w.rna.s $r14r15 = $r48r49.lo, $r48r49r50r51.x
	;;
	fmma212w.rnz $r12r13r14r15.hi = $r49, $r48r49.hi
	;;
	fmma242hw0 $a0_lo = $a0a1, $a0a1.hi, $a0a1a2a3.y
	;;
	fmma242hw1 $a0_hi = $a0a1a2a3.lo, $a2, $a2a3.lo
	;;
	fmma242hw2 $a1_lo = $a2a3, $a0a1a2a3.z, $a3
	;;
	fmma242hw3 $a1_hi = $a0a1a2a3.hi, $a2a3.hi, $a0a1a2a3.t
	;;
	fmms212w.ro.s $r16r17 = $r48r49r50r51.y, $r50
	;;
	fmuld $r50r51.lo = $r48r49r50r51.z, 2305843009213693951
	;;
	fmuld $r51 = $r50r51.hi, -64
	;;
	fmuld $r48r49r50r51.t = $r52, -8589934592
	;;
	fmuld $r52r53.lo = $r52r53r54r55.x, $r53
	;;
	fmulhq $r52r53.hi = $r52r53r54r55.y, 2305843009213693951
	;;
	fmulhq $r54 = $r54r55.lo, -64
	;;
	fmulhq $r52r53r54r55.z = $r55, -8589934592
	;;
	fmulhq.rn.s $r54r55.hi = $r52r53r54r55.t, $r56
	;;
	fmulhwq $r16r17r18r19.lo = $r56r57.lo, 2305843009213693951
	;;
	fmulhwq $r18r19 = $r56r57r58r59.x, -64
	;;
	fmulhwq $r16r17r18r19.hi = $r57, -8589934592
	;;
	fmulhwq.ru $r20r21 = $r56r57.hi, $r56r57r58r59.y
	;;
	fmulhw $r58 = $r58r59.lo, 2305843009213693951
	;;
	fmulhw $r56r57r58r59.z = $r59, -64
	;;
	fmulhw $r58r59.hi = $r56r57r58r59.t, -8589934592
	;;
	fmulhw.rd.s $r60 = $r60r61.lo, $r60r61r62r63.x
	;;
	fmulwc.c $r61 = $r60r61.hi, 2305843009213693951
	;;
	fmulwc.c $r60r61r62r63.y = $r62, -64
	;;
	fmulwc.c $r62r63.lo = $r60r61r62r63.z, -8589934592
	;;
	fmulwc.c.rz $r63 = $r62r63.hi, $r60r61r62r63.t
	;;
	fmulwc $r0 = $r0r1.lo, 2305843009213693951
	;;
	fmulwc $r0r1r2r3.x = $r1, -64
	;;
	fmulwc $r0r1.hi = $r0r1r2r3.y, -8589934592
	;;
	fmulwc.rna.s $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	fmulwdc.c $r20r21r22r23.lo = $r3, 2305843009213693951
	;;
	fmulwdc.c $r22r23 = $r2r3.hi, -64
	;;
	fmulwdc.c $r20r21r22r23.hi = $r0r1r2r3.t, -8589934592
	;;
	fmulwdc.c.rnz $r24r25 = $r4, $r4r5.lo
	;;
	fmulwdc $r24r25r26r27.lo = $r4r5r6r7.x, 2305843009213693951
	;;
	fmulwdc $r26r27 = $r5, -64
	;;
	fmulwdc $r24r25r26r27.hi = $r4r5.hi, -8589934592
	;;
	fmulwdc.ro.s $r28r29 = $r4r5r6r7.y, $r6
	;;
	fmulwdp $r28r29r30r31.lo = $r6r7.lo, 2305843009213693951
	;;
	fmulwdp $r30r31 = $r4r5r6r7.z, -64
	;;
	fmulwdp $r28r29r30r31.hi = $r7, -8589934592
	;;
	fmulwdp $r32r33 = $r6r7.hi, $r4r5r6r7.t
	;;
	fmulwd $r8 = $r8r9.lo, 2305843009213693951
	;;
	fmulwd $r8r9r10r11.x = $r9, -64
	;;
	fmulwd $r8r9.hi = $r8r9r10r11.y, -8589934592
	;;
	fmulwd.rn.s $r10 = $r10r11.lo, $r8r9r10r11.z
	;;
	fmulwp $r11 = $r10r11.hi, 2305843009213693951
	;;
	fmulwp $r8r9r10r11.t = $r12, -64
	;;
	fmulwp $sp = $r13, -8589934592
	;;
	fmulwp.ru $tp = $r14, $fp
	;;
	fmulwq.rd.s $r32r33r34r35.lo = $r34r35, $r32r33r34r35.hi
	;;
	fmulw $r15 = $rp, 2305843009213693951
	;;
	fmulw $r16 = $r16r17.lo, -64
	;;
	fmulw $r16r17r18r19.x = $r17, -8589934592
	;;
	fmulw.rz $r16r17.hi = $r16r17r18r19.y, $r18
	;;
	fnarrow44wh.rna.s $a4 = $a4a5
	;;
	fnarrowdwp.rnz $r18r19.lo = $r36r37
	;;
	fnarrowdw.ro.s $r16r17r18r19.z = $r19
	;;
	fnarrowwhq $r18r19.hi = $r36r37r38r39.lo
	;;
	fnarrowwh.rn.s $r16r17r18r19.t = $r20
	;;
	fnegd $r20r21.lo = $r20r21r22r23.x
	;;
	fneghq $r21 = $r20r21.hi
	;;
	fnegwp $r20r21r22r23.y = $r22
	;;
	fnegw $r22r23.lo = $r20r21r22r23.z
	;;
	frecw.ru $r23 = $r22r23.hi
	;;
	frsrw.rd.s $r20r21r22r23.t = $r24
	;;
	fsbfdc.c.rz $r38r39 = $r36r37r38r39.hi, $r40r41
	;;
	fsbfdc.rna.s $r40r41r42r43.lo = $r42r43, $r40r41r42r43.hi
	;;
	fsbfdp.rnz $r44r45 = $r44r45r46r47.lo, $r46r47
	;;
	fsbfd $r24r25.lo = $r24r25r26r27.x, 2305843009213693951
	;;
	fsbfd $r25 = $r24r25.hi, -64
	;;
	fsbfd $r24r25r26r27.y = $r26, -8589934592
	;;
	fsbfd.ro.s $r26r27.lo = $r24r25r26r27.z, $r27
	;;
	fsbfhq $r26r27.hi = $r24r25r26r27.t, 2305843009213693951
	;;
	fsbfhq $r28 = $r28r29.lo, -64
	;;
	fsbfhq $r28r29r30r31.x = $r29, -8589934592
	;;
	fsbfhq $r28r29.hi = $r28r29r30r31.y, $r30
	;;
	fsbfwc.c $r30r31.lo = $r28r29r30r31.z, 2305843009213693951
	;;
	fsbfwc.c $r31 = $r30r31.hi, -64
	;;
	fsbfwc.c $r28r29r30r31.t = $r32, -8589934592
	;;
	fsbfwc.c.rn.s $r32r33.lo = $r32r33r34r35.x, $r33
	;;
	fsbfwcp.c.ru $r44r45r46r47.hi = $r48r49, $r48r49r50r51.lo
	;;
	fsbfwcp.rd.s $r50r51 = $r48r49r50r51.hi, $r52r53
	;;
	fsbfwc.rz $r32r33.hi = $r32r33r34r35.y, $r34
	;;
	fsbfwp $r34r35.lo = $r32r33r34r35.z, 2305843009213693951
	;;
	fsbfwp $r35 = $r34r35.hi, -64
	;;
	fsbfwp $r32r33r34r35.t = $r36, -8589934592
	;;
	fsbfwp.rna.s $r36r37.lo = $r36r37r38r39.x, $r37
	;;
	fsbfwq.rnz $r52r53r54r55.lo = $r54r55, $r52r53r54r55.hi
	;;
	fsbfw $r36r37.hi = $r36r37r38r39.y, 2305843009213693951
	;;
	fsbfw $r38 = $r38r39.lo, -64
	;;
	fsbfw $r36r37r38r39.z = $r39, -8589934592
	;;
	fsbfw.ro.s $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	fscalewv $a4a5.lo = $a4a5a6a7.x
	;;
	fsdivd.s $r40r41.lo = $r56r57
	;;
	fsdivwp $r40r41r42r43.x = $r56r57r58r59.lo
	;;
	fsdivw.s $r41 = $r58r59
	;;
	fsrecd $r40r41.hi = $r40r41r42r43.y
	;;
	fsrecwp.s $r42 = $r42r43.lo
	;;
	fsrecw $r40r41r42r43.z = $r43
	;;
	fsrsrd $r42r43.hi = $r40r41r42r43.t
	;;
	fsrsrwp $r44 = $r44r45.lo
	;;
	fsrsrw $r44r45r46r47.x = $r45
	;;
	fwidenlhwp.s $r44r45.hi = $r44r45r46r47.y
	;;
	fwidenlhw $r46 = $r46r47.lo
	;;
	fwidenlwd.s $r44r45r46r47.z = $r47
	;;
	fwidenmhwp $r46r47.hi = $r44r45r46r47.t
	;;
	fwidenmhw.s $r48 = $r48r49.lo
	;;
	fwidenmwd $r48r49r50r51.x = $r49
	;;
	get $r48r49.hi = $pc
	;;
	get $r48r49r50r51.y = $pc
	;;
	goto -33554432
	;;
	i1invals 2305843009213693951[$r50]
	;;
	i1invals.dlez $r50r51.lo? -1125899906842624[$r48r49r50r51.z]
	;;
	i1invals.dgtz $r51? -8388608[$r50r51.hi]
	;;
	i1invals.odd $r48r49r50r51.t? [$r52]
	;;
	i1invals $r52r53.lo[$r52r53r54r55.x]
	;;
	i1invals -64[$r53]
	;;
	i1invals -8589934592[$r52r53.hi]
	;;
	i1inval
	;;
	icall $r52r53r54r55.y
	;;
	iget $r54
	;;
	igoto $r54r55.lo
	;;
	insf $r52r53r54r55.z = $r55, 7, 7
	;;
	landd $r54r55.hi = $r52r53r54r55.t, $r56
	;;
	landd $r56r57.lo = $r56r57r58r59.x, 536870911
	;;
	landhq $r57 = $r56r57.hi, $r56r57r58r59.y
	;;
	landhq.@ $r58 = $r58r59.lo, 536870911
	;;
	landwp $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	landwp $r56r57r58r59.t = $r60, 536870911
	;;
	landw $r60r61.lo = $r60r61r62r63.x, $r61
	;;
	landw $r60r61.hi = $r60r61r62r63.y, 536870911
	;;
	lbs $r62 = $r62r63.lo[$r60r61r62r63.z]
	;;
	lbs.s.even $r63? $r62r63.hi = -1125899906842624[$r60r61r62r63.t]
	;;
	lbs.u.wnez $r0? $r0r1.lo = -8388608[$r0r1r2r3.x]
	;;
	lbs.us.weqz $r1? $r0r1.hi = [$r0r1r2r3.y]
	;;
	lbs $r2 = 2305843009213693951[$r2r3.lo]
	;;
	lbs.s $r0r1r2r3.z = -64[$r3]
	;;
	lbs.u $r2r3.hi = -8589934592[$r0r1r2r3.t]
	;;
	lbz.us.xs $r4 = $r4r5.lo[$r4r5r6r7.x]
	;;
	lbz.wltz $r5? $r4r5.hi = -1125899906842624[$r4r5r6r7.y]
	;;
	lbz.s.wgez $r6? $r6r7.lo = -8388608[$r4r5r6r7.z]
	;;
	lbz.u.wlez $r7? $r6r7.hi = [$r4r5r6r7.t]
	;;
	lbz.us $r8 = 2305843009213693951[$r8r9.lo]
	;;
	lbz $r8r9r10r11.x = -64[$r9]
	;;
	lbz.s $r8r9.hi = -8589934592[$r8r9r10r11.y]
	;;
	ld.u $r10 = $r10r11.lo[$r8r9r10r11.z]
	;;
	ld.us.wgtz $r11? $r10r11.hi = -1125899906842624[$r8r9r10r11.t]
	;;
	ld.dnez $r12? $sp = -8388608[$r13]
	;;
	ld.s.deqz $tp? $r14 = [$fp]
	;;
	ld.u $r15 = 2305843009213693951[$rp]
	;;
	ld.us $r16 = -64[$r16r17.lo]
	;;
	ld $r16r17r18r19.x = -8589934592[$r17]
	;;
	lhs.s.xs $r16r17.hi = $r16r17r18r19.y[$r18]
	;;
	lhs.u.dltz $r18r19.lo? $r16r17r18r19.z = -1125899906842624[$r19]
	;;
	lhs.us.dgez $r18r19.hi? $r16r17r18r19.t = -8388608[$r20]
	;;
	lhs.dlez $r20r21.lo? $r20r21r22r23.x = [$r21]
	;;
	lhs.s $r20r21.hi = 2305843009213693951[$r20r21r22r23.y]
	;;
	lhs.u $r22 = -64[$r22r23.lo]
	;;
	lhs.us $r20r21r22r23.z = -8589934592[$r23]
	;;
	lhz $r22r23.hi = $r20r21r22r23.t[$r24]
	;;
	lhz.s.dgtz $r24r25.lo? $r24r25r26r27.x = -1125899906842624[$r25]
	;;
	lhz.u.odd $r24r25.hi? $r24r25r26r27.y = -8388608[$r26]
	;;
	lhz.us.even $r26r27.lo? $r24r25r26r27.z = [$r27]
	;;
	lhz $r26r27.hi = 2305843009213693951[$r24r25r26r27.t]
	;;
	lhz.s $r28 = -64[$r28r29.lo]
	;;
	lhz.u $r28r29r30r31.x = -8589934592[$r29]
	;;
	lnandd $r28r29.hi = $r28r29r30r31.y, $r30
	;;
	lnandd.@ $r30r31.lo = $r28r29r30r31.z, 536870911
	;;
	lnandhq $r31 = $r30r31.hi, $r28r29r30r31.t
	;;
	lnandhq $r32 = $r32r33.lo, 536870911
	;;
	lnandwp $r32r33r34r35.x = $r33, $r32r33.hi
	;;
	lnandwp.@ $r32r33r34r35.y = $r34, 536870911
	;;
	lnandw $r34r35.lo = $r32r33r34r35.z, $r35
	;;
	lnandw $r34r35.hi = $r32r33r34r35.t, 536870911
	;;
	lnord $r36 = $r36r37.lo, $r36r37r38r39.x
	;;
	lnord $r37 = $r36r37.hi, 536870911
	;;
	lnorhq $r36r37r38r39.y = $r38, $r38r39.lo
	;;
	lnorhq.@ $r36r37r38r39.z = $r39, 536870911
	;;
	lnorwp $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	lnorwp $r40r41.lo = $r40r41r42r43.x, 536870911
	;;
	lnorw $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	lnorw $r42 = $r42r43.lo, 536870911
	;;
	loopdo $r40r41r42r43.z, -32768
	;;
	lord $r43 = $r42r43.hi, $r40r41r42r43.t
	;;
	lord.@ $r44 = $r44r45.lo, 536870911
	;;
	lorhq $r44r45r46r47.x = $r45, $r44r45.hi
	;;
	lorhq $r44r45r46r47.y = $r46, 536870911
	;;
	lorwp $r46r47.lo = $r44r45r46r47.z, $r47
	;;
	lorwp.@ $r46r47.hi = $r44r45r46r47.t, 536870911
	;;
	lorw $r48 = $r48r49.lo, $r48r49r50r51.x
	;;
	lorw $r49 = $r48r49.hi, 536870911
	;;
	lo.us.xs $r24r25r26r27 = $r48r49r50r51.y[$r50]
	;;
	lo.wnez $r50r51.lo? $r28r29r30r31 = -1125899906842624[$r48r49r50r51.z]
	;;
	lo.s.weqz $r51? $r32r33r34r35 = -8388608[$r50r51.hi]
	;;
	lo.u.wltz $r48r49r50r51.t? $r36r37r38r39 = [$r52]
	;;
	lo.us $r40r41r42r43 = 2305843009213693951[$r52r53.lo]
	;;
	lo $r44r45r46r47 = -64[$r52r53r54r55.x]
	;;
	lo.s $r48r49r50r51 = -8589934592[$r53]
	;;
	lq.u $r56r57r58r59.hi = $r52r53.hi[$r52r53r54r55.y]
	;;
	lq.us.wgez $r54? $r60r61 = -1125899906842624[$r54r55.lo]
	;;
	lq.wlez $r52r53r54r55.z? $r60r61r62r63.lo = -8388608[$r55]
	;;
	lq.s.wgtz $r54r55.hi? $r62r63 = [$r52r53r54r55.t]
	;;
	lq.u $r60r61r62r63.hi = 2305843009213693951[$r56]
	;;
	lq.us $r0r1 = -64[$r56r57.lo]
	;;
	lq $r0r1r2r3.lo = -8589934592[$r56r57r58r59.x]
	;;
	lws.s.xs $r57 = $r56r57.hi[$r56r57r58r59.y]
	;;
	lws.u.dnez $r58? $r58r59.lo = -1125899906842624[$r56r57r58r59.z]
	;;
	lws.us.deqz $r59? $r58r59.hi = -8388608[$r56r57r58r59.t]
	;;
	lws.dltz $r60? $r60r61.lo = [$r60r61r62r63.x]
	;;
	lws.s $r61 = 2305843009213693951[$r60r61.hi]
	;;
	lws.u $r60r61r62r63.y = -64[$r62]
	;;
	lws.us $r62r63.lo = -8589934592[$r60r61r62r63.z]
	;;
	lwz $r63 = $r62r63.hi[$r60r61r62r63.t]
	;;
	lwz.s.dgez $r0? $r0r1.lo = -1125899906842624[$r0r1r2r3.x]
	;;
	lwz.u.dlez $r1? $r0r1.hi = -8388608[$r0r1r2r3.y]
	;;
	lwz.us.dgtz $r2? $r2r3.lo = [$r0r1r2r3.z]
	;;
	lwz $r3 = 2305843009213693951[$r2r3.hi]
	;;
	lwz.s $r0r1r2r3.t = -64[$r4]
	;;
	lwz.u $r4r5.lo = -8589934592[$r4r5r6r7.x]
	;;
	madddt $r2r3 = $r5, 2305843009213693951
	;;
	madddt $r0r1r2r3.hi = $r4r5.hi, $r4r5r6r7.y
	;;
	madddt $r4r5 = $r6, -64
	;;
	madddt $r4r5r6r7.lo = $r6r7.lo, -8589934592
	;;
	maddd $r4r5r6r7.z = $r7, 2305843009213693951
	;;
	maddd $r6r7.hi = $r4r5r6r7.t, $r8
	;;
	maddd $r8r9.lo = $r8r9r10r11.x, -64
	;;
	maddd $r9 = $r8r9.hi, -8589934592
	;;
	maddhq $r8r9r10r11.y = $r10, 2305843009213693951
	;;
	maddhq $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	maddhq $r10r11.hi = $r8r9r10r11.t, -64
	;;
	maddhq $r12 = $sp, -8589934592
	;;
	maddhwq $r6r7 = $r13, $tp
	;;
	maddsudt $r4r5r6r7.hi = $r14, 2305843009213693951
	;;
	maddsudt $r8r9 = $fp, $r15
	;;
	maddsudt $r8r9r10r11.lo = $rp, -64
	;;
	maddsudt $r10r11 = $r16, -8589934592
	;;
	maddsuhwq $r8r9r10r11.hi = $r16r17.lo, $r16r17r18r19.x
	;;
	maddsuwdp $r12r13 = $r17, $r16r17.hi
	;;
	maddsuwd $r16r17r18r19.y = $r18, $r18r19.lo
	;;
	maddsuwd $r16r17r18r19.z = $r19, 536870911
	;;
	maddudt $r12r13r14r15.lo = $r18r19.hi, 2305843009213693951
	;;
	maddudt $r14r15 = $r16r17r18r19.t, $r20
	;;
	maddudt $r12r13r14r15.hi = $r20r21.lo, -64
	;;
	maddudt $r16r17 = $r20r21r22r23.x, -8589934592
	;;
	madduhwq $r16r17r18r19.lo = $r21, $r20r21.hi
	;;
	madduwdp $r18r19 = $r20r21r22r23.y, $r22
	;;
	madduwd $r22r23.lo = $r20r21r22r23.z, $r23
	;;
	madduwd $r22r23.hi = $r20r21r22r23.t, 536870911
	;;
	madduzdt $r16r17r18r19.hi = $r24, 2305843009213693951
	;;
	madduzdt $r20r21 = $r24r25.lo, $r24r25r26r27.x
	;;
	madduzdt $r20r21r22r23.lo = $r25, -64
	;;
	madduzdt $r22r23 = $r24r25.hi, -8589934592
	;;
	maddwdp $r20r21r22r23.hi = $r24r25r26r27.y, $r26
	;;
	maddwd $r26r27.lo = $r24r25r26r27.z, $r27
	;;
	maddwd $r26r27.hi = $r24r25r26r27.t, 536870911
	;;
	maddwp $r28 = $r28r29.lo, 2305843009213693951
	;;
	maddwp $r28r29r30r31.x = $r29, $r28r29.hi
	;;
	maddwp $r28r29r30r31.y = $r30, -64
	;;
	maddwp $r30r31.lo = $r28r29r30r31.z, -8589934592
	;;
	maddw $r31 = $r30r31.hi, $r28r29r30r31.t
	;;
	maddw $r32 = $r32r33.lo, 536870911
	;;
	make $r32r33r34r35.x = 2305843009213693951
	;;
	make $r33 = -549755813888
	;;
	make $r32r33.hi = -4096
	;;
	maxd $r32r33r34r35.y = $r34, 2305843009213693951
	;;
	maxd $r34r35.lo = $r32r33r34r35.z, $r35
	;;
	maxd $r34r35.hi = $r32r33r34r35.t, -64
	;;
	maxd $r36 = $r36r37.lo, -8589934592
	;;
	maxd.@ $r36r37r38r39.x = $r37, 536870911
	;;
	maxhq $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	maxhq $r38r39.lo = $r36r37r38r39.z, 536870911
	;;
	maxud $r39 = $r38r39.hi, 2305843009213693951
	;;
	maxud $r36r37r38r39.t = $r40, $r40r41.lo
	;;
	maxud $r40r41r42r43.x = $r41, -64
	;;
	maxud $r40r41.hi = $r40r41r42r43.y, -8589934592
	;;
	maxud.@ $r42 = $r42r43.lo, 536870911
	;;
	maxuhq $r40r41r42r43.z = $r43, $r42r43.hi
	;;
	maxuhq.@ $r40r41r42r43.t = $r44, 536870911
	;;
	maxuwp $r44r45.lo = $r44r45r46r47.x, $r45
	;;
	maxuwp $r44r45.hi = $r44r45r46r47.y, 536870911
	;;
	maxuw $r46 = $r46r47.lo, $r44r45r46r47.z
	;;
	maxuw $r47 = $r46r47.hi, -64
	;;
	maxuw $r44r45r46r47.t = $r48, -8589934592
	;;
	maxwp $r48r49.lo = $r48r49r50r51.x, $r49
	;;
	maxwp.@ $r48r49.hi = $r48r49r50r51.y, 536870911
	;;
	maxw $r50 = $r50r51.lo, $r48r49r50r51.z
	;;
	maxw $r51 = $r50r51.hi, -64
	;;
	maxw $r48r49r50r51.t = $r52, -8589934592
	;;
	mind $r52r53.lo = $r52r53r54r55.x, 2305843009213693951
	;;
	mind $r53 = $r52r53.hi, $r52r53r54r55.y
	;;
	mind $r54 = $r54r55.lo, -64
	;;
	mind $r52r53r54r55.z = $r55, -8589934592
	;;
	mind.@ $r54r55.hi = $r52r53r54r55.t, 536870911
	;;
	minhq $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	minhq $r57 = $r56r57.hi, 536870911
	;;
	minud $r56r57r58r59.y = $r58, 2305843009213693951
	;;
	minud $r58r59.lo = $r56r57r58r59.z, $r59
	;;
	minud $r58r59.hi = $r56r57r58r59.t, -64
	;;
	minud $r60 = $r60r61.lo, -8589934592
	;;
	minud.@ $r60r61r62r63.x = $r61, 536870911
	;;
	minuhq $r60r61.hi = $r60r61r62r63.y, $r62
	;;
	minuhq.@ $r62r63.lo = $r60r61r62r63.z, 536870911
	;;
	minuwp $r63 = $r62r63.hi, $r60r61r62r63.t
	;;
	minuwp $r0 = $r0r1.lo, 536870911
	;;
	minuw $r0r1r2r3.x = $r1, $r0r1.hi
	;;
	minuw $r0r1r2r3.y = $r2, -64
	;;
	minuw $r2r3.lo = $r0r1r2r3.z, -8589934592
	;;
	minwp $r3 = $r2r3.hi, $r0r1r2r3.t
	;;
	minwp.@ $r4 = $r4r5.lo, 536870911
	;;
	minw $r4r5r6r7.x = $r5, $r4r5.hi
	;;
	minw $r4r5r6r7.y = $r6, -64
	;;
	minw $r6r7.lo = $r4r5r6r7.z, -8589934592
	;;
	mm212w $r24r25 = $r7, $r6r7.hi
	;;
	mma212w $r24r25r26r27.lo = $r4r5r6r7.t, $r8
	;;
	mma444hbd0 $a24a25a26a27 = $a28a29a30a31, $a5, $a4a5.hi
	;;
	mma444hbd1 $a32a33a34a35 = $a36a37a38a39, $a4a5a6a7.y, $a6
	;;
	mma444hd $a40a41a42a43 = $a44a45a46a47, $a6a7.lo, $a4a5a6a7.z
	;;
	mma444suhbd0 $a48a49a50a51 = $a52a53a54a55, $a7, $a6a7.hi
	;;
	mma444suhbd1 $a56a57a58a59 = $a60a61a62a63, $a4a5a6a7.t, $a8
	;;
	mma444suhd $a0a1a2a3 = $a4a5a6a7, $a8a9.lo, $a8a9a10a11.x
	;;
	mma444uhbd0 $a8a9a10a11 = $a12a13a14a15, $a9, $a8a9.hi
	;;
	mma444uhbd1 $a16a17a18a19 = $a20a21a22a23, $a8a9a10a11.y, $a10
	;;
	mma444uhd $a24a25a26a27 = $a28a29a30a31, $a10a11.lo, $a8a9a10a11.z
	;;
	mma444ushbd0 $a32a33a34a35 = $a36a37a38a39, $a11, $a10a11.hi
	;;
	mma444ushbd1 $a40a41a42a43 = $a44a45a46a47, $a8a9a10a11.t, $a12
	;;
	mma444ushd $a48a49a50a51 = $a52a53a54a55, $a12a13.lo, $a12a13a14a15.x
	;;
	mms212w $r26r27 = $r8r9.lo, $r8r9r10r11.x
	;;
	movetq $a0.lo = $r9, $r8r9.hi
	;;
	movetq $a0.hi = $r8r9r10r11.y, $r10
	;;
	msbfdt $r24r25r26r27.hi = $r10r11.lo, $r8r9r10r11.z
	;;
	msbfd $r11 = $r10r11.hi, $r8r9r10r11.t
	;;
	msbfhq $r12 = $sp, $r13
	;;
	msbfhwq $r28r29 = $tp, $r14
	;;
	msbfsudt $r28r29r30r31.lo = $fp, $r15
	;;
	msbfsuhwq $r30r31 = $rp, $r16
	;;
	msbfsuwdp $r28r29r30r31.hi = $r16r17.lo, $r16r17r18r19.x
	;;
	msbfsuwd $r17 = $r16r17.hi, $r16r17r18r19.y
	;;
	msbfsuwd $r18 = $r18r19.lo, 536870911
	;;
	msbfudt $r32r33 = $r16r17r18r19.z, $r19
	;;
	msbfuhwq $r32r33r34r35.lo = $r18r19.hi, $r16r17r18r19.t
	;;
	msbfuwdp $r34r35 = $r20, $r20r21.lo
	;;
	msbfuwd $r20r21r22r23.x = $r21, $r20r21.hi
	;;
	msbfuwd $r20r21r22r23.y = $r22, 536870911
	;;
	msbfuzdt $r32r33r34r35.hi = $r22r23.lo, $r20r21r22r23.z
	;;
	msbfwdp $r36r37 = $r23, $r22r23.hi
	;;
	msbfwd $r20r21r22r23.t = $r24, $r24r25.lo
	;;
	msbfwd $r24r25r26r27.x = $r25, 536870911
	;;
	msbfwp $r24r25.hi = $r24r25r26r27.y, $r26
	;;
	msbfw $r26r27.lo = $r24r25r26r27.z, $r27
	;;
	msbfw $r26r27.hi = $r24r25r26r27.t, 536870911
	;;
	muldt $r36r37r38r39.lo = $r28, 2305843009213693951
	;;
	muldt $r38r39 = $r28r29.lo, $r28r29r30r31.x
	;;
	muldt $r36r37r38r39.hi = $r29, -64
	;;
	muldt $r40r41 = $r28r29.hi, -8589934592
	;;
	muld $r28r29r30r31.y = $r30, 2305843009213693951
	;;
	muld $r30r31.lo = $r28r29r30r31.z, $r31
	;;
	muld $r30r31.hi = $r28r29r30r31.t, -64
	;;
	muld $r32 = $r32r33.lo, -8589934592
	;;
	mulhq $r32r33r34r35.x = $r33, 2305843009213693951
	;;
	mulhq $r32r33.hi = $r32r33r34r35.y, $r34
	;;
	mulhq $r34r35.lo = $r32r33r34r35.z, -64
	;;
	mulhq $r35 = $r34r35.hi, -8589934592
	;;
	mulhwq $r40r41r42r43.lo = $r32r33r34r35.t, $r36
	;;
	mulsudt $r42r43 = $r36r37.lo, 2305843009213693951
	;;
	mulsudt $r40r41r42r43.hi = $r36r37r38r39.x, $r37
	;;
	mulsudt $r44r45 = $r36r37.hi, -64
	;;
	mulsudt $r44r45r46r47.lo = $r36r37r38r39.y, -8589934592
	;;
	mulsuhwq $r46r47 = $r38, $r38r39.lo
	;;
	mulsuwdp $r44r45r46r47.hi = $r36r37r38r39.z, $r39
	;;
	mulsuwd $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	mulsuwd $r40r41.lo = $r40r41r42r43.x, 536870911
	;;
	muludt $r48r49 = $r41, 2305843009213693951
	;;
	muludt $r48r49r50r51.lo = $r40r41.hi, $r40r41r42r43.y
	;;
	muludt $r50r51 = $r42, -64
	;;
	muludt $r48r49r50r51.hi = $r42r43.lo, -8589934592
	;;
	muluhwq $r52r53 = $r40r41r42r43.z, $r43
	;;
	muluwdp $r52r53r54r55.lo = $r42r43.hi, $r40r41r42r43.t
	;;
	muluwd $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	muluwd $r45 = $r44r45.hi, 536870911
	;;
	mulwc.c $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	mulwc $r44r45r46r47.z = $r47, 2305843009213693951
	;;
	mulwc $r46r47.hi = $r44r45r46r47.t, $r48
	;;
	mulwc $r48r49.lo = $r48r49r50r51.x, -64
	;;
	mulwc $r49 = $r48r49.hi, -8589934592
	;;
	mulwdc.c $r54r55 = $r48r49r50r51.y, $r50
	;;
	mulwdc $r52r53r54r55.hi = $r50r51.lo, $r48r49r50r51.z
	;;
	mulwdp $r56r57 = $r51, $r50r51.hi
	;;
	mulwd $r48r49r50r51.t = $r52, $r52r53.lo
	;;
	mulwd $r52r53r54r55.x = $r53, 536870911
	;;
	mulwp $r52r53.hi = $r52r53r54r55.y, 2305843009213693951
	;;
	mulwp $r54 = $r54r55.lo, $r52r53r54r55.z
	;;
	mulwp $r55 = $r54r55.hi, -64
	;;
	mulwp $r52r53r54r55.t = $r56, -8589934592
	;;
	mulwq $r56r57r58r59.lo = $r58r59, $r56r57r58r59.hi
	;;
	mulw $r56r57.lo = $r56r57r58r59.x, $r57
	;;
	mulw $r56r57.hi = $r56r57r58r59.y, 536870911
	;;
	nandd $r58 = $r58r59.lo, 2305843009213693951
	;;
	nandd $r56r57r58r59.z = $r59, $r58r59.hi
	;;
	nandd $r56r57r58r59.t = $r60, -64
	;;
	nandd $r60r61.lo = $r60r61r62r63.x, -8589934592
	;;
	nandd.@ $r61 = $r60r61.hi, 536870911
	;;
	nandw $r60r61r62r63.y = $r62, $r62r63.lo
	;;
	nandw $r60r61r62r63.z = $r63, -64
	;;
	nandw $r62r63.hi = $r60r61r62r63.t, -8589934592
	;;
	negd $r0 = $r0r1.lo
	;;
	neghq $r0r1r2r3.x = $r1
	;;
	negwp $r0r1.hi = $r0r1r2r3.y
	;;
	negw $r2 = $r2r3.lo
	;;
	nop
	;;
	nord $r0r1r2r3.z = $r3, 2305843009213693951
	;;
	nord $r2r3.hi = $r0r1r2r3.t, $r4
	;;
	nord $r4r5.lo = $r4r5r6r7.x, -64
	;;
	nord $r5 = $r4r5.hi, -8589934592
	;;
	nord.@ $r4r5r6r7.y = $r6, 536870911
	;;
	norw $r6r7.lo = $r4r5r6r7.z, $r7
	;;
	norw $r6r7.hi = $r4r5r6r7.t, -64
	;;
	norw $r8 = $r8r9.lo, -8589934592
	;;
	notd $r8r9r10r11.x = $r9
	;;
	notw $r8r9.hi = $r8r9r10r11.y
	;;
	nxord $r10 = $r10r11.lo, 2305843009213693951
	;;
	nxord $r8r9r10r11.z = $r11, $r10r11.hi
	;;
	nxord $r8r9r10r11.t = $r12, -64
	;;
	nxord $sp = $r13, -8589934592
	;;
	nxord.@ $tp = $r14, 536870911
	;;
	nxorw $fp = $r15, $rp
	;;
	nxorw $r16 = $r16r17.lo, -64
	;;
	nxorw $r16r17r18r19.x = $r17, -8589934592
	;;
	ord $r16r17.hi = $r16r17r18r19.y, 2305843009213693951
	;;
	ord $r18 = $r18r19.lo, $r16r17r18r19.z
	;;
	ord $r19 = $r18r19.hi, -64
	;;
	ord $r16r17r18r19.t = $r20, -8589934592
	;;
	ord.@ $r20r21.lo = $r20r21r22r23.x, 536870911
	;;
	ornd $r21 = $r20r21.hi, 2305843009213693951
	;;
	ornd $r20r21r22r23.y = $r22, $r22r23.lo
	;;
	ornd $r20r21r22r23.z = $r23, -64
	;;
	ornd $r22r23.hi = $r20r21r22r23.t, -8589934592
	;;
	ornd.@ $r24 = $r24r25.lo, 536870911
	;;
	ornw $r24r25r26r27.x = $r25, $r24r25.hi
	;;
	ornw $r24r25r26r27.y = $r26, -64
	;;
	ornw $r26r27.lo = $r24r25r26r27.z, -8589934592
	;;
	orw $r27 = $r26r27.hi, $r24r25r26r27.t
	;;
	orw $r28 = $r28r29.lo, -64
	;;
	orw $r28r29r30r31.x = $r29, -8589934592
	;;
	pcrel $r28r29.hi = 2305843009213693951
	;;
	pcrel $r28r29r30r31.y = -549755813888
	;;
	pcrel $r30 = -4096
	;;
	ret
	;;
	rfe
	;;
	rolwps $r30r31.lo = $r28r29r30r31.z, $r31
	;;
	rolwps $r30r31.hi = $r28r29r30r31.t, 7
	;;
	rolw $r32 = $r32r33.lo, $r32r33r34r35.x
	;;
	rolw $r33 = $r32r33.hi, 7
	;;
	rorwps $r32r33r34r35.y = $r34, $r34r35.lo
	;;
	rorwps $r32r33r34r35.z = $r35, 7
	;;
	rorw $r34r35.hi = $r32r33r34r35.t, $r36
	;;
	rorw $r36r37.lo = $r36r37r38r39.x, 7
	;;
	rswap $r37 = $mmc
	;;
	rswap $r36r37.hi = $s0
	;;
	rswap $r36r37r38r39.y = $pc
	;;
	satdh $r38 = $r38r39.lo
	;;
	satdw $r36r37r38r39.z = $r39
	;;
	satd $r38r39.hi = $r36r37r38r39.t, $r40
	;;
	satd $r40r41.lo = $r40r41r42r43.x, 7
	;;
	sbfcd.i $r41 = $r40r41.hi, $r40r41r42r43.y
	;;
	sbfcd.i $r42 = $r42r43.lo, 536870911
	;;
	sbfcd $r40r41r42r43.z = $r43, $r42r43.hi
	;;
	sbfcd $r40r41r42r43.t = $r44, 536870911
	;;
	sbfd $r44r45.lo = $r44r45r46r47.x, 2305843009213693951
	;;
	sbfd $r45 = $r44r45.hi, $r44r45r46r47.y
	;;
	sbfd $r46 = $r46r47.lo, -64
	;;
	sbfd $r44r45r46r47.z = $r47, -8589934592
	;;
	sbfd.@ $r46r47.hi = $r44r45r46r47.t, 536870911
	;;
	sbfhcp.c $r48 = $r48r49.lo, $r48r49r50r51.x
	;;
	sbfhcp.c $r49 = $r48r49.hi, 536870911
	;;
	sbfhq $r48r49r50r51.y = $r50, $r50r51.lo
	;;
	sbfhq.@ $r48r49r50r51.z = $r51, 536870911
	;;
	sbfsd $r50r51.hi = $r48r49r50r51.t, 2305843009213693951
	;;
	sbfsd $r52 = $r52r53.lo, $r52r53r54r55.x
	;;
	sbfsd $r53 = $r52r53.hi, -64
	;;
	sbfsd $r52r53r54r55.y = $r54, -8589934592
	;;
	sbfshq $r54r55.lo = $r52r53r54r55.z, $r55
	;;
	sbfshq $r54r55.hi = $r52r53r54r55.t, 536870911
	;;
	sbfswp $r56 = $r56r57.lo, $r56r57r58r59.x
	;;
	sbfswp.@ $r57 = $r56r57.hi, 536870911
	;;
	sbfsw $r56r57r58r59.y = $r58, $r58r59.lo
	;;
	sbfsw $r56r57r58r59.z = $r59, 536870911
	;;
	sbfuwd $r58r59.hi = $r56r57r58r59.t, $r60
	;;
	sbfuwd $r60r61.lo = $r60r61r62r63.x, 536870911
	;;
	sbfwc.c $r61 = $r60r61.hi, $r60r61r62r63.y
	;;
	sbfwc.c.@ $r62 = $r62r63.lo, 536870911
	;;
	sbfwd $r60r61r62r63.z = $r63, $r62r63.hi
	;;
	sbfwd $r60r61r62r63.t = $r0, 536870911
	;;
	sbfwp $r0r1.lo = $r0r1r2r3.x, $r1
	;;
	sbfwp.@ $r0r1.hi = $r0r1r2r3.y, 536870911
	;;
	sbfw $r2 = $r2r3.lo, $r0r1r2r3.z
	;;
	sbfw $r3 = $r2r3.hi, -64
	;;
	sbfw $r0r1r2r3.t = $r4, -8589934592
	;;
	sbfx16d $r4r5.lo = $r4r5r6r7.x, $r5
	;;
	sbfx16d $r4r5.hi = $r4r5r6r7.y, 536870911
	;;
	sbfx16hq $r6 = $r6r7.lo, $r4r5r6r7.z
	;;
	sbfx16hq.@ $r7 = $r6r7.hi, 536870911
	;;
	sbfx16uwd $r4r5r6r7.t = $r8, $r8r9.lo
	;;
	sbfx16uwd $r8r9r10r11.x = $r9, 536870911
	;;
	sbfx16wd $r8r9.hi = $r8r9r10r11.y, $r10
	;;
	sbfx16wd $r10r11.lo = $r8r9r10r11.z, 536870911
	;;
	sbfx16wp $r11 = $r10r11.hi, $r8r9r10r11.t
	;;
	sbfx16wp $r12 = $sp, 536870911
	;;
	sbfx16w $r13 = $tp, $r14
	;;
	sbfx16w $fp = $r15, 536870911
	;;
	sbfx2d $rp = $r16, $r16r17.lo
	;;
	sbfx2d.@ $r16r17r18r19.x = $r17, 536870911
	;;
	sbfx2hq $r16r17.hi = $r16r17r18r19.y, $r18
	;;
	sbfx2hq $r18r19.lo = $r16r17r18r19.z, 536870911
	;;
	sbfx2uwd $r19 = $r18r19.hi, $r16r17r18r19.t
	;;
	sbfx2uwd $r20 = $r20r21.lo, 536870911
	;;
	sbfx2wd $r20r21r22r23.x = $r21, $r20r21.hi
	;;
	sbfx2wd $r20r21r22r23.y = $r22, 536870911
	;;
	sbfx2wp $r22r23.lo = $r20r21r22r23.z, $r23
	;;
	sbfx2wp.@ $r22r23.hi = $r20r21r22r23.t, 536870911
	;;
	sbfx2w $r24 = $r24r25.lo, $r24r25r26r27.x
	;;
	sbfx2w $r25 = $r24r25.hi, 536870911
	;;
	sbfx4d $r24r25r26r27.y = $r26, $r26r27.lo
	;;
	sbfx4d $r24r25r26r27.z = $r27, 536870911
	;;
	sbfx4hq $r26r27.hi = $r24r25r26r27.t, $r28
	;;
	sbfx4hq.@ $r28r29.lo = $r28r29r30r31.x, 536870911
	;;
	sbfx4uwd $r29 = $r28r29.hi, $r28r29r30r31.y
	;;
	sbfx4uwd $r30 = $r30r31.lo, 536870911
	;;
	sbfx4wd $r28r29r30r31.z = $r31, $r30r31.hi
	;;
	sbfx4wd $r28r29r30r31.t = $r32, 536870911
	;;
	sbfx4wp $r32r33.lo = $r32r33r34r35.x, $r33
	;;
	sbfx4wp $r32r33.hi = $r32r33r34r35.y, 536870911
	;;
	sbfx4w $r34 = $r34r35.lo, $r32r33r34r35.z
	;;
	sbfx4w $r35 = $r34r35.hi, 536870911
	;;
	sbfx8d $r32r33r34r35.t = $r36, $r36r37.lo
	;;
	sbfx8d.@ $r36r37r38r39.x = $r37, 536870911
	;;
	sbfx8hq $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	sbfx8hq $r38r39.lo = $r36r37r38r39.z, 536870911
	;;
	sbfx8uwd $r39 = $r38r39.hi, $r36r37r38r39.t
	;;
	sbfx8uwd $r40 = $r40r41.lo, 536870911
	;;
	sbfx8wd $r40r41r42r43.x = $r41, $r40r41.hi
	;;
	sbfx8wd $r40r41r42r43.y = $r42, 536870911
	;;
	sbfx8wp $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	sbfx8wp.@ $r42r43.hi = $r40r41r42r43.t, 536870911
	;;
	sbfx8w $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	sbfx8w $r45 = $r44r45.hi, 536870911
	;;
	sbmm8 $r44r45r46r47.y = $r46, 2305843009213693951
	;;
	sbmm8 $r46r47.lo = $r44r45r46r47.z, $r47
	;;
	sbmm8 $r46r47.hi = $r44r45r46r47.t, -64
	;;
	sbmm8 $r48 = $r48r49.lo, -8589934592
	;;
	sbmm8.@ $r48r49r50r51.x = $r49, 536870911
	;;
	sbmmt8 $r48r49.hi = $r48r49r50r51.y, 2305843009213693951
	;;
	sbmmt8 $r50 = $r50r51.lo, $r48r49r50r51.z
	;;
	sbmmt8 $r51 = $r50r51.hi, -64
	;;
	sbmmt8 $r48r49r50r51.t = $r52, -8589934592
	;;
	sbmmt8.@ $r52r53.lo = $r52r53r54r55.x, 536870911
	;;
	sb.xs $r53[$r52r53.hi] = $r52r53r54r55.y
	;;
	sb 2305843009213693951[$r54] = $r54r55.lo
	;;
	sb.odd $r52r53r54r55.z? -1125899906842624[$r55] = $r54r55.hi
	;;
	sb.even $r52r53r54r55.t? -8388608[$r56] = $r56r57.lo
	;;
	sb.wnez $r56r57r58r59.x? [$r57] = $r56r57.hi
	;;
	sb -64[$r56r57r58r59.y] = $r58
	;;
	sb -8589934592[$r58r59.lo] = $r56r57r58r59.z
	;;
	scall $r59
	;;
	scall 511
	;;
	sd $r58r59.hi[$r56r57r58r59.t] = $r60
	;;
	sd 2305843009213693951[$r60r61.lo] = $r60r61r62r63.x
	;;
	sd.weqz $r61? -1125899906842624[$r60r61.hi] = $r60r61r62r63.y
	;;
	sd.wltz $r62? -8388608[$r62r63.lo] = $r60r61r62r63.z
	;;
	sd.wgez $r63? [$r62r63.hi] = $r60r61r62r63.t
	;;
	sd -64[$r0] = $r0r1.lo
	;;
	sd -8589934592[$r0r1r2r3.x] = $r1
	;;
	set $s28 = $r0r1.hi
	;;
	set $ra = $r0r1r2r3.y
	;;
	set $ps = $r2
	;;
	set $ps = $r2r3.lo
	;;
	sh.xs $r0r1r2r3.z[$r3] = $r2r3.hi
	;;
	sh 2305843009213693951[$r0r1r2r3.t] = $r4
	;;
	sh.wlez $r4r5.lo? -1125899906842624[$r4r5r6r7.x] = $r5
	;;
	sh.wgtz $r4r5.hi? -8388608[$r4r5r6r7.y] = $r6
	;;
	sh.dnez $r6r7.lo? [$r4r5r6r7.z] = $r7
	;;
	sh -64[$r6r7.hi] = $r4r5r6r7.t
	;;
	sh -8589934592[$r8] = $r8r9.lo
	;;
	sleep
	;;
	slld $r8r9r10r11.x = $r9, $r8r9.hi
	;;
	slld $r8r9r10r11.y = $r10, 7
	;;
	sllhqs $r10r11.lo = $r8r9r10r11.z, $r11
	;;
	sllhqs $r10r11.hi = $r8r9r10r11.t, 7
	;;
	sllwps $r12 = $sp, $r13
	;;
	sllwps $tp = $r14, 7
	;;
	sllw $fp = $r15, $rp
	;;
	sllw $r16 = $r16r17.lo, 7
	;;
	slsd $r16r17r18r19.x = $r17, $r16r17.hi
	;;
	slsd $r16r17r18r19.y = $r18, 7
	;;
	slshqs $r18r19.lo = $r16r17r18r19.z, $r19
	;;
	slshqs $r18r19.hi = $r16r17r18r19.t, 7
	;;
	slswps $r20 = $r20r21.lo, $r20r21r22r23.x
	;;
	slswps $r21 = $r20r21.hi, 7
	;;
	slsw $r20r21r22r23.y = $r22, $r22r23.lo
	;;
	slsw $r20r21r22r23.z = $r23, 7
	;;
	so $r22r23.hi[$r20r21r22r23.t] = $r52r53r54r55
	;;
	so 2305843009213693951[$r24] = $r56r57r58r59
	;;
	so.deqz $r24r25.lo? -1125899906842624[$r24r25r26r27.x] = $r60r61r62r63
	;;
	so.dltz $r25? -8388608[$r24r25.hi] = $r0r1r2r3
	;;
	so.dgez $r24r25r26r27.y? [$r26] = $r4r5r6r7
	;;
	so -64[$r26r27.lo] = $r8r9r10r11
	;;
	so -8589934592[$r24r25r26r27.z] = $r12r13r14r15
	;;
	sq.xs $r27[$r26r27.hi] = $r60r61
	;;
	sq 2305843009213693951[$r24r25r26r27.t] = $r60r61r62r63.lo
	;;
	sq.dlez $r28? -1125899906842624[$r28r29.lo] = $r62r63
	;;
	sq.dgtz $r28r29r30r31.x? -8388608[$r29] = $r60r61r62r63.hi
	;;
	sq.odd $r28r29.hi? [$r28r29r30r31.y] = $r0r1
	;;
	sq -64[$r30] = $r0r1r2r3.lo
	;;
	sq -8589934592[$r30r31.lo] = $r2r3
	;;
	srad $r28r29r30r31.z = $r31, $r30r31.hi
	;;
	srad $r28r29r30r31.t = $r32, 7
	;;
	srahqs $r32r33.lo = $r32r33r34r35.x, $r33
	;;
	srahqs $r32r33.hi = $r32r33r34r35.y, 7
	;;
	srawps $r34 = $r34r35.lo, $r32r33r34r35.z
	;;
	srawps $r35 = $r34r35.hi, 7
	;;
	sraw $r32r33r34r35.t = $r36, $r36r37.lo
	;;
	sraw $r36r37r38r39.x = $r37, 7
	;;
	srld $r36r37.hi = $r36r37r38r39.y, $r38
	;;
	srld $r38r39.lo = $r36r37r38r39.z, 7
	;;
	srlhqs $r39 = $r38r39.hi, $r36r37r38r39.t
	;;
	srlhqs $r40 = $r40r41.lo, 7
	;;
	srlwps $r40r41r42r43.x = $r41, $r40r41.hi
	;;
	srlwps $r40r41r42r43.y = $r42, 7
	;;
	srlw $r42r43.lo = $r40r41r42r43.z, $r43
	;;
	srlw $r42r43.hi = $r40r41r42r43.t, 7
	;;
	srsd $r44 = $r44r45.lo, $r44r45r46r47.x
	;;
	srsd $r45 = $r44r45.hi, 7
	;;
	srshqs $r44r45r46r47.y = $r46, $r46r47.lo
	;;
	srshqs $r44r45r46r47.z = $r47, 7
	;;
	srswps $r46r47.hi = $r44r45r46r47.t, $r48
	;;
	srswps $r48r49.lo = $r48r49r50r51.x, 7
	;;
	srsw $r49 = $r48r49.hi, $r48r49r50r51.y
	;;
	srsw $r50 = $r50r51.lo, 7
	;;
	stop
	;;
	stsud $r48r49r50r51.z = $r51, $r50r51.hi
	;;
	stsuw $r48r49r50r51.t = $r52, $r52r53.lo
	;;
	sw $r52r53r54r55.x[$r53] = $r52r53.hi
	;;
	sw 2305843009213693951[$r52r53r54r55.y] = $r54
	;;
	sw.even $r54r55.lo? -1125899906842624[$r52r53r54r55.z] = $r55
	;;
	sw.wnez $r54r55.hi? -8388608[$r52r53r54r55.t] = $r56
	;;
	sw.weqz $r56r57.lo? [$r56r57r58r59.x] = $r57
	;;
	sw -64[$r56r57.hi] = $r56r57r58r59.y
	;;
	sw -8589934592[$r58] = $r58r59.lo
	;;
	sxbd $r56r57r58r59.z = $r59
	;;
	sxhd $r58r59.hi = $r56r57r58r59.t
	;;
	sxlbhq $r60 = $r60r61.lo
	;;
	sxlhwp $r60r61r62r63.x = $r61
	;;
	sxmbhq $r60r61.hi = $r60r61r62r63.y
	;;
	sxmhwp $r62 = $r62r63.lo
	;;
	sxwd $r60r61r62r63.z = $r63
	;;
	syncgroup $r62r63.hi
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
	waitit $r60r61r62r63.t
	;;
	wfxl $ps, $r0
	;;
	wfxl $pcr, $r0r1.lo
	;;
	wfxl $s1, $r0r1r2r3.x
	;;
	wfxm $s1, $r1
	;;
	wfxm $s2, $r0r1.hi
	;;
	wfxm $pcr, $r0r1r2r3.y
	;;
	xcopyo $a13 = $a4a5a6a7.x
	;;
	xcopyo $a12a13.hi = $a4a5a6a7.y
	;;
	xlo.u.xs $a12a13a14a15.y = $r2[$r2r3.lo]
	;;
	xlo.us.wltz.q0 $r0r1r2r3.z? $a56a57a58a59 = -1125899906842624[$r3]
	;;
	xlo.u.wgez.q1 $r2r3.hi? $a60a61a62a63 = -8388608[$r0r1r2r3.t]
	;;
	xlo.us.wlez.q2 $r4? $a0a1a2a3 = [$r4r5.lo]
	;;
	xlo.u.wgtz $r4r5r6r7.x? $a14 = -1125899906842624[$r5]
	;;
	xlo.us.dnez $r4r5.hi? $a14a15.lo = -8388608[$r4r5r6r7.y]
	;;
	xlo.u.deqz $r6? $a12a13a14a15.z = [$r6r7.lo]
	;;
	xlo.us.q3 $a4a5a6a7 = $r4r5r6r7.z[$r7]
	;;
	xlo.u.q0 $a8a9a10a11 = 2305843009213693951[$r6r7.hi]
	;;
	xlo.us.q1 $a12a13a14a15 = -64[$r4r5r6r7.t]
	;;
	xlo.u.q2 $a16a17a18a19 = -8589934592[$r8]
	;;
	xlo.us $a15 = 2305843009213693951[$r8r9.lo]
	;;
	xlo.u $a14a15.hi = -64[$r8r9r10r11.x]
	;;
	xlo.us $a12a13a14a15.t = -8589934592[$r9]
	;;
	xmma484bw $a4a5a6a7.lo = $a6a7, $a16, $a16a17.lo
	;;
	xmma484subw $a4a5a6a7.hi = $a8a9, $a16a17a18a19.x, $a17
	;;
	xmma484ubw $a8a9a10a11.lo = $a10a11, $a16a17.hi, $a16a17a18a19.y
	;;
	xmma484usbw $a8a9a10a11.hi = $a12a13, $a18, $a18a19.lo
	;;
	xmovefo $r16r17r18r19 = $a6
	;;
	xmovefo $r20r21r22r23 = $a7
	;;
	xmovetq $a1_lo = $r8r9.hi, $r8r9r10r11.y
	;;
	xmovetq $a1_hi = $r10, $r10r11.lo
	;;
	xmt44d $a20a21a22a23 = $a24a25a26a27
	;;
	xord $r8r9r10r11.z = $r11, 2305843009213693951
	;;
	xord $r10r11.hi = $r8r9r10r11.t, $r12
	;;
	xord $sp = $r13, -64
	;;
	xord $tp = $r14, -8589934592
	;;
	xord.@ $fp = $r15, 536870911
	;;
	xorw $rp = $r16, $r16r17.lo
	;;
	xorw $r16r17r18r19.x = $r17, -64
	;;
	xorw $r16r17.hi = $r16r17r18r19.y, -8589934592
	;;
	xso.xs $r18[$r18r19.lo] = $a16a17a18a19.z
	;;
	xso 2305843009213693951[$r16r17r18r19.z] = $a19
	;;
	xso.dltz $r19? -1125899906842624[$r18r19.hi] = $a18a19.hi
	;;
	xso.dgez $r16r17r18r19.t? -8388608[$r20] = $a16a17a18a19.t
	;;
	xso.dlez $r20r21.lo? [$r20r21r22r23.x] = $a20
	;;
	xso -64[$r21] = $a20a21.lo
	;;
	xso -8589934592[$r20r21.hi] = $a20a21a22a23.x
	;;
	zxbd $r20r21r22r23.y = $r22
	;;
	zxhd $r22r23.lo = $r20r21r22r23.z
	;;
	zxwd $r23 = $r22r23.hi
	;;
	.endp	main
	.section .text
