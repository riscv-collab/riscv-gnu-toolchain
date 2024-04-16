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

# Exercise the following:
# - Printing ZA registers when there is no ZA state.
# - Setting values of ZA registers when there is no ZA state.
# - Validating ZA state is activated when we write to ZA registers.
# - Validate that reading ZT0 without an active ZA state works as expected.

load_lib aarch64-scalable.exp

#
# Validate that the ZA registers have the expected state.
#
proc_with_prefix check_regs { vl svl } {
    # Check VG to make sure it is correct
    set expected_vg [expr $vl / 8]
    gdb_test "print \$vg" "= ${expected_vg}"

    # Check SVG to make sure it is correct
    set expected_svg [expr $svl / 8]
    gdb_test "print \$svg" "= ${expected_svg}"

    # Make sure there is no SM or ZA state.
    if [gdb_test "print \$svcr" "= \\\[ \\\]"] {
	fail "incorrect ZA state"
	return -1
    }

    # Check the size of ZA.
    set expected_za_size [expr $svl * $svl]
    gdb_test "print sizeof \$za" " = $expected_za_size"

    # Check the size of Z0.
    gdb_test "print sizeof \$z0" " = $vl"

    # Set the expected ZA pattern.
    set za_pattern [string_to_regexp [2d_array_value_pattern 0 $svl $svl]]

    # Check ZA.
    gdb_test "print \$za" $za_pattern

    # Exercise reading/writing the tile slice pseudo-registers.
    set last_tile 1
    set last_slice $svl
    set elements $svl
    set expected_size $svl
    foreach_with_prefix granularity {"b" "h" "s" "d" "q"} {
	set pattern [string_to_regexp [1d_array_value_pattern 0 $elements]]
	for {set tile 0} {$tile < $last_tile} {incr tile} {
	    for {set slice 0} {$slice < $last_slice} {incr slice} {
		foreach_with_prefix direction {"h" "v"} {
		    set register_name "\$za${tile}${direction}${granularity}${slice}"
		    # Test the size.
		    gdb_test "print sizeof ${register_name}" " = ${expected_size}"
		    gdb_test "print ${register_name}" $pattern
		}
	    }
	}
	set last_tile [expr $last_tile * 2]
	set last_slice [expr ($last_slice / 2)]
	set elements [expr ($elements / 2)]
    }

    # Exercise reading/writing the tile pseudo-registers.
    set last_tile 1
    set elements $svl
    set expected_size [expr $svl * $svl]
    foreach_with_prefix granularity {"b" "h" "s" "d" "q"} {
	set pattern [string_to_regexp [2d_array_value_pattern 0 $elements $elements]]
	for {set tile 0} {$tile < $last_tile} {incr tile} {
	    set register_name "\$za${tile}${granularity}"
	    # Test the size.
	    gdb_test "print sizeof ${register_name}" " = ${expected_size}"
	    gdb_test "print ${register_name}" $pattern
	}
	set last_tile [expr $last_tile * 2]
	set expected_size [expr $expected_size / 2]
	set elements [expr ($elements / 2)]
    }

    # Exercise reading from SME2 registers.
    if [is_sme2_available] {
	# The target supports SME2.
	set zt_size 64
	gdb_test "print sizeof \$zt0" " = $zt_size"

	# If ZA is not active, ZT0 will always be zero.
	set zt_pattern [string_to_regexp [1d_array_value_pattern 0 $zt_size]]
	gdb_test "print \$zt0" " = $zt_pattern"
    }
}

#
# Cycle through all ZA registers and pseudo-registers and validate that their
# contents are unavailable (zeroed out) for vector length SVL.
#
proc test_sme_registers_unavailable { id_start id_end } {

    set compile_flags {"debug" "macros"}
    lappend compile_flags "additional_flags=-DID_START=${id_start}"
    lappend compile_flags "additional_flags=-DID_END=${id_end}"

    standard_testfile ${::srcdir}/${::subdir}/aarch64-sme-regs-unavailable.c
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

    gdb_test_no_output "set print repeats 1"

    set prctl_breakpoint "stop 1"
    gdb_breakpoint [gdb_get_line_number $prctl_breakpoint]

    for {set id $id_start} {$id <= $id_end} {incr id} {
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

	with_test_prefix "prctl, vl=${vl} svl=${svl}" {
	    # If the SVE or SME vector length is not supported, just skip
	    # these next tests.
	    if {$skip_unsupported} {
		untested "unsupported configuration on target"
		continue
	    }

	    # Run the program until it has adjusted svl.
	    gdb_continue_to_breakpoint $prctl_breakpoint

	    check_regs $vl $svl
	}
    }

    set non_prctl_breakpoint "stop 2"
    gdb_breakpoint [gdb_get_line_number $non_prctl_breakpoint]
    gdb_continue_to_breakpoint $non_prctl_breakpoint

    for {set id $id_start} {$id <= $id_end} {incr id} {
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

	with_test_prefix "gdb, vl=${vl} svl=${svl}" {

	    # If the SVE or SME vector length is not supported, just skip
	    # these next tests.
	    if {$skip_unsupported} {
		untested "unsupported configuration on target"
		continue
	    }

	    # Adjust vg and svg.
	    set vg_value [expr $vl / 8]
	    set svg_value [expr $svl / 8]
	    gdb_test_no_output "set \$vg = ${vg_value}"
	    gdb_test_no_output "set \$svg = ${svg_value}"

	    check_regs $vl $svl
	}
    }
}

require is_aarch64_target
require allow_aarch64_sve_tests
require allow_aarch64_sme_tests

test_sme_registers_unavailable $id_start $id_end
