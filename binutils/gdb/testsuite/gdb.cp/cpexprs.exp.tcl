# cpexprs.exp - C++ expressions tests
#
# Copyright 2008-2024 Free Software Foundation, Inc.
#
# Contributed by Red Hat, originally written by Keith Seitz.
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

# This file is part of the gdb testsuite.

# A helper proc which sets a breakpoint at FUNC and attempts to
# run to the breakpoint.
proc test_breakpoint {func} {
    global DEC

    # Return to the top of the test function every time.
    delete_breakpoints
    if { ! [gdb_breakpoint test_function] } {
	fail "set test_function breakpoint for $func"
    } elseif { [gdb_test "continue" \
		    "Continuing.\r\n\r\nBreakpoint $DEC+,.*test_function.*" \
		    "continue to test_function for $func"] != 0 } {
    } else {
	gdb_breakpoint "$func"
	set i [expr {[string last : $func] + 1}]
	set efunc [string_to_regexp [string range $func $i end]]
	gdb_test "continue" \
	    "Continuing.\r\n\r\nBreakpoint $DEC+,.*$efunc.*" \
	    "continue to $func"
    }
}

# Add a function to the list of tested functions
# FUNC is the name of the function (which will be passed to gdb commands)
# TYPE is the type of the function, as expected from the "print" command
# PRINT is the name of the function, as expected result of the print command
#  *OR* "-", indicating that FUNC should be used (needed for virtual/inherited
#   funcs)
# LST is either the expected result of the list command (the comment from
#  the source code) *OR* "-", in which case FUNC will be used
#
# Usage:
# add NAME TYPE PRINT LST
# add NAME TYPE PRINT -
proc add_type_regexp {func type print lst} {
    global all_functions CONVAR ADDR

    set all_functions($func,type) $type
    if {$print == "-"} {
	set print $func
    }

    # An exception: since gdb canonicalizes C++ output,
    # "(void)" must be mutated to "()".
    regsub {\(void\)} $print {()} print

    set all_functions($func,print) \
	"$CONVAR = {$type} $ADDR <[string_to_regexp $print].*>"
    if {$lst == "-"} {
	set lst "$func"
    }
    set all_functions($func,list) ".*// [string_to_regexp $lst]"
}

proc add {func type print lst} {
    add_type_regexp $func [string_to_regexp $type] $print $lst
}

proc get {func cmd} {
    global all_functions
    return $all_functions($func,$cmd)
}

# Returns a list of function names for a given command
proc get_functions {cmd} {
    global all_functions
    set result {}
    foreach i [array names all_functions *,$cmd] {
	if {$all_functions($i) != ""} {
	    set idx [string last , $i]
	    if {$idx != -1} {
		lappend result [string range $i 0 [expr {$idx - 1}]]
	    }
	}
    }

    return [lsort $result]
}

# Some convenience variables for this test
set DEC {[0-9]}; # a decimal number
set HEX {[0-9a-fA-F]}; # a hexidecimal number
set CONVAR "\\\$$DEC+"; # convenience variable regexp
set ADDR "0x$HEX+"; # address

# An array of functions/methods that we are testing...
# Each element consists is indexed by NAME,COMMAND, where
# NAME is the function name and COMMAND is the gdb command that
# we are testing. The value of the array for any index pair is
# the expected result of running COMMAND with the NAME as argument.

# The array holding all functions/methods to test. Valid subindexes
# are (none need character escaping -- "add" will take care of that):

# add name type print_name list
# NAME,type: value is type of function 
# NAME,print: value is print name of function (careful w/inherited/virtual!)
# NAME,list: value is comment in source code on first line of function
#   (without the leading "//")
array set all_functions {}

# "Normal" functions/methods
add {test_function} \
    {int (int, char **)} \
    - \
    -
add {derived::a_function} \
    {void (const derived * const)} \
    - \
    -
add {base1::a_function} \
    {void (const base1 * const)} \
    - \
    -
add {base2::a_function} \
    {void (const base2 * const)} \
    - \
    -

# Constructors

# On targets using the ARM EABI, the constructor is expected to return
# "this".
proc ctor_ret { type } {
    if { [istarget arm*-*eabi*] || [is_aarch32_target] } {
	return "$type *"
    } else {
	return "void "
    }
}

proc ctor_prefix { type } {
    set ret [ctor_ret $type]
    return "${ret}($type * const"
}

proc ctor { type arglist } {
    if { $arglist != "" } {
	set arglist ", $arglist"
    }
    return "[ctor_prefix $type]$arglist)"
}

add {derived::derived} \
    [ctor derived ""] \
    - \
    -
