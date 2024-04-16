# Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

# This file is part of the gdb testsuite.

# Test manually setting _all_ combinations of all supported bfd
# architectures and OS ABIs.  This ensures that gdbarch initialization
# routines handle unusual combinations gracefully, at least without
# crashing.

# While at it, because the number of possible combinations is quite
# large, embed other checks that might be useful to do with all
# supported archs.

# One such test is ensuring that printing float, double and long
# double types works in cross/bi-arch scenarios.  Some of GDB's float
# format conversion routines used to fail to consider that even if
# host and target floating formats match, their sizes may still
# differ.  E.g., on x86, long double is 80-bit extended precision on
# both 32-bit vs 64-bit, but it's stored as 96 bit on 32-bit, and 128
# bit on 64-bit.  This resulted in GDB accessing memory out of bounds.
# This test catches the issue when run against gdb linked with
# libmcheck, or run under Valgrind.

# Note: this test is actually split in several driver .exp files, in
# order to be able to parallelize the work.  Each driver .exp file
# exercises a different slice of the supported architectures.  See
# all-architectures-*.exp and the TEST_SLICE variable.

clean_restart

# By default, preparation steps don't output a PASS message.  This is
# because the testcase has several thousand such steps.
set want_tests_messages 0

# Call this when an "internal" preparation-like step test passed.
# Logs the pass in gdb.log, but not in gdb.sum.

proc internal_pass {message} {
    global want_tests_messages

    if {$want_tests_messages} {
	pass $message
    } else {
	# Skip the sum file, but still log an internal pass in the log
	# file.
	global pf_prefix

	verbose -log "IPASS: $pf_prefix $message"
    }
}

# The number of times gdb_test_internal was called, and the number of
# time that resulted in an internal pass.  If these don't match, then
# some test failed.
set test_count 0
set internal_pass_count 0

# Like gdb_test, but calls internal_pass instead of pass, on success.

proc gdb_test_internal {cmd pattern {message ""}} {
    global test_count internal_pass_count
    global gdb_prompt

    incr test_count

    if {$message == ""} {
	set message $cmd
    }

    gdb_test_multiple $cmd $message {
	-re "$pattern\r\n$gdb_prompt $" {
	    internal_pass $message
	    incr internal_pass_count
	}
    }
}

gdb_test_internal "set max-completions unlimited" \
    "^set max-completions unlimited"

set supported_archs [get_set_option_choices "set architecture"]
# There should be at least one more than "auto".
gdb_assert {[llength $supported_archs] > 1} "at least one architecture"

set supported_osabis [get_set_option_choices "set osabi"]
# There should be at least one more than "auto" and "default".
gdb_assert {[llength $supported_osabis] > 2} "at least one osabi"

if {[lsearch $supported_archs "mips"] >= 0} {
    set supported_mipsfpu [get_set_option_choices "set mipsfpu"]
    set supported_mips_abi [get_set_option_choices "set mips abi"]

    gdb_assert {[llength $supported_mipsfpu] != 0} "at least one mipsfpu"
    gdb_assert {[llength $supported_mips_abi] != 0} "at least one mips abi"
}

if {[lsearch $supported_archs "arm"] >= 0} {
    set supported_arm_fpu [get_set_option_choices "set arm fpu"]
    set supported_arm_abi [get_set_option_choices "set arm abi"]

    gdb_assert {[llength $supported_arm_fpu] != 0} "at least one arm fpu"
    gdb_assert {[llength $supported_arm_abi] != 0} "at least one arm abi"
}

set default_architecture "i386"

# Exercise printing float, double and long double.

proc print_floats {} {
    gdb_test_internal "ptype 1.0L" "type = long double" "ptype, long double"
    gdb_test_internal "print 1.0L" " = 1" "print, long double"

    gdb_test_internal "ptype 1.0" "type = double" "ptype, double"
    gdb_test_internal "print 1.0" " = 1" "print, double"

    gdb_test_internal "ptype 1.0f" "type = float" "ptype, float"
    gdb_test_internal "print 1.0f" " = 1" "print, float"
}

# Run tests on the current architecture ARCH.

proc do_arch_tests {arch} {
    print_floats

    # When we disassemble using the default architecture then we
    # expect that the only error we should get from the disassembler
    # is a memory error.
    #
    # When we force the architecture to something other than the
    # default then we might get the message about unknown errors, this
    # happens if the libopcodes disassembler returns -1 without first
    # registering a memory error.
    set pattern "Cannot access memory at address 0x100"
    if { $arch != $::default_architecture } {
	set pattern "(($pattern)|(unknown disassembler error \\(error = -1\\)))"
    }

    # GDB can't access memory because there is no loaded executable
    # nor live inferior.
    gdb_test_internal "disassemble 0x100,+4" "${pattern}"
}

# Given we can't change arch, osabi, endianness, etc. atomically, we
# need to silently ignore the case of the current OS ABI (not the one
# we'll switch to) not having a handler for the arch.
set osabi_warning \
    [multi_line \
	 "warning: A handler for the OS ABI .* is not built into this configuration" \
	 "of GDB.  Attempting to continue with the default .* settings." \
	 "" \
	 "" \
	]

set endian_warning "(Little|Big) endian target not supported by GDB\r\n"

# Like gdb_test_no_output, but use internal_pass instead of pass, and
# ignore "no handler for OS ABI" warnings.

