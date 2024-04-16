# Source 2 must be of type register
target:
	cv.slet t4,t2,3
	cv.sletu t4,t2,4
	cv.min t4,t2,13
	cv.minu t4,t2,7
	cv.max t4,t2,17
	cv.maxu t4,t2,30
	cv.clipr t4,t2,18
	cv.clipur t4,t2,29
	cv.addn t4,t2,24,4
	cv.addun t4,t2,6,4
	cv.addrn t6,t0,7,9
	cv.addurn t6,t0,18,14
	cv.addnr t6,t0,15
	cv.addunr t6,t0,24
	cv.addrnr t6,t0,3
	cv.addurnr t6,t0,2
	cv.subn t6,t0,1,6
	cv.subun t6,t0,8,24
	cv.subrn t6,t0,18,21
	cv.suburn t6,t0,25,3
	cv.subnr t6,t0,14
	cv.subunr t6,t0,7
	cv.subrnr t6,t0,18
	cv.suburnr t6,t0,26
