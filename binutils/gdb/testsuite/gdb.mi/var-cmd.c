/* Copyright 1999-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include <stdlib.h>
#include <string.h>

struct _simple_struct {
  int integer;
  unsigned int unsigned_integer;
  char character;
  signed char signed_character;
  char *char_ptr;
  int array_of_10[10];
};

typedef struct _simple_struct simpleton;

simpleton global_simple;

enum foo {
  bar = 1,
  baz
};

typedef enum foo efoo;

union named_union
{
  int integer;
  char *char_ptr;
};

typedef struct _struct_decl {
  int   integer;
  char  character;
  char *char_ptr;
  long  long_int;
  int  **int_ptr_ptr;
  long  long_array[10];

  void (*func_ptr) (void);
  struct _struct_decl (*func_ptr_struct) (int, char *, long);
  struct _struct_decl *(*func_ptr_ptr) (int, char *, long);
  union {
    int   a;
    char *b;
    long  c;
    enum foo d;
  } u1;

  struct {
    union {
      struct {
        int d;
        char e[10];
        int *(*func) (void);
        efoo foo;
      } u1s1;

      long f;
      struct {
        char array_ptr[2];
        int (*func) (int, char *);
      } u1s2;
    } u2;

    int g;
    char h;
    long i[10];
  } s2;
} weird_struct;

struct _struct_n_pointer {
  char ****char_ptr;
  long ****long_ptr;
  struct _struct_n_pointer *ptrs[3];
  struct _struct_n_pointer *next;
};

struct anonymous {
  int a;
  struct {
    int b;
    char *c;
    union {
      int d;
      void *e;
      char f;
      struct {
	char g;
	const char **h;
	simpleton ***simple;
      };
    };
  };
};

void do_locals_tests (void);
void do_block_tests (void);
void subroutine1 (int, long *);
void nothing (void);
void do_children_tests (void);
void do_special_tests (void);
void incr_a (char);

void incr_a (char a)
{
  int b;
  b = a;
}

int array[] = {1,2,3};
int array2[] = {4,5,6};
int *array_ptr = array;

void
do_locals_tests ()
{
  int linteger = 0;
  int *lpinteger = 0;
  char lcharacter[2] = { 0, 0 };
  char *lpcharacter = 0;
  long llong = 0;
  long *lplong = 0;
  float lfloat = 0;
  float *lpfloat = 0;
  double ldouble = 0;
  double *lpdouble = 0;
  struct _simple_struct lsimple = { 0 };
  struct _simple_struct *lpsimple = 0;
  void (*func) (void) = 0;

  /* Simple assignments */
  linteger = 1234;
  lpinteger = &linteger;
  lcharacter[0] = 'a';
  lpcharacter = lcharacter;
  llong = 2121L;
  lplong = &llong;
  lfloat = 2.1;
  lpfloat = &lfloat;
  ldouble = 2.718281828459045;
  lpdouble = &ldouble;
  lsimple.integer = 1234;
  lsimple.unsigned_integer = 255;
  lsimple.character = 'a';
  lsimple.signed_character = 21;
  lsimple.char_ptr = lcharacter;
  lpsimple = &lsimple;
  func = nothing;

  /* Check pointers */
  linteger = 4321;
  lcharacter[0] = 'b';
  llong = 1212L;
  lfloat = 1.2;
  ldouble = 5.498548281828172;
  lsimple.integer = 255;
  lsimple.unsigned_integer = 4321;
  lsimple.character = 'b';
  lsimple.signed_character = 0;

  subroutine1 (linteger, &llong);
}

void
nothing ()
{
}

struct _struct_decl
nothing1 (int a, char *b, long c)
{
  struct _struct_decl foo;

  return foo;
}

struct _struct_decl *
nothing2 (int a, char *b, long c)
{
  return (struct _struct_decl *) 0;
}

void
subroutine1 (int i, long *l)
{
  global_simple.integer = i + 3;
  i = 212;
  *l = 12;
}

void
do_block_tests ()
{
  int cb = 0;
  cb = 12;

  {
    int foo;
    foo = 123;
    {
      int foo2;
      foo2 = 123;
      {
        int foo;
        foo = 321;
      }
      foo2 = 0;
    }
    foo = 0;
  }

  cb = 21;
}

