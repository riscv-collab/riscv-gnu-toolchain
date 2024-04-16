/* File to test the +ite option.  */
func:
	trcit x1

	mrs  x1, trcitecr_el1
	mrs  x3, trcitecr_el12
	mrs  x5, trcitecr_el2
	mrs  x7, trciteedcr
	msr  trcitecr_el1, x1
	msr  trcitecr_el12, x3
	msr  trcitecr_el2, x5
	msr  trciteedcr, x7
