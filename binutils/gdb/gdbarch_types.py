# Architecture commands for GDB, the GNU debugger.
#
# Copyright (C) 1998-2024 Free Software Foundation, Inc.
#
# This file is part of GDB.
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

from typing import List, Optional, Tuple, Union


def join_type_and_name(t: str, n: str):
    "Combine the type T and the name N into a C declaration."
    if t.endswith("*") or t.endswith("&"):
        return t + n
    else:
        return t + " " + n


def join_params(params: List[Tuple[str, str]]):
    """Given a sequence of (TYPE, NAME) pairs, generate a comma-separated
    list of declarations."""
    return ", ".join([join_type_and_name(p[0], p[1]) for p in params])


class Component:
    "Base class for all components."

    def __init__(
        self,
        name: str,
        type: str,
        printer: Optional[str] = None,
        comment: Optional[str] = None,
        predicate: bool = False,
        predefault: Optional[str] = None,
        postdefault: Optional[str] = None,
        invalid: Union[bool, str] = True,
        params: Optional[List[Tuple[str, str]]] = None,
        param_checks: Optional[List[str]] = None,
        result_checks: Optional[List[str]] = None,
        implement: bool = True,
    ):
        self.name = name
        self.type = type
        self.printer = printer
        self.comment = comment
        self.predicate = predicate
        self.predefault = predefault
        self.postdefault = postdefault
        self.invalid = invalid
        self.params = params or []
        self.param_checks = param_checks
        self.result_checks = result_checks
        self.implement = implement

        components.append(self)

        # It doesn't make sense to have a check of the result value
        # for a function or method with void return type.
        if self.type == "void" and self.result_checks:
            raise Exception("can't have result checks with a void return type")

    def get_predicate(self):
        "Return the expression used for validity checking."
        if self.predefault:
            predicate = f"gdbarch->{self.name} != {self.predefault}"
        else:
            predicate = f"gdbarch->{self.name} != NULL"
        return predicate


class Info(Component):
    "An Info component is copied from the gdbarch_info."


class Value(Component):
    "A Value component is just a data member."

    def __init__(
        self,
        *,
        name: str,
        type: str,
        comment: Optional[str] = None,
        predicate: bool = False,
        predefault: Optional[str] = None,
        postdefault: Optional[str] = None,
        invalid: Union[bool, str] = True,
        printer: Optional[str] = None,
    ):
        super().__init__(
            comment=comment,
            name=name,
            type=type,
            predicate=predicate,
            predefault=predefault,
            postdefault=postdefault,
            invalid=invalid,
            printer=printer,
        )


class Function(Component):
    "A Function component is a function pointer member."

    def __init__(
        self,
        *,
        name: str,
        type: str,
        params: List[Tuple[str, str]],
        comment: Optional[str] = None,
        predicate: bool = False,
        predefault: Optional[str] = None,
        postdefault: Optional[str] = None,
        invalid: Union[bool, str] = True,
        printer: Optional[str] = None,
        param_checks: Optional[List[str]] = None,
        result_checks: Optional[List[str]] = None,
        implement: bool = True,
    ):
        super().__init__(
            comment=comment,
            name=name,
            type=type,
            predicate=predicate,
            predefault=predefault,
            postdefault=postdefault,
            invalid=invalid,
            printer=printer,
            params=params,
            param_checks=param_checks,
            result_checks=result_checks,
            implement=implement,
        )

    def ftype(self):
        "Return the name of the function typedef to use."
        return f"gdbarch_{self.name}_ftype"

    def param_list(self):
        "Return the formal parameter list as a string."
        return join_params(self.params)

    def set_list(self):
        """Return the formal parameter list of the caller function,
        as a string.  This list includes the gdbarch."""
        arch_arg = ("struct gdbarch *", "gdbarch")
        arch_tuple = [arch_arg]
        return join_params(arch_tuple + list(self.params))

    def actuals(self):
        "Return the actual parameters to forward, as a string."
        return ", ".join([p[1] for p in self.params])


class Method(Function):
    "A Method is like a Function but passes the gdbarch through."

    def param_list(self):
        "See superclass."
        return self.set_list()

    def actuals(self):
        "See superclass."
        result = ["gdbarch"] + [p[1] for p in self.params]
        return ", ".join(result)


# All the components created in gdbarch-components.py.
components: List[Component] = []
