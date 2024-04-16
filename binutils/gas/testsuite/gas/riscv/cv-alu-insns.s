target:
	cv.abs t0,t1
	cv.abs t4,t2
	cv.abs t3,t5

	cv.addnr t0, t3, t6
	cv.addnr t6, t0, t3
	cv.addnr t3, t6, t0

	cv.addn t0, t1, t2, 0
	cv.addn t4, t2, t0, 4
	cv.addn t3, t5, t1, 31

	cv.addrnr t0, t3, t6
	cv.addrnr t6, t0, t3
	cv.addrnr t3, t6, t0

	cv.addrn t0, t3, t6, 0
	cv.addrn t6, t0, t3, 9
	cv.addrn t3, t6, t0, 31

	cv.addunr t0, t3, t6
	cv.addunr t6, t0, t3
	cv.addunr t3, t6, t0

	cv.addun t0, t1, t2, 0
	cv.addun t4, t2, t0, 4
	cv.addun t3, t5, t1, 31

	cv.addurnr t0, t3, t6
	cv.addurnr t6, t0, t3
	cv.addurnr t3, t6, t0

	cv.addurn t0, t3, t6, 0
	cv.addurn t6, t0, t3, 14
	cv.addurn t3, t6, t0, 31

	cv.clipr t0,t1,t2
	cv.clipr t4,t2,t6
	cv.clipr t3,t5,t1

	cv.clip t0,t1,0
	cv.clip t4,t2,5
	cv.clip t3,t5,31

	cv.clipur t0,t1,t2
	cv.clipur t4,t2,t6
	cv.clipur t3,t5,t1

	cv.clipu t0,t1,0
	cv.clipu t4,t2,5
	cv.clipu t3,t5,31

	cv.extbs t0,t1
	cv.extbs t4,t2
	cv.extbs t3,t5

	cv.extbz t0,t1
	cv.extbz t4,t2
	cv.extbz t3,t5

	cv.exths t0,t1
	cv.exths t4,t2
	cv.exths t3,t5

	cv.exthz t0,t1
	cv.exthz t4,t2
	cv.exthz t3,t5

	cv.max t0,t1,t2
	cv.max t4,t2,t6
	cv.max t3,t5,t1

	cv.maxu t0,t1,t2
	cv.maxu t4,t2,t6
	cv.maxu t3,t5,t1

	cv.min t0,t1,t2
	cv.min t4,t2,t6
	cv.min t3,t5,t1

	cv.minu t0,t1,t2
	cv.minu t4,t2,t6
	cv.minu t3,t5,t1

	cv.sle t0,t1,t2
	cv.sle t4,t2,t6
	cv.sle t3,t5,t1

	cv.sleu t0,t1,t2
	cv.sleu t4,t2,t6
	cv.sleu t3,t5,t1

	cv.subnr t0, t3, t6
	cv.subnr t6, t0, t3
	cv.subnr t3, t6, t0

	cv.subn t0, t3, t6, 0
	cv.subn t6, t0, t3, 6
	cv.subn t3, t6, t0, 31

	cv.subrnr t0, t3, t6
	cv.subrnr t6, t0, t3
	cv.subrnr t3, t6, t0

	cv.subrn t0, t3, t6, 0
	cv.subrn t6, t0, t3, 21
	cv.subrn t3, t6, t0, 31

	cv.subunr t0, t3, t6
	cv.subunr t6, t0, t3
	cv.subunr t3, t6, t0

	cv.subun t0, t3, t6, 0
	cv.subun t6, t0, t3, 24
	cv.subun t3, t6, t0, 31

	cv.suburnr t0, t3, t6
	cv.suburnr t6, t0, t3
	cv.suburnr t3, t6, t0

	cv.suburn t0, t3, t6, 0
	cv.suburn t6, t0, t3, 3
	cv.suburn t3, t6, t0, 31