add_type_regexp {base1::base1(void)} \
    "[string_to_regexp [ctor_prefix base1]], (const )?void \\*\\*( const)?\\)" \
    - \
    -
add {base1::base1(int)} \
    [ctor base1 "int"] \
    - \
    -
add_type_regexp {base2::base2} \
    "[string_to_regexp [ctor_prefix base2]], (const )?void \\*\\*( const)?\\)" \
    - \
    -
add {base::base(void)} \
    [ctor base ""] \
    - \
    -
add {base::base(int)} \
    [ctor base "int"] \
    - \
    -

# Destructors

# On targets using the ARM EABI, some destructors are expected
# to return "this".  Others are void.  For internal reasons,
# GCC returns void * instead of $type *; RealView appears to do
# the same.
proc dtor { type } {
    if { [istarget arm*-*eabi*] || [is_aarch32_target] } {
	set ret "void *"
    } else {
	set ret "void "
    }
    return "${ret}($type * const)"
}

add {base::~base} \
    [dtor base] \
    - \
    -

# Overloaded methods (all are const)
add {base::overload(void) const} \
    {int (const base * const)} \
    - \
    {base::overload(void) const}
add {base::overload(int) const} \
    {int (const base * const, int)} \
    - \
    -
add {base::overload(short) const} \
    {int (const base * const, short)} \
    - \
    -
add {base::overload(long) const} \
    {int (const base * const, long)} \
    - \
    -
add {base::overload(char*) const} \
    {int (const base * const, char *)} \
    - \
    -
add {base::overload(base&) const} \
    {int (const base * const, base &)} \
    - \
    -

# Operators
add {base::operator+} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator++} \
    {base (base * const)} \
    - \
    -
add {base::operator+=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator-} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator--} \
    {base (base * const)} \
    - \
    -
add {base::operator-=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator*} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator*=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator/} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator/=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator%} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator%=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator<} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator<=} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator>} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator>=} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator!=} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator==} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator!} \
    {bool (const base * const)} \
    - \
    -
add {base::operator&&} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator||} \
    {bool (const base * const, const base &)} \
    - \
    -
add {base::operator<<} \
    {int (const base * const, int)} \
    - \
    -
add {base::operator<<=} \
    {base (base * const, int)} \
    - \
    -
add {base::operator>>} \
    {int (const base * const, int)} \
    - \
    -
add {base::operator>>=} \
    {base (base * const, int)} \
    - \
    -
add {base::operator~} \
    {int (const base * const)} \
    - \
    -
add {base::operator&} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator&=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator|} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator|=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator^} \
    {int (const base * const, const base &)} \
    - \
    -
add {base::operator^=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator=} \
    {base (base * const, const base &)} \
    - \
    -
add {base::operator()} \
    {void (const base * const)} \
    - \
    -
add {base::operator[]} \
    {int (const base * const, int)} \
    - \
    -
add {base::operator new} \
    {void *(size_t)} \
    - \
    -
add {base::operator delete} \
    {void (void *)} \
    - \
    -
add {base::operator new[]} \
    {void *(size_t)} \
    - \
    -
add {base::operator delete[]} \
    {void (void *)} \
    - \
    -
add {base::operator char*} \
    {char *(const base * const)} \
    - \
    -
add {base::operator fluff*} \
    {fluff *(const base * const)} \
    - \
    -
add {base::operator fluff**} \
    {fluff **(const base * const)} \
    - \
    -
add {base::operator int} \
    {int (const base * const)} \
    - \
    -
add {base::operator fluff const* const*} \
    {const fluff * const *(const base * const)} \
    - \
    -

# Templates
add {tclass<char>::do_something} \
    {void (tclass<char> * const)} \
    - \
    -
add {tclass<int>::do_something} \
    {void (tclass<int> * const)} \
    - \
    -
add {tclass<long>::do_something} \
    {void (tclass<long> * const)} \
    - \
    -
add {tclass<short>::do_something} \
    {void (tclass<short> * const)} \
    - \
    -
add {tclass<base>::do_something} \
    {void (tclass<base> * const)} \
    - \
    -
add {flubber<int, int, int, int, int>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, int, short>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, int, long>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, int, char>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, short, int>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, short, short>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, short, long>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, short, char>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, long, int>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, long, short>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, long, long>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, long, char>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, char, int>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, char, short>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, char, long>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, int, char, char>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, short, int, int>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, short, int, short>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, short, int, long>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, short, int, char>} \
    {void (void)} \
    - \
    flubber
add {flubber<int, int, short, short, int>} \
    {void (void)} \
    - \
    flubber
add {flubber<short, int, short, int, short>} \
    {void (void)} \
    - \
    flubber
