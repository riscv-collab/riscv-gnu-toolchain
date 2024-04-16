# Five bit immediate must be an absolute value in range [0, 31]
target:
	cv.clip t0,t3,-1
	cv.clipu t0,t3,-1
	cv.clip t0,t3,-400
	cv.clipu t0,t3,-985
	cv.clip t0,t3,32
	cv.clipu t0,t3,32
	cv.clip t0,t3,859
	cv.clipu t0,t3,7283
