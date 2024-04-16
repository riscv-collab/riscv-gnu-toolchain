/* Copyright 2003-2024 Free Software Foundation, Inc.

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

namespace C
{
  class OtherFileClass {
  public:
    int z;

    typedef short cOtherFileClassType;
    typedef long cOtherFileClassType2;
    static const cOtherFileClassType cOtherFileClassVar = 318;
    static const cOtherFileClassType2 cOtherFileClassVar2 = 320;
    cOtherFileClassType cOtherFileClassVar_use ();
  };
  OtherFileClass::cOtherFileClassType OtherFileClass::cOtherFileClassVar_use ()
  {
    return cOtherFileClassVar + cOtherFileClassVar2;
  }

  namespace {
    int cXOtherFile = 29;
  };

  int cOtherFile = 316;

  void ensureOtherRefs () {
    // NOTE (2004-04-23, carlton): This function is here only to make
    // sure that GCC 3.4 outputs debug info for this class.
    static OtherFileClass *c = new OtherFileClass();
    c->z = cOtherFile + cXOtherFile;
  }

  typedef short cOtherFileType;
  typedef long cOtherFileType2;
  static const cOtherFileType cOtherFileVar = 319;
  static const cOtherFileType2 cOtherFileVar2 = 321;
  cOtherFileType cOtherFileVar_use ()
  {
    return cOtherFileVar + cOtherFileVar2;
  }
}

namespace {
  int XOtherFile = 317;
}

int ensureOtherRefs ()
{
  C::ensureOtherRefs ();
  return XOtherFile;
}
