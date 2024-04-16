# Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

import gdb


class StringRecognizer(object):
    def __init__(self):
        self.enabled = True

    def recognize(self, type_obj):
        if type_obj.tag == "basic_string":
            return "string"
        return None


class StringTypePrinter(object):
    def __init__(self):
        self.name = "string"
        self.enabled = True

    def instantiate(self):
        return StringRecognizer()


gdb.type_printers.append(StringTypePrinter())


class OtherRecognizer(object):
    def __init__(self):
        self.enabled = True

    def recognize(self, type_obj):
        if type_obj.tag == "Other":
            return "Another"
        return None


class OtherTypePrinter(object):
    def __init__(self):
        self.name = "other"
        self.enabled = True

    def instantiate(self):
        return OtherRecognizer()


import gdb.types

gdb.types.register_type_printer(gdb.objfiles()[0], OtherTypePrinter())
