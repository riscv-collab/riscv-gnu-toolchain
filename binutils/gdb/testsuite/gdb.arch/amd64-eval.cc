/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>

/* A simple structure with a single integer field. Should be returned in
   a register.  */
struct SimpleBase
{
  SimpleBase (int32_t x) : x (x) {}

  int32_t x;
};

/* A simple structure derived from the simple base. Should be returned in
   a register.  */
struct SimpleDerived : public SimpleBase
{
  SimpleDerived (int32_t x) : SimpleBase (x) {}
};

/* A structure derived from the simple base with a non-trivial destructor.
   Should be returned on the stack.  */
struct NonTrivialDestructorDerived : public SimpleBase
{
  NonTrivialDestructorDerived (int32_t x) : SimpleBase (x) {}
  ~NonTrivialDestructorDerived() { x = 1; }
};

/* A structure with unaligned fields. Should be returned on the stack.  */
struct UnalignedFields
{
  UnalignedFields (int32_t x, double y) : x (x), y (y) {}

  int32_t x;
  double y;
} __attribute__((packed));

/* A structure with unaligned fields in its base class. Should be
   returned on the stack.  */
struct UnalignedFieldsInBase : public UnalignedFields
{
  UnalignedFieldsInBase (int32_t x, double y, int32_t x2)
  : UnalignedFields (x, y), x2 (x2) {}

  int32_t x2;
};

struct Bitfields
{
  Bitfields(unsigned int x, unsigned int y)
    : fld(x), fld2(y)
  {}

  unsigned fld : 7;
  unsigned fld2 : 7;
};

class Foo
{
public:
  SimpleBase
  return_simple_base (int32_t x)
  {
    assert (this->tag == EXPECTED_TAG);
    return SimpleBase (x);
  }

  SimpleDerived
  return_simple_derived (int32_t x)
  {
    assert (this->tag == EXPECTED_TAG);
    return SimpleDerived (x);
  }

  NonTrivialDestructorDerived
  return_non_trivial_destructor (int32_t x)
  {
    assert (this->tag == EXPECTED_TAG);
    return NonTrivialDestructorDerived (x);
  }

  UnalignedFields
  return_unaligned (int32_t x, double y)
  {
    assert (this->tag == EXPECTED_TAG);
    return UnalignedFields (x, y);
  }

  UnalignedFieldsInBase
  return_unaligned_in_base (int32_t x, double y, int32_t x2)
  {
    assert (this->tag == EXPECTED_TAG);
    return UnalignedFieldsInBase (x, y, x2);
  }

  Bitfields
  return_bitfields (unsigned int x, unsigned int y)
  {
    assert (this->tag == EXPECTED_TAG);
    return Bitfields(x, y);
  }

private:
  /* Use a tag to detect if the "this" value is correct.  */
  static const int EXPECTED_TAG = 0xF00F00F0;
  int tag = EXPECTED_TAG;
};

int
main (int argc, char *argv[])
{
  Foo foo;
  foo.return_simple_base(1);
  foo.return_simple_derived(2);
  foo.return_non_trivial_destructor(3);
  foo.return_unaligned(4, 5);
  foo.return_unaligned_in_base(6, 7, 8);
  foo.return_bitfields(23, 74);
  return 0;  // break-here
}
