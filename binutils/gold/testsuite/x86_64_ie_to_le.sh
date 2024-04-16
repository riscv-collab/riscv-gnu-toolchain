#!/bin/sh

# x86_64_ie_to_le.sh -- a test for IE -> LE conversion.

# Copyright (C) 2023-2024 Free Software Foundation, Inc.

# This file is part of gold.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.

set -e

grep -q "add[ \t]\+\$0x[a-f0-9]\+,%r12" x86_64_ie_to_le.stdout
grep -q "mov[ \t]\+\$0x[a-f0-9]\+,%rax" x86_64_ie_to_le.stdout
grep -q "add[ \t]\+\$0x[a-f0-9]\+,%r16" x86_64_ie_to_le.stdout
grep -q "mov[ \t]\+\$0x[a-f0-9]\+,%r20" x86_64_ie_to_le.stdout
