/* This testcase is part of GDB, the GNU debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

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

/* Test program partial trace data visualization.  */

/* Typedefs.  */

typedef struct TEST_STRUCT {
  char   memberc;
  int    memberi;
  float  memberf;
  double memberd;
} test_struct;

struct small_struct
{
  int member;
};

struct small_struct_b : public small_struct
{
};

typedef int test_array [4];

/* Global variables to be collected.  */

char         globalc;
int          globali;
float        globalf;
double       globald;
test_struct  globalstruct;
test_struct *globalp;
int          globalarr[16];
small_struct g_smallstruct;
small_struct_b g_smallstruct_b;

/* Strings.  */

const char g_const_string[] = "hello world";
char g_string_unavail[sizeof (g_const_string)];
char g_string_partial[sizeof (g_const_string)];
const char *g_string_p;

/* Used to check that <unavailable> is not the same as 0 in array
   element repetitions.  */

struct tuple
{
  int a;
  int b;
};

struct tuple tarray[8];

/* Test for overcollection.  GDB used to merge memory ranges to
   collect if they were close enough --- say, collect `a' and 'c'
   below, and you'd get 'b' as well.  This had been presumably done to
   cater for some target's inefficient trace buffer layout, but it is
   really not GDB's business to assume how the target manages its
   buffer.  If the target wants to overcollect, that's okay, since it
   knows what is and what isn't safe to touch (think memory mapped
   registers), and knows it's buffer layout.

   The test assumes these three variables are laid out consecutively
   in memory.  Unfortunately, we can't use an array instead, since the
   agent expression generator does not even do constant folding,
   meaning that anything that's more complicated than collecting a
   global will generate an agent expression action to evaluate on the
   target, instead of a simple "collect memory" action.  */
int a;
int b;
int c;

/* Random tests.  */

struct StructA
{
  int a, b;
  int array[10000];
  void *ptr;
  int bitfield:1;
};

struct StructB
{
  int d, ef;
  StructA struct_a;
  int s:1;
  static StructA static_struct_a;
  const char *string;
};

/* References.  */

int g_int;
int &g_ref = g_int;

struct StructRef
{
  StructRef (unsigned int val) : ref(d) {}

  void clear ()
  {
    d = 0;
  }

  unsigned int d;
  unsigned int &ref;
};

struct StructB struct_b;
struct StructA StructB::static_struct_a;

StructRef g_structref(0x12345678);
StructRef *g_structref_p = &g_structref;

class Base
{
protected:
  int x;

public:
  Base(void) { x = 2; };
};

class Middle: public virtual Base
{
protected:
  int y;

public:
  Middle(void): Base() { y = 3; };
};

class Derived: public virtual Middle {
protected:
  int z;

public:
  Derived(void): Middle() { z = 4; };
};

Derived derived_unavail;
Derived derived_partial;
Derived derived_whole;

struct Virtual {
  int z;

  virtual ~Virtual() {}
};

Virtual virtual_partial;
Virtual *virtualp = &virtual_partial;

/* Test functions.  */

static void
begin ()	/* called before anything else */
{
}

static void
end ()		/* called after everything else */
{
}

/* Test (not) collecting args.  */

int
args_test_func (char   argc,
		int    argi,
		float  argf,
		double argd,
		test_struct argstruct,
		int argarray[4])
{
  int i;

  i =  (int) argc + argi + argf + argd + argstruct.memberi + argarray[1];

  return i;
}

/* Test (not) collecting array args.  */

/* Test (not) collecting locals.  */

int
local_test_func ()
{
  char        locc  = 11;
  int         loci  = 12;
  float       locf  = 13.3;
  double      locd  = 14.4;
  test_struct locst;
  int         locar[4];
  int         i;
  struct localstruct {} locdefst;

  locst.memberc  = 15;
  locst.memberi  = 16;
  locst.memberf  = 17.7;
  locst.memberd  = 18.8;
  locar[0] = 121;
  locar[1] = 122;
  locar[2] = 123;
  locar[3] = 124;

  i = /* set local_test_func tracepoint here */
    (int) locc + loci + locf + locd + locst.memberi + locar[1];

  return i;
}

/* Test collecting register locals.  */

