/* This testcase is part of GDB, the GNU debugger.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Red Hat, originally written by Keith Seitz.  */

#include <stdlib.h>

typedef const char* const* my_type;
typedef int my_type_2;
typedef my_type my_other_type;
typedef my_type_2 my_other_type_2;
typedef unsigned long CORE_ADDR;
typedef enum {E_A, E_B, E_C} anon_enum;
typedef struct {int a; char b;} anon_struct;
typedef union {int a; char b;} anon_union;
typedef anon_enum aenum;
typedef anon_struct astruct;
typedef anon_union aunion;

typedef void (*fptr1) (my_other_type);
typedef void (*fptr2) (fptr1, my_other_type_2);
typedef void (*fptr3) (fptr2, my_other_type);
typedef void (*fptr4) (anon_enum a, anon_struct const& b, anon_union const*** c);

// For c++/24367 testing
typedef struct incomplete_struct incomplete_struct;
typedef struct _incomplete_struct another_incomplete_struct;
int test_incomplete (incomplete_struct *p) { return 0; } // test_incomplete(incomplete_struct*)
int test_incomplete (another_incomplete_struct *p) { return 1; } // test_incomplete(another_incomplete_struct*)
int test_incomplete (int *p) { return -1; } // test_incomplete(int*)

namespace A
{
  class foo
  {
  public:
    foo (void) { }
    foo (my_other_type a) { } // A::FOO::foo(my_other_type)
    foo (my_other_type_2 a) { } // A::FOO::foo(my_other_type_2)
    foo (my_other_type_2 a, const my_other_type b) { } // A::FOO::foo(my_other_type_2, const my_other_type)
    foo (fptr3) { } // A::FOO::foo(fptr3)
    foo (fptr1* a) { } // A::FOO::foo(fptr1*)
    foo (CORE_ADDR (*) [10]) { } // A::FOO::foo(CORE_ADDR (*) [10])
    foo (aenum a, astruct const& b, aunion const*** c) { } // A::FOO::foo(aenum, astruct const&, aunion const***)

    void test (my_other_type a) { } // A::FOO::test(my_other_type)
    void test (my_other_type_2 a) { } // A::FOO::test(my_other_type_2)
    void test (my_other_type_2 a, const my_other_type b) { } // A::FOO::test(my_other_type_2, const my_other_type)
    void test (fptr3 a) { } // A::FOO::test(fptr3)
    void test (fptr1* a) { } // A::FOO::test(fptr1*)
    void test (CORE_ADDR (*) [10]) { } // A::FOO::test(CORE_ADDR (*) [10])
    void test (aenum a, astruct const& b, aunion const*** c) { }; // A::FOO::test(aenum, astruct const&, aunion const***)
  };

  typedef foo FOO;
};

namespace B
{
  void
  test (my_other_type foo) { } // B::test(my_other_type)

  void
  test (aenum a, astruct const& b, aunion const*** c) { } // B::test(aenum, astruct const&, aunion const***)

  template <typename T1, typename T2>
  void test (T1 a, T2 b) { } // B::test (T1, T2)

  template <>
  void test (my_other_type foo, my_other_type_2) { } // B::test<my_other_type, my_other_type_2>(my_other_type, my_other_type_2)
};

namespace a
{
  namespace b
  {
    namespace c
    {
      namespace d
      {
	class bar { };
      }
    }

    typedef c::d::bar BAR;
  }
}

typedef a::b::BAR _BAR_;

template <typename T1, typename T2>
void test (T1 a, T2 b) {} // test (T1, T2)

template <>
void test (my_other_type foo, my_other_type_2) { } // test<my_other_type, my_other_type_2>(my_other_type, my_other_type_2)

void
test (my_other_type foo) { } // test(my_other_type)

void
test (_BAR_ &b) { } // test(_BAR_&)

void
test (aenum a, astruct const& b, aunion const*** c) { } // test(aenum, astruct const&, aunion const***)

int
main (void)
{
  A::FOO my_foo;
  fptr1 fptr;
  astruct as = { 0, 0 };
  aunion const au = { 0 };
  aunion const* aup = &au;
  aunion const** aupp = &aup;
  aunion const*** auppp = &aupp;

  my_foo.test (static_cast<my_other_type> (NULL));
  my_foo.test (0);
  my_foo.test (0, static_cast<my_type> (NULL));
  my_foo.test (static_cast<fptr3> (NULL));
  my_foo.test (&fptr);
  my_foo.test (static_cast<CORE_ADDR (*) [10]> (0));
  my_foo.test (E_A, as, auppp);

  B::test (static_cast<my_other_type> (NULL));
  B::test (static_cast<my_other_type> (NULL), 0);
  B::test (E_A, as, auppp);

  test (static_cast<my_other_type> (NULL));
  test<my_other_type, my_other_type_2> (static_cast<my_other_type> (NULL), 0);
  test (E_A, as, auppp);

  A::foo a (static_cast<my_other_type> (NULL));
  A::foo b (0);
  A::foo c (0, static_cast<my_other_type> (NULL));
  A::foo d (static_cast<fptr3> (NULL));
  A::foo e (&fptr);
  A::foo f (static_cast<CORE_ADDR (*) [10]> (0));
  A::foo g (E_A, as, auppp);

  fptr4 f4;

  // Tests for c++/24367
  int *i = nullptr;
  incomplete_struct *is = nullptr;
  another_incomplete_struct *ais = nullptr;
  int result = (test_incomplete (i) + test_incomplete (is)
		+ test_incomplete (ais));
  return 0;
}
