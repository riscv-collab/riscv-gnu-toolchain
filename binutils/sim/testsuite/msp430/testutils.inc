# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text

	# Skip over these inlined funcs.
	jmp __start;

	.global __pass
	.type __pass, function
__pass:
	# Note - we cannot just invoke:
	#
	#  write 1, _passmsg, 5
	#
	# here because _passmsg contains the run-time (VMA) address of
	# the pass string (probably 0x500) not the load-time (LMA)
	# address (probably 0x804c).  Normally using the VMA address
	# would be the correct thing to do - *if* there was some start
	# up code which copied data from LMA to VMA.  But we have no
	# start up code, so the data still resides at the LMA
	# address.  Hence we use __romdatastart instead.
	#
	# Note - we are cheating because the address that we pass to
	# "write" should actually be:
	#
	#    __romdatastart + (_passmsg - __datastart)
	#
	# but the assembler cannot cope with this expression.  So we
	# cheat and use the fact that we know that _passmsg is the
	# first string in the .data section and so (_passmsg -
	# __datastart) evaluates to zero.

	write 1, __romdatastart, 5
	exit 0

	.global __fail
	.type __fail, function
__fail:
	# Note - see above.
	#
	#   write 1, _failmsg, 5
	#
	# This time we use the fact that _passmsg is aligned to a
	# 16 byte boundary to work out that (_failmsg - __datastart)
	# evaluates to 0x10.

	write 1, __romdatastart + 0x10, 5
	exit 1

	.data
_passmsg:
	.ascii "pass\n"
	.align 4

_failmsg:
	.ascii "fail\n"
	.align 4

	.text
	.global __start
	.type __start, function
__start:
	.endm

# MACRO: system_call
# Make a libgloss/Linux system call
	.macro system_call nr:req
	call #(0x180|\nr);
	.endm

# MACRO: exit
# Quit the current test
	.macro exit rc:req
	mov #\rc, r12
	system_call 1
	.endm

# MACRO: pass
# Write 'pass' to stdout via syscalls and quit;
# meant for non-OS operating environments
	.macro pass
	jmp __pass;
	.endm

# MACRO: fail
# Write 'fail' to stdout via syscalls and quit;
# meant for non-OS operating environments
	.macro fail
	jmp __fail;
	.endm

# MACRO: write
# Just like the write() C function; uses system calls
	.macro write fd:req, buf:req, count:req
	mov #\fd, r12;
	mov #\buf, r13;
	mov #\count, r14;
	system_call 5
	.endm
