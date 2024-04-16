#as: -march=kv4-1
#objdump: -d
.*\/kv4-1-insns-64.o:     file format elf64-kvx

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

     56c:	ee fb ba 75                                     	avgrbo \$r46 = \$r46, \$r47;;

     570:	ef ff be f5 ff ff ff 00                         	avgrbo.@ \$r47 = \$r47, 536870911 \(0x1fffffff\);;

     578:	30 9c c2 75                                     	avgrhq \$r48 = \$r48, \$r48;;

     57c:	71 fc c6 77                                     	avgrubo \$r49 = \$r49, \$r49;;

     580:	f2 f7 ca f7 ff ff ff 00                         	avgrubo \$r50 = \$r50, 536870911 \(0x1fffffff\);;

     588:	f3 9c ca 77                                     	avgruhq \$r50 = \$r51, \$r51;;

     58c:	34 8d ce 77                                     	avgruwp \$r51 = \$r52, \$r52;;

     590:	f5 8f d2 f7 ff ff ff 00                         	avgruwp.@ \$r52 = \$r53, 536870911 \(0x1fffffff\);;

     598:	b5 7d d6 77                                     	avgruw \$r53 = \$r53, \$r54;;

     59c:	f6 77 da f7 ff ff ff 00                         	avgruw \$r54 = \$r54, 536870911 \(0x1fffffff\);;

     5a4:	f7 8d de 75                                     	avgrwp \$r55 = \$r55, \$r55;;

     5a8:	f8 87 e2 f5 ff ff ff 00                         	avgrwp \$r56 = \$r56, 536870911 \(0x1fffffff\);;

     5b0:	79 7e e2 75                                     	avgrw \$r56 = \$r57, \$r57;;

     5b4:	fa 77 e6 f5 ff ff ff 00                         	avgrw \$r57 = \$r58, 536870911 \(0x1fffffff\);;

     5bc:	fa fe ea 76                                     	avgubo \$r58 = \$r58, \$r59;;

     5c0:	fb ff ee f6 ff ff ff 00                         	avgubo.@ \$r59 = \$r59, 536870911 \(0x1fffffff\);;

     5c8:	3c 9f f2 76                                     	avguhq \$r60 = \$r60, \$r60;;

     5cc:	7d 8f f6 76                                     	avguwp \$r61 = \$r61, \$r61;;

     5d0:	fe 87 fa f6 ff ff ff 00                         	avguwp \$r62 = \$r62, 536870911 \(0x1fffffff\);;

     5d8:	ff 7f fa 76                                     	avguw \$r62 = \$r63, \$r63;;

     5dc:	c0 77 fe f6 ff ff ff 00                         	avguw \$r63 = \$r0, 536870911 \(0x1fffffff\);;

     5e4:	40 80 02 74                                     	avgwp \$r0 = \$r0, \$r1;;

     5e8:	c1 8f 06 f4 ff ff ff 00                         	avgwp.@ \$r1 = \$r1, 536870911 \(0x1fffffff\);;

     5f0:	82 70 0a 74                                     	avgw \$r2 = \$r2, \$r2;;

     5f4:	c3 77 0e f4 ff ff ff 00                         	avgw \$r3 = \$r3, 536870911 \(0x1fffffff\);;

     5fc:	00 00 a0 0f                                     	await;;

     600:	00 00 ac 0f                                     	barrier;;

     604:	00 80 00 00                                     	break 0 \(0x0\);;

     608:	00 00 80 1f                                     	call fffffffffe000608 <main\+0xfffffffffe000608>;;

     60c:	04 20 0e 72                                     	cbsd \$r3 = \$r4;;

     610:	04 40 12 72                                     	cbswp \$r4 = \$r4;;

     614:	05 30 16 72                                     	cbsw \$r5 = \$r5;;

     618:	05 00 78 08                                     	cb.dnez \$r5\? ffffffffffff8618 <main\+0xffffffffffff8618>;;

     61c:	c6 71 1b 6c                                     	clrf \$r6 = \$r6, 7 \(0x7\), 7 \(0x7\);;

     620:	07 20 1a 71                                     	clsd \$r6 = \$r7;;

     624:	07 40 1e 71                                     	clswp \$r7 = \$r7;;

     628:	08 30 22 71                                     	clsw \$r8 = \$r8;;

     62c:	09 20 22 70                                     	clzd \$r8 = \$r9;;

     630:	09 40 26 70                                     	clzwp \$r9 = \$r9;;

     634:	0a 30 2a 70                                     	clzw \$r10 = \$r10;;

     638:	ca d2 2e 70                                     	cmovebo.nez \$r10\? \$r11 = \$r11;;

     63c:	cb ff 32 e1 ff ff ff 87 ff ff ff 00             	cmoved.deqz \$r11\? \$r12 = 2305843009213693951 \(0x1fffffffffffffff\);;

     648:	4c 03 36 72                                     	cmoved.dltz \$r12\? \$r13 = \$r13;;

     64c:	0e f0 3a 63                                     	cmoved.dgez \$r14\? \$r14 = -64 \(0xffffffc0\);;

     650:	0f 00 3e e4 00 00 80 07                         	cmoved.dlez \$r15\? \$r15 = -8589934592 \(0xfffffffe00000000\);;

     658:	10 14 42 79                                     	cmovehq.eqz \$r16\? \$r16 = \$r16;;

     65c:	51 14 46 72                                     	cmovewp.ltz \$r17\? \$r17 = \$r17;;

     660:	92 14 24 5b                                     	cmuldt \$r8r9 = \$r18, \$r18;;

     664:	d2 14 2c 5f                                     	cmulghxdt \$r10r11 = \$r18, \$r19;;

     668:	d3 14 2c 5d                                     	cmulglxdt \$r10r11 = \$r19, \$r19;;

     66c:	14 15 34 5e                                     	cmulgmxdt \$r12r13 = \$r20, \$r20;;

     670:	54 15 34 5c                                     	cmulxdt \$r12r13 = \$r20, \$r21;;

     674:	d5 ff 55 e0 ff ff ff 87 ff ff ff 00             	compd.ne \$r21 = \$r21, 2305843009213693951 \(0x1fffffffffffffff\);;

     680:	96 a5 59 71                                     	compd.eq \$r22 = \$r22, \$r22;;

     684:	17 f0 5d 62                                     	compd.lt \$r23 = \$r23, -64 \(0xffffffc0\);;

     688:	18 00 5d e3 00 00 80 07                         	compd.ge \$r23 = \$r24, -8589934592 \(0xfffffffe00000000\);;

     690:	58 c6 62 74                                     	compnbo.le \$r24 = \$r24, \$r25;;

     694:	d9 c7 66 f5 ff ff ff 00                         	compnbo.gt \$r25 = \$r25, 536870911 \(0x1fffffff\);;

     69c:	9a b6 6b 76                                     	compnd.ltu \$r26 = \$r26, \$r26;;

     6a0:	db b7 6f f7 ff ff ff 00                         	compnd.geu \$r27 = \$r27, 536870911 \(0x1fffffff\);;

     6a8:	1c f7 6d 78                                     	compnhq.leu \$r27 = \$r28, \$r28;;

     6ac:	dd ff 71 f9 ff ff ff 00                         	compnhq.gtu.@ \$r28 = \$r29, 536870911 \(0x1fffffff\);;

     6b4:	9d e7 75 7a                                     	compnwp.all \$r29 = \$r29, \$r30;;

     6b8:	de e7 79 fb ff ff ff 00                         	compnwp.nall \$r30 = \$r30, 536870911 \(0x1fffffff\);;

     6c0:	df a7 7f 7c                                     	compnw.any \$r31 = \$r31, \$r31;;

     6c4:	e0 a7 83 fd ff ff ff 00                         	compnw.none \$r32 = \$r32, 536870911 \(0x1fffffff\);;

     6cc:	61 d8 81 70                                     	compuwd.ne \$r32 = \$r33, \$r33;;

     6d0:	e2 d7 85 f1 ff ff ff 00                         	compuwd.eq \$r33 = \$r34, 536870911 \(0x1fffffff\);;

     6d8:	e2 c8 89 72                                     	compwd.lt \$r34 = \$r34, \$r35;;

     6dc:	e3 c7 8d f3 ff ff ff 00                         	compwd.ge \$r35 = \$r35, 536870911 \(0x1fffffff\);;

     6e4:	24 b9 91 74                                     	compw.le \$r36 = \$r36, \$r36;;

     6e8:	e5 b7 95 f5 ff ff ff 00                         	compw.gt \$r37 = \$r37, 536870911 \(0x1fffffff\);;

     6f0:	26 00 94 6a                                     	copyd \$r37 = \$r38;;

     6f4:	10 00 3d 34                                     	copyo \$r12r13r14r15 = \$r16r17r18r19;;

     6f8:	a6 f9 38 5f                                     	copyq \$r14r15 = \$r38, \$r38;;

     6fc:	27 00 9c 7a                                     	copyw \$r39 = \$r39;;

     700:	28 2a 9c 59                                     	crcbellw \$r39 = \$r40, \$r40;;

     704:	e9 27 a0 d9 ff ff ff 10                         	crcbellw \$r40 = \$r41, 536870911 \(0x1fffffff\);;

     70c:	a9 2a a4 58                                     	crcbelmw \$r41 = \$r41, \$r42;;

     710:	ea 27 a8 d8 ff ff ff 10                         	crcbelmw \$r42 = \$r42, 536870911 \(0x1fffffff\);;

     718:	eb 2a ac 5b                                     	crclellw \$r43 = \$r43, \$r43;;

     71c:	ec 27 b0 db ff ff ff 10                         	crclellw \$r44 = \$r44, 536870911 \(0x1fffffff\);;

     724:	6d 2b b0 5a                                     	crclelmw \$r44 = \$r45, \$r45;;

     728:	ee 27 b4 da ff ff ff 10                         	crclelmw \$r45 = \$r46, 536870911 \(0x1fffffff\);;

     730:	2e 20 ba 73                                     	ctzd \$r46 = \$r46;;

     734:	2f 40 be 73                                     	ctzwp \$r47 = \$r47;;

     738:	30 30 be 73                                     	ctzw \$r47 = \$r48;;

     73c:	00 00 8c 3c                                     	d1inval;;

     740:	30 ec 3e 3c                                     	dflushl \$r48\[\$r48\];;

     744:	f1 ff 3c bc ff ff ff 9f ff ff ff 18             	dflushl 2305843009213693951 \(0x1fffffffffffffff\)\[\$r49\];;

     750:	31 f0 3c 3c                                     	dflushl -64 \(0xffffffc0\)\[\$r49\];;

     754:	31 00 3c bc 00 00 80 1f                         	dflushl -8589934592 \(0xfffffffe00000000\)\[\$r49\];;

     75c:	b2 ec be 3c                                     	dflushsw.l1 \$r50, \$r50;;

     760:	b3 fc 1e 3c                                     	dinvall.xs \$r50\[\$r51\];;

     764:	f3 ff 1c bc ff ff ff 9f ff ff ff 18             	dinvall 2305843009213693951 \(0x1fffffffffffffff\)\[\$r51\];;

     770:	33 f0 1c 3c                                     	dinvall -64 \(0xffffffc0\)\[\$r51\];;

     774:	34 00 1c bc 00 00 80 1f                         	dinvall -8589934592 \(0xfffffffe00000000\)\[\$r52\];;

     77c:	34 ed 9e 3d                                     	dinvalsw.l2 \$r52, \$r52;;

     780:	10 24 38 52                                     	dot2suwdp \$r14r15 = \$r16r17, \$r16r17;;

     784:	75 2d d4 5e                                     	dot2suwd \$r53 = \$r53, \$r53;;

     788:	12 25 48 51                                     	dot2uwdp \$r18r19 = \$r18r19, \$r20r21;;

     78c:	b6 2d d8 5d                                     	dot2uwd \$r54 = \$r54, \$r54;;

     790:	96 25 50 50                                     	dot2wdp \$r20r21 = \$r22r23, \$r22r23;;

     794:	f7 2d dc 5c                                     	dot2wd \$r55 = \$r55, \$r55;;

     798:	98 26 60 53                                     	dot2wzp \$r24r25 = \$r24r25, \$r26r27;;

     79c:	38 2e e0 5f                                     	dot2w \$r56 = \$r56, \$r56;;

     7a0:	79 ee 2e 3c                                     	dpurgel \$r57\[\$r57\];;

     7a4:	f9 ff 2c bc ff ff ff 9f ff ff ff 18             	dpurgel 2305843009213693951 \(0x1fffffffffffffff\)\[\$r57\];;

     7b0:	3a f0 2c 3c                                     	dpurgel -64 \(0xffffffc0\)\[\$r58\];;

     7b4:	3a 00 2c bc 00 00 80 1f                         	dpurgel -8589934592 \(0xfffffffe00000000\)\[\$r58\];;

     7bc:	bb ee ae 3c                                     	dpurgesw.l1 \$r58, \$r59;;

     7c0:	fb fe 0e 3c                                     	dtouchl.xs \$r59\[\$r59\];;

     7c4:	fc ff 0c bc ff ff ff 9f ff ff ff 18             	dtouchl 2305843009213693951 \(0x1fffffffffffffff\)\[\$r60\];;

     7d0:	3c f0 0c 3c                                     	dtouchl -64 \(0xffffffc0\)\[\$r60\];;

     7d4:	3c 00 0c bc 00 00 80 1f                         	dtouchl -8589934592 \(0xfffffffe00000000\)\[\$r60\];;

     7dc:	00 00 00 00                                     	errop;;

     7e0:	fd 71 f7 68                                     	extfs \$r61 = \$r61, 7 \(0x7\), 7 \(0x7\);;

     7e4:	fe 71 f7 64                                     	extfz \$r61 = \$r62, 7 \(0x7\), 7 \(0x7\);;

     7e8:	3e 20 fb 71                                     	fabsd \$r62 = \$r62;;

     7ec:	3f 20 ff 77                                     	fabshq \$r63 = \$r63;;

     7f0:	00 20 ff 75                                     	fabswp \$r63 = \$r0;;

     7f4:	00 20 03 73                                     	fabsw \$r0 = \$r0;;

     7f8:	1c 07 6b 5d                                     	fadddc.c.rn \$r26r27 = \$r28r29, \$r28r29;;

     7fc:	1e 98 7b 5c                                     	fadddp.ru.s \$r30r31 = \$r30r31, \$r32r33;;

     800:	a2 28 83 5c                                     	fadddp.rd \$r32r33 = \$r34r35, \$r34r35;;

     804:	41 b0 06 50                                     	faddd.rz.s \$r1 = \$r1, \$r1;;

     808:	a4 49 97 56                                     	faddho.rna \$r36r37 = \$r36r37, \$r38r39;;

     80c:	82 d0 0a 52                                     	faddhq.rnz.s \$r2 = \$r2, \$r2;;

     810:	c3 60 0e 53                                     	faddwc.c.ro \$r3 = \$r3, \$r3;;

     814:	28 fa 9f 59                                     	faddwcp.c.s \$r38r39 = \$r40r41, \$r40r41;;

     818:	2a 0b af 58                                     	faddwq.rn \$r42r43 = \$r42r43, \$r44r45;;

     81c:	04 91 12 51                                     	faddwp.ru.s \$r4 = \$r4, \$r4;;

     820:	45 21 16 51                                     	faddwp.rd \$r5 = \$r5, \$r5;;

     824:	ae bb b7 58                                     	faddwq.rz.s \$r44r45 = \$r46r47, \$r46r47;;

     828:	86 41 1a 5c                                     	faddw.rna \$r6 = \$r6, \$r6;;

     82c:	30 58 1f 71                                     	fcdivd.s \$r7 = \$r48r49;;

     830:	30 50 1f 75                                     	fcdivwp \$r7 = \$r48r49;;

     834:	32 58 1f 73                                     	fcdivw.s \$r7 = \$r50r51;;

     838:	08 02 23 78                                     	fcompd.one \$r8 = \$r8, \$r8;;

     83c:	c9 07 27 f9 ff ff ff 00                         	fcompd.ueq \$r9 = \$r9, 536870911 \(0x1fffffff\);;

     844:	8a 92 27 7a                                     	fcompnd.oeq \$r9 = \$r10, \$r10;;

     848:	cb 97 2b fb ff ff ff 00                         	fcompnd.une \$r10 = \$r11, 536870911 \(0x1fffffff\);;

     850:	0b 13 2f 7c                                     	fcompnhq.olt \$r11 = \$r11, \$r12;;

     854:	cd 1f 33 fd ff ff ff 00                         	fcompnhq.uge.@ \$r12 = \$r13, 536870911 \(0x1fffffff\);;

     85c:	8e 13 37 76                                     	fcompnwp.oge \$r13 = \$r14, \$r14;;

     860:	cf 17 3f f7 ff ff ff 00                         	fcompnwp.ult \$r15 = \$r15, 536870911 \(0x1fffffff\);;

     868:	10 94 43 70                                     	fcompnw.one \$r16 = \$r16, \$r16;;

     86c:	d1 97 47 f1 ff ff ff 00                         	fcompnw.ueq \$r17 = \$r17, 536870911 \(0x1fffffff\);;

     874:	92 04 47 72                                     	fcompw.oeq \$r17 = \$r18, \$r18;;

     878:	d3 07 4b f3 ff ff ff 00                         	fcompw.une \$r18 = \$r19, 536870911 \(0x1fffffff\);;

     880:	34 5d cf 5c                                     	fdot2wdp.rnz \$r50r51 = \$r52r53, \$r52r53;;

     884:	13 e5 4d 5d                                     	fdot2wd.ro.s \$r19 = \$r19, \$r20;;

     888:	36 7e df 5d                                     	fdot2wzp \$r54r55 = \$r54r55, \$r56r57;;

     88c:	54 85 51 5c                                     	fdot2w.rn.s \$r20 = \$r20, \$r21;;

     890:	00 00 fc 3c                                     	fence;;

     894:	b8 1e 56 47                                     	ffdmaswp.ru \$r21 = \$r56r57, \$r58r59;;

     898:	14 a6 ea 4f                                     	ffdmaswq.rd.s \$r58r59 = \$r20r21r22r23, \$r24r25r26r27;;

     89c:	96 35 56 43                                     	ffdmasw.rz \$r21 = \$r22, \$r22;;

     8a0:	3c cf 59 42                                     	ffdmawp.rna.s \$r22 = \$r60r61, \$r60r61;;

     8a4:	1c 58 f9 46                                     	ffdmawq.rnz \$r62r63 = \$r28r29r30r31, \$r32r33r34r35;;

     8a8:	d7 e5 5d 40                                     	ffdmaw.ro.s \$r23 = \$r23, \$r23;;

     8ac:	3e 70 62 44                                     	ffdmdawp \$r24 = \$r62r63, \$r0r1;;

     8b0:	24 8a 02 4c                                     	ffdmdawq.rn.s \$r0r1 = \$r36r37r38r39, \$r40r41r42r43;;

     8b4:	58 16 62 40                                     	ffdmdaw.ru \$r24 = \$r24, \$r25;;

     8b8:	82 a0 66 46                                     	ffdmdswp.rd.s \$r25 = \$r2r3, \$r2r3;;

     8bc:	2c 3c 12 4e                                     	ffdmdswq.rz \$r4r5 = \$r44r45r46r47, \$r48r49r50r51;;

     8c0:	9a c6 66 42                                     	ffdmdsw.rna.s \$r25 = \$r26, \$r26;;

     8c4:	84 51 6a 45                                     	ffdmsawp.rnz \$r26 = \$r4r5, \$r6r7;;

     8c8:	34 ee 1a 4d                                     	ffdmsawq.ro.s \$r6r7 = \$r52r53r54r55, \$r56r57r58r59;;

     8cc:	db 76 6e 41                                     	ffdmsaw \$r27 = \$r27, \$r27;;

     8d0:	08 82 71 43                                     	ffdmswp.rn.s \$r28 = \$r8r9, \$r8r9;;

     8d4:	3c 10 29 47                                     	ffdmswq.ru \$r10r11 = \$r60r61r62r63, \$r0r1r2r3;;

     8d8:	5c a7 71 41                                     	ffdmsw.rd.s \$r28 = \$r28, \$r29;;

     8dc:	9d 37 74 44                                     	ffmad.rz \$r29 = \$r29, \$r30;;

     8e0:	0c c3 2b 5a                                     	ffmaho.rna.s \$r10r11 = \$r12r13, \$r12r13;;

     8e4:	de 57 79 53                                     	ffmahq.rnz \$r30 = \$r30, \$r31;;

     8e8:	df e7 3b 51                                     	ffmahwq.ro.s \$r14r15 = \$r31, \$r31;;

     8ec:	20 78 82 58                                     	ffmahw \$r32 = \$r32, \$r32;;

     8f0:	10 84 39 4c                                     	ffmawcp.rn.s \$r14r15 = \$r16r17, \$r16r17;;

     8f4:	61 18 85 49                                     	ffmawc.c.ru \$r33 = \$r33, \$r33;;

     8f8:	a2 a8 4b 50                                     	ffmawdp.rd.s \$r18r19 = \$r34, \$r34;;

     8fc:	e3 38 89 51                                     	ffmawd.rz \$r34 = \$r35, \$r35;;

     900:	24 c9 8c 42                                     	ffmawp.rna.s \$r35 = \$r36, \$r36;;

     904:	14 55 48 46                                     	ffmawq.rnz \$r18r19 = \$r20r21, \$r20r21;;

     908:	65 e9 90 40                                     	ffmaw.ro.s \$r36 = \$r37, \$r37;;

     90c:	a6 79 94 45                                     	ffmsd \$r37 = \$r38, \$r38;;

     910:	16 86 5b 5b                                     	ffmsho.rn.s \$r22r23 = \$r22r23, \$r24r25;;

     914:	e7 19 99 57                                     	ffmshq.ru \$r38 = \$r39, \$r39;;

     918:	27 aa 63 53                                     	ffmshwq.rd.s \$r24r25 = \$r39, \$r40;;

     91c:	68 3a a2 5a                                     	ffmshw.rz \$r40 = \$r40, \$r41;;

     920:	1a c7 69 4e                                     	ffmswcp.rna.s \$r26r27 = \$r26r27, \$r28r29;;

     924:	a9 5a a5 4b                                     	ffmswc.c.rnz \$r41 = \$r41, \$r42;;

     928:	aa ea 73 52                                     	ffmswdp.ro.s \$r28r29 = \$r42, \$r42;;

     92c:	eb 7a ad 55                                     	ffmswd \$r43 = \$r43, \$r43;;

     930:	2c 8b b0 43                                     	ffmswp.rn.s \$r44 = \$r44, \$r44;;

     934:	1e 18 78 47                                     	ffmswq.ru \$r30r31 = \$r30r31, \$r32r33;;

     938:	6d ab b4 41                                     	ffmsw.rd.s \$r45 = \$r45, \$r45;;

     93c:	ee 31 bb 46                                     	fixedd.rz \$r46 = \$r46, 7 \(0x7\);;

     940:	ef c1 bb 47                                     	fixedud.rna.s \$r46 = \$r47, 7 \(0x7\);;

     944:	ef 51 bf 4f                                     	fixeduwp.rnz \$r47 = \$r47, 7 \(0x7\);;

     948:	f0 e1 c3 4b                                     	fixeduw.ro.s \$r48 = \$r48, 7 \(0x7\);;

     94c:	f1 71 c3 4e                                     	fixedwp \$r48 = \$r49, 7 \(0x7\);;

     950:	f1 81 c7 4a                                     	fixedw.rn.s \$r49 = \$r49, 7 \(0x7\);;

     954:	f2 11 cb 44                                     	floatd.ru \$r50 = \$r50, 7 \(0x7\);;

     958:	f3 a1 cb 45                                     	floatud.rd.s \$r50 = \$r51, 7 \(0x7\);;

     95c:	f3 31 cf 4d                                     	floatuwp.rz \$r51 = \$r51, 7 \(0x7\);;

     960:	f4 c1 d3 49                                     	floatuw.rna.s \$r52 = \$r52, 7 \(0x7\);;

     964:	f5 51 d3 4c                                     	floatwp.rnz \$r52 = \$r53, 7 \(0x7\);;

     968:	f5 e1 d7 48                                     	floatw.ro.s \$r53 = \$r53, 7 \(0x7\);;

     96c:	b6 8d db 71                                     	fmaxd \$r54 = \$r54, \$r54;;

     970:	f7 8d df 77                                     	fmaxhq \$r55 = \$r55, \$r55;;

     974:	38 8e e3 75                                     	fmaxwp \$r56 = \$r56, \$r56;;

     978:	79 8e e7 73                                     	fmaxw \$r57 = \$r57, \$r57;;

     97c:	ba 8e eb 70                                     	fmind \$r58 = \$r58, \$r58;;

     980:	fb 8e ef 76                                     	fminhq \$r59 = \$r59, \$r59;;

     984:	3c 8f f3 74                                     	fminwp \$r60 = \$r60, \$r60;;

     988:	7d 8f f7 72                                     	fminw \$r61 = \$r61, \$r61;;

     98c:	be 7f 80 4c                                     	fmm212w \$r32r33 = \$r62, \$r62;;

     990:	22 89 8c 4c                                     	fmm222w.rn.s \$r34r35 = \$r34r35, \$r36r37;;

     994:	fe 1f 90 4e                                     	fmma212w.ru \$r36r37 = \$r62, \$r63;;

     998:	27 aa 9c 4e                                     	fmma222w.tn.rd.s \$r38r39 = \$r38r39, \$r40r41;;

     99c:	ff 3f a0 4f                                     	fmms212w.rz \$r40r41 = \$r63, \$r63;;

     9a0:	6a cb ac 4f                                     	fmms222w.nt.rna.s \$r42r43 = \$r42r43, \$r44r45;;

     9a4:	00 50 01 58                                     	fmuld.rnz \$r0 = \$r0, \$r0;;

     9a8:	ae eb b7 55                                     	fmulho.ro.s \$r44r45 = \$r46r47, \$r46r47;;

     9ac:	41 70 05 5b                                     	fmulhq \$r1 = \$r1, \$r1;;

     9b0:	82 80 c7 51                                     	fmulhwq.rn.s \$r48r49 = \$r2, \$r2;;

     9b4:	c3 10 0a 5f                                     	fmulhw.ru \$r2 = \$r3, \$r3;;

     9b8:	b2 ac c0 4a                                     	fmulwcp.rd.s \$r48r49 = \$r50r51, \$r50r51;;

     9bc:	04 31 0c 49                                     	fmulwc.c.rz \$r3 = \$r4, \$r4;;

     9c0:	44 c1 d7 50                                     	fmulwdp.rna.s \$r52r53 = \$r4, \$r5;;

     9c4:	85 51 15 59                                     	fmulwd.rnz \$r5 = \$r5, \$r6;;

     9c8:	c6 e1 19 5a                                     	fmulwp.ro.s \$r6 = \$r6, \$r7;;

     9cc:	b6 7d d7 5e                                     	fmulwq \$r52r53 = \$r54r55, \$r54r55;;

     9d0:	07 82 1e 5e                                     	fmulw.rn.s \$r7 = \$r7, \$r8;;

     9d4:	38 61 23 7c                                     	fnarrowdwp.ru \$r8 = \$r56r57;;

     9d8:	09 6a 23 78                                     	fnarrowdw.rd.s \$r8 = \$r9;;

     9dc:	38 63 27 7e                                     	fnarrowwhq.rz \$r9 = \$r56r57;;

     9e0:	0a 6c 27 7a                                     	fnarrowwh.rna.s \$r9 = \$r10;;

     9e4:	0a 20 2b 70                                     	fnegd \$r10 = \$r10;;

     9e8:	0b 20 2f 76                                     	fneghq \$r11 = \$r11;;

     9ec:	0c 20 2f 74                                     	fnegwp \$r11 = \$r12;;

     9f0:	0d 20 33 72                                     	fnegw \$r12 = \$r13;;

     9f4:	0e 65 37 72                                     	frecw.rnz \$r13 = \$r14;;

     9f8:	0f 6e 3b 73                                     	frsrw.ro.s \$r14 = \$r15;;

     9fc:	3a 7f eb 5f                                     	fsbfdc.c \$r58r59 = \$r58r59, \$r60r61;;

     a00:	be 8f f3 5e                                     	fsbfdp.rn.s \$r60r61 = \$r62r63, \$r62r63;;

     a04:	80 10 03 5e                                     	fsbfdp.ru \$r0r1 = \$r0r1, \$r2r3;;

     a08:	10 a4 3e 54                                     	fsbfd.rd.s \$r15 = \$r16, \$r16;;

     a0c:	04 31 0f 57                                     	fsbfho.rz \$r2r3 = \$r4r5, \$r4r5;;

     a10:	51 c4 42 56                                     	fsbfhq.rna.s \$r16 = \$r17, \$r17;;

     a14:	92 54 46 57                                     	fsbfwc.c.rnz \$r17 = \$r18, \$r18;;

     a18:	06 e2 1f 5b                                     	fsbfwcp.c.ro.s \$r6r7 = \$r6r7, \$r8r9;;

     a1c:	8a 72 27 5a                                     	fsbfwq \$r8r9 = \$r10r11, \$r10r11;;

     a20:	d3 84 4a 55                                     	fsbfwp.rn.s \$r18 = \$r19, \$r19;;

     a24:	14 15 4e 55                                     	fsbfwp.ru \$r19 = \$r20, \$r20;;

     a28:	8c a3 37 5a                                     	fsbfwq.rd.s \$r12r13 = \$r12r13, \$r14r15;;

     a2c:	55 35 52 5d                                     	fsbfw.rz \$r20 = \$r21, \$r21;;

     a30:	0e 58 57 70                                     	fsdivd.s \$r21 = \$r14r15;;

     a34:	10 50 5b 74                                     	fsdivwp \$r22 = \$r16r17;;

     a38:	10 58 5b 72                                     	fsdivw.s \$r22 = \$r16r17;;

     a3c:	17 40 5b 70                                     	fsrecd \$r22 = \$r23;;

     a40:	17 48 5f 74                                     	fsrecwp.s \$r23 = \$r23;;

     a44:	18 40 63 72                                     	fsrecw \$r24 = \$r24;;

     a48:	19 20 63 78                                     	fsrsrd \$r24 = \$r25;;

     a4c:	19 20 67 7c                                     	fsrsrwp \$r25 = \$r25;;

     a50:	1a 20 6b 7a                                     	fsrsrw \$r26 = \$r26;;

     a54:	1b 38 6b 7c                                     	fwidenlhwp.s \$r26 = \$r27;;

     a58:	1b 30 6f 7a                                     	fwidenlhw \$r27 = \$r27;;

     a5c:	1c 38 73 78                                     	fwidenlwd.s \$r28 = \$r28;;

     a60:	1d 30 73 7d                                     	fwidenmhwp \$r28 = \$r29;;

     a64:	1d 38 77 7b                                     	fwidenmhw.s \$r29 = \$r29;;

     a68:	1e 30 7b 79                                     	fwidenmwd \$r30 = \$r30;;

     a6c:	1e 00 c4 0f                                     	get \$r30 = \$pc;;

     a70:	1f 00 c4 0f                                     	get \$r31 = \$pc;;

     a74:	00 00 80 17                                     	goto fffffffffe000a74 <main\+0xfffffffffe000a74>;;

     a78:	df e7 5e 3c                                     	i1invals \$r31\[\$r31\];;

     a7c:	e0 ff 5c bc ff ff ff 9f ff ff ff 18             	i1invals 2305843009213693951 \(0x1fffffffffffffff\)\[\$r32\];;

     a88:	20 f0 5c 3c                                     	i1invals -64 \(0xffffffc0\)\[\$r32\];;

     a8c:	20 00 5c bc 00 00 80 1f                         	i1invals -8589934592 \(0xfffffffe00000000\)\[\$r32\];;

     a94:	00 00 cc 3c                                     	i1inval;;

     a98:	21 00 dc 0f                                     	icall \$r33;;

     a9c:	21 00 cc 0f                                     	iget \$r33;;

     aa0:	21 00 d8 0f                                     	igoto \$r33;;

     aa4:	e2 71 8b 60                                     	insf \$r34 = \$r34, 7 \(0x7\), 7 \(0x7\);;

     aa8:	e3 68 8a 70                                     	landd \$r34 = \$r35, \$r35;;

     aac:	24 79 8e 70                                     	landw \$r35 = \$r36, \$r36;;

     ab0:	e5 77 92 f0 ff ff ff 00                         	landw \$r36 = \$r37, 536870911 \(0x1fffffff\);;

     ab8:	66 f9 96 24                                     	lbs.xs \$r37 = \$r37\[\$r38\];;

     abc:	a7 59 9a a5 00 00 00 98 00 00 80 1f             	lbs.s.dgtz \$r38\? \$r38 = -1125899906842624 \(0xfffc000000000000\)\[\$r39\];;

     ac8:	e8 69 9e a6 00 00 80 1f                         	lbs.u.odd \$r39\? \$r39 = -8388608 \(0xff800000\)\[\$r40\];;

     ad0:	29 7a a2 27                                     	lbs.us.even \$r40\? \$r40 = \[\$r41\];;

     ad4:	e9 ff a4 a4 ff ff ff 9f ff ff ff 18             	lbs \$r41 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r41\];;

     ae0:	2a f0 a8 25                                     	lbs.s \$r42 = -64 \(0xffffffc0\)\[\$r42\];;

     ae4:	2b 00 a8 a6 00 00 80 1f                         	lbs.u \$r42 = -8589934592 \(0xfffffffe00000000\)\[\$r43\];;

     aec:	ec ea ae 23                                     	lbz.us \$r43 = \$r43\[\$r44\];;

     af0:	2d 8b b2 a0 00 00 00 98 00 00 80 1f             	lbz.wnez \$r44\? \$r44 = -1125899906842624 \(0xfffc000000000000\)\[\$r45\];;

     afc:	6e 9b b6 a1 00 00 80 1f                         	lbz.s.weqz \$r45\? \$r45 = -8388608 \(0xff800000\)\[\$r46\];;

     b04:	af ab ba 22                                     	lbz.u.wltz \$r46\? \$r46 = \[\$r47\];;

     b08:	ef ff bc a3 ff ff ff 9f ff ff ff 18             	lbz.us \$r47 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r47\];;

     b14:	30 f0 c0 20                                     	lbz \$r48 = -64 \(0xffffffc0\)\[\$r48\];;

     b18:	31 00 c0 a1 00 00 80 1f                         	lbz.s \$r48 = -8589934592 \(0xfffffffe00000000\)\[\$r49\];;

     b20:	72 fc c6 3a                                     	ld.u.xs \$r49 = \$r49\[\$r50\];;

     b24:	b3 bc ca bb 00 00 00 98 00 00 80 1f             	ld.us.wgez \$r50\? \$r50 = -1125899906842624 \(0xfffc000000000000\)\[\$r51\];;

     b30:	f4 cc ce b8 00 00 80 1f                         	ld.wlez \$r51\? \$r51 = -8388608 \(0xff800000\)\[\$r52\];;

     b38:	35 dd d2 39                                     	ld.s.wgtz \$r52\? \$r52 = \[\$r53\];;

     b3c:	f5 ff d4 ba ff ff ff 9f ff ff ff 18             	ld.u \$r53 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r53\];;

     b48:	36 f0 d8 3b                                     	ld.us \$r54 = -64 \(0xffffffc0\)\[\$r54\];;

     b4c:	37 00 d8 b8 00 00 80 1f                         	ld \$r54 = -8589934592 \(0xfffffffe00000000\)\[\$r55\];;

     b54:	f8 ed de 2d                                     	lhs.s \$r55 = \$r55\[\$r56\];;

     b58:	39 0e e2 ae 00 00 00 98 00 00 80 1f             	lhs.u.dnez \$r56\? \$r56 = -1125899906842624 \(0xfffc000000000000\)\[\$r57\];;

     b64:	7a 1e e6 af 00 00 80 1f                         	lhs.us.deqz \$r57\? \$r57 = -8388608 \(0xff800000\)\[\$r58\];;

     b6c:	bb 2e ea 2c                                     	lhs.dltz \$r58\? \$r58 = \[\$r59\];;

     b70:	fb ff ec ad ff ff ff 9f ff ff ff 18             	lhs.s \$r59 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r59\];;

     b7c:	3c f0 f0 2e                                     	lhs.u \$r60 = -64 \(0xffffffc0\)\[\$r60\];;

     b80:	3d 00 f0 af 00 00 80 1f                         	lhs.us \$r60 = -8589934592 \(0xfffffffe00000000\)\[\$r61\];;

     b88:	7e ff f6 28                                     	lhz.xs \$r61 = \$r61\[\$r62\];;

     b8c:	bf 3f fa a9 00 00 00 98 00 00 80 1f             	lhz.s.dgez \$r62\? \$r62 = -1125899906842624 \(0xfffc000000000000\)\[\$r63\];;

     b98:	c0 4f fe aa 00 00 80 1f                         	lhz.u.dlez \$r63\? \$r63 = -8388608 \(0xff800000\)\[\$r0\];;

     ba0:	01 50 02 2b                                     	lhz.us.dgtz \$r0\? \$r0 = \[\$r1\];;

     ba4:	c1 ff 04 a8 ff ff ff 9f ff ff ff 18             	lhz \$r1 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r1\];;

     bb0:	02 f0 08 29                                     	lhz.s \$r2 = -64 \(0xffffffc0\)\[\$r2\];;

     bb4:	03 00 08 aa 00 00 80 1f                         	lhz.u \$r2 = -8589934592 \(0xfffffffe00000000\)\[\$r3\];;

     bbc:	03 61 0e 71                                     	lnandd \$r3 = \$r3, \$r4;;

     bc0:	44 71 12 71                                     	lnandw \$r4 = \$r4, \$r5;;

     bc4:	c5 77 16 f1 ff ff ff 00                         	lnandw \$r5 = \$r5, 536870911 \(0x1fffffff\);;

     bcc:	86 61 1a 73                                     	lnord \$r6 = \$r6, \$r6;;

     bd0:	c7 71 1e 73                                     	lnorw \$r7 = \$r7, \$r7;;

     bd4:	c8 77 22 f3 ff ff ff 00                         	lnorw \$r8 = \$r8, 536870911 \(0x1fffffff\);;

     bdc:	08 00 78 0f                                     	loopdo \$r8, ffffffffffff8bdc <main\+0xffffffffffff8bdc>;;

     be0:	49 62 26 72                                     	lord \$r9 = \$r9, \$r9;;

     be4:	8a 72 2a 72                                     	lorw \$r10 = \$r10, \$r10;;

     be8:	cb 77 2e f2 ff ff ff 00                         	lorw \$r11 = \$r11, 536870911 \(0x1fffffff\);;

     bf0:	cc e2 16 3f                                     	lo.us \$r4r5r6r7 = \$r11\[\$r12\];;

     bf4:	0d 03 2e bc 00 00 00 98 00 00 80 1f             	lo.u0 \$r12\? \$r8r9r10r11 = -1125899906842624 \(0xfffc000000000000\)\[\$r13\];;

     c00:	4e 13 3e bd 00 00 80 1f                         	lo.s.u1 \$r13\? \$r12r13r14r15 = -8388608 \(0xff800000\)\[\$r14\];;

     c08:	8f 23 4e 3e                                     	lo.u.u2 \$r14\? \$r16r17r18r19 = \[\$r15\];;

     c0c:	d0 63 56 bf 00 00 00 98 00 00 80 1f             	lo.us.odd \$r15\? \$r20r21r22r23 = -1125899906842624 \(0xfffc000000000000\)\[\$r16\];;

     c18:	10 74 66 bc 00 00 80 1f                         	lo.even \$r16\? \$r24r25r26r27 = -8388608 \(0xff800000\)\[\$r16\];;

     c20:	51 84 76 3d                                     	lo.s.wnez \$r17\? \$r28r29r30r31 = \[\$r17\];;

     c24:	d1 ff 84 be ff ff ff 9f ff ff ff 18             	lo.u \$r32r33r34r35 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r17\];;

     c30:	12 f0 94 3f                                     	lo.us \$r36r37r38r39 = -64 \(0xffffffc0\)\[\$r18\];;

     c34:	12 00 a4 bc 00 00 80 1f                         	lo \$r40r41r42r43 = -8589934592 \(0xfffffffe00000000\)\[\$r18\];;

     c3c:	93 f4 4a 3d                                     	lq.s.xs \$r18r19 = \$r18\[\$r19\];;

     c40:	d3 94 4a be 00 00 00 98 00 00 80 1f             	lq.u.weqz \$r19\? \$r18r19 = -1125899906842624 \(0xfffc000000000000\)\[\$r19\];;

     c4c:	14 a5 52 bf 00 00 80 1f                         	lq.us.wltz \$r20\? \$r20r21 = -8388608 \(0xff800000\)\[\$r20\];;

     c54:	15 b5 52 3c                                     	lq.wgez \$r20\? \$r20r21 = \[\$r21\];;

     c58:	d5 ff 58 bd ff ff ff 9f ff ff ff 18             	lq.s \$r22r23 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r21\];;

     c64:	15 f0 58 3e                                     	lq.u \$r22r23 = -64 \(0xffffffc0\)\[\$r21\];;

     c68:	16 00 60 bf 00 00 80 1f                         	lq.us \$r24r25 = -8589934592 \(0xfffffffe00000000\)\[\$r22\];;

     c70:	97 e5 5a 34                                     	lws \$r22 = \$r22\[\$r23\];;

     c74:	d8 c5 5e b5 00 00 00 98 00 00 80 1f             	lws.s.wlez \$r23\? \$r23 = -1125899906842624 \(0xfffc000000000000\)\[\$r24\];;

     c80:	19 d6 62 b6 00 00 80 1f                         	lws.u.wgtz \$r24\? \$r24 = -8388608 \(0xff800000\)\[\$r25\];;

     c88:	5a 06 66 37                                     	lws.us.dnez \$r25\? \$r25 = \[\$r26\];;

     c8c:	da ff 68 b4 ff ff ff 9f ff ff ff 18             	lws \$r26 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r26\];;

     c98:	1b f0 6c 35                                     	lws.s \$r27 = -64 \(0xffffffc0\)\[\$r27\];;

     c9c:	1c 00 6c b6 00 00 80 1f                         	lws.u \$r27 = -8589934592 \(0xfffffffe00000000\)\[\$r28\];;

     ca4:	1d f7 72 33                                     	lwz.us.xs \$r28 = \$r28\[\$r29\];;

     ca8:	5e 17 76 b0 00 00 00 98 00 00 80 1f             	lwz.deqz \$r29\? \$r29 = -1125899906842624 \(0xfffc000000000000\)\[\$r30\];;

     cb4:	9f 27 7a b1 00 00 80 1f                         	lwz.s.dltz \$r30\? \$r30 = -8388608 \(0xff800000\)\[\$r31\];;

     cbc:	e0 37 7e 32                                     	lwz.u.dgez \$r31\? \$r31 = \[\$r32\];;

     cc0:	e0 ff 80 b3 ff ff ff 9f ff ff ff 18             	lwz.us \$r32 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r32\];;

     ccc:	21 f0 84 30                                     	lwz \$r33 = -64 \(0xffffffc0\)\[\$r33\];;

     cd0:	22 00 84 b1 00 00 80 1f                         	lwz.s \$r33 = -8589934592 \(0xfffffffe00000000\)\[\$r34\];;

     cd8:	a2 18 60 58                                     	madddt \$r24r25 = \$r34, \$r34;;

     cdc:	e3 08 8c 50                                     	maddd \$r35 = \$r35, \$r35;;

     ce0:	e4 07 90 d0 ff ff ff 10                         	maddd \$r36 = \$r36, 536870911 \(0x1fffffff\);;

     ce8:	65 09 90 52                                     	maddhq \$r36 = \$r37, \$r37;;

     cec:	e6 07 94 d2 ff ff ff 10                         	maddhq \$r37 = \$r38, 536870911 \(0x1fffffff\);;

     cf4:	a6 49 68 50                                     	maddhwq \$r26r27 = \$r38, \$r38;;

     cf8:	1c 27 6c 51                                     	maddmwq \$r26r27 = \$r28r29, \$r28r29;;

     cfc:	e7 19 78 5a                                     	maddsudt \$r30r31 = \$r39, \$r39;;

     d00:	27 4a 78 52                                     	maddsuhwq \$r30r31 = \$r39, \$r40;;

     d04:	a0 28 84 53                                     	maddsumwq \$r32r33 = \$r32r33, \$r34r35;;

     d08:	28 0a 88 5a                                     	maddsuwdp \$r34r35 = \$r40, \$r40;;

     d0c:	69 3a a4 52                                     	maddsuwd \$r41 = \$r41, \$r41;;

     d10:	ea 37 a8 d2 ff ff ff 10                         	maddsuwd \$r42 = \$r42, 536870911 \(0x1fffffff\);;

     d18:	ea 1a 90 59                                     	maddudt \$r36r37 = \$r42, \$r43;;

     d1c:	eb 4a 90 51                                     	madduhwq \$r36r37 = \$r43, \$r43;;

     d20:	26 2a 9c 52                                     	maddumwq \$r38r39 = \$r38r39, \$r40r41;;

     d24:	2c 0b a0 59                                     	madduwdp \$r40r41 = \$r44, \$r44;;

     d28:	6d 3b b0 51                                     	madduwd \$r44 = \$r45, \$r45;;

     d2c:	ee 37 b4 d1 ff ff ff 10                         	madduwd \$r45 = \$r46, 536870911 \(0x1fffffff\);;

     d34:	ae 1b a8 5b                                     	madduzdt \$r42r43 = \$r46, \$r46;;

     d38:	ef 0b a8 58                                     	maddwdp \$r42r43 = \$r47, \$r47;;

     d3c:	30 3c bc 50                                     	maddwd \$r47 = \$r48, \$r48;;

     d40:	f1 37 c0 d0 ff ff ff 10                         	maddwd \$r48 = \$r49, 536870911 \(0x1fffffff\);;

     d48:	b1 0c c4 51                                     	maddwp \$r49 = \$r49, \$r50;;

     d4c:	f2 07 c8 d1 ff ff ff 10                         	maddwp \$r50 = \$r50, 536870911 \(0x1fffffff\);;

     d54:	ac 2b b4 50                                     	maddwq \$r44r45 = \$r44r45, \$r46r47;;

     d58:	f3 3c cc 53                                     	maddw \$r51 = \$r51, \$r51;;

     d5c:	f4 37 d0 d3 ff ff ff 10                         	maddw \$r52 = \$r52, 536870911 \(0x1fffffff\);;

     d64:	c0 ff d0 e0 ff ff ff 87 ff ff ff 00             	make \$r52 = 2305843009213693951 \(0x1fffffffffffffff\);;

     d70:	3c 00 d4 e0 00 00 00 00                         	make \$r53 = -549755813888 \(0xffffff8000000000\);;

     d78:	00 f0 d4 60                                     	make \$r53 = -4096 \(0xfffff000\);;

     d7c:	b6 ad d6 75                                     	maxbo \$r53 = \$r54, \$r54;;

     d80:	f7 af da f5 ff ff ff 00                         	maxbo.@ \$r54 = \$r55, 536870911 \(0x1fffffff\);;

     d88:	f7 ff dc e5 ff ff ff 87 ff ff ff 00             	maxd \$r55 = \$r55, 2305843009213693951 \(0x1fffffffffffffff\);;

     d94:	38 0e e1 75                                     	maxd \$r56 = \$r56, \$r56;;

     d98:	39 f0 e4 65                                     	maxd \$r57 = \$r57, -64 \(0xffffffc0\);;

     d9c:	3a 00 e4 e5 00 00 80 07                         	maxd \$r57 = \$r58, -8589934592 \(0xfffffffe00000000\);;

     da4:	fa 0f e9 f5 ff ff ff 00                         	maxd.@ \$r58 = \$r58, 536870911 \(0x1fffffff\);;

     dac:	fb 3e ed 75                                     	maxhq \$r59 = \$r59, \$r59;;

     db0:	fc 37 f1 f5 ff ff ff 00                         	maxhq \$r60 = \$r60, 536870911 \(0x1fffffff\);;

     db8:	bd c0 f3 75                                     	maxrbod \$r60 = \$r61;;

     dbc:	7d c0 f7 75                                     	maxrhqd \$r61 = \$r61;;

     dc0:	3e c0 fb 75                                     	maxrwpd \$r62 = \$r62;;

     dc4:	ff af fa 77                                     	maxubo \$r62 = \$r63, \$r63;;

     dc8:	c0 af fe f7 ff ff ff 00                         	maxubo.@ \$r63 = \$r0, 536870911 \(0x1fffffff\);;

     dd0:	c0 ff 00 e7 ff ff ff 87 ff ff ff 00             	maxud \$r0 = \$r0, 2305843009213693951 \(0x1fffffffffffffff\);;

     ddc:	41 00 05 77                                     	maxud \$r1 = \$r1, \$r1;;

     de0:	02 f0 08 67                                     	maxud \$r2 = \$r2, -64 \(0xffffffc0\);;

     de4:	03 00 08 e7 00 00 80 07                         	maxud \$r2 = \$r3, -8589934592 \(0xfffffffe00000000\);;

     dec:	c3 0f 0d f7 ff ff ff 00                         	maxud.@ \$r3 = \$r3, 536870911 \(0x1fffffff\);;

     df4:	04 31 11 77                                     	maxuhq \$r4 = \$r4, \$r4;;

     df8:	c5 37 15 f7 ff ff ff 00                         	maxuhq \$r5 = \$r5, 536870911 \(0x1fffffff\);;

     e00:	86 c0 17 79                                     	maxurbod \$r5 = \$r6;;

     e04:	46 c0 1b 79                                     	maxurhqd \$r6 = \$r6;;

     e08:	07 c0 1f 79                                     	maxurwpd \$r7 = \$r7;;

     e0c:	08 22 1d 77                                     	maxuwp \$r7 = \$r8, \$r8;;

     e10:	c9 2f 21 f7 ff ff ff 00                         	maxuwp.@ \$r8 = \$r9, 536870911 \(0x1fffffff\);;

     e18:	89 12 25 77                                     	maxuw \$r9 = \$r9, \$r10;;

     e1c:	0a f0 28 77                                     	maxuw \$r10 = \$r10, -64 \(0xffffffc0\);;

     e20:	0b 00 2c f7 00 00 80 07                         	maxuw \$r11 = \$r11, -8589934592 \(0xfffffffe00000000\);;

     e28:	0c 23 2d 75                                     	maxwp \$r11 = \$r12, \$r12;;

     e2c:	cd 27 35 f5 ff ff ff 00                         	maxwp \$r13 = \$r13, 536870911 \(0x1fffffff\);;

     e34:	ce 13 39 75                                     	maxw \$r14 = \$r14, \$r15;;

     e38:	10 f0 3c 75                                     	maxw \$r15 = \$r16, -64 \(0xffffffc0\);;

     e3c:	10 00 40 f5 00 00 80 07                         	maxw \$r16 = \$r16, -8589934592 \(0xfffffffe00000000\);;

     e44:	51 a4 46 74                                     	minbo \$r17 = \$r17, \$r17;;

     e48:	d2 af 4a f4 ff ff ff 00                         	minbo.@ \$r18 = \$r18, 536870911 \(0x1fffffff\);;

     e50:	d3 ff 48 e4 ff ff ff 87 ff ff ff 00             	mind \$r18 = \$r19, 2305843009213693951 \(0x1fffffffffffffff\);;

     e5c:	13 05 4d 74                                     	mind \$r19 = \$r19, \$r20;;

     e60:	14 f0 50 64                                     	mind \$r20 = \$r20, -64 \(0xffffffc0\);;

     e64:	15 00 54 e4 00 00 80 07                         	mind \$r21 = \$r21, -8589934592 \(0xfffffffe00000000\);;

     e6c:	d6 0f 55 f4 ff ff ff 00                         	mind.@ \$r21 = \$r22, 536870911 \(0x1fffffff\);;

     e74:	d6 35 59 74                                     	minhq \$r22 = \$r22, \$r23;;

     e78:	d7 37 5d f4 ff ff ff 00                         	minhq \$r23 = \$r23, 536870911 \(0x1fffffff\);;

     e80:	98 c0 63 74                                     	minrbod \$r24 = \$r24;;

     e84:	59 c0 63 74                                     	minrhqd \$r24 = \$r25;;

     e88:	19 c0 67 74                                     	minrwpd \$r25 = \$r25;;

     e8c:	9a a6 6a 76                                     	minubo \$r26 = \$r26, \$r26;;

     e90:	db af 6e f6 ff ff ff 00                         	minubo.@ \$r27 = \$r27, 536870911 \(0x1fffffff\);;

     e98:	dc ff 6c e6 ff ff ff 87 ff ff ff 00             	minud \$r27 = \$r28, 2305843009213693951 \(0x1fffffffffffffff\);;

     ea4:	5c 07 71 76                                     	minud \$r28 = \$r28, \$r29;;

     ea8:	1d f0 74 66                                     	minud \$r29 = \$r29, -64 \(0xffffffc0\);;

     eac:	1e 00 78 e6 00 00 80 07                         	minud \$r30 = \$r30, -8589934592 \(0xfffffffe00000000\);;

     eb4:	df 0f 79 f6 ff ff ff 00                         	minud.@ \$r30 = \$r31, 536870911 \(0x1fffffff\);;

     ebc:	1f 38 7d 76                                     	minuhq \$r31 = \$r31, \$r32;;

     ec0:	e0 37 81 f6 ff ff ff 00                         	minuhq \$r32 = \$r32, 536870911 \(0x1fffffff\);;

     ec8:	a1 c0 87 78                                     	minurbod \$r33 = \$r33;;

     ecc:	62 c0 87 78                                     	minurhqd \$r33 = \$r34;;

     ed0:	22 c0 8b 78                                     	minurwpd \$r34 = \$r34;;

     ed4:	e3 28 8d 76                                     	minuwp \$r35 = \$r35, \$r35;;

     ed8:	e4 2f 91 f6 ff ff ff 00                         	minuwp.@ \$r36 = \$r36, 536870911 \(0x1fffffff\);;

     ee0:	65 19 91 76                                     	minuw \$r36 = \$r37, \$r37;;

     ee4:	26 f0 94 76                                     	minuw \$r37 = \$r38, -64 \(0xffffffc0\);;

     ee8:	26 00 98 f6 00 00 80 07                         	minuw \$r38 = \$r38, -8589934592 \(0xfffffffe00000000\);;

     ef0:	e7 29 9d 74                                     	minwp \$r39 = \$r39, \$r39;;

     ef4:	e8 27 a1 f4 ff ff ff 00                         	minwp \$r40 = \$r40, 536870911 \(0x1fffffff\);;

     efc:	69 1a a1 74                                     	minw \$r40 = \$r41, \$r41;;

     f00:	2a f0 a4 74                                     	minw \$r41 = \$r42, -64 \(0xffffffc0\);;

     f04:	2a 00 a8 f4 00 00 80 07                         	minw \$r42 = \$r42, -8589934592 \(0xfffffffe00000000\);;

     f0c:	eb 1a b8 53                                     	mm212w \$r46r47 = \$r43, \$r43;;

     f10:	2b 0b c0 5b                                     	mma212w \$r48r49 = \$r43, \$r44;;

     f14:	2c 0b c0 5f                                     	mms212w \$r48r49 = \$r44, \$r44;;

     f18:	6d 1b c8 5c                                     	msbfdt \$r50r51 = \$r45, \$r45;;

     f1c:	ae 0b b4 54                                     	msbfd \$r45 = \$r46, \$r46;;

     f20:	ef 0b b8 56                                     	msbfhq \$r46 = \$r47, \$r47;;

     f24:	2f 4c c8 54                                     	msbfhwq \$r50r51 = \$r47, \$r48;;

     f28:	b4 2d d4 55                                     	msbfmwq \$r52r53 = \$r52r53, \$r54r55;;

     f2c:	30 1c d8 5e                                     	msbfsudt \$r54r55 = \$r48, \$r48;;

     f30:	71 4c e0 56                                     	msbfsuhwq \$r56r57 = \$r49, \$r49;;

     f34:	ba 2e e4 57                                     	msbfsumwq \$r56r57 = \$r58r59, \$r58r59;;

     f38:	b1 0c f0 5e                                     	msbfsuwdp \$r60r61 = \$r49, \$r50;;

     f3c:	f2 3c c8 56                                     	msbfsuwd \$r50 = \$r50, \$r51;;

     f40:	f3 37 cc d6 ff ff ff 10                         	msbfsuwd \$r51 = \$r51, 536870911 \(0x1fffffff\);;

     f48:	34 1d f0 5d                                     	msbfudt \$r60r61 = \$r52, \$r52;;

     f4c:	74 4d f8 55                                     	msbfuhwq \$r62r63 = \$r52, \$r53;;

     f50:	00 20 fc 56                                     	msbfumwq \$r62r63 = \$r0r1, \$r0r1;;

     f54:	75 0d 08 5d                                     	msbfuwdp \$r2r3 = \$r53, \$r53;;

     f58:	b6 3d d8 55                                     	msbfuwd \$r54 = \$r54, \$r54;;

     f5c:	f7 37 dc d5 ff ff ff 10                         	msbfuwd \$r55 = \$r55, 536870911 \(0x1fffffff\);;

     f64:	37 1e 08 5f                                     	msbfuzdt \$r2r3 = \$r55, \$r56;;

     f68:	38 0e 10 5c                                     	msbfwdp \$r4r5 = \$r56, \$r56;;

     f6c:	79 3e e4 54                                     	msbfwd \$r57 = \$r57, \$r57;;

     f70:	fa 37 e8 d4 ff ff ff 10                         	msbfwd \$r58 = \$r58, 536870911 \(0x1fffffff\);;

     f78:	fb 0e e8 55                                     	msbfwp \$r58 = \$r59, \$r59;;

     f7c:	86 21 14 54                                     	msbfwq \$r4r5 = \$r6r7, \$r6r7;;

     f80:	3c 3f ec 57                                     	msbfw \$r59 = \$r60, \$r60;;

     f84:	fd 37 f0 d7 ff ff ff 10                         	msbfw \$r60 = \$r61, 536870911 \(0x1fffffff\);;

     f8c:	7d 1f 24 58                                     	muldt \$r8r9 = \$r61, \$r61;;

     f90:	be 1f f8 54                                     	muld \$r62 = \$r62, \$r62;;

     f94:	ff 17 fc d4 ff ff ff 10                         	muld \$r63 = \$r63, 536870911 \(0x1fffffff\);;

     f9c:	00 10 fc 56                                     	mulhq \$r63 = \$r0, \$r0;;

     fa0:	c1 17 00 d6 ff ff ff 10                         	mulhq \$r0 = \$r1, 536870911 \(0x1fffffff\);;

     fa8:	41 40 20 58                                     	mulhwq \$r8r9 = \$r1, \$r1;;

     fac:	0a 23 28 55                                     	mulmwq \$r10r11 = \$r10r11, \$r12r13;;

     fb0:	82 10 34 5a                                     	mulsudt \$r12r13 = \$r2, \$r2;;

     fb4:	c2 40 38 5a                                     	mulsuhwq \$r14r15 = \$r2, \$r3;;

     fb8:	10 24 38 57                                     	mulsumwq \$r14r15 = \$r16r17, \$r16r17;;

     fbc:	c3 10 48 52                                     	mulsuwdp \$r18r19 = \$r3, \$r3;;

     fc0:	04 31 10 5a                                     	mulsuwd \$r4 = \$r4, \$r4;;

     fc4:	c5 37 14 da ff ff ff 10                         	mulsuwd \$r5 = \$r5, 536870911 \(0x1fffffff\);;

     fcc:	85 11 4c 59                                     	muludt \$r18r19 = \$r5, \$r6;;

     fd0:	86 41 50 59                                     	muluhwq \$r20r21 = \$r6, \$r6;;

     fd4:	96 25 50 56                                     	mulumwq \$r20r21 = \$r22r23, \$r22r23;;

     fd8:	c7 11 60 51                                     	muluwdp \$r24r25 = \$r7, \$r7;;

     fdc:	08 32 1c 59                                     	muluwd \$r7 = \$r8, \$r8;;

     fe0:	c9 37 20 d9 ff ff ff 10                         	muluwd \$r8 = \$r9, 536870911 \(0x1fffffff\);;

     fe8:	49 12 60 50                                     	mulwdp \$r24r25 = \$r9, \$r9;;

     fec:	8a 32 28 58                                     	mulwd \$r10 = \$r10, \$r10;;

     ff0:	cb 37 2c d8 ff ff ff 10                         	mulwd \$r11 = \$r11, 536870911 \(0x1fffffff\);;

     ff8:	0c 13 2c 55                                     	mulwp \$r11 = \$r12, \$r12;;

     ffc:	cd 17 34 d5 ff ff ff 10                         	mulwp \$r13 = \$r13, 536870911 \(0x1fffffff\);;

    1004:	1a 27 68 54                                     	mulwq \$r26r27 = \$r26r27, \$r28r29;;

    1008:	ce 33 38 5b                                     	mulw \$r14 = \$r14, \$r15;;

    100c:	d0 37 3c db ff ff ff 10                         	mulw \$r15 = \$r16, 536870911 \(0x1fffffff\);;

    1014:	d0 ff 40 e9 ff ff ff 87 ff ff ff 00             	nandd \$r16 = \$r16, 2305843009213693951 \(0x1fffffffffffffff\);;

    1020:	51 04 45 79                                     	nandd \$r17 = \$r17, \$r17;;

    1024:	12 f0 48 69                                     	nandd \$r18 = \$r18, -64 \(0xffffffc0\);;

    1028:	13 00 48 e9 00 00 80 07                         	nandd \$r18 = \$r19, -8589934592 \(0xfffffffe00000000\);;

    1030:	d3 0f 4d f9 ff ff ff 00                         	nandd.@ \$r19 = \$r19, 536870911 \(0x1fffffff\);;

    1038:	14 15 51 79                                     	nandw \$r20 = \$r20, \$r20;;

    103c:	15 f0 54 79                                     	nandw \$r21 = \$r21, -64 \(0xffffffc0\);;

    1040:	16 00 54 f9 00 00 80 07                         	nandw \$r21 = \$r22, -8589934592 \(0xfffffffe00000000\);;

    1048:	16 a0 5a f1 00 00 00 00                         	negbo \$r22 = \$r22;;

    1050:	17 00 5c 63                                     	negd \$r23 = \$r23;;

    1054:	18 30 5d f3 00 00 00 00                         	neghq \$r23 = \$r24;;

    105c:	18 b0 62 fd 00 00 00 00                         	negsbo \$r24 = \$r24;;

    1064:	19 40 65 fd 00 00 00 00                         	negsd \$r25 = \$r25;;

    106c:	1a 70 65 fd 00 00 00 00                         	negshq \$r25 = \$r26;;

    1074:	1a 60 69 fd 00 00 00 00                         	negswp \$r26 = \$r26;;

    107c:	1b 50 6d fd 00 00 00 00                         	negsw \$r27 = \$r27;;

    1084:	1c 20 6d f3 00 00 00 00                         	negwp \$r27 = \$r28;;

    108c:	1c 00 70 73                                     	negw \$r28 = \$r28;;

    1090:	00 f0 03 7f                                     	nop;;

    1094:	dd ff 74 eb ff ff ff 87 ff ff ff 00             	nord \$r29 = \$r29, 2305843009213693951 \(0x1fffffffffffffff\);;

    10a0:	9e 07 75 7b                                     	nord \$r29 = \$r30, \$r30;;

    10a4:	1f f0 78 6b                                     	nord \$r30 = \$r31, -64 \(0xffffffc0\);;

    10a8:	1f 00 7c eb 00 00 80 07                         	nord \$r31 = \$r31, -8589934592 \(0xfffffffe00000000\);;

    10b0:	e0 0f 81 fb ff ff ff 00                         	nord.@ \$r32 = \$r32, 536870911 \(0x1fffffff\);;

    10b8:	61 18 81 7b                                     	norw \$r32 = \$r33, \$r33;;

    10bc:	22 f0 84 7b                                     	norw \$r33 = \$r34, -64 \(0xffffffc0\);;

    10c0:	22 00 88 fb 00 00 80 07                         	norw \$r34 = \$r34, -8589934592 \(0xfffffffe00000000\);;

    10c8:	e3 ff 8c 6c                                     	notd \$r35 = \$r35;;

    10cc:	e4 ff 8c 7c                                     	notw \$r35 = \$r36;;

    10d0:	e4 ff 90 ed ff ff ff 87 ff ff ff 00             	nxord \$r36 = \$r36, 2305843009213693951 \(0x1fffffffffffffff\);;

    10dc:	65 09 95 7d                                     	nxord \$r37 = \$r37, \$r37;;

    10e0:	26 f0 98 6d                                     	nxord \$r38 = \$r38, -64 \(0xffffffc0\);;

    10e4:	27 00 98 ed 00 00 80 07                         	nxord \$r38 = \$r39, -8589934592 \(0xfffffffe00000000\);;

    10ec:	e7 0f 9d fd ff ff ff 00                         	nxord.@ \$r39 = \$r39, 536870911 \(0x1fffffff\);;

    10f4:	28 1a a1 7d                                     	nxorw \$r40 = \$r40, \$r40;;

    10f8:	29 f0 a4 7d                                     	nxorw \$r41 = \$r41, -64 \(0xffffffc0\);;

    10fc:	2a 00 a4 fd 00 00 80 07                         	nxorw \$r41 = \$r42, -8589934592 \(0xfffffffe00000000\);;

    1104:	ea ff a8 ea ff ff ff 87 ff ff ff 00             	ord \$r42 = \$r42, 2305843009213693951 \(0x1fffffffffffffff\);;

    1110:	eb 0a ad 7a                                     	ord \$r43 = \$r43, \$r43;;

    1114:	2c f0 b0 6a                                     	ord \$r44 = \$r44, -64 \(0xffffffc0\);;

    1118:	2d 00 b0 ea 00 00 80 07                         	ord \$r44 = \$r45, -8589934592 \(0xfffffffe00000000\);;

    1120:	ed 0f b5 fa ff ff ff 00                         	ord.@ \$r45 = \$r45, 536870911 \(0x1fffffff\);;

    1128:	ee ff b8 ef ff ff ff 87 ff ff ff 00             	ornd \$r46 = \$r46, 2305843009213693951 \(0x1fffffffffffffff\);;

    1134:	ef 0b b9 7f                                     	ornd \$r46 = \$r47, \$r47;;

    1138:	30 f0 bc 6f                                     	ornd \$r47 = \$r48, -64 \(0xffffffc0\);;

    113c:	30 00 c0 ef 00 00 80 07                         	ornd \$r48 = \$r48, -8589934592 \(0xfffffffe00000000\);;

    1144:	f1 0f c5 ff ff ff ff 00                         	ornd.@ \$r49 = \$r49, 536870911 \(0x1fffffff\);;

    114c:	b2 1c c5 7f                                     	ornw \$r49 = \$r50, \$r50;;

    1150:	33 f0 c8 7f                                     	ornw \$r50 = \$r51, -64 \(0xffffffc0\);;

    1154:	33 00 cc ff 00 00 80 07                         	ornw \$r51 = \$r51, -8589934592 \(0xfffffffe00000000\);;

    115c:	b4 c0 d3 71                                     	orrbod \$r52 = \$r52;;

    1160:	75 c0 d3 71                                     	orrhqd \$r52 = \$r53;;

    1164:	35 c0 d7 71                                     	orrwpd \$r53 = \$r53;;

    1168:	b6 1d d9 7a                                     	orw \$r54 = \$r54, \$r54;;

    116c:	37 f0 dc 7a                                     	orw \$r55 = \$r55, -64 \(0xffffffc0\);;

    1170:	38 00 dc fa 00 00 80 07                         	orw \$r55 = \$r56, -8589934592 \(0xfffffffe00000000\);;

    1178:	c0 ff e0 f0 ff ff ff 87 ff ff ff 00             	pcrel \$r56 = 2305843009213693951 \(0x1fffffffffffffff\);;

    1184:	3c 00 e0 f0 00 00 00 00                         	pcrel \$r56 = -549755813888 \(0xffffff8000000000\);;

    118c:	00 f0 e4 70                                     	pcrel \$r57 = -4096 \(0xfffff000\);;

    1190:	00 00 d0 0f                                     	ret;;

    1194:	00 00 d4 0f                                     	rfe;;

    1198:	b9 8e e6 7e                                     	rolwps \$r57 = \$r57, \$r58;;

    119c:	fa 41 ea 7e                                     	rolwps \$r58 = \$r58, 7 \(0x7\);;

    11a0:	fb 7e ee 7e                                     	rolw \$r59 = \$r59, \$r59;;

    11a4:	fc 31 f2 7e                                     	rolw \$r60 = \$r60, 7 \(0x7\);;

    11a8:	7d 8f f2 7f                                     	rorwps \$r60 = \$r61, \$r61;;

    11ac:	fe 41 f6 7f                                     	rorwps \$r61 = \$r62, 7 \(0x7\);;

    11b0:	fe 7f fa 7f                                     	rorw \$r62 = \$r62, \$r63;;

    11b4:	ff 31 fe 7f                                     	rorw \$r63 = \$r63, 7 \(0x7\);;

    11b8:	00 07 c8 0f                                     	rswap \$r0 = \$mmc;;

    11bc:	00 00 c8 0f                                     	rswap \$r0 = \$pc;;

    11c0:	00 00 c8 0f                                     	rswap \$r0 = \$pc;;

    11c4:	41 a0 06 71                                     	sbfbo \$r1 = \$r1, \$r1;;

    11c8:	c2 af 0a f1 ff ff ff 00                         	sbfbo.@ \$r2 = \$r2, 536870911 \(0x1fffffff\);;

    11d0:	c3 90 09 7f                                     	sbfcd.i \$r2 = \$r3, \$r3;;

    11d4:	c4 97 0d ff ff ff ff 00                         	sbfcd.i \$r3 = \$r4, 536870911 \(0x1fffffff\);;

    11dc:	44 81 11 7f                                     	sbfcd \$r4 = \$r4, \$r5;;

    11e0:	c5 87 15 ff ff ff ff 00                         	sbfcd \$r5 = \$r5, 536870911 \(0x1fffffff\);;

    11e8:	c6 ff 18 e3 ff ff ff 87 ff ff ff 00             	sbfd \$r6 = \$r6, 2305843009213693951 \(0x1fffffffffffffff\);;

    11f4:	c7 01 19 73                                     	sbfd \$r6 = \$r7, \$r7;;

    11f8:	08 f0 1c 63                                     	sbfd \$r7 = \$r8, -64 \(0xffffffc0\);;

    11fc:	08 00 20 e3 00 00 80 07                         	sbfd \$r8 = \$r8, -8589934592 \(0xfffffffe00000000\);;

    1204:	c9 0f 25 f3 ff ff ff 00                         	sbfd.@ \$r9 = \$r9, 536870911 \(0x1fffffff\);;

    120c:	8a 32 25 73                                     	sbfhq \$r9 = \$r10, \$r10;;

    1210:	cb 37 29 f3 ff ff ff 00                         	sbfhq \$r10 = \$r11, 536870911 \(0x1fffffff\);;

    1218:	0b b3 2e 7d                                     	sbfsbo \$r11 = \$r11, \$r12;;

    121c:	cd bf 32 fd ff ff ff 00                         	sbfsbo.@ \$r12 = \$r13, 536870911 \(0x1fffffff\);;

    1224:	8e 43 35 7d                                     	sbfsd \$r13 = \$r14, \$r14;;

    1228:	cf 47 3d fd ff ff ff 00                         	sbfsd \$r15 = \$r15, 536870911 \(0x1fffffff\);;

    1230:	10 74 41 7d                                     	sbfshq \$r16 = \$r16, \$r16;;

    1234:	d1 7f 45 fd ff ff ff 00                         	sbfshq.@ \$r17 = \$r17, 536870911 \(0x1fffffff\);;

    123c:	92 64 45 7d                                     	sbfswp \$r17 = \$r18, \$r18;;

    1240:	d3 67 49 fd ff ff ff 00                         	sbfswp \$r18 = \$r19, 536870911 \(0x1fffffff\);;

    1248:	13 55 4d 7d                                     	sbfsw \$r19 = \$r19, \$r20;;

    124c:	d4 57 51 fd ff ff ff 00                         	sbfsw \$r20 = \$r20, 536870911 \(0x1fffffff\);;

    1254:	55 b5 56 7f                                     	sbfusbo \$r21 = \$r21, \$r21;;

    1258:	d6 bf 5a ff ff ff ff 00                         	sbfusbo.@ \$r22 = \$r22, 536870911 \(0x1fffffff\);;

    1260:	d7 45 59 7f                                     	sbfusd \$r22 = \$r23, \$r23;;

    1264:	d8 47 5d ff ff ff ff 00                         	sbfusd \$r23 = \$r24, 536870911 \(0x1fffffff\);;

    126c:	58 76 61 7f                                     	sbfushq \$r24 = \$r24, \$r25;;

    1270:	d9 7f 65 ff ff ff ff 00                         	sbfushq.@ \$r25 = \$r25, 536870911 \(0x1fffffff\);;

    1278:	9a 66 69 7f                                     	sbfuswp \$r26 = \$r26, \$r26;;

    127c:	db 67 6d ff ff ff ff 00                         	sbfuswp \$r27 = \$r27, 536870911 \(0x1fffffff\);;

    1284:	1c 57 6d 7f                                     	sbfusw \$r27 = \$r28, \$r28;;

    1288:	dd 57 71 ff ff ff ff 00                         	sbfusw \$r28 = \$r29, 536870911 \(0x1fffffff\);;

    1290:	9d 97 75 7d                                     	sbfuwd \$r29 = \$r29, \$r30;;

    1294:	de 97 79 fd ff ff ff 00                         	sbfuwd \$r30 = \$r30, 536870911 \(0x1fffffff\);;

    129c:	df 87 7d 7d                                     	sbfwd \$r31 = \$r31, \$r31;;

    12a0:	e0 87 81 fd ff ff ff 00                         	sbfwd \$r32 = \$r32, 536870911 \(0x1fffffff\);;

    12a8:	61 28 81 73                                     	sbfwp \$r32 = \$r33, \$r33;;

    12ac:	e2 2f 85 f3 ff ff ff 00                         	sbfwp.@ \$r33 = \$r34, 536870911 \(0x1fffffff\);;

    12b4:	e2 18 89 73                                     	sbfw \$r34 = \$r34, \$r35;;

    12b8:	23 f0 8c 73                                     	sbfw \$r35 = \$r35, -64 \(0xffffffc0\);;

    12bc:	24 00 90 f3 00 00 80 07                         	sbfw \$r36 = \$r36, -8589934592 \(0xfffffffe00000000\);;

    12c4:	65 b9 92 77                                     	sbfx16bo \$r36 = \$r37, \$r37;;

    12c8:	e6 b7 96 f7 ff ff ff 00                         	sbfx16bo \$r37 = \$r38, 536870911 \(0x1fffffff\);;

    12d0:	e6 49 99 77                                     	sbfx16d \$r38 = \$r38, \$r39;;

    12d4:	e7 4f 9d f7 ff ff ff 00                         	sbfx16d.@ \$r39 = \$r39, 536870911 \(0x1fffffff\);;

    12dc:	28 7a a1 77                                     	sbfx16hq \$r40 = \$r40, \$r40;;

    12e0:	e9 77 a5 f7 ff ff ff 00                         	sbfx16hq \$r41 = \$r41, 536870911 \(0x1fffffff\);;

    12e8:	aa 9a a5 77                                     	sbfx16uwd \$r41 = \$r42, \$r42;;

    12ec:	eb 97 a9 f7 ff ff ff 00                         	sbfx16uwd \$r42 = \$r43, 536870911 \(0x1fffffff\);;

    12f4:	2b 8b ad 77                                     	sbfx16wd \$r43 = \$r43, \$r44;;

    12f8:	ec 87 b1 f7 ff ff ff 00                         	sbfx16wd \$r44 = \$r44, 536870911 \(0x1fffffff\);;

    1300:	6d 6b b5 77                                     	sbfx16wp \$r45 = \$r45, \$r45;;

    1304:	ee 6f b9 f7 ff ff ff 00                         	sbfx16wp.@ \$r46 = \$r46, 536870911 \(0x1fffffff\);;

    130c:	ef 5b b9 77                                     	sbfx16w \$r46 = \$r47, \$r47;;

    1310:	f0 57 bd f7 ff ff ff 00                         	sbfx16w \$r47 = \$r48, 536870911 \(0x1fffffff\);;

    1318:	70 bc c2 71                                     	sbfx2bo \$r48 = \$r48, \$r49;;

    131c:	f1 b7 c6 f1 ff ff ff 00                         	sbfx2bo \$r49 = \$r49, 536870911 \(0x1fffffff\);;

    1324:	b2 4c c9 71                                     	sbfx2d \$r50 = \$r50, \$r50;;

    1328:	f3 4f cd f1 ff ff ff 00                         	sbfx2d.@ \$r51 = \$r51, 536870911 \(0x1fffffff\);;

    1330:	34 7d cd 71                                     	sbfx2hq \$r51 = \$r52, \$r52;;

    1334:	f5 77 d1 f1 ff ff ff 00                         	sbfx2hq \$r52 = \$r53, 536870911 \(0x1fffffff\);;

    133c:	b5 9d d5 71                                     	sbfx2uwd \$r53 = \$r53, \$r54;;

    1340:	f6 97 d9 f1 ff ff ff 00                         	sbfx2uwd \$r54 = \$r54, 536870911 \(0x1fffffff\);;

    1348:	f7 8d dd 71                                     	sbfx2wd \$r55 = \$r55, \$r55;;

    134c:	f8 87 e1 f1 ff ff ff 00                         	sbfx2wd \$r56 = \$r56, 536870911 \(0x1fffffff\);;

    1354:	79 6e e1 71                                     	sbfx2wp \$r56 = \$r57, \$r57;;

    1358:	fa 6f e5 f1 ff ff ff 00                         	sbfx2wp.@ \$r57 = \$r58, 536870911 \(0x1fffffff\);;

    1360:	fa 5e e9 71                                     	sbfx2w \$r58 = \$r58, \$r59;;

    1364:	fb 57 ed f1 ff ff ff 00                         	sbfx2w \$r59 = \$r59, 536870911 \(0x1fffffff\);;

    136c:	3c 4f f1 79                                     	sbfx32d \$r60 = \$r60, \$r60;;

    1370:	fd 47 f5 f9 ff ff ff 00                         	sbfx32d \$r61 = \$r61, 536870911 \(0x1fffffff\);;

    1378:	be 9f f5 79                                     	sbfx32uwd \$r61 = \$r62, \$r62;;

    137c:	ff 97 f9 f9 ff ff ff 00                         	sbfx32uwd \$r62 = \$r63, 536870911 \(0x1fffffff\);;

    1384:	3f 80 fd 79                                     	sbfx32wd \$r63 = \$r63, \$r0;;

    1388:	c0 87 01 f9 ff ff ff 00                         	sbfx32wd \$r0 = \$r0, 536870911 \(0x1fffffff\);;

    1390:	41 50 05 79                                     	sbfx32w \$r1 = \$r1, \$r1;;

    1394:	c2 57 09 f9 ff ff ff 00                         	sbfx32w \$r2 = \$r2, 536870911 \(0x1fffffff\);;

    139c:	c3 b0 0a 73                                     	sbfx4bo \$r2 = \$r3, \$r3;;

    13a0:	c4 bf 0e f3 ff ff ff 00                         	sbfx4bo.@ \$r3 = \$r4, 536870911 \(0x1fffffff\);;

    13a8:	44 41 11 73                                     	sbfx4d \$r4 = \$r4, \$r5;;

    13ac:	c5 47 15 f3 ff ff ff 00                         	sbfx4d \$r5 = \$r5, 536870911 \(0x1fffffff\);;

    13b4:	86 71 19 73                                     	sbfx4hq \$r6 = \$r6, \$r6;;

    13b8:	c7 7f 1d f3 ff ff ff 00                         	sbfx4hq.@ \$r7 = \$r7, 536870911 \(0x1fffffff\);;

    13c0:	08 92 1d 73                                     	sbfx4uwd \$r7 = \$r8, \$r8;;

    13c4:	c9 97 21 f3 ff ff ff 00                         	sbfx4uwd \$r8 = \$r9, 536870911 \(0x1fffffff\);;

    13cc:	89 82 25 73                                     	sbfx4wd \$r9 = \$r9, \$r10;;

    13d0:	ca 87 29 f3 ff ff ff 00                         	sbfx4wd \$r10 = \$r10, 536870911 \(0x1fffffff\);;

    13d8:	cb 62 2d 73                                     	sbfx4wp \$r11 = \$r11, \$r11;;

    13dc:	cc 67 31 f3 ff ff ff 00                         	sbfx4wp \$r12 = \$r12, 536870911 \(0x1fffffff\);;

    13e4:	8d 53 35 73                                     	sbfx4w \$r13 = \$r13, \$r14;;

    13e8:	cf 57 39 f3 ff ff ff 00                         	sbfx4w \$r14 = \$r15, 536870911 \(0x1fffffff\);;

    13f0:	10 44 3d 7b                                     	sbfx64d \$r15 = \$r16, \$r16;;

    13f4:	d1 4f 41 fb ff ff ff 00                         	sbfx64d.@ \$r16 = \$r17, 536870911 \(0x1fffffff\);;

    13fc:	91 94 45 7b                                     	sbfx64uwd \$r17 = \$r17, \$r18;;

    1400:	d2 97 49 fb ff ff ff 00                         	sbfx64uwd \$r18 = \$r18, 536870911 \(0x1fffffff\);;

    1408:	d3 84 4d 7b                                     	sbfx64wd \$r19 = \$r19, \$r19;;

    140c:	d4 87 51 fb ff ff ff 00                         	sbfx64wd \$r20 = \$r20, 536870911 \(0x1fffffff\);;

    1414:	55 55 51 7b                                     	sbfx64w \$r20 = \$r21, \$r21;;

    1418:	d6 57 55 fb ff ff ff 00                         	sbfx64w \$r21 = \$r22, 536870911 \(0x1fffffff\);;

    1420:	d6 b5 5a 75                                     	sbfx8bo \$r22 = \$r22, \$r23;;

    1424:	d7 b7 5e f5 ff ff ff 00                         	sbfx8bo \$r23 = \$r23, 536870911 \(0x1fffffff\);;

    142c:	18 46 61 75                                     	sbfx8d \$r24 = \$r24, \$r24;;

    1430:	d9 4f 65 f5 ff ff ff 00                         	sbfx8d.@ \$r25 = \$r25, 536870911 \(0x1fffffff\);;

    1438:	9a 76 65 75                                     	sbfx8hq \$r25 = \$r26, \$r26;;

    143c:	db 77 69 f5 ff ff ff 00                         	sbfx8hq \$r26 = \$r27, 536870911 \(0x1fffffff\);;

    1444:	1b 97 6d 75                                     	sbfx8uwd \$r27 = \$r27, \$r28;;

    1448:	dc 97 71 f5 ff ff ff 00                         	sbfx8uwd \$r28 = \$r28, 536870911 \(0x1fffffff\);;

    1450:	5d 87 75 75                                     	sbfx8wd \$r29 = \$r29, \$r29;;

    1454:	de 87 79 f5 ff ff ff 00                         	sbfx8wd \$r30 = \$r30, 536870911 \(0x1fffffff\);;

    145c:	df 67 79 75                                     	sbfx8wp \$r30 = \$r31, \$r31;;

    1460:	e0 6f 7d f5 ff ff ff 00                         	sbfx8wp.@ \$r31 = \$r32, 536870911 \(0x1fffffff\);;

    1468:	60 58 81 75                                     	sbfx8w \$r32 = \$r32, \$r33;;

    146c:	e1 57 85 f5 ff ff ff 00                         	sbfx8w \$r33 = \$r33, 536870911 \(0x1fffffff\);;

    1474:	e2 ff 8a ee ff ff ff 87 ff ff ff 00             	sbmm8 \$r34 = \$r34, 2305843009213693951 \(0x1fffffffffffffff\);;

    1480:	e3 08 8a 7e                                     	sbmm8 \$r34 = \$r35, \$r35;;

    1484:	24 f0 8e 6e                                     	sbmm8 \$r35 = \$r36, -64 \(0xffffffc0\);;

    1488:	24 00 92 ee 00 00 80 07                         	sbmm8 \$r36 = \$r36, -8589934592 \(0xfffffffe00000000\);;

    1490:	e5 0f 96 fe ff ff ff 00                         	sbmm8.@ \$r37 = \$r37, 536870911 \(0x1fffffff\);;

    1498:	e6 ff 96 ef ff ff ff 87 ff ff ff 00             	sbmmt8 \$r37 = \$r38, 2305843009213693951 \(0x1fffffffffffffff\);;

    14a4:	e6 09 9a 7f                                     	sbmmt8 \$r38 = \$r38, \$r39;;

    14a8:	27 f0 9e 6f                                     	sbmmt8 \$r39 = \$r39, -64 \(0xffffffc0\);;

    14ac:	28 00 a2 ef 00 00 80 07                         	sbmmt8 \$r40 = \$r40, -8589934592 \(0xfffffffe00000000\);;

    14b4:	e9 0f a2 ff ff ff ff 00                         	sbmmt8.@ \$r40 = \$r41, 536870911 \(0x1fffffff\);;

    14bc:	69 ea ab 30                                     	sb \$r41\[\$r41\] = \$r42;;

    14c0:	ea ff a9 b0 ff ff ff 9f ff ff ff 18             	sb 2305843009213693951 \(0x1fffffffffffffff\)\[\$r42\] = \$r42;;

    14cc:	eb 4a af b0 00 00 00 98 00 00 80 1f             	sb.dlez \$r43\? -1125899906842624 \(0xfffc000000000000\)\[\$r43\] = \$r43;;

    14d8:	2c 5b b3 b0 00 00 80 1f                         	sb.dgtz \$r44\? -8388608 \(0xff800000\)\[\$r44\] = \$r44;;

    14e0:	6d 6b b7 30                                     	sb.odd \$r45\? \[\$r45\] = \$r45;;

    14e4:	2e f0 b9 30                                     	sb -64 \(0xffffffc0\)\[\$r46\] = \$r46;;

    14e8:	2e 00 bd b0 00 00 80 1f                         	sb -8589934592 \(0xfffffffe00000000\)\[\$r46\] = \$r47;;

    14f0:	2f 00 e4 0f                                     	scall \$r47;;

    14f4:	ff 01 e0 0f                                     	scall 511 \(0x1ff\);;

    14f8:	f0 fb c3 33                                     	sd.xs \$r47\[\$r48\] = \$r48;;

    14fc:	f0 ff c5 b3 ff ff ff 9f ff ff ff 18             	sd 2305843009213693951 \(0x1fffffffffffffff\)\[\$r48\] = \$r49;;

    1508:	71 7c cb b3 00 00 00 98 00 00 80 1f             	sd.even \$r49\? -1125899906842624 \(0xfffc000000000000\)\[\$r49\] = \$r50;;

    1514:	b2 8c cf b3 00 00 80 1f                         	sd.wnez \$r50\? -8388608 \(0xff800000\)\[\$r50\] = \$r51;;

    151c:	f3 9c d3 33                                     	sd.weqz \$r51\? \[\$r51\] = \$r52;;

    1520:	34 f0 d1 33                                     	sd -64 \(0xffffffc0\)\[\$r52\] = \$r52;;

    1524:	35 00 d5 b3 00 00 80 1f                         	sd -8589934592 \(0xfffffffe00000000\)\[\$r53\] = \$r53;;

    152c:	35 07 c0 0f                                     	set \$mmc = \$r53;;

    1530:	f6 00 c0 0f                                     	set \$ra = \$r54;;

    1534:	76 00 c0 0f                                     	set \$ps = \$r54;;

    1538:	76 00 c0 0f                                     	set \$ps = \$r54;;

    153c:	f7 ed df 31                                     	sh \$r55\[\$r55\] = \$r55;;

    1540:	f8 ff e1 b1 ff ff ff 9f ff ff ff 18             	sh 2305843009213693951 \(0x1fffffffffffffff\)\[\$r56\] = \$r56;;

    154c:	39 ae e7 b1 00 00 00 98 00 00 80 1f             	sh.wltz \$r56\? -1125899906842624 \(0xfffc000000000000\)\[\$r57\] = \$r57;;

    1558:	7a be eb b1 00 00 80 1f                         	sh.wgez \$r57\? -8388608 \(0xff800000\)\[\$r58\] = \$r58;;

    1560:	bb ce ef 31                                     	sh.wlez \$r58\? \[\$r59\] = \$r59;;

    1564:	3b f0 f1 31                                     	sh -64 \(0xffffffc0\)\[\$r59\] = \$r60;;

    1568:	3c 00 f1 b1 00 00 80 1f                         	sh -8589934592 \(0xfffffffe00000000\)\[\$r60\] = \$r60;;

    1570:	00 00 a4 0f                                     	sleep;;

    1574:	7d ff f6 79                                     	sllbos \$r61 = \$r61, \$r61;;

    1578:	fe e1 fa 79                                     	sllbos \$r62 = \$r62, 7 \(0x7\);;

    157c:	ff 6f fa 79                                     	slld \$r62 = \$r63, \$r63;;

    1580:	c0 21 fe 79                                     	slld \$r63 = \$r0, 7 \(0x7\);;

    1584:	40 90 02 79                                     	sllhqs \$r0 = \$r0, \$r1;;

    1588:	c1 51 06 79                                     	sllhqs \$r1 = \$r1, 7 \(0x7\);;

    158c:	82 80 0a 79                                     	sllwps \$r2 = \$r2, \$r2;;

    1590:	c3 41 0e 79                                     	sllwps \$r3 = \$r3, 7 \(0x7\);;

    1594:	04 71 0e 79                                     	sllw \$r3 = \$r4, \$r4;;

    1598:	c5 31 12 79                                     	sllw \$r4 = \$r5, 7 \(0x7\);;

    159c:	85 f1 16 7c                                     	slsbos \$r5 = \$r5, \$r6;;

    15a0:	c6 e1 1a 7c                                     	slsbos \$r6 = \$r6, 7 \(0x7\);;

    15a4:	c7 61 1e 7c                                     	slsd \$r7 = \$r7, \$r7;;

    15a8:	c8 21 22 7c                                     	slsd \$r8 = \$r8, 7 \(0x7\);;

    15ac:	49 92 22 7c                                     	slshqs \$r8 = \$r9, \$r9;;

    15b0:	ca 51 26 7c                                     	slshqs \$r9 = \$r10, 7 \(0x7\);;

    15b4:	ca 82 2a 7c                                     	slswps \$r10 = \$r10, \$r11;;

    15b8:	cb 41 2e 7c                                     	slswps \$r11 = \$r11, 7 \(0x7\);;

    15bc:	4c 73 32 7c                                     	slsw \$r12 = \$r12, \$r13;;

    15c0:	ce 31 36 7c                                     	slsw \$r13 = \$r14, 7 \(0x7\);;

    15c4:	cf f3 3a 7d                                     	slusbos \$r14 = \$r15, \$r15;;

    15c8:	d0 e1 42 7d                                     	slusbos \$r16 = \$r16, 7 \(0x7\);;

    15cc:	51 64 42 7d                                     	slusd \$r16 = \$r17, \$r17;;

    15d0:	d2 21 46 7d                                     	slusd \$r17 = \$r18, 7 \(0x7\);;

    15d4:	d2 94 4a 7d                                     	slushqs \$r18 = \$r18, \$r19;;

    15d8:	d3 51 4e 7d                                     	slushqs \$r19 = \$r19, 7 \(0x7\);;

    15dc:	14 85 52 7d                                     	sluswps \$r20 = \$r20, \$r20;;

    15e0:	d5 41 56 7d                                     	sluswps \$r21 = \$r21, 7 \(0x7\);;

    15e4:	96 75 56 7d                                     	slusw \$r21 = \$r22, \$r22;;

    15e8:	d7 31 5a 7d                                     	slusw \$r22 = \$r23, 7 \(0x7\);;

    15ec:	d7 f5 b7 34                                     	so.xs \$r23\[\$r23\] = \$r44r45r46r47;;

    15f0:	d8 ff c5 b4 ff ff ff 9f ff ff ff 18             	so 2305843009213693951 \(0x1fffffffffffffff\)\[\$r24\] = \$r48r49r50r51;;

    15fc:	18 36 df b4 00 00 00 98 00 00 80 1f             	so.u3 \$r24\? -1125899906842624 \(0xfffc000000000000\)\[\$r24\] = \$r52r53r54r55;;

    1608:	59 46 ef b4 00 00 80 1f                         	so.mt \$r25\? -8388608 \(0xff800000\)\[\$r25\] = \$r56r57r58r59;;

    1610:	5a 56 ff 34                                     	so.mf \$r25\? \[\$r26\] = \$r60r61r62r63;;

    1614:	9a d6 07 b4 00 00 00 98 00 00 80 1f             	so.wgtz \$r26\? -1125899906842624 \(0xfffc000000000000\)\[\$r26\] = \$r0r1r2r3;;

    1620:	db 06 17 b4 00 00 80 1f                         	so.dnez \$r27\? -8388608 \(0xff800000\)\[\$r27\] = \$r4r5r6r7;;

    1628:	dc 16 27 34                                     	so.deqz \$r27\? \[\$r28\] = \$r8r9r10r11;;

    162c:	1c f0 35 34                                     	so -64 \(0xffffffc0\)\[\$r28\] = \$r12r13r14r15;;

    1630:	1c 00 45 b4 00 00 80 1f                         	so -8589934592 \(0xfffffffe00000000\)\[\$r28\] = \$r16r17r18r19;;

    1638:	5d e7 73 34                                     	sq \$r29\[\$r29\] = \$r28r29;;

    163c:	dd ff 79 b4 ff ff ff 9f ff ff ff 18             	sq 2305843009213693951 \(0x1fffffffffffffff\)\[\$r29\] = \$r30r31;;

    1648:	9e 27 7b b4 00 00 00 98 00 00 80 1f             	sq.dltz \$r30\? -1125899906842624 \(0xfffc000000000000\)\[\$r30\] = \$r30r31;;

    1654:	9f 37 83 b4 00 00 80 1f                         	sq.dgez \$r30\? -8388608 \(0xff800000\)\[\$r31\] = \$r32r33;;

    165c:	df 47 83 34                                     	sq.dlez \$r31\? \[\$r31\] = \$r32r33;;

    1660:	20 f0 89 34                                     	sq -64 \(0xffffffc0\)\[\$r32\] = \$r34r35;;

    1664:	20 00 89 b4 00 00 80 1f                         	sq -8589934592 \(0xfffffffe00000000\)\[\$r32\] = \$r34r35;;

    166c:	61 f8 82 7a                                     	srabos \$r32 = \$r33, \$r33;;

    1670:	e2 e1 86 7a                                     	srabos \$r33 = \$r34, 7 \(0x7\);;

    1674:	e2 68 8a 7a                                     	srad \$r34 = \$r34, \$r35;;

    1678:	e3 21 8e 7a                                     	srad \$r35 = \$r35, 7 \(0x7\);;

    167c:	24 99 92 7a                                     	srahqs \$r36 = \$r36, \$r36;;

    1680:	e5 51 96 7a                                     	srahqs \$r37 = \$r37, 7 \(0x7\);;

    1684:	a6 89 96 7a                                     	srawps \$r37 = \$r38, \$r38;;

    1688:	e7 41 9a 7a                                     	srawps \$r38 = \$r39, 7 \(0x7\);;

    168c:	27 7a 9e 7a                                     	sraw \$r39 = \$r39, \$r40;;

    1690:	e8 31 a2 7a                                     	sraw \$r40 = \$r40, 7 \(0x7\);;

    1694:	69 fa a6 7b                                     	srlbos \$r41 = \$r41, \$r41;;

    1698:	ea e1 aa 7b                                     	srlbos \$r42 = \$r42, 7 \(0x7\);;

    169c:	eb 6a aa 7b                                     	srld \$r42 = \$r43, \$r43;;

    16a0:	ec 21 ae 7b                                     	srld \$r43 = \$r44, 7 \(0x7\);;

    16a4:	6c 9b b2 7b                                     	srlhqs \$r44 = \$r44, \$r45;;

    16a8:	ed 51 b6 7b                                     	srlhqs \$r45 = \$r45, 7 \(0x7\);;

    16ac:	ae 8b ba 7b                                     	srlwps \$r46 = \$r46, \$r46;;

    16b0:	ef 41 be 7b                                     	srlwps \$r47 = \$r47, 7 \(0x7\);;

    16b4:	30 7c be 7b                                     	srlw \$r47 = \$r48, \$r48;;

    16b8:	f1 31 c2 7b                                     	srlw \$r48 = \$r49, 7 \(0x7\);;

    16bc:	b1 fc c6 78                                     	srsbos \$r49 = \$r49, \$r50;;

    16c0:	f2 e1 ca 78                                     	srsbos \$r50 = \$r50, 7 \(0x7\);;

    16c4:	f3 6c ce 78                                     	srsd \$r51 = \$r51, \$r51;;

    16c8:	f4 21 d2 78                                     	srsd \$r52 = \$r52, 7 \(0x7\);;

    16cc:	75 9d d2 78                                     	srshqs \$r52 = \$r53, \$r53;;

    16d0:	f6 51 d6 78                                     	srshqs \$r53 = \$r54, 7 \(0x7\);;

    16d4:	f6 8d da 78                                     	srswps \$r54 = \$r54, \$r55;;

    16d8:	f7 41 de 78                                     	srswps \$r55 = \$r55, 7 \(0x7\);;

    16dc:	38 7e e2 78                                     	srsw \$r56 = \$r56, \$r56;;

    16e0:	f9 31 e6 78                                     	srsw \$r57 = \$r57, 7 \(0x7\);;

    16e4:	00 00 a8 0f                                     	stop;;

    16e8:	ba ae e5 7e                                     	stsud \$r57 = \$r58, \$r58;;

    16ec:	fb a7 e9 fe ff ff ff 00                         	stsud \$r58 = \$r59, 536870911 \(0x1fffffff\);;

    16f4:	3b ff ed 7e                                     	stsuhq \$r59 = \$r59, \$r60;;

    16f8:	fc ff f1 fe ff ff ff 00                         	stsuhq.@ \$r60 = \$r60, 536870911 \(0x1fffffff\);;

    1700:	7d ef f5 7e                                     	stsuwp \$r61 = \$r61, \$r61;;

    1704:	fe e7 f9 fe ff ff ff 00                         	stsuwp \$r62 = \$r62, 536870911 \(0x1fffffff\);;

    170c:	ff bf f9 7e                                     	stsuw \$r62 = \$r63, \$r63;;

    1710:	c0 b7 fd fe ff ff ff 00                         	stsuw \$r63 = \$r0, 536870911 \(0x1fffffff\);;

    1718:	00 f0 07 32                                     	sw.xs \$r0\[\$r0\] = \$r1;;

    171c:	c1 ff 05 b2 ff ff ff 9f ff ff ff 18             	sw 2305843009213693951 \(0x1fffffffffffffff\)\[\$r1\] = \$r1;;

    1728:	82 50 0b b2 00 00 00 98 00 00 80 1f             	sw.dgtz \$r2\? -1125899906842624 \(0xfffc000000000000\)\[\$r2\] = \$r2;;

    1734:	c3 60 0f b2 00 00 80 1f                         	sw.odd \$r3\? -8388608 \(0xff800000\)\[\$r3\] = \$r3;;

    173c:	04 71 13 32                                     	sw.even \$r4\? \[\$r4\] = \$r4;;

    1740:	05 f0 15 32                                     	sw -64 \(0xffffffc0\)\[\$r5\] = \$r5;;

    1744:	05 00 19 b2 00 00 80 1f                         	sw -8589934592 \(0xfffffffe00000000\)\[\$r5\] = \$r6;;

    174c:	06 70 1b 68                                     	sxbd \$r6 = \$r6;;

    1750:	07 f0 1f 68                                     	sxhd \$r7 = \$r7;;

    1754:	08 50 1e 76                                     	sxlbhq \$r7 = \$r8;;

    1758:	08 40 22 76                                     	sxlhwp \$r8 = \$r8;;

    175c:	09 50 26 77                                     	sxmbhq \$r9 = \$r9;;

    1760:	0a 40 26 77                                     	sxmhwp \$r9 = \$r10;;

    1764:	0a f0 2b 69                                     	sxwd \$r10 = \$r10;;

    1768:	0b 00 b4 0f                                     	syncgroup \$r11;;

    176c:	00 00 8c 0f                                     	tlbdinval;;

    1770:	00 00 90 0f                                     	tlbiinval;;

    1774:	00 00 84 0f                                     	tlbprobe;;

    1778:	00 00 80 0f                                     	tlbread;;

    177c:	00 00 88 0f                                     	tlbwrite;;

    1780:	0b 00 b0 0f                                     	waitit \$r11;;

    1784:	4b 00 b8 0f                                     	wfxl \$ps, \$r11;;

    1788:	8c 00 b8 0f                                     	wfxl \$pcr, \$r12;;

    178c:	4c 00 b8 0f                                     	wfxl \$ps, \$r12;;

    1790:	4d 00 bc 0f                                     	wfxm \$ps, \$r13;;

    1794:	8d 00 bc 0f                                     	wfxm \$pcr, \$r13;;

    1798:	8e 00 bc 0f                                     	wfxm \$pcr, \$r14;;

    179c:	0e 80 5c 00                                     	xaccesso \$r20r21r22r23 = \$a0..a1, \$r14;;

    17a0:	4f 80 6c 00                                     	xaccesso \$r24r25r26r27 = \$a0..a3, \$r15;;

    17a4:	cf 80 7c 00                                     	xaccesso \$r28r29r30r31 = \$a0..a7, \$r15;;

    17a8:	d0 81 8c 00                                     	xaccesso \$r32r33r34r35 = \$a0..a15, \$r16;;

    17ac:	d0 83 9c 00                                     	xaccesso \$r36r37r38r39 = \$a0..a31, \$r16;;

    17b0:	d0 87 ac 00                                     	xaccesso \$r40r41r42r43 = \$a0..a63, \$r16;;

    17b4:	91 80 00 01                                     	xaligno \$a0 = \$a2..a3, \$r17;;

    17b8:	51 81 00 01                                     	xaligno \$a0 = \$a4..a7, \$r17;;

    17bc:	d1 82 00 01                                     	xaligno \$a0 = \$a8..a15, \$r17;;

    17c0:	d2 85 04 01                                     	xaligno \$a1 = \$a16..a31, \$r18;;

    17c4:	d2 8b 04 01                                     	xaligno \$a1 = \$a32..a63, \$r18;;

    17c8:	d2 87 04 01                                     	xaligno \$a1 = \$a0..a63, \$r18;;

    17cc:	82 60 0b 07                                     	xandno \$a2 = \$a2, \$a2;;

    17d0:	c3 00 0f 07                                     	xando \$a3 = \$a3, \$a3;;

    17d4:	04 01 13 05                                     	xclampwo \$a4 = \$a4, \$a4;;

    17d8:	40 01 14 01                                     	xcopyo \$a5 = \$a5;;

    17dc:	00 01 05 07                                     	xcopyv \$a0a1a2a3 = \$a4a5a6a7;;

    17e0:	00 00 04 07                                     	xcopyx \$a0a1 = \$a0a1;;

    17e4:	46 c1 0a 04                                     	xffma44hw.rna.s \$a2a3 = \$a5, \$a6;;

    17e8:	87 01 1a 05                                     	xfmaxhx \$a6 = \$a6, \$a7;;

    17ec:	c8 01 1d 05                                     	xfminhx \$a7 = \$a7, \$a8;;

    17f0:	04 51 0b 04                                     	xfmma484hw.rnz \$a2a3 = \$a4a5, \$a4a5;;

    17f4:	80 e1 20 05                                     	xfnarrow44wh.ro.s \$a8 = \$a6a7;;

    17f8:	53 72 23 01                                     	xfscalewo \$a8 = \$a9, \$r19;;

    17fc:	d3 e4 23 2a                                     	xlo.u.q0 \$a8a9a10a11 = \$r19\[\$r19\];;

    1800:	14 f5 27 23                                     	xlo.us.xs \$a9 = \$r20\[\$r20\];;

    1804:	15 05 37 a8 00 00 00 98 00 00 80 1f             	xlo.dnez.q1 \$r20\? \$a12a13a14a15 = -1125899906842624 \(0xfffc000000000000\)\[\$r21\];;

    1810:	55 15 4b a9 00 00 80 1f                         	xlo.s.deqz.q2 \$r21\? \$a16a17a18a19 = -8388608 \(0xff800000\)\[\$r21\];;

    1818:	96 25 5f 2a                                     	xlo.u.wnez.q3 \$r22\? \$a20a21a22a23 = \[\$r22\];;

    181c:	97 35 27 a3 00 00 00 98 00 00 80 1f             	xlo.us.weqz \$r22\? \$a9 = -1125899906842624 \(0xfffc000000000000\)\[\$r23\];;

    1828:	d7 45 2b a0 00 00 80 1f                         	xlo.mt \$r23\? \$a10 = -8388608 \(0xff800000\)\[\$r23\];;

    1830:	18 56 2b 21                                     	xlo.s.mf \$r24\? \$a10 = \[\$r24\];;

    1834:	19 06 13 ae 00 00 00 98 00 00 80 1f             	xlo.u \$a4..a5, \$r24 = -1125899906842624 \(0xfffc000000000000\)\[\$r25\];;

    1840:	59 16 1b af 00 00 80 1f                         	xlo.us.q \$a6..a7, \$r25 = -8388608 \(0xff800000\)\[\$r25\];;

    1848:	9a 26 23 2c                                     	xlo.d \$a8..a9, \$r26 = \[\$r26\];;

    184c:	9b 36 27 ad 00 00 00 98 00 00 80 1f             	xlo.s.w \$a8..a11, \$r26 = -1125899906842624 \(0xfffc000000000000\)\[\$r27\];;

    1858:	db 46 37 ae 00 00 80 1f                         	xlo.u.h \$a12..a15, \$r27 = -8388608 \(0xff800000\)\[\$r27\];;

    1860:	1c 57 47 2f                                     	xlo.us.b \$a16..a19, \$r28 = \[\$r28\];;

    1864:	1d 07 4f ac 00 00 00 98 00 00 80 1f             	xlo \$a16..a23, \$r28 = -1125899906842624 \(0xfffc000000000000\)\[\$r29\];;

    1870:	5d 17 6f ad 00 00 80 1f                         	xlo.s.q \$a24..a31, \$r29 = -8388608 \(0xff800000\)\[\$r29\];;

    1878:	9e 27 8f 2e                                     	xlo.u.d \$a32..a39, \$r30 = \[\$r30\];;

    187c:	9f 37 9f af 00 00 00 98 00 00 80 1f             	xlo.us.w \$a32..a47, \$r30 = -1125899906842624 \(0xfffc000000000000\)\[\$r31\];;

    1888:	df 47 df ac 00 00 80 1f                         	xlo.h \$a48..a63, \$r31 = -8388608 \(0xff800000\)\[\$r31\];;

    1890:	20 58 1f 2d                                     	xlo.s.b \$a0..a15, \$r32 = \[\$r32\];;

    1894:	21 08 3f ae 00 00 00 98 00 00 80 1f             	xlo.u \$a0..a31, \$r32 = -1125899906842624 \(0xfffc000000000000\)\[\$r33\];;

    18a0:	61 18 bf af 00 00 80 1f                         	xlo.us.q \$a32..a63, \$r33 = -8388608 \(0xff800000\)\[\$r33\];;

    18a8:	a2 28 3f 2c                                     	xlo.d \$a0..a31, \$r34 = \[\$r34\];;

    18ac:	a3 38 7f ad 00 00 00 98 00 00 80 1f             	xlo.s.w \$a0..a63, \$r34 = -1125899906842624 \(0xfffc000000000000\)\[\$r35\];;

    18b8:	e3 48 7f ae 00 00 80 1f                         	xlo.u.h \$a0..a63, \$r35 = -8388608 \(0xff800000\)\[\$r35\];;

    18c0:	24 59 7f 2f                                     	xlo.us.b \$a0..a63, \$r36 = \[\$r36\];;

    18c4:	e4 ff 61 a8 ff ff ff 9f ff ff ff 18             	xlo.q0 \$a24a25a26a27 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r36\];;

    18d0:	25 f0 75 29                                     	xlo.s.q1 \$a28a29a30a31 = -64 \(0xffffffc0\)\[\$r37\];;

    18d4:	25 00 89 aa 00 00 80 1f                         	xlo.u.q2 \$a32a33a34a35 = -8589934592 \(0xfffffffe00000000\)\[\$r37\];;

    18dc:	e5 ff 29 a3 ff ff ff 9f ff ff ff 18             	xlo.us \$a10 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r37\];;

    18e8:	26 f0 2d 20                                     	xlo \$a11 = -64 \(0xffffffc0\)\[\$r38\];;

    18ec:	26 00 2d a1 00 00 80 1f                         	xlo.s \$a11 = -8589934592 \(0xfffffffe00000000\)\[\$r38\];;

    18f4:	cc 02 18 03                                     	xmadd44bw0 \$a6a7 = \$a11, \$a12;;

    18f8:	0c 03 24 03                                     	xmadd44bw1 \$a8a9 = \$a12, \$a12;;

    18fc:	4d 83 34 04                                     	xmaddifwo.rn.s \$a13 = \$a13, \$a13;;

    1900:	8e 03 22 03                                     	xmaddsu44bw0 \$a8a9 = \$a14, \$a14;;

    1904:	8f 03 2e 03                                     	xmaddsu44bw1 \$a10a11 = \$a14, \$a15;;

    1908:	cf 03 29 03                                     	xmaddu44bw0 \$a10a11 = \$a15, \$a15;;

    190c:	10 04 35 03                                     	xmaddu44bw1 \$a12a13 = \$a16, \$a16;;

    1910:	8e 03 30 02                                     	xmma4164bw \$a12a13 = \$a14a15, \$a14a15;;

    1914:	11 04 44 02                                     	xmma484bw \$a16a17 = \$a16, \$a17;;

    1918:	92 04 42 02                                     	xmmasu4164bw \$a16a17 = \$a18a19, \$a18a19;;

    191c:	51 04 56 02                                     	xmmasu484bw \$a20a21 = \$a17, \$a17;;

    1920:	96 05 51 02                                     	xmmau4164bw \$a20a21 = \$a22a23, \$a22a23;;

    1924:	92 04 65 02                                     	xmmau484bw \$a24a25 = \$a18, \$a18;;

    1928:	9a 06 63 02                                     	xmmaus4164bw \$a24a25 = \$a26a27, \$a26a27;;

    192c:	93 04 77 02                                     	xmmaus484bw \$a28a29 = \$a18, \$a19;;

    1930:	00 c0 98 00                                     	xmovefd \$r38 = \$a0_x;;

    1934:	c0 84 b4 00                                     	xmovefo \$r44r45r46r47 = \$a19;;

    1938:	00 00 94 00                                     	xmovefq \$r36r37 = \$a0_lo;;

    193c:	27 e0 03 73                                     	xmovetd \$a0_t = \$r39;;

    1940:	27 e0 03 70                                     	xmovetd \$a0_x = \$r39;;

    1944:	27 e0 03 71                                     	xmovetd \$a0_y = \$r39;;

    1948:	28 e0 03 72                                     	xmovetd \$a0_z = \$r40;;

    194c:	28 ea 03 74                                     	xmovetq \$a0_lo = \$r40, \$r40;;

    1950:	69 ea 03 75                                     	xmovetq \$a0_hi = \$r41, \$r41;;

    1954:	14 15 4d 04                                     	xmsbfifwo.ru \$a19 = \$a20, \$a20;;

    1958:	00 1a 95 07                                     	xcopyv.td \$a36a37a38a39 = \$a40a41a42a43;;

    195c:	55 15 53 07                                     	xnando \$a20 = \$a21, \$a21;;

    1960:	96 35 57 07                                     	xnoro \$a21 = \$a22, \$a22;;

    1964:	d7 55 5b 07                                     	xnxoro \$a22 = \$a23, \$a23;;

    1968:	ea ff a4 ec ff ff ff 87 ff ff ff 00             	xord \$r41 = \$r42, 2305843009213693951 \(0x1fffffffffffffff\);;

    1974:	ea 0a a9 7c                                     	xord \$r42 = \$r42, \$r43;;

    1978:	2b f0 ac 6c                                     	xord \$r43 = \$r43, -64 \(0xffffffc0\);;

    197c:	2c 00 b0 ec 00 00 80 07                         	xord \$r44 = \$r44, -8589934592 \(0xfffffffe00000000\);;

    1984:	ed 0f b1 fc ff ff ff 00                         	xord.@ \$r44 = \$r45, 536870911 \(0x1fffffff\);;

    198c:	18 76 5f 07                                     	xorno \$a23 = \$a24, \$a24;;

    1990:	59 26 63 07                                     	xoro \$a24 = \$a25, \$a25;;

    1994:	ad c0 b7 72                                     	xorrbod \$r45 = \$r45;;

    1998:	6e c0 bb 72                                     	xorrhqd \$r46 = \$r46;;

    199c:	2f c0 bb 72                                     	xorrwpd \$r46 = \$r47;;

    19a0:	2f 1c bd 7c                                     	xorw \$r47 = \$r47, \$r48;;

    19a4:	30 f0 c0 7c                                     	xorw \$r48 = \$r48, -64 \(0xffffffc0\);;

    19a8:	31 00 c4 fc 00 00 80 07                         	xorw \$r49 = \$r49, -8589934592 \(0xfffffffe00000000\);;

    19b0:	00 e0 67 78                                     	xrecvo.f \$a25;;

    19b4:	9a e6 6a 07                                     	xsbmm8dq \$a26 = \$a26, \$a26;;

    19b8:	db f6 6e 07                                     	xsbmmt8dq \$a27 = \$a27, \$a27;;

    19bc:	00 e7 03 77                                     	xsendo.b \$a28;;

    19c0:	00 e7 73 7e                                     	xsendrecvo.f.b \$a28, \$a28;;

    19c4:	72 ec 77 35                                     	xso \$r49\[\$r50\] = \$a29;;

    19c8:	f2 ff 75 b5 ff ff ff 9f ff ff ff 18             	xso 2305843009213693951 \(0x1fffffffffffffff\)\[\$r50\] = \$a29;;

    19d4:	b3 6c 77 b5 00 00 00 98 00 00 80 1f             	xso.mtc \$r50\? -1125899906842624 \(0xfffc000000000000\)\[\$r51\] = \$a29;;

    19e0:	f3 7c 7b b5 00 00 80 1f                         	xso.mfc \$r51\? -8388608 \(0xff800000\)\[\$r51\] = \$a30;;

    19e8:	34 0d 7b 35                                     	xso.dnez \$r52\? \[\$r52\] = \$a30;;

    19ec:	34 f0 79 35                                     	xso -64 \(0xffffffc0\)\[\$r52\] = \$a30;;

    19f0:	35 00 7d b5 00 00 80 1f                         	xso -8589934592 \(0xfffffffe00000000\)\[\$r53\] = \$a31;;

    19f8:	c0 ff 7d ee ff ff ff 87 ff ff ff 00             	xsplatdo \$a31 = 2305843009213693951 \(0x1fffffffffffffff\);;

    1a04:	3c 00 7d ee 00 00 00 00                         	xsplatdo \$a31 = -549755813888 \(0xffffff8000000000\);;

    1a0c:	00 f0 81 6e                                     	xsplatdo \$a32 = -4096 \(0xfffff000\);;

    1a10:	00 18 b1 07                                     	xsplatov.td \$a44a45a46a47 = \$a32;;

    1a14:	00 18 70 07                                     	xsplatox.zd \$a28a29 = \$a32;;

    1a18:	40 08 c1 06                                     	xsx48bw \$a48a49a50a51 = \$a33;;

    1a1c:	00 0d 84 06                                     	xtrunc48wb \$a33 = \$a52a53a54a55;;

    1a20:	a2 48 87 07                                     	xxoro \$a33 = \$a34, \$a34;;

    1a24:	80 08 e5 06                                     	xzx48bw \$a56a57a58a59 = \$a34;;

    1a28:	f5 3f d4 78                                     	zxbd \$r53 = \$r53;;

    1a2c:	36 f0 db 64                                     	zxhd \$r54 = \$r54;;

    1a30:	37 50 da 74                                     	zxlbhq \$r54 = \$r55;;

    1a34:	37 40 de 74                                     	zxlhwp \$r55 = \$r55;;

    1a38:	38 50 e2 75                                     	zxmbhq \$r56 = \$r56;;

    1a3c:	39 40 e2 75                                     	zxmhwp \$r56 = \$r57;;

    1a40:	f9 ff e4 78                                     	zxwd \$r57 = \$r57;;

