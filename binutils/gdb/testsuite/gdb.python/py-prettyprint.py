# Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests python pretty
# printers.

import re
import gdb


def _iterator(pointer, len):
    start = pointer
    end = pointer + len
    while pointer != end:
        yield ("[%d]" % int(pointer - start), pointer.dereference())
        pointer += 1


# Same as _iterator but can be told to raise an exception.
def _iterator_except(pointer, len):
    start = pointer
    end = pointer + len
    while pointer != end:
        if exception_flag:
            raise gdb.MemoryError("hi bob")
        yield ("[%d]" % int(pointer - start), pointer.dereference())
        pointer += 1


# Test returning a Value from a printer.
class string_print(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val["whybother"]["contents"]


# Test a class-based printer.
class ContainerPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "container %s with %d elements" % (self.val["name"], self.val["len"])

    def children(self):
        return _iterator(self.val["elements"], self.val["len"])

    def display_hint(self):
        if self.val["is_map_p"] and self.val["is_array_p"]:
            raise Exception("invalid object state found in display_hint")

        if self.val["is_map_p"]:
            return "map"
        elif self.val["is_array_p"]:
            return "array"
        else:
            return None


# Treats a container as array.
class ArrayPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "array %s with %d elements" % (self.val["name"], self.val["len"])

    def children(self):
        return _iterator(self.val["elements"], self.val["len"])

    def display_hint(self):
        return "array"


# Flag to make NoStringContainerPrinter throw an exception.
exception_flag = False


# Test a printer where to_string is None
class NoStringContainerPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return None

    def children(self):
        return _iterator_except(self.val["elements"], self.val["len"])


# See ToStringReturnsValueWrapper.
class ToStringReturnsValueInner:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "Inner to_string {}".format(int(self.val["val"]))


# Test a printer that returns a gdb.Value in its to_string.  That gdb.Value
# also has its own pretty-printer.
class ToStringReturnsValueWrapper:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val["inner"]


class pp_s(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = self.val["a"]
        b = self.val["b"]
        if a.address != b:
            raise Exception("&a(%s) != b(%s)" % (str(a.address), str(b)))
        return " a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


class pp_ss(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


class pp_sss(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


class pp_multiple_virtual(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "pp value variable is: " + str(self.val["value"])


class pp_vbase1(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "pp class name: " + self.val.type.tag


class pp_nullstr(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val["s"].string(gdb.target_charset())


class pp_ns(object):
    "Print a std::basic_string of some kind"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        len = self.val["length"]
        return self.val["null_str"].string(gdb.target_charset(), length=len)

    def display_hint(self):
        return "string"


pp_ls_encoding = None


class pp_ls(object):
    "Print a std::basic_string of some kind"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        length = self.val["len"]
        if pp_ls_encoding is not None:
            if length >= 0:
                return self.val["lazy_str"].lazy_string(
                    encoding=pp_ls_encoding, length=length
                )
            else:
                return self.val["lazy_str"].lazy_string(encoding=pp_ls_encoding)
        else:
            if length >= 0:
                return self.val["lazy_str"].lazy_string(length=length)
            else:
                return self.val["lazy_str"].lazy_string()

    def display_hint(self):
        return "string"


class pp_hint_error(object):
    "Throw error from display_hint"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "hint_error_val"

    def display_hint(self):
        raise Exception("hint failed")


class pp_children_as_list(object):
    "Throw error from display_hint"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "children_as_list_val"

    def children(self):
        return [("one", 1)]


class pp_outer(object):
    "Print struct outer"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "x = %s" % self.val["x"]

    def children(self):
        yield "s", self.val["s"]
        yield "x", self.val["x"]


class MemoryErrorString(object):
    "Raise an error"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        raise gdb.MemoryError("Cannot access memory.")

    def display_hint(self):
        return "string"


class pp_eval_type(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        gdb.execute("bt", to_string=True)
        return (
            "eval=<"
            + str(gdb.parse_and_eval("eval_func (123456789, 2, 3, 4, 5, 6, 7, 8)"))
            + ">"
        )


class pp_int_typedef(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "type=%s, val=%s" % (self.val.type, int(self.val))


class pp_int_typedef3(object):
    "A printer without a to_string method"

    def __init__(self, val):
        self.val = val

    def children(self):
        yield "s", 27


def lookup_function(val):
    "Look-up and return a pretty-printer that can print val."

    # Get the type.
    type = val.type

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified().strip_typedefs()

    # Get the type name.
    typename = type.tag

    if typename is None:
        return None

    # Iterate over local dictionary of types to determine
    # if a printer is registered for that type.  Return an
    # instantiation of the printer if found.
    for function in pretty_printers_dict:
        if function.match(typename):
            return pretty_printers_dict[function](val)

    # Cannot find a pretty printer.  Return None.

    return None


def disable_lookup_function():
    lookup_function.enabled = False


def enable_lookup_function():
    lookup_function.enabled = True


# Lookup a printer for VAL in the typedefs dict.
def lookup_typedefs_function(val):
    "Look-up and return a pretty-printer that can print val (typedefs)."

    # Get the type.
    type = val.type

    if type is None or type.name is None or type.code != gdb.TYPE_CODE_TYPEDEF:
        return None

    # Iterate over local dictionary of typedef types to determine if a
    # printer is registered for that type.  Return an instantiation of
    # the printer if found.
    for function in typedefs_pretty_printers_dict:
        if function.match(type.name):
            return typedefs_pretty_printers_dict[function](val)

    # Cannot find a pretty printer.
    return None


def register_pretty_printers():
    pretty_printers_dict[re.compile("^struct s$")] = pp_s
    pretty_printers_dict[re.compile("^s$")] = pp_s
    pretty_printers_dict[re.compile("^S$")] = pp_s

    pretty_printers_dict[re.compile("^struct ss$")] = pp_ss
    pretty_printers_dict[re.compile("^ss$")] = pp_ss
    pretty_printers_dict[re.compile("^const S &$")] = pp_s
    pretty_printers_dict[re.compile("^SSS$")] = pp_sss

    pretty_printers_dict[re.compile("^VirtualTest$")] = pp_multiple_virtual
    pretty_printers_dict[re.compile("^Vbase1$")] = pp_vbase1

    pretty_printers_dict[re.compile("^struct nullstr$")] = pp_nullstr
    pretty_printers_dict[re.compile("^nullstr$")] = pp_nullstr

    # Note that we purposely omit the typedef names here.
    # Printer lookup is based on canonical name.
    # However, we do need both tagged and untagged variants, to handle
    # both the C and C++ cases.
    pretty_printers_dict[re.compile("^struct string_repr$")] = string_print
    pretty_printers_dict[re.compile("^struct container$")] = ContainerPrinter
    pretty_printers_dict[re.compile("^struct justchildren$")] = NoStringContainerPrinter
    pretty_printers_dict[re.compile("^string_repr$")] = string_print
    pretty_printers_dict[re.compile("^container$")] = ContainerPrinter
    pretty_printers_dict[re.compile("^justchildren$")] = NoStringContainerPrinter

    pretty_printers_dict[
        re.compile("^struct to_string_returns_value_inner$")
    ] = ToStringReturnsValueInner
    pretty_printers_dict[
        re.compile("^to_string_returns_value_inner$")
    ] = ToStringReturnsValueInner
    pretty_printers_dict[
        re.compile("^struct to_string_returns_value_wrapper$")
    ] = ToStringReturnsValueWrapper
    pretty_printers_dict[
        re.compile("^to_string_returns_value_wrapper$")
    ] = ToStringReturnsValueWrapper

    pretty_printers_dict[re.compile("^struct ns$")] = pp_ns
    pretty_printers_dict[re.compile("^ns$")] = pp_ns

    pretty_printers_dict[re.compile("^struct lazystring$")] = pp_ls
    pretty_printers_dict[re.compile("^lazystring$")] = pp_ls

    pretty_printers_dict[re.compile("^struct outerstruct$")] = pp_outer
    pretty_printers_dict[re.compile("^outerstruct$")] = pp_outer

    pretty_printers_dict[re.compile("^struct hint_error$")] = pp_hint_error
    pretty_printers_dict[re.compile("^hint_error$")] = pp_hint_error

    pretty_printers_dict[re.compile("^struct children_as_list$")] = pp_children_as_list
    pretty_printers_dict[re.compile("^children_as_list$")] = pp_children_as_list

    pretty_printers_dict[re.compile("^memory_error$")] = MemoryErrorString

    pretty_printers_dict[re.compile("^eval_type_s$")] = pp_eval_type

    typedefs_pretty_printers_dict[re.compile("^int_type$")] = pp_int_typedef
    typedefs_pretty_printers_dict[re.compile("^int_type2$")] = pp_int_typedef
    typedefs_pretty_printers_dict[re.compile("^int_type3$")] = pp_int_typedef3


# Dict for struct types with typedefs fully stripped.
pretty_printers_dict = {}
# Dict for typedef types.
typedefs_pretty_printers_dict = {}

register_pretty_printers()
gdb.pretty_printers.append(lookup_function)
gdb.pretty_printers.append(lookup_typedefs_function)
