target:
	cv.mac t0, t1, t2
	cv.mac t4, t2, t0
	cv.mac t3, t5, t1
	cv.machhsn t0, t1, t2, 0
	cv.machhsn t4, t2, t0, 11
	cv.machhsn t3, t5, t1, 31
	cv.machhsrn t0, t1, t2, 0
	cv.machhsrn t4, t2, t0, 24
	cv.machhsrn t3, t5, t1, 31
	cv.machhun t0, t1, t2, 0
	cv.machhun t4, t2, t0, 18
	cv.machhun t3, t5, t1, 31
	cv.machhurn t0, t1, t2, 0
	cv.machhurn t4, t2, t0, 5
	cv.machhurn t3, t5, t1, 31
	cv.macsn t0, t1, t2, 0
	cv.macsn t4, t2, t0, 24
	cv.macsn t3, t5, t1, 31
	cv.macsrn t0, t1, t2, 0
	cv.macsrn t4, t2, t0, 9
	cv.macsrn t3, t5, t1, 31
	cv.macun t0, t1, t2, 0
	cv.macun t4, t2, t0, 27
	cv.macun t3, t5, t1, 31
	cv.macurn t0, t1, t2, 0
	cv.macurn t4, t2, t0, 25
	cv.macurn t3, t5, t1, 31
	cv.msu t0, t1, t2
	cv.msu t4, t2, t0
	cv.msu t3, t5, t1
	cv.mulhhs t0, t1, t2
	cv.mulhhs t4, t2, t0
	cv.mulhhs t3, t5, t1
	cv.mulhhsn t0, t1, t2, 0
	cv.mulhhsn t4, t2, t0, 16
	cv.mulhhsn t3, t5, t1, 31
	cv.mulhhsrn t0, t1, t2, 0
	cv.mulhhsrn t4, t2, t0, 17
	cv.mulhhsrn t3, t5, t1, 31
	cv.mulhhu t0, t1, t2
	cv.mulhhu t4, t2, t0
	cv.mulhhu t3, t5, t1
	cv.mulhhun t0, t1, t2, 0
	cv.mulhhun t4, t2, t0, 16
	cv.mulhhun t3, t5, t1, 31
	cv.mulhhurn t0, t1, t2, 0
	cv.mulhhurn t4, t2, t0, 9
	cv.mulhhurn t3, t5, t1, 31
	cv.muls t0, t1, t2
	cv.muls t4, t2, t0
	cv.muls t3, t5, t1
	cv.mulsn t0, t1, t2, 0
	cv.mulsn t4, t2, t0, 4
	cv.mulsn t3, t5, t1, 31
	cv.mulsrn t0, t1, t2, 0
	cv.mulsrn t4, t2, t0, 10
	cv.mulsrn t3, t5, t1, 31
	cv.mulu t0, t1, t2
	cv.mulu t4, t2, t0
	cv.mulu t3, t5, t1
	cv.mulun t0, t1, t2, 0
	cv.mulun t4, t2, t0, 7
	cv.mulun t3, t5, t1, 31
	cv.mulurn t0, t1, t2, 0
	cv.mulurn t4, t2, t0, 11
	cv.mulurn t3, t5, t1, 31

  # Pseudo-instructions
	cv.mulhhsn t0, t1, t2, 0
	cv.mulhhsn t4, t2, t0, 0
	cv.mulhhsn t3, t5, t1, 0
	cv.mulhhun t0, t1, t2, 0
	cv.mulhhun t4, t2, t0, 0
	cv.mulhhun t3, t5, t1, 0
	cv.mulsn t0, t1, t2, 0
	cv.mulsn t4, t2, t0, 0
	cv.mulsn t3, t5, t1, 0
	cv.mulun t0, t1, t2, 0
	cv.mulun t4, t2, t0, 0
	cv.mulun t3, t5, t1, 0
