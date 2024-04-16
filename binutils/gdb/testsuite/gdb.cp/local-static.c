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

struct aggregate
{
  int i1;
  int i2;
  int i3;
};

void keepalive_float (double *var) { }
void keepalive_int (int *var) { }
void keepalive_aggregate (struct aggregate *var) { }

#define PREFIXIFY(PREFIX, VAR)			\
  PREFIX ## _ ## VAR

#define DEF_STATICS(PREFIX)						\
  static int PREFIXIFY(PREFIX, s_var_int) = 4;				\
  static double PREFIXIFY(PREFIX, s_var_float) = 3.14f;			\
  static struct aggregate PREFIXIFY(PREFIX, s_var_aggregate)		\
    = { 1, 2, 3 };							\
									\
  keepalive_int (&PREFIXIFY(PREFIX, s_var_int));			\
  keepalive_float (&PREFIXIFY(PREFIX, s_var_float));			\
  keepalive_aggregate (&PREFIXIFY(PREFIX, s_var_aggregate));

#ifdef __cplusplus

struct S
{
  void inline_method ()
  {
    DEF_STATICS (S_IM);
  }
  static void static_inline_method ()
  {
    DEF_STATICS (S_SIM);
  }

  void method ();
  void method () const;
  void method () volatile;
  void method () volatile const;

  static void static_method ();
};

S s;
const S c_s = {};
volatile S v_s = {};
const volatile S cv_s = {};

void
S::method ()
{
  DEF_STATICS (S_M);
}

void
S::method () const
{
  DEF_STATICS (S_M_C);
}

void
S::method () volatile
{
  DEF_STATICS (S_M_V);
}

void
S::method () const volatile
{
  DEF_STATICS (S_M_CV);
}

void
S::static_method ()
{
  DEF_STATICS (S_SM);
}

template <typename T>
struct S2
{
  void method ();
  static void static_method ();

  void inline_method ()
  {
    DEF_STATICS (S2_IM);
  }

  static void static_inline_method ()
  {
    DEF_STATICS (S2_SIM);
  }
};

template<typename T>
void
S2<T>::method ()
{
  DEF_STATICS (S2_M);
}

template<typename T>
void
S2<T>::static_method ()
{
  DEF_STATICS (S2_SM);
}

S2<int> s2;

#endif

void
free_func (void)
{
  DEF_STATICS (FF);
}

static inline void
free_inline_func (void)
{
  DEF_STATICS (FIF);
}

int
main ()
{
  int i;

  for (i = 0; i < 1000; i++)
    {
      free_func ();
      free_inline_func ();

#ifdef __cplusplus
      s.method ();
      c_s.method ();
      v_s.method ();
      cv_s.method ();
      s.inline_method ();
      S::static_method ();
      S::static_inline_method ();

      s2.method ();
      s2.inline_method ();
      S2<int>::static_method ();
      S2<int>::static_inline_method ();
#endif
    }

  return 0;
}
