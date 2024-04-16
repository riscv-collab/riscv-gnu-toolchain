#as: -march=kv3-2
#objdump: -d
.*\/kv3-2-insns-64.o:     file format elf64-kvx

Disassembly of section .text:

0000000000000000 <main>:
       0:	00 a0 02 72                                     	abdbo \$r0 = \$r0, \$r0;;

       4:	c1 a7 06 f2 ff ff ff 00                         	abdbo \$r1 = \$r1, 536870911 \(0x1fffffff\);;

       c:	c2 ff 04 e1 ff ff ff 87 ff ff ff 00             	abdd \$r1 = \$r2, 2305843009213693951 \(0x1fffffffffffffff\);;

      18:	c2 00 09 71                                     	abdd \$r2 = \$r2, \$r3;;

      1c:	03 f0 0c 61                                     	abdd \$r3 = \$r3, -64 \(0xffffffc0\);;

      20:	04 00 10 e1 00 00 80 07                         	abdd \$r4 = \$r4, -8589934592 \(0xfffffffe00000000\);;

      28:	c5 0f 11 f1 ff ff ff 00                         	abdd.@ \$r4 = \$r5, 536870911 \(0x1fffffff\);;

      30:	85 31 15 71                                     	abdhq \$r5 = \$r5, \$r6;;

      34:	c6 3f 19 f1 ff ff ff 00                         	abdhq.@ \$r6 = \$r6, 536870911 \(0x1fffffff\);;

      3c:	c7 a1 1e 73                                     	abdsbo \$r7 = \$r7, \$r7;;

      40:	c8 a7 22 f3 ff ff ff 00                         	abdsbo \$r8 = \$r8, 536870911 \(0x1fffffff\);;

      48:	49 02 21 70                                     	abdsd \$r8 = \$r9, \$r9;;

      4c:	ca 0f 25 f0 ff ff ff 00                         	abdsd.@ \$r9 = \$r10, 536870911 \(0x1fffffff\);;

      54:	ca 32 29 70                                     	abdshq \$r10 = \$r10, \$r11;;

      58:	cb 37 2d f0 ff ff ff 00                         	abdshq \$r11 = \$r11, 536870911 \(0x1fffffff\);;

      60:	4c 23 31 70                                     	abdswp \$r12 = \$r12, \$r13;;

      64:	ce 2f 35 f0 ff ff ff 00                         	abdswp.@ \$r13 = \$r14, 536870911 \(0x1fffffff\);;

      6c:	cf 13 39 70                                     	abdsw \$r14 = \$r15, \$r15;;

      70:	d0 17 41 f0 ff ff ff 00                         	abdsw \$r16 = \$r16, 536870911 \(0x1fffffff\);;

      78:	51 c4 42 7f                                     	abdubo \$r16 = \$r17, \$r17;;

      7c:	d2 c7 46 ff ff ff ff 00                         	abdubo \$r17 = \$r18, 536870911 \(0x1fffffff\);;

      84:	d2 a4 49 7f                                     	abdud \$r18 = \$r18, \$r19;;

      88:	d3 af 4d ff ff ff ff 00                         	abdud.@ \$r19 = \$r19, 536870911 \(0x1fffffff\);;

      90:	14 f5 51 7f                                     	abduhq \$r20 = \$r20, \$r20;;

      94:	d5 f7 55 ff ff ff ff 00                         	abduhq \$r21 = \$r21, 536870911 \(0x1fffffff\);;

      9c:	96 e5 55 7f                                     	abduwp \$r21 = \$r22, \$r22;;

      a0:	d7 ef 59 ff ff ff ff 00                         	abduwp.@ \$r22 = \$r23, 536870911 \(0x1fffffff\);;

      a8:	17 b6 5d 7f                                     	abduw \$r23 = \$r23, \$r24;;

      ac:	d8 b7 61 ff ff ff ff 00                         	abduw \$r24 = \$r24, 536870911 \(0x1fffffff\);;

      b4:	59 26 65 71                                     	abdwp \$r25 = \$r25, \$r25;;

      b8:	da 27 69 f1 ff ff ff 00                         	abdwp \$r26 = \$r26, 536870911 \(0x1fffffff\);;

      c0:	db 16 69 71                                     	abdw \$r26 = \$r27, \$r27;;

      c4:	1c f0 6c 71                                     	abdw \$r27 = \$r28, -64 \(0xffffffc0\);;

      c8:	1c 00 70 f1 00 00 80 07                         	abdw \$r28 = \$r28, -8589934592 \(0xfffffffe00000000\);;

      d0:	1d a0 76 f2 00 00 00 00                         	absbo \$r29 = \$r29;;

      d8:	1e 00 74 61                                     	absd \$r29 = \$r30;;

      dc:	1e 30 79 f1 00 00 00 00                         	abshq \$r30 = \$r30;;

      e4:	1f a0 7e f3 00 00 00 00                         	abssbo \$r31 = \$r31;;

      ec:	20 00 7d f0 00 00 00 00                         	abssd \$r31 = \$r32;;

      f4:	20 30 81 f0 00 00 00 00                         	absshq \$r32 = \$r32;;

      fc:	21 20 85 f0 00 00 00 00                         	absswp \$r33 = \$r33;;

     104:	22 10 85 f0 00 00 00 00                         	abssw \$r33 = \$r34;;

     10c:	22 20 89 f1 00 00 00 00                         	abswp \$r34 = \$r34;;

     114:	23 00 8c 71                                     	absw \$r35 = \$r35;;

     118:	24 a0 8f bc 00 00 00 98 00 00 80 1f             	acswapd.v \$r35, -1125899906842624 \(0xfffc000000000000\)\[\$r36\] = \$r0r1;;

     124:	24 b0 93 bd 00 00 80 1f                         	acswapd.g \$r36, -8388608 \(0xff800000\)\[\$r36\] = \$r0r1;;

     12c:	a5 a0 97 3e                                     	acswapd.v.s \$r37, \[\$r37\] = \$r2r3;;

     130:	25 d0 0b bc 00 00 00 98 00 00 80 1f             	acswapq \$r2r3, -1125899906842624 \(0xfffc000000000000\)\[\$r37\] = \$r0r1r2r3;;

     13c:	26 c1 13 bd 00 00 80 1f                         	acswapq.v.g \$r4r5, -8388608 \(0xff800000\)\[\$r38\] = \$r4r5r6r7;;

     144:	26 d2 13 3e                                     	acswapq.s \$r4r5, \[\$r38\] = \$r8r9r10r11;;

     148:	a7 81 9b bc 00 00 00 98 00 00 80 1f             	acswapw.v \$r38, -1125899906842624 \(0xfffc000000000000\)\[\$r39\] = \$r6r7;;

     154:	a7 91 9f bd 00 00 80 1f                         	acswapw.g \$r39, -8388608 \(0xff800000\)\[\$r39\] = \$r6r7;;

     15c:	28 82 a3 3e                                     	acswapw.v.s \$r40, \[\$r40\] = \$r8r9;;

     160:	69 aa a2 70                                     	addbo \$r40 = \$r41, \$r41;;

     164:	ea af a6 f0 ff ff ff 00                         	addbo.@ \$r41 = \$r42, 536870911 \(0x1fffffff\);;

     16c:	ea 9a a9 7e                                     	addcd.i \$r42 = \$r42, \$r43;;

     170:	eb 97 ad fe ff ff ff 00                         	addcd.i \$r43 = \$r43, 536870911 \(0x1fffffff\);;

     178:	2c 8b b1 7e                                     	addcd \$r44 = \$r44, \$r44;;

     17c:	ed 87 b5 fe ff ff ff 00                         	addcd \$r45 = \$r45, 536870911 \(0x1fffffff\);;

     184:	ee ff b4 e2 ff ff ff 87 ff ff ff 00             	addd \$r45 = \$r46, 2305843009213693951 \(0x1fffffffffffffff\);;

     190:	ee 0b b9 72                                     	addd \$r46 = \$r46, \$r47;;

     194:	2f f0 bc 62                                     	addd \$r47 = \$r47, -64 \(0xffffffc0\);;

     198:	30 00 c0 e2 00 00 80 07                         	addd \$r48 = \$r48, -8589934592 \(0xfffffffe00000000\);;

     1a0:	f1 0f c1 f2 ff ff ff 00                         	addd.@ \$r48 = \$r49, 536870911 \(0x1fffffff\);;

     1a8:	b1 3c c5 72                                     	addhq \$r49 = \$r49, \$r50;;

     1ac:	f2 37 c9 f2 ff ff ff 00                         	addhq \$r50 = \$r50, 536870911 \(0x1fffffff\);;

     1b4:	b3 c0 cf 76                                     	addrbod \$r51 = \$r51;;

     1b8:	74 c0 cf 76                                     	addrhqd \$r51 = \$r52;;

     1bc:	34 c0 d3 76                                     	addrwpd \$r52 = \$r52;;

     1c0:	75 bd d6 7c                                     	addsbo \$r53 = \$r53, \$r53;;

     1c4:	f6 bf da fc ff ff ff 00                         	addsbo.@ \$r54 = \$r54, 536870911 \(0x1fffffff\);;

     1cc:	f7 4d d9 7c                                     	addsd \$r54 = \$r55, \$r55;;

     1d0:	f8 47 dd fc ff ff ff 00                         	addsd \$r55 = \$r56, 536870911 \(0x1fffffff\);;

     1d8:	78 7e e1 7c                                     	addshq \$r56 = \$r56, \$r57;;

     1dc:	f9 7f e5 fc ff ff ff 00                         	addshq.@ \$r57 = \$r57, 536870911 \(0x1fffffff\);;

     1e4:	ba 6e e9 7c                                     	addswp \$r58 = \$r58, \$r58;;

     1e8:	fb 67 ed fc ff ff ff 00                         	addswp \$r59 = \$r59, 536870911 \(0x1fffffff\);;

     1f0:	3c 5f ed 7c                                     	addsw \$r59 = \$r60, \$r60;;

     1f4:	fd 57 f1 fc ff ff ff 00                         	addsw \$r60 = \$r61, 536870911 \(0x1fffffff\);;

     1fc:	bd c0 f7 7a                                     	addurbod \$r61 = \$r61;;

     200:	7e c0 fb 7a                                     	addurhqd \$r62 = \$r62;;

     204:	3f c0 fb 7a                                     	addurwpd \$r62 = \$r63;;

     208:	3f b0 fe 7e                                     	addusbo \$r63 = \$r63, \$r0;;

     20c:	c0 bf 02 fe ff ff ff 00                         	addusbo.@ \$r0 = \$r0, 536870911 \(0x1fffffff\);;

     214:	41 40 05 7e                                     	addusd \$r1 = \$r1, \$r1;;

     218:	c2 47 09 fe ff ff ff 00                         	addusd \$r2 = \$r2, 536870911 \(0x1fffffff\);;

     220:	c3 70 09 7e                                     	addushq \$r2 = \$r3, \$r3;;

     224:	c4 7f 0d fe ff ff ff 00                         	addushq.@ \$r3 = \$r4, 536870911 \(0x1fffffff\);;

     22c:	44 61 11 7e                                     	adduswp \$r4 = \$r4, \$r5;;

     230:	c5 67 15 fe ff ff ff 00                         	adduswp \$r5 = \$r5, 536870911 \(0x1fffffff\);;

     238:	86 51 19 7e                                     	addusw \$r6 = \$r6, \$r6;;

     23c:	c7 57 1d fe ff ff ff 00                         	addusw \$r7 = \$r7, 536870911 \(0x1fffffff\);;

     244:	08 92 1d 7c                                     	adduwd \$r7 = \$r8, \$r8;;

     248:	c9 97 21 fc ff ff ff 00                         	adduwd \$r8 = \$r9, 536870911 \(0x1fffffff\);;

     250:	89 82 25 7c                                     	addwd \$r9 = \$r9, \$r10;;

     254:	ca 87 29 fc ff ff ff 00                         	addwd \$r10 = \$r10, 536870911 \(0x1fffffff\);;

     25c:	cb 22 2d 72                                     	addwp \$r11 = \$r11, \$r11;;

     260:	cc 2f 31 f2 ff ff ff 00                         	addwp.@ \$r12 = \$r12, 536870911 \(0x1fffffff\);;

     268:	8d 13 35 72                                     	addw \$r13 = \$r13, \$r14;;

     26c:	0f f0 38 72                                     	addw \$r14 = \$r15, -64 \(0xffffffc0\);;

     270:	10 00 3c f2 00 00 80 07                         	addw \$r15 = \$r16, -8589934592 \(0xfffffffe00000000\);;

     278:	50 b4 42 76                                     	addx16bo \$r16 = \$r16, \$r17;;

     27c:	d1 b7 46 f6 ff ff ff 00                         	addx16bo \$r17 = \$r17, 536870911 \(0x1fffffff\);;

     284:	92 44 49 76                                     	addx16d \$r18 = \$r18, \$r18;;

     288:	d3 4f 4d f6 ff ff ff 00                         	addx16d.@ \$r19 = \$r19, 536870911 \(0x1fffffff\);;

     290:	14 75 4d 76                                     	addx16hq \$r19 = \$r20, \$r20;;

     294:	d5 77 51 f6 ff ff ff 00                         	addx16hq \$r20 = \$r21, 536870911 \(0x1fffffff\);;

     29c:	95 95 55 76                                     	addx16uwd \$r21 = \$r21, \$r22;;

     2a0:	d6 97 59 f6 ff ff ff 00                         	addx16uwd \$r22 = \$r22, 536870911 \(0x1fffffff\);;

     2a8:	d7 85 5d 76                                     	addx16wd \$r23 = \$r23, \$r23;;

     2ac:	d8 87 61 f6 ff ff ff 00                         	addx16wd \$r24 = \$r24, 536870911 \(0x1fffffff\);;

     2b4:	59 66 61 76                                     	addx16wp \$r24 = \$r25, \$r25;;

     2b8:	da 6f 65 f6 ff ff ff 00                         	addx16wp.@ \$r25 = \$r26, 536870911 \(0x1fffffff\);;

     2c0:	da 56 69 76                                     	addx16w \$r26 = \$r26, \$r27;;

     2c4:	db 57 6d f6 ff ff ff 00                         	addx16w \$r27 = \$r27, 536870911 \(0x1fffffff\);;

     2cc:	1c b7 72 70                                     	addx2bo \$r28 = \$r28, \$r28;;

     2d0:	dd b7 76 f0 ff ff ff 00                         	addx2bo \$r29 = \$r29, 536870911 \(0x1fffffff\);;

     2d8:	9e 47 75 70                                     	addx2d \$r29 = \$r30, \$r30;;

     2dc:	df 4f 79 f0 ff ff ff 00                         	addx2d.@ \$r30 = \$r31, 536870911 \(0x1fffffff\);;

     2e4:	1f 78 7d 70                                     	addx2hq \$r31 = \$r31, \$r32;;

     2e8:	e0 77 81 f0 ff ff ff 00                         	addx2hq \$r32 = \$r32, 536870911 \(0x1fffffff\);;

     2f0:	61 98 85 70                                     	addx2uwd \$r33 = \$r33, \$r33;;

     2f4:	e2 97 89 f0 ff ff ff 00                         	addx2uwd \$r34 = \$r34, 536870911 \(0x1fffffff\);;

     2fc:	e3 88 89 70                                     	addx2wd \$r34 = \$r35, \$r35;;

     300:	e4 87 8d f0 ff ff ff 00                         	addx2wd \$r35 = \$r36, 536870911 \(0x1fffffff\);;

     308:	64 69 91 70                                     	addx2wp \$r36 = \$r36, \$r37;;

     30c:	e5 6f 95 f0 ff ff ff 00                         	addx2wp.@ \$r37 = \$r37, 536870911 \(0x1fffffff\);;

     314:	a6 59 99 70                                     	addx2w \$r38 = \$r38, \$r38;;

     318:	e7 57 9d f0 ff ff ff 00                         	addx2w \$r39 = \$r39, 536870911 \(0x1fffffff\);;

     320:	28 4a 9d 78                                     	addx32d \$r39 = \$r40, \$r40;;

     324:	e9 47 a1 f8 ff ff ff 00                         	addx32d \$r40 = \$r41, 536870911 \(0x1fffffff\);;

     32c:	a9 9a a5 78                                     	addx32uwd \$r41 = \$r41, \$r42;;

     330:	ea 97 a9 f8 ff ff ff 00                         	addx32uwd \$r42 = \$r42, 536870911 \(0x1fffffff\);;

     338:	eb 8a ad 78                                     	addx32wd \$r43 = \$r43, \$r43;;

     33c:	ec 87 b1 f8 ff ff ff 00                         	addx32wd \$r44 = \$r44, 536870911 \(0x1fffffff\);;

     344:	6d 5b b1 78                                     	addx32w \$r44 = \$r45, \$r45;;

     348:	ee 57 b5 f8 ff ff ff 00                         	addx32w \$r45 = \$r46, 536870911 \(0x1fffffff\);;

     350:	ee bb ba 72                                     	addx4bo \$r46 = \$r46, \$r47;;

     354:	ef bf be f2 ff ff ff 00                         	addx4bo.@ \$r47 = \$r47, 536870911 \(0x1fffffff\);;

     35c:	30 4c c1 72                                     	addx4d \$r48 = \$r48, \$r48;;

     360:	f1 47 c5 f2 ff ff ff 00                         	addx4d \$r49 = \$r49, 536870911 \(0x1fffffff\);;

     368:	b2 7c c5 72                                     	addx4hq \$r49 = \$r50, \$r50;;

     36c:	f3 7f c9 f2 ff ff ff 00                         	addx4hq.@ \$r50 = \$r51, 536870911 \(0x1fffffff\);;

     374:	33 9d cd 72                                     	addx4uwd \$r51 = \$r51, \$r52;;

     378:	f4 97 d1 f2 ff ff ff 00                         	addx4uwd \$r52 = \$r52, 536870911 \(0x1fffffff\);;

     380:	75 8d d5 72                                     	addx4wd \$r53 = \$r53, \$r53;;

     384:	f6 87 d9 f2 ff ff ff 00                         	addx4wd \$r54 = \$r54, 536870911 \(0x1fffffff\);;

     38c:	f7 6d d9 72                                     	addx4wp \$r54 = \$r55, \$r55;;

     390:	f8 67 dd f2 ff ff ff 00                         	addx4wp \$r55 = \$r56, 536870911 \(0x1fffffff\);;

     398:	78 5e e1 72                                     	addx4w \$r56 = \$r56, \$r57;;

     39c:	f9 57 e5 f2 ff ff ff 00                         	addx4w \$r57 = \$r57, 536870911 \(0x1fffffff\);;

     3a4:	ba 4e e9 7a                                     	addx64d \$r58 = \$r58, \$r58;;

     3a8:	fb 4f ed fa ff ff ff 00                         	addx64d.@ \$r59 = \$r59, 536870911 \(0x1fffffff\);;

     3b0:	3c 9f ed 7a                                     	addx64uwd \$r59 = \$r60, \$r60;;

     3b4:	fd 97 f1 fa ff ff ff 00                         	addx64uwd \$r60 = \$r61, 536870911 \(0x1fffffff\);;

     3bc:	bd 8f f5 7a                                     	addx64wd \$r61 = \$r61, \$r62;;

     3c0:	fe 87 f9 fa ff ff ff 00                         	addx64wd \$r62 = \$r62, 536870911 \(0x1fffffff\);;

     3c8:	ff 5f fd 7a                                     	addx64w \$r63 = \$r63, \$r63;;

     3cc:	c0 57 01 fa ff ff ff 00                         	addx64w \$r0 = \$r0, 536870911 \(0x1fffffff\);;

     3d4:	41 b0 02 74                                     	addx8bo \$r0 = \$r1, \$r1;;

     3d8:	c2 b7 06 f4 ff ff ff 00                         	addx8bo \$r1 = \$r2, 536870911 \(0x1fffffff\);;

     3e0:	c2 40 09 74                                     	addx8d \$r2 = \$r2, \$r3;;

     3e4:	c3 4f 0d f4 ff ff ff 00                         	addx8d.@ \$r3 = \$r3, 536870911 \(0x1fffffff\);;

     3ec:	04 71 11 74                                     	addx8hq \$r4 = \$r4, \$r4;;

     3f0:	c5 77 15 f4 ff ff ff 00                         	addx8hq \$r5 = \$r5, 536870911 \(0x1fffffff\);;

     3f8:	86 91 15 74                                     	addx8uwd \$r5 = \$r6, \$r6;;

     3fc:	c7 97 19 f4 ff ff ff 00                         	addx8uwd \$r6 = \$r7, 536870911 \(0x1fffffff\);;

     404:	07 82 1d 74                                     	addx8wd \$r7 = \$r7, \$r8;;

     408:	c8 87 21 f4 ff ff ff 00                         	addx8wd \$r8 = \$r8, 536870911 \(0x1fffffff\);;

     410:	49 62 25 74                                     	addx8wp \$r9 = \$r9, \$r9;;

     414:	ca 6f 29 f4 ff ff ff 00                         	addx8wp.@ \$r10 = \$r10, 536870911 \(0x1fffffff\);;

     41c:	cb 52 29 74                                     	addx8w \$r10 = \$r11, \$r11;;

     420:	cc 57 2d f4 ff ff ff 00                         	addx8w \$r11 = \$r12, 536870911 \(0x1fffffff\);;

     428:	0c 70 37 bc 00 00 00 98 00 00 80 1f             	aladdd -1125899906842624 \(0xfffc000000000000\)\[\$r12\] = \$r13;;

     434:	0d 70 3b bd 00 00 80 1f                         	aladdd.g -8388608 \(0xff800000\)\[\$r13\] = \$r14;;

     43c:	0e 70 3f 3e                                     	aladdd.s \[\$r14\] = \$r15;;

     440:	0f 60 43 bc 00 00 00 98 00 00 80 1f             	aladdw -1125899906842624 \(0xfffc000000000000\)\[\$r15\] = \$r16;;

     44c:	10 60 43 bd 00 00 80 1f                         	aladdw.g -8388608 \(0xff800000\)\[\$r16\] = \$r16;;

     454:	11 60 47 3e                                     	aladdw.s \[\$r17\] = \$r17;;

     458:	12 30 47 bc 00 00 00 98 00 00 80 1f             	alclrd \$r17 = -1125899906842624 \(0xfffc000000000000\)\[\$r18\];;

     464:	12 30 4b bd 00 00 80 1f                         	alclrd.g \$r18 = -8388608 \(0xff800000\)\[\$r18\];;

     46c:	13 30 4f 3e                                     	alclrd.s \$r19 = \[\$r19\];;

     470:	14 20 4f bc 00 00 00 98 00 00 80 1f             	alclrw \$r19 = -1125899906842624 \(0xfffc000000000000\)\[\$r20\];;

     47c:	14 20 53 bd 00 00 80 1f                         	alclrw.g \$r20 = -8388608 \(0xff800000\)\[\$r20\];;

     484:	15 20 57 3e                                     	alclrw.s \$r21 = \[\$r21\];;

     488:	16 10 57 bc 00 00 00 98 00 00 80 1f             	ald \$r21 = -1125899906842624 \(0xfffc000000000000\)\[\$r22\];;

     494:	16 10 5b bd 00 00 80 1f                         	ald.g \$r22 = -8388608 \(0xff800000\)\[\$r22\];;

     49c:	17 10 5f 3e                                     	ald.s \$r23 = \[\$r23\];;

     4a0:	18 00 5f bc 00 00 00 98 00 00 80 1f             	alw \$r23 = -1125899906842624 \(0xfffc000000000000\)\[\$r24\];;

     4ac:	18 00 63 bd 00 00 80 1f                         	alw.g \$r24 = -8388608 \(0xff800000\)\[\$r24\];;

     4b4:	19 00 67 3e                                     	alw.s \$r25 = \[\$r25\];;

     4b8:	da ff 64 e8 ff ff ff 87 ff ff ff 00             	andd \$r25 = \$r26, 2305843009213693951 \(0x1fffffffffffffff\);;

     4c4:	da 06 69 78                                     	andd \$r26 = \$r26, \$r27;;

     4c8:	1b f0 6c 68                                     	andd \$r27 = \$r27, -64 \(0xffffffc0\);;

     4cc:	1c 00 70 e8 00 00 80 07                         	andd \$r28 = \$r28, -8589934592 \(0xfffffffe00000000\);;

     4d4:	dd 0f 71 f8 ff ff ff 00                         	andd.@ \$r28 = \$r29, 536870911 \(0x1fffffff\);;

     4dc:	dd ff 74 ee ff ff ff 87 ff ff ff 00             	andnd \$r29 = \$r29, 2305843009213693951 \(0x1fffffffffffffff\);;

     4e8:	9e 07 79 7e                                     	andnd \$r30 = \$r30, \$r30;;

     4ec:	1f f0 7c 6e                                     	andnd \$r31 = \$r31, -64 \(0xffffffc0\);;

     4f0:	20 00 7c ee 00 00 80 07                         	andnd \$r31 = \$r32, -8589934592 \(0xfffffffe00000000\);;

     4f8:	e0 0f 81 fe ff ff ff 00                         	andnd.@ \$r32 = \$r32, 536870911 \(0x1fffffff\);;

     500:	61 18 85 7e                                     	andnw \$r33 = \$r33, \$r33;;

     504:	22 f0 88 7e                                     	andnw \$r34 = \$r34, -64 \(0xffffffc0\);;

     508:	23 00 88 fe 00 00 80 07                         	andnw \$r34 = \$r35, -8589934592 \(0xfffffffe00000000\);;

     510:	a3 c0 8f 70                                     	andrbod \$r35 = \$r35;;

     514:	64 c0 93 70                                     	andrhqd \$r36 = \$r36;;

     518:	25 c0 93 70                                     	andrwpd \$r36 = \$r37;;

     51c:	a5 19 95 78                                     	andw \$r37 = \$r37, \$r38;;

     520:	26 f0 98 78                                     	andw \$r38 = \$r38, -64 \(0xffffffc0\);;

     524:	27 00 9c f8 00 00 80 07                         	andw \$r39 = \$r39, -8589934592 \(0xfffffffe00000000\);;

     52c:	27 50 a3 bc 00 00 00 98 00 00 80 1f             	asd -1125899906842624 \(0xfffc000000000000\)\[\$r39\] = \$r40;;

     538:	28 50 a3 bd 00 00 80 1f                         	asd.g -8388608 \(0xff800000\)\[\$r40\] = \$r40;;

     540:	29 50 a7 3e                                     	asd.s \[\$r41\] = \$r41;;

     544:	29 40 ab bc 00 00 00 98 00 00 80 1f             	asw -1125899906842624 \(0xfffc000000000000\)\[\$r41\] = \$r42;;

     550:	2a 40 ab bd 00 00 80 1f                         	asw.g -8388608 \(0xff800000\)\[\$r42\] = \$r42;;

     558:	2b 40 af 3e                                     	asw.s \[\$r43\] = \$r43;;

     55c:	2c fb ae 74                                     	avgbo \$r43 = \$r44, \$r44;;

     560:	ed f7 b2 f4 ff ff ff 00                         	avgbo \$r44 = \$r45, 536870911 \(0x1fffffff\);;

     568:	ad 9b b6 74                                     	avghq \$r45 = \$r45, \$r46;;

     56c:	ee 9f ba f4 ff ff ff 00                         	avghq.@ \$r46 = \$r46, 536870911 \(0x1fffffff\);;

     574:	ef fb be 75                                     	avgrbo \$r47 = \$r47, \$r47;;

     578:	f0 f7 c2 f5 ff ff ff 00                         	avgrbo \$r48 = \$r48, 536870911 \(0x1fffffff\);;

     580:	71 9c c2 75                                     	avgrhq \$r48 = \$r49, \$r49;;

     584:	f2 9f c6 f5 ff ff ff 00                         	avgrhq.@ \$r49 = \$r50, 536870911 \(0x1fffffff\);;

     58c:	f2 fc ca 77                                     	avgrubo \$r50 = \$r50, \$r51;;

     590:	f3 f7 ce f7 ff ff ff 00                         	avgrubo \$r51 = \$r51, 536870911 \(0x1fffffff\);;

     598:	34 9d d2 77                                     	avgruhq \$r52 = \$r52, \$r52;;

     59c:	f5 9f d6 f7 ff ff ff 00                         	avgruhq.@ \$r53 = \$r53, 536870911 \(0x1fffffff\);;

     5a4:	b6 8d d6 77                                     	avgruwp \$r53 = \$r54, \$r54;;

     5a8:	f7 87 da f7 ff ff ff 00                         	avgruwp \$r54 = \$r55, 536870911 \(0x1fffffff\);;

     5b0:	37 7e de 77                                     	avgruw \$r55 = \$r55, \$r56;;

     5b4:	f8 77 e2 f7 ff ff ff 00                         	avgruw \$r56 = \$r56, 536870911 \(0x1fffffff\);;

     5bc:	79 8e e6 75                                     	avgrwp \$r57 = \$r57, \$r57;;

     5c0:	fa 8f ea f5 ff ff ff 00                         	avgrwp.@ \$r58 = \$r58, 536870911 \(0x1fffffff\);;

     5c8:	fb 7e ea 75                                     	avgrw \$r58 = \$r59, \$r59;;

     5cc:	fc 77 ee f5 ff ff ff 00                         	avgrw \$r59 = \$r60, 536870911 \(0x1fffffff\);;

     5d4:	7c ff f2 76                                     	avgubo \$r60 = \$r60, \$r61;;

     5d8:	fd f7 f6 f6 ff ff ff 00                         	avgubo \$r61 = \$r61, 536870911 \(0x1fffffff\);;

     5e0:	be 9f fa 76                                     	avguhq \$r62 = \$r62, \$r62;;

     5e4:	ff 9f fe f6 ff ff ff 00                         	avguhq.@ \$r63 = \$r63, 536870911 \(0x1fffffff\);;

     5ec:	00 80 fe 76                                     	avguwp \$r63 = \$r0, \$r0;;

     5f0:	c1 87 02 f6 ff ff ff 00                         	avguwp \$r0 = \$r1, 536870911 \(0x1fffffff\);;

     5f8:	81 70 06 76                                     	avguw \$r1 = \$r1, \$r2;;

     5fc:	c2 77 0a f6 ff ff ff 00                         	avguw \$r2 = \$r2, 536870911 \(0x1fffffff\);;

     604:	c3 80 0e 74                                     	avgwp \$r3 = \$r3, \$r3;;

     608:	c4 8f 12 f4 ff ff ff 00                         	avgwp.@ \$r4 = \$r4, 536870911 \(0x1fffffff\);;

     610:	45 71 12 74                                     	avgw \$r4 = \$r5, \$r5;;

     614:	c6 77 16 f4 ff ff ff 00                         	avgw \$r5 = \$r6, 536870911 \(0x1fffffff\);;

     61c:	00 00 a0 0f                                     	await;;

     620:	00 00 ac 0f                                     	barrier;;

     624:	00 80 00 00                                     	break 0 \(0x0\);;

     628:	00 00 80 1f                                     	call fffffffffe000628 <main\+0xfffffffffe000628>;;

     62c:	06 20 1a 72                                     	cbsd \$r6 = \$r6;;

     630:	07 40 1e 72                                     	cbswp \$r7 = \$r7;;

     634:	08 30 1e 72                                     	cbsw \$r7 = \$r8;;

     638:	08 00 78 08                                     	cb.dnez \$r8\? ffffffffffff8638 <main\+0xffffffffffff8638>;;

     63c:	c9 71 23 6c                                     	clrf \$r8 = \$r9, 7 \(0x7\), 7 \(0x7\);;

     640:	09 20 26 71                                     	clsd \$r9 = \$r9;;

     644:	0a 40 2a 71                                     	clswp \$r10 = \$r10;;

     648:	0b 30 2a 71                                     	clsw \$r10 = \$r11;;

     64c:	0b 20 2e 70                                     	clzd \$r11 = \$r11;;

     650:	0c 40 32 70                                     	clzwp \$r12 = \$r12;;

     654:	0d 30 36 70                                     	clzw \$r13 = \$r13;;

     658:	ce d3 3a 70                                     	cmovebo.nez \$r14\? \$r14 = \$r15;;

     65c:	cf ff 42 e1 ff ff ff 87 ff ff ff 00             	cmoved.deqz \$r15\? \$r16 = 2305843009213693951 \(0x1fffffffffffffff\);;

     668:	50 04 42 72                                     	cmoved.dltz \$r16\? \$r16 = \$r17;;

     66c:	11 f0 46 63                                     	cmoved.dgez \$r17\? \$r17 = -64 \(0xffffffc0\);;

     670:	12 00 4a e4 00 00 80 07                         	cmoved.dlez \$r18\? \$r18 = -8589934592 \(0xfffffffe00000000\);;

     678:	d2 14 4e 79                                     	cmovehq.eqz \$r18\? \$r19 = \$r19;;

     67c:	13 15 52 72                                     	cmovewp.ltz \$r19\? \$r20 = \$r20;;

     680:	54 15 24 5b                                     	cmuldt \$r8r9 = \$r20, \$r21;;

     684:	55 15 2c 5f                                     	cmulghxdt \$r10r11 = \$r21, \$r21;;

     688:	96 15 2c 5d                                     	cmulglxdt \$r10r11 = \$r22, \$r22;;

     68c:	d6 15 34 5e                                     	cmulgmxdt \$r12r13 = \$r22, \$r23;;

     690:	d7 15 34 5c                                     	cmulxdt \$r12r13 = \$r23, \$r23;;

     694:	d8 ff 61 e0 ff ff ff 87 ff ff ff 00             	compd.ne \$r24 = \$r24, 2305843009213693951 \(0x1fffffffffffffff\);;

     6a0:	59 a6 61 71                                     	compd.eq \$r24 = \$r25, \$r25;;

     6a4:	1a f0 65 62                                     	compd.lt \$r25 = \$r26, -64 \(0xffffffc0\);;

     6a8:	1a 00 69 e3 00 00 80 07                         	compd.ge \$r26 = \$r26, -8589934592 \(0xfffffffe00000000\);;

     6b0:	db c6 6e 74                                     	compnbo.le \$r27 = \$r27, \$r27;;

     6b4:	dc c7 72 f5 ff ff ff 00                         	compnbo.gt \$r28 = \$r28, 536870911 \(0x1fffffff\);;

     6bc:	5d b7 73 76                                     	compnd.ltu \$r28 = \$r29, \$r29;;

     6c0:	de b7 77 f7 ff ff ff 00                         	compnd.geu \$r29 = \$r30, 536870911 \(0x1fffffff\);;

     6c8:	de f7 79 78                                     	compnhq.leu \$r30 = \$r30, \$r31;;

     6cc:	df ff 7d f9 ff ff ff 00                         	compnhq.gtu.@ \$r31 = \$r31, 536870911 \(0x1fffffff\);;

     6d4:	20 e8 81 7a                                     	compnwp.all \$r32 = \$r32, \$r32;;

     6d8:	e1 e7 85 fb ff ff ff 00                         	compnwp.nall \$r33 = \$r33, 536870911 \(0x1fffffff\);;

     6e0:	a2 a8 87 7c                                     	compnw.any \$r33 = \$r34, \$r34;;

     6e4:	e3 a7 8b fd ff ff ff 00                         	compnw.none \$r34 = \$r35, 536870911 \(0x1fffffff\);;

     6ec:	23 d9 8d 70                                     	compuwd.ne \$r35 = \$r35, \$r36;;

     6f0:	e4 d7 91 f1 ff ff ff 00                         	compuwd.eq \$r36 = \$r36, 536870911 \(0x1fffffff\);;

     6f8:	65 c9 95 72                                     	compwd.lt \$r37 = \$r37, \$r37;;

     6fc:	e6 c7 99 f3 ff ff ff 00                         	compwd.ge \$r38 = \$r38, 536870911 \(0x1fffffff\);;

     704:	e7 b9 99 74                                     	compw.le \$r38 = \$r39, \$r39;;

     708:	e8 b7 9d f5 ff ff ff 00                         	compw.gt \$r39 = \$r40, 536870911 \(0x1fffffff\);;

     710:	28 00 a0 6a                                     	copyd \$r40 = \$r40;;

     714:	10 00 3d 34                                     	copyo \$r12r13r14r15 = \$r16r17r18r19;;

     718:	69 fa 38 5f                                     	copyq \$r14r15 = \$r41, \$r41;;

     71c:	2a 00 a4 7a                                     	copyw \$r41 = \$r42;;

     720:	ea 2a a8 59                                     	crcbellw \$r42 = \$r42, \$r43;;

     724:	eb 27 ac d9 ff ff ff 10                         	crcbellw \$r43 = \$r43, 536870911 \(0x1fffffff\);;

     72c:	2c 2b b0 58                                     	crcbelmw \$r44 = \$r44, \$r44;;

     730:	ed 27 b4 d8 ff ff ff 10                         	crcbelmw \$r45 = \$r45, 536870911 \(0x1fffffff\);;

     738:	ae 2b b4 5b                                     	crclellw \$r45 = \$r46, \$r46;;

     73c:	ef 27 b8 db ff ff ff 10                         	crclellw \$r46 = \$r47, 536870911 \(0x1fffffff\);;

     744:	2f 2c bc 5a                                     	crclelmw \$r47 = \$r47, \$r48;;

     748:	f0 27 c0 da ff ff ff 10                         	crclelmw \$r48 = \$r48, 536870911 \(0x1fffffff\);;

     750:	31 20 c6 73                                     	ctzd \$r49 = \$r49;;

     754:	32 40 c6 73                                     	ctzwp \$r49 = \$r50;;

     758:	32 30 ca 73                                     	ctzw \$r50 = \$r50;;

     75c:	00 00 8c 3c                                     	d1inval;;

     760:	f3 ec 3e 3c                                     	dflushl \$r51\[\$r51\];;

     764:	f3 ff 3c bc ff ff ff 9f ff ff ff 18             	dflushl 2305843009213693951 \(0x1fffffffffffffff\)\[\$r51\];;

     770:	34 f0 3c 3c                                     	dflushl -64 \(0xffffffc0\)\[\$r52\];;

     774:	34 00 3c bc 00 00 80 1f                         	dflushl -8589934592 \(0xfffffffe00000000\)\[\$r52\];;

     77c:	35 ed be 3c                                     	dflushsw.l1 \$r52, \$r53;;

     780:	75 fd 1e 3c                                     	dinvall.xs \$r53\[\$r53\];;

     784:	f6 ff 1c bc ff ff ff 9f ff ff ff 18             	dinvall 2305843009213693951 \(0x1fffffffffffffff\)\[\$r54\];;

     790:	36 f0 1c 3c                                     	dinvall -64 \(0xffffffc0\)\[\$r54\];;

     794:	36 00 1c bc 00 00 80 1f                         	dinvall -8589934592 \(0xfffffffe00000000\)\[\$r54\];;

     79c:	f7 ed 9e 3d                                     	dinvalsw.l2 \$r55, \$r55;;

     7a0:	10 24 38 52                                     	dot2suwdp \$r14r15 = \$r16r17, \$r16r17;;

     7a4:	38 2e dc 5e                                     	dot2suwd \$r55 = \$r56, \$r56;;

     7a8:	12 25 48 51                                     	dot2uwdp \$r18r19 = \$r18r19, \$r20r21;;

     7ac:	79 2e e0 5d                                     	dot2uwd \$r56 = \$r57, \$r57;;

     7b0:	96 25 50 50                                     	dot2wdp \$r20r21 = \$r22r23, \$r22r23;;

     7b4:	ba 2e e4 5c                                     	dot2wd \$r57 = \$r58, \$r58;;

     7b8:	98 26 60 53                                     	dot2wzp \$r24r25 = \$r24r25, \$r26r27;;

     7bc:	fb 2e e8 5f                                     	dot2w \$r58 = \$r59, \$r59;;

     7c0:	fc ee 2e 3c                                     	dpurgel \$r59\[\$r60\];;

     7c4:	fc ff 2c bc ff ff ff 9f ff ff ff 18             	dpurgel 2305843009213693951 \(0x1fffffffffffffff\)\[\$r60\];;

     7d0:	3c f0 2c 3c                                     	dpurgel -64 \(0xffffffc0\)\[\$r60\];;

     7d4:	3d 00 2c bc 00 00 80 1f                         	dpurgel -8589934592 \(0xfffffffe00000000\)\[\$r61\];;

     7dc:	7d ef ae 3c                                     	dpurgesw.l1 \$r61, \$r61;;

     7e0:	be ff 0e 3c                                     	dtouchl.xs \$r62\[\$r62\];;

     7e4:	fe ff 0c bc ff ff ff 9f ff ff ff 18             	dtouchl 2305843009213693951 \(0x1fffffffffffffff\)\[\$r62\];;

     7f0:	3f f0 0c 3c                                     	dtouchl -64 \(0xffffffc0\)\[\$r63\];;

     7f4:	3f 00 0c bc 00 00 80 1f                         	dtouchl -8589934592 \(0xfffffffe00000000\)\[\$r63\];;

     7fc:	00 00 00 00                                     	errop;;

     800:	c0 71 ff 68                                     	extfs \$r63 = \$r0, 7 \(0x7\), 7 \(0x7\);;

     804:	c0 71 03 64                                     	extfz \$r0 = \$r0, 7 \(0x7\), 7 \(0x7\);;

     808:	01 20 07 71                                     	fabsd \$r1 = \$r1;;

     80c:	02 20 07 77                                     	fabshq \$r1 = \$r2;;

     810:	02 20 0b 75                                     	fabswp \$r2 = \$r2;;

     814:	03 20 0f 73                                     	fabsw \$r3 = \$r3;;

     818:	1c 07 6b 5d                                     	fadddc.c.rn \$r26r27 = \$r28r29, \$r28r29;;

     81c:	1e 98 7b 5c                                     	fadddp.ru.s \$r30r31 = \$r30r31, \$r32r33;;

     820:	a2 28 83 5c                                     	fadddp.rd \$r32r33 = \$r34r35, \$r34r35;;

     824:	04 b1 0e 50                                     	faddd.rz.s \$r3 = \$r4, \$r4;;

     828:	a4 49 97 56                                     	faddho.rna \$r36r37 = \$r36r37, \$r38r39;;

     82c:	45 d1 12 52                                     	faddhq.rnz.s \$r4 = \$r5, \$r5;;

     830:	86 61 16 53                                     	faddwc.c.ro \$r5 = \$r6, \$r6;;

     834:	28 fa 9f 59                                     	faddwcp.c.s \$r38r39 = \$r40r41, \$r40r41;;

     838:	2a 0b af 58                                     	faddwq.rn \$r42r43 = \$r42r43, \$r44r45;;

     83c:	c7 91 1a 51                                     	faddwp.ru.s \$r6 = \$r7, \$r7;;

     840:	08 22 1e 51                                     	faddwp.rd \$r7 = \$r8, \$r8;;

     844:	ae bb b7 58                                     	faddwq.rz.s \$r44r45 = \$r46r47, \$r46r47;;

     848:	49 42 22 5c                                     	faddw.rna \$r8 = \$r9, \$r9;;

     84c:	30 58 27 71                                     	fcdivd.s \$r9 = \$r48r49;;

     850:	30 50 2b 75                                     	fcdivwp \$r10 = \$r48r49;;

     854:	32 58 2b 73                                     	fcdivw.s \$r10 = \$r50r51;;

     858:	cb 02 2b 78                                     	fcompd.one \$r10 = \$r11, \$r11;;

     85c:	cc 07 2f f9 ff ff ff 00                         	fcompd.ueq \$r11 = \$r12, 536870911 \(0x1fffffff\);;

     864:	4d 93 33 7a                                     	fcompnd.oeq \$r12 = \$r13, \$r13;;

     868:	ce 97 3b fb ff ff ff 00                         	fcompnd.une \$r14 = \$r14, 536870911 \(0x1fffffff\);;

     870:	0f 14 3f 7c                                     	fcompnhq.olt \$r15 = \$r15, \$r16;;

     874:	d0 1f 43 fd ff ff ff 00                         	fcompnhq.uge.@ \$r16 = \$r16, 536870911 \(0x1fffffff\);;

     87c:	51 14 47 76                                     	fcompnwp.oge \$r17 = \$r17, \$r17;;

     880:	d2 17 4b f7 ff ff ff 00                         	fcompnwp.ult \$r18 = \$r18, 536870911 \(0x1fffffff\);;

     888:	d3 94 4b 70                                     	fcompnw.one \$r18 = \$r19, \$r19;;

     88c:	d4 97 4f f1 ff ff ff 00                         	fcompnw.ueq \$r19 = \$r20, 536870911 \(0x1fffffff\);;

     894:	54 05 53 72                                     	fcompw.oeq \$r20 = \$r20, \$r21;;

     898:	d5 07 57 f3 ff ff ff 00                         	fcompw.une \$r21 = \$r21, 536870911 \(0x1fffffff\);;

     8a0:	34 5d cf 5c                                     	fdot2wdp.rnz \$r50r51 = \$r52r53, \$r52r53;;

     8a4:	96 e5 59 5d                                     	fdot2wd.ro.s \$r22 = \$r22, \$r22;;

     8a8:	36 7e df 5d                                     	fdot2wzp \$r54r55 = \$r54r55, \$r56r57;;

     8ac:	d7 85 5d 5c                                     	fdot2w.rn.s \$r23 = \$r23, \$r23;;

     8b0:	00 00 fc 3c                                     	fence;;

     8b4:	b8 1e 62 47                                     	ffdmaswp.ru \$r24 = \$r56r57, \$r58r59;;

     8b8:	14 a6 ea 4f                                     	ffdmaswq.rd.s \$r58r59 = \$r20r21r22r23, \$r24r25r26r27;;

     8bc:	58 36 62 43                                     	ffdmasw.rz \$r24 = \$r24, \$r25;;

     8c0:	3c cf 65 42                                     	ffdmawp.rna.s \$r25 = \$r60r61, \$r60r61;;

     8c4:	1c 58 f9 46                                     	ffdmawq.rnz \$r62r63 = \$r28r29r30r31, \$r32r33r34r35;;

     8c8:	9a e6 65 40                                     	ffdmaw.ro.s \$r25 = \$r26, \$r26;;

     8cc:	3e 70 6a 44                                     	ffdmdawp \$r26 = \$r62r63, \$r0r1;;

     8d0:	24 8a 02 4c                                     	ffdmdawq.rn.s \$r0r1 = \$r36r37r38r39, \$r40r41r42r43;;

     8d4:	db 16 6e 40                                     	ffdmdaw.ru \$r27 = \$r27, \$r27;;

     8d8:	82 a0 72 46                                     	ffdmdswp.rd.s \$r28 = \$r2r3, \$r2r3;;

     8dc:	2c 3c 12 4e                                     	ffdmdswq.rz \$r4r5 = \$r44r45r46r47, \$r48r49r50r51;;

     8e0:	5c c7 72 42                                     	ffdmdsw.rna.s \$r28 = \$r28, \$r29;;

     8e4:	84 51 76 45                                     	ffdmsawp.rnz \$r29 = \$r4r5, \$r6r7;;

     8e8:	34 ee 1a 4d                                     	ffdmsawq.ro.s \$r6r7 = \$r52r53r54r55, \$r56r57r58r59;;

     8ec:	9e 77 76 41                                     	ffdmsaw \$r29 = \$r30, \$r30;;

     8f0:	08 82 79 43                                     	ffdmswp.rn.s \$r30 = \$r8r9, \$r8r9;;

     8f4:	3c 10 29 47                                     	ffdmswq.ru \$r10r11 = \$r60r61r62r63, \$r0r1r2r3;;

     8f8:	df a7 7d 41                                     	ffdmsw.rd.s \$r31 = \$r31, \$r31;;

     8fc:	20 38 80 44                                     	ffmad.rz \$r32 = \$r32, \$r32;;

     900:	0c c3 2b 5a                                     	ffmaho.rna.s \$r10r11 = \$r12r13, \$r12r13;;

     904:	61 58 85 53                                     	ffmahq.rnz \$r33 = \$r33, \$r33;;

     908:	a2 e8 3b 51                                     	ffmahwq.ro.s \$r14r15 = \$r34, \$r34;;

     90c:	e3 78 8a 58                                     	ffmahw \$r34 = \$r35, \$r35;;

     910:	10 84 39 4c                                     	ffmawcp.rn.s \$r14r15 = \$r16r17, \$r16r17;;

     914:	24 19 8d 49                                     	ffmawc.c.ru \$r35 = \$r36, \$r36;;

     918:	64 a9 4b 50                                     	ffmawdp.rd.s \$r18r19 = \$r36, \$r37;;

     91c:	a5 39 95 51                                     	ffmawd.rz \$r37 = \$r37, \$r38;;

     920:	e6 c9 98 42                                     	ffmawp.rna.s \$r38 = \$r38, \$r39;;

     924:	14 55 48 46                                     	ffmawq.rnz \$r18r19 = \$r20r21, \$r20r21;;

     928:	27 ea 9c 40                                     	ffmaw.ro.s \$r39 = \$r39, \$r40;;

     92c:	68 7a a0 45                                     	ffmsd \$r40 = \$r40, \$r41;;

     930:	16 86 5b 5b                                     	ffmsho.rn.s \$r22r23 = \$r22r23, \$r24r25;;

     934:	a9 1a a5 57                                     	ffmshq.ru \$r41 = \$r41, \$r42;;

     938:	aa aa 63 53                                     	ffmshwq.rd.s \$r24r25 = \$r42, \$r42;;

     93c:	eb 3a ae 5a                                     	ffmshw.rz \$r43 = \$r43, \$r43;;

     940:	1a c7 69 4e                                     	ffmswcp.rna.s \$r26r27 = \$r26r27, \$r28r29;;

     944:	2c 5b b1 4b                                     	ffmswc.c.rnz \$r44 = \$r44, \$r44;;

     948:	6d eb 73 52                                     	ffmswdp.ro.s \$r28r29 = \$r45, \$r45;;

     94c:	ae 7b b5 55                                     	ffmswd \$r45 = \$r46, \$r46;;

     950:	ef 8b b8 43                                     	ffmswp.rn.s \$r46 = \$r47, \$r47;;

     954:	1e 18 78 47                                     	ffmswq.ru \$r30r31 = \$r30r31, \$r32r33;;

     958:	30 ac bc 41                                     	ffmsw.rd.s \$r47 = \$r48, \$r48;;

     95c:	f1 31 c3 46                                     	fixedd.rz \$r48 = \$r49, 7 \(0x7\);;

     960:	f1 c1 c7 47                                     	fixedud.rna.s \$r49 = \$r49, 7 \(0x7\);;

     964:	f2 51 cb 4f                                     	fixeduwp.rnz \$r50 = \$r50, 7 \(0x7\);;

     968:	f3 e1 cb 4b                                     	fixeduw.ro.s \$r50 = \$r51, 7 \(0x7\);;

     96c:	f3 71 cf 4e                                     	fixedwp \$r51 = \$r51, 7 \(0x7\);;

     970:	f4 81 d3 4a                                     	fixedw.rn.s \$r52 = \$r52, 7 \(0x7\);;

     974:	f5 11 d3 44                                     	floatd.ru \$r52 = \$r53, 7 \(0x7\);;

     978:	f5 a1 d7 45                                     	floatud.rd.s \$r53 = \$r53, 7 \(0x7\);;

     97c:	f6 31 db 4d                                     	floatuwp.rz \$r54 = \$r54, 7 \(0x7\);;

     980:	f7 c1 db 49                                     	floatuw.rna.s \$r54 = \$r55, 7 \(0x7\);;

     984:	f7 51 df 4c                                     	floatwp.rnz \$r55 = \$r55, 7 \(0x7\);;

     988:	f8 e1 e3 48                                     	floatw.ro.s \$r56 = \$r56, 7 \(0x7\);;

     98c:	79 8e e3 71                                     	fmaxd \$r56 = \$r57, \$r57;;

     990:	ba 8e e7 77                                     	fmaxhq \$r57 = \$r58, \$r58;;

     994:	fb 8e eb 75                                     	fmaxwp \$r58 = \$r59, \$r59;;

     998:	3c 8f ef 73                                     	fmaxw \$r59 = \$r60, \$r60;;

     99c:	7d 8f f3 70                                     	fmind \$r60 = \$r61, \$r61;;

     9a0:	be 8f f7 76                                     	fminhq \$r61 = \$r62, \$r62;;

     9a4:	ff 8f fb 74                                     	fminwp \$r62 = \$r63, \$r63;;

     9a8:	00 80 ff 72                                     	fminw \$r63 = \$r0, \$r0;;

     9ac:	40 70 80 4c                                     	fmm212w \$r32r33 = \$r0, \$r1;;

     9b0:	22 89 8c 4c                                     	fmm222w.rn.s \$r34r35 = \$r34r35, \$r36r37;;

     9b4:	41 10 90 4e                                     	fmma212w.ru \$r36r37 = \$r1, \$r1;;

     9b8:	27 aa 9c 4e                                     	fmma222w.tn.rd.s \$r38r39 = \$r38r39, \$r40r41;;

     9bc:	82 30 a0 4f                                     	fmms212w.rz \$r40r41 = \$r2, \$r2;;

     9c0:	6a cb ac 4f                                     	fmms222w.nt.rna.s \$r42r43 = \$r42r43, \$r44r45;;

     9c4:	c3 50 09 58                                     	fmuld.rnz \$r2 = \$r3, \$r3;;

     9c8:	ae eb b7 55                                     	fmulho.ro.s \$r44r45 = \$r46r47, \$r46r47;;

     9cc:	04 71 0d 5b                                     	fmulhq \$r3 = \$r4, \$r4;;

     9d0:	44 81 c7 51                                     	fmulhwq.rn.s \$r48r49 = \$r4, \$r5;;

     9d4:	85 11 16 5f                                     	fmulhw.ru \$r5 = \$r5, \$r6;;

     9d8:	b2 ac c0 4a                                     	fmulwcp.rd.s \$r48r49 = \$r50r51, \$r50r51;;

     9dc:	c6 31 18 49                                     	fmulwc.c.rz \$r6 = \$r6, \$r7;;

     9e0:	c7 c1 d7 50                                     	fmulwdp.rna.s \$r52r53 = \$r7, \$r7;;

     9e4:	08 52 21 59                                     	fmulwd.rnz \$r8 = \$r8, \$r8;;

     9e8:	49 e2 25 5a                                     	fmulwp.ro.s \$r9 = \$r9, \$r9;;

     9ec:	b6 7d d7 5e                                     	fmulwq \$r52r53 = \$r54r55, \$r54r55;;

     9f0:	8a 82 2a 5e                                     	fmulw.rn.s \$r10 = \$r10, \$r10;;

     9f4:	38 61 2f 7c                                     	fnarrowdwp.ru \$r11 = \$r56r57;;

     9f8:	0b 6a 2f 78                                     	fnarrowdw.rd.s \$r11 = \$r11;;

     9fc:	38 63 33 7e                                     	fnarrowwhq.rz \$r12 = \$r56r57;;

     a00:	0d 6c 33 7a                                     	fnarrowwh.rna.s \$r12 = \$r13;;

     a04:	0e 20 37 70                                     	fnegd \$r13 = \$r14;;

     a08:	0f 20 3b 76                                     	fneghq \$r14 = \$r15;;

     a0c:	10 20 3f 74                                     	fnegwp \$r15 = \$r16;;

     a10:	10 20 43 72                                     	fnegw \$r16 = \$r16;;

     a14:	11 65 47 72                                     	frecw.rnz \$r17 = \$r17;;

     a18:	12 6e 47 73                                     	frsrw.ro.s \$r17 = \$r18;;

     a1c:	3a 7f eb 5f                                     	fsbfdc.c \$r58r59 = \$r58r59, \$r60r61;;

     a20:	be 8f f3 5e                                     	fsbfdp.rn.s \$r60r61 = \$r62r63, \$r62r63;;

     a24:	80 10 03 5e                                     	fsbfdp.ru \$r0r1 = \$r0r1, \$r2r3;;

     a28:	d2 a4 4a 54                                     	fsbfd.rd.s \$r18 = \$r18, \$r19;;

     a2c:	04 31 0f 57                                     	fsbfho.rz \$r2r3 = \$r4r5, \$r4r5;;

     a30:	13 c5 4e 56                                     	fsbfhq.rna.s \$r19 = \$r19, \$r20;;

     a34:	54 55 52 57                                     	fsbfwc.c.rnz \$r20 = \$r20, \$r21;;

     a38:	06 e2 1f 5b                                     	fsbfwcp.c.ro.s \$r6r7 = \$r6r7, \$r8r9;;

     a3c:	8a 72 27 5a                                     	fsbfwq \$r8r9 = \$r10r11, \$r10r11;;

     a40:	95 85 56 55                                     	fsbfwp.rn.s \$r21 = \$r21, \$r22;;

     a44:	d6 15 5a 55                                     	fsbfwp.ru \$r22 = \$r22, \$r23;;

     a48:	8c a3 37 5a                                     	fsbfwq.rd.s \$r12r13 = \$r12r13, \$r14r15;;

     a4c:	17 36 5e 5d                                     	fsbfw.rz \$r23 = \$r23, \$r24;;

     a50:	0e 58 63 70                                     	fsdivd.s \$r24 = \$r14r15;;

     a54:	10 50 63 74                                     	fsdivwp \$r24 = \$r16r17;;

     a58:	10 58 67 72                                     	fsdivw.s \$r25 = \$r16r17;;

     a5c:	19 40 67 70                                     	fsrecd \$r25 = \$r25;;

     a60:	1a 48 6b 74                                     	fsrecwp.s \$r26 = \$r26;;

     a64:	1b 40 6b 72                                     	fsrecw \$r26 = \$r27;;

     a68:	1b 20 6f 78                                     	fsrsrd \$r27 = \$r27;;

     a6c:	1c 20 73 7c                                     	fsrsrwp \$r28 = \$r28;;

     a70:	1d 20 73 7a                                     	fsrsrw \$r28 = \$r29;;

     a74:	1d 38 77 7c                                     	fwidenlhwp.s \$r29 = \$r29;;

     a78:	1e 30 7b 7a                                     	fwidenlhw \$r30 = \$r30;;

     a7c:	1f 38 7b 78                                     	fwidenlwd.s \$r30 = \$r31;;

     a80:	1f 30 7f 7d                                     	fwidenmhwp \$r31 = \$r31;;

     a84:	20 38 83 7b                                     	fwidenmhw.s \$r32 = \$r32;;

     a88:	21 30 83 79                                     	fwidenmwd \$r32 = \$r33;;

     a8c:	21 00 c4 0f                                     	get \$r33 = \$pc;;

     a90:	21 00 c4 0f                                     	get \$r33 = \$pc;;

     a94:	00 00 80 17                                     	goto fffffffffe000a94 <main\+0xfffffffffe000a94>;;

     a98:	a2 e8 5e 3c                                     	i1invals \$r34\[\$r34\];;

     a9c:	e2 ff 5c bc ff ff ff 9f ff ff ff 18             	i1invals 2305843009213693951 \(0x1fffffffffffffff\)\[\$r34\];;

     aa8:	23 f0 5c 3c                                     	i1invals -64 \(0xffffffc0\)\[\$r35\];;

     aac:	23 00 5c bc 00 00 80 1f                         	i1invals -8589934592 \(0xfffffffe00000000\)\[\$r35\];;

     ab4:	00 00 cc 3c                                     	i1inval;;

     ab8:	23 00 dc 0f                                     	icall \$r35;;

     abc:	24 00 cc 0f                                     	iget \$r36;;

     ac0:	24 00 d8 0f                                     	igoto \$r36;;

     ac4:	e5 71 93 60                                     	insf \$r36 = \$r37, 7 \(0x7\), 7 \(0x7\);;

     ac8:	a5 69 96 70                                     	landd \$r37 = \$r37, \$r38;;

     acc:	e6 79 9a 70                                     	landw \$r38 = \$r38, \$r39;;

     ad0:	e7 77 9e f0 ff ff ff 00                         	landw \$r39 = \$r39, 536870911 \(0x1fffffff\);;

     ad8:	28 fa a2 24                                     	lbs.xs \$r40 = \$r40\[\$r40\];;

     adc:	69 5a a6 a5 00 00 00 98 00 00 80 1f             	lbs.s.dgtz \$r41\? \$r41 = -1125899906842624 \(0xfffc000000000000\)\[\$r41\];;

     ae8:	aa 6a aa a6 00 00 80 1f                         	lbs.u.odd \$r42\? \$r42 = -8388608 \(0xff800000\)\[\$r42\];;

     af0:	eb 7a ae 27                                     	lbs.us.even \$r43\? \$r43 = \[\$r43\];;

     af4:	ec ff b0 a4 ff ff ff 9f ff ff ff 18             	lbs \$r44 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r44\];;

     b00:	2d f0 b0 25                                     	lbs.s \$r44 = -64 \(0xffffffc0\)\[\$r45\];;

     b04:	2d 00 b4 a6 00 00 80 1f                         	lbs.u \$r45 = -8589934592 \(0xfffffffe00000000\)\[\$r45\];;

     b0c:	ae eb ba 23                                     	lbz.us \$r46 = \$r46\[\$r46\];;

     b10:	ef 8b be a0 00 00 00 98 00 00 80 1f             	lbz.wnez \$r47\? \$r47 = -1125899906842624 \(0xfffc000000000000\)\[\$r47\];;

     b1c:	30 9c c2 a1 00 00 80 1f                         	lbz.s.weqz \$r48\? \$r48 = -8388608 \(0xff800000\)\[\$r48\];;

     b24:	71 ac c6 22                                     	lbz.u.wltz \$r49\? \$r49 = \[\$r49\];;

     b28:	f2 ff c8 a3 ff ff ff 9f ff ff ff 18             	lbz.us \$r50 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r50\];;

     b34:	33 f0 c8 20                                     	lbz \$r50 = -64 \(0xffffffc0\)\[\$r51\];;

     b38:	33 00 cc a1 00 00 80 1f                         	lbz.s \$r51 = -8589934592 \(0xfffffffe00000000\)\[\$r51\];;

     b40:	34 fd d2 3a                                     	ld.u.xs \$r52 = \$r52\[\$r52\];;

     b44:	75 bd d6 bb 00 00 00 98 00 00 80 1f             	ld.us.wgez \$r53\? \$r53 = -1125899906842624 \(0xfffc000000000000\)\[\$r53\];;

     b50:	b6 cd da b8 00 00 80 1f                         	ld.wlez \$r54\? \$r54 = -8388608 \(0xff800000\)\[\$r54\];;

     b58:	f7 dd de 39                                     	ld.s.wgtz \$r55\? \$r55 = \[\$r55\];;

     b5c:	f8 ff e0 ba ff ff ff 9f ff ff ff 18             	ld.u \$r56 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r56\];;

     b68:	39 f0 e0 3b                                     	ld.us \$r56 = -64 \(0xffffffc0\)\[\$r57\];;

     b6c:	39 00 e4 b8 00 00 80 1f                         	ld \$r57 = -8589934592 \(0xfffffffe00000000\)\[\$r57\];;

     b74:	ba ee ea 2d                                     	lhs.s \$r58 = \$r58\[\$r58\];;

     b78:	fb 0e ee ae 00 00 00 98 00 00 80 1f             	lhs.u.dnez \$r59\? \$r59 = -1125899906842624 \(0xfffc000000000000\)\[\$r59\];;

     b84:	3c 1f f2 af 00 00 80 1f                         	lhs.us.deqz \$r60\? \$r60 = -8388608 \(0xff800000\)\[\$r60\];;

     b8c:	7d 2f f6 2c                                     	lhs.dltz \$r61\? \$r61 = \[\$r61\];;

     b90:	fe ff f8 ad ff ff ff 9f ff ff ff 18             	lhs.s \$r62 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r62\];;

     b9c:	3f f0 f8 2e                                     	lhs.u \$r62 = -64 \(0xffffffc0\)\[\$r63\];;

     ba0:	3f 00 fc af 00 00 80 1f                         	lhs.us \$r63 = -8589934592 \(0xfffffffe00000000\)\[\$r63\];;

     ba8:	00 f0 02 28                                     	lhz.xs \$r0 = \$r0\[\$r0\];;

     bac:	41 30 06 a9 00 00 00 98 00 00 80 1f             	lhz.s.dgez \$r1\? \$r1 = -1125899906842624 \(0xfffc000000000000\)\[\$r1\];;

     bb8:	82 40 0a aa 00 00 80 1f                         	lhz.u.dlez \$r2\? \$r2 = -8388608 \(0xff800000\)\[\$r2\];;

     bc0:	c3 50 0e 2b                                     	lhz.us.dgtz \$r3\? \$r3 = \[\$r3\];;

     bc4:	c4 ff 10 a8 ff ff ff 9f ff ff ff 18             	lhz \$r4 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r4\];;

     bd0:	05 f0 10 29                                     	lhz.s \$r4 = -64 \(0xffffffc0\)\[\$r5\];;

     bd4:	05 00 14 aa 00 00 80 1f                         	lhz.u \$r5 = -8589934592 \(0xfffffffe00000000\)\[\$r5\];;

     bdc:	86 61 1a 71                                     	lnandd \$r6 = \$r6, \$r6;;

     be0:	c7 71 1e 71                                     	lnandw \$r7 = \$r7, \$r7;;

     be4:	c8 77 22 f1 ff ff ff 00                         	lnandw \$r8 = \$r8, 536870911 \(0x1fffffff\);;

     bec:	49 62 22 73                                     	lnord \$r8 = \$r9, \$r9;;

     bf0:	8a 72 26 73                                     	lnorw \$r9 = \$r10, \$r10;;

     bf4:	cb 77 2a f3 ff ff ff 00                         	lnorw \$r10 = \$r11, 536870911 \(0x1fffffff\);;

     bfc:	0b 00 78 0f                                     	loopdo \$r11, ffffffffffff8bfc <main\+0xffffffffffff8bfc>;;

     c00:	0c 63 2e 72                                     	lord \$r11 = \$r12, \$r12;;

     c04:	8d 73 36 72                                     	lorw \$r13 = \$r13, \$r14;;

     c08:	cf 77 3a f2 ff ff ff 00                         	lorw \$r14 = \$r15, 536870911 \(0x1fffffff\);;

     c10:	d0 e3 16 3f                                     	lo.us \$r4r5r6r7 = \$r15\[\$r16\];;

     c14:	10 04 2e bc 00 00 00 98 00 00 80 1f             	lo.u0 \$r16\? \$r8r9r10r11 = -1125899906842624 \(0xfffc000000000000\)\[\$r16\];;

     c20:	51 14 3e bd 00 00 80 1f                         	lo.s.u1 \$r17\? \$r12r13r14r15 = -8388608 \(0xff800000\)\[\$r17\];;

     c28:	52 24 4e 3e                                     	lo.u.u2 \$r17\? \$r16r17r18r19 = \[\$r18\];;

     c2c:	92 64 56 bf 00 00 00 98 00 00 80 1f             	lo.us.odd \$r18\? \$r20r21r22r23 = -1125899906842624 \(0xfffc000000000000\)\[\$r18\];;

     c38:	d3 74 66 bc 00 00 80 1f                         	lo.even \$r19\? \$r24r25r26r27 = -8388608 \(0xff800000\)\[\$r19\];;

     c40:	d4 84 76 3d                                     	lo.s.wnez \$r19\? \$r28r29r30r31 = \[\$r20\];;

     c44:	d4 ff 84 be ff ff ff 9f ff ff ff 18             	lo.u \$r32r33r34r35 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r20\];;

     c50:	14 f0 94 3f                                     	lo.us \$r36r37r38r39 = -64 \(0xffffffc0\)\[\$r20\];;

     c54:	15 00 a4 bc 00 00 80 1f                         	lo \$r40r41r42r43 = -8589934592 \(0xfffffffe00000000\)\[\$r21\];;

     c5c:	55 f5 4a 3d                                     	lq.s.xs \$r18r19 = \$r21\[\$r21\];;

     c60:	96 95 4a be 00 00 00 98 00 00 80 1f             	lq.u.weqz \$r22\? \$r18r19 = -1125899906842624 \(0xfffc000000000000\)\[\$r22\];;

     c6c:	97 a5 52 bf 00 00 80 1f                         	lq.us.wltz \$r22\? \$r20r21 = -8388608 \(0xff800000\)\[\$r23\];;

     c74:	d7 b5 52 3c                                     	lq.wgez \$r23\? \$r20r21 = \[\$r23\];;

     c78:	d8 ff 58 bd ff ff ff 9f ff ff ff 18             	lq.s \$r22r23 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r24\];;

     c84:	18 f0 58 3e                                     	lq.u \$r22r23 = -64 \(0xffffffc0\)\[\$r24\];;

     c88:	18 00 60 bf 00 00 80 1f                         	lq.us \$r24r25 = -8589934592 \(0xfffffffe00000000\)\[\$r24\];;

     c90:	59 e6 66 34                                     	lws \$r25 = \$r25\[\$r25\];;

     c94:	9a c6 6a b5 00 00 00 98 00 00 80 1f             	lws.s.wlez \$r26\? \$r26 = -1125899906842624 \(0xfffc000000000000\)\[\$r26\];;

     ca0:	db d6 6e b6 00 00 80 1f                         	lws.u.wgtz \$r27\? \$r27 = -8388608 \(0xff800000\)\[\$r27\];;

     ca8:	1c 07 72 37                                     	lws.us.dnez \$r28\? \$r28 = \[\$r28\];;

     cac:	dd ff 74 b4 ff ff ff 9f ff ff ff 18             	lws \$r29 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r29\];;

     cb8:	1e f0 74 35                                     	lws.s \$r29 = -64 \(0xffffffc0\)\[\$r30\];;

     cbc:	1e 00 78 b6 00 00 80 1f                         	lws.u \$r30 = -8589934592 \(0xfffffffe00000000\)\[\$r30\];;

     cc4:	df f7 7e 33                                     	lwz.us.xs \$r31 = \$r31\[\$r31\];;

     cc8:	20 18 82 b0 00 00 00 98 00 00 80 1f             	lwz.deqz \$r32\? \$r32 = -1125899906842624 \(0xfffc000000000000\)\[\$r32\];;

     cd4:	61 28 86 b1 00 00 80 1f                         	lwz.s.dltz \$r33\? \$r33 = -8388608 \(0xff800000\)\[\$r33\];;

     cdc:	a2 38 8a 32                                     	lwz.u.dgez \$r34\? \$r34 = \[\$r34\];;

     ce0:	e3 ff 8c b3 ff ff ff 9f ff ff ff 18             	lwz.us \$r35 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r35\];;

     cec:	24 f0 8c 30                                     	lwz \$r35 = -64 \(0xffffffc0\)\[\$r36\];;

     cf0:	24 00 90 b1 00 00 80 1f                         	lwz.s \$r36 = -8589934592 \(0xfffffffe00000000\)\[\$r36\];;

     cf8:	65 19 60 58                                     	madddt \$r24r25 = \$r37, \$r37;;

     cfc:	a6 09 94 50                                     	maddd \$r37 = \$r38, \$r38;;

     d00:	e7 07 98 d0 ff ff ff 10                         	maddd \$r38 = \$r39, 536870911 \(0x1fffffff\);;

     d08:	27 0a 9c 52                                     	maddhq \$r39 = \$r39, \$r40;;

     d0c:	e8 07 a0 d2 ff ff ff 10                         	maddhq \$r40 = \$r40, 536870911 \(0x1fffffff\);;

     d14:	69 4a 68 50                                     	maddhwq \$r26r27 = \$r41, \$r41;;

     d18:	1c 27 6c 51                                     	maddmwq \$r26r27 = \$r28r29, \$r28r29;;

     d1c:	a9 1a 78 5a                                     	maddsudt \$r30r31 = \$r41, \$r42;;

     d20:	aa 4a 78 52                                     	maddsuhwq \$r30r31 = \$r42, \$r42;;

     d24:	a0 28 84 53                                     	maddsumwq \$r32r33 = \$r32r33, \$r34r35;;

     d28:	eb 0a 88 5a                                     	maddsuwdp \$r34r35 = \$r43, \$r43;;

     d2c:	2c 3b ac 52                                     	maddsuwd \$r43 = \$r44, \$r44;;

     d30:	ed 37 b0 d2 ff ff ff 10                         	maddsuwd \$r44 = \$r45, 536870911 \(0x1fffffff\);;

     d38:	6d 1b 90 59                                     	maddudt \$r36r37 = \$r45, \$r45;;

     d3c:	ae 4b 90 51                                     	madduhwq \$r36r37 = \$r46, \$r46;;

     d40:	26 2a 9c 52                                     	maddumwq \$r38r39 = \$r38r39, \$r40r41;;

     d44:	ee 0b a0 59                                     	madduwdp \$r40r41 = \$r46, \$r47;;

     d48:	2f 3c bc 51                                     	madduwd \$r47 = \$r47, \$r48;;

     d4c:	f0 37 c0 d1 ff ff ff 10                         	madduwd \$r48 = \$r48, 536870911 \(0x1fffffff\);;

     d54:	71 1c a8 5b                                     	madduzdt \$r42r43 = \$r49, \$r49;;

     d58:	b1 0c a8 58                                     	maddwdp \$r42r43 = \$r49, \$r50;;

     d5c:	f2 3c c8 50                                     	maddwd \$r50 = \$r50, \$r51;;

     d60:	f3 37 cc d0 ff ff ff 10                         	maddwd \$r51 = \$r51, 536870911 \(0x1fffffff\);;

     d68:	34 0d d0 51                                     	maddwp \$r52 = \$r52, \$r52;;

     d6c:	f5 07 d4 d1 ff ff ff 10                         	maddwp \$r53 = \$r53, 536870911 \(0x1fffffff\);;

     d74:	ac 2b b4 50                                     	maddwq \$r44r45 = \$r44r45, \$r46r47;;

     d78:	b6 3d d4 53                                     	maddw \$r53 = \$r54, \$r54;;

     d7c:	f7 37 d8 d3 ff ff ff 10                         	maddw \$r54 = \$r55, 536870911 \(0x1fffffff\);;

     d84:	c0 ff dc e0 ff ff ff 87 ff ff ff 00             	make \$r55 = 2305843009213693951 \(0x1fffffffffffffff\);;

     d90:	3c 00 dc e0 00 00 00 00                         	make \$r55 = -549755813888 \(0xffffff8000000000\);;

     d98:	00 f0 e0 60                                     	make \$r56 = -4096 \(0xfffff000\);;

     d9c:	78 ae e2 75                                     	maxbo \$r56 = \$r56, \$r57;;

     da0:	f9 af e6 f5 ff ff ff 00                         	maxbo.@ \$r57 = \$r57, 536870911 \(0x1fffffff\);;

     da8:	fa ff e8 e5 ff ff ff 87 ff ff ff 00             	maxd \$r58 = \$r58, 2305843009213693951 \(0x1fffffffffffffff\);;

     db4:	fb 0e e9 75                                     	maxd \$r58 = \$r59, \$r59;;

     db8:	3c f0 ec 65                                     	maxd \$r59 = \$r60, -64 \(0xffffffc0\);;

     dbc:	3c 00 f0 e5 00 00 80 07                         	maxd \$r60 = \$r60, -8589934592 \(0xfffffffe00000000\);;

     dc4:	fd 0f f5 f5 ff ff ff 00                         	maxd.@ \$r61 = \$r61, 536870911 \(0x1fffffff\);;

     dcc:	be 3f f5 75                                     	maxhq \$r61 = \$r62, \$r62;;

     dd0:	ff 37 f9 f5 ff ff ff 00                         	maxhq \$r62 = \$r63, 536870911 \(0x1fffffff\);;

     dd8:	bf c0 ff 75                                     	maxrbod \$r63 = \$r63;;

     ddc:	40 c0 03 75                                     	maxrhqd \$r0 = \$r0;;

     de0:	01 c0 03 75                                     	maxrwpd \$r0 = \$r1;;

     de4:	81 a0 06 77                                     	maxubo \$r1 = \$r1, \$r2;;

     de8:	c2 af 0a f7 ff ff ff 00                         	maxubo.@ \$r2 = \$r2, 536870911 \(0x1fffffff\);;

     df0:	c3 ff 0c e7 ff ff ff 87 ff ff ff 00             	maxud \$r3 = \$r3, 2305843009213693951 \(0x1fffffffffffffff\);;

     dfc:	04 01 0d 77                                     	maxud \$r3 = \$r4, \$r4;;

     e00:	05 f0 10 67                                     	maxud \$r4 = \$r5, -64 \(0xffffffc0\);;

     e04:	05 00 14 e7 00 00 80 07                         	maxud \$r5 = \$r5, -8589934592 \(0xfffffffe00000000\);;

     e0c:	c6 0f 19 f7 ff ff ff 00                         	maxud.@ \$r6 = \$r6, 536870911 \(0x1fffffff\);;

     e14:	c7 31 19 77                                     	maxuhq \$r6 = \$r7, \$r7;;

     e18:	c8 37 1d f7 ff ff ff 00                         	maxuhq \$r7 = \$r8, 536870911 \(0x1fffffff\);;

     e20:	88 c0 23 79                                     	maxurbod \$r8 = \$r8;;

     e24:	49 c0 27 79                                     	maxurhqd \$r9 = \$r9;;

     e28:	0a c0 27 79                                     	maxurwpd \$r9 = \$r10;;

     e2c:	ca 22 29 77                                     	maxuwp \$r10 = \$r10, \$r11;;

     e30:	cb 2f 2d f7 ff ff ff 00                         	maxuwp.@ \$r11 = \$r11, 536870911 \(0x1fffffff\);;

     e38:	4c 13 31 77                                     	maxuw \$r12 = \$r12, \$r13;;

     e3c:	0e f0 34 77                                     	maxuw \$r13 = \$r14, -64 \(0xffffffc0\);;

     e40:	0f 00 38 f7 00 00 80 07                         	maxuw \$r14 = \$r15, -8589934592 \(0xfffffffe00000000\);;

     e48:	10 24 3d 75                                     	maxwp \$r15 = \$r16, \$r16;;

     e4c:	d1 27 41 f5 ff ff ff 00                         	maxwp \$r16 = \$r17, 536870911 \(0x1fffffff\);;

     e54:	91 14 45 75                                     	maxw \$r17 = \$r17, \$r18;;

     e58:	12 f0 48 75                                     	maxw \$r18 = \$r18, -64 \(0xffffffc0\);;

     e5c:	13 00 4c f5 00 00 80 07                         	maxw \$r19 = \$r19, -8589934592 \(0xfffffffe00000000\);;

     e64:	14 a5 4e 74                                     	minbo \$r19 = \$r20, \$r20;;

     e68:	d5 af 52 f4 ff ff ff 00                         	minbo.@ \$r20 = \$r21, 536870911 \(0x1fffffff\);;

     e70:	d5 ff 54 e4 ff ff ff 87 ff ff ff 00             	mind \$r21 = \$r21, 2305843009213693951 \(0x1fffffffffffffff\);;

     e7c:	96 05 59 74                                     	mind \$r22 = \$r22, \$r22;;

     e80:	17 f0 5c 64                                     	mind \$r23 = \$r23, -64 \(0xffffffc0\);;

     e84:	18 00 5c e4 00 00 80 07                         	mind \$r23 = \$r24, -8589934592 \(0xfffffffe00000000\);;

     e8c:	d8 0f 61 f4 ff ff ff 00                         	mind.@ \$r24 = \$r24, 536870911 \(0x1fffffff\);;

     e94:	59 36 65 74                                     	minhq \$r25 = \$r25, \$r25;;

     e98:	da 37 69 f4 ff ff ff 00                         	minhq \$r26 = \$r26, 536870911 \(0x1fffffff\);;

     ea0:	9b c0 6b 74                                     	minrbod \$r26 = \$r27;;

     ea4:	5b c0 6f 74                                     	minrhqd \$r27 = \$r27;;

     ea8:	1c c0 73 74                                     	minrwpd \$r28 = \$r28;;

     eac:	5d a7 72 76                                     	minubo \$r28 = \$r29, \$r29;;

     eb0:	de af 76 f6 ff ff ff 00                         	minubo.@ \$r29 = \$r30, 536870911 \(0x1fffffff\);;

     eb8:	de ff 78 e6 ff ff ff 87 ff ff ff 00             	minud \$r30 = \$r30, 2305843009213693951 \(0x1fffffffffffffff\);;

     ec4:	df 07 7d 76                                     	minud \$r31 = \$r31, \$r31;;

     ec8:	20 f0 80 66                                     	minud \$r32 = \$r32, -64 \(0xffffffc0\);;

     ecc:	21 00 80 e6 00 00 80 07                         	minud \$r32 = \$r33, -8589934592 \(0xfffffffe00000000\);;

     ed4:	e1 0f 85 f6 ff ff ff 00                         	minud.@ \$r33 = \$r33, 536870911 \(0x1fffffff\);;

     edc:	a2 38 89 76                                     	minuhq \$r34 = \$r34, \$r34;;

     ee0:	e3 37 8d f6 ff ff ff 00                         	minuhq \$r35 = \$r35, 536870911 \(0x1fffffff\);;

     ee8:	a4 c0 8f 78                                     	minurbod \$r35 = \$r36;;

     eec:	64 c0 93 78                                     	minurhqd \$r36 = \$r36;;

     ef0:	25 c0 97 78                                     	minurwpd \$r37 = \$r37;;

     ef4:	a6 29 95 76                                     	minuwp \$r37 = \$r38, \$r38;;

     ef8:	e7 2f 99 f6 ff ff ff 00                         	minuwp.@ \$r38 = \$r39, 536870911 \(0x1fffffff\);;

     f00:	27 1a 9d 76                                     	minuw \$r39 = \$r39, \$r40;;

     f04:	28 f0 a0 76                                     	minuw \$r40 = \$r40, -64 \(0xffffffc0\);;

     f08:	29 00 a4 f6 00 00 80 07                         	minuw \$r41 = \$r41, -8589934592 \(0xfffffffe00000000\);;

     f10:	aa 2a a5 74                                     	minwp \$r41 = \$r42, \$r42;;

     f14:	eb 27 a9 f4 ff ff ff 00                         	minwp \$r42 = \$r43, 536870911 \(0x1fffffff\);;

     f1c:	2b 1b ad 74                                     	minw \$r43 = \$r43, \$r44;;

     f20:	2c f0 b0 74                                     	minw \$r44 = \$r44, -64 \(0xffffffc0\);;

     f24:	2d 00 b4 f4 00 00 80 07                         	minw \$r45 = \$r45, -8589934592 \(0xfffffffe00000000\);;

     f2c:	ad 1b b8 53                                     	mm212w \$r46r47 = \$r45, \$r46;;

     f30:	ae 0b c0 5b                                     	mma212w \$r48r49 = \$r46, \$r46;;

     f34:	ef 0b c0 5f                                     	mms212w \$r48r49 = \$r47, \$r47;;

     f38:	2f 1c c8 5c                                     	msbfdt \$r50r51 = \$r47, \$r48;;

     f3c:	70 0c c0 54                                     	msbfd \$r48 = \$r48, \$r49;;

     f40:	b1 0c c4 56                                     	msbfhq \$r49 = \$r49, \$r50;;

     f44:	b2 4c c8 54                                     	msbfhwq \$r50r51 = \$r50, \$r50;;

     f48:	b4 2d d4 55                                     	msbfmwq \$r52r53 = \$r52r53, \$r54r55;;

     f4c:	f3 1c d8 5e                                     	msbfsudt \$r54r55 = \$r51, \$r51;;

     f50:	33 4d e0 56                                     	msbfsuhwq \$r56r57 = \$r51, \$r52;;

     f54:	ba 2e e4 57                                     	msbfsumwq \$r56r57 = \$r58r59, \$r58r59;;

     f58:	34 0d f0 5e                                     	msbfsuwdp \$r60r61 = \$r52, \$r52;;

     f5c:	75 3d d4 56                                     	msbfsuwd \$r53 = \$r53, \$r53;;

     f60:	f6 37 d8 d6 ff ff ff 10                         	msbfsuwd \$r54 = \$r54, 536870911 \(0x1fffffff\);;

     f68:	f6 1d f0 5d                                     	msbfudt \$r60r61 = \$r54, \$r55;;

     f6c:	f7 4d f8 55                                     	msbfuhwq \$r62r63 = \$r55, \$r55;;

     f70:	00 20 fc 56                                     	msbfumwq \$r62r63 = \$r0r1, \$r0r1;;

     f74:	38 0e 08 5d                                     	msbfuwdp \$r2r3 = \$r56, \$r56;;

     f78:	79 3e e0 55                                     	msbfuwd \$r56 = \$r57, \$r57;;

     f7c:	fa 37 e4 d5 ff ff ff 10                         	msbfuwd \$r57 = \$r58, 536870911 \(0x1fffffff\);;

     f84:	ba 1e 08 5f                                     	msbfuzdt \$r2r3 = \$r58, \$r58;;

     f88:	fb 0e 10 5c                                     	msbfwdp \$r4r5 = \$r59, \$r59;;

     f8c:	3c 3f ec 54                                     	msbfwd \$r59 = \$r60, \$r60;;

     f90:	fd 37 f0 d4 ff ff ff 10                         	msbfwd \$r60 = \$r61, 536870911 \(0x1fffffff\);;

     f98:	bd 0f f4 55                                     	msbfwp \$r61 = \$r61, \$r62;;

     f9c:	86 21 14 54                                     	msbfwq \$r4r5 = \$r6r7, \$r6r7;;

     fa0:	fe 3f f8 57                                     	msbfw \$r62 = \$r62, \$r63;;

     fa4:	ff 37 fc d7 ff ff ff 10                         	msbfw \$r63 = \$r63, 536870911 \(0x1fffffff\);;

     fac:	00 10 24 58                                     	muldt \$r8r9 = \$r0, \$r0;;

     fb0:	41 10 00 54                                     	muld \$r0 = \$r1, \$r1;;

     fb4:	c2 17 04 d4 ff ff ff 10                         	muld \$r1 = \$r2, 536870911 \(0x1fffffff\);;

     fbc:	c2 10 08 56                                     	mulhq \$r2 = \$r2, \$r3;;

     fc0:	c3 17 0c d6 ff ff ff 10                         	mulhq \$r3 = \$r3, 536870911 \(0x1fffffff\);;

     fc8:	04 41 20 58                                     	mulhwq \$r8r9 = \$r4, \$r4;;

     fcc:	0a 23 28 55                                     	mulmwq \$r10r11 = \$r10r11, \$r12r13;;

     fd0:	44 11 34 5a                                     	mulsudt \$r12r13 = \$r4, \$r5;;

     fd4:	45 41 38 5a                                     	mulsuhwq \$r14r15 = \$r5, \$r5;;

     fd8:	10 24 38 57                                     	mulsumwq \$r14r15 = \$r16r17, \$r16r17;;

     fdc:	86 11 48 52                                     	mulsuwdp \$r18r19 = \$r6, \$r6;;

     fe0:	c7 31 18 5a                                     	mulsuwd \$r6 = \$r7, \$r7;;

     fe4:	c8 37 1c da ff ff ff 10                         	mulsuwd \$r7 = \$r8, 536870911 \(0x1fffffff\);;

     fec:	08 12 4c 59                                     	muludt \$r18r19 = \$r8, \$r8;;

     ff0:	49 42 50 59                                     	muluhwq \$r20r21 = \$r9, \$r9;;

     ff4:	96 25 50 56                                     	mulumwq \$r20r21 = \$r22r23, \$r22r23;;

     ff8:	89 12 60 51                                     	muluwdp \$r24r25 = \$r9, \$r10;;

     ffc:	ca 32 28 59                                     	muluwd \$r10 = \$r10, \$r11;;

    1000:	cb 37 2c d9 ff ff ff 10                         	muluwd \$r11 = \$r11, 536870911 \(0x1fffffff\);;

    1008:	0c 13 60 50                                     	mulwdp \$r24r25 = \$r12, \$r12;;

    100c:	8d 33 34 58                                     	mulwd \$r13 = \$r13, \$r14;;

    1010:	cf 37 38 d8 ff ff ff 10                         	mulwd \$r14 = \$r15, 536870911 \(0x1fffffff\);;

    1018:	10 14 3c 55                                     	mulwp \$r15 = \$r16, \$r16;;

    101c:	d1 17 40 d5 ff ff ff 10                         	mulwp \$r16 = \$r17, 536870911 \(0x1fffffff\);;

    1024:	1a 27 68 54                                     	mulwq \$r26r27 = \$r26r27, \$r28r29;;

    1028:	91 34 44 5b                                     	mulw \$r17 = \$r17, \$r18;;

    102c:	d2 37 48 db ff ff ff 10                         	mulw \$r18 = \$r18, 536870911 \(0x1fffffff\);;

    1034:	d3 ff 4c e9 ff ff ff 87 ff ff ff 00             	nandd \$r19 = \$r19, 2305843009213693951 \(0x1fffffffffffffff\);;

    1040:	14 05 4d 79                                     	nandd \$r19 = \$r20, \$r20;;

    1044:	15 f0 50 69                                     	nandd \$r20 = \$r21, -64 \(0xffffffc0\);;

    1048:	15 00 54 e9 00 00 80 07                         	nandd \$r21 = \$r21, -8589934592 \(0xfffffffe00000000\);;

    1050:	d6 0f 59 f9 ff ff ff 00                         	nandd.@ \$r22 = \$r22, 536870911 \(0x1fffffff\);;

    1058:	d7 15 59 79                                     	nandw \$r22 = \$r23, \$r23;;

    105c:	18 f0 5c 79                                     	nandw \$r23 = \$r24, -64 \(0xffffffc0\);;

    1060:	18 00 60 f9 00 00 80 07                         	nandw \$r24 = \$r24, -8589934592 \(0xfffffffe00000000\);;

    1068:	19 a0 66 f1 00 00 00 00                         	negbo \$r25 = \$r25;;

    1070:	1a 00 64 63                                     	negd \$r25 = \$r26;;

    1074:	1a 30 69 f3 00 00 00 00                         	neghq \$r26 = \$r26;;

    107c:	1b b0 6e fd 00 00 00 00                         	negsbo \$r27 = \$r27;;

    1084:	1c 40 6d fd 00 00 00 00                         	negsd \$r27 = \$r28;;

    108c:	1c 70 71 fd 00 00 00 00                         	negshq \$r28 = \$r28;;

    1094:	1d 60 75 fd 00 00 00 00                         	negswp \$r29 = \$r29;;

    109c:	1e 50 75 fd 00 00 00 00                         	negsw \$r29 = \$r30;;

    10a4:	1e 20 79 f3 00 00 00 00                         	negwp \$r30 = \$r30;;

    10ac:	1f 00 7c 73                                     	negw \$r31 = \$r31;;

    10b0:	00 f0 03 7f                                     	nop;;

    10b4:	e0 ff 7c eb ff ff ff 87 ff ff ff 00             	nord \$r31 = \$r32, 2305843009213693951 \(0x1fffffffffffffff\);;

    10c0:	60 08 81 7b                                     	nord \$r32 = \$r32, \$r33;;

    10c4:	21 f0 84 6b                                     	nord \$r33 = \$r33, -64 \(0xffffffc0\);;

    10c8:	22 00 88 eb 00 00 80 07                         	nord \$r34 = \$r34, -8589934592 \(0xfffffffe00000000\);;

    10d0:	e3 0f 89 fb ff ff ff 00                         	nord.@ \$r34 = \$r35, 536870911 \(0x1fffffff\);;

    10d8:	23 19 8d 7b                                     	norw \$r35 = \$r35, \$r36;;

    10dc:	24 f0 90 7b                                     	norw \$r36 = \$r36, -64 \(0xffffffc0\);;

    10e0:	25 00 94 fb 00 00 80 07                         	norw \$r37 = \$r37, -8589934592 \(0xfffffffe00000000\);;

    10e8:	e6 ff 94 6c                                     	notd \$r37 = \$r38;;

    10ec:	e6 ff 98 7c                                     	notw \$r38 = \$r38;;

    10f0:	e7 ff 9c ed ff ff ff 87 ff ff ff 00             	nxord \$r39 = \$r39, 2305843009213693951 \(0x1fffffffffffffff\);;

    10fc:	28 0a 9d 7d                                     	nxord \$r39 = \$r40, \$r40;;

    1100:	29 f0 a0 6d                                     	nxord \$r40 = \$r41, -64 \(0xffffffc0\);;

    1104:	29 00 a4 ed 00 00 80 07                         	nxord \$r41 = \$r41, -8589934592 \(0xfffffffe00000000\);;

    110c:	ea 0f a9 fd ff ff ff 00                         	nxord.@ \$r42 = \$r42, 536870911 \(0x1fffffff\);;

    1114:	eb 1a a9 7d                                     	nxorw \$r42 = \$r43, \$r43;;

    1118:	2c f0 ac 7d                                     	nxorw \$r43 = \$r44, -64 \(0xffffffc0\);;

    111c:	2c 00 b0 fd 00 00 80 07                         	nxorw \$r44 = \$r44, -8589934592 \(0xfffffffe00000000\);;

    1124:	ed ff b4 ea ff ff ff 87 ff ff ff 00             	ord \$r45 = \$r45, 2305843009213693951 \(0x1fffffffffffffff\);;

    1130:	ae 0b b5 7a                                     	ord \$r45 = \$r46, \$r46;;

    1134:	2f f0 b8 6a                                     	ord \$r46 = \$r47, -64 \(0xffffffc0\);;

    1138:	2f 00 bc ea 00 00 80 07                         	ord \$r47 = \$r47, -8589934592 \(0xfffffffe00000000\);;

    1140:	f0 0f c1 fa ff ff ff 00                         	ord.@ \$r48 = \$r48, 536870911 \(0x1fffffff\);;

    1148:	f1 ff c0 ef ff ff ff 87 ff ff ff 00             	ornd \$r48 = \$r49, 2305843009213693951 \(0x1fffffffffffffff\);;

    1154:	b1 0c c5 7f                                     	ornd \$r49 = \$r49, \$r50;;

    1158:	32 f0 c8 6f                                     	ornd \$r50 = \$r50, -64 \(0xffffffc0\);;

    115c:	33 00 cc ef 00 00 80 07                         	ornd \$r51 = \$r51, -8589934592 \(0xfffffffe00000000\);;

    1164:	f4 0f cd ff ff ff ff 00                         	ornd.@ \$r51 = \$r52, 536870911 \(0x1fffffff\);;

    116c:	74 1d d1 7f                                     	ornw \$r52 = \$r52, \$r53;;

    1170:	35 f0 d4 7f                                     	ornw \$r53 = \$r53, -64 \(0xffffffc0\);;

    1174:	36 00 d8 ff 00 00 80 07                         	ornw \$r54 = \$r54, -8589934592 \(0xfffffffe00000000\);;

    117c:	b7 c0 db 71                                     	orrbod \$r54 = \$r55;;

    1180:	77 c0 df 71                                     	orrhqd \$r55 = \$r55;;

    1184:	38 c0 e3 71                                     	orrwpd \$r56 = \$r56;;

    1188:	79 1e e1 7a                                     	orw \$r56 = \$r57, \$r57;;

    118c:	3a f0 e4 7a                                     	orw \$r57 = \$r58, -64 \(0xffffffc0\);;

    1190:	3a 00 e8 fa 00 00 80 07                         	orw \$r58 = \$r58, -8589934592 \(0xfffffffe00000000\);;

    1198:	c0 ff ec f0 ff ff ff 87 ff ff ff 00             	pcrel \$r59 = 2305843009213693951 \(0x1fffffffffffffff\);;

    11a4:	3c 00 ec f0 00 00 00 00                         	pcrel \$r59 = -549755813888 \(0xffffff8000000000\);;

    11ac:	00 f0 ec 70                                     	pcrel \$r59 = -4096 \(0xfffff000\);;

    11b0:	00 00 d0 0f                                     	ret;;

    11b4:	00 00 d4 0f                                     	rfe;;

    11b8:	3c 8f f2 7e                                     	rolwps \$r60 = \$r60, \$r60;;

    11bc:	fd 41 f6 7e                                     	rolwps \$r61 = \$r61, 7 \(0x7\);;

    11c0:	be 7f f6 7e                                     	rolw \$r61 = \$r62, \$r62;;

    11c4:	ff 31 fa 7e                                     	rolw \$r62 = \$r63, 7 \(0x7\);;

    11c8:	3f 80 fe 7f                                     	rorwps \$r63 = \$r63, \$r0;;

    11cc:	c0 41 02 7f                                     	rorwps \$r0 = \$r0, 7 \(0x7\);;

    11d0:	41 70 06 7f                                     	rorw \$r1 = \$r1, \$r1;;

    11d4:	c2 31 0a 7f                                     	rorw \$r2 = \$r2, 7 \(0x7\);;

    11d8:	02 07 c8 0f                                     	rswap \$r2 = \$mmc;;

    11dc:	03 00 c8 0f                                     	rswap \$r3 = \$pc;;

    11e0:	03 00 c8 0f                                     	rswap \$r3 = \$pc;;

    11e4:	04 a1 0e 71                                     	sbfbo \$r3 = \$r4, \$r4;;

    11e8:	c5 af 12 f1 ff ff ff 00                         	sbfbo.@ \$r4 = \$r5, 536870911 \(0x1fffffff\);;

    11f0:	85 91 15 7f                                     	sbfcd.i \$r5 = \$r5, \$r6;;

    11f4:	c6 97 19 ff ff ff ff 00                         	sbfcd.i \$r6 = \$r6, 536870911 \(0x1fffffff\);;

    11fc:	c7 81 1d 7f                                     	sbfcd \$r7 = \$r7, \$r7;;

    1200:	c8 87 21 ff ff ff ff 00                         	sbfcd \$r8 = \$r8, 536870911 \(0x1fffffff\);;

    1208:	c9 ff 20 e3 ff ff ff 87 ff ff ff 00             	sbfd \$r8 = \$r9, 2305843009213693951 \(0x1fffffffffffffff\);;

    1214:	89 02 25 73                                     	sbfd \$r9 = \$r9, \$r10;;

    1218:	0a f0 28 63                                     	sbfd \$r10 = \$r10, -64 \(0xffffffc0\);;

    121c:	0b 00 2c e3 00 00 80 07                         	sbfd \$r11 = \$r11, -8589934592 \(0xfffffffe00000000\);;

    1224:	cc 0f 2d f3 ff ff ff 00                         	sbfd.@ \$r11 = \$r12, 536870911 \(0x1fffffff\);;

    122c:	4d 33 31 73                                     	sbfhq \$r12 = \$r13, \$r13;;

    1230:	ce 37 39 f3 ff ff ff 00                         	sbfhq \$r14 = \$r14, 536870911 \(0x1fffffff\);;

    1238:	0f b4 3e 7d                                     	sbfsbo \$r15 = \$r15, \$r16;;

    123c:	d0 bf 42 fd ff ff ff 00                         	sbfsbo.@ \$r16 = \$r16, 536870911 \(0x1fffffff\);;

    1244:	51 44 45 7d                                     	sbfsd \$r17 = \$r17, \$r17;;

    1248:	d2 47 49 fd ff ff ff 00                         	sbfsd \$r18 = \$r18, 536870911 \(0x1fffffff\);;

    1250:	d3 74 49 7d                                     	sbfshq \$r18 = \$r19, \$r19;;

    1254:	d4 7f 4d fd ff ff ff 00                         	sbfshq.@ \$r19 = \$r20, 536870911 \(0x1fffffff\);;

    125c:	54 65 51 7d                                     	sbfswp \$r20 = \$r20, \$r21;;

    1260:	d5 67 55 fd ff ff ff 00                         	sbfswp \$r21 = \$r21, 536870911 \(0x1fffffff\);;

    1268:	96 55 59 7d                                     	sbfsw \$r22 = \$r22, \$r22;;

    126c:	d7 57 5d fd ff ff ff 00                         	sbfsw \$r23 = \$r23, 536870911 \(0x1fffffff\);;

    1274:	18 b6 5e 7f                                     	sbfusbo \$r23 = \$r24, \$r24;;

    1278:	d9 bf 62 ff ff ff ff 00                         	sbfusbo.@ \$r24 = \$r25, 536870911 \(0x1fffffff\);;

    1280:	99 46 65 7f                                     	sbfusd \$r25 = \$r25, \$r26;;

    1284:	da 47 69 ff ff ff ff 00                         	sbfusd \$r26 = \$r26, 536870911 \(0x1fffffff\);;

    128c:	db 76 6d 7f                                     	sbfushq \$r27 = \$r27, \$r27;;

    1290:	dc 7f 71 ff ff ff ff 00                         	sbfushq.@ \$r28 = \$r28, 536870911 \(0x1fffffff\);;

    1298:	5d 67 71 7f                                     	sbfuswp \$r28 = \$r29, \$r29;;

    129c:	de 67 75 ff ff ff ff 00                         	sbfuswp \$r29 = \$r30, 536870911 \(0x1fffffff\);;

    12a4:	de 57 79 7f                                     	sbfusw \$r30 = \$r30, \$r31;;

    12a8:	df 57 7d ff ff ff ff 00                         	sbfusw \$r31 = \$r31, 536870911 \(0x1fffffff\);;

    12b0:	20 98 81 7d                                     	sbfuwd \$r32 = \$r32, \$r32;;

    12b4:	e1 97 85 fd ff ff ff 00                         	sbfuwd \$r33 = \$r33, 536870911 \(0x1fffffff\);;

    12bc:	a2 88 85 7d                                     	sbfwd \$r33 = \$r34, \$r34;;

    12c0:	e3 87 89 fd ff ff ff 00                         	sbfwd \$r34 = \$r35, 536870911 \(0x1fffffff\);;

    12c8:	23 29 8d 73                                     	sbfwp \$r35 = \$r35, \$r36;;

    12cc:	e4 2f 91 f3 ff ff ff 00                         	sbfwp.@ \$r36 = \$r36, 536870911 \(0x1fffffff\);;

    12d4:	65 19 95 73                                     	sbfw \$r37 = \$r37, \$r37;;

    12d8:	26 f0 98 73                                     	sbfw \$r38 = \$r38, -64 \(0xffffffc0\);;

    12dc:	27 00 98 f3 00 00 80 07                         	sbfw \$r38 = \$r39, -8589934592 \(0xfffffffe00000000\);;

    12e4:	27 ba 9e 77                                     	sbfx16bo \$r39 = \$r39, \$r40;;

    12e8:	e8 b7 a2 f7 ff ff ff 00                         	sbfx16bo \$r40 = \$r40, 536870911 \(0x1fffffff\);;

    12f0:	69 4a a5 77                                     	sbfx16d \$r41 = \$r41, \$r41;;

    12f4:	ea 4f a9 f7 ff ff ff 00                         	sbfx16d.@ \$r42 = \$r42, 536870911 \(0x1fffffff\);;

    12fc:	eb 7a a9 77                                     	sbfx16hq \$r42 = \$r43, \$r43;;

    1300:	ec 77 ad f7 ff ff ff 00                         	sbfx16hq \$r43 = \$r44, 536870911 \(0x1fffffff\);;

    1308:	6c 9b b1 77                                     	sbfx16uwd \$r44 = \$r44, \$r45;;

    130c:	ed 97 b5 f7 ff ff ff 00                         	sbfx16uwd \$r45 = \$r45, 536870911 \(0x1fffffff\);;

    1314:	ae 8b b9 77                                     	sbfx16wd \$r46 = \$r46, \$r46;;

    1318:	ef 87 bd f7 ff ff ff 00                         	sbfx16wd \$r47 = \$r47, 536870911 \(0x1fffffff\);;

    1320:	30 6c bd 77                                     	sbfx16wp \$r47 = \$r48, \$r48;;

    1324:	f1 6f c1 f7 ff ff ff 00                         	sbfx16wp.@ \$r48 = \$r49, 536870911 \(0x1fffffff\);;

    132c:	b1 5c c5 77                                     	sbfx16w \$r49 = \$r49, \$r50;;

    1330:	f2 57 c9 f7 ff ff ff 00                         	sbfx16w \$r50 = \$r50, 536870911 \(0x1fffffff\);;

    1338:	f3 bc ce 71                                     	sbfx2bo \$r51 = \$r51, \$r51;;

    133c:	f4 b7 d2 f1 ff ff ff 00                         	sbfx2bo \$r52 = \$r52, 536870911 \(0x1fffffff\);;

    1344:	75 4d d1 71                                     	sbfx2d \$r52 = \$r53, \$r53;;

    1348:	f6 4f d5 f1 ff ff ff 00                         	sbfx2d.@ \$r53 = \$r54, 536870911 \(0x1fffffff\);;

    1350:	f6 7d d9 71                                     	sbfx2hq \$r54 = \$r54, \$r55;;

    1354:	f7 77 dd f1 ff ff ff 00                         	sbfx2hq \$r55 = \$r55, 536870911 \(0x1fffffff\);;

    135c:	38 9e e1 71                                     	sbfx2uwd \$r56 = \$r56, \$r56;;

    1360:	f9 97 e5 f1 ff ff ff 00                         	sbfx2uwd \$r57 = \$r57, 536870911 \(0x1fffffff\);;

    1368:	ba 8e e5 71                                     	sbfx2wd \$r57 = \$r58, \$r58;;

    136c:	fb 87 e9 f1 ff ff ff 00                         	sbfx2wd \$r58 = \$r59, 536870911 \(0x1fffffff\);;

    1374:	3b 6f ed 71                                     	sbfx2wp \$r59 = \$r59, \$r60;;

    1378:	fc 6f f1 f1 ff ff ff 00                         	sbfx2wp.@ \$r60 = \$r60, 536870911 \(0x1fffffff\);;

    1380:	7d 5f f5 71                                     	sbfx2w \$r61 = \$r61, \$r61;;

    1384:	fe 57 f9 f1 ff ff ff 00                         	sbfx2w \$r62 = \$r62, 536870911 \(0x1fffffff\);;

    138c:	ff 4f f9 79                                     	sbfx32d \$r62 = \$r63, \$r63;;

    1390:	c0 47 fd f9 ff ff ff 00                         	sbfx32d \$r63 = \$r0, 536870911 \(0x1fffffff\);;

    1398:	40 90 01 79                                     	sbfx32uwd \$r0 = \$r0, \$r1;;

    139c:	c1 97 05 f9 ff ff ff 00                         	sbfx32uwd \$r1 = \$r1, 536870911 \(0x1fffffff\);;

    13a4:	82 80 09 79                                     	sbfx32wd \$r2 = \$r2, \$r2;;

    13a8:	c3 87 0d f9 ff ff ff 00                         	sbfx32wd \$r3 = \$r3, 536870911 \(0x1fffffff\);;

    13b0:	04 51 0d 79                                     	sbfx32w \$r3 = \$r4, \$r4;;

    13b4:	c5 57 11 f9 ff ff ff 00                         	sbfx32w \$r4 = \$r5, 536870911 \(0x1fffffff\);;

    13bc:	85 b1 16 73                                     	sbfx4bo \$r5 = \$r5, \$r6;;

    13c0:	c6 bf 1a f3 ff ff ff 00                         	sbfx4bo.@ \$r6 = \$r6, 536870911 \(0x1fffffff\);;

    13c8:	c7 41 1d 73                                     	sbfx4d \$r7 = \$r7, \$r7;;

    13cc:	c8 47 21 f3 ff ff ff 00                         	sbfx4d \$r8 = \$r8, 536870911 \(0x1fffffff\);;

    13d4:	49 72 21 73                                     	sbfx4hq \$r8 = \$r9, \$r9;;

    13d8:	ca 7f 25 f3 ff ff ff 00                         	sbfx4hq.@ \$r9 = \$r10, 536870911 \(0x1fffffff\);;

    13e0:	ca 92 29 73                                     	sbfx4uwd \$r10 = \$r10, \$r11;;

    13e4:	cb 97 2d f3 ff ff ff 00                         	sbfx4uwd \$r11 = \$r11, 536870911 \(0x1fffffff\);;

    13ec:	4c 83 31 73                                     	sbfx4wd \$r12 = \$r12, \$r13;;

    13f0:	ce 87 35 f3 ff ff ff 00                         	sbfx4wd \$r13 = \$r14, 536870911 \(0x1fffffff\);;

    13f8:	cf 63 39 73                                     	sbfx4wp \$r14 = \$r15, \$r15;;

    13fc:	d0 67 41 f3 ff ff ff 00                         	sbfx4wp \$r16 = \$r16, 536870911 \(0x1fffffff\);;

    1404:	51 54 41 73                                     	sbfx4w \$r16 = \$r17, \$r17;;

    1408:	d2 57 45 f3 ff ff ff 00                         	sbfx4w \$r17 = \$r18, 536870911 \(0x1fffffff\);;

    1410:	d2 44 49 7b                                     	sbfx64d \$r18 = \$r18, \$r19;;

    1414:	d3 4f 4d fb ff ff ff 00                         	sbfx64d.@ \$r19 = \$r19, 536870911 \(0x1fffffff\);;

    141c:	14 95 51 7b                                     	sbfx64uwd \$r20 = \$r20, \$r20;;

    1420:	d5 97 55 fb ff ff ff 00                         	sbfx64uwd \$r21 = \$r21, 536870911 \(0x1fffffff\);;

    1428:	96 85 55 7b                                     	sbfx64wd \$r21 = \$r22, \$r22;;

    142c:	d7 87 59 fb ff ff ff 00                         	sbfx64wd \$r22 = \$r23, 536870911 \(0x1fffffff\);;

    1434:	17 56 5d 7b                                     	sbfx64w \$r23 = \$r23, \$r24;;

    1438:	d8 57 61 fb ff ff ff 00                         	sbfx64w \$r24 = \$r24, 536870911 \(0x1fffffff\);;

    1440:	59 b6 66 75                                     	sbfx8bo \$r25 = \$r25, \$r25;;

    1444:	da b7 6a f5 ff ff ff 00                         	sbfx8bo \$r26 = \$r26, 536870911 \(0x1fffffff\);;

    144c:	db 46 69 75                                     	sbfx8d \$r26 = \$r27, \$r27;;

    1450:	dc 4f 6d f5 ff ff ff 00                         	sbfx8d.@ \$r27 = \$r28, 536870911 \(0x1fffffff\);;

    1458:	5c 77 71 75                                     	sbfx8hq \$r28 = \$r28, \$r29;;

    145c:	dd 77 75 f5 ff ff ff 00                         	sbfx8hq \$r29 = \$r29, 536870911 \(0x1fffffff\);;

    1464:	9e 97 79 75                                     	sbfx8uwd \$r30 = \$r30, \$r30;;

    1468:	df 97 7d f5 ff ff ff 00                         	sbfx8uwd \$r31 = \$r31, 536870911 \(0x1fffffff\);;

    1470:	20 88 7d 75                                     	sbfx8wd \$r31 = \$r32, \$r32;;

    1474:	e1 87 81 f5 ff ff ff 00                         	sbfx8wd \$r32 = \$r33, 536870911 \(0x1fffffff\);;

    147c:	a1 68 85 75                                     	sbfx8wp \$r33 = \$r33, \$r34;;

    1480:	e2 6f 89 f5 ff ff ff 00                         	sbfx8wp.@ \$r34 = \$r34, 536870911 \(0x1fffffff\);;

    1488:	e3 58 8d 75                                     	sbfx8w \$r35 = \$r35, \$r35;;

    148c:	e4 57 91 f5 ff ff ff 00                         	sbfx8w \$r36 = \$r36, 536870911 \(0x1fffffff\);;

    1494:	e5 ff 92 ee ff ff ff 87 ff ff ff 00             	sbmm8 \$r36 = \$r37, 2305843009213693951 \(0x1fffffffffffffff\);;

    14a0:	a5 09 96 7e                                     	sbmm8 \$r37 = \$r37, \$r38;;

    14a4:	26 f0 9a 6e                                     	sbmm8 \$r38 = \$r38, -64 \(0xffffffc0\);;

    14a8:	27 00 9e ee 00 00 80 07                         	sbmm8 \$r39 = \$r39, -8589934592 \(0xfffffffe00000000\);;

    14b0:	e8 0f 9e fe ff ff ff 00                         	sbmm8.@ \$r39 = \$r40, 536870911 \(0x1fffffff\);;

    14b8:	e8 ff a2 ef ff ff ff 87 ff ff ff 00             	sbmmt8 \$r40 = \$r40, 2305843009213693951 \(0x1fffffffffffffff\);;

    14c4:	69 0a a6 7f                                     	sbmmt8 \$r41 = \$r41, \$r41;;

    14c8:	2a f0 aa 6f                                     	sbmmt8 \$r42 = \$r42, -64 \(0xffffffc0\);;

    14cc:	2b 00 aa ef 00 00 80 07                         	sbmmt8 \$r42 = \$r43, -8589934592 \(0xfffffffe00000000\);;

    14d4:	eb 0f ae ff ff ff ff 00                         	sbmmt8.@ \$r43 = \$r43, 536870911 \(0x1fffffff\);;

    14dc:	2c eb b3 30                                     	sb \$r44\[\$r44\] = \$r44;;

    14e0:	ed ff b5 b0 ff ff ff 9f ff ff ff 18             	sb 2305843009213693951 \(0x1fffffffffffffff\)\[\$r45\] = \$r45;;

    14ec:	6e 4b bb b0 00 00 00 98 00 00 80 1f             	sb.dlez \$r45\? -1125899906842624 \(0xfffc000000000000\)\[\$r46\] = \$r46;;

    14f8:	af 5b bf b0 00 00 80 1f                         	sb.dgtz \$r46\? -8388608 \(0xff800000\)\[\$r47\] = \$r47;;

    1500:	f0 6b c3 30                                     	sb.odd \$r47\? \[\$r48\] = \$r48;;

    1504:	30 f0 c5 30                                     	sb -64 \(0xffffffc0\)\[\$r48\] = \$r49;;

    1508:	31 00 c5 b0 00 00 80 1f                         	sb -8589934592 \(0xfffffffe00000000\)\[\$r49\] = \$r49;;

    1510:	32 00 e4 0f                                     	scall \$r50;;

    1514:	ff 01 e0 0f                                     	scall 511 \(0x1ff\);;

    1518:	b2 fc cf 33                                     	sd.xs \$r50\[\$r50\] = \$r51;;

    151c:	f3 ff cd b3 ff ff ff 9f ff ff ff 18             	sd 2305843009213693951 \(0x1fffffffffffffff\)\[\$r51\] = \$r51;;

    1528:	34 7d d3 b3 00 00 00 98 00 00 80 1f             	sd.even \$r52\? -1125899906842624 \(0xfffc000000000000\)\[\$r52\] = \$r52;;

    1534:	75 8d d7 b3 00 00 80 1f                         	sd.wnez \$r53\? -8388608 \(0xff800000\)\[\$r53\] = \$r53;;

    153c:	b6 9d db 33                                     	sd.weqz \$r54\? \[\$r54\] = \$r54;;

    1540:	37 f0 dd 33                                     	sd -64 \(0xffffffc0\)\[\$r55\] = \$r55;;

    1544:	37 00 e1 b3 00 00 80 1f                         	sd -8589934592 \(0xfffffffe00000000\)\[\$r55\] = \$r56;;

    154c:	38 07 c0 0f                                     	set \$mmc = \$r56;;

    1550:	f8 00 c0 0f                                     	set \$ra = \$r56;;

    1554:	79 00 c0 0f                                     	set \$ps = \$r57;;

    1558:	79 00 c0 0f                                     	set \$ps = \$r57;;

    155c:	7a ee eb 31                                     	sh \$r57\[\$r58\] = \$r58;;

    1560:	fa ff ed b1 ff ff ff 9f ff ff ff 18             	sh 2305843009213693951 \(0x1fffffffffffffff\)\[\$r58\] = \$r59;;

    156c:	fb ae f3 b1 00 00 00 98 00 00 80 1f             	sh.wltz \$r59\? -1125899906842624 \(0xfffc000000000000\)\[\$r59\] = \$r60;;

    1578:	3c bf f7 b1 00 00 80 1f                         	sh.wgez \$r60\? -8388608 \(0xff800000\)\[\$r60\] = \$r61;;

    1580:	7d cf fb 31                                     	sh.wlez \$r61\? \[\$r61\] = \$r62;;

    1584:	3e f0 f9 31                                     	sh -64 \(0xffffffc0\)\[\$r62\] = \$r62;;

    1588:	3f 00 fd b1 00 00 80 1f                         	sh -8589934592 \(0xfffffffe00000000\)\[\$r63\] = \$r63;;

    1590:	00 00 a4 0f                                     	sleep;;

    1594:	00 f0 fe 79                                     	sllbos \$r63 = \$r0, \$r0;;

    1598:	c1 e1 02 79                                     	sllbos \$r0 = \$r1, 7 \(0x7\);;

    159c:	81 60 06 79                                     	slld \$r1 = \$r1, \$r2;;

    15a0:	c2 21 0a 79                                     	slld \$r2 = \$r2, 7 \(0x7\);;

    15a4:	c3 90 0e 79                                     	sllhqs \$r3 = \$r3, \$r3;;

    15a8:	c4 51 12 79                                     	sllhqs \$r4 = \$r4, 7 \(0x7\);;

    15ac:	45 81 12 79                                     	sllwps \$r4 = \$r5, \$r5;;

    15b0:	c6 41 16 79                                     	sllwps \$r5 = \$r6, 7 \(0x7\);;

    15b4:	c6 71 1a 79                                     	sllw \$r6 = \$r6, \$r7;;

    15b8:	c7 31 1e 79                                     	sllw \$r7 = \$r7, 7 \(0x7\);;

    15bc:	08 f2 22 7c                                     	slsbos \$r8 = \$r8, \$r8;;

    15c0:	c9 e1 26 7c                                     	slsbos \$r9 = \$r9, 7 \(0x7\);;

    15c4:	8a 62 26 7c                                     	slsd \$r9 = \$r10, \$r10;;

    15c8:	cb 21 2a 7c                                     	slsd \$r10 = \$r11, 7 \(0x7\);;

    15cc:	0b 93 2e 7c                                     	slshqs \$r11 = \$r11, \$r12;;

    15d0:	cd 51 32 7c                                     	slshqs \$r12 = \$r13, 7 \(0x7\);;

    15d4:	8e 83 36 7c                                     	slswps \$r13 = \$r14, \$r14;;

    15d8:	cf 41 3e 7c                                     	slswps \$r15 = \$r15, 7 \(0x7\);;

    15dc:	10 74 42 7c                                     	slsw \$r16 = \$r16, \$r16;;

    15e0:	d1 31 46 7c                                     	slsw \$r17 = \$r17, 7 \(0x7\);;

    15e4:	92 f4 46 7d                                     	slusbos \$r17 = \$r18, \$r18;;

    15e8:	d3 e1 4a 7d                                     	slusbos \$r18 = \$r19, 7 \(0x7\);;

    15ec:	13 65 4e 7d                                     	slusd \$r19 = \$r19, \$r20;;

    15f0:	d4 21 52 7d                                     	slusd \$r20 = \$r20, 7 \(0x7\);;

    15f4:	55 95 56 7d                                     	slushqs \$r21 = \$r21, \$r21;;

    15f8:	d6 51 5a 7d                                     	slushqs \$r22 = \$r22, 7 \(0x7\);;

    15fc:	d7 85 5a 7d                                     	sluswps \$r22 = \$r23, \$r23;;

    1600:	d8 41 5e 7d                                     	sluswps \$r23 = \$r24, 7 \(0x7\);;

    1604:	58 76 62 7d                                     	slusw \$r24 = \$r24, \$r25;;

    1608:	d9 31 66 7d                                     	slusw \$r25 = \$r25, 7 \(0x7\);;

    160c:	9a f6 b7 34                                     	so.xs \$r26\[\$r26\] = \$r44r45r46r47;;

    1610:	da ff c5 b4 ff ff ff 9f ff ff ff 18             	so 2305843009213693951 \(0x1fffffffffffffff\)\[\$r26\] = \$r48r49r50r51;;

    161c:	db 36 df b4 00 00 00 98 00 00 80 1f             	so.u3 \$r27\? -1125899906842624 \(0xfffc000000000000\)\[\$r27\] = \$r52r53r54r55;;

    1628:	dc 46 ef b4 00 00 80 1f                         	so.mt \$r27\? -8388608 \(0xff800000\)\[\$r28\] = \$r56r57r58r59;;

    1630:	1c 57 ff 34                                     	so.mf \$r28\? \[\$r28\] = \$r60r61r62r63;;

    1634:	5d d7 07 b4 00 00 00 98 00 00 80 1f             	so.wgtz \$r29\? -1125899906842624 \(0xfffc000000000000\)\[\$r29\] = \$r0r1r2r3;;

    1640:	5e 07 17 b4 00 00 80 1f                         	so.dnez \$r29\? -8388608 \(0xff800000\)\[\$r30\] = \$r4r5r6r7;;

    1648:	9e 17 27 34                                     	so.deqz \$r30\? \[\$r30\] = \$r8r9r10r11;;

    164c:	1f f0 35 34                                     	so -64 \(0xffffffc0\)\[\$r31\] = \$r12r13r14r15;;

    1650:	1f 00 45 b4 00 00 80 1f                         	so -8589934592 \(0xfffffffe00000000\)\[\$r31\] = \$r16r17r18r19;;

    1658:	e0 e7 73 34                                     	sq \$r31\[\$r32\] = \$r28r29;;

    165c:	e0 ff 79 b4 ff ff ff 9f ff ff ff 18             	sq 2305843009213693951 \(0x1fffffffffffffff\)\[\$r32\] = \$r30r31;;

    1668:	21 28 7b b4 00 00 00 98 00 00 80 1f             	sq.dltz \$r32\? -1125899906842624 \(0xfffc000000000000\)\[\$r33\] = \$r30r31;;

    1674:	61 38 83 b4 00 00 80 1f                         	sq.dgez \$r33\? -8388608 \(0xff800000\)\[\$r33\] = \$r32r33;;

    167c:	a2 48 83 34                                     	sq.dlez \$r34\? \[\$r34\] = \$r32r33;;

    1680:	22 f0 89 34                                     	sq -64 \(0xffffffc0\)\[\$r34\] = \$r34r35;;

    1684:	23 00 89 b4 00 00 80 1f                         	sq -8589934592 \(0xfffffffe00000000\)\[\$r35\] = \$r34r35;;

    168c:	23 f9 8e 7a                                     	srabos \$r35 = \$r35, \$r36;;

    1690:	e4 e1 92 7a                                     	srabos \$r36 = \$r36, 7 \(0x7\);;

    1694:	65 69 96 7a                                     	srad \$r37 = \$r37, \$r37;;

    1698:	e6 21 9a 7a                                     	srad \$r38 = \$r38, 7 \(0x7\);;

    169c:	e7 99 9a 7a                                     	srahqs \$r38 = \$r39, \$r39;;

    16a0:	e8 51 9e 7a                                     	srahqs \$r39 = \$r40, 7 \(0x7\);;

    16a4:	68 8a a2 7a                                     	srawps \$r40 = \$r40, \$r41;;

    16a8:	e9 41 a6 7a                                     	srawps \$r41 = \$r41, 7 \(0x7\);;

    16ac:	aa 7a aa 7a                                     	sraw \$r42 = \$r42, \$r42;;

    16b0:	eb 31 ae 7a                                     	sraw \$r43 = \$r43, 7 \(0x7\);;

    16b4:	2c fb ae 7b                                     	srlbos \$r43 = \$r44, \$r44;;

    16b8:	ed e1 b2 7b                                     	srlbos \$r44 = \$r45, 7 \(0x7\);;

    16bc:	ad 6b b6 7b                                     	srld \$r45 = \$r45, \$r46;;

    16c0:	ee 21 ba 7b                                     	srld \$r46 = \$r46, 7 \(0x7\);;

    16c4:	ef 9b be 7b                                     	srlhqs \$r47 = \$r47, \$r47;;

    16c8:	f0 51 c2 7b                                     	srlhqs \$r48 = \$r48, 7 \(0x7\);;

    16cc:	71 8c c2 7b                                     	srlwps \$r48 = \$r49, \$r49;;

    16d0:	f2 41 c6 7b                                     	srlwps \$r49 = \$r50, 7 \(0x7\);;

    16d4:	f2 7c ca 7b                                     	srlw \$r50 = \$r50, \$r51;;

    16d8:	f3 31 ce 7b                                     	srlw \$r51 = \$r51, 7 \(0x7\);;

    16dc:	34 fd d2 78                                     	srsbos \$r52 = \$r52, \$r52;;

    16e0:	f5 e1 d6 78                                     	srsbos \$r53 = \$r53, 7 \(0x7\);;

    16e4:	b6 6d d6 78                                     	srsd \$r53 = \$r54, \$r54;;

    16e8:	f7 21 da 78                                     	srsd \$r54 = \$r55, 7 \(0x7\);;

    16ec:	37 9e de 78                                     	srshqs \$r55 = \$r55, \$r56;;

    16f0:	f8 51 e2 78                                     	srshqs \$r56 = \$r56, 7 \(0x7\);;

    16f4:	79 8e e6 78                                     	srswps \$r57 = \$r57, \$r57;;

    16f8:	fa 41 ea 78                                     	srswps \$r58 = \$r58, 7 \(0x7\);;

    16fc:	fb 7e ea 78                                     	srsw \$r58 = \$r59, \$r59;;

    1700:	fc 31 ee 78                                     	srsw \$r59 = \$r60, 7 \(0x7\);;

    1704:	00 00 a8 0f                                     	stop;;

    1708:	7c af f1 7e                                     	stsud \$r60 = \$r60, \$r61;;

    170c:	fd a7 f5 fe ff ff ff 00                         	stsud \$r61 = \$r61, 536870911 \(0x1fffffff\);;

    1714:	be ff f9 7e                                     	stsuhq \$r62 = \$r62, \$r62;;

    1718:	ff ff fd fe ff ff ff 00                         	stsuhq.@ \$r63 = \$r63, 536870911 \(0x1fffffff\);;

    1720:	00 e0 fd 7e                                     	stsuwp \$r63 = \$r0, \$r0;;

    1724:	c1 e7 01 fe ff ff ff 00                         	stsuwp \$r0 = \$r1, 536870911 \(0x1fffffff\);;

    172c:	81 b0 05 7e                                     	stsuw \$r1 = \$r1, \$r2;;

    1730:	c2 b7 09 fe ff ff ff 00                         	stsuw \$r2 = \$r2, 536870911 \(0x1fffffff\);;

    1738:	c3 f0 0f 32                                     	sw.xs \$r3\[\$r3\] = \$r3;;

    173c:	c4 ff 11 b2 ff ff ff 9f ff ff ff 18             	sw 2305843009213693951 \(0x1fffffffffffffff\)\[\$r4\] = \$r4;;

    1748:	05 51 17 b2 00 00 00 98 00 00 80 1f             	sw.dgtz \$r4\? -1125899906842624 \(0xfffc000000000000\)\[\$r5\] = \$r5;;

    1754:	46 61 1b b2 00 00 80 1f                         	sw.odd \$r5\? -8388608 \(0xff800000\)\[\$r6\] = \$r6;;

    175c:	87 71 1f 32                                     	sw.even \$r6\? \[\$r7\] = \$r7;;

    1760:	07 f0 21 32                                     	sw -64 \(0xffffffc0\)\[\$r7\] = \$r8;;

    1764:	08 00 21 b2 00 00 80 1f                         	sw -8589934592 \(0xfffffffe00000000\)\[\$r8\] = \$r8;;

    176c:	09 70 27 68                                     	sxbd \$r9 = \$r9;;

    1770:	0a f0 27 68                                     	sxhd \$r9 = \$r10;;

    1774:	0a 50 2a 76                                     	sxlbhq \$r10 = \$r10;;

    1778:	0b 40 2e 76                                     	sxlhwp \$r11 = \$r11;;

    177c:	0c 50 2e 77                                     	sxmbhq \$r11 = \$r12;;

    1780:	0d 40 32 77                                     	sxmhwp \$r12 = \$r13;;

    1784:	0e f0 37 69                                     	sxwd \$r13 = \$r14;;

    1788:	0e 00 b4 0f                                     	syncgroup \$r14;;

    178c:	00 00 8c 0f                                     	tlbdinval;;

    1790:	00 00 90 0f                                     	tlbiinval;;

    1794:	00 00 84 0f                                     	tlbprobe;;

    1798:	00 00 80 0f                                     	tlbread;;

    179c:	00 00 88 0f                                     	tlbwrite;;

    17a0:	0f 00 b0 0f                                     	waitit \$r15;;

    17a4:	4f 00 b8 0f                                     	wfxl \$ps, \$r15;;

    17a8:	90 00 b8 0f                                     	wfxl \$pcr, \$r16;;

    17ac:	50 00 b8 0f                                     	wfxl \$ps, \$r16;;

    17b0:	50 00 bc 0f                                     	wfxm \$ps, \$r16;;

    17b4:	91 00 bc 0f                                     	wfxm \$pcr, \$r17;;

    17b8:	91 00 bc 0f                                     	wfxm \$pcr, \$r17;;

    17bc:	11 80 5c 00                                     	xaccesso \$r20r21r22r23 = \$a0..a1, \$r17;;

    17c0:	52 80 6c 00                                     	xaccesso \$r24r25r26r27 = \$a0..a3, \$r18;;

    17c4:	d2 80 7c 00                                     	xaccesso \$r28r29r30r31 = \$a0..a7, \$r18;;

    17c8:	d2 81 8c 00                                     	xaccesso \$r32r33r34r35 = \$a0..a15, \$r18;;

    17cc:	d3 83 9c 00                                     	xaccesso \$r36r37r38r39 = \$a0..a31, \$r19;;

    17d0:	d3 87 ac 00                                     	xaccesso \$r40r41r42r43 = \$a0..a63, \$r19;;

    17d4:	93 80 00 01                                     	xaligno \$a0 = \$a2..a3, \$r19;;

    17d8:	54 81 00 01                                     	xaligno \$a0 = \$a4..a7, \$r20;;

    17dc:	d4 82 00 01                                     	xaligno \$a0 = \$a8..a15, \$r20;;

    17e0:	d4 85 04 01                                     	xaligno \$a1 = \$a16..a31, \$r20;;

    17e4:	d5 8b 04 01                                     	xaligno \$a1 = \$a32..a63, \$r21;;

    17e8:	d5 87 04 01                                     	xaligno \$a1 = \$a0..a63, \$r21;;

    17ec:	82 60 0b 07                                     	xandno \$a2 = \$a2, \$a2;;

    17f0:	c3 00 0f 07                                     	xando \$a3 = \$a3, \$a3;;

    17f4:	04 01 13 05                                     	xclampwo \$a4 = \$a4, \$a4;;

    17f8:	40 01 14 01                                     	xcopyo \$a5 = \$a5;;

    17fc:	00 01 05 07                                     	xcopyv \$a0a1a2a3 = \$a4a5a6a7;;

    1800:	00 00 04 07                                     	xcopyx \$a0a1 = \$a0a1;;

    1804:	46 c1 0a 04                                     	xffma44hw.rna.s \$a2a3 = \$a5, \$a6;;

    1808:	87 01 1a 05                                     	xfmaxhx \$a6 = \$a6, \$a7;;

    180c:	c8 01 1d 05                                     	xfminhx \$a7 = \$a7, \$a8;;

    1810:	04 51 0b 04                                     	xfmma484hw.rnz \$a2a3 = \$a4a5, \$a4a5;;

    1814:	80 e1 20 05                                     	xfnarrow44wh.ro.s \$a8 = \$a6a7;;

    1818:	55 72 23 01                                     	xfscalewo \$a8 = \$a9, \$r21;;

    181c:	96 e5 23 2a                                     	xlo.u.q0 \$a8a9a10a11 = \$r22\[\$r22\];;

    1820:	97 f5 27 23                                     	xlo.us.xs \$a9 = \$r22\[\$r23\];;

    1824:	d7 05 37 a8 00 00 00 98 00 00 80 1f             	xlo.dnez.q1 \$r23\? \$a12a13a14a15 = -1125899906842624 \(0xfffc000000000000\)\[\$r23\];;

    1830:	18 16 4b a9 00 00 80 1f                         	xlo.s.deqz.q2 \$r24\? \$a16a17a18a19 = -8388608 \(0xff800000\)\[\$r24\];;

    1838:	19 26 5f 2a                                     	xlo.u.wnez.q3 \$r24\? \$a20a21a22a23 = \[\$r25\];;

    183c:	59 36 27 a3 00 00 00 98 00 00 80 1f             	xlo.us.weqz \$r25\? \$a9 = -1125899906842624 \(0xfffc000000000000\)\[\$r25\];;

    1848:	9a 46 2b a0 00 00 80 1f                         	xlo.mt \$r26\? \$a10 = -8388608 \(0xff800000\)\[\$r26\];;

    1850:	9b 56 2b 21                                     	xlo.s.mf \$r26\? \$a10 = \[\$r27\];;

    1854:	db 06 13 ae 00 00 00 98 00 00 80 1f             	xlo.u \$a4..a5, \$r27 = -1125899906842624 \(0xfffc000000000000\)\[\$r27\];;

    1860:	1c 17 1b af 00 00 80 1f                         	xlo.us.q \$a6..a7, \$r28 = -8388608 \(0xff800000\)\[\$r28\];;

    1868:	1d 27 23 2c                                     	xlo.d \$a8..a9, \$r28 = \[\$r29\];;

    186c:	5d 37 27 ad 00 00 00 98 00 00 80 1f             	xlo.s.w \$a8..a11, \$r29 = -1125899906842624 \(0xfffc000000000000\)\[\$r29\];;

    1878:	9e 47 37 ae 00 00 80 1f                         	xlo.u.h \$a12..a15, \$r30 = -8388608 \(0xff800000\)\[\$r30\];;

    1880:	9f 57 47 2f                                     	xlo.us.b \$a16..a19, \$r30 = \[\$r31\];;

    1884:	df 07 4f ac 00 00 00 98 00 00 80 1f             	xlo \$a16..a23, \$r31 = -1125899906842624 \(0xfffc000000000000\)\[\$r31\];;

    1890:	20 18 6f ad 00 00 80 1f                         	xlo.s.q \$a24..a31, \$r32 = -8388608 \(0xff800000\)\[\$r32\];;

    1898:	21 28 8f 2e                                     	xlo.u.d \$a32..a39, \$r32 = \[\$r33\];;

    189c:	61 38 9f af 00 00 00 98 00 00 80 1f             	xlo.us.w \$a32..a47, \$r33 = -1125899906842624 \(0xfffc000000000000\)\[\$r33\];;

    18a8:	a2 48 df ac 00 00 80 1f                         	xlo.h \$a48..a63, \$r34 = -8388608 \(0xff800000\)\[\$r34\];;

    18b0:	a3 58 1f 2d                                     	xlo.s.b \$a0..a15, \$r34 = \[\$r35\];;

    18b4:	e3 08 3f ae 00 00 00 98 00 00 80 1f             	xlo.u \$a0..a31, \$r35 = -1125899906842624 \(0xfffc000000000000\)\[\$r35\];;

    18c0:	24 19 bf af 00 00 80 1f                         	xlo.us.q \$a32..a63, \$r36 = -8388608 \(0xff800000\)\[\$r36\];;

    18c8:	25 29 3f 2c                                     	xlo.d \$a0..a31, \$r36 = \[\$r37\];;

    18cc:	65 39 7f ad 00 00 00 98 00 00 80 1f             	xlo.s.w \$a0..a63, \$r37 = -1125899906842624 \(0xfffc000000000000\)\[\$r37\];;

    18d8:	a6 49 7f ae 00 00 80 1f                         	xlo.u.h \$a0..a63, \$r38 = -8388608 \(0xff800000\)\[\$r38\];;

    18e0:	a7 59 7f 2f                                     	xlo.us.b \$a0..a63, \$r38 = \[\$r39\];;

    18e4:	e7 ff 61 a8 ff ff ff 9f ff ff ff 18             	xlo.q0 \$a24a25a26a27 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r39\];;

    18f0:	27 f0 75 29                                     	xlo.s.q1 \$a28a29a30a31 = -64 \(0xffffffc0\)\[\$r39\];;

    18f4:	28 00 89 aa 00 00 80 1f                         	xlo.u.q2 \$a32a33a34a35 = -8589934592 \(0xfffffffe00000000\)\[\$r40\];;

    18fc:	e8 ff 29 a3 ff ff ff 9f ff ff ff 18             	xlo.us \$a10 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r40\];;

    1908:	28 f0 2d 20                                     	xlo \$a11 = -64 \(0xffffffc0\)\[\$r40\];;

    190c:	29 00 2d a1 00 00 80 1f                         	xlo.s \$a11 = -8589934592 \(0xfffffffe00000000\)\[\$r41\];;

    1914:	cc 02 18 03                                     	xmadd44bw0 \$a6a7 = \$a11, \$a12;;

    1918:	0c 03 24 03                                     	xmadd44bw1 \$a8a9 = \$a12, \$a12;;

    191c:	4d 83 34 04                                     	xmaddifwo.rn.s \$a13 = \$a13, \$a13;;

    1920:	8e 03 22 03                                     	xmaddsu44bw0 \$a8a9 = \$a14, \$a14;;

    1924:	8f 03 2e 03                                     	xmaddsu44bw1 \$a10a11 = \$a14, \$a15;;

    1928:	cf 03 29 03                                     	xmaddu44bw0 \$a10a11 = \$a15, \$a15;;

    192c:	10 04 35 03                                     	xmaddu44bw1 \$a12a13 = \$a16, \$a16;;

    1930:	8e 03 30 02                                     	xmma4164bw \$a12a13 = \$a14a15, \$a14a15;;

    1934:	11 04 44 02                                     	xmma484bw \$a16a17 = \$a16, \$a17;;

    1938:	92 04 42 02                                     	xmmasu4164bw \$a16a17 = \$a18a19, \$a18a19;;

    193c:	51 04 56 02                                     	xmmasu484bw \$a20a21 = \$a17, \$a17;;

    1940:	96 05 51 02                                     	xmmau4164bw \$a20a21 = \$a22a23, \$a22a23;;

    1944:	92 04 65 02                                     	xmmau484bw \$a24a25 = \$a18, \$a18;;

    1948:	9a 06 63 02                                     	xmmaus4164bw \$a24a25 = \$a26a27, \$a26a27;;

    194c:	93 04 77 02                                     	xmmaus484bw \$a28a29 = \$a18, \$a19;;

    1950:	00 c0 a4 00                                     	xmovefd \$r41 = \$a0_x;;

    1954:	c0 84 b4 00                                     	xmovefo \$r44r45r46r47 = \$a19;;

    1958:	00 00 94 00                                     	xmovefq \$r36r37 = \$a0_lo;;

    195c:	29 e0 03 73                                     	xmovetd \$a0_t = \$r41;;

    1960:	2a e0 03 70                                     	xmovetd \$a0_x = \$r42;;

    1964:	2a e0 03 71                                     	xmovetd \$a0_y = \$r42;;

    1968:	2a e0 03 72                                     	xmovetd \$a0_z = \$r42;;

    196c:	eb ea 03 74                                     	xmovetq \$a0_lo = \$r43, \$r43;;

    1970:	2b eb 03 75                                     	xmovetq \$a0_hi = \$r43, \$r44;;

    1974:	14 15 4d 04                                     	xmsbfifwo.ru \$a19 = \$a20, \$a20;;

    1978:	00 1a 95 07                                     	xcopyv.td \$a36a37a38a39 = \$a40a41a42a43;;

    197c:	55 15 53 07                                     	xnando \$a20 = \$a21, \$a21;;

    1980:	96 35 57 07                                     	xnoro \$a21 = \$a22, \$a22;;

    1984:	d7 55 5b 07                                     	xnxoro \$a22 = \$a23, \$a23;;

    1988:	ec ff b0 ec ff ff ff 87 ff ff ff 00             	xord \$r44 = \$r44, 2305843009213693951 \(0x1fffffffffffffff\);;

    1994:	6d 0b b5 7c                                     	xord \$r45 = \$r45, \$r45;;

    1998:	2e f0 b8 6c                                     	xord \$r46 = \$r46, -64 \(0xffffffc0\);;

    199c:	2f 00 b8 ec 00 00 80 07                         	xord \$r46 = \$r47, -8589934592 \(0xfffffffe00000000\);;

    19a4:	ef 0f bd fc ff ff ff 00                         	xord.@ \$r47 = \$r47, 536870911 \(0x1fffffff\);;

    19ac:	18 76 5f 07                                     	xorno \$a23 = \$a24, \$a24;;

    19b0:	59 26 63 07                                     	xoro \$a24 = \$a25, \$a25;;

    19b4:	b0 c0 c3 72                                     	xorrbod \$r48 = \$r48;;

    19b8:	71 c0 c3 72                                     	xorrhqd \$r48 = \$r49;;

    19bc:	31 c0 c7 72                                     	xorrwpd \$r49 = \$r49;;

    19c0:	b2 1c c9 7c                                     	xorw \$r50 = \$r50, \$r50;;

    19c4:	33 f0 cc 7c                                     	xorw \$r51 = \$r51, -64 \(0xffffffc0\);;

    19c8:	34 00 cc fc 00 00 80 07                         	xorw \$r51 = \$r52, -8589934592 \(0xfffffffe00000000\);;

    19d0:	00 e0 67 78                                     	xrecvo.f \$a25;;

    19d4:	9a e6 6a 07                                     	xsbmm8dq \$a26 = \$a26, \$a26;;

    19d8:	db f6 6e 07                                     	xsbmmt8dq \$a27 = \$a27, \$a27;;

    19dc:	00 e7 03 77                                     	xsendo.b \$a28;;

    19e0:	00 e7 73 7e                                     	xsendrecvo.f.b \$a28, \$a28;;

    19e4:	34 ed 77 35                                     	xso \$r52\[\$r52\] = \$a29;;

    19e8:	f5 ff 75 b5 ff ff ff 9f ff ff ff 18             	xso 2305843009213693951 \(0x1fffffffffffffff\)\[\$r53\] = \$a29;;

    19f4:	75 6d 77 b5 00 00 00 98 00 00 80 1f             	xso.mtc \$r53\? -1125899906842624 \(0xfffc000000000000\)\[\$r53\] = \$a29;;

    1a00:	b6 7d 7b b5 00 00 80 1f                         	xso.mfc \$r54\? -8388608 \(0xff800000\)\[\$r54\] = \$a30;;

    1a08:	b7 0d 7b 35                                     	xso.dnez \$r54\? \[\$r55\] = \$a30;;

    1a0c:	37 f0 79 35                                     	xso -64 \(0xffffffc0\)\[\$r55\] = \$a30;;

    1a10:	37 00 7d b5 00 00 80 1f                         	xso -8589934592 \(0xfffffffe00000000\)\[\$r55\] = \$a31;;

    1a18:	c0 ff 7d ee ff ff ff 87 ff ff ff 00             	xsplatdo \$a31 = 2305843009213693951 \(0x1fffffffffffffff\);;

    1a24:	3c 00 7d ee 00 00 00 00                         	xsplatdo \$a31 = -549755813888 \(0xffffff8000000000\);;

    1a2c:	00 f0 81 6e                                     	xsplatdo \$a32 = -4096 \(0xfffff000\);;

    1a30:	00 18 b1 07                                     	xsplatov.td \$a44a45a46a47 = \$a32;;

    1a34:	00 18 70 07                                     	xsplatox.zd \$a28a29 = \$a32;;

    1a38:	40 08 c1 06                                     	xsx48bw \$a48a49a50a51 = \$a33;;

    1a3c:	00 0d 84 06                                     	xtrunc48wb \$a33 = \$a52a53a54a55;;

    1a40:	a2 48 87 07                                     	xxoro \$a33 = \$a34, \$a34;;

    1a44:	80 08 e5 06                                     	xzx48bw \$a56a57a58a59 = \$a34;;

    1a48:	f8 3f e0 78                                     	zxbd \$r56 = \$r56;;

    1a4c:	39 f0 e3 64                                     	zxhd \$r56 = \$r57;;

    1a50:	39 50 e6 74                                     	zxlbhq \$r57 = \$r57;;

    1a54:	3a 40 ea 74                                     	zxlhwp \$r58 = \$r58;;

    1a58:	3b 50 ea 75                                     	zxmbhq \$r58 = \$r59;;

    1a5c:	3b 40 ee 75                                     	zxmhwp \$r59 = \$r59;;

    1a60:	fc ff f0 78                                     	zxwd \$r60 = \$r60;;