void
do_children_tests (void)
{
  weird_struct *weird;
  struct _struct_n_pointer *psnp;
  struct _struct_n_pointer snp0, snp1, snp2;
  char a0[2] = {}, *a1, **a2, ***a3;
  char b0[2] = {}, *b1, **b2, ***b3;
  char c0[2] = {}, *c1, **c2, ***c3;
  long z0, *z1, **z2, ***z3;
  long y0, *y1, **y2, ***y3;
  long x0, *x1, **x2, ***x3;
  int *foo;
  int bar;

  /* Avoid pointing into NULL, as that is editable on some
     systems.  */
  int dummy;
  int *dummy_ptr = &dummy;

  struct _struct_decl struct_declarations = { 0, 0, NULL, 0, &dummy_ptr };
  weird = &struct_declarations;

  struct_declarations.integer = 123;
  weird->char_ptr = "hello";
  bar = 2121;
  foo = &bar;
  struct_declarations.int_ptr_ptr = &foo;
  weird->long_array[0] = 1234;
  struct_declarations.long_array[1] = 2345;
  weird->long_array[2] = 3456;
  struct_declarations.long_array[3] = 4567;
  weird->long_array[4] = 5678;
  struct_declarations.long_array[5] = 6789;
  weird->long_array[6] = 7890;
  struct_declarations.long_array[7] = 8901;
  weird->long_array[8] = 9012;
  struct_declarations.long_array[9] = 1234;

  weird->func_ptr = nothing;
  weird->func_ptr_struct = nothing1;
  weird->func_ptr_ptr = nothing2;

  /* Struct/pointer/array tests */
  a0[0] = '0';
  a1 = a0;
  a2 = &a1;
  a3 = &a2;
  b0[0] = '1';
  b1 = b0;
  b2 = &b1;
  b3 = &b2;
  c0[0] = '2';
  c1 = c0;
  c2 = &c1;
  c3 = &c2;
  z0 = 0xdead + 0;
  z1 = &z0;
  z2 = &z1;
  z3 = &z2;
  y0 = 0xdead + 1;
  y1 = &y0;
  y2 = &y1;
  y3 = &y2;
  x0 = 0xdead + 2;
  x1 = &x0;
  x2 = &x1;
  x3 = &x2;
  snp0.char_ptr = &a3;
  snp0.long_ptr = &z3;
  snp0.ptrs[0] = &snp0;
  snp0.ptrs[1] = &snp1;
  snp0.ptrs[2] = &snp2;
  snp0.next = &snp1;
  snp1.char_ptr = &b3;
  snp1.long_ptr = &y3;
  snp1.ptrs[0] = &snp0;
  snp1.ptrs[1] = &snp1;
  snp1.ptrs[2] = &snp2;
  snp1.next = &snp2;
  snp2.char_ptr = &c3;
  snp2.long_ptr = &x3;
  snp2.ptrs[0] = &snp0;
  snp2.ptrs[1] = &snp1;
  snp2.ptrs[2] = &snp2;
  snp2.next = 0x0;
  psnp = &snp0;
  snp0.char_ptr = &b3;
  snp1.char_ptr = &c3;
  snp2.char_ptr = &a3;
  snp0.long_ptr = &y3;
  snp1.long_ptr = &x3;
  snp2.long_ptr = &z3;
  {int a = 0;}
}

void
do_special_tests (void)
{
  union named_union u;
  union {
    int a;
    char b;
    long c;
  } anonu;
  struct _simple_struct s;
  struct {
    int a;
    char b;
    long c;
  } anons;
  enum foo e;
  enum { A, B, C } anone;
  int array[21];
  int a;

  a = 1;   
  u.integer = a;
  anonu.a = a;
  s.integer = a;
  anons.a = a;
  e = bar;
  anone = A;
  incr_a(2);
}

