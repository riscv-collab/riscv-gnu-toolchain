.*: Assembler messages:
.*: Error: 128-bit-wide accsess not allowed on selected system register 'accdata_el1'
.*: Error: 128-bit-wide accsess not allowed on selected system register 'accdata_el1'
.*: Error: operand mismatch -- `mrrs w0,w1,ttbr0_el1'
.*: Info:    did you mean this\?
.*: Info:    	mrrs x0, x1, ttbr0_el1
.*: Error: operand mismatch -- `msrr ttbr0_el1,w0,w1'
.*: Info:    did you mean this\?
.*: Info:    	msrr ttbr0_el1, x0, x1
.*: Error: reg pair must be contiguous at operand 2 -- `mrrs x0,x2,ttbr0_el1'
.*: Error: reg pair must be contiguous at operand 3 -- `msrr ttbr0_el1,x0,x2'
