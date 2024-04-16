#as: -march=kv3-1
#objdump: -d
.*\/kv3-1-insns-64.o:     file format elf64-kvx

Disassembly of section .text:

0000000000000000 <main>:
       0:	c0 ff 00 e4 ff ff ff 87 ff ff ff 00             	abdd \$r0 = \$r0, 2305843009213693951 \(0x1fffffffffffffff\);;

       c:	41 00 01 74                                     	abdd \$r0 = \$r1, \$r1;;

      10:	02 f0 04 64                                     	abdd \$r1 = \$r2, -64 \(0xffffffc0\);;

      14:	02 00 08 e4 00 00 80 07                         	abdd \$r2 = \$r2, -8589934592 \(0xfffffffe00000000\);;

      1c:	c3 0f 0d f4 ff ff ff 00                         	abdd.@ \$r3 = \$r3, 536870911 \(0x1fffffff\);;

      24:	04 31 0d 74                                     	abdhq \$r3 = \$r4, \$r4;;

      28:	c5 37 11 f4 ff ff ff 00                         	abdhq \$r4 = \$r5, 536870911 \(0x1fffffff\);;

      30:	85 21 15 74                                     	abdwp \$r5 = \$r5, \$r6;;

      34:	c6 2f 19 f4 ff ff ff 00                         	abdwp.@ \$r6 = \$r6, 536870911 \(0x1fffffff\);;

      3c:	c7 11 1d 74                                     	abdw \$r7 = \$r7, \$r7;;

      40:	08 f0 20 74                                     	abdw \$r8 = \$r8, -64 \(0xffffffc0\);;

      44:	09 00 20 f4 00 00 80 07                         	abdw \$r8 = \$r9, -8589934592 \(0xfffffffe00000000\);;

      4c:	09 00 24 64                                     	absd \$r9 = \$r9;;

      50:	0a 30 29 f4 00 00 00 00                         	abshq \$r10 = \$r10;;

      58:	0b 20 29 f4 00 00 00 00                         	abswp \$r10 = \$r11;;

      60:	0b 00 2c 74                                     	absw \$r11 = \$r11;;

      64:	0c e3 03 3f                                     	acswapd \$r12\[\$r12\] = \$r0r1;;

      68:	cd ff 01 bf ff ff ff 9f ff ff ff 18             	acswapd 2305843009213693951 \(0x1fffffffffffffff\)\[\$r13\] = \$r0r1;;

      74:	4e 03 0b bf 00 00 00 98 00 00 80 1f             	acswapd.dnez \$r13\? -1125899906842624 \(0xfffc000000000000\)\[\$r14\] = \$r2r3;;

      80:	8f 13 0b bf 00 00 80 1f                         	acswapd.deqz \$r14\? -8388608 \(0xff800000\)\[\$r15\] = \$r2r3;;

      88:	d0 23 13 3f                                     	acswapd.dltz \$r15\? \[\$r16\] = \$r4r5;;

      8c:	10 f0 11 3f                                     	acswapd -64 \(0xffffffc0\)\[\$r16\] = \$r4r5;;

      90:	10 00 19 bf 00 00 80 1f                         	acswapd -8589934592 \(0xfffffffe00000000\)\[\$r16\] = \$r6r7;;

      98:	51 f4 1b 3e                                     	acswapw.xs \$r17\[\$r17\] = \$r6r7;;

      9c:	d1 ff 21 be ff ff ff 9f ff ff ff 18             	acswapw 2305843009213693951 \(0x1fffffffffffffff\)\[\$r17\] = \$r8r9;;

      a8:	92 34 23 be 00 00 00 98 00 00 80 1f             	acswapw.dgez \$r18\? -1125899906842624 \(0xfffc000000000000\)\[\$r18\] = \$r8r9;;

      b4:	93 44 2b be 00 00 80 1f                         	acswapw.dlez \$r18\? -8388608 \(0xff800000\)\[\$r19\] = \$r10r11;;

      bc:	d3 54 2b 3e                                     	acswapw.dgtz \$r19\? \[\$r19\] = \$r10r11;;

      c0:	14 f0 31 3e                                     	acswapw -64 \(0xffffffc0\)\[\$r20\] = \$r12r13;;

      c4:	14 00 31 be 00 00 80 1f                         	acswapw -8589934592 \(0xfffffffe00000000\)\[\$r20\] = \$r12r13;;

      cc:	55 d5 51 7e                                     	addcd.i \$r20 = \$r21, \$r21;;

      d0:	d6 d7 55 fe ff ff ff 00                         	addcd.i \$r21 = \$r22, 536870911 \(0x1fffffff\);;

      d8:	d6 c5 59 7e                                     	addcd \$r22 = \$r22, \$r23;;

      dc:	d7 c7 5d fe ff ff ff 00                         	addcd \$r23 = \$r23, 536870911 \(0x1fffffff\);;

      e4:	d8 ff 60 e1 ff ff ff 87 ff ff ff 00             	addd \$r24 = \$r24, 2305843009213693951 \(0x1fffffffffffffff\);;

      f0:	59 06 61 71                                     	addd \$r24 = \$r25, \$r25;;

      f4:	1a f0 64 61                                     	addd \$r25 = \$r26, -64 \(0xffffffc0\);;

      f8:	1a 00 68 e1 00 00 80 07                         	addd \$r26 = \$r26, -8589934592 \(0xfffffffe00000000\);;

     100:	db 0f 6d f1 ff ff ff 00                         	addd.@ \$r27 = \$r27, 536870911 \(0x1fffffff\);;

     108:	1c 37 6d 7c                                     	addhcp.c \$r27 = \$r28, \$r28;;

     10c:	dd 37 71 fc ff ff ff 00                         	addhcp.c \$r28 = \$r29, 536870911 \(0x1fffffff\);;

     114:	9d 37 75 71                                     	addhq \$r29 = \$r29, \$r30;;

     118:	de 3f 79 f1 ff ff ff 00                         	addhq.@ \$r30 = \$r30, 536870911 \(0x1fffffff\);;

     120:	df ff 7d ee ff ff ff 87 ff ff ff 00             	addsd \$r31 = \$r31, 2305843009213693951 \(0x1fffffffffffffff\);;

     12c:	20 a8 7d 7e                                     	addsd \$r31 = \$r32, \$r32;;

     130:	21 f0 81 6e                                     	addsd \$r32 = \$r33, -64 \(0xffffffc0\);;

     134:	21 00 85 ee 00 00 80 07                         	addsd \$r33 = \$r33, -8589934592 \(0xfffffffe00000000\);;

     13c:	a2 f8 89 7e                                     	addshq \$r34 = \$r34, \$r34;;

     140:	e3 f7 8d fe ff ff ff 00                         	addshq \$r35 = \$r35, 536870911 \(0x1fffffff\);;

     148:	24 e9 8d 7e                                     	addswp \$r35 = \$r36, \$r36;;

     14c:	e5 ef 91 fe ff ff ff 00                         	addswp.@ \$r36 = \$r37, 536870911 \(0x1fffffff\);;

     154:	a5 b9 95 7e                                     	addsw \$r37 = \$r37, \$r38;;

     158:	e6 b7 99 fe ff ff ff 00                         	addsw \$r38 = \$r38, 536870911 \(0x1fffffff\);;

     160:	e7 49 9d 7a                                     	adduwd \$r39 = \$r39, \$r39;;

     164:	e8 47 a1 fa ff ff ff 00                         	adduwd \$r40 = \$r40, 536870911 \(0x1fffffff\);;

     16c:	69 2a a1 7c                                     	addwc.c \$r40 = \$r41, \$r41;;

     170:	ea 2f a5 fc ff ff ff 00                         	addwc.c.@ \$r41 = \$r42, 536870911 \(0x1fffffff\);;

     178:	ea 4a a9 78                                     	addwd \$r42 = \$r42, \$r43;;

     17c:	eb 47 ad f8 ff ff ff 00                         	addwd \$r43 = \$r43, 536870911 \(0x1fffffff\);;

     184:	2c 2b b1 71                                     	addwp \$r44 = \$r44, \$r44;;

     188:	ed 2f b5 f1 ff ff ff 00                         	addwp.@ \$r45 = \$r45, 536870911 \(0x1fffffff\);;

     190:	ae 1b b5 71                                     	addw \$r45 = \$r46, \$r46;;

     194:	2f f0 b8 71                                     	addw \$r46 = \$r47, -64 \(0xffffffc0\);;

     198:	2f 00 bc f1 00 00 80 07                         	addw \$r47 = \$r47, -8589934592 \(0xfffffffe00000000\);;

     1a0:	30 4c c1 76                                     	addx16d \$r48 = \$r48, \$r48;;

     1a4:	f1 47 c5 f6 ff ff ff 00                         	addx16d \$r49 = \$r49, 536870911 \(0x1fffffff\);;

     1ac:	b2 7c c5 76                                     	addx16hq \$r49 = \$r50, \$r50;;

     1b0:	f3 7f c9 f6 ff ff ff 00                         	addx16hq.@ \$r50 = \$r51, 536870911 \(0x1fffffff\);;

     1b8:	33 8d cd 7e                                     	addx16uwd \$r51 = \$r51, \$r52;;

     1bc:	f4 87 d1 fe ff ff ff 00                         	addx16uwd \$r52 = \$r52, 536870911 \(0x1fffffff\);;

     1c4:	75 8d d5 76                                     	addx16wd \$r53 = \$r53, \$r53;;

     1c8:	f6 87 d9 f6 ff ff ff 00                         	addx16wd \$r54 = \$r54, 536870911 \(0x1fffffff\);;

     1d0:	f7 6d d9 76                                     	addx16wp \$r54 = \$r55, \$r55;;

     1d4:	f8 67 dd f6 ff ff ff 00                         	addx16wp \$r55 = \$r56, 536870911 \(0x1fffffff\);;

     1dc:	78 5e e1 76                                     	addx16w \$r56 = \$r56, \$r57;;

     1e0:	f9 57 e5 f6 ff ff ff 00                         	addx16w \$r57 = \$r57, 536870911 \(0x1fffffff\);;

     1e8:	ba 4e e9 70                                     	addx2d \$r58 = \$r58, \$r58;;

     1ec:	fb 4f ed f0 ff ff ff 00                         	addx2d.@ \$r59 = \$r59, 536870911 \(0x1fffffff\);;

     1f4:	3c 7f ed 70                                     	addx2hq \$r59 = \$r60, \$r60;;

     1f8:	fd 77 f1 f0 ff ff ff 00                         	addx2hq \$r60 = \$r61, 536870911 \(0x1fffffff\);;

     200:	bd 8f f5 78                                     	addx2uwd \$r61 = \$r61, \$r62;;

     204:	fe 87 f9 f8 ff ff ff 00                         	addx2uwd \$r62 = \$r62, 536870911 \(0x1fffffff\);;

     20c:	ff 8f fd 70                                     	addx2wd \$r63 = \$r63, \$r63;;

     210:	c0 87 01 f0 ff ff ff 00                         	addx2wd \$r0 = \$r0, 536870911 \(0x1fffffff\);;

     218:	41 60 01 70                                     	addx2wp \$r0 = \$r1, \$r1;;

     21c:	c2 6f 05 f0 ff ff ff 00                         	addx2wp.@ \$r1 = \$r2, 536870911 \(0x1fffffff\);;

     224:	c2 50 09 70                                     	addx2w \$r2 = \$r2, \$r3;;

     228:	c3 57 0d f0 ff ff ff 00                         	addx2w \$r3 = \$r3, 536870911 \(0x1fffffff\);;

     230:	04 41 11 72                                     	addx4d \$r4 = \$r4, \$r4;;

     234:	c5 47 15 f2 ff ff ff 00                         	addx4d \$r5 = \$r5, 536870911 \(0x1fffffff\);;

     23c:	86 71 15 72                                     	addx4hq \$r5 = \$r6, \$r6;;

     240:	c7 7f 19 f2 ff ff ff 00                         	addx4hq.@ \$r6 = \$r7, 536870911 \(0x1fffffff\);;

     248:	07 82 1d 7a                                     	addx4uwd \$r7 = \$r7, \$r8;;

     24c:	c8 87 21 fa ff ff ff 00                         	addx4uwd \$r8 = \$r8, 536870911 \(0x1fffffff\);;

     254:	49 82 25 72                                     	addx4wd \$r9 = \$r9, \$r9;;

     258:	ca 87 29 f2 ff ff ff 00                         	addx4wd \$r10 = \$r10, 536870911 \(0x1fffffff\);;

     260:	cb 62 29 72                                     	addx4wp \$r10 = \$r11, \$r11;;

     264:	cc 67 2d f2 ff ff ff 00                         	addx4wp \$r11 = \$r12, 536870911 \(0x1fffffff\);;

     26c:	4d 53 31 72                                     	addx4w \$r12 = \$r13, \$r13;;

     270:	ce 57 39 f2 ff ff ff 00                         	addx4w \$r14 = \$r14, 536870911 \(0x1fffffff\);;

     278:	0f 44 3d 74                                     	addx8d \$r15 = \$r15, \$r16;;

     27c:	d0 4f 41 f4 ff ff ff 00                         	addx8d.@ \$r16 = \$r16, 536870911 \(0x1fffffff\);;

     284:	51 74 45 74                                     	addx8hq \$r17 = \$r17, \$r17;;

     288:	d2 77 49 f4 ff ff ff 00                         	addx8hq \$r18 = \$r18, 536870911 \(0x1fffffff\);;

     290:	d3 84 49 7c                                     	addx8uwd \$r18 = \$r19, \$r19;;

     294:	d4 87 4d fc ff ff ff 00                         	addx8uwd \$r19 = \$r20, 536870911 \(0x1fffffff\);;

     29c:	54 85 51 74                                     	addx8wd \$r20 = \$r20, \$r21;;

     2a0:	d5 87 55 f4 ff ff ff 00                         	addx8wd \$r21 = \$r21, 536870911 \(0x1fffffff\);;

     2a8:	96 65 59 74                                     	addx8wp \$r22 = \$r22, \$r22;;

     2ac:	d7 6f 5d f4 ff ff ff 00                         	addx8wp.@ \$r23 = \$r23, 536870911 \(0x1fffffff\);;

     2b4:	18 56 5d 74                                     	addx8w \$r23 = \$r24, \$r24;;

     2b8:	d9 57 61 f4 ff ff ff 00                         	addx8w \$r24 = \$r25, 536870911 \(0x1fffffff\);;

     2c0:	59 e6 6b 2f                                     	aladdd \$r25\[\$r25\] = \$r26;;

     2c4:	da ff 69 af ff ff ff 9f ff ff ff 18             	aladdd 2305843009213693951 \(0x1fffffffffffffff\)\[\$r26\] = \$r26;;

     2d0:	db 66 6f af 00 00 00 98 00 00 80 1f             	aladdd.odd \$r27\? -1125899906842624 \(0xfffc000000000000\)\[\$r27\] = \$r27;;

     2dc:	1c 77 73 af 00 00 80 1f                         	aladdd.even \$r28\? -8388608 \(0xff800000\)\[\$r28\] = \$r28;;

     2e4:	5d 87 77 2f                                     	aladdd.wnez \$r29\? \[\$r29\] = \$r29;;

     2e8:	1e f0 79 2f                                     	aladdd -64 \(0xffffffc0\)\[\$r30\] = \$r30;;

     2ec:	1e 00 7d af 00 00 80 1f                         	aladdd -8589934592 \(0xfffffffe00000000\)\[\$r30\] = \$r31;;

     2f4:	df f7 83 2e                                     	aladdw.xs \$r31\[\$r31\] = \$r32;;

     2f8:	e0 ff 81 ae ff ff ff 9f ff ff ff 18             	aladdw 2305843009213693951 \(0x1fffffffffffffff\)\[\$r32\] = \$r32;;

     304:	61 98 87 ae 00 00 00 98 00 00 80 1f             	aladdw.weqz \$r33\? -1125899906842624 \(0xfffc000000000000\)\[\$r33\] = \$r33;;

     310:	a2 a8 8b ae 00 00 80 1f                         	aladdw.wltz \$r34\? -8388608 \(0xff800000\)\[\$r34\] = \$r34;;

     318:	e3 b8 8f 2e                                     	aladdw.wgez \$r35\? \[\$r35\] = \$r35;;

     31c:	24 f0 91 2e                                     	aladdw -64 \(0xffffffc0\)\[\$r36\] = \$r36;;

     320:	24 00 95 ae 00 00 80 1f                         	aladdw -8589934592 \(0xfffffffe00000000\)\[\$r36\] = \$r37;;

     328:	66 e9 97 2b                                     	alclrd \$r37 = \$r37\[\$r38\];;

     32c:	a7 c9 9b ab 00 00 00 98 00 00 80 1f             	alclrd.wlez \$r38\? \$r38 = -1125899906842624 \(0xfffc000000000000\)\[\$r39\];;

     338:	e8 d9 9f ab 00 00 80 1f                         	alclrd.wgtz \$r39\? \$r39 = -8388608 \(0xff800000\)\[\$r40\];;

     340:	29 0a a3 2b                                     	alclrd.dnez \$r40\? \$r40 = \[\$r41\];;

     344:	e9 ff a5 ab ff ff ff 9f ff ff ff 18             	alclrd \$r41 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r41\];;

     350:	2a f0 a9 2b                                     	alclrd \$r42 = -64 \(0xffffffc0\)\[\$r42\];;

     354:	2b 00 a9 ab 00 00 80 1f                         	alclrd \$r42 = -8589934592 \(0xfffffffe00000000\)\[\$r43\];;

     35c:	ec fa af 2a                                     	alclrw.xs \$r43 = \$r43\[\$r44\];;

     360:	2d 1b b3 aa 00 00 00 98 00 00 80 1f             	alclrw.deqz \$r44\? \$r44 = -1125899906842624 \(0xfffc000000000000\)\[\$r45\];;

     36c:	6e 2b b7 aa 00 00 80 1f                         	alclrw.dltz \$r45\? \$r45 = -8388608 \(0xff800000\)\[\$r46\];;

     374:	af 3b bb 2a                                     	alclrw.dgez \$r46\? \$r46 = \[\$r47\];;

     378:	ef ff bd aa ff ff ff 9f ff ff ff 18             	alclrw \$r47 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r47\];;

     384:	30 f0 c1 2a                                     	alclrw \$r48 = -64 \(0xffffffc0\)\[\$r48\];;

     388:	31 00 c1 aa 00 00 80 1f                         	alclrw \$r48 = -8589934592 \(0xfffffffe00000000\)\[\$r49\];;

     390:	07 10 08 00                                     	aligno \$r0r1r2r3 = \$a0, \$a1, 7 \(0x7\);;

     394:	71 10 18 00                                     	aligno \$r4r5r6r7 = \$a0, \$a1, \$r49;;

     398:	07 10 2c 00                                     	aligno \$r8r9r10r11 = \$a1, \$a0, 7 \(0x7\);;

     39c:	f1 30 3c 00                                     	aligno \$r12r13r14r15 = \$a3, \$a2, \$r49;;

     3a0:	87 20 00 01                                     	alignv \$a0 = \$a2, \$a3, 7 \(0x7\);;

     3a4:	f2 20 00 01                                     	alignv \$a0 = \$a2, \$a3, \$r50;;

     3a8:	07 51 00 01                                     	alignv \$a0 = \$a5, \$a4, 7 \(0x7\);;

     3ac:	72 51 04 01                                     	alignv \$a1 = \$a5, \$a4, \$r50;;

     3b0:	f3 ff c8 e8 ff ff ff 87 ff ff ff 00             	andd \$r50 = \$r51, 2305843009213693951 \(0x1fffffffffffffff\);;

     3bc:	33 0d cd 78                                     	andd \$r51 = \$r51, \$r52;;

     3c0:	34 f0 d0 68                                     	andd \$r52 = \$r52, -64 \(0xffffffc0\);;

     3c4:	35 00 d4 e8 00 00 80 07                         	andd \$r53 = \$r53, -8589934592 \(0xfffffffe00000000\);;

     3cc:	f6 0f d5 f8 ff ff ff 00                         	andd.@ \$r53 = \$r54, 536870911 \(0x1fffffff\);;

     3d4:	f6 ff d8 ee ff ff ff 87 ff ff ff 00             	andnd \$r54 = \$r54, 2305843009213693951 \(0x1fffffffffffffff\);;

     3e0:	f7 0d dd 7e                                     	andnd \$r55 = \$r55, \$r55;;

     3e4:	38 f0 e0 6e                                     	andnd \$r56 = \$r56, -64 \(0xffffffc0\);;

     3e8:	39 00 e0 ee 00 00 80 07                         	andnd \$r56 = \$r57, -8589934592 \(0xfffffffe00000000\);;

     3f0:	f9 0f e5 fe ff ff ff 00                         	andnd.@ \$r57 = \$r57, 536870911 \(0x1fffffff\);;

     3f8:	ba 1e e9 7e                                     	andnw \$r58 = \$r58, \$r58;;

     3fc:	3b f0 ec 7e                                     	andnw \$r59 = \$r59, -64 \(0xffffffc0\);;

     400:	3c 00 ec fe 00 00 80 07                         	andnw \$r59 = \$r60, -8589934592 \(0xfffffffe00000000\);;

     408:	7c 1f f1 78                                     	andw \$r60 = \$r60, \$r61;;

     40c:	3d f0 f4 78                                     	andw \$r61 = \$r61, -64 \(0xffffffc0\);;

     410:	3e 00 f8 f8 00 00 80 07                         	andw \$r62 = \$r62, -8589934592 \(0xfffffffe00000000\);;

     418:	ff 7f f9 78                                     	avghq \$r62 = \$r63, \$r63;;

     41c:	c0 77 fd f8 ff ff ff 00                         	avghq \$r63 = \$r0, 536870911 \(0x1fffffff\);;

     424:	40 70 01 7a                                     	avgrhq \$r0 = \$r0, \$r1;;

     428:	c1 7f 05 fa ff ff ff 00                         	avgrhq.@ \$r1 = \$r1, 536870911 \(0x1fffffff\);;

     430:	82 70 09 7b                                     	avgruhq \$r2 = \$r2, \$r2;;

     434:	c3 77 0d fb ff ff ff 00                         	avgruhq \$r3 = \$r3, 536870911 \(0x1fffffff\);;

     43c:	04 61 0d 7b                                     	avgruwp \$r3 = \$r4, \$r4;;

     440:	c5 6f 11 fb ff ff ff 00                         	avgruwp.@ \$r4 = \$r5, 536870911 \(0x1fffffff\);;

     448:	85 51 15 7b                                     	avgruw \$r5 = \$r5, \$r6;;

     44c:	c6 57 19 fb ff ff ff 00                         	avgruw \$r6 = \$r6, 536870911 \(0x1fffffff\);;

     454:	c7 61 1d 7a                                     	avgrwp \$r7 = \$r7, \$r7;;

     458:	c8 67 21 fa ff ff ff 00                         	avgrwp \$r8 = \$r8, 536870911 \(0x1fffffff\);;

     460:	49 52 21 7a                                     	avgrw \$r8 = \$r9, \$r9;;

     464:	ca 57 25 fa ff ff ff 00                         	avgrw \$r9 = \$r10, 536870911 \(0x1fffffff\);;

     46c:	ca 72 29 79                                     	avguhq \$r10 = \$r10, \$r11;;

     470:	cb 7f 2d f9 ff ff ff 00                         	avguhq.@ \$r11 = \$r11, 536870911 \(0x1fffffff\);;

     478:	4c 63 31 79                                     	avguwp \$r12 = \$r12, \$r13;;

     47c:	ce 67 35 f9 ff ff ff 00                         	avguwp \$r13 = \$r14, 536870911 \(0x1fffffff\);;

     484:	cf 53 39 79                                     	avguw \$r14 = \$r15, \$r15;;

     488:	d0 57 41 f9 ff ff ff 00                         	avguw \$r16 = \$r16, 536870911 \(0x1fffffff\);;

     490:	51 64 41 78                                     	avgwp \$r16 = \$r17, \$r17;;

     494:	d2 6f 45 f8 ff ff ff 00                         	avgwp.@ \$r17 = \$r18, 536870911 \(0x1fffffff\);;

     49c:	d2 54 49 78                                     	avgw \$r18 = \$r18, \$r19;;

     4a0:	d3 57 4d f8 ff ff ff 00                         	avgw \$r19 = \$r19, 536870911 \(0x1fffffff\);;

     4a8:	00 00 a0 0f                                     	await;;

     4ac:	00 00 ac 0f                                     	barrier;;

     4b0:	00 00 80 1f                                     	call fffffffffe0004b0 <main\+0xfffffffffe0004b0>;;

     4b4:	14 20 52 72                                     	cbsd \$r20 = \$r20;;

     4b8:	15 40 52 72                                     	cbswp \$r20 = \$r21;;

     4bc:	15 30 56 72                                     	cbsw \$r21 = \$r21;;

     4c0:	16 00 78 0a                                     	cb.dlez \$r22\? ffffffffffff84c0 <main\+0xffffffffffff84c0>;;

     4c4:	d6 71 5b 6c                                     	clrf \$r22 = \$r22, 7 \(0x7\), 7 \(0x7\);;

     4c8:	17 20 5e 71                                     	clsd \$r23 = \$r23;;

     4cc:	18 40 5e 71                                     	clswp \$r23 = \$r24;;

     4d0:	18 30 62 71                                     	clsw \$r24 = \$r24;;

     4d4:	19 20 66 70                                     	clzd \$r25 = \$r25;;

     4d8:	1a 40 66 70                                     	clzwp \$r25 = \$r26;;

     4dc:	1a 30 6a 70                                     	clzw \$r26 = \$r26;;

     4e0:	db ff 6e e5 ff ff ff 87 ff ff ff 00             	cmoved.dgtz \$r27\? \$r27 = 2305843009213693951 \(0x1fffffffffffffff\);;

     4ec:	1b 07 72 76                                     	cmoved.odd \$r27\? \$r28 = \$r28;;

     4f0:	1c f0 76 67                                     	cmoved.even \$r28\? \$r29 = -64 \(0xffffffc0\);;

     4f4:	1d 00 76 e8 00 00 80 07                         	cmoved.wnez \$r29\? \$r29 = -8589934592 \(0xfffffffe00000000\);;

     4fc:	9e 17 7a 78                                     	cmovehq.nez \$r30\? \$r30 = \$r30;;

     500:	df 17 7e 71                                     	cmovewp.eqz \$r31\? \$r31 = \$r31;;

     504:	e0 ff 3c cb ff ff ff 97 ff ff ff 10             	cmuldt \$r14r15 = \$r32, 2305843009213693951 \(0x1fffffffffffffff\);;

     510:	20 18 3c 5b                                     	cmuldt \$r14r15 = \$r32, \$r32;;

     514:	21 f0 44 4b                                     	cmuldt \$r16r17 = \$r33, -64 \(0xffffffc0\);;

     518:	21 00 44 cb 00 00 80 17                         	cmuldt \$r16r17 = \$r33, -8589934592 \(0xfffffffe00000000\);;

     520:	a1 18 4c 5f                                     	cmulghxdt \$r18r19 = \$r33, \$r34;;

     524:	a2 18 4c 5d                                     	cmulglxdt \$r18r19 = \$r34, \$r34;;

     528:	e3 18 54 5e                                     	cmulgmxdt \$r20r21 = \$r35, \$r35;;

     52c:	23 19 54 5c                                     	cmulxdt \$r20r21 = \$r35, \$r36;;

     530:	e4 ff 91 e0 ff ff ff 87 ff ff ff 00             	compd.ne \$r36 = \$r36, 2305843009213693951 \(0x1fffffffffffffff\);;

     53c:	65 a9 95 71                                     	compd.eq \$r37 = \$r37, \$r37;;

     540:	26 f0 99 62                                     	compd.lt \$r38 = \$r38, -64 \(0xffffffc0\);;

     544:	27 00 99 e3 00 00 80 07                         	compd.ge \$r38 = \$r39, -8589934592 \(0xfffffffe00000000\);;

     54c:	27 fa 9d 74                                     	compnhq.le \$r39 = \$r39, \$r40;;

     550:	e8 f7 a1 f5 ff ff ff 00                         	compnhq.gt \$r40 = \$r40, 536870911 \(0x1fffffff\);;

     558:	69 ea a5 76                                     	compnwp.ltu \$r41 = \$r41, \$r41;;

     55c:	ea ef a9 f7 ff ff ff 00                         	compnwp.geu.@ \$r42 = \$r42, 536870911 \(0x1fffffff\);;

     564:	eb da a9 78                                     	compuwd.leu \$r42 = \$r43, \$r43;;

     568:	ec d7 ad f9 ff ff ff 00                         	compuwd.gtu \$r43 = \$r44, 536870911 \(0x1fffffff\);;

     570:	6c cb b1 7a                                     	compwd.all \$r44 = \$r44, \$r45;;

     574:	ed c7 b5 fb ff ff ff 00                         	compwd.nall \$r45 = \$r45, 536870911 \(0x1fffffff\);;

     57c:	ae bb b9 7c                                     	compw.any \$r46 = \$r46, \$r46;;

     580:	ef b7 bd fd ff ff ff 00                         	compw.none \$r47 = \$r47, 536870911 \(0x1fffffff\);;

     588:	00 00 00 05                                     	convdhv0.rn.sat \$a0_lo = \$a0a1a2a3;;

     58c:	80 51 00 05                                     	convdhv1.ru.satu \$a0_hi = \$a4a5a6a7;;

     590:	00 82 00 06                                     	convwbv0.rd.sat \$a0_x = \$a8a9a10a11;;

     594:	80 d3 00 06                                     	convwbv1.rz.satu \$a0_y = \$a12a13a14a15;;

     598:	00 24 01 06                                     	convwbv2.rhu.sat \$a0_z = \$a16a17a18a19;;

     59c:	80 70 01 06                                     	convwbv3.rn.satu \$a0_t = \$a20a21a22a23;;

     5a0:	30 00 bc 6a                                     	copyd \$r47 = \$r48;;

     5a4:	14 00 45 3e                                     	copyo \$r16r17r18r19 = \$r20r21r22r23;;

     5a8:	30 fc 58 5f                                     	copyq \$r22r23 = \$r48, \$r48;;

     5ac:	31 00 c4 7a                                     	copyw \$r49 = \$r49;;

     5b0:	b2 2c c4 59                                     	crcbellw \$r49 = \$r50, \$r50;;

     5b4:	f3 27 c8 d9 ff ff ff 10                         	crcbellw \$r50 = \$r51, 536870911 \(0x1fffffff\);;

     5bc:	33 2d cc 58                                     	crcbelmw \$r51 = \$r51, \$r52;;

     5c0:	f4 27 d0 d8 ff ff ff 10                         	crcbelmw \$r52 = \$r52, 536870911 \(0x1fffffff\);;

     5c8:	75 2d d4 5b                                     	crclellw \$r53 = \$r53, \$r53;;

     5cc:	f6 27 d8 db ff ff ff 10                         	crclellw \$r54 = \$r54, 536870911 \(0x1fffffff\);;

     5d4:	f7 2d d8 5a                                     	crclelmw \$r54 = \$r55, \$r55;;

     5d8:	f8 27 dc da ff ff ff 10                         	crclelmw \$r55 = \$r56, 536870911 \(0x1fffffff\);;

     5e0:	38 20 e2 73                                     	ctzd \$r56 = \$r56;;

     5e4:	39 40 e6 73                                     	ctzwp \$r57 = \$r57;;

     5e8:	3a 30 e6 73                                     	ctzw \$r57 = \$r58;;

     5ec:	00 00 8d 3f                                     	d1inval;;

     5f0:	fa ff 1c bc ff ff ff 9f ff ff ff 18             	dinvall 2305843009213693951 \(0x1fffffffffffffff\)\[\$r58\];;

     5fc:	bb 9e 1e bc 00 00 00 98 00 00 80 1f             	dinvall.weqz \$r58\? -1125899906842624 \(0xfffc000000000000\)\[\$r59\];;

     608:	fb ae 1e bc 00 00 80 1f                         	dinvall.wltz \$r59\? -8388608 \(0xff800000\)\[\$r59\];;

     610:	3c bf 1e 3c                                     	dinvall.wgez \$r60\? \[\$r60\];;

     614:	3d ef 1e 3c                                     	dinvall \$r60\[\$r61\];;

     618:	3d f0 1c 3c                                     	dinvall -64 \(0xffffffc0\)\[\$r61\];;

     61c:	3d 00 1c bc 00 00 80 1f                         	dinvall -8589934592 \(0xfffffffe00000000\)\[\$r61\];;

     624:	18 26 58 52                                     	dot2suwdp \$r22r23 = \$r24r25, \$r24r25;;

     628:	fe ff f8 ce ff ff ff 97 ff ff ff 10             	dot2suwd \$r62 = \$r62, 2305843009213693951 \(0x1fffffffffffffff\);;

     634:	ff 2f f8 5e                                     	dot2suwd \$r62 = \$r63, \$r63;;

     638:	00 f0 fc 4e                                     	dot2suwd \$r63 = \$r0, -64 \(0xffffffc0\);;

     63c:	00 00 00 ce 00 00 80 17                         	dot2suwd \$r0 = \$r0, -8589934592 \(0xfffffffe00000000\);;

     644:	1a 27 68 51                                     	dot2uwdp \$r26r27 = \$r26r27, \$r28r29;;

     648:	c1 ff 04 cd ff ff ff 97 ff ff ff 10             	dot2uwd \$r1 = \$r1, 2305843009213693951 \(0x1fffffffffffffff\);;

     654:	82 20 04 5d                                     	dot2uwd \$r1 = \$r2, \$r2;;

     658:	03 f0 08 4d                                     	dot2uwd \$r2 = \$r3, -64 \(0xffffffc0\);;

     65c:	03 00 0c cd 00 00 80 17                         	dot2uwd \$r3 = \$r3, -8589934592 \(0xfffffffe00000000\);;

     664:	9e 27 70 50                                     	dot2wdp \$r28r29 = \$r30r31, \$r30r31;;

     668:	c4 ff 10 cc ff ff ff 97 ff ff ff 10             	dot2wd \$r4 = \$r4, 2305843009213693951 \(0x1fffffffffffffff\);;

     674:	45 21 10 5c                                     	dot2wd \$r4 = \$r5, \$r5;;

     678:	06 f0 14 4c                                     	dot2wd \$r5 = \$r6, -64 \(0xffffffc0\);;

     67c:	06 00 18 cc 00 00 80 17                         	dot2wd \$r6 = \$r6, -8589934592 \(0xfffffffe00000000\);;

     684:	a0 28 80 53                                     	dot2wzp \$r32r33 = \$r32r33, \$r34r35;;

     688:	c7 ff 1c cf ff ff ff 97 ff ff ff 10             	dot2w \$r7 = \$r7, 2305843009213693951 \(0x1fffffffffffffff\);;

     694:	08 22 1c 5f                                     	dot2w \$r7 = \$r8, \$r8;;

     698:	09 f0 20 4f                                     	dot2w \$r8 = \$r9, -64 \(0xffffffc0\);;

     69c:	09 00 24 cf 00 00 80 17                         	dot2w \$r9 = \$r9, -8589934592 \(0xfffffffe00000000\);;

     6a4:	ca ff 0c bc ff ff ff 9f ff ff ff 18             	dtouchl 2305843009213693951 \(0x1fffffffffffffff\)\[\$r10\];;

     6b0:	8a c2 0e bc 00 00 00 98 00 00 80 1f             	dtouchl.wlez \$r10\? -1125899906842624 \(0xfffc000000000000\)\[\$r10\];;

     6bc:	cb d2 0e bc 00 00 80 1f                         	dtouchl.wgtz \$r11\? -8388608 \(0xff800000\)\[\$r11\];;

     6c4:	cc 02 0e 3c                                     	dtouchl.dnez \$r11\? \[\$r12\];;

     6c8:	0d e3 0e 3c                                     	dtouchl \$r12\[\$r13\];;

     6cc:	0d f0 0c 3c                                     	dtouchl -64 \(0xffffffc0\)\[\$r13\];;

     6d0:	0e 00 0c bc 00 00 80 1f                         	dtouchl -8589934592 \(0xfffffffe00000000\)\[\$r14\];;

     6d8:	ce ff 0d a8 ff ff ff 9f ff ff ff 18             	dzerol 2305843009213693951 \(0x1fffffffffffffff\)\[\$r14\];;

     6e4:	cf 13 0f a8 00 00 00 98 00 00 80 1f             	dzerol.deqz \$r15\? -1125899906842624 \(0xfffc000000000000\)\[\$r15\];;

     6f0:	10 24 0f a8 00 00 80 1f                         	dzerol.dltz \$r16\? -8388608 \(0xff800000\)\[\$r16\];;

     6f8:	11 34 0f 28                                     	dzerol.dgez \$r16\? \[\$r17\];;

     6fc:	51 e4 0f 28                                     	dzerol \$r17\[\$r17\];;

     700:	12 f0 0d 28                                     	dzerol -64 \(0xffffffc0\)\[\$r18\];;

     704:	12 00 0d a8 00 00 80 1f                         	dzerol -8589934592 \(0xfffffffe00000000\)\[\$r18\];;

     70c:	00 00 00 00                                     	errop;;

     710:	d3 71 4b 68                                     	extfs \$r18 = \$r19, 7 \(0x7\), 7 \(0x7\);;

     714:	d3 71 4f 64                                     	extfz \$r19 = \$r19, 7 \(0x7\), 7 \(0x7\);;

     718:	14 20 53 71                                     	fabsd \$r20 = \$r20;;

     71c:	15 20 53 77                                     	fabshq \$r20 = \$r21;;

     720:	15 20 57 75                                     	fabswp \$r21 = \$r21;;

     724:	16 20 5b 73                                     	fabsw \$r22 = \$r22;;

     728:	24 09 8b 5d                                     	fadddc.c.rn \$r34r35 = \$r36r37, \$r36r37;;

     72c:	26 9a 9b 5c                                     	fadddp.ru.s \$r38r39 = \$r38r39, \$r40r41;;

     730:	aa 2a a3 5c                                     	fadddp.rd \$r40r41 = \$r42r43, \$r42r43;;

     734:	d7 ff 5a c0 ff ff ff 97 ff ff ff 10             	faddd \$r22 = \$r23, 2305843009213693951 \(0x1fffffffffffffff\);;

     740:	17 f0 5e 40                                     	faddd \$r23 = \$r23, -64 \(0xffffffc0\);;

     744:	18 00 62 c0 00 00 80 17                         	faddd \$r24 = \$r24, -8589934592 \(0xfffffffe00000000\);;

     74c:	59 b6 62 50                                     	faddd.rz.s \$r24 = \$r25, \$r25;;

     750:	da ff 66 c2 ff ff ff 97 ff ff ff 10             	faddhq \$r25 = \$r26, 2305843009213693951 \(0x1fffffffffffffff\);;

     75c:	1a f0 6a 42                                     	faddhq \$r26 = \$r26, -64 \(0xffffffc0\);;

     760:	1b 00 6e c2 00 00 80 17                         	faddhq \$r27 = \$r27, -8589934592 \(0xfffffffe00000000\);;

     768:	1c 47 6e 52                                     	faddhq.rna \$r27 = \$r28, \$r28;;

     76c:	dd ff 72 c3 ff ff ff 97 ff ff ff 10             	faddwc.c \$r28 = \$r29, 2305843009213693951 \(0x1fffffffffffffff\);;

     778:	1d f0 76 43                                     	faddwc.c \$r29 = \$r29, -64 \(0xffffffc0\);;

     77c:	1e 00 7a c3 00 00 80 17                         	faddwc.c \$r30 = \$r30, -8589934592 \(0xfffffffe00000000\);;

     784:	df d7 7a 53                                     	faddwc.c.rnz.s \$r30 = \$r31, \$r31;;

     788:	ac 6b b7 59                                     	faddwcp.c.ro \$r44r45 = \$r44r45, \$r46r47;;

     78c:	30 fc bf 58                                     	faddwq.s \$r46r47 = \$r48r49, \$r48r49;;

     790:	20 08 7e 51                                     	faddwp.rn \$r31 = \$r32, \$r32;;

     794:	e1 ff 82 c1 ff ff ff 97 ff ff ff 10             	faddwp \$r32 = \$r33, 2305843009213693951 \(0x1fffffffffffffff\);;

     7a0:	21 f0 86 41                                     	faddwp \$r33 = \$r33, -64 \(0xffffffc0\);;

     7a4:	22 00 8a c1 00 00 80 17                         	faddwp \$r34 = \$r34, -8589934592 \(0xfffffffe00000000\);;

     7ac:	e3 98 8a 51                                     	faddwp.ru.s \$r34 = \$r35, \$r35;;

     7b0:	32 2d cf 58                                     	faddwq.rd \$r50r51 = \$r50r51, \$r52r53;;

     7b4:	e4 ff 8e cc ff ff ff 97 ff ff ff 10             	faddw \$r35 = \$r36, 2305843009213693951 \(0x1fffffffffffffff\);;

     7c0:	24 f0 92 4c                                     	faddw \$r36 = \$r36, -64 \(0xffffffc0\);;

     7c4:	25 00 96 cc 00 00 80 17                         	faddw \$r37 = \$r37, -8589934592 \(0xfffffffe00000000\);;

     7cc:	a6 b9 96 5c                                     	faddw.rz.s \$r37 = \$r38, \$r38;;

     7d0:	34 50 9b 71                                     	fcdivd \$r38 = \$r52r53;;

     7d4:	36 58 9f 75                                     	fcdivwp.s \$r39 = \$r54r55;;

     7d8:	36 50 9f 73                                     	fcdivw \$r39 = \$r54r55;;

     7dc:	28 0a 9f 78                                     	fcompd.one \$r39 = \$r40, \$r40;;

     7e0:	e9 07 a3 f9 ff ff ff 00                         	fcompd.ueq \$r40 = \$r41, 536870911 \(0x1fffffff\);;

     7e8:	a9 1a a7 7a                                     	fcompnhq.oeq \$r41 = \$r41, \$r42;;

     7ec:	ea 17 ab fb ff ff ff 00                         	fcompnhq.une \$r42 = \$r42, 536870911 \(0x1fffffff\);;

     7f4:	eb 1a af 74                                     	fcompnwp.olt \$r43 = \$r43, \$r43;;

     7f8:	ec 1f b3 f5 ff ff ff 00                         	fcompnwp.uge.@ \$r44 = \$r44, 536870911 \(0x1fffffff\);;

     800:	6d 0b b3 76                                     	fcompw.oge \$r44 = \$r45, \$r45;;

     804:	ee 07 b7 f7 ff ff ff 00                         	fcompw.ult \$r45 = \$r46, 536870911 \(0x1fffffff\);;

     80c:	b8 ce e7 5c                                     	fdot2wdp.rna.s \$r56r57 = \$r56r57, \$r58r59;;

     810:	ee ff b9 cd ff ff ff 97 ff ff ff 10             	fdot2wd \$r46 = \$r46, 2305843009213693951 \(0x1fffffffffffffff\);;

     81c:	2f f0 bd 4d                                     	fdot2wd \$r47 = \$r47, -64 \(0xffffffc0\);;

     820:	30 00 bd cd 00 00 80 17                         	fdot2wd \$r47 = \$r48, -8589934592 \(0xfffffffe00000000\);;

     828:	70 5c c1 5d                                     	fdot2wd.rnz \$r48 = \$r48, \$r49;;

     82c:	3c ef ef 5d                                     	fdot2wzp.ro.s \$r58r59 = \$r60r61, \$r60r61;;

     830:	f1 ff c5 cc ff ff ff 97 ff ff ff 10             	fdot2w \$r49 = \$r49, 2305843009213693951 \(0x1fffffffffffffff\);;

     83c:	32 f0 c9 4c                                     	fdot2w \$r50 = \$r50, -64 \(0xffffffc0\);;

     840:	33 00 c9 cc 00 00 80 17                         	fdot2w \$r50 = \$r51, -8589934592 \(0xfffffffe00000000\);;

     848:	33 7d cd 5c                                     	fdot2w \$r51 = \$r51, \$r52;;

     84c:	00 00 cd 3f                                     	fence;;

     850:	f4 ff d1 c0 ff ff ff 97 ff ff ff 10             	ffmad \$r52 = \$r52, 2305843009213693951 \(0x1fffffffffffffff\);;

     85c:	35 f0 d5 40                                     	ffmad \$r53 = \$r53, -64 \(0xffffffc0\);;

     860:	36 00 d5 c0 00 00 80 17                         	ffmad \$r53 = \$r54, -8589934592 \(0xfffffffe00000000\);;

     868:	f6 8d d9 50                                     	ffmad.rn.s \$r54 = \$r54, \$r55;;

     86c:	f7 ff dd c3 ff ff ff 97 ff ff ff 10             	ffmahq \$r55 = \$r55, 2305843009213693951 \(0x1fffffffffffffff\);;

     878:	38 f0 e1 43                                     	ffmahq \$r56 = \$r56, -64 \(0xffffffc0\);;

     87c:	39 00 e1 c3 00 00 80 17                         	ffmahq \$r56 = \$r57, -8589934592 \(0xfffffffe00000000\);;

     884:	b9 1e e5 53                                     	ffmahq.ru \$r57 = \$r57, \$r58;;

     888:	fa ff fb c1 ff ff ff 97 ff ff ff 10             	ffmahwq \$r62r63 = \$r58, 2305843009213693951 \(0x1fffffffffffffff\);;

     894:	3a f0 fb 41                                     	ffmahwq \$r62r63 = \$r58, -64 \(0xffffffc0\);;

     898:	3b 00 03 c1 00 00 80 17                         	ffmahwq \$r0r1 = \$r59, -8589934592 \(0xfffffffe00000000\);;

     8a0:	fb ae 03 51                                     	ffmahwq.rd.s \$r0r1 = \$r59, \$r59;;

     8a4:	fc ff f2 c8 ff ff ff 97 ff ff ff 10             	ffmahw \$r60 = \$r60, 2305843009213693951 \(0x1fffffffffffffff\);;

     8b0:	3d f0 f2 48                                     	ffmahw \$r60 = \$r61, -64 \(0xffffffc0\);;

     8b4:	3d 00 f6 c8 00 00 80 17                         	ffmahw \$r61 = \$r61, -8589934592 \(0xfffffffe00000000\);;

     8bc:	be 3f fa 58                                     	ffmahw.rz \$r62 = \$r62, \$r62;;

     8c0:	ff ff 0b c0 ff ff ff 97 ff ff ff 10             	ffmawdp \$r2r3 = \$r63, 2305843009213693951 \(0x1fffffffffffffff\);;

     8cc:	3f f0 0b 40                                     	ffmawdp \$r2r3 = \$r63, -64 \(0xffffffc0\);;

     8d0:	3f 00 13 c0 00 00 80 17                         	ffmawdp \$r4r5 = \$r63, -8589934592 \(0xfffffffe00000000\);;

     8d8:	00 c0 13 50                                     	ffmawdp.rna.s \$r4r5 = \$r0, \$r0;;

     8dc:	c1 ff 01 c1 ff ff ff 97 ff ff ff 10             	ffmawd \$r0 = \$r1, 2305843009213693951 \(0x1fffffffffffffff\);;

     8e8:	01 f0 05 41                                     	ffmawd \$r1 = \$r1, -64 \(0xffffffc0\);;

     8ec:	02 00 09 c1 00 00 80 17                         	ffmawd \$r2 = \$r2, -8589934592 \(0xfffffffe00000000\);;

     8f4:	c3 50 09 51                                     	ffmawd.rnz \$r2 = \$r3, \$r3;;

     8f8:	c4 ff 0d c2 ff ff ff 97 ff ff ff 10             	ffmawp \$r3 = \$r4, 2305843009213693951 \(0x1fffffffffffffff\);;

     904:	04 f0 11 42                                     	ffmawp \$r4 = \$r4, -64 \(0xffffffc0\);;

     908:	05 00 15 c2 00 00 80 17                         	ffmawp \$r5 = \$r5, -8589934592 \(0xfffffffe00000000\);;

     910:	86 e1 15 52                                     	ffmawp.ro.s \$r5 = \$r6, \$r6;;

     914:	c7 ff 1a c9 ff ff ff 97 ff ff ff 10             	ffmaw \$r6 = \$r7, 2305843009213693951 \(0x1fffffffffffffff\);;

     920:	07 f0 1e 49                                     	ffmaw \$r7 = \$r7, -64 \(0xffffffc0\);;

     924:	08 00 22 c9 00 00 80 17                         	ffmaw \$r8 = \$r8, -8589934592 \(0xfffffffe00000000\);;

     92c:	49 72 22 59                                     	ffmaw \$r8 = \$r9, \$r9;;

     930:	ca ff 25 c4 ff ff ff 97 ff ff ff 10             	ffmsd \$r9 = \$r10, 2305843009213693951 \(0x1fffffffffffffff\);;

     93c:	0a f0 29 44                                     	ffmsd \$r10 = \$r10, -64 \(0xffffffc0\);;

     940:	0b 00 2d c4 00 00 80 17                         	ffmsd \$r11 = \$r11, -8589934592 \(0xfffffffe00000000\);;

     948:	0c 83 2d 54                                     	ffmsd.rn.s \$r11 = \$r12, \$r12;;

     94c:	cd ff 35 c7 ff ff ff 97 ff ff ff 10             	ffmshq \$r13 = \$r13, 2305843009213693951 \(0x1fffffffffffffff\);;

     958:	0e f0 39 47                                     	ffmshq \$r14 = \$r14, -64 \(0xffffffc0\);;

     95c:	0f 00 3d c7 00 00 80 17                         	ffmshq \$r15 = \$r15, -8589934592 \(0xfffffffe00000000\);;

     964:	10 14 41 57                                     	ffmshq.ru \$r16 = \$r16, \$r16;;

     968:	d1 ff 1b c3 ff ff ff 97 ff ff ff 10             	ffmshwq \$r6r7 = \$r17, 2305843009213693951 \(0x1fffffffffffffff\);;

     974:	11 f0 1b 43                                     	ffmshwq \$r6r7 = \$r17, -64 \(0xffffffc0\);;

     978:	11 00 23 c3 00 00 80 17                         	ffmshwq \$r8r9 = \$r17, -8589934592 \(0xfffffffe00000000\);;

     980:	92 a4 23 53                                     	ffmshwq.rd.s \$r8r9 = \$r18, \$r18;;

     984:	d3 ff 4a ca ff ff ff 97 ff ff ff 10             	ffmshw \$r18 = \$r19, 2305843009213693951 \(0x1fffffffffffffff\);;

     990:	13 f0 4e 4a                                     	ffmshw \$r19 = \$r19, -64 \(0xffffffc0\);;

     994:	14 00 52 ca 00 00 80 17                         	ffmshw \$r20 = \$r20, -8589934592 \(0xfffffffe00000000\);;

     99c:	55 35 52 5a                                     	ffmshw.rz \$r20 = \$r21, \$r21;;

     9a0:	d5 ff 2b c2 ff ff ff 97 ff ff ff 10             	ffmswdp \$r10r11 = \$r21, 2305843009213693951 \(0x1fffffffffffffff\);;

     9ac:	16 f0 2b 42                                     	ffmswdp \$r10r11 = \$r22, -64 \(0xffffffc0\);;

     9b0:	16 00 33 c2 00 00 80 17                         	ffmswdp \$r12r13 = \$r22, -8589934592 \(0xfffffffe00000000\);;

     9b8:	d6 c5 33 52                                     	ffmswdp.rna.s \$r12r13 = \$r22, \$r23;;

     9bc:	d7 ff 5d c5 ff ff ff 97 ff ff ff 10             	ffmswd \$r23 = \$r23, 2305843009213693951 \(0x1fffffffffffffff\);;

     9c8:	18 f0 61 45                                     	ffmswd \$r24 = \$r24, -64 \(0xffffffc0\);;

     9cc:	19 00 61 c5 00 00 80 17                         	ffmswd \$r24 = \$r25, -8589934592 \(0xfffffffe00000000\);;

     9d4:	99 56 65 55                                     	ffmswd.rnz \$r25 = \$r25, \$r26;;

     9d8:	da ff 69 c6 ff ff ff 97 ff ff ff 10             	ffmswp \$r26 = \$r26, 2305843009213693951 \(0x1fffffffffffffff\);;

     9e4:	1b f0 6d 46                                     	ffmswp \$r27 = \$r27, -64 \(0xffffffc0\);;

     9e8:	1c 00 6d c6 00 00 80 17                         	ffmswp \$r27 = \$r28, -8589934592 \(0xfffffffe00000000\);;

     9f0:	5c e7 71 56                                     	ffmswp.ro.s \$r28 = \$r28, \$r29;;

     9f4:	dd ff 76 cb ff ff ff 97 ff ff ff 10             	ffmsw \$r29 = \$r29, 2305843009213693951 \(0x1fffffffffffffff\);;

     a00:	1e f0 7a 4b                                     	ffmsw \$r30 = \$r30, -64 \(0xffffffc0\);;

     a04:	1f 00 7a cb 00 00 80 17                         	ffmsw \$r30 = \$r31, -8589934592 \(0xfffffffe00000000\);;

     a0c:	1f 78 7e 5b                                     	ffmsw \$r31 = \$r31, \$r32;;

     a10:	e0 81 83 46                                     	fixedd.rn.s \$r32 = \$r32, 7 \(0x7\);;

     a14:	e1 11 87 47                                     	fixedud.ru \$r33 = \$r33, 7 \(0x7\);;

     a18:	e2 a1 87 4f                                     	fixeduwp.rd.s \$r33 = \$r34, 7 \(0x7\);;

     a1c:	e2 31 8b 4b                                     	fixeduw.rz \$r34 = \$r34, 7 \(0x7\);;

     a20:	e3 c1 8f 4e                                     	fixedwp.rna.s \$r35 = \$r35, 7 \(0x7\);;

     a24:	e4 51 8f 4a                                     	fixedw.rnz \$r35 = \$r36, 7 \(0x7\);;

     a28:	e4 e1 93 44                                     	floatd.ro.s \$r36 = \$r36, 7 \(0x7\);;

     a2c:	e5 71 97 45                                     	floatud \$r37 = \$r37, 7 \(0x7\);;

     a30:	e6 81 97 4d                                     	floatuwp.rn.s \$r37 = \$r38, 7 \(0x7\);;

     a34:	e6 11 9b 49                                     	floatuw.ru \$r38 = \$r38, 7 \(0x7\);;

     a38:	e7 a1 9f 4c                                     	floatwp.rd.s \$r39 = \$r39, 7 \(0x7\);;

     a3c:	e8 31 9f 48                                     	floatw.rz \$r39 = \$r40, 7 \(0x7\);;

     a40:	68 8a a3 71                                     	fmaxd \$r40 = \$r40, \$r41;;

     a44:	a9 8a a7 77                                     	fmaxhq \$r41 = \$r41, \$r42;;

     a48:	ea 8a ab 75                                     	fmaxwp \$r42 = \$r42, \$r43;;

     a4c:	2b 8b af 73                                     	fmaxw \$r43 = \$r43, \$r44;;

     a50:	6c 8b b3 70                                     	fmind \$r44 = \$r44, \$r45;;

     a54:	ad 8b b7 76                                     	fminhq \$r45 = \$r45, \$r46;;

     a58:	ee 8b bb 74                                     	fminwp \$r46 = \$r46, \$r47;;

     a5c:	2f 8c bf 72                                     	fminw \$r47 = \$r47, \$r48;;

     a60:	30 cc 3f 54                                     	fmm212w.rna.s \$r14r15 = \$r48, \$r48;;

     a64:	71 5c 3b 54                                     	fmma212w.rnz \$r14r15 = \$r49, \$r49;;

     a68:	41 00 00 03                                     	fmma242hw0 \$a0_lo = \$a0a1, \$a1, \$a1;;

     a6c:	82 10 00 03                                     	fmma242hw1 \$a0_hi = \$a0a1, \$a2, \$a2;;

     a70:	83 20 04 03                                     	fmma242hw2 \$a1_lo = \$a2a3, \$a2, \$a3;;

     a74:	c3 30 04 03                                     	fmma242hw3 \$a1_hi = \$a2a3, \$a3, \$a3;;

     a78:	b1 ec 43 56                                     	fmms212w.ro.s \$r16r17 = \$r49, \$r50;;

     a7c:	f2 ff c9 c8 ff ff ff 97 ff ff ff 10             	fmuld \$r50 = \$r50, 2305843009213693951 \(0x1fffffffffffffff\);;

     a88:	33 f0 cd 48                                     	fmuld \$r51 = \$r51, -64 \(0xffffffc0\);;

     a8c:	34 00 cd c8 00 00 80 17                         	fmuld \$r51 = \$r52, -8589934592 \(0xfffffffe00000000\);;

     a94:	74 7d d1 58                                     	fmuld \$r52 = \$r52, \$r53;;

     a98:	f5 ff d5 cb ff ff ff 97 ff ff ff 10             	fmulhq \$r53 = \$r53, 2305843009213693951 \(0x1fffffffffffffff\);;

     aa4:	36 f0 d9 4b                                     	fmulhq \$r54 = \$r54, -64 \(0xffffffc0\);;

     aa8:	37 00 d9 cb 00 00 80 17                         	fmulhq \$r54 = \$r55, -8589934592 \(0xfffffffe00000000\);;

     ab0:	37 8e dd 5b                                     	fmulhq.rn.s \$r55 = \$r55, \$r56;;

     ab4:	f8 ff 47 c1 ff ff ff 97 ff ff ff 10             	fmulhwq \$r16r17 = \$r56, 2305843009213693951 \(0x1fffffffffffffff\);;

     ac0:	38 f0 4f 41                                     	fmulhwq \$r18r19 = \$r56, -64 \(0xffffffc0\);;

     ac4:	39 00 4f c1 00 00 80 17                         	fmulhwq \$r18r19 = \$r57, -8589934592 \(0xfffffffe00000000\);;

     acc:	79 1e 57 51                                     	fmulhwq.ru \$r20r21 = \$r57, \$r57;;

     ad0:	fa ff ea cf ff ff ff 97 ff ff ff 10             	fmulhw \$r58 = \$r58, 2305843009213693951 \(0x1fffffffffffffff\);;

     adc:	3b f0 ea 4f                                     	fmulhw \$r58 = \$r59, -64 \(0xffffffc0\);;

     ae0:	3b 00 ee cf 00 00 80 17                         	fmulhw \$r59 = \$r59, -8589934592 \(0xfffffffe00000000\);;

     ae8:	3c af f2 5f                                     	fmulhw.rd.s \$r60 = \$r60, \$r60;;

     aec:	fd ff f5 cf ff ff ff 97 ff ff ff 10             	fmulwc.c \$r61 = \$r61, 2305843009213693951 \(0x1fffffffffffffff\);;

     af8:	3e f0 f5 4f                                     	fmulwc.c \$r61 = \$r62, -64 \(0xffffffc0\);;

     afc:	3e 00 f9 cf 00 00 80 17                         	fmulwc.c \$r62 = \$r62, -8589934592 \(0xfffffffe00000000\);;

     b04:	ff 3f fd 5f                                     	fmulwc.c.rz \$r63 = \$r63, \$r63;;

     b08:	c0 ff 01 ce ff ff ff 97 ff ff ff 10             	fmulwc \$r0 = \$r0, 2305843009213693951 \(0x1fffffffffffffff\);;

     b14:	01 f0 01 4e                                     	fmulwc \$r0 = \$r1, -64 \(0xffffffc0\);;

     b18:	01 00 05 ce 00 00 80 17                         	fmulwc \$r1 = \$r1, -8589934592 \(0xfffffffe00000000\);;

     b20:	82 c0 09 5e                                     	fmulwc.rna.s \$r2 = \$r2, \$r2;;

     b24:	c3 ff 57 c3 ff ff ff 97 ff ff ff 10             	fmulwdc.c \$r20r21 = \$r3, 2305843009213693951 \(0x1fffffffffffffff\);;

     b30:	03 f0 5f 43                                     	fmulwdc.c \$r22r23 = \$r3, -64 \(0xffffffc0\);;

     b34:	03 00 5f c3 00 00 80 17                         	fmulwdc.c \$r22r23 = \$r3, -8589934592 \(0xfffffffe00000000\);;

     b3c:	04 51 67 53                                     	fmulwdc.c.rnz \$r24r25 = \$r4, \$r4;;

     b40:	c4 ff 67 c2 ff ff ff 97 ff ff ff 10             	fmulwdc \$r24r25 = \$r4, 2305843009213693951 \(0x1fffffffffffffff\);;

     b4c:	05 f0 6f 42                                     	fmulwdc \$r26r27 = \$r5, -64 \(0xffffffc0\);;

     b50:	05 00 6f c2 00 00 80 17                         	fmulwdc \$r26r27 = \$r5, -8589934592 \(0xfffffffe00000000\);;

     b58:	85 e1 77 52                                     	fmulwdc.ro.s \$r28r29 = \$r5, \$r6;;

     b5c:	c6 ff 77 c0 ff ff ff 97 ff ff ff 10             	fmulwdp \$r28r29 = \$r6, 2305843009213693951 \(0x1fffffffffffffff\);;

     b68:	06 f0 7f 40                                     	fmulwdp \$r30r31 = \$r6, -64 \(0xffffffc0\);;

     b6c:	07 00 7f c0 00 00 80 17                         	fmulwdp \$r30r31 = \$r7, -8589934592 \(0xfffffffe00000000\);;

     b74:	c7 71 87 50                                     	fmulwdp \$r32r33 = \$r7, \$r7;;

     b78:	c8 ff 21 c9 ff ff ff 97 ff ff ff 10             	fmulwd \$r8 = \$r8, 2305843009213693951 \(0x1fffffffffffffff\);;

     b84:	09 f0 21 49                                     	fmulwd \$r8 = \$r9, -64 \(0xffffffc0\);;

     b88:	09 00 25 c9 00 00 80 17                         	fmulwd \$r9 = \$r9, -8589934592 \(0xfffffffe00000000\);;

     b90:	8a 82 29 59                                     	fmulwd.rn.s \$r10 = \$r10, \$r10;;

     b94:	cb ff 2d ca ff ff ff 97 ff ff ff 10             	fmulwp \$r11 = \$r11, 2305843009213693951 \(0x1fffffffffffffff\);;

     ba0:	0c f0 2d 4a                                     	fmulwp \$r11 = \$r12, -64 \(0xffffffc0\);;

     ba4:	0d 00 31 ca 00 00 80 17                         	fmulwp \$r12 = \$r13, -8589934592 \(0xfffffffe00000000\);;

     bac:	8e 13 35 5a                                     	fmulwp.ru \$r13 = \$r14, \$r14;;

     bb0:	a2 a8 87 5e                                     	fmulwq.rd.s \$r32r33 = \$r34r35, \$r34r35;;

     bb4:	cf ff 3e ce ff ff ff 97 ff ff ff 10             	fmulw \$r15 = \$r15, 2305843009213693951 \(0x1fffffffffffffff\);;

     bc0:	10 f0 42 4e                                     	fmulw \$r16 = \$r16, -64 \(0xffffffc0\);;

     bc4:	11 00 42 ce 00 00 80 17                         	fmulw \$r16 = \$r17, -8589934592 \(0xfffffffe00000000\);;

     bcc:	91 34 46 5e                                     	fmulw.rz \$r17 = \$r17, \$r18;;

     bd0:	40 4c 10 07                                     	fnarrow44wh.rna.s \$a4 = \$a4a5;;

     bd4:	24 65 4b 7c                                     	fnarrowdwp.rnz \$r18 = \$r36r37;;

     bd8:	13 6e 4b 78                                     	fnarrowdw.ro.s \$r18 = \$r19;;

     bdc:	24 67 4f 7e                                     	fnarrowwhq \$r19 = \$r36r37;;

     be0:	14 68 4f 7a                                     	fnarrowwh.rn.s \$r19 = \$r20;;

     be4:	14 20 53 70                                     	fnegd \$r20 = \$r20;;

     be8:	15 20 57 76                                     	fneghq \$r21 = \$r21;;

     bec:	16 20 57 74                                     	fnegwp \$r21 = \$r22;;

     bf0:	16 20 5b 72                                     	fnegw \$r22 = \$r22;;

     bf4:	17 61 5f 72                                     	frecw.ru \$r23 = \$r23;;

     bf8:	18 6a 5f 73                                     	frsrw.rd.s \$r23 = \$r24;;

     bfc:	26 3a 9b 5f                                     	fsbfdc.c.rz \$r38r39 = \$r38r39, \$r40r41;;

     c00:	aa ca a3 5e                                     	fsbfdp.rna.s \$r40r41 = \$r42r43, \$r42r43;;

     c04:	ac 5b b3 5e                                     	fsbfdp.rnz \$r44r45 = \$r44r45, \$r46r47;;

     c08:	d8 ff 62 c4 ff ff ff 97 ff ff ff 10             	fsbfd \$r24 = \$r24, 2305843009213693951 \(0x1fffffffffffffff\);;

     c14:	19 f0 66 44                                     	fsbfd \$r25 = \$r25, -64 \(0xffffffc0\);;

     c18:	1a 00 66 c4 00 00 80 17                         	fsbfd \$r25 = \$r26, -8589934592 \(0xfffffffe00000000\);;

     c20:	da e6 6a 54                                     	fsbfd.ro.s \$r26 = \$r26, \$r27;;

     c24:	db ff 6e c6 ff ff ff 97 ff ff ff 10             	fsbfhq \$r27 = \$r27, 2305843009213693951 \(0x1fffffffffffffff\);;

     c30:	1c f0 72 46                                     	fsbfhq \$r28 = \$r28, -64 \(0xffffffc0\);;

     c34:	1d 00 72 c6 00 00 80 17                         	fsbfhq \$r28 = \$r29, -8589934592 \(0xfffffffe00000000\);;

     c3c:	9d 77 76 56                                     	fsbfhq \$r29 = \$r29, \$r30;;

     c40:	de ff 7a c7 ff ff ff 97 ff ff ff 10             	fsbfwc.c \$r30 = \$r30, 2305843009213693951 \(0x1fffffffffffffff\);;

     c4c:	1f f0 7e 47                                     	fsbfwc.c \$r31 = \$r31, -64 \(0xffffffc0\);;

     c50:	20 00 7e c7 00 00 80 17                         	fsbfwc.c \$r31 = \$r32, -8589934592 \(0xfffffffe00000000\);;

     c58:	60 88 82 57                                     	fsbfwc.c.rn.s \$r32 = \$r32, \$r33;;

     c5c:	30 1c bf 5b                                     	fsbfwcp.c.ru \$r46r47 = \$r48r49, \$r48r49;;

     c60:	32 ad cf 5a                                     	fsbfwq.rd.s \$r50r51 = \$r50r51, \$r52r53;;

     c64:	a1 38 86 55                                     	fsbfwp.rz \$r33 = \$r33, \$r34;;

     c68:	e2 ff 8a c5 ff ff ff 97 ff ff ff 10             	fsbfwp \$r34 = \$r34, 2305843009213693951 \(0x1fffffffffffffff\);;

     c74:	23 f0 8e 45                                     	fsbfwp \$r35 = \$r35, -64 \(0xffffffc0\);;

     c78:	24 00 8e c5 00 00 80 17                         	fsbfwp \$r35 = \$r36, -8589934592 \(0xfffffffe00000000\);;

     c80:	64 c9 92 55                                     	fsbfwp.rna.s \$r36 = \$r36, \$r37;;

     c84:	b6 5d d7 5a                                     	fsbfwq.rnz \$r52r53 = \$r54r55, \$r54r55;;

     c88:	e5 ff 96 cd ff ff ff 97 ff ff ff 10             	fsbfw \$r37 = \$r37, 2305843009213693951 \(0x1fffffffffffffff\);;

     c94:	26 f0 9a 4d                                     	fsbfw \$r38 = \$r38, -64 \(0xffffffc0\);;

     c98:	27 00 9a cd 00 00 80 17                         	fsbfw \$r38 = \$r39, -8589934592 \(0xfffffffe00000000\);;

     ca0:	27 ea 9e 5d                                     	fsbfw.ro.s \$r39 = \$r39, \$r40;;

     ca4:	00 47 10 07                                     	fscalewv \$a4 = \$a4;;

     ca8:	38 58 a3 70                                     	fsdivd.s \$r40 = \$r56r57;;

     cac:	38 50 a3 74                                     	fsdivwp \$r40 = \$r56r57;;

     cb0:	3a 58 a7 72                                     	fsdivw.s \$r41 = \$r58r59;;

     cb4:	29 40 a7 70                                     	fsrecd \$r41 = \$r41;;

     cb8:	2a 48 ab 74                                     	fsrecwp.s \$r42 = \$r42;;

     cbc:	2b 40 ab 72                                     	fsrecw \$r42 = \$r43;;

     cc0:	2b 20 af 78                                     	fsrsrd \$r43 = \$r43;;

     cc4:	2c 20 b3 7c                                     	fsrsrwp \$r44 = \$r44;;

     cc8:	2d 20 b3 7a                                     	fsrsrw \$r44 = \$r45;;

     ccc:	2d 38 b7 7c                                     	fwidenlhwp.s \$r45 = \$r45;;

     cd0:	2e 30 bb 7a                                     	fwidenlhw \$r46 = \$r46;;

     cd4:	2f 38 bb 78                                     	fwidenlwd.s \$r46 = \$r47;;

     cd8:	2f 30 bf 7d                                     	fwidenmhwp \$r47 = \$r47;;

     cdc:	30 38 c3 7b                                     	fwidenmhw.s \$r48 = \$r48;;

     ce0:	31 30 c3 79                                     	fwidenmwd \$r48 = \$r49;;

     ce4:	31 00 c4 0f                                     	get \$r49 = \$pc;;

     ce8:	31 00 c4 0f                                     	get \$r49 = \$pc;;

     cec:	00 00 80 17                                     	goto fffffffffe000cec <main\+0xfffffffffe000cec>;;

     cf0:	f2 ff 5c bc ff ff ff 9f ff ff ff 18             	i1invals 2305843009213693951 \(0x1fffffffffffffff\)\[\$r50\];;

     cfc:	b2 4c 5e bc 00 00 00 98 00 00 80 1f             	i1invals.dlez \$r50\? -1125899906842624 \(0xfffc000000000000\)\[\$r50\];;

     d08:	f3 5c 5e bc 00 00 80 1f                         	i1invals.dgtz \$r51\? -8388608 \(0xff800000\)\[\$r51\];;

     d10:	f4 6c 5e 3c                                     	i1invals.odd \$r51\? \[\$r52\];;

     d14:	34 ed 5e 3c                                     	i1invals \$r52\[\$r52\];;

     d18:	35 f0 5c 3c                                     	i1invals -64 \(0xffffffc0\)\[\$r53\];;

     d1c:	35 00 5c bc 00 00 80 1f                         	i1invals -8589934592 \(0xfffffffe00000000\)\[\$r53\];;

     d24:	00 00 9d 3f                                     	i1inval;;

     d28:	35 00 dc 0f                                     	icall \$r53;;

     d2c:	36 00 cc 0f                                     	iget \$r54;;

     d30:	36 00 d8 0f                                     	igoto \$r54;;

     d34:	f7 71 db 60                                     	insf \$r54 = \$r55, 7 \(0x7\), 7 \(0x7\);;

     d38:	37 4e dd 7c                                     	landd \$r55 = \$r55, \$r56;;

     d3c:	f8 47 e1 fc ff ff ff 00                         	landd \$r56 = \$r56, 536870911 \(0x1fffffff\);;

     d44:	79 7e e5 7c                                     	landhq \$r57 = \$r57, \$r57;;

     d48:	fa 7f e9 fc ff ff ff 00                         	landhq.@ \$r58 = \$r58, 536870911 \(0x1fffffff\);;

     d50:	fb 6e e9 7c                                     	landwp \$r58 = \$r59, \$r59;;

     d54:	fc 67 ed fc ff ff ff 00                         	landwp \$r59 = \$r60, 536870911 \(0x1fffffff\);;

     d5c:	7c 5f f1 7c                                     	landw \$r60 = \$r60, \$r61;;

     d60:	fd 57 f5 fc ff ff ff 00                         	landw \$r61 = \$r61, 536870911 \(0x1fffffff\);;

     d68:	be ef fa 24                                     	lbs \$r62 = \$r62\[\$r62\];;

     d6c:	ff 7f fe a5 00 00 00 98 00 00 80 1f             	lbs.s.even \$r63\? \$r63 = -1125899906842624 \(0xfffc000000000000\)\[\$r63\];;

     d78:	00 80 02 a6 00 00 80 1f                         	lbs.u.wnez \$r0\? \$r0 = -8388608 \(0xff800000\)\[\$r0\];;

     d80:	41 90 06 27                                     	lbs.us.weqz \$r1\? \$r1 = \[\$r1\];;

     d84:	c2 ff 08 a4 ff ff ff 9f ff ff ff 18             	lbs \$r2 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r2\];;

     d90:	03 f0 08 25                                     	lbs.s \$r2 = -64 \(0xffffffc0\)\[\$r3\];;

     d94:	03 00 0c a6 00 00 80 1f                         	lbs.u \$r3 = -8589934592 \(0xfffffffe00000000\)\[\$r3\];;

     d9c:	04 f1 12 23                                     	lbz.us.xs \$r4 = \$r4\[\$r4\];;

     da0:	45 a1 16 a0 00 00 00 98 00 00 80 1f             	lbz.wltz \$r5\? \$r5 = -1125899906842624 \(0xfffc000000000000\)\[\$r5\];;

     dac:	86 b1 1a a1 00 00 80 1f                         	lbz.s.wgez \$r6\? \$r6 = -8388608 \(0xff800000\)\[\$r6\];;

     db4:	c7 c1 1e 22                                     	lbz.u.wlez \$r7\? \$r7 = \[\$r7\];;

     db8:	c8 ff 20 a3 ff ff ff 9f ff ff ff 18             	lbz.us \$r8 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r8\];;

     dc4:	09 f0 20 20                                     	lbz \$r8 = -64 \(0xffffffc0\)\[\$r9\];;

     dc8:	09 00 24 a1 00 00 80 1f                         	lbz.s \$r9 = -8589934592 \(0xfffffffe00000000\)\[\$r9\];;

     dd0:	8a e2 2a 3a                                     	ld.u \$r10 = \$r10\[\$r10\];;

     dd4:	cb d2 2e bb 00 00 00 98 00 00 80 1f             	ld.us.wgtz \$r11\? \$r11 = -1125899906842624 \(0xfffc000000000000\)\[\$r11\];;

     de0:	0d 03 32 b8 00 00 80 1f                         	ld.dnez \$r12\? \$r12 = -8388608 \(0xff800000\)\[\$r13\];;

     de8:	4e 13 3a 39                                     	ld.s.deqz \$r13\? \$r14 = \[\$r14\];;

     dec:	cf ff 3c ba ff ff ff 9f ff ff ff 18             	ld.u \$r15 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r15\];;

     df8:	10 f0 40 3b                                     	ld.us \$r16 = -64 \(0xffffffc0\)\[\$r16\];;

     dfc:	11 00 40 b8 00 00 80 1f                         	ld \$r16 = -8589934592 \(0xfffffffe00000000\)\[\$r17\];;

     e04:	52 f4 46 2d                                     	lhs.s.xs \$r17 = \$r17\[\$r18\];;

     e08:	93 24 4a ae 00 00 00 98 00 00 80 1f             	lhs.u.dltz \$r18\? \$r18 = -1125899906842624 \(0xfffc000000000000\)\[\$r19\];;

     e14:	d4 34 4e af 00 00 80 1f                         	lhs.us.dgez \$r19\? \$r19 = -8388608 \(0xff800000\)\[\$r20\];;

     e1c:	15 45 52 2c                                     	lhs.dlez \$r20\? \$r20 = \[\$r21\];;

     e20:	d5 ff 54 ad ff ff ff 9f ff ff ff 18             	lhs.s \$r21 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r21\];;

     e2c:	16 f0 58 2e                                     	lhs.u \$r22 = -64 \(0xffffffc0\)\[\$r22\];;

     e30:	17 00 58 af 00 00 80 1f                         	lhs.us \$r22 = -8589934592 \(0xfffffffe00000000\)\[\$r23\];;

     e38:	d8 e5 5e 28                                     	lhz \$r23 = \$r23\[\$r24\];;

     e3c:	19 56 62 a9 00 00 00 98 00 00 80 1f             	lhz.s.dgtz \$r24\? \$r24 = -1125899906842624 \(0xfffc000000000000\)\[\$r25\];;

     e48:	5a 66 66 aa 00 00 80 1f                         	lhz.u.odd \$r25\? \$r25 = -8388608 \(0xff800000\)\[\$r26\];;

     e50:	9b 76 6a 2b                                     	lhz.us.even \$r26\? \$r26 = \[\$r27\];;

     e54:	db ff 6c a8 ff ff ff 9f ff ff ff 18             	lhz \$r27 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r27\];;

     e60:	1c f0 70 29                                     	lhz.s \$r28 = -64 \(0xffffffc0\)\[\$r28\];;

     e64:	1d 00 70 aa 00 00 80 1f                         	lhz.u \$r28 = -8589934592 \(0xfffffffe00000000\)\[\$r29\];;

     e6c:	9d 47 75 7d                                     	lnandd \$r29 = \$r29, \$r30;;

     e70:	de 4f 79 fd ff ff ff 00                         	lnandd.@ \$r30 = \$r30, 536870911 \(0x1fffffff\);;

     e78:	df 77 7d 7d                                     	lnandhq \$r31 = \$r31, \$r31;;

     e7c:	e0 77 81 fd ff ff ff 00                         	lnandhq \$r32 = \$r32, 536870911 \(0x1fffffff\);;

     e84:	61 68 81 7d                                     	lnandwp \$r32 = \$r33, \$r33;;

     e88:	e2 6f 85 fd ff ff ff 00                         	lnandwp.@ \$r33 = \$r34, 536870911 \(0x1fffffff\);;

     e90:	e2 58 89 7d                                     	lnandw \$r34 = \$r34, \$r35;;

     e94:	e3 57 8d fd ff ff ff 00                         	lnandw \$r35 = \$r35, 536870911 \(0x1fffffff\);;

     e9c:	24 49 91 7f                                     	lnord \$r36 = \$r36, \$r36;;

     ea0:	e5 47 95 ff ff ff ff 00                         	lnord \$r37 = \$r37, 536870911 \(0x1fffffff\);;

     ea8:	a6 79 95 7f                                     	lnorhq \$r37 = \$r38, \$r38;;

     eac:	e7 7f 99 ff ff ff ff 00                         	lnorhq.@ \$r38 = \$r39, 536870911 \(0x1fffffff\);;

     eb4:	27 6a 9d 7f                                     	lnorwp \$r39 = \$r39, \$r40;;

     eb8:	e8 67 a1 ff ff ff ff 00                         	lnorwp \$r40 = \$r40, 536870911 \(0x1fffffff\);;

     ec0:	69 5a a5 7f                                     	lnorw \$r41 = \$r41, \$r41;;

     ec4:	ea 57 a9 ff ff ff ff 00                         	lnorw \$r42 = \$r42, 536870911 \(0x1fffffff\);;

     ecc:	2a 00 78 0f                                     	loopdo \$r42, ffffffffffff8ecc <main\+0xffffffffffff8ecc>;;

     ed0:	eb 4a ad 7e                                     	lord \$r43 = \$r43, \$r43;;

     ed4:	ec 4f b1 fe ff ff ff 00                         	lord.@ \$r44 = \$r44, 536870911 \(0x1fffffff\);;

     edc:	6d 7b b1 7e                                     	lorhq \$r44 = \$r45, \$r45;;

     ee0:	ee 77 b5 fe ff ff ff 00                         	lorhq \$r45 = \$r46, 536870911 \(0x1fffffff\);;

     ee8:	ee 6b b9 7e                                     	lorwp \$r46 = \$r46, \$r47;;

     eec:	ef 6f bd fe ff ff ff 00                         	lorwp.@ \$r47 = \$r47, 536870911 \(0x1fffffff\);;

     ef4:	30 5c c1 7e                                     	lorw \$r48 = \$r48, \$r48;;

     ef8:	f1 57 c5 fe ff ff ff 00                         	lorw \$r49 = \$r49, 536870911 \(0x1fffffff\);;

     f00:	72 fc 66 3f                                     	lo.us.xs \$r24r25r26r27 = \$r49\[\$r50\];;

     f04:	b2 8c 76 bc 00 00 00 98 00 00 80 1f             	lo.wnez \$r50\? \$r28r29r30r31 = -1125899906842624 \(0xfffc000000000000\)\[\$r50\];;

     f10:	f3 9c 86 bd 00 00 80 1f                         	lo.s.weqz \$r51\? \$r32r33r34r35 = -8388608 \(0xff800000\)\[\$r51\];;

     f18:	f4 ac 96 3e                                     	lo.u.wltz \$r51\? \$r36r37r38r39 = \[\$r52\];;

     f1c:	f4 ff a4 bf ff ff ff 9f ff ff ff 18             	lo.us \$r40r41r42r43 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r52\];;

     f28:	34 f0 b4 3c                                     	lo \$r44r45r46r47 = -64 \(0xffffffc0\)\[\$r52\];;

     f2c:	35 00 c4 bd 00 00 80 1f                         	lo.s \$r48r49r50r51 = -8589934592 \(0xfffffffe00000000\)\[\$r53\];;

     f34:	75 ed ea 3e                                     	lq.u \$r58r59 = \$r53\[\$r53\];;

     f38:	b6 bd f2 bf 00 00 00 98 00 00 80 1f             	lq.us.wgez \$r54\? \$r60r61 = -1125899906842624 \(0xfffc000000000000\)\[\$r54\];;

     f44:	b7 cd f2 bc 00 00 80 1f                         	lq.wlez \$r54\? \$r60r61 = -8388608 \(0xff800000\)\[\$r55\];;

     f4c:	f7 dd fa 3d                                     	lq.s.wgtz \$r55\? \$r62r63 = \[\$r55\];;

     f50:	f8 ff f8 be ff ff ff 9f ff ff ff 18             	lq.u \$r62r63 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r56\];;

     f5c:	38 f0 00 3f                                     	lq.us \$r0r1 = -64 \(0xffffffc0\)\[\$r56\];;

     f60:	38 00 00 bc 00 00 80 1f                         	lq \$r0r1 = -8589934592 \(0xfffffffe00000000\)\[\$r56\];;

     f68:	79 fe e6 35                                     	lws.s.xs \$r57 = \$r57\[\$r57\];;

     f6c:	ba 0e ea b6 00 00 00 98 00 00 80 1f             	lws.u.dnez \$r58\? \$r58 = -1125899906842624 \(0xfffc000000000000\)\[\$r58\];;

     f78:	fb 1e ee b7 00 00 80 1f                         	lws.us.deqz \$r59\? \$r59 = -8388608 \(0xff800000\)\[\$r59\];;

     f80:	3c 2f f2 34                                     	lws.dltz \$r60\? \$r60 = \[\$r60\];;

     f84:	fd ff f4 b5 ff ff ff 9f ff ff ff 18             	lws.s \$r61 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r61\];;

     f90:	3e f0 f4 36                                     	lws.u \$r61 = -64 \(0xffffffc0\)\[\$r62\];;

     f94:	3e 00 f8 b7 00 00 80 1f                         	lws.us \$r62 = -8589934592 \(0xfffffffe00000000\)\[\$r62\];;

     f9c:	ff ef fe 30                                     	lwz \$r63 = \$r63\[\$r63\];;

     fa0:	00 30 02 b1 00 00 00 98 00 00 80 1f             	lwz.s.dgez \$r0\? \$r0 = -1125899906842624 \(0xfffc000000000000\)\[\$r0\];;

     fac:	41 40 06 b2 00 00 80 1f                         	lwz.u.dlez \$r1\? \$r1 = -8388608 \(0xff800000\)\[\$r1\];;

     fb4:	82 50 0a 33                                     	lwz.us.dgtz \$r2\? \$r2 = \[\$r2\];;

     fb8:	c3 ff 0c b0 ff ff ff 9f ff ff ff 18             	lwz \$r3 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r3\];;

     fc4:	04 f0 0c 31                                     	lwz.s \$r3 = -64 \(0xffffffc0\)\[\$r4\];;

     fc8:	04 00 10 b2 00 00 80 1f                         	lwz.u \$r4 = -8589934592 \(0xfffffffe00000000\)\[\$r4\];;

     fd0:	c5 ff 08 c8 ff ff ff 97 ff ff ff 10             	madddt \$r2r3 = \$r5, 2305843009213693951 \(0x1fffffffffffffff\);;

     fdc:	45 11 08 58                                     	madddt \$r2r3 = \$r5, \$r5;;

     fe0:	06 f0 10 48                                     	madddt \$r4r5 = \$r6, -64 \(0xffffffc0\);;

     fe4:	06 00 10 c8 00 00 80 17                         	madddt \$r4r5 = \$r6, -8589934592 \(0xfffffffe00000000\);;

     fec:	c7 ff 18 c0 ff ff ff 97 ff ff ff 10             	maddd \$r6 = \$r7, 2305843009213693951 \(0x1fffffffffffffff\);;

     ff8:	07 02 1c 50                                     	maddd \$r7 = \$r7, \$r8;;

     ffc:	08 f0 20 40                                     	maddd \$r8 = \$r8, -64 \(0xffffffc0\);;

    1000:	09 00 24 c0 00 00 80 17                         	maddd \$r9 = \$r9, -8589934592 \(0xfffffffe00000000\);;

    1008:	ca ff 24 c2 ff ff ff 97 ff ff ff 10             	maddhq \$r9 = \$r10, 2305843009213693951 \(0x1fffffffffffffff\);;

    1014:	ca 02 28 52                                     	maddhq \$r10 = \$r10, \$r11;;

    1018:	0b f0 2c 42                                     	maddhq \$r11 = \$r11, -64 \(0xffffffc0\);;

    101c:	0c 00 30 c2 00 00 80 17                         	maddhq \$r12 = \$r12, -8589934592 \(0xfffffffe00000000\);;

    1024:	4d 43 18 50                                     	maddhwq \$r6r7 = \$r13, \$r13;;

    1028:	ce ff 18 ca ff ff ff 97 ff ff ff 10             	maddsudt \$r6r7 = \$r14, 2305843009213693951 \(0x1fffffffffffffff\);;

    1034:	ce 13 20 5a                                     	maddsudt \$r8r9 = \$r14, \$r15;;

    1038:	0f f0 20 4a                                     	maddsudt \$r8r9 = \$r15, -64 \(0xffffffc0\);;

    103c:	10 00 28 ca 00 00 80 17                         	maddsudt \$r10r11 = \$r16, -8589934592 \(0xfffffffe00000000\);;

    1044:	10 44 28 52                                     	maddsuhwq \$r10r11 = \$r16, \$r16;;

    1048:	51 04 30 5a                                     	maddsuwdp \$r12r13 = \$r17, \$r17;;

    104c:	92 34 44 52                                     	maddsuwd \$r17 = \$r18, \$r18;;

    1050:	d3 37 48 d2 ff ff ff 10                         	maddsuwd \$r18 = \$r19, 536870911 \(0x1fffffff\);;

    1058:	d3 ff 30 c9 ff ff ff 97 ff ff ff 10             	maddudt \$r12r13 = \$r19, 2305843009213693951 \(0x1fffffffffffffff\);;

    1064:	13 15 38 59                                     	maddudt \$r14r15 = \$r19, \$r20;;

    1068:	14 f0 38 49                                     	maddudt \$r14r15 = \$r20, -64 \(0xffffffc0\);;

    106c:	14 00 40 c9 00 00 80 17                         	maddudt \$r16r17 = \$r20, -8589934592 \(0xfffffffe00000000\);;

    1074:	55 45 40 51                                     	madduhwq \$r16r17 = \$r21, \$r21;;

    1078:	95 05 48 59                                     	madduwdp \$r18r19 = \$r21, \$r22;;

    107c:	d6 35 58 51                                     	madduwd \$r22 = \$r22, \$r23;;

    1080:	d7 37 5c d1 ff ff ff 10                         	madduwd \$r23 = \$r23, 536870911 \(0x1fffffff\);;

    1088:	d8 ff 48 cb ff ff ff 97 ff ff ff 10             	madduzdt \$r18r19 = \$r24, 2305843009213693951 \(0x1fffffffffffffff\);;

    1094:	18 16 50 5b                                     	madduzdt \$r20r21 = \$r24, \$r24;;

    1098:	19 f0 50 4b                                     	madduzdt \$r20r21 = \$r25, -64 \(0xffffffc0\);;

    109c:	19 00 58 cb 00 00 80 17                         	madduzdt \$r22r23 = \$r25, -8589934592 \(0xfffffffe00000000\);;

    10a4:	99 06 58 58                                     	maddwdp \$r22r23 = \$r25, \$r26;;

    10a8:	da 36 68 50                                     	maddwd \$r26 = \$r26, \$r27;;

    10ac:	db 37 6c d0 ff ff ff 10                         	maddwd \$r27 = \$r27, 536870911 \(0x1fffffff\);;

    10b4:	dc ff 70 c1 ff ff ff 97 ff ff ff 10             	maddwp \$r28 = \$r28, 2305843009213693951 \(0x1fffffffffffffff\);;

    10c0:	5d 07 70 51                                     	maddwp \$r28 = \$r29, \$r29;;

    10c4:	1e f0 74 41                                     	maddwp \$r29 = \$r30, -64 \(0xffffffc0\);;

    10c8:	1e 00 78 c1 00 00 80 17                         	maddwp \$r30 = \$r30, -8589934592 \(0xfffffffe00000000\);;

    10d0:	df 37 7c 53                                     	maddw \$r31 = \$r31, \$r31;;

    10d4:	e0 37 80 d3 ff ff ff 10                         	maddw \$r32 = \$r32, 536870911 \(0x1fffffff\);;

    10dc:	c0 ff 80 e0 ff ff ff 87 ff ff ff 00             	make \$r32 = 2305843009213693951 \(0x1fffffffffffffff\);;

    10e8:	3c 00 84 e0 00 00 00 00                         	make \$r33 = -549755813888 \(0xffffff8000000000\);;

    10f0:	00 f0 84 60                                     	make \$r33 = -4096 \(0xfffff000\);;

    10f4:	e2 ff 84 e3 ff ff ff 87 ff ff ff 00             	maxd \$r33 = \$r34, 2305843009213693951 \(0x1fffffffffffffff\);;

    1100:	e2 08 89 73                                     	maxd \$r34 = \$r34, \$r35;;

    1104:	23 f0 8c 63                                     	maxd \$r35 = \$r35, -64 \(0xffffffc0\);;

    1108:	24 00 90 e3 00 00 80 07                         	maxd \$r36 = \$r36, -8589934592 \(0xfffffffe00000000\);;

    1110:	e5 0f 91 f3 ff ff ff 00                         	maxd.@ \$r36 = \$r37, 536870911 \(0x1fffffff\);;

    1118:	a5 39 95 73                                     	maxhq \$r37 = \$r37, \$r38;;

    111c:	e6 37 99 f3 ff ff ff 00                         	maxhq \$r38 = \$r38, 536870911 \(0x1fffffff\);;

    1124:	e7 ff 9c e7 ff ff ff 87 ff ff ff 00             	maxud \$r39 = \$r39, 2305843009213693951 \(0x1fffffffffffffff\);;

    1130:	28 0a 9d 77                                     	maxud \$r39 = \$r40, \$r40;;

    1134:	29 f0 a0 67                                     	maxud \$r40 = \$r41, -64 \(0xffffffc0\);;

    1138:	29 00 a4 e7 00 00 80 07                         	maxud \$r41 = \$r41, -8589934592 \(0xfffffffe00000000\);;

    1140:	ea 0f a9 f7 ff ff ff 00                         	maxud.@ \$r42 = \$r42, 536870911 \(0x1fffffff\);;

    1148:	eb 3a a9 77                                     	maxuhq \$r42 = \$r43, \$r43;;

    114c:	ec 3f ad f7 ff ff ff 00                         	maxuhq.@ \$r43 = \$r44, 536870911 \(0x1fffffff\);;

    1154:	6c 2b b1 77                                     	maxuwp \$r44 = \$r44, \$r45;;

    1158:	ed 27 b5 f7 ff ff ff 00                         	maxuwp \$r45 = \$r45, 536870911 \(0x1fffffff\);;

    1160:	ae 1b b9 77                                     	maxuw \$r46 = \$r46, \$r46;;

    1164:	2f f0 bc 77                                     	maxuw \$r47 = \$r47, -64 \(0xffffffc0\);;

    1168:	30 00 bc f7 00 00 80 07                         	maxuw \$r47 = \$r48, -8589934592 \(0xfffffffe00000000\);;

    1170:	70 2c c1 73                                     	maxwp \$r48 = \$r48, \$r49;;

    1174:	f1 2f c5 f3 ff ff ff 00                         	maxwp.@ \$r49 = \$r49, 536870911 \(0x1fffffff\);;

    117c:	b2 1c c9 73                                     	maxw \$r50 = \$r50, \$r50;;

    1180:	33 f0 cc 73                                     	maxw \$r51 = \$r51, -64 \(0xffffffc0\);;

    1184:	34 00 cc f3 00 00 80 07                         	maxw \$r51 = \$r52, -8589934592 \(0xfffffffe00000000\);;

    118c:	f4 ff d0 e2 ff ff ff 87 ff ff ff 00             	mind \$r52 = \$r52, 2305843009213693951 \(0x1fffffffffffffff\);;

    1198:	75 0d d5 72                                     	mind \$r53 = \$r53, \$r53;;

    119c:	36 f0 d8 62                                     	mind \$r54 = \$r54, -64 \(0xffffffc0\);;

    11a0:	37 00 d8 e2 00 00 80 07                         	mind \$r54 = \$r55, -8589934592 \(0xfffffffe00000000\);;

    11a8:	f7 0f dd f2 ff ff ff 00                         	mind.@ \$r55 = \$r55, 536870911 \(0x1fffffff\);;

    11b0:	38 3e e1 72                                     	minhq \$r56 = \$r56, \$r56;;

    11b4:	f9 37 e5 f2 ff ff ff 00                         	minhq \$r57 = \$r57, 536870911 \(0x1fffffff\);;

    11bc:	fa ff e4 e6 ff ff ff 87 ff ff ff 00             	minud \$r57 = \$r58, 2305843009213693951 \(0x1fffffffffffffff\);;

    11c8:	fa 0e e9 76                                     	minud \$r58 = \$r58, \$r59;;

    11cc:	3b f0 ec 66                                     	minud \$r59 = \$r59, -64 \(0xffffffc0\);;

    11d0:	3c 00 f0 e6 00 00 80 07                         	minud \$r60 = \$r60, -8589934592 \(0xfffffffe00000000\);;

    11d8:	fd 0f f1 f6 ff ff ff 00                         	minud.@ \$r60 = \$r61, 536870911 \(0x1fffffff\);;

    11e0:	bd 3f f5 76                                     	minuhq \$r61 = \$r61, \$r62;;

    11e4:	fe 3f f9 f6 ff ff ff 00                         	minuhq.@ \$r62 = \$r62, 536870911 \(0x1fffffff\);;

    11ec:	ff 2f fd 76                                     	minuwp \$r63 = \$r63, \$r63;;

    11f0:	c0 27 01 f6 ff ff ff 00                         	minuwp \$r0 = \$r0, 536870911 \(0x1fffffff\);;

    11f8:	41 10 01 76                                     	minuw \$r0 = \$r1, \$r1;;

    11fc:	02 f0 04 76                                     	minuw \$r1 = \$r2, -64 \(0xffffffc0\);;

    1200:	02 00 08 f6 00 00 80 07                         	minuw \$r2 = \$r2, -8589934592 \(0xfffffffe00000000\);;

    1208:	c3 20 0d 72                                     	minwp \$r3 = \$r3, \$r3;;

    120c:	c4 2f 11 f2 ff ff ff 00                         	minwp.@ \$r4 = \$r4, 536870911 \(0x1fffffff\);;

    1214:	45 11 11 72                                     	minw \$r4 = \$r5, \$r5;;

    1218:	06 f0 14 72                                     	minw \$r5 = \$r6, -64 \(0xffffffc0\);;

    121c:	06 00 18 f2 00 00 80 07                         	minw \$r6 = \$r6, -8589934592 \(0xfffffffe00000000\);;

    1224:	c7 11 60 53                                     	mm212w \$r24r25 = \$r7, \$r7;;

    1228:	07 02 60 5b                                     	mma212w \$r24r25 = \$r7, \$r8;;

    122c:	45 c1 61 04                                     	mma444hbd0 \$a24a25a26a27 = \$a28a29a30a31, \$a5, \$a5;;

    1230:	46 41 86 04                                     	mma444hbd1 \$a32a33a34a35 = \$a36a37a38a39, \$a5, \$a6;;

    1234:	86 c1 aa 04                                     	mma444hd \$a40a41a42a43 = \$a44a45a46a47, \$a6, \$a6;;

    1238:	c7 61 c3 04                                     	mma444suhbd0 \$a48a49a50a51 = \$a52a53a54a55, \$a7, \$a7;;

    123c:	c8 e1 e7 04                                     	mma444suhbd1 \$a56a57a58a59 = \$a60a61a62a63, \$a7, \$a8;;

    1240:	08 62 08 04                                     	mma444suhd \$a0a1a2a3 = \$a4a5a6a7, \$a8, \$a8;;

    1244:	49 d2 20 04                                     	mma444uhbd0 \$a8a9a10a11 = \$a12a13a14a15, \$a9, \$a9;;

    1248:	4a 52 45 04                                     	mma444uhbd1 \$a16a17a18a19 = \$a20a21a22a23, \$a9, \$a10;;

    124c:	8a d2 69 04                                     	mma444uhd \$a24a25a26a27 = \$a28a29a30a31, \$a10, \$a10;;

    1250:	cb 72 82 04                                     	mma444ushbd0 \$a32a33a34a35 = \$a36a37a38a39, \$a11, \$a11;;

    1254:	cc f2 a6 04                                     	mma444ushbd1 \$a40a41a42a43 = \$a44a45a46a47, \$a11, \$a12;;

    1258:	0c 73 cb 04                                     	mma444ushd \$a48a49a50a51 = \$a52a53a54a55, \$a12, \$a12;;

    125c:	08 02 68 5f                                     	mms212w \$r26r27 = \$r8, \$r8;;

    1260:	49 e2 02 7f                                     	movetq \$a0_lo = \$r9, \$r9;;

    1264:	89 f2 02 7f                                     	movetq \$a0_hi = \$r9, \$r10;;

    1268:	8a 12 68 5c                                     	msbfdt \$r26r27 = \$r10, \$r10;;

    126c:	cb 02 2c 54                                     	msbfd \$r11 = \$r11, \$r11;;

    1270:	4c 03 30 56                                     	msbfhq \$r12 = \$r12, \$r13;;

    1274:	8d 43 70 54                                     	msbfhwq \$r28r29 = \$r13, \$r14;;

    1278:	ce 13 70 5e                                     	msbfsudt \$r28r29 = \$r14, \$r15;;

    127c:	0f 44 78 56                                     	msbfsuhwq \$r30r31 = \$r15, \$r16;;

    1280:	10 04 78 5e                                     	msbfsuwdp \$r30r31 = \$r16, \$r16;;

    1284:	51 34 44 56                                     	msbfsuwd \$r17 = \$r17, \$r17;;

    1288:	d2 37 48 d6 ff ff ff 10                         	msbfsuwd \$r18 = \$r18, 536870911 \(0x1fffffff\);;

    1290:	d2 14 80 5d                                     	msbfudt \$r32r33 = \$r18, \$r19;;

    1294:	d3 44 80 55                                     	msbfuhwq \$r32r33 = \$r19, \$r19;;

    1298:	14 05 88 5d                                     	msbfuwdp \$r34r35 = \$r20, \$r20;;

    129c:	55 35 50 55                                     	msbfuwd \$r20 = \$r21, \$r21;;

    12a0:	d6 37 54 d5 ff ff ff 10                         	msbfuwd \$r21 = \$r22, 536870911 \(0x1fffffff\);;

    12a8:	96 15 88 5f                                     	msbfuzdt \$r34r35 = \$r22, \$r22;;

    12ac:	d7 05 90 5c                                     	msbfwdp \$r36r37 = \$r23, \$r23;;

    12b0:	18 36 5c 54                                     	msbfwd \$r23 = \$r24, \$r24;;

    12b4:	d9 37 60 d4 ff ff ff 10                         	msbfwd \$r24 = \$r25, 536870911 \(0x1fffffff\);;

    12bc:	99 06 64 55                                     	msbfwp \$r25 = \$r25, \$r26;;

    12c0:	da 36 68 57                                     	msbfw \$r26 = \$r26, \$r27;;

    12c4:	db 37 6c d7 ff ff ff 10                         	msbfw \$r27 = \$r27, 536870911 \(0x1fffffff\);;

    12cc:	dc ff 94 c8 ff ff ff 97 ff ff ff 10             	muldt \$r36r37 = \$r28, 2305843009213693951 \(0x1fffffffffffffff\);;

    12d8:	1c 17 9c 58                                     	muldt \$r38r39 = \$r28, \$r28;;

    12dc:	1d f0 9c 48                                     	muldt \$r38r39 = \$r29, -64 \(0xffffffc0\);;

    12e0:	1d 00 a4 c8 00 00 80 17                         	muldt \$r40r41 = \$r29, -8589934592 \(0xfffffffe00000000\);;

    12e8:	de ff 74 c4 ff ff ff 97 ff ff ff 10             	muld \$r29 = \$r30, 2305843009213693951 \(0x1fffffffffffffff\);;

    12f4:	de 17 78 54                                     	muld \$r30 = \$r30, \$r31;;

    12f8:	1f f0 7c 44                                     	muld \$r31 = \$r31, -64 \(0xffffffc0\);;

    12fc:	20 00 80 c4 00 00 80 17                         	muld \$r32 = \$r32, -8589934592 \(0xfffffffe00000000\);;

    1304:	e1 ff 80 c6 ff ff ff 97 ff ff ff 10             	mulhq \$r32 = \$r33, 2305843009213693951 \(0x1fffffffffffffff\);;

    1310:	a1 18 84 56                                     	mulhq \$r33 = \$r33, \$r34;;

    1314:	22 f0 88 46                                     	mulhq \$r34 = \$r34, -64 \(0xffffffc0\);;

    1318:	23 00 8c c6 00 00 80 17                         	mulhq \$r35 = \$r35, -8589934592 \(0xfffffffe00000000\);;

    1320:	23 49 a0 58                                     	mulhwq \$r40r41 = \$r35, \$r36;;

    1324:	e4 ff ac ca ff ff ff 97 ff ff ff 10             	mulsudt \$r42r43 = \$r36, 2305843009213693951 \(0x1fffffffffffffff\);;

    1330:	64 19 ac 5a                                     	mulsudt \$r42r43 = \$r36, \$r37;;

    1334:	25 f0 b4 4a                                     	mulsudt \$r44r45 = \$r37, -64 \(0xffffffc0\);;

    1338:	25 00 b4 ca 00 00 80 17                         	mulsudt \$r44r45 = \$r37, -8589934592 \(0xfffffffe00000000\);;

    1340:	a6 49 b8 5a                                     	mulsuhwq \$r46r47 = \$r38, \$r38;;

    1344:	e6 19 b8 52                                     	mulsuwdp \$r46r47 = \$r38, \$r39;;

    1348:	27 3a 9c 5a                                     	mulsuwd \$r39 = \$r39, \$r40;;

    134c:	e8 37 a0 da ff ff ff 10                         	mulsuwd \$r40 = \$r40, 536870911 \(0x1fffffff\);;

    1354:	e9 ff c4 c9 ff ff ff 97 ff ff ff 10             	muludt \$r48r49 = \$r41, 2305843009213693951 \(0x1fffffffffffffff\);;

    1360:	69 1a c4 59                                     	muludt \$r48r49 = \$r41, \$r41;;

    1364:	2a f0 cc 49                                     	muludt \$r50r51 = \$r42, -64 \(0xffffffc0\);;

    1368:	2a 00 cc c9 00 00 80 17                         	muludt \$r50r51 = \$r42, -8589934592 \(0xfffffffe00000000\);;

    1370:	ea 4a d0 59                                     	muluhwq \$r52r53 = \$r42, \$r43;;

    1374:	eb 1a d0 51                                     	muluwdp \$r52r53 = \$r43, \$r43;;

    1378:	2c 3b b0 59                                     	muluwd \$r44 = \$r44, \$r44;;

    137c:	ed 37 b4 d9 ff ff ff 10                         	muluwd \$r45 = \$r45, 536870911 \(0x1fffffff\);;

    1384:	ae 2b b4 55                                     	mulwc.c \$r45 = \$r46, \$r46;;

    1388:	ef ff b8 c7 ff ff ff 97 ff ff ff 10             	mulwc \$r46 = \$r47, 2305843009213693951 \(0x1fffffffffffffff\);;

    1394:	2f 1c bc 57                                     	mulwc \$r47 = \$r47, \$r48;;

    1398:	30 f0 c0 47                                     	mulwc \$r48 = \$r48, -64 \(0xffffffc0\);;

    139c:	31 00 c4 c7 00 00 80 17                         	mulwc \$r49 = \$r49, -8589934592 \(0xfffffffe00000000\);;

    13a4:	b1 2c d8 57                                     	mulwdc.c \$r54r55 = \$r49, \$r50;;

    13a8:	b2 2c d8 56                                     	mulwdc \$r54r55 = \$r50, \$r50;;

    13ac:	f3 1c e0 50                                     	mulwdp \$r56r57 = \$r51, \$r51;;

    13b0:	34 3d cc 58                                     	mulwd \$r51 = \$r52, \$r52;;

    13b4:	f5 37 d0 d8 ff ff ff 10                         	mulwd \$r52 = \$r53, 536870911 \(0x1fffffff\);;

    13bc:	f5 ff d4 c5 ff ff ff 97 ff ff ff 10             	mulwp \$r53 = \$r53, 2305843009213693951 \(0x1fffffffffffffff\);;

    13c8:	b6 1d d8 55                                     	mulwp \$r54 = \$r54, \$r54;;

    13cc:	37 f0 dc 45                                     	mulwp \$r55 = \$r55, -64 \(0xffffffc0\);;

    13d0:	38 00 dc c5 00 00 80 17                         	mulwp \$r55 = \$r56, -8589934592 \(0xfffffffe00000000\);;

    13d8:	ba 2e e0 54                                     	mulwq \$r56r57 = \$r58r59, \$r58r59;;

    13dc:	78 3e e0 5b                                     	mulw \$r56 = \$r56, \$r57;;

    13e0:	f9 37 e4 db ff ff ff 10                         	mulw \$r57 = \$r57, 536870911 \(0x1fffffff\);;

    13e8:	fa ff e8 e9 ff ff ff 87 ff ff ff 00             	nandd \$r58 = \$r58, 2305843009213693951 \(0x1fffffffffffffff\);;

    13f4:	fb 0e e9 79                                     	nandd \$r58 = \$r59, \$r59;;

    13f8:	3c f0 ec 69                                     	nandd \$r59 = \$r60, -64 \(0xffffffc0\);;

    13fc:	3c 00 f0 e9 00 00 80 07                         	nandd \$r60 = \$r60, -8589934592 \(0xfffffffe00000000\);;

    1404:	fd 0f f5 f9 ff ff ff 00                         	nandd.@ \$r61 = \$r61, 536870911 \(0x1fffffff\);;

    140c:	be 1f f5 79                                     	nandw \$r61 = \$r62, \$r62;;

    1410:	3f f0 f8 79                                     	nandw \$r62 = \$r63, -64 \(0xffffffc0\);;

    1414:	3f 00 fc f9 00 00 80 07                         	nandw \$r63 = \$r63, -8589934592 \(0xfffffffe00000000\);;

    141c:	00 00 00 65                                     	negd \$r0 = \$r0;;

    1420:	01 30 01 f5 00 00 00 00                         	neghq \$r0 = \$r1;;

    1428:	01 20 05 f5 00 00 00 00                         	negwp \$r1 = \$r1;;

    1430:	02 00 08 75                                     	negw \$r2 = \$r2;;

    1434:	00 f0 03 7f                                     	nop;;

    1438:	c3 ff 08 eb ff ff ff 87 ff ff ff 00             	nord \$r2 = \$r3, 2305843009213693951 \(0x1fffffffffffffff\);;

    1444:	03 01 0d 7b                                     	nord \$r3 = \$r3, \$r4;;

    1448:	04 f0 10 6b                                     	nord \$r4 = \$r4, -64 \(0xffffffc0\);;

    144c:	05 00 14 eb 00 00 80 07                         	nord \$r5 = \$r5, -8589934592 \(0xfffffffe00000000\);;

    1454:	c6 0f 15 fb ff ff ff 00                         	nord.@ \$r5 = \$r6, 536870911 \(0x1fffffff\);;

    145c:	c6 11 19 7b                                     	norw \$r6 = \$r6, \$r7;;

    1460:	07 f0 1c 7b                                     	norw \$r7 = \$r7, -64 \(0xffffffc0\);;

    1464:	08 00 20 fb 00 00 80 07                         	norw \$r8 = \$r8, -8589934592 \(0xfffffffe00000000\);;

    146c:	c9 ff 20 6c                                     	notd \$r8 = \$r9;;

    1470:	c9 ff 24 7c                                     	notw \$r9 = \$r9;;

    1474:	ca ff 28 ed ff ff ff 87 ff ff ff 00             	nxord \$r10 = \$r10, 2305843009213693951 \(0x1fffffffffffffff\);;

    1480:	cb 02 29 7d                                     	nxord \$r10 = \$r11, \$r11;;

    1484:	0c f0 2c 6d                                     	nxord \$r11 = \$r12, -64 \(0xffffffc0\);;

    1488:	0d 00 30 ed 00 00 80 07                         	nxord \$r12 = \$r13, -8589934592 \(0xfffffffe00000000\);;

    1490:	ce 0f 35 fd ff ff ff 00                         	nxord.@ \$r13 = \$r14, 536870911 \(0x1fffffff\);;

    1498:	cf 13 39 7d                                     	nxorw \$r14 = \$r15, \$r15;;

    149c:	10 f0 40 7d                                     	nxorw \$r16 = \$r16, -64 \(0xffffffc0\);;

    14a0:	11 00 40 fd 00 00 80 07                         	nxorw \$r16 = \$r17, -8589934592 \(0xfffffffe00000000\);;

    14a8:	d1 ff 44 ea ff ff ff 87 ff ff ff 00             	ord \$r17 = \$r17, 2305843009213693951 \(0x1fffffffffffffff\);;

    14b4:	92 04 49 7a                                     	ord \$r18 = \$r18, \$r18;;

    14b8:	13 f0 4c 6a                                     	ord \$r19 = \$r19, -64 \(0xffffffc0\);;

    14bc:	14 00 4c ea 00 00 80 07                         	ord \$r19 = \$r20, -8589934592 \(0xfffffffe00000000\);;

    14c4:	d4 0f 51 fa ff ff ff 00                         	ord.@ \$r20 = \$r20, 536870911 \(0x1fffffff\);;

    14cc:	d5 ff 54 ef ff ff ff 87 ff ff ff 00             	ornd \$r21 = \$r21, 2305843009213693951 \(0x1fffffffffffffff\);;

    14d8:	96 05 55 7f                                     	ornd \$r21 = \$r22, \$r22;;

    14dc:	17 f0 58 6f                                     	ornd \$r22 = \$r23, -64 \(0xffffffc0\);;

    14e0:	17 00 5c ef 00 00 80 07                         	ornd \$r23 = \$r23, -8589934592 \(0xfffffffe00000000\);;

    14e8:	d8 0f 61 ff ff ff ff 00                         	ornd.@ \$r24 = \$r24, 536870911 \(0x1fffffff\);;

    14f0:	59 16 61 7f                                     	ornw \$r24 = \$r25, \$r25;;

    14f4:	1a f0 64 7f                                     	ornw \$r25 = \$r26, -64 \(0xffffffc0\);;

    14f8:	1a 00 68 ff 00 00 80 07                         	ornw \$r26 = \$r26, -8589934592 \(0xfffffffe00000000\);;

    1500:	db 16 6d 7a                                     	orw \$r27 = \$r27, \$r27;;

    1504:	1c f0 70 7a                                     	orw \$r28 = \$r28, -64 \(0xffffffc0\);;

    1508:	1d 00 70 fa 00 00 80 07                         	orw \$r28 = \$r29, -8589934592 \(0xfffffffe00000000\);;

    1510:	c0 ff 74 f0 ff ff ff 87 ff ff ff 00             	pcrel \$r29 = 2305843009213693951 \(0x1fffffffffffffff\);;

    151c:	3c 00 74 f0 00 00 00 00                         	pcrel \$r29 = -549755813888 \(0xffffff8000000000\);;

    1524:	00 f0 78 70                                     	pcrel \$r30 = -4096 \(0xfffff000\);;

    1528:	00 00 d0 0f                                     	ret;;

    152c:	00 00 d4 0f                                     	rfe;;

    1530:	de 87 7a 7e                                     	rolwps \$r30 = \$r30, \$r31;;

    1534:	df 41 7e 7e                                     	rolwps \$r31 = \$r31, 7 \(0x7\);;

    1538:	20 78 82 7e                                     	rolw \$r32 = \$r32, \$r32;;

    153c:	e1 31 86 7e                                     	rolw \$r33 = \$r33, 7 \(0x7\);;

    1540:	a2 88 86 7f                                     	rorwps \$r33 = \$r34, \$r34;;

    1544:	e3 41 8a 7f                                     	rorwps \$r34 = \$r35, 7 \(0x7\);;

    1548:	23 79 8e 7f                                     	rorw \$r35 = \$r35, \$r36;;

    154c:	e4 31 92 7f                                     	rorw \$r36 = \$r36, 7 \(0x7\);;

    1550:	25 07 c8 0f                                     	rswap \$r37 = \$mmc;;

    1554:	25 00 c8 0f                                     	rswap \$r37 = \$pc;;

    1558:	25 00 c8 0f                                     	rswap \$r37 = \$pc;;

    155c:	26 24 9a 7e                                     	satdh \$r38 = \$r38;;

    1560:	27 28 9a 7e                                     	satdw \$r38 = \$r39;;

    1564:	27 6a 9e 7e                                     	satd \$r39 = \$r39, \$r40;;

    1568:	e8 21 a2 7e                                     	satd \$r40 = \$r40, 7 \(0x7\);;

    156c:	69 da a5 7f                                     	sbfcd.i \$r41 = \$r41, \$r41;;

    1570:	ea d7 a9 ff ff ff ff 00                         	sbfcd.i \$r42 = \$r42, 536870911 \(0x1fffffff\);;

    1578:	eb ca a9 7f                                     	sbfcd \$r42 = \$r43, \$r43;;

    157c:	ec c7 ad ff ff ff ff 00                         	sbfcd \$r43 = \$r44, 536870911 \(0x1fffffff\);;

    1584:	ec ff b0 e5 ff ff ff 87 ff ff ff 00             	sbfd \$r44 = \$r44, 2305843009213693951 \(0x1fffffffffffffff\);;

    1590:	6d 0b b5 75                                     	sbfd \$r45 = \$r45, \$r45;;

    1594:	2e f0 b8 65                                     	sbfd \$r46 = \$r46, -64 \(0xffffffc0\);;

    1598:	2f 00 b8 e5 00 00 80 07                         	sbfd \$r46 = \$r47, -8589934592 \(0xfffffffe00000000\);;

    15a0:	ef 0f bd f5 ff ff ff 00                         	sbfd.@ \$r47 = \$r47, 536870911 \(0x1fffffff\);;

    15a8:	30 3c c1 7d                                     	sbfhcp.c \$r48 = \$r48, \$r48;;

    15ac:	f1 37 c5 fd ff ff ff 00                         	sbfhcp.c \$r49 = \$r49, 536870911 \(0x1fffffff\);;

    15b4:	b2 3c c5 75                                     	sbfhq \$r49 = \$r50, \$r50;;

    15b8:	f3 3f c9 f5 ff ff ff 00                         	sbfhq.@ \$r50 = \$r51, 536870911 \(0x1fffffff\);;

    15c0:	f3 ff cd ef ff ff ff 87 ff ff ff 00             	sbfsd \$r51 = \$r51, 2305843009213693951 \(0x1fffffffffffffff\);;

    15cc:	34 ad d1 7f                                     	sbfsd \$r52 = \$r52, \$r52;;

    15d0:	35 f0 d5 6f                                     	sbfsd \$r53 = \$r53, -64 \(0xffffffc0\);;

    15d4:	36 00 d5 ef 00 00 80 07                         	sbfsd \$r53 = \$r54, -8589934592 \(0xfffffffe00000000\);;

    15dc:	f6 fd d9 7f                                     	sbfshq \$r54 = \$r54, \$r55;;

    15e0:	f7 f7 dd ff ff ff ff 00                         	sbfshq \$r55 = \$r55, 536870911 \(0x1fffffff\);;

    15e8:	38 ee e1 7f                                     	sbfswp \$r56 = \$r56, \$r56;;

    15ec:	f9 ef e5 ff ff ff ff 00                         	sbfswp.@ \$r57 = \$r57, 536870911 \(0x1fffffff\);;

    15f4:	ba be e5 7f                                     	sbfsw \$r57 = \$r58, \$r58;;

    15f8:	fb b7 e9 ff ff ff ff 00                         	sbfsw \$r58 = \$r59, 536870911 \(0x1fffffff\);;

    1600:	3b 4f ed 7b                                     	sbfuwd \$r59 = \$r59, \$r60;;

    1604:	fc 47 f1 fb ff ff ff 00                         	sbfuwd \$r60 = \$r60, 536870911 \(0x1fffffff\);;

    160c:	7d 2f f5 7d                                     	sbfwc.c \$r61 = \$r61, \$r61;;

    1610:	fe 2f f9 fd ff ff ff 00                         	sbfwc.c.@ \$r62 = \$r62, 536870911 \(0x1fffffff\);;

    1618:	ff 4f f9 79                                     	sbfwd \$r62 = \$r63, \$r63;;

    161c:	c0 47 fd f9 ff ff ff 00                         	sbfwd \$r63 = \$r0, 536870911 \(0x1fffffff\);;

    1624:	40 20 01 75                                     	sbfwp \$r0 = \$r0, \$r1;;

    1628:	c1 2f 05 f5 ff ff ff 00                         	sbfwp.@ \$r1 = \$r1, 536870911 \(0x1fffffff\);;

    1630:	82 10 09 75                                     	sbfw \$r2 = \$r2, \$r2;;

    1634:	03 f0 0c 75                                     	sbfw \$r3 = \$r3, -64 \(0xffffffc0\);;

    1638:	04 00 0c f5 00 00 80 07                         	sbfw \$r3 = \$r4, -8589934592 \(0xfffffffe00000000\);;

    1640:	44 41 11 77                                     	sbfx16d \$r4 = \$r4, \$r5;;

    1644:	c5 47 15 f7 ff ff ff 00                         	sbfx16d \$r5 = \$r5, 536870911 \(0x1fffffff\);;

    164c:	86 71 19 77                                     	sbfx16hq \$r6 = \$r6, \$r6;;

    1650:	c7 7f 1d f7 ff ff ff 00                         	sbfx16hq.@ \$r7 = \$r7, 536870911 \(0x1fffffff\);;

    1658:	08 82 1d 7f                                     	sbfx16uwd \$r7 = \$r8, \$r8;;

    165c:	c9 87 21 ff ff ff ff 00                         	sbfx16uwd \$r8 = \$r9, 536870911 \(0x1fffffff\);;

    1664:	89 82 25 77                                     	sbfx16wd \$r9 = \$r9, \$r10;;

    1668:	ca 87 29 f7 ff ff ff 00                         	sbfx16wd \$r10 = \$r10, 536870911 \(0x1fffffff\);;

    1670:	cb 62 2d 77                                     	sbfx16wp \$r11 = \$r11, \$r11;;

    1674:	cc 67 31 f7 ff ff ff 00                         	sbfx16wp \$r12 = \$r12, 536870911 \(0x1fffffff\);;

    167c:	8d 53 35 77                                     	sbfx16w \$r13 = \$r13, \$r14;;

    1680:	cf 57 39 f7 ff ff ff 00                         	sbfx16w \$r14 = \$r15, 536870911 \(0x1fffffff\);;

    1688:	10 44 3d 71                                     	sbfx2d \$r15 = \$r16, \$r16;;

    168c:	d1 4f 41 f1 ff ff ff 00                         	sbfx2d.@ \$r16 = \$r17, 536870911 \(0x1fffffff\);;

    1694:	91 74 45 71                                     	sbfx2hq \$r17 = \$r17, \$r18;;

    1698:	d2 77 49 f1 ff ff ff 00                         	sbfx2hq \$r18 = \$r18, 536870911 \(0x1fffffff\);;

    16a0:	d3 84 4d 79                                     	sbfx2uwd \$r19 = \$r19, \$r19;;

    16a4:	d4 87 51 f9 ff ff ff 00                         	sbfx2uwd \$r20 = \$r20, 536870911 \(0x1fffffff\);;

    16ac:	55 85 51 71                                     	sbfx2wd \$r20 = \$r21, \$r21;;

    16b0:	d6 87 55 f1 ff ff ff 00                         	sbfx2wd \$r21 = \$r22, 536870911 \(0x1fffffff\);;

    16b8:	d6 65 59 71                                     	sbfx2wp \$r22 = \$r22, \$r23;;

    16bc:	d7 6f 5d f1 ff ff ff 00                         	sbfx2wp.@ \$r23 = \$r23, 536870911 \(0x1fffffff\);;

    16c4:	18 56 61 71                                     	sbfx2w \$r24 = \$r24, \$r24;;

    16c8:	d9 57 65 f1 ff ff ff 00                         	sbfx2w \$r25 = \$r25, 536870911 \(0x1fffffff\);;

    16d0:	9a 46 65 73                                     	sbfx4d \$r25 = \$r26, \$r26;;

    16d4:	db 47 69 f3 ff ff ff 00                         	sbfx4d \$r26 = \$r27, 536870911 \(0x1fffffff\);;

    16dc:	1b 77 6d 73                                     	sbfx4hq \$r27 = \$r27, \$r28;;

    16e0:	dc 7f 71 f3 ff ff ff 00                         	sbfx4hq.@ \$r28 = \$r28, 536870911 \(0x1fffffff\);;

    16e8:	5d 87 75 7b                                     	sbfx4uwd \$r29 = \$r29, \$r29;;

    16ec:	de 87 79 fb ff ff ff 00                         	sbfx4uwd \$r30 = \$r30, 536870911 \(0x1fffffff\);;

    16f4:	df 87 79 73                                     	sbfx4wd \$r30 = \$r31, \$r31;;

    16f8:	e0 87 7d f3 ff ff ff 00                         	sbfx4wd \$r31 = \$r32, 536870911 \(0x1fffffff\);;

    1700:	60 68 81 73                                     	sbfx4wp \$r32 = \$r32, \$r33;;

    1704:	e1 67 85 f3 ff ff ff 00                         	sbfx4wp \$r33 = \$r33, 536870911 \(0x1fffffff\);;

    170c:	a2 58 89 73                                     	sbfx4w \$r34 = \$r34, \$r34;;

    1710:	e3 57 8d f3 ff ff ff 00                         	sbfx4w \$r35 = \$r35, 536870911 \(0x1fffffff\);;

    1718:	24 49 8d 75                                     	sbfx8d \$r35 = \$r36, \$r36;;

    171c:	e5 4f 91 f5 ff ff ff 00                         	sbfx8d.@ \$r36 = \$r37, 536870911 \(0x1fffffff\);;

    1724:	a5 79 95 75                                     	sbfx8hq \$r37 = \$r37, \$r38;;

    1728:	e6 77 99 f5 ff ff ff 00                         	sbfx8hq \$r38 = \$r38, 536870911 \(0x1fffffff\);;

    1730:	e7 89 9d 7d                                     	sbfx8uwd \$r39 = \$r39, \$r39;;

    1734:	e8 87 a1 fd ff ff ff 00                         	sbfx8uwd \$r40 = \$r40, 536870911 \(0x1fffffff\);;

    173c:	69 8a a1 75                                     	sbfx8wd \$r40 = \$r41, \$r41;;

    1740:	ea 87 a5 f5 ff ff ff 00                         	sbfx8wd \$r41 = \$r42, 536870911 \(0x1fffffff\);;

    1748:	ea 6a a9 75                                     	sbfx8wp \$r42 = \$r42, \$r43;;

    174c:	eb 6f ad f5 ff ff ff 00                         	sbfx8wp.@ \$r43 = \$r43, 536870911 \(0x1fffffff\);;

    1754:	2c 5b b1 75                                     	sbfx8w \$r44 = \$r44, \$r44;;

    1758:	ed 57 b5 f5 ff ff ff 00                         	sbfx8w \$r45 = \$r45, 536870911 \(0x1fffffff\);;

    1760:	ee ff b6 ee ff ff ff 87 ff ff ff 00             	sbmm8 \$r45 = \$r46, 2305843009213693951 \(0x1fffffffffffffff\);;

    176c:	ee 0b ba 7e                                     	sbmm8 \$r46 = \$r46, \$r47;;

    1770:	2f f0 be 6e                                     	sbmm8 \$r47 = \$r47, -64 \(0xffffffc0\);;

    1774:	30 00 c2 ee 00 00 80 07                         	sbmm8 \$r48 = \$r48, -8589934592 \(0xfffffffe00000000\);;

    177c:	f1 0f c2 fe ff ff ff 00                         	sbmm8.@ \$r48 = \$r49, 536870911 \(0x1fffffff\);;

    1784:	f1 ff c6 ef ff ff ff 87 ff ff ff 00             	sbmmt8 \$r49 = \$r49, 2305843009213693951 \(0x1fffffffffffffff\);;

    1790:	b2 0c ca 7f                                     	sbmmt8 \$r50 = \$r50, \$r50;;

    1794:	33 f0 ce 6f                                     	sbmmt8 \$r51 = \$r51, -64 \(0xffffffc0\);;

    1798:	34 00 ce ef 00 00 80 07                         	sbmmt8 \$r51 = \$r52, -8589934592 \(0xfffffffe00000000\);;

    17a0:	f4 0f d2 ff ff ff ff 00                         	sbmmt8.@ \$r52 = \$r52, 536870911 \(0x1fffffff\);;

    17a8:	75 fd d7 24                                     	sb.xs \$r53\[\$r53\] = \$r53;;

    17ac:	f6 ff d9 a4 ff ff ff 9f ff ff ff 18             	sb 2305843009213693951 \(0x1fffffffffffffff\)\[\$r54\] = \$r54;;

    17b8:	b7 6d df a4 00 00 00 98 00 00 80 1f             	sb.odd \$r54\? -1125899906842624 \(0xfffc000000000000\)\[\$r55\] = \$r55;;

    17c4:	f8 7d e3 a4 00 00 80 1f                         	sb.even \$r55\? -8388608 \(0xff800000\)\[\$r56\] = \$r56;;

    17cc:	39 8e e7 24                                     	sb.wnez \$r56\? \[\$r57\] = \$r57;;

    17d0:	39 f0 e9 24                                     	sb -64 \(0xffffffc0\)\[\$r57\] = \$r58;;

    17d4:	3a 00 e9 a4 00 00 80 1f                         	sb -8589934592 \(0xfffffffe00000000\)\[\$r58\] = \$r58;;

    17dc:	3b 00 e4 0f                                     	scall \$r59;;

    17e0:	ff 01 e0 0f                                     	scall 511 \(0x1ff\);;

    17e4:	fb ee f3 27                                     	sd \$r59\[\$r59\] = \$r60;;

    17e8:	fc ff f1 a7 ff ff ff 9f ff ff ff 18             	sd 2305843009213693951 \(0x1fffffffffffffff\)\[\$r60\] = \$r60;;

    17f4:	7d 9f f7 a7 00 00 00 98 00 00 80 1f             	sd.weqz \$r61\? -1125899906842624 \(0xfffc000000000000\)\[\$r61\] = \$r61;;

    1800:	be af fb a7 00 00 80 1f                         	sd.wltz \$r62\? -8388608 \(0xff800000\)\[\$r62\] = \$r62;;

    1808:	ff bf ff 27                                     	sd.wgez \$r63\? \[\$r63\] = \$r63;;

    180c:	00 f0 01 27                                     	sd -64 \(0xffffffc0\)\[\$r0\] = \$r0;;

    1810:	00 00 05 a7 00 00 80 1f                         	sd -8589934592 \(0xfffffffe00000000\)\[\$r0\] = \$r1;;

    1818:	01 07 c0 0f                                     	set \$mmc = \$r1;;

    181c:	c1 00 c0 0f                                     	set \$ra = \$r1;;

    1820:	42 00 c0 0f                                     	set \$ps = \$r2;;

    1824:	42 00 c0 0f                                     	set \$ps = \$r2;;

    1828:	83 f0 0f 25                                     	sh.xs \$r2\[\$r3\] = \$r3;;

    182c:	c3 ff 11 a5 ff ff ff 9f ff ff ff 18             	sh 2305843009213693951 \(0x1fffffffffffffff\)\[\$r3\] = \$r4;;

    1838:	04 c1 17 a5 00 00 00 98 00 00 80 1f             	sh.wlez \$r4\? -1125899906842624 \(0xfffc000000000000\)\[\$r4\] = \$r5;;

    1844:	45 d1 1b a5 00 00 80 1f                         	sh.wgtz \$r5\? -8388608 \(0xff800000\)\[\$r5\] = \$r6;;

    184c:	86 01 1f 25                                     	sh.dnez \$r6\? \[\$r6\] = \$r7;;

    1850:	07 f0 1d 25                                     	sh -64 \(0xffffffc0\)\[\$r7\] = \$r7;;

    1854:	08 00 21 a5 00 00 80 1f                         	sh -8589934592 \(0xfffffffe00000000\)\[\$r8\] = \$r8;;

    185c:	00 00 a4 0f                                     	sleep;;

    1860:	49 62 22 79                                     	slld \$r8 = \$r9, \$r9;;

    1864:	ca 21 26 79                                     	slld \$r9 = \$r10, 7 \(0x7\);;

    1868:	ca 92 2a 79                                     	sllhqs \$r10 = \$r10, \$r11;;

    186c:	cb 51 2e 79                                     	sllhqs \$r11 = \$r11, 7 \(0x7\);;

    1870:	4c 83 32 79                                     	sllwps \$r12 = \$r12, \$r13;;

    1874:	ce 41 36 79                                     	sllwps \$r13 = \$r14, 7 \(0x7\);;

    1878:	cf 73 3a 79                                     	sllw \$r14 = \$r15, \$r15;;

    187c:	d0 31 42 79                                     	sllw \$r16 = \$r16, 7 \(0x7\);;

    1880:	51 64 42 7c                                     	slsd \$r16 = \$r17, \$r17;;

    1884:	d2 21 46 7c                                     	slsd \$r17 = \$r18, 7 \(0x7\);;

    1888:	d2 94 4a 7c                                     	slshqs \$r18 = \$r18, \$r19;;

    188c:	d3 51 4e 7c                                     	slshqs \$r19 = \$r19, 7 \(0x7\);;

    1890:	14 85 52 7c                                     	slswps \$r20 = \$r20, \$r20;;

    1894:	d5 41 56 7c                                     	slswps \$r21 = \$r21, 7 \(0x7\);;

    1898:	96 75 56 7c                                     	slsw \$r21 = \$r22, \$r22;;

    189c:	d7 31 5a 7c                                     	slsw \$r22 = \$r23, 7 \(0x7\);;

    18a0:	d7 e5 d7 28                                     	so \$r23\[\$r23\] = \$r52r53r54r55;;

    18a4:	d8 ff e5 a8 ff ff ff 9f ff ff ff 18             	so 2305843009213693951 \(0x1fffffffffffffff\)\[\$r24\] = \$r56r57r58r59;;

    18b0:	18 16 f7 a8 00 00 00 98 00 00 80 1f             	so.deqz \$r24\? -1125899906842624 \(0xfffc000000000000\)\[\$r24\] = \$r60r61r62r63;;

    18bc:	59 26 07 a8 00 00 80 1f                         	so.dltz \$r25\? -8388608 \(0xff800000\)\[\$r25\] = \$r0r1r2r3;;

    18c4:	5a 36 17 28                                     	so.dgez \$r25\? \[\$r26\] = \$r4r5r6r7;;

    18c8:	1a f0 25 28                                     	so -64 \(0xffffffc0\)\[\$r26\] = \$r8r9r10r11;;

    18cc:	1a 00 35 a8 00 00 80 1f                         	so -8589934592 \(0xfffffffe00000000\)\[\$r26\] = \$r12r13r14r15;;

    18d4:	db f6 f3 28                                     	sq.xs \$r27\[\$r27\] = \$r60r61;;

    18d8:	db ff f1 a8 ff ff ff 9f ff ff ff 18             	sq 2305843009213693951 \(0x1fffffffffffffff\)\[\$r27\] = \$r60r61;;

    18e4:	1c 47 fb a8 00 00 00 98 00 00 80 1f             	sq.dlez \$r28\? -1125899906842624 \(0xfffc000000000000\)\[\$r28\] = \$r62r63;;

    18f0:	1d 57 fb a8 00 00 80 1f                         	sq.dgtz \$r28\? -8388608 \(0xff800000\)\[\$r29\] = \$r62r63;;

    18f8:	5d 67 03 28                                     	sq.odd \$r29\? \[\$r29\] = \$r0r1;;

    18fc:	1e f0 01 28                                     	sq -64 \(0xffffffc0\)\[\$r30\] = \$r0r1;;

    1900:	1e 00 09 a8 00 00 80 1f                         	sq -8589934592 \(0xfffffffe00000000\)\[\$r30\] = \$r2r3;;

    1908:	df 67 7a 7a                                     	srad \$r30 = \$r31, \$r31;;

    190c:	e0 21 7e 7a                                     	srad \$r31 = \$r32, 7 \(0x7\);;

    1910:	60 98 82 7a                                     	srahqs \$r32 = \$r32, \$r33;;

    1914:	e1 51 86 7a                                     	srahqs \$r33 = \$r33, 7 \(0x7\);;

    1918:	a2 88 8a 7a                                     	srawps \$r34 = \$r34, \$r34;;

    191c:	e3 41 8e 7a                                     	srawps \$r35 = \$r35, 7 \(0x7\);;

    1920:	24 79 8e 7a                                     	sraw \$r35 = \$r36, \$r36;;

    1924:	e5 31 92 7a                                     	sraw \$r36 = \$r37, 7 \(0x7\);;

    1928:	a5 69 96 7b                                     	srld \$r37 = \$r37, \$r38;;

    192c:	e6 21 9a 7b                                     	srld \$r38 = \$r38, 7 \(0x7\);;

    1930:	e7 99 9e 7b                                     	srlhqs \$r39 = \$r39, \$r39;;

    1934:	e8 51 a2 7b                                     	srlhqs \$r40 = \$r40, 7 \(0x7\);;

    1938:	69 8a a2 7b                                     	srlwps \$r40 = \$r41, \$r41;;

    193c:	ea 41 a6 7b                                     	srlwps \$r41 = \$r42, 7 \(0x7\);;

    1940:	ea 7a aa 7b                                     	srlw \$r42 = \$r42, \$r43;;

    1944:	eb 31 ae 7b                                     	srlw \$r43 = \$r43, 7 \(0x7\);;

    1948:	2c 6b b2 78                                     	srsd \$r44 = \$r44, \$r44;;

    194c:	ed 21 b6 78                                     	srsd \$r45 = \$r45, 7 \(0x7\);;

    1950:	ae 9b b6 78                                     	srshqs \$r45 = \$r46, \$r46;;

    1954:	ef 51 ba 78                                     	srshqs \$r46 = \$r47, 7 \(0x7\);;

    1958:	2f 8c be 78                                     	srswps \$r47 = \$r47, \$r48;;

    195c:	f0 41 c2 78                                     	srswps \$r48 = \$r48, 7 \(0x7\);;

    1960:	71 7c c6 78                                     	srsw \$r49 = \$r49, \$r49;;

    1964:	f2 31 ca 78                                     	srsw \$r50 = \$r50, 7 \(0x7\);;

    1968:	00 00 a8 0f                                     	stop;;

    196c:	f3 0c c9 70                                     	stsud \$r50 = \$r51, \$r51;;

    1970:	34 1d cd 70                                     	stsuw \$r51 = \$r52, \$r52;;

    1974:	35 ed d7 26                                     	sw \$r52\[\$r53\] = \$r53;;

    1978:	f5 ff d9 a6 ff ff ff 9f ff ff ff 18             	sw 2305843009213693951 \(0x1fffffffffffffff\)\[\$r53\] = \$r54;;

    1984:	b6 7d df a6 00 00 00 98 00 00 80 1f             	sw.even \$r54\? -1125899906842624 \(0xfffc000000000000\)\[\$r54\] = \$r55;;

    1990:	f7 8d e3 a6 00 00 80 1f                         	sw.wnez \$r55\? -8388608 \(0xff800000\)\[\$r55\] = \$r56;;

    1998:	38 9e e7 26                                     	sw.weqz \$r56\? \[\$r56\] = \$r57;;

    199c:	39 f0 e5 26                                     	sw -64 \(0xffffffc0\)\[\$r57\] = \$r57;;

    19a0:	3a 00 e9 a6 00 00 80 1f                         	sw -8589934592 \(0xfffffffe00000000\)\[\$r58\] = \$r58;;

    19a8:	3b 70 eb 68                                     	sxbd \$r58 = \$r59;;

    19ac:	3b f0 ef 68                                     	sxhd \$r59 = \$r59;;

    19b0:	3c 50 f2 76                                     	sxlbhq \$r60 = \$r60;;

    19b4:	3d 40 f2 76                                     	sxlhwp \$r60 = \$r61;;

    19b8:	3d 50 f6 77                                     	sxmbhq \$r61 = \$r61;;

    19bc:	3e 40 fa 77                                     	sxmhwp \$r62 = \$r62;;

    19c0:	3f f0 fb 69                                     	sxwd \$r62 = \$r63;;

    19c4:	3f 00 b4 0f                                     	syncgroup \$r63;;

    19c8:	00 00 8c 0f                                     	tlbdinval;;

    19cc:	00 00 90 0f                                     	tlbiinval;;

    19d0:	00 00 84 0f                                     	tlbprobe;;

    19d4:	00 00 80 0f                                     	tlbread;;

    19d8:	00 00 88 0f                                     	tlbwrite;;

    19dc:	3f 00 b0 0f                                     	waitit \$r63;;

    19e0:	40 00 b8 0f                                     	wfxl \$ps, \$r0;;

    19e4:	80 00 b8 0f                                     	wfxl \$pcr, \$r0;;

    19e8:	40 00 b8 0f                                     	wfxl \$ps, \$r0;;

    19ec:	41 00 bc 0f                                     	wfxm \$ps, \$r1;;

    19f0:	81 00 bc 0f                                     	wfxm \$pcr, \$r1;;

    19f4:	81 00 bc 0f                                     	wfxm \$pcr, \$r1;;

    19f8:	80 4f 34 01                                     	xcopyo \$a13 = \$a4;;

    19fc:	80 5f 34 01                                     	xcopyo \$a13 = \$a5;;

    1a00:	82 f0 37 20                                     	xlo.u.xs \$a13 = \$r2\[\$r2\];;

    1a04:	83 a0 e3 a3 00 00 00 98 00 00 80 1f             	xlo.us.wltz.q0 \$r2\? \$a56a57a58a59 = -1125899906842624 \(0xfffc000000000000\)\[\$r3\];;

    1a10:	c3 b0 f7 a2 00 00 80 1f                         	xlo.u.wgez.q1 \$r3\? \$a60a61a62a63 = -8388608 \(0xff800000\)\[\$r3\];;

    1a18:	04 c1 0b 23                                     	xlo.us.wlez.q2 \$r4\? \$a0a1a2a3 = \[\$r4\];;

    1a1c:	05 d1 3b a0 00 00 00 98 00 00 80 1f             	xlo.u.wgtz \$r4\? \$a14 = -1125899906842624 \(0xfffc000000000000\)\[\$r5\];;

    1a28:	45 01 3b a1 00 00 80 1f                         	xlo.us.dnez \$r5\? \$a14 = -8388608 \(0xff800000\)\[\$r5\];;

    1a30:	86 11 3b 20                                     	xlo.u.deqz \$r6\? \$a14 = \[\$r6\];;

    1a34:	87 e1 1f 23                                     	xlo.us.q3 \$a4a5a6a7 = \$r6\[\$r7\];;

    1a38:	c7 ff 21 a2 ff ff ff 9f ff ff ff 18             	xlo.u.q0 \$a8a9a10a11 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r7\];;

    1a44:	07 f0 35 23                                     	xlo.us.q1 \$a12a13a14a15 = -64 \(0xffffffc0\)\[\$r7\];;

    1a48:	08 00 49 a2 00 00 80 1f                         	xlo.u.q2 \$a16a17a18a19 = -8589934592 \(0xfffffffe00000000\)\[\$r8\];;

    1a50:	c8 ff 3d a1 ff ff ff 9f ff ff ff 18             	xlo.us \$a15 = 2305843009213693951 \(0x1fffffffffffffff\)\[\$r8\];;

    1a5c:	08 f0 3d 20                                     	xlo.u \$a15 = -64 \(0xffffffc0\)\[\$r8\];;

    1a60:	09 00 3d a1 00 00 80 1f                         	xlo.us \$a15 = -8589934592 \(0xfffffffe00000000\)\[\$r9\];;

    1a68:	10 64 10 02                                     	xmma484bw \$a4a5 = \$a6a7, \$a16, \$a16;;

    1a6c:	11 84 1c 02                                     	xmma484subw \$a6a7 = \$a8a9, \$a16, \$a17;;

    1a70:	51 b4 20 02                                     	xmma484ubw \$a8a9 = \$a10a11, \$a17, \$a17;;

    1a74:	92 d4 2c 02                                     	xmma484usbw \$a10a11 = \$a12a13, \$a18, \$a18;;

    1a78:	80 7f 48 00                                     	xmovefo \$r16r17r18r19 = \$a6;;

    1a7c:	80 7f 5c 00                                     	xmovefo \$r20r21r22r23 = \$a7;;

    1a80:	49 e2 06 7f                                     	movetq \$a1_lo = \$r9, \$r9;;

    1a84:	8a f2 06 7f                                     	movetq \$a1_hi = \$r10, \$r10;;

    1a88:	00 80 5d 04                                     	xmt44d \$a20a21a22a23 = \$a24a25a26a27;;

    1a8c:	cb ff 28 ec ff ff ff 87 ff ff ff 00             	xord \$r10 = \$r11, 2305843009213693951 \(0x1fffffffffffffff\);;

    1a98:	0b 03 2d 7c                                     	xord \$r11 = \$r11, \$r12;;

    1a9c:	0d f0 30 6c                                     	xord \$r12 = \$r13, -64 \(0xffffffc0\);;

    1aa0:	0e 00 34 ec 00 00 80 07                         	xord \$r13 = \$r14, -8589934592 \(0xfffffffe00000000\);;

    1aa8:	cf 0f 39 fc ff ff ff 00                         	xord.@ \$r14 = \$r15, 536870911 \(0x1fffffff\);;

    1ab0:	10 14 3d 7c                                     	xorw \$r15 = \$r16, \$r16;;

    1ab4:	11 f0 40 7c                                     	xorw \$r16 = \$r17, -64 \(0xffffffc0\);;

    1ab8:	11 00 44 fc 00 00 80 07                         	xorw \$r17 = \$r17, -8589934592 \(0xfffffffe00000000\);;

    1ac0:	92 f4 4b 29                                     	xso.xs \$r18\[\$r18\] = \$a18;;

    1ac4:	d2 ff 4d a9 ff ff ff 9f ff ff ff 18             	xso 2305843009213693951 \(0x1fffffffffffffff\)\[\$r18\] = \$a19;;

    1ad0:	d3 24 4f a9 00 00 00 98 00 00 80 1f             	xso.dltz \$r19\? -1125899906842624 \(0xfffc000000000000\)\[\$r19\] = \$a19;;

    1adc:	d4 34 4f a9 00 00 80 1f                         	xso.dgez \$r19\? -8388608 \(0xff800000\)\[\$r20\] = \$a19;;

    1ae4:	14 45 53 29                                     	xso.dlez \$r20\? \[\$r20\] = \$a20;;

    1ae8:	15 f0 51 29                                     	xso -64 \(0xffffffc0\)\[\$r21\] = \$a20;;

    1aec:	15 00 51 a9 00 00 80 1f                         	xso -8589934592 \(0xfffffffe00000000\)\[\$r21\] = \$a20;;

    1af4:	d6 3f 54 78                                     	zxbd \$r21 = \$r22;;

    1af8:	16 f0 5b 64                                     	zxhd \$r22 = \$r22;;

    1afc:	d7 ff 5c 78                                     	zxwd \$r23 = \$r23;;