void do_frozen_tests ()
{
  /*: BEGIN: frozen :*/
  struct {
    int i;
    struct {
      int j;
      int k;
    } nested;
  } v1 = {1, {2, 3}};

  int v2 = 4;
  /*: 
    with_test_prefix "create varobj V1 and V2" {
	mi_create_varobj V1 v1 "create varobj for v1"
	mi_create_varobj V2 v2 "create varobj for v2"

	mi_list_varobj_children "V1" {
	    {"V1.i" "i" "0" "int"}
	    {"V1.nested" "nested" "2" "struct {...}"}
	} "list children of v1"

	mi_list_varobj_children "V1.nested" {
	    {"V1.nested.j" "j" "0" "int"}
	    {"V1.nested.k" "k" "0" "int"}
	} "list children of v1.nested"

	mi_check_varobj_value V1.i 1 "check V1.i: 1"
	mi_check_varobj_value V1.nested.j 2 "check V1.nested.j: 2"
	mi_check_varobj_value V1.nested.k 3 "check V1.nested.k: 3"
	mi_check_varobj_value V2 4 "check V2: 4"
    }
  :*/
  v2 = 5;
  /*: 
    mi_varobj_update * {V2} "update varobjs: V2 changed"
    set_frozen V2 1
  :*/
  v2 = 6;
  /*: 
    mi_varobj_update * {} "update varobjs: nothing changed"
    mi_check_varobj_value V2 5 "check V2: 5"
    mi_varobj_update V2 {V2} "update V2 explicitly"
    mi_check_varobj_value V2 6 "check V2: 6"
  :*/
  v1.i = 7;
  v1.nested.j = 8;
  v1.nested.k = 9;
  /*:
    with_test_prefix "frozen V1" {
	set_frozen V1 1
	mi_varobj_update * {} "update varobjs: nothing changed"
	mi_check_varobj_value V1.i 1 "check V1.i: 1"
	mi_check_varobj_value V1.nested.j 2 "check V1.nested.j: 2"
	mi_check_varobj_value V1.nested.k 3 "check V1.nested.k: 3"
    }
    # Check that explicit update for elements of structures
    # works.
    with_test_prefix "update v1.j" {
	# Update v1.j
	mi_varobj_update V1.nested.j {V1.nested.j} "update V1.nested.j"
	mi_check_varobj_value V1.i 1 "check V1.i: 1"
	mi_check_varobj_value V1.nested.j 8 "check V1.nested.j: 8"
	mi_check_varobj_value V1.nested.k 3 "check V1.nested.k: 3"
    }
    with_test_prefix "update v1.nested" {
	# Update v1.nested, check that children is updated.
	mi_varobj_update V1.nested {V1.nested.k} "update V1.nested"
	mi_check_varobj_value V1.i 1 "check V1.i: 1"
	mi_check_varobj_value V1.nested.j 8 "check V1.nested.j: 8"
	mi_check_varobj_value V1.nested.k 9 "check V1.nested.k: 9"
    }
    with_test_prefix "update v1.i" {
	# Update v1.i
	mi_varobj_update V1.i {V1.i} "update V1.i"
	mi_check_varobj_value V1.i 7 "check V1.i: 7"
    }
  :*/
  v1.i = 10;
  v1.nested.j = 11;
  v1.nested.k = 12;
  /*:
    # Check that unfreeze itself does not updates the values.
    with_test_prefix "unfrozen V1" {
	set_frozen V1 0
	mi_check_varobj_value V1.i 7 "check V1.i: 7"
	mi_check_varobj_value V1.nested.j 8 "check V1.nested.j: 8"
	mi_check_varobj_value V1.nested.k 9 "check V1.nested.k: 9"
	mi_varobj_update V1 {V1.i V1.nested.j V1.nested.k} "update V1"
	mi_check_varobj_value V1.i 10 "check V1.i: 10"
	mi_check_varobj_value V1.nested.j 11 "check V1.nested.j: 11"
	mi_check_varobj_value V1.nested.k 12 "check V1.nested.k: 12"
    }
  :*/    
  
  /*: END: frozen :*/
}

void do_at_tests_callee ()
{
  /* This is a test of wrong DWARF data being assigned to expression.
     The DWARF location expression is bound to symbol when expression
     is parsed.  So, if we create floating varobj in one function,
     and then try to reevaluate it in other frame without reparsing
     the expression, we will access local variables using DWARF
     location expression from the original frame, and are likely
     to grab wrong symbol.  To reliably reproduce this bug, we need 
     to wrap our variable with a bunch of buffers, so that those
     buffers are accessed instead of the real one.  */
  int buffer1 = 10;
  int buffer2 = 11;
  int buffer3 = 12;
  int i = 7;
  int buffer4 = 13;
  int buffer5 = 14;
  int buffer6 = 15;
  i++;  /* breakpoint inside callee */
  i++;
}

