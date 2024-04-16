# Copyright 2022-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

# Single step through a simple (empty) function that was compiled
# without DWARF debug information.
#
# At each instruction check that the frame-id, and frame base address,
# are calculated correctly.
#
# Additionally, check we can correctly unwind to the previous frame,
# and that the previous stack-pointer value, and frame base address
# value, can be calculated correctly.

if {[prepare_for_testing_full "failed to prepare" \
	 [list ${testfile} $ldflags \
	      $srcfile $srcfile_flags $srcfile2 $srcfile2_flags]]} {
    return -1
}

if {![runto_main]} {
    return 0
}

# Return a two element list, the first element is the stack-pointer
# value (from the $sp register), and the second element is the frame
# base address (from the 'info frame' output).
proc get_sp_and_fba { testname } {
    with_test_prefix "get \$sp and frame base $testname" {
	set sp [get_hexadecimal_valueof "\$sp" "*UNKNOWN*"]

	set fba ""
	gdb_test_multiple "info frame" "" {
	    -re -wrap ".*Stack level ${::decimal}, frame at ($::hex):.*" {
		set fba $expect_out(1,string)
	    }
	}

	return [list $sp $fba]
    }
}

# Return the frame-id of the current frame, collected using the 'maint
# print frame-id' command.
proc get_fid { } {
    set fid ""
    gdb_test_multiple "maint print frame-id" "" {
	-re -wrap ".*frame-id for frame #${::decimal}: (.*)" {
	    set fid $expect_out(1,string)
	}
    }
    return $fid
}

# Record the current stack-pointer, and the frame base address.
lassign [get_sp_and_fba "in main"] main_sp main_fba
set main_fid [get_fid]

proc do_test { function step_cmd } {
    # Now enter the function.  Ideally, stop at the first insn, so set a
    # breakpoint at "*$function".  The "*$function" breakpoint may not trigger
    # for archs with gdbarch_skip_entrypoint_p, so set a backup breakpoint at
    # "$function".
    gdb_breakpoint "*$function"
    gdb_breakpoint "$function"
    gdb_continue_to_breakpoint "enter $function"
    # Cleanup breakpoints.
    delete_breakpoints

    # Record the current stack-pointer, and the frame base address.
    lassign [get_sp_and_fba "in $function"] fn_sp fn_fba
    set fn_fid [get_fid]

    for { set i_count 1 } { true } { incr i_count } {
	with_test_prefix "instruction ${i_count}" {

	    # The current stack-pointer value can legitimately change
	    # throughout the lifetime of a function, so we don't check the
	    # current stack-pointer value.  But the frame base address
	    # should not change, so we do check for that.
	    lassign [get_sp_and_fba "for fn"] sp_value fba_value
	    gdb_assert { $fba_value == $fn_fba }

	    # The frame-id should never change within a function, so check
	    # that now.
	    set fid [get_fid]
	    gdb_assert { [string equal $fid $fn_fid] } \
		"check frame-id matches"

	    # Check that the previous frame is 'main'.
	    gdb_test "bt 2" "\r\n#1\\s+\[^\r\n\]+ in main \\(\\)( .*)?"

	    # Move up the stack (to main).
	    gdb_test "up" \
		"\r\n#1\\s+\[^\r\n\]+ in main \\(\\)( .*)?"

	    # Check we can unwind the stack-pointer and the frame base
	    # address correctly.
	    lassign [get_sp_and_fba "for main"] sp_value fba_value
	    if { $i_count == 1 } {
		# The stack-pointer may have changed while running to *$function.
		set ::main_sp $sp_value
	    } else {
		gdb_assert { $sp_value == $::main_sp }
	    }
	    gdb_assert { $fba_value == $::main_fba }

	    # Check we have a consistent value for main's frame-id.
	    set fid [get_fid]
	    gdb_assert { [string equal $fid $::main_fid] }

	    # Move back to the inner most frame.
	    gdb_test "frame 0" ".*"

	    if { $i_count > 100 } {
		# We expect a handful of instructions, if we reach 100,
		# something is going wrong.  Avoid an infinite loop.
		fail "exceeded max number of instructions"
		break
	    }

	    gdb_test $step_cmd

	    set in_fn 0
	    gdb_test_multiple "info frame" "" {
		-re -wrap " = $::hex in ${function}( \\(.*\\))?;.*" {
		    set in_fn 1
		}
		-re -wrap "" {}
	    }

	    if { ! $in_fn } {
		break
	    }
	}
    }
}

foreach {
    function step_cmd
} {
    foo stepi
    bar nexti
} {
    with_test_prefix $function {
	do_test $function $step_cmd
    }
}
