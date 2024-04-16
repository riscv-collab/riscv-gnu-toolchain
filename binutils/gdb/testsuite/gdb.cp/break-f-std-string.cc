/* This test file is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <string>

#if _GLIBCXX_USE_CXX11_ABI == 1
#if defined (__GNUC__) && (__GNUC__ >= 5) && (__GNUC__ <= 8)

/* Work around missing std::string typedef before gcc commit
   "Define std::string and related typedefs outside __cxx11 namespace".  */

namespace std {
typedef __cxx11::string string;
}

#endif
#endif

void
f (std::string s)
{
}
