# Copyright 2014-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It test the xmethods support
# in the Python extension language.

import gdb
import re

from gdb.xmethod import XMethod
from gdb.xmethod import XMethodMatcher, XMethodWorker
from gdb.xmethod import SimpleXMethodMatcher


def A_plus_A(obj, opr):
    print("From Python <A_plus_A>:")
    return obj["a"] + opr["a"]


def plus_plus_A(obj):
    print("From Python <plus_plus_A>:")
    return obj["a"] + 1


def A_geta(obj):
    print("From Python <A_geta>:")
    return obj["a"]


def A_getarray(obj):
    print("From Python <A_getarray>:")
    return obj["array"]


def A_getarrayind(obj, index):
    print("From Python <A_getarrayind>:")
    return obj["array"][index]


def A_indexoper(obj, index):
    return obj["array"][index].reference_value()


def B_getarray(obj):
    print("From Python <B_getarray>:")
    return obj["array"].const_value()


def B_indexoper(obj, index):
    return obj["array"][index].const_value().reference_value()


type_A = gdb.parse_and_eval("(dop::A *) 0").type.target()
type_B = gdb.parse_and_eval("(dop::B *) 0").type.target()
type_array = gdb.parse_and_eval("(int[10] *) 0").type.target()
type_int = gdb.parse_and_eval("(int *) 0").type.target()


# The E class matcher and worker test two things:
#   1. xmethod returning None.
#   2. Matcher returning a list of workers.


class E_method_char_worker(XMethodWorker):
    def __init__(self):
        pass

    def get_arg_types(self):
        return gdb.lookup_type("char")

    def get_result_type(self, obj, arg):
        return gdb.lookup_type("void")

    def __call__(self, obj, arg):
        print("From Python <E_method_char>")
        return None


class E_method_int_worker(XMethodWorker):
    def __init__(self):
        pass

    def get_arg_types(self):
        return gdb.lookup_type("int")

    # Note: get_result_type method elided on purpose

    def __call__(self, obj, arg):
        print("From Python <E_method_int>")
        return None


class E_method_matcher(XMethodMatcher):
    def __init__(self):
        XMethodMatcher.__init__(self, "E_methods")
        self.methods = [XMethod("method_int"), XMethod("method_char")]

    def match(self, class_type, method_name):
        class_tag = class_type.unqualified().tag
        if not re.match("^dop::E$", class_tag):
            return None
        if not re.match("^method$", method_name):
            return None
        workers = []
        if self.methods[0].enabled:
            workers.append(E_method_int_worker())
        if self.methods[1].enabled:
            workers.append(E_method_char_worker())
        return workers


# The G class method matcher and worker illustrate how to write
# xmethod matchers and workers for template classes and template
# methods.


class G_size_diff_worker(XMethodWorker):
    def __init__(self, class_template_type, method_template_type):
        self._class_template_type = class_template_type
        self._method_template_type = method_template_type

    def get_arg_types(self):
        pass

    def __call__(self, obj):
        print("From Python G<>::size_diff()")
        return self._method_template_type.sizeof - self._class_template_type.sizeof


class G_size_mul_worker(XMethodWorker):
    def __init__(self, class_template_type, method_template_val):
        self._class_template_type = class_template_type
        self._method_template_val = method_template_val

    def get_arg_types(self):
        pass

    def __call__(self, obj):
        print("From Python G<>::size_mul()")
        return self._class_template_type.sizeof * self._method_template_val


class G_mul_worker(XMethodWorker):
    def __init__(self, class_template_type, method_template_type):
        self._class_template_type = class_template_type
        self._method_template_type = method_template_type

    def get_arg_types(self):
        return self._method_template_type

    def __call__(self, obj, arg):
        print("From Python G<>::mul()")
        return obj["t"] * arg


class G_methods_matcher(XMethodMatcher):
    def __init__(self):
        XMethodMatcher.__init__(self, "G_methods")
        self.methods = [XMethod("size_diff"), XMethod("size_mul"), XMethod("mul")]

    def _is_enabled(self, name):
        for method in self.methods:
            if method.name == name and method.enabled:
                return True

    def match(self, class_type, method_name):
        class_tag = class_type.unqualified().tag
        if not re.match("^dop::G<[ ]*[_a-zA-Z][ _a-zA-Z0-9]*>$", class_tag):
            return None
        t_name = class_tag[7:-1]
        try:
            t_type = gdb.lookup_type(t_name)
        except gdb.error:
            return None
        if re.match("^size_diff<[ ]*[_a-zA-Z][ _a-zA-Z0-9]*>$", method_name):
            if not self._is_enabled("size_diff"):
                return None
            t1_name = method_name[10:-1]
            try:
                t1_type = gdb.lookup_type(t1_name)
                return G_size_diff_worker(t_type, t1_type)
            except gdb.error:
                return None
        if re.match("^size_mul<[ ]*[0-9]+[ ]*>$", method_name):
            if not self._is_enabled("size_mul"):
                return None
            m_val = int(method_name[9:-1])
            return G_size_mul_worker(t_type, m_val)
        if re.match("^mul<[ ]*[_a-zA-Z][ _a-zA-Z0-9]*>$", method_name):
            if not self._is_enabled("mul"):
                return None
            t1_name = method_name[4:-1]
            try:
                t1_type = gdb.lookup_type(t1_name)
                return G_mul_worker(t_type, t1_type)
            except gdb.error:
                return None


global_dm_list = [
    SimpleXMethodMatcher(
        r"A_plus_A",
        r"^dop::A$",
        r"operator\+",
        A_plus_A,
        # This is a replacement, hence match the arg type
        # exactly!
        type_A.const().reference(),
    ),
    SimpleXMethodMatcher(r"plus_plus_A", r"^dop::A$", r"operator\+\+", plus_plus_A),
    SimpleXMethodMatcher(r"A_geta", r"^dop::A$", r"^geta$", A_geta),
    SimpleXMethodMatcher(
        r"A_getarray", r"^dop::A$", r"^getarray$", A_getarray, type_array
    ),
    SimpleXMethodMatcher(
        r"A_getarrayind", r"^dop::A$", r"^getarrayind$", A_getarrayind, type_int
    ),
    SimpleXMethodMatcher(
        r"A_indexoper", r"^dop::A$", r"operator\[\]", A_indexoper, type_int
    ),
    SimpleXMethodMatcher(
        r"B_getarray", r"^dop::B$", r"^getarray$", B_getarray, type_array
    ),
    SimpleXMethodMatcher(
        r"B_indexoper", r"^dop::B$", r"operator\[\]", B_indexoper, type_int
    ),
]

for matcher in global_dm_list:
    gdb.xmethod.register_xmethod_matcher(gdb, matcher)
gdb.xmethod.register_xmethod_matcher(gdb.current_progspace(), G_methods_matcher())
gdb.xmethod.register_xmethod_matcher(gdb.current_progspace(), E_method_matcher())
