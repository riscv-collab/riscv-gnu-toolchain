# This testcase is part of GDB, the GNU debugger.

# Copyright 2018-2024 Free Software Foundation, Inc.

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

# Some targets can't call functions, so don't even bother with this
# test.

if [target_info exists gdb,cannot_call_functions] {
    unsupported "this target can not call functions"
    continue
}

set int_types { tc ts ti tl tll }
set float_types { tf td tld }
set complex_types { tfc tdc tldc }

set compile_flags {debug}
if [support_complex_tests] {
    lappend compile_flags "additional_flags=-DTEST_COMPLEX"
    lappend compile_flags "additional_flags=-Wno-psabi"
}

if { $lang == "c++" && [test_compiler_info clang*] } {
    # Clang rightly deduces that the static member tests are
    # tautological comparisons, so we need to inhibit that
    # particular warning in order to build.
    lappend compile_flags "additional_flags=-Wno-tautological-compare"
}

lappend_include_file compile_flags $srcdir/lib/attributes.h

# Given N (0..25), return the corresponding alphabetic letter in upper
# case.

proc I2A { n } {
    return [string range "ABCDEFGHIJKLMNOPQRSTUVWXYZ" $n $n]
}

# Compile a variant of nested-structs.c using TYPES to specify the
# types of the struct fields within the source.  Run up to main.
# Also updates the global "testfile" to reflect the most recent build.

proc start_nested_structs_test { lang types } {
    global testfile
    global srcfile
    global binfile
    global subdir
    global srcdir
    global compile_flags

    standard_testfile infcall-nested-structs.c

    # Create the additional flags
    set flags $compile_flags
    lappend flags $lang
    lappend flags "additional_flags=-O2"

    for {set n 0} {$n<[llength ${types}]} {incr n} {
	set m [I2A ${n}]
	set t [lindex ${types} $n]
	lappend flags "additional_flags=-Dt${m}=${t}"
	append testfile "-" "$t"
    }

    if  { [gdb_compile "${srcdir}/${subdir}/${srcfile}" "${binfile}" executable "${flags}"] != "" } {
	unresolved "failed to compile"
	return 0
    }

    # Start with a fresh gdb.
    clean_restart ${binfile}

    # Make certain that the output is consistent
    gdb_test_no_output "set print sevenbit-strings"
    gdb_test_no_output "set print address off"
    gdb_test_no_output "set print pretty off"
    gdb_test_no_output "set width 0"
    gdb_test_no_output "set print elements 300"

    # Advance to main
    if { ![runto_main] } then {
	return 0
    }

    # Now continue forward to a suitable location to run the tests.
    # Some targets only enable the FPU on first use, so ensure that we
    # have used the FPU before we make calls from GDB to code that
    # could use the FPU.
    gdb_breakpoint [gdb_get_line_number "Break Here"] temporary
    gdb_continue_to_breakpoint "breakpt" ".* Break Here\\. .*"

    return 1
}

# Assuming GDB is stopped at main within a test binary, run some tests
# passing structures, and reading return value structures.

proc run_tests { lang types } {
    global gdb_prompt

    foreach {name} {struct_01_01 struct_01_02 struct_01_03 struct_01_04
                    struct_02_01 struct_02_02 struct_02_03 struct_02_04
                    struct_04_01 struct_04_02 struct_04_03 struct_04_04
                    struct_05_01 struct_05_02 struct_05_03 struct_05_04
                    struct_static_02_01 struct_static_02_02 struct_static_02_03 struct_static_02_04
                    struct_static_04_01 struct_static_04_02 struct_static_04_03 struct_static_04_04
                    struct_static_06_01 struct_static_06_02 struct_static_06_03 struct_static_06_04} {

	# Only run static member tests on C++
	if { $lang == "c" && [regexp "static" $name match] } {
	    continue
	}

	gdb_test "p/d check_arg_${name} (ref_val_${name})" "= 1"

	set refval [ get_valueof "" "ref_val_${name}" "" ]
	verbose -log "Refval: ${refval}"

	set test "check return value ${name}"
	if { ${refval} != "" } {

	    set answer [ get_valueof "" "rtn_str_${name} ()" "XXXX"]
	    verbose -log "Answer: ${answer}"

	    gdb_assert [string eq ${answer} ${refval}] ${test}
	} else {
	    unresolved $test
	}
    }
}

# Set up a test prefix, compile the test binary, run to main, and then
# run some tests.

proc start_gdb_and_run_tests { lang types } {
    set prefix "types"

    foreach t $types {
	append prefix "-" "${t}"
    }

    with_test_prefix $prefix {
	if { [start_nested_structs_test $lang $types] } {
	    run_tests $lang $prefix
	}
    }
}

foreach ta $int_types {
    start_gdb_and_run_tests $lang $ta
}

if [support_complex_tests] {
    foreach ta $complex_types {
	start_gdb_and_run_tests $lang $ta
    }
}

if {[allow_float_test]} {
    foreach ta $float_types {
	start_gdb_and_run_tests $lang $ta
    }

    foreach ta $int_types {
	foreach tb $float_types {
	    start_gdb_and_run_tests $lang [list $ta $tb]
	}
    }

    foreach ta $float_types {
	foreach tb $int_types {
	    start_gdb_and_run_tests $lang [list $ta $tb]
	}

	foreach tb $float_types {
	    start_gdb_and_run_tests $lang [list $ta $tb]
	}
    }
}
