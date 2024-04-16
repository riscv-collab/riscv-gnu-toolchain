/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#include <cstddef>

/* Code for operator() tests.  */

struct test_unique_op_call
{
  void operator() (int);
};

void
test_unique_op_call::operator() (int)
{}

struct test_op_call
{
  void operator() ();
  void operator() (int);
  void operator() (long);

  template<typename T>
  void operator() (T *);
};

void
test_op_call::operator() (int)
{}

void
test_op_call::operator() ()
{}

void
test_op_call::operator() (long)
{}

template<typename T>
void
test_op_call::operator() (T *t)
{
}

/* Code for operator[] tests.  */

struct test_unique_op_array
{
  void operator[] (int);
};

void
test_unique_op_array::operator[] (int)
{}

struct test_op_array
{
  void operator[] (int);
  void operator[] (long);

  template<typename T>
  void operator[] (T *);
};

void
test_op_array::operator[] (int)
{}

void
test_op_array::operator[] (long)
{}

template<typename T>
void
test_op_array::operator[] (T *t)
{}

/* Code for operator new tests.  */

static int dummy;

struct test_op_new
{
  void *operator new (size_t);
};

void *
test_op_new::operator new (size_t)
{
  return &dummy;
}

/* Code for operator delete tests.  */

struct test_op_delete
{
  void operator delete (void *);
};

void
test_op_delete::operator delete (void *)
{
}

/* Code for operator new[] tests.  */

struct test_op_new_array
{
  void *operator new[] (size_t);
};

void *
test_op_new_array::operator new[] (size_t)
{
  return &dummy;
}

/* Code for operator delete[] tests.  */

struct test_op_delete_array
{
  void operator delete[] (void *);
};

void
test_op_delete_array::operator delete[] (void *)
{
}

/* Code for user-defined conversion tests.  */

struct test_op_conversion_res;

struct test_op_conversion
{
  operator const volatile test_op_conversion_res **() const volatile;
};

test_op_conversion::operator const volatile test_op_conversion_res **()
  const volatile
{
  return NULL;
}

/* Code for the assignment operator tests.  */

struct test_op_assign
{
  test_op_assign operator= (const test_op_assign &);
};

test_op_assign
test_op_assign::operator= (const test_op_assign &)
{
  return test_op_assign ();
}

/* Code for the arrow operator tests.  */

struct test_op_arrow
{
  test_op_arrow operator-> ();
};

test_op_arrow
test_op_arrow::operator-> ()
{
  return test_op_arrow ();
}

/* Code for the logical/arithmetic operators tests.  */

struct E
{
};

#define GEN_OP(NS, ...)				\
  namespace test_ops {				\
    void operator __VA_ARGS__ {}		\
  }						\
  namespace test_op_ ## NS {			\
    void operator __VA_ARGS__ {}		\
  }

GEN_OP (PLUS_A,    +=  (E, E)  )
GEN_OP (PLUS,      +   (E, E)  )
GEN_OP (MINUS_A,   -=  (E, E)  )
GEN_OP (MINUS,     -   (E, E)  )
GEN_OP (MOD_A,     %=  (E, E)  )
GEN_OP (MOD,       %   (E, E)  )
GEN_OP (EQ,        ==  (E, E)  )
GEN_OP (NEQ,       !=  (E, E)  )
GEN_OP (LAND,      &&  (E, E)  )
GEN_OP (LOR,       ||  (E, E)  )
GEN_OP (SL_A,      <<= (E, E)  )
GEN_OP (SR_A,      >>= (E, E)  )
GEN_OP (SL,        <<  (E, E)  )
GEN_OP (SR,        >>  (E, E)  )
GEN_OP (OE,        |=  (E, E)  )
GEN_OP (BIT_O,     |   (E, E)  )
GEN_OP (XOR_A,     ^=  (E, E)  )
GEN_OP (XOR,       ^   (E, E)  )
GEN_OP (BIT_AND_A, &=  (E, E)  )
GEN_OP (BIT_AND,   &   (E, E)  )
GEN_OP (LT,        <   (E, E)  )
GEN_OP (LTE,       <=  (E, E)  )
GEN_OP (GTE,       >=  (E, E)  )
GEN_OP (GT,        >   (E, E)  )
GEN_OP (MUL_A,     *=  (E, E)  )
GEN_OP (MUL,       *   (E, E)  )
GEN_OP (DIV_A,     /=  (E, E)  )
GEN_OP (DIV,       /   (E, E)  )
GEN_OP (NEG,       ~   (E)      )
GEN_OP (NOT,       !   (E)      )
GEN_OP (PRE_INC,   ++  (E)      )
GEN_OP (POST_INC,  ++  (E, int) )
GEN_OP (PRE_DEC,   --  (E)      )
GEN_OP (POST_DEC,  --  (E, int) )
GEN_OP (COMMA,     ,   (E, E)  )

int
main ()
{
  test_op_call opcall;
  opcall ();
  opcall (1);
  opcall (1l);
  opcall ((int *) 0);

  test_unique_op_call opcall2;
  opcall2 (1);

  test_op_array op_array;
  op_array[1];
  op_array[1l];
  op_array[(int *) 0];

  test_unique_op_array unique_op_array;
  unique_op_array[1];

  return 0;
}