proc gdb_test_no_output_osabi {cmd test} {
    global osabi_warning
    global gdb_prompt

    gdb_test_multiple "$cmd" $test {
	-re "^${cmd}\r\n(${osabi_warning})?$gdb_prompt $" {
	    internal_pass $test
	}
    }
}

# It'd be nicer/safer to restart GDB on each iteration, but, that
# increases the testcase's run time several times fold.  At the time
# of writing, it'd jump from 20s to 4min on x86-64 GNU/Linux with
# --enable-targets=all.

set num_slices 8
set num_archs [llength $supported_archs]
set archs_per_slice [expr (($num_archs + $num_slices - 1) / $num_slices)]

with_test_prefix "tests" {
    foreach_with_prefix osabi $supported_osabis {

	gdb_test_no_output_osabi "set osabi $osabi" \
	    "set osabi"

	set arch_count 0
	foreach_with_prefix arch $supported_archs {

	    incr arch_count

	    # Skip architectures outside our slice.
	    if {$arch_count < [expr $test_slice * $archs_per_slice]} {
		continue
	    }
	    if {$arch_count >= [expr ($test_slice + 1) * $archs_per_slice]} {
		continue
	    }

	    if {$arch == "auto"} {
		continue
	    }
	    set default_endian ""
	    foreach_with_prefix endian {"auto" "big" "little"} {

		set test "set endian"
		if {$endian == $default_endian} {
		    continue
		} elseif {$endian == "auto"} {
		    gdb_test_multiple "set endian $endian" $test {
			-re "^set endian $endian\r\n(${osabi_warning})?The target endianness is set automatically \\(currently .* endian\\)\\.\r\n$gdb_prompt $" {
			    internal_pass $test
			}
		    }
		} else {
		    gdb_test_multiple "set endian $endian" $test {
			-re "^set endian $endian\r\n${endian_warning}.*\r\n$gdb_prompt $" {
			    internal_pass $test
			    continue
			}
			-re "^set endian $endian\r\n(${osabi_warning})?The target is set to $endian endian\\.\r\n$gdb_prompt $" {
			    internal_pass $test
			}
		    }
		}

		# Skip setting the same architecture again.
		if {$endian == "auto"} {
		    set arch_re [string_to_regexp $arch]
		    set test "set architecture $arch"
		    gdb_test_multiple $test $test {
			-re "^set architecture $arch_re\r\n(${osabi_warning})?The target architecture is set to \"$arch_re\"\\.\r\n$gdb_prompt $" {
			    internal_pass $test
			}
			-re "Architecture .* not recognized.*$gdb_prompt $" {
			    # GDB is missing support for a few
			    # machines that bfd supports.
			    if  {$arch == "powerpc:EC603e"
				 || $arch == "powerpc:e500mc"
				 || $arch == "powerpc:e500mc64"
				 || $arch == "powerpc:vle"
				 || $arch == "powerpc:titan"
				 || $arch == "powerpc:e5500"
				 || $arch == "powerpc:e6500"} {
				if {$want_tests_messages} {
				    kfail $test "gdb/19797"
				}
			    } else {
				fail "$test (arch not recognized)"
			    }
			    continue
			}
		    }

		    # Record what is the default endianess.  As an
		    # optimization, we'll skip testing the manual "set
		    # endian DEFAULT" case.
		    set test "show endian"
		    gdb_test_multiple "show endian" $test {
			-re "currently little endian.*$gdb_prompt $" {
			    set default_endian "little"
			    internal_pass $test
			}
			-re "currently big endian.*$gdb_prompt $" {
			    set default_endian "big"
			    internal_pass $test
			}
		    }
		}

		# Some architectures have extra settings that affect
		# the ABI.  Specify the extra testing axes in a
		# declarative form.
		#
		# A list of {COMMAND, VAR, OPTIONS-LIST} elements.
		set all_axes {}

		if {$arch == "mips"} {
		    lappend all_axes [list "set mips abi" mips_abi $supported_mips_abi]
		    lappend all_axes [list "set mipsfpu" mipsfpu $supported_mipsfpu]
		} elseif {$arch == "arm"} {
		    lappend all_axes [list "set arm abi" arm_abi $supported_arm_abi]
		    lappend all_axes [list "set arm fpu" arm_fpu $supported_arm_fpu]
		}

		# Run testing axis CUR_AXIS.  This is a recursive
		# procedure that tries all combinations of options of
		# all the testing axes.
		proc run_axis {all_axes cur_axis arch} {
		    if {$cur_axis == [llength $all_axes]} {
			do_arch_tests $arch
			return
		    }

		    # Unpack the axis.
		    set axis [lindex $all_axes $cur_axis]
		    set cmd [lindex $axis 0]
		    set var [lindex $axis 1]
		    set options [lindex $axis 2]

		    foreach v $options {
			with_test_prefix "$var=$v" {
			    gdb_test_no_output_osabi "$cmd $v" "$cmd"
			    run_axis $all_axes [expr $cur_axis + 1] $arch
			}
		    }
		}

		run_axis $all_axes 0 $arch
	    }
	}
    }
}

# A test that:
#
#  - ensures there's a PASS if all internal tests actually passed
#
#  - ensures there's at least one test that is interpreted as a
#    regression (a matching PASS->FAIL) if some of the internal tests
#    failed, instead of looking like it could be a new FAIL that could
#    be ignored.
#
gdb_assert {$internal_pass_count == $test_count} "all passed"
