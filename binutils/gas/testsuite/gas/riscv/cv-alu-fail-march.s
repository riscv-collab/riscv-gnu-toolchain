# Absence of xcv or xcvalu march option disables all CORE-V general ALU ops extensions
target:
	cv.abs t4,t2
	cv.slet t4,t2,t6
	cv.sletu t4,t2,t6
	cv.min t4,t2,t6
	cv.minu t4,t2,t6
	cv.max t4,t2,t6
	cv.maxu t4,t2,t6
	cv.exths t4,t2
	cv.exthz t4,t2
	cv.extbs t4,t2
	cv.extbz t4,t2
	cv.clip t4,t2,5
	cv.clipu t4,t2,5
	cv.clipr t4,t2,t6
	cv.clipur t4,t2,t6
	cv.addn t4, t2, t0, 4
	cv.addun t4, t2, t0, 4
	cv.addrn t6, t0, t3, 9
	cv.addurn t6, t0, t3, 14
	cv.addnr t6, t0, t3
	cv.addunr t6, t0, t3
	cv.addrnr t6, t0, t3
	cv.addurnr t6, t0, t3
	cv.subn t6, t0, t3, 6
	cv.subun t6, t0, t3, 24
	cv.subrn t6, t0, t3, 21
	cv.suburn t6, t0, t3, 3
	cv.subnr t6, t0, t3
	cv.subunr t6, t0, t3
	cv.subrnr t6, t0, t3
	cv.suburnr t6, t0, t3
