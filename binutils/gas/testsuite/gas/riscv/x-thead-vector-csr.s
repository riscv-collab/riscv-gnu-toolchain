	csrr a0, th.vstart
	csrr a0, th.vxsat
	csrr a0, th.vxrm
	csrr a0, th.vl
	csrr a0, th.vtype
	csrr a0, th.vlenb

	csrw th.vstart, a0
	csrw th.vxsat, a0
	csrw th.vxrm, a0
	csrw th.vl, a0		# read-only CSR
	csrw th.vtype, a0	# read-only CSR
	csrw th.vlenb, a0	# read-only CSR
