#!/bin/sh
# Replace $LINENO on the fly.
# Copyright (C) 2023-2024 Free Software Foundation, Inc.
#
# This file is part of the GNU simulators.
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

# Since $LINENO is not reliable in shells/subshells, generate it on the fly.

if [ $# -lt 2 ]; then
  cat <<EOF >&2
Usage: $0 <script> <tempfile> [script args]

Rewrite the $LINENO usage in <script> with the line number.  The temp script is
written to <tempfile>, and then removed when done.
EOF
  exit 1
fi

input=$1
shift
output=$1
shift

${AWK:-awk} '{
  gsub("[$]LINENO", NR + 1)
  gsub("\"[$]0\"", "\"" FILENAME "\"")
  print
}' "${input}" >"${output}"
${SHELL} "${output}" "$@"

rm -f "${output}"
