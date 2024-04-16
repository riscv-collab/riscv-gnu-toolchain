# Copyright (C) 2023-2024 Free Software Foundation, Inc.

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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Exercise restoring SME/TPIDR2 state from a signal frame.

load_lib aarch64-scalable.exp

#
# Validate the state of registers in the signal frame for various states.
#
proc test_sme_registers_sigframe { id_start id_end } {

    set compile_flags {"debug" "macros"}
    lappend compile_flags "additional_flags=-march=armv8.5-a+sve"
    lappend compile_flags "additional_flags=-DID_START=${id_start}"
    lappend compile_flags "additional_flags=-DID_END=${id_end}"

    standard_testfile ${::srcdir}/${::subdir}/aarch64-sme-regs-sigframe.c
    set executable "${::testfile}-${id_start}-${id_end}"
    if {[prepare_for_testing "failed to prepare" ${executable} ${::srcfile} ${compile_flags}]} {
	return -1
    }
    set binfile [standard_output_file ${executable}]

    if ![runto_main] {
	untested "could not run to main"
	return -1
    }

    # Check if we are talking to a remote target.  If so, bail out, as right now
    # remote targets can't communicate vector length (vl or svl) changes to gdb
    # via the RSP.  When this restriction is lifted, we can remove this guard.
    if {[gdb_is_target_remote]} {
	unsupported "aarch64 sve/sme tests not supported for remote targets"
	return -1
    }

    set sigill_breakpoint "stop before SIGILL"
    set handler_breakpoint "handler"
    gdb_breakpoint [gdb_get_line_number $sigill_breakpoint]
    gdb_breakpoint [gdb_get_line_number $handler_breakpoint]

    for {set id $id_start} {$id <= $id_end} {incr id} {
	set state [test_id_to_state $id]
	set vl [test_id_to_vl $id]
	set svl [test_id_to_svl $id]

	set skip_unsupported 0
	if {![aarch64_supports_sve_vl $vl]
	    || ![aarch64_supports_sme_svl $svl]} {
	    # We have a vector length or streaming vector length that
	    # is not supported by this target.  Skip to the next iteration
	    # since it is no use running tests for an unsupported vector
	    # length.
	    if {![aarch64_supports_sve_vl $vl]} {
		verbose -log "SVE vector length $vl not supported."
	    } elseif {![aarch64_supports_sme_svl $svl]} {
		verbose -log "SME streaming vector length $svl not supported."
	    }
	    verbose -log "Skipping test."
	    set skip_unsupported 1
	}

	with_test_prefix "state=${state} vl=${vl} svl=${svl}" {

	    # If the SVE or SME vector length is not supported, just skip
	    # these next tests.
	    if {$skip_unsupported} {
		untested "unsupported configuration on target"
		continue
	    }

	    # Run the program until it has adjusted the svl.
	    if [gdb_continue_to_breakpoint $sigill_breakpoint] {
		return -1
	    }

	    # Check SVG to make sure it is correct
	    set expected_svg [expr $svl / 8]
	    gdb_test "print \$svg" "= ${expected_svg}"

	    # Check the size of ZA.
	    set expected_za_size [expr $svl * $svl]
	    gdb_test "print sizeof \$za" " = $expected_za_size"

	    # Check the value of SVCR.
	    gdb_test "print \$svcr" [get_svcr_value $state] "svcr before signal"

	    # Handle SME ZA and SME2 initialization and state.
	    set byte 0
	    set sme2_byte 0
	    if { $state == "za" || $state == "za_ssve" } {
		set byte 170
		set sme2_byte 255
	    }

	    # Set the expected ZA pattern.
	    set za_pattern [string_to_regexp [2d_array_value_pattern $byte $svl $svl]]

	    # Handle SVE/SSVE initialization and state.
	    set sve_vl $svl
	    if { $state == "ssve" || $state == "za_ssve" } {
		# SVE state comes from SSVE.
		set sve_vl $svl
	    } else {
		# SVE state comes from regular SVE.
		set sve_vl $vl
	    }

	    # Initialize the SVE state.
	    set sve_pattern [string_to_regexp [sve_value_pattern $state $sve_vl 85 255]]
	    for {set row 0} {$row < 32} {incr row} {
		set register_name "\$z${row}\.b\.u"
		gdb_test "print sizeof $register_name" " = $sve_vl" "size of $register_name"
		gdb_test "print $register_name" $sve_pattern "read back from $register_name"
	    }

	    # Print ZA to check its value.
	    gdb_test "print \$za" $za_pattern "read back from za"

	    # Test TPIDR2 restore from signal frame as well.
	    gdb_test_no_output "set \$tpidr2=0x0102030405060708"

	    # Run to the illegal instruction.
	    if [gdb_test "continue" "Continuing\.\r\n\r\nProgram received signal SIGILL, Illegal instruction\..*in main.*"] {
		return
	    }

	    # Skip the illegal instruction.  The signal handler will be called after we continue.
	    gdb_test_no_output "set \$pc=\$pc+4"
	    # Continue to the signal handler.
	    if [gdb_continue_to_breakpoint $handler_breakpoint] {
		return -1
	    }

	    # Modify TPIDR2 so it is different from its value past the signal
	    # frame.
	    gdb_test_no_output "set \$tpidr2 = 0x0"

	    # Select the frame that contains "main".
	    gdb_test "frame 2" "#2.* main \\\(.*\\\) at.*"

	    for {set row 0} {$row < 32} {incr row} {
		set register_name "\$z${row}\.b\.u"
		gdb_test "print sizeof $register_name" " = $sve_vl" "size of $register_name in the signal frame"
		gdb_test "print $register_name" $sve_pattern "$register_name contents from signal frame"
	    }

	    # Check the size of ZA in the signal frame.
	    set expected_za_size [expr $svl * $svl]
	    gdb_test "print sizeof \$za" " = $expected_za_size" "size of za in signal frame"

	    # Check the value of SVCR in the signal frame.
	    gdb_test "print \$svcr" [get_svcr_value $state] "svcr from signal frame"

	    # Check the value of ZA in the signal frame.
	    gdb_test "print \$za" $za_pattern "za contents from signal frame"

	    # Check the value of TPIDR2 in the signal frame.
	    gdb_test "print/x \$tpidr2" " = 0x102030405060708" "tpidr2 contents from signal frame"

	    # Check the value of SME2 ZT0 in the signal frame.
	    if [is_sme2_available] {
		# The target supports SME2.
		set zt_size 64
		gdb_test "print sizeof \$zt0" " = $zt_size"
		set zt_pattern [string_to_regexp [1d_array_value_pattern $sme2_byte $zt_size]]
		gdb_test "print \$zt0" " = $zt_pattern" "zt contents from signal frame"
	    }
	}
    }
}

require is_aarch64_target
require allow_aarch64_sve_tests
require allow_aarch64_sme_tests

test_sme_registers_sigframe $id_start $id_end
