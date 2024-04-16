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

# This file is part of the gdb testsuite

# The types we're going to test.

set typelist {
    char {unsigned char}
    short {unsigned short}
    int {unsigned int}
    long {unsigned long}
    {long long} {unsigned long long}
    float
    double {long double}
}

if {[has_int128_c]} {
    # Note we don't check "unsigned __int128" yet because at least gcc
    # canonicalizes the name to "__int128 unsigned", and there isn't a
    # c-exp.y production for this.
    # https://sourceware.org/bugzilla/show_bug.cgi?id=20991
    lappend typelist __int128
}

# Build source file for testing alignment handling of language LANG.
# Returns the name of the newly created source file.
proc prepare_test_source_file { lang } {
    global typelist

    # Create the test file.

    if { $lang == "c++" } {
	set suffix "cpp"
	set align_func "alignof"
    } else {
	set suffix "c"
	set align_func "_Alignof"
    }

    set filename [standard_output_file "align.$suffix"]
    set outfile [open $filename w]

    # Prologue.
    puts -nonewline $outfile "#define DEF(T,U) struct align_pair_ ## T ## _x_ ## U "
    puts $outfile "{ T one; U two; }"
    if { $lang == "c++" } {
	puts -nonewline $outfile "#define DEF_WITH_1_STATIC(T,U) struct align_pair_static_ ## T ## _x_ ## U "
	puts $outfile "{ static T one; U two; }"
	puts -nonewline $outfile "#define DEF_WITH_2_STATIC(T,U) struct align_pair_static_ ## T ## _x_static_ ## U "
	puts $outfile "{ static T one; static U two; }"
    }
    if { $lang == "c" } {
	puts $outfile "unsigned a_void = ${align_func} (void);"
    }

    # First emit single items.
    foreach type $typelist {
	set utype [join [split $type] _]
	if {$type != $utype} {
	    puts $outfile "typedef $type $utype;"
	}
	puts $outfile "$type item_$utype;"
	if { $lang == "c" } {
	    puts $outfile "unsigned a_$utype\n  = ${align_func} ($type);"
	}
	set utype [join [split $type] _]
    }

    # Now emit all pairs.
    foreach type $typelist {
	set utype [join [split $type] _]
	foreach inner $typelist {
	    set uinner [join [split $inner] _]
	    puts $outfile "DEF ($utype, $uinner);"
	    set joined "${utype}_x_${uinner}"
	    puts $outfile "struct align_pair_$joined item_${joined};"
	    puts $outfile "unsigned a_${joined}"
	    puts $outfile "  = ${align_func} (struct align_pair_${joined});"

	    if { $lang == "c++" } {
		puts $outfile "DEF_WITH_1_STATIC ($utype, $uinner);"
		set joined "static_${utype}_x_${uinner}"
		puts $outfile "struct align_pair_$joined item_${joined};"
		puts $outfile "$utype align_pair_${joined}::one = 0;"
		puts $outfile "unsigned a_${joined}"
		puts $outfile "  = ${align_func} (struct align_pair_${joined});"

		puts $outfile "DEF_WITH_2_STATIC ($utype, $uinner);"
		set joined "static_${utype}_x_static_${uinner}"
		puts $outfile "struct align_pair_$joined item_${joined};"
		puts $outfile "$utype align_pair_${joined}::one = 0;"
		puts $outfile "$uinner align_pair_${joined}::two = 0;"
		puts $outfile "unsigned a_${joined}"
		puts $outfile "  = ${align_func} (struct align_pair_${joined});"
	    }
	}
    }

    # Epilogue.
    puts $outfile "
	int main() {
    "

    # Clang with LTO garbage collects unused global variables, even at
    # -O0.  Likewise AIX GCC.  Add uses to all global variables to
    # prevent it.

    if { $lang == "c" } {
	puts $outfile "a_void++;"
    }

    # First, add uses for single items.
    foreach type $typelist {
	set utype [join [split $type] _]
	puts $outfile "item_$utype++;"
	if { $lang == "c" } {
	    puts $outfile "a_$utype++;"
	}
    }

    # Now add uses for all pairs.
    foreach type $typelist {
	set utype [join [split $type] _]
	foreach inner $typelist {
	    set uinner [join [split $inner] _]
	    set joined "${utype}_x_${uinner}"
	    puts $outfile "item_${joined}.one++;"
	    puts $outfile "a_${joined}++;"

	    if { $lang == "c++" } {
		set joined "static_${utype}_x_${uinner}"
		puts $outfile "item_${joined}.one++;"
		puts $outfile "a_${joined}++;"

		set joined "static_${utype}_x_static_${uinner}"
		puts $outfile "item_${joined}.one++;"
		puts $outfile "a_${joined}++;"
	    }
	}
    }

    puts $outfile "
	    return 0;
	}
    "

    close $outfile

    return $filename
}

# Run the alignment test for the language LANG.
proc run_alignment_test { lang } {
    global testfile srcfile typelist
    global subdir

    set filename [prepare_test_source_file $lang]

    set flags {debug}
    if { "$lang" == "c++" } {
	lappend flags "c++"
	lappend flags "additional_flags=-std=c++11"
    }
    standard_testfile $filename
    if {[prepare_for_testing "failed to prepare" "$testfile" $srcfile $flags]} {
	return -1
    }

    if {![runto_main]} {
	perror "test suppressed"
	return
    }

    if { $lang == "c++" } {
	set align_func "alignof"
    } else {
	set align_func "_Alignof"
    }

    foreach type $typelist {
	set utype [join [split $type] _]
	if { $lang == "c" } {
	    set expected [get_integer_valueof a_$utype 0]
	    gdb_test "print ${align_func}($type)" " = $expected"
	}

	foreach inner $typelist {
	    set uinner [join [split $inner] _]
	    set expected [get_integer_valueof a_${utype}_x_${uinner} 0]
	    gdb_test "print ${align_func}(struct align_pair_${utype}_x_${uinner})" \
		" = $expected"

	    if { $lang == "c++" } {
		set expected [get_integer_valueof a_static_${utype}_x_${uinner} 0]
		gdb_test "print ${align_func}(struct align_pair_static_${utype}_x_${uinner})" \
		    " = $expected"

		set expected [get_integer_valueof a_static_${utype}_x_static_${uinner} 0]
		gdb_test "print ${align_func}(struct align_pair_static_${utype}_x_static_${uinner})" \
		    " = $expected"
	    }
	}
    }

    if { $lang == "c" } {
	set expected [get_integer_valueof a_void 0]
	gdb_test "print ${align_func}(void)" " = $expected"
    }
}

run_alignment_test $lang