add {flubber<long, short, long, short, long>} \
    {void (void)} \
    - \
    flubber
add {tclass<base>::do_something} \
    {void (tclass<base> * const)} \
    - \
    {tclass<T>::do_something}
add {policy1::policy} \
    [ctor "policy<int, operation_1<void*> >" "int"] \
    {policy<int, operation_1<void*> >::policy} \
    {policy<T, Policy>::policy}
add {policy2::policy} \
    [ctor "policy<int, operation_2<void*> >" int] \
    {policy<int, operation_2<void*> >::policy} \
    {policy<T, Policy>::policy}
add {policy3::policy} \
    [ctor "policy<int, operation_3<void*> >" "int"] \
    {policy<int, operation_3<void*> >::policy} \
    {policy<T, Policy>::policy}
add {policy4::policy} \
    [ctor "policy<int, operation_4<void*> >" "int"] \
    {policy<int, operation_4<void*> >::policy} \
    {policy<T, Policy>::policy}
add {policy1::function} \
    {void (void)} \
    {operation_1<void*>::function} \
    {operation_1<T>::function}
add {policy2::function} \
    {void (void)} \
    {operation_2<void*>::function} \
    {operation_2<T>::function}
add {policy3::function} \
    {void (void)} \
    {operation_3<void*>::function} \
    {operation_3<T>::function}
add {policy4::function} \
    {void (void)} \
    {operation_4<void*>::function} \
    {operation_4<T>::function}
add {policyd<int, operation_1<int> >::policyd} \
    [ctor "policyd<int, operation_1<int> >" "int"] \
    - \
    {policyd<T, Policy>::policyd}
add {policyd1::policyd} \
    [ctor "policyd<int, operation_1<int> >" "int"] \
    {policyd<int, operation_1<int> >::policyd} \
    {policyd<T, Policy>::policyd}
add {policyd<int, operation_1<int> >::~policyd} \
    [dtor "policyd<int, operation_1<int> >"] \
    - \
    {policyd<T, Policy>::~policyd}
add {policyd1::~policyd} \
    [dtor "policyd<int, operation_1<int> >"] \
    {policyd<int, operation_1<int> >::~policyd} \
    {policyd<T, Policy>::~policyd}
add {policyd<long, operation_1<long> >::policyd} \
    [ctor "policyd<long, operation_1<long> >" "long"] \
    - \
    {policyd<T, Policy>::policyd}
add {policyd2::policyd} \
    [ctor "policyd<long, operation_1<long> >" "long"] \
    {policyd<long, operation_1<long> >::policyd} \
    {policyd<T, Policy>::policyd}
add {policyd<long, operation_1<long> >::~policyd} \
    [dtor "policyd<long, operation_1<long> >"] \
    - \
    {policyd<T, Policy>::~policyd}
add {policyd2::~policyd} \
    [dtor "policyd<long, operation_1<long> >"] \
    {policyd<long, operation_1<long> >::~policyd} \
    {policyd<T, Policy>::~policyd}
add {policyd<char, operation_1<char> >::policyd} \
    [ctor "policyd<char, operation_1<char> >" "char"] \
    - \
    {policyd<T, Policy>::policyd}
add {policyd3::policyd} \
    [ctor "policyd<char, operation_1<char> >" "char"] \
    {policyd<char, operation_1<char> >::policyd} \
    {policyd<T, Policy>::policyd}
add {policyd<char, operation_1<char> >::~policyd} \
    [dtor "policyd<char, operation_1<char> >"] \
    - \
    {policyd<T, Policy>::~policyd}
add {policyd3::~policyd} \
    [dtor "policyd<char, operation_1<char> >"] \
    {policyd<char, operation_1<char> >::~policyd} \
    {policyd<T, Policy>::~policyd}
add {policyd<base, operation_1<base> >::policyd} \
    [ctor "policyd<base, operation_1<base> >" "base"] \
    - \
    {policyd<T, Policy>::policyd}
add {policyd4::policyd} \
    [ctor "policyd<base, operation_1<base> >" "base"] \
    {policyd<base, operation_1<base> >::policyd} \
    {policyd<T, Policy>::policyd}
add {policyd<base, operation_1<base> >::~policyd} \
    [dtor "policyd<base, operation_1<base> >"] \
    - \
    {policyd<T, Policy>::~policyd}
add {policyd4::~policyd} \
    [dtor "policyd<base, operation_1<base> >"] \
    {policyd<base, operation_1<base> >::~policyd} \
    {policyd<T, Policy>::~policyd}
