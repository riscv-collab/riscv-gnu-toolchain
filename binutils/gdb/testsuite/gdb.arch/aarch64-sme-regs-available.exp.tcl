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

# Exercise reading/writing ZA registers when there is ZA state.
# Exercise reading/writing to ZT0 when there is ZA state available.

load_lib aarch64-scalable.exp

#
# Cycle through all ZA registers and pseudo-registers and validate that their
# contents are available for vector length SVL.
#
# Make sure reading/writing to ZA registers work as expected.
#
proc check_regs { mode vl svl } {
    # Check VG to make sure it is correct
    set expected_vg [expr $vl / 8]
    gdb_test "print \$vg" "= ${expected_vg}"

    # Check SVG to make sure it is correct
    set expected_svg [expr $svl / 8]
    gdb_test "print \$svg" "= ${expected_svg}"

    # If svl is adjusted by prctl, we will have ZA enabled.  If gdb is
    # adjusting svl, ZA will not be enabled by default.  It will only be
    # enabled when ZA is written to.
    set za_state "= \\\[ ZA \\\]"
    if {$mode == "gdb"} {
	set za_state "= \\\[ \\\]"
    }

    # Check SVCR.
    if [gdb_test "print \$svcr" $za_state "svcr before assignments" ] {
	fail "incorrect za state"
	return -1
    }

    # Check the size of ZA.
    set expected_za_size [expr $svl * $svl]
    gdb_test "print sizeof \$za" " = $expected_za_size"

    # Check the size of Z0.
    gdb_test "print sizeof \$z0" " = $vl"

    # Exercise reading/writing from/to ZA.
    initialize_2d_array "\$za" 255 $svl $svl
    set pattern [string_to_regexp [2d_array_value_pattern 255 $svl $svl]]
    gdb_test "print \$za" " = $pattern" "read back from za"

    # Exercise reading/writing from/to the tile pseudo-registers.
    set last_tile 1
    set expected_size [expr $svl * $svl]
    set tile_svl $svl
    set za_state "= \\\[ ZA \\\]"
    foreach_with_prefix granularity {"b" "h" "s" "d" "q"} {
	for {set tile 0} {$tile < $last_tile} {incr tile} {
	    set register_name "\$za${tile}${granularity}"

	    # Test the size.
	    gdb_test "print sizeof ${register_name}" " = ${expected_size}"

	    # Test reading/writing
	    initialize_2d_array $register_name 255 $tile_svl $tile_svl

	    # Make sure we have ZA state.
	    if [gdb_test "print \$svcr" $za_state "svcr after assignment to ${register_name}" ] {
		fail "incorrect za state"
		return -1
	    }

	    set pattern [string_to_regexp [2d_array_value_pattern 255 $tile_svl $tile_svl]]
	    gdb_test "print $register_name" " = $pattern" "read back from $register_name"
	}
	set last_tile [expr $last_tile * 2]
	set expected_size [expr $expected_size / 2]
	set tile_svl [expr $tile_svl / 2]
    }

    # Exercise reading/writing from/to the tile slice pseudo-registers.
    set last_tile 1
    set last_slice $svl
    set expected_size $svl
    set num_elements $svl
    foreach_with_prefix granularity {"b" "h" "s" "d" "q"} {
	for {set tile 0} {$tile < $last_tile} {incr tile} {
	    for {set slice 0} {$slice < $last_slice} {incr slice} {
		foreach_with_prefix direction {"h" "v"} {
		    set register_name "\$za${tile}${direction}${granularity}${slice}"

		    # Test the size.
		    gdb_test "print sizeof ${register_name}" " = ${expected_size}"

		    # Test reading/writing
		    initialize_1d_array $register_name 255 $num_elements

		    # Make sure we have ZA state.
		    if [gdb_test "print \$svcr" $za_state "svcr after assignment of ${register_name}" ] {
			fail "incorrect za state"
			return -1
		    }

		    set pattern [string_to_regexp [1d_array_value_pattern 255 $num_elements]]
		    gdb_test "print $register_name" " = $pattern" "read back from $register_name"
		}
	    }
	}
	set last_tile [expr $last_tile * 2]
	set last_slice [expr ($last_slice / 2)]
	set num_elements [expr $num_elements / 2]
    }

    # Exercise reading/writing from/to SME2 registers.
    if [is_sme2_available] {
      # The target supports SME2.
      set zt_size 64
      gdb_test "print sizeof \$zt0" " = $zt_size"

      # Initially, when ZA is activated, ZT0 will be all zeroes.
      set zt_pattern [string_to_regexp [1d_array_value_pattern 0 $zt_size]]
      gdb_test "print \$zt0" " = $zt_pattern" "validate zeroed zt0"

      # Validate that writing to ZT0 does the right thing.
      initialize_1d_array "\$zt0" 255 $zt_size
      set zt_pattern [string_to_regexp [1d_array_value_pattern 255 $zt_size]]
      gdb_test "print \$zt0" " = $zt_pattern" "read back from zt0"
    }
}

#
# Cycle through all ZA registers and pseudo-registers and validate their
# contents.
#
proc test_sme_registers_available { id_start id_end } {

    set compile_flags {"debug" "macros"}
    lappend compile_flags "additional_flags=-DID_START=${id_start}"
    lappend compile_flags "additional_flags=-DID_END=${id_end}"

    standard_testfile ${::srcdir}/${::subdir}/aarch64-sme-regs-available.c
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

	set mode "prctl"
	with_test_prefix "$mode, vl=${vl} svl=${svl}" {
	    # If the SVE or SME vector length is not supported, just skip
	    # these next tests.
	    if {$skip_unsupported} {
		untested "unsupported configuration on target"
		continue
	    }

	    # Run the program until it has adjusted svl.
	    gdb_continue_to_breakpoint $prctl_breakpoint
	    check_regs $mode $vl $svl
	}
    }

    set non_prctl_breakpoint "stop 2"
    gdb_breakpoint [gdb_get_line_number $non_prctl_breakpoint]

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

	set mode "gdb"
	with_test_prefix "$mode, vl=${vl} svl=${svl}" {
	    # If the SVE or SME vector length is not supported, just skip
	    # these next tests.
	    if {$skip_unsupported} {
		untested "unsupported configuration on target"
		continue
	    }

	    # Run the program until we stop at the point where gdb should
	    # adjust the SVE and SME vector lengths.
	    gdb_continue_to_breakpoint $non_prctl_breakpoint

	    # Adjust svl via gdb.
	    set vg_value [expr $vl / 8]
	    set svg_value [expr $svl / 8]
	    gdb_test_no_output "set \$vg = ${vg_value}"
	    gdb_test_no_output "set \$svg = ${svg_value}"

	    check_regs $mode $vl $svl
	}
    }
}

require is_aarch64_target
require allow_aarch64_sve_tests
require allow_aarch64_sme_tests

test_sme_registers_available $id_start $id_end
