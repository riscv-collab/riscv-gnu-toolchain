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

# Test function calls on C++ functions that have no debug information.
# See gdb/22736.  Put the called function in a different object to ensure
# the rest of the test can be complied with debug information.  Whilst we
# are at it, also test functions with debug information and C functions too.

if [target_info exists gdb,cannot_call_functions] {
    unsupported "this target can not call functions"
    continue
}

set main_basename infcall-nodebug-main
set lib_basename infcall-nodebug-lib
standard_testfile ${main_basename}.c ${lib_basename}.c

set mainsrc "${srcdir}/${subdir}/${srcfile}"
set libsrc "${srcdir}/${subdir}/${srcfile2}"

# Build both source files to objects using language LANG. Use SYMBOLS to build
# with either debug symbols or without - but always build the main file with
# debug.  Then make function calls across the files.

proc build_and_run_test { lang symbols } {

    global main_basename lib_basename mainsrc libsrc binfile testfile
    global gdb_prompt

    if { $symbols == "debug" } {
	set debug_flags "debug"
    } else {
	set debug_flags ""
    }

    # Compile both files to objects, then link together.

    set main_flags "$lang debug"
    set lib_flags "$lang $debug_flags"
    set main_o [standard_output_file ${main_basename}.o]
    set lib_o [standard_output_file ${lib_basename}.o]
    set binfile [standard_output_file ${testfile}]

    if { [gdb_compile $mainsrc $main_o object ${main_flags}] != "" } {
	untested "failed to compile main file to object"
	return -1
    }

    if { [gdb_compile $libsrc $lib_o object ${lib_flags}] != "" } {
	untested "failed to compile secondary file to object"
	return -1
    }

    if { [gdb_compile "$main_o $lib_o" ${binfile} executable ""] != "" } {
	untested "failed to compile"
	return -1
    }

    # Startup and run to main.

    clean_restart $binfile

    if ![runto_main] then {
	return
    }

    # Function call with cast.

    gdb_test "p (int)foo()" " = 1"

    # Function call without cast.  Will error if there are no debug symbols.

    set test "p foo()"
    gdb_test_multiple $test $test {
	-re " = 1\r\n$gdb_prompt " {
	    gdb_assert [ string equal $symbols "debug" ]
	    pass $test
	}
	-re "has unknown return type; cast the call to its declared return type\r\n$gdb_prompt " {
	    gdb_assert ![ string equal $symbols "debug" ]
	    pass $test
	}
    }

}

build_and_run_test $lang $debug
