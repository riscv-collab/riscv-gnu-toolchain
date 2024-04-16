/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef GM_STD_H
#define GM_STD_H

#include <iostream>

namespace gm_std
{

// Mock std::cerr, so we don't have to worry about the vagaries of the
// system-provided one.  E.g., gcc pr 65669.
// This contains just enough to exercise what we want to.
template<typename T>
  class basic_ostream
{
 public:
  std::ostream *stream;
};

template<typename T>
  basic_ostream<T>&
operator<< (basic_ostream<T>& out, const char* s)
{
  (*out.stream) << s;
  return out;
}

typedef basic_ostream<char> ostream;

// Inhibit implicit instantiations for required instantiations,
// which are defined via explicit instantiations elsewhere.
extern template class basic_ostream<char>;
extern template ostream& operator<< (ostream&, const char*);

extern ostream cerr;

// Call this from main so we don't have to do the same tricks that
// libstcd++ does with ios init'n.
extern void init ();

}

#endif /* GM_STD_H */
