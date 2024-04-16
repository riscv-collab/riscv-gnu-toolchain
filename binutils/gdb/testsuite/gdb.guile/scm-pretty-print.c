/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#include <string.h>

struct s
{
  int a;
  int *b;
};

struct ss
{
  struct s a;
  struct s b;
};

struct arraystruct
{
  int y;
  struct s x[2];
};

struct ns {
  const char *null_str;
  int length;
};

struct lazystring {
  const char *lazy_str;
};

struct hint_error {
  int x;
};

struct children_as_list {
  int x;
};

#ifdef __cplusplus
struct S : public s {
  int zs;
};

struct SS {
  int zss;
  S s;
};

struct SSS
{
  SSS (int x, const S& r);
  int a;
  const S &b;
};
SSS::SSS (int x, const S& r) : a(x), b(r) { }

class VirtualTest 
{ 
 private: 
  int value; 

 public: 
  VirtualTest () 
    { 
      value = 1;
    } 
};

class Vbase1 : public virtual VirtualTest { };
class Vbase2 : public virtual VirtualTest { };
class Vbase3 : public virtual VirtualTest { };

class Derived : public Vbase1, public Vbase2, public Vbase3
{ 
 private: 
  int value; 
  
 public:
  Derived () 
    { 
      value = 2; 
    }
};

class Fake
{
  int sname;
  
 public:
  Fake (const int name = 0):
  sname (name)
  {
  }
};
#endif

struct substruct {
  int a;
  int b;
};

struct outerstruct {
  struct substruct s;
  int x;
};

struct outerstruct
substruct_test (void)
{
  struct outerstruct outer;
  outer.s.a = 0;
  outer.s.b = 0;
  outer.x = 0;

  outer.s.a = 3;		/* MI outer breakpoint here */

  return outer;  
}

typedef struct string_repr
{
  struct whybother
  {
    const char *contents;
  } whybother;
} string;

/* This lets us avoid malloc.  */
int array[100];
int narray[10];

struct justchildren
{
  int len;
  int *elements;
};

typedef struct justchildren nostring_type;

struct memory_error
{
  const char *s;
};

struct container
{
  string name;
  int len;
  int *elements;
};

typedef struct container zzz_type;

string
make_string (const char *s)
{
  string result;
  result.whybother.contents = s;
  return result;
}

zzz_type
make_container (const char *s)
{
  zzz_type result;

  result.name = make_string (s);
  result.len = 0;
  result.elements = 0;

  return result;
}

void
add_item (zzz_type *c, int val)
{
  if (c->len == 0)
    c->elements = array;
  c->elements[c->len] = val;
  ++c->len;
}

void
set_item(zzz_type *c, int i, int val)
{
  if (i < c->len)
    c->elements[i] = val;
}

void init_s(struct s *s, int a)
{
  s->a = a;
  s->b = &s->a;
}

void init_ss(struct ss *s, int a, int b)
{
  init_s(&s->a, a);
  init_s(&s->b, b);
}

void do_nothing(void)
{
  int c;

  c = 23;			/* Another MI breakpoint */
}

struct nullstr
{
  char *s;
};

struct string_repr string_1 = { { "one" } };
struct string_repr string_2 = { { "two" } };

static int __attribute__ ((used))
eval_func (int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8)
{
  return p1;
}

static void
eval_sub (void)
{
  struct eval_type_s { int x; } eval1 = { 1 }, eval2 = { 2 }, eval3 = { 3 },
				eval4 = { 4 }, eval5 = { 5 }, eval6 = { 6 },
				eval7 = { 7 }, eval8 = { 8 }, eval9 = { 9 };

  eval1.x++; /* eval-break */
}

static void
bug_14741()
{
  zzz_type c = make_container ("bug_14741");
  add_item (&c, 71);
  set_item(&c, 0, 42); /* breakpoint bug 14741 */
  set_item(&c, 0, 5);
}

int
main ()
{
  struct ss  ss;
  struct ss  ssa[2];
  struct arraystruct arraystruct;
  string x = make_string ("this is x");
  zzz_type c = make_container ("container");
  zzz_type c2 = make_container ("container2");
  const struct string_repr cstring = { { "const string" } };
  /* Clearing by being `static' could invoke an other GDB C++ bug.  */
  struct nullstr nullstr;
  nostring_type nstype, nstype2;
  struct memory_error me;
  struct ns ns, ns2;
  struct lazystring estring, estring2;
  struct hint_error hint_error;
  struct children_as_list children_as_list;

  nstype.elements = narray;
  nstype.len = 0;

  me.s = "blah";

  init_ss(&ss, 1, 2);
  init_ss(ssa+0, 3, 4);
  init_ss(ssa+1, 5, 6);
  memset (&nullstr, 0, sizeof nullstr);

  arraystruct.y = 7;
  init_s (&arraystruct.x[0], 23);
  init_s (&arraystruct.x[1], 24);

  ns.null_str = "embedded\0null\0string";
  ns.length = 20;

  /* Make a "corrupted" string.  */
  ns2.null_str = NULL;
  ns2.length = 20;

  estring.lazy_str = "embedded x\201\202\203\204" ;

  /* Incomplete UTF-8, but ok Latin-1.  */
  estring2.lazy_str = "embedded x\302";

#ifdef __cplusplus
  S cps;

  cps.zs = 7;
  init_s(&cps, 8);

  SS cpss;
  cpss.zss = 9;
  init_s(&cpss.s, 10);

  SS cpssa[2];
  cpssa[0].zss = 11;
  init_s(&cpssa[0].s, 12);
  cpssa[1].zss = 13;
  init_s(&cpssa[1].s, 14);

  SSS sss(15, cps);

  SSS& ref (sss);

  Derived derived;
  
  Fake fake (42);
#endif

  add_item (&c, 23);		/* MI breakpoint here */
  add_item (&c, 72);

#ifdef MI
  add_item (&c, 1011);
  c.elements[0] = 1023;
  c.elements[0] = 2323;

  add_item (&c2, 2222);
  add_item (&c2, 3333);

  substruct_test ();
  do_nothing ();
#endif

  nstype.elements[0] = 7;
  nstype.elements[1] = 42;
  nstype.len = 2;
  
  nstype2 = nstype;

  eval_sub ();

  bug_14741();      /* break to inspect struct and union */
  return 0;
}
