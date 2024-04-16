# Check Illegal MSRLIST instructions

	.text
_start:
	rdmsrlist		 #MSRLIST
	wrmsrlist		 #MSRLIST
