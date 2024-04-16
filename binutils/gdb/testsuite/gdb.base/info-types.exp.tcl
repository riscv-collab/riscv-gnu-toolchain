# Copyright 2019-2024 Free Software Foundation, Inc.
#
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

# Check that 'info types' produces the expected output for an inferior
# containing a number of different types.

# Run 'info types' test, compiling the test file for language LANG,
# which should be either 'c' or 'c++'.
proc run_test { lang } {
    global testfile
    global srcfile
    global binfile
    global subdir
    global srcdir
    global compile_flags

    standard_testfile info-types.c

    if {[prepare_for_testing "failed to prepare" \
	     "${testfile}" $srcfile "debug $lang"]} {
	return -1
    }
    gdb_test_no_output "set auto-solib-add off"

    if ![runto_main] then {
	return 0
    }

    set file_re "File .*[string_to_regexp $srcfile]:"

    if { $lang == "c++" } {
	set output_lines \
	    [list \
		 "^All defined types:" \
		 ".*" \
		 $file_re \
		 "98:\[\t \]+CL;" \
		 "42:\[\t \]+anon_struct_t;" \
		 "65:\[\t \]+anon_union_t;" \
		 "21:\[\t \]+baz_t;" \
		 "33:\[\t \]+enum_t;" \
		 "56:\[\t \]+union_t;" \
		 "52:\[\t \]+typedef enum {\\.\\.\\.} anon_enum_t;" \
		 "45:\[\t \]+typedef anon_struct_t anon_struct_t;" \
		 "68:\[\t \]+typedef anon_union_t anon_union_t;" \
		 "28:\[\t \]+typedef baz_t baz;" \
		 "31:\[\t \]+typedef baz_t \\* baz_ptr;" \
		 "27:\[\t \]+typedef baz_t baz_t;" \
		 "\[\t \]+double" \
		 "\[\t \]+float" \
		 "\[\t \]+int" \
		 "103:\[\t \]+typedef CL my_cl;" \
		 "38:\[\t \]+typedef enum_t my_enum_t;" \
		 "17:\[\t \]+typedef float my_float_t;" \
		 "16:\[\t \]+typedef int my_int_t;" \
		 "104:\[\t \]+typedef CL \\* my_ptr;" \
		 "54:\[\t \]+typedef enum {\\.\\.\\.} nested_anon_enum_t;" \
		 "47:\[\t \]+typedef anon_struct_t nested_anon_struct_t;" \
		 "70:\[\t \]+typedef anon_union_t nested_anon_union_t;" \
		 "30:\[\t \]+typedef baz_t nested_baz;" \
		 "29:\[\t \]+typedef baz_t nested_baz_t;" \
		 "39:\[\t \]+typedef enum_t nested_enum_t;" \
		 "19:\[\t \]+typedef float nested_float_t;" \
		 "18:\[\t \]+typedef int nested_int_t;" \
		 "62:\[\t \]+typedef union_t nested_union_t;(" \
		 "\[\t \]+unsigned int)?" \
		 "($|\r\n.*)"]
    } else {
	set output_lines \
	    [list \
		 "^All defined types:" \
		 ".*" \
		 $file_re \
		 "52:\[\t \]+typedef enum {\\.\\.\\.} anon_enum_t;" \
		 "45:\[\t \]+typedef struct {\\.\\.\\.} anon_struct_t;" \
		 "68:\[\t \]+typedef union {\\.\\.\\.} anon_union_t;" \
		 "28:\[\t \]+typedef struct baz_t baz;" \
		 "31:\[\t \]+typedef struct baz_t \\* baz_ptr;" \
		 "21:\[\t \]+struct baz_t;" \
		 "\[\t \]+double" \
		 "33:\[\t \]+enum enum_t;" \
		 "\[\t \]+float" \
		 "\[\t \]+int" \
		 "38:\[\t \]+typedef enum enum_t my_enum_t;" \
		 "17:\[\t \]+typedef float my_float_t;" \
		 "16:\[\t \]+typedef int my_int_t;" \
		 "54:\[\t \]+typedef enum {\\.\\.\\.} nested_anon_enum_t;" \
		 "47:\[\t \]+typedef struct {\\.\\.\\.} nested_anon_struct_t;" \
		 "70:\[\t \]+typedef union {\\.\\.\\.} nested_anon_union_t;" \
		 "30:\[\t \]+typedef struct baz_t nested_baz;" \
		 "29:\[\t \]+typedef struct baz_t nested_baz_t;" \
		 "39:\[\t \]+typedef enum enum_t nested_enum_t;" \
		 "19:\[\t \]+typedef float nested_float_t;" \
		 "18:\[\t \]+typedef int nested_int_t;" \
		 "62:\[\t \]+typedef union union_t nested_union_t;" \
		 "56:\[\t \]+union union_t;(" \
		 "\[\t \]+unsigned int)?" \
		 "($|\r\n.*)"]
    }

    gdb_test_lines "info types" "" [multi_line {*}$output_lines]
}

run_test $lang
