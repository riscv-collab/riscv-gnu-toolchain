# Destination must be of type register
target:
	cv.mac 8, t2, t0
	cv.msu 23, t2, t0
	cv.muls 43, t2, t0
	cv.mulhhs 7, t2, t0
	cv.mulsn 345, t2, t0, 4
	cv.mulhhsn 356, t2, t0, 16
	cv.mulsrn 867, t2, t0, 10
	cv.mulhhsrn 3454, t2, t0, 17
	cv.mulu 9, t2, t0
	cv.mulhhu 54, t2, t0
	cv.mulun 965, t2, t0, 7
	cv.mulhhun 35, t2, t0, 16
	cv.mulurn 87, t2, t0, 11
	cv.mulhhurn 38, t2, t0, 9
	cv.macsn 985, t2, t0, 24
	cv.machhsn 83, t2, t0, 11
	cv.macsrn 960, t2, t0, 9
	cv.machhsrn 385, t2, t0, 24
	cv.macun 58, t2, t0, 27
	cv.machhun 6, t2, t0, 18
	cv.macurn 35, t2, t0, 25
	cv.machhurn 67, t2, t0, 5

# Source one must be of type register
	cv.mac t4, 43, t0
	cv.msu t4, 3, t0
	cv.muls t4, 345, t0
	cv.mulhhs t4, 54, t0
	cv.mulsn t4, 4, t0, 4
	cv.mulhhsn t4, 35, t0, 16
	cv.mulsrn t4, 53, t0, 10
	cv.mulhhsrn t4, 4456, t0, 17
	cv.mulu t4, 868, t0
	cv.mulhhu t4, 95, t0
	cv.mulun t4, 584, t0, 7
	cv.mulhhun t4, 37545, t0, 16
	cv.mulurn t4, 943, t0, 11
	cv.mulhhurn t4, 34, t0, 9
	cv.macsn t4, 93, t0, 24
	cv.machhsn t4, 584, t0, 11
	cv.macsrn t4, 28, t0, 9
	cv.machhsrn t4, 9, t0, 24
	cv.macun t4, 834, t0, 27
	cv.machhun t4, 92, t0, 18
	cv.macurn t4, 49, t0, 25
	cv.machhurn t4, 6, t0, 5

# Source two must be of type register
	cv.mac t4, t2, 344
	cv.msu t4, t2, 23
	cv.muls t4, t2, 2
	cv.mulhhs t4, t2, 8
	cv.mulsn t4, t2, 45, 4
	cv.mulhhsn t4, t2, 655, 16
	cv.mulsrn t4, t2, 465, 10
	cv.mulhhsrn t4, t2, 3534, 17
	cv.mulu t4, t2, 46
	cv.mulhhu t4, t2, 35
	cv.mulun t4, t2, 67, 7
	cv.mulhhun t4, t2, 6, 16
	cv.mulurn t4, t2, 787, 11
	cv.mulhhurn t4, t2, 3545, 9
	cv.macsn t4, t2, 6, 24
	cv.machhsn t4, t2, 765, 11
	cv.macsrn t4, t2, 45, 9
	cv.machhsrn t4, t2, 7, 24
	cv.macun t4, t2, 98, 27
	cv.machhun t4, t2, 654, 18
	cv.macurn t4, t2, 900, 25
	cv.machhurn t4, t2, 354, 5

# Immediate value must be in range [0, 31]
	cv.mulsn t4, t2, t0, -1
	cv.mulhhsn t4, t2, t0, -1
	cv.mulsrn t4, t2, t0, -1
	cv.mulhhsrn t4, t2, t0, -1
	cv.mulun t4, t2, t0, -1
	cv.mulhhun t4, t2, t0, -1
	cv.mulurn t4, t2, t0, -1
	cv.mulhhurn t4, t2, t0, -1
	cv.macsn t4, t2, t0, -1
	cv.machhsn t4, t2, t0, -1
	cv.macsrn t4, t2, t0, -1
	cv.machhsrn t4, t2, t0, -1
	cv.macun t4, t2, t0, -1
	cv.machhun t4, t2, t0, -1
	cv.macurn t4, t2, t0, -1
	cv.machhurn t4, t2, t0, -1
	cv.mulsn t4, t2, t0, -43
	cv.mulhhsn t4, t2, t0, -531
	cv.mulsrn t4, t2, t0, -4454
	cv.mulhhsrn t4, t2, t0, -32
	cv.mulun t4, t2, t0, -23
	cv.mulhhun t4, t2, t0, -459
	cv.mulurn t4, t2, t0, -549
	cv.mulhhurn t4, t2, t0, -32
	cv.macsn t4, t2, t0, -223
	cv.machhsn t4, t2, t0, -56
	cv.macsrn t4, t2, t0, -8
	cv.machhsrn t4, t2, t0, -2
	cv.macun t4, t2, t0, -432
	cv.machhun t4, t2, t0, -1245
	cv.macurn t4, t2, t0, -45
	cv.machhurn t4, t2, t0, -354
	cv.mulsn t4, t2, t0, 32
	cv.mulhhsn t4, t2, t0, 32
	cv.mulsrn t4, t2, t0, 32
	cv.mulhhsrn t4, t2, t0, 32
	cv.mulun t4, t2, t0, 32
	cv.mulhhun t4, t2, t0, 32
	cv.mulurn t4, t2, t0, 32
	cv.mulhhurn t4, t2, t0, 32
	cv.macsn t4, t2, t0, 32
	cv.machhsn t4, t2, t0, 32
	cv.macsrn t4, t2, t0, 32
	cv.machhsrn t4, t2, t0, 32
	cv.macun t4, t2, t0, 32
	cv.machhun t4, t2, t0, 32
	cv.macurn t4, t2, t0, 32
	cv.machhurn t4, t2, t0, 32
	cv.mulsn t4, t2, t0, 325
	cv.mulhhsn t4, t2, t0, 531
	cv.mulsrn t4, t2, t0, 4454
	cv.mulhhsrn t4, t2, t0, 254
	cv.mulun t4, t2, t0, 76
	cv.mulhhun t4, t2, t0, 459
	cv.mulurn t4, t2, t0, 549
	cv.mulhhurn t4, t2, t0, 5364
	cv.macsn t4, t2, t0, 34435
	cv.machhsn t4, t2, t0, 56
	cv.macsrn t4, t2, t0, 3423
	cv.machhsrn t4, t2, t0, 365
	cv.macun t4, t2, t0, 432
	cv.machhun t4, t2, t0, 1245
	cv.macurn t4, t2, t0, 45

# Immediate value must be an absolute expression
	cv.mulsn t4, t2, t0, t3
	cv.mulhhsn t4, t2, t0, t1
	cv.mulsrn t4, t2, t0, t6
	cv.mulhhsrn t4, t2, t0, t3
	cv.mulun t4, t2, t0, t1
	cv.mulhhun t4, t2, t0, t3
	cv.mulurn t4, t2, t0, t5
	cv.mulhhurn t4, t2, t0, t1
	cv.macsn t4, t2, t0, t3
	cv.machhsn t4, t2, t0, t5
	cv.macsrn t4, t2, t0, t1
	cv.machhsrn t4, t2, t0, t6
	cv.macun t4, t2, t0, t1
	cv.machhun t4, t2, t0, t3
	cv.macurn t4, t2, t0, t6
	cv.machhurn t4, t2, t0, t5
	cv.machhurn t4, t2, t0, 354