add {policyd<tclass<int>, operation_1<tclass<int> > >::policyd} \
    [ctor "policyd<tclass<int>, operation_1<tclass<int> > >" "tclass<int>"] \
    - \
    {policyd<T, Policy>::policyd}
add {policyd5::policyd} \
    [ctor "policyd<tclass<int>, operation_1<tclass<int> > >" "tclass<int>"] \
    {policyd<tclass<int>, operation_1<tclass<int> > >::policyd} \
    {policyd<T, Policy>::policyd}
add {policyd<tclass<int>, operation_1<tclass<int> > >::~policyd} \
    [dtor "policyd<tclass<int>, operation_1<tclass<int> > >"] \
    - \
    {policyd<T, Policy>::~policyd}
add {policyd5::~policyd} \
    [dtor "policyd<tclass<int>, operation_1<tclass<int> > >"] \
    {policyd<tclass<int>, operation_1<tclass<int> > >::~policyd} \
    {policyd<T, Policy>::~policyd}
add {policyd<int, operation_1<int> >::function} \
    {void (void)} \
    {operation_1<int>::function}\
    {operation_1<T>::function}
add {policyd1::function} \
    {void (void)} \
    {operation_1<int>::function} \
    {operation_1<T>::function}
add {policyd2::function} \
    {void (void)} \
    {operation_1<long>::function} \
    {operation_1<T>::function}
add {policyd<char, operation_1<char> >::function} \
    {void (void)} \
    {operation_1<char>::function} \
    {operation_1<T>::function}
add {policyd3::function} \
    {void (void)} \
    {operation_1<char>::function} \
    {operation_1<T>::function}
add {policyd<base, operation_1<base> >::function} \
    {void (void)} \
    {operation_1<base>::function} \
    {operation_1<T>::function}
add {policyd4::function} \
    {void (void)} \
    {operation_1<base>::function} \
    {operation_1<T>::function}
add {policyd<tclass<int>, operation_1<tclass<int> > >::function} \
    {void (void)} \
    {operation_1<tclass<int> >::function} \
    {operation_1<T>::function}
add {policyd5::function} \
    {void (void)} \
    {operation_1<tclass<int> >::function} \
    {operation_1<T>::function}

# Start the test
if {![allow_cplus_tests]} { continue }

#
# test running programs
#

standard_testfile cpexprs.cc

# Include required flags.
set flags "$flags debug c++"

if {[prepare_for_testing "failed to prepare" $testfile $srcfile "$flags"]} {
    return -1
}

if {![runto_main]} {
    perror "couldn't run to breakpoint"
    continue
}

# Set the listsize to one. This will help with testing "list".
gdb_test "set listsize 1"

# "print METHOD"
foreach name [get_functions print] {
    gdb_test "print $name" [get $name print]
}

# "list METHOD"
foreach name [get_functions list] {
    gdb_test "list $name" [get $name list]
}

# Running to breakpoint -- use any function we can "list"
foreach name [get_functions list] {
    # Skip "test_function", since test_breakpoint uses it
    if {[string compare $name "test_function"] != 0} {
	test_breakpoint $name
    }
}

# Test c/v gets recognized even without quoting.
foreach cv {{} { const} { volatile} { const volatile}} {
  set test "p 'CV::m(int)$cv'"
  set correct dummy_value

  gdb_test_multiple $test $test {
      -re "( = {.*} 0x\[0-9a-f\]+ <CV::m.*>)\r\n$gdb_prompt $" {
	  # = {void (CV * const, CV::t)} 0x400944 <CV::m(int)>
	  set correct $expect_out(1,string)
	  pass $test
      }
  }
  gdb_test "p CV::m(int)$cv" [string_to_regexp $correct]
}

# Test TYPENAME:: gets recognized even in parentheses.
gdb_test "p CV_f(int)"   { = {int \(int\)} 0x[0-9a-f]+ <CV_f\(int\)>}
gdb_test "p CV_f(CV::t)" { = {int \(int\)} 0x[0-9a-f]+ <CV_f\(int\)>}
gdb_test "p CV_f(CV::i)" " = 43"

gdb_test "p CV_f('cpexprs.cc'::CV::t)" \
    { = {int \(int\)} 0x[0-9a-f]+ <CV_f\(int\)>}

# Make sure conversion operator names are canonicalized and properly
# "spelled."
gdb_test "p base::operator const fluff * const *" \
    [get "base::operator fluff const* const*" print] \
    "canonicalized conversion operator name 1"
gdb_test "p base::operator const fluff* const*" \
    [get "base::operator fluff const* const*" print] \
    "canonicalized conversion operator name 2"
gdb_test "p base::operator derived*" \
    "There is no field named operator derived\\*" \
    "undefined conversion operator"

gdb_exit
return 0