void do_at_tests ()
{
  int x;
  /*: BEGIN: floating :*/
  int i = 10;
  int y = 15;
  /*:
    mi_create_floating_varobj F i "create floating varobj"
    :*/
  i++;
  /*:
    mi_varobj_update F {F} "update F (1)"
    mi_check_varobj_value F 11 "check F (1)"
    :*/
  i++;
  {
    double i = 15;
    /*:
      mi_varobj_update_with_type_change F "double" "0" "update F (2)"
      mi_check_varobj_value F 15 "check F (2)"
      :*/
    i += 2.0;
  }
  {
    float i = 19;
    /*:
      mi_gdb_test "-var-update --all-values F" {.*value="19".*} "update F (--all-values)"
      :*/
    i += 2.0;
  }
  i++;
  /*:
    mi_varobj_update_with_type_change F "int" "0" "update F (3)"
    mi_check_varobj_value F 13 "check F (3)"
    :*/
  i++;
  do_at_tests_callee ();
  i++;
  /*: END: floating :*/
}

/* Some header appear to define uint already, so apply some
   uglification.  Note that without uglification, the compile
   does not fail, rather, we don't test what we want because
   something else calls check_typedef on 'uint' already.  */
typedef unsigned int uint_for_mi_testing;

struct Data {
  int alloc;
  uint_for_mi_testing sharable : 4;
};

/* Accessing a value of a bitfield whose type is a typed used to
   result in division by zero.  See:

         http://sourceware.org/bugzilla/show_bug.cgi?id=10884

   This tests for this bug.  */

void do_bitfield_tests ()
{
  /*: BEGIN: bitfield :*/
  struct Data d = {0, 3};
  /*:
    mi_create_varobj V d "create varobj for Data"
    mi_list_varobj_children "V" {
        {"V.alloc" "alloc" "0" "int"}
        {"V.sharable" "sharable" "0" "uint_for_mi_testing"}
    } "list children of Data"
    mi_check_varobj_value V.sharable 3 "access bitfield"
    :*/
  return;
  /*: END: bitfield :*/  
}

void
do_anonymous_type_tests (void)
{
  struct anonymous *anon;
  struct anonymous **ptr;
  struct
  {
    int x;
    struct
    {
      int a;
    };
    struct
    {
      int b;
    };
  } v = {1, {2}, {3}};

  anon = (struct anonymous *) malloc (sizeof (struct anonymous));
  anon->a = 1;
  anon->b = 2;
  anon->c = (char *) 3;
  anon->d = 4;
  anon->g = '5';
  anon->h = (const char **) 6;
  anon->simple = (simpleton ***) 7;

  ptr = &anon;
  free (anon);
  return; /* anonymous type tests breakpoint */
}

void
do_nested_struct_union_tests (void)
{
  struct s_a
  {
    int a;
  };
  struct s_b
  {
    int b;
  };
  union u_ab
  {
    struct s_a a;
    struct s_b b;
  };
  struct ss
  {
    struct s_a a1;
    struct s_b b1;
    union u_ab u1;

    /* Anonymous union.  */
    union
    {
      struct s_a a2;
      struct s_b b2;
    };

    union
    {
      struct s_a a3;
      struct s_b b3;
    } u2;
  };

  typedef struct
  {
    int a;
  } td_s_a;

  typedef struct
  {
    int b;
  } td_s_b;

  typedef union
  {
    td_s_a a;
    td_s_b b;
  } td_u_ab;

  struct ss var;
  struct
  {
    td_u_ab ab;
  } var2;

  struct ss *ss_ptr;

  memset (&var, 0, sizeof (var));
  memset (&var2, 0, sizeof (var2));
  ss_ptr = &var;

  return; /* nested struct union tests breakpoint */
}

int
main (int argc, char *argv [])
{
  do_locals_tests ();
  do_block_tests ();
  do_children_tests ();
  do_special_tests ();
  do_frozen_tests ();
  do_at_tests ();
  do_bitfield_tests ();
  do_anonymous_type_tests ();
  do_nested_struct_union_tests ();
  exit (0);
}

  
