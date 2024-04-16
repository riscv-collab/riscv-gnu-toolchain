# Five bit immediate must be an absolute value
target:
	cv.addn t4,t2,t0,t3
	cv.addun t4,t2,t0,t3
	cv.addrn t6,t0,t3,t2
	cv.addurn t6,t0,t3,t2
	cv.subn t6,t0,t3,t2
	cv.subun t6,t0,t3,t2
	cv.subrn t6,t0,t3,t2
	cv.suburn t6,t0,t3,t2