int
reglocal_test_func ()
{
  register char        locc = 11;
  register int         loci = 12;
  register float       locf = 13.3;
  register double      locd = 14.4;
  register test_struct locst;
  register int         locar[4];
  int                  i;

  locst.memberc  = 15;
  locst.memberi  = 16;
  locst.memberf  = 17.7;
  locst.memberd  = 18.8;
  locar[0] = 121;
  locar[1] = 122;
  locar[2] = 123;
  locar[3] = 124;

  i = /* set reglocal_test_func tracepoint here */
    (int) locc + loci + locf + locd + locst.memberi + locar[1];

  return i;
}

/* Test collecting static locals.  */

int
statlocal_test_func ()
{
  static   char        locc;
  static   int         loci;
  static   float       locf;
  static   double      locd;
  static   test_struct locst;
  static   int         locar[4];
  int                  i;

  locc = 11;
  loci = 12;
  locf = 13.3;
  locd = 14.4;
  locst.memberc = 15;
  locst.memberi = 16;
  locst.memberf = 17.7;
  locst.memberd = 18.8;
  locar[0] = 121;
  locar[1] = 122;
  locar[2] = 123;
  locar[3] = 124;

  i = /* set statlocal_test_func tracepoint here */
    (int) locc + loci + locf + locd + locst.memberi + locar[1];

  /* Set static locals back to zero so collected values are clearly special. */
  locc = 0;
  loci = 0;
  locf = 0;
  locd = 0;
  locst.memberc = 0;
  locst.memberi = 0;
  locst.memberf = 0;
  locst.memberd = 0;
  locar[0] = 0;
  locar[1] = 0;
  locar[2] = 0;
  locar[3] = 0;

  return i;
}

int
globals_test_func ()
{
  int i = 0;

  i += globalc + globali + globalf + globald;
  i += globalstruct.memberc + globalstruct.memberi;
  i += globalstruct.memberf + globalstruct.memberd;
  i += globalarr[1];

  return i;	/* set globals_test_func tracepoint here */
}

int
main (int argc, char **argv, char **envp)
{
  int         i = 0;
  test_struct mystruct;
  int         myarray[4];

  begin ();
  /* Assign collectable values to global variables.  */
  globalc = 71;
  globali = 72;
  globalf = 73.3;
  globald = 74.4;
  globalstruct.memberc = 81;
  globalstruct.memberi = 82;
  globalstruct.memberf = 83.3;
  globalstruct.memberd = 84.4;
  globalp = &globalstruct;

  for (i = 0; i < 15; i++)
    globalarr[i] = i;

  mystruct.memberc = 101;
  mystruct.memberi = 102;
  mystruct.memberf = 103.3;
  mystruct.memberd = 104.4;
  myarray[0] = 111;
  myarray[1] = 112;
  myarray[2] = 113;
  myarray[3] = 114;

  g_int = 123;
  memset (&struct_b, 0xaa, sizeof struct_b);
  memset (&struct_b.static_struct_a, 0xaa, sizeof struct_b.static_struct_a);
  struct_b.string = g_const_string;
  memcpy (g_string_unavail, g_const_string, sizeof (g_const_string));
  memcpy (g_string_partial, g_const_string, sizeof (g_const_string));
  g_string_p = g_const_string;
  a = 1; b = 2; c = 3;

  /* Call test functions, so they can be traced and data collected.  */
  i = 0;
  i += args_test_func (1, 2, 3.3, 4.4, mystruct, myarray);
  i += local_test_func ();
  i += reglocal_test_func ();
  i += statlocal_test_func ();
  i += globals_test_func ();

  /* Set 'em back to zero, so that the collected values will be
     distinctly different from the "realtime" (end of test) values.  */

  globalc = 0;
  globali = 0;
  globalf = 0;
  globald = 0;
  globalstruct.memberc = 0;
  globalstruct.memberi = 0;
  globalstruct.memberf = 0;
  globalstruct.memberd = 0;
  globalp = 0;
  for (i = 0; i < 15; i++)
    globalarr[i] = 0;

  memset (&struct_b, 0, sizeof struct_b);
  memset (&struct_b.static_struct_a, 0, sizeof struct_b.static_struct_a);
  struct_b.string = NULL;
  memset (g_string_unavail, 0, sizeof (g_string_unavail));
  memset (g_string_partial, 0, sizeof (g_string_partial));
  g_string_p = NULL;

  a = b = c = 0;

  g_int = 0;

  g_structref.clear ();
  g_structref_p = NULL;

  end ();
  return 0;
}
