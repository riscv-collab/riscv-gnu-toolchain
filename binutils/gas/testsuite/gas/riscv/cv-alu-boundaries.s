# Destination must be of type register
target:
	cv.subnr 10, t3, t6
# Source 1 must be of type register
	cv.addrnr t4, 26, t6
# Source 2 must be of type register
	cv.subunr t6, t3, 15
# Five bit immediate must be an absolute value
	cv.clipu t0, t3, t6
# Five bit immediate must be an absolute value
	cv.addn t0, t3, t6, t2
# Five bit immediate must be an absolute value in range [0, 31]
	cv.clipu t0, t3, -10
# Five bit immediate must be an absolute value in range [0, 31]
	cv.clipu t0, t3, 500
# Five bit immediate must be an absolute value in range [0, 31]
	cv.addn t0, t3, t6, -60
# Five bit immediate must be an absolute value in range [0, 31]
	cv.addn t0, t3, t6, 302
# Five bit immediate must be an absolute value in range [0, 31]
	cv.clipu t0, t3, -1
# Five bit immediate must be an absolute value in range [0, 31]
	cv.clipu t0, t3, 32
# Five bit immediate must be an absolute value in range [0, 31]
	cv.addn t0, t3, t6, -1
# Five bit immediate must be an absolute value in range [0, 31]
	cv.addn t0, t3, t6, 32
