/* This testcase is part of GDB, the GNU debugger.

   Copyright 1993-2024 Free Software Foundation, Inc.

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

/* Support program for testing gdb's ability to call functions
   in the inferior, pass appropriate arguments to those functions,
   and get the returned result. */

#ifdef NO_PROTOTYPES
#define PARAMS(paramlist) ()
#else
#define PARAMS(paramlist) paramlist
#endif

# include <stdlib.h>
# include <string.h>

char char_val1 = 'a';
char char_val2 = 'b';

short short_val1 = 10;
short short_val2 = -23;

int int_val1 = 87;
int int_val2 = -26;

long long_val1 = 789;
long long_val2 = -321;

float float_val1 = 3.14159;
float float_val2 = -2.3765;
float float_val3 = 0.25;
float float_val4 = 1.25;
float float_val5 = 2.25;
float float_val6 = 3.25;
float float_val7 = 4.25;
float float_val8 = 5.25;
float float_val9 = 6.25;
float float_val10 = 7.25;
float float_val11 = 8.25;
float float_val12 = 9.25;
float float_val13 = 10.25;
float float_val14 = 11.25;
float float_val15 = 12.25;

double double_val1 = 45.654;
double double_val2 = -67.66;
double double_val3 = 0.25;
double double_val4 = 1.25;
double double_val5 = 2.25;
double double_val6 = 3.25;
double double_val7 = 4.25;
double double_val8 = 5.25;
double double_val9 = 6.25;
double double_val10 = 7.25;
double double_val11 = 8.25;
double double_val12 = 9.25;
double double_val13 = 10.25;
double double_val14 = 11.25;
double double_val15 = 12.25;

#ifdef TEST_COMPLEX
extern float crealf (float _Complex);
extern float cimagf (float _Complex);
extern double creal (double _Complex);
extern double cimag (double _Complex);
extern long double creall (long double _Complex);
extern long double cimagl (long double _Complex);

float _Complex fc1 = 1.0F + 1.0iF;
float _Complex fc2 = 2.0F + 2.0iF;
float _Complex fc3 = 3.0F + 3.0iF;
float _Complex fc4 = 4.0F + 4.0iF;

double _Complex dc1 = 1.0 + 1.0i;
double _Complex dc2 = 2.0 + 2.0i;
double _Complex dc3 = 3.0 + 3.0i;
double _Complex dc4 = 4.0 + 4.0i;

long double _Complex ldc1 = 1.0L + 1.0Li;
long double _Complex ldc2 = 2.0L + 2.0Li;
long double _Complex ldc3 = 3.0L + 3.0Li;
long double _Complex ldc4 = 4.0L + 4.0Li;
#endif /* TEST_COMPLEX */

#define DELTA (0.001)

char *string_val1 = (char *)"string 1";
char *string_val2 = (char *)"string 2";

char char_array_val1[] = "carray 1";
char char_array_val2[] = "carray 2";

struct struct1 {
  char c;
  short s;
  int i;
  long l;
  float f;
  double d;
  char a[4];
#ifdef TEST_COMPLEX
  float _Complex fc;
  double _Complex dc;
  long double _Complex ldc;
} struct_val1 ={ 'x', 87, 76, 51, 2.1234, 9.876, "foo", 3.0F + 3.0Fi,
		 4.0L + 4.0Li, 5.0L + 5.0Li};
#else
} struct_val1 = { 'x', 87, 76, 51, 2.1234, 9.876, "foo" };
#endif /* TEST_COMPLEX */
/* Some functions that can be passed as arguments to other test
   functions, or called directly. */
#ifdef PROTOTYPES
int add (int a, int b)
#else
int add (a, b) int a, b;
#endif
{
  return (a + b);
}

#ifdef PROTOTYPES
int doubleit (int a)
#else
int doubleit (a) int a;
#endif
{
  return (a + a);
}

int (*func_val1) PARAMS((int,int)) = add;
int (*func_val2) PARAMS((int)) = doubleit;

/* An enumeration and functions that test for specific values. */

enum enumtype { enumval1, enumval2, enumval3 };
enum enumtype enum_val1 = enumval1;
enum enumtype enum_val2 = enumval2;
enum enumtype enum_val3 = enumval3;

#ifdef PROTOTYPES
int t_enum_value1 (enum enumtype enum_arg)
#else
int t_enum_value1 (enum_arg) enum enumtype enum_arg;
#endif
{
  return (enum_arg == enum_val1);
}

#ifdef PROTOTYPES
int t_enum_value2 (enum enumtype enum_arg)
#else
int t_enum_value2 (enum_arg) enum enumtype enum_arg;
#endif
{
  return (enum_arg == enum_val2);
}

#ifdef PROTOTYPES
int t_enum_value3 (enum enumtype enum_arg)
#else
int t_enum_value3 (enum_arg) enum enumtype enum_arg;
#endif
{
  return (enum_arg == enum_val3);
}

/* A function that takes a vector of integers (along with an explicit
   count) and returns their sum. */

#ifdef PROTOTYPES
int sum_args (int argc, int argv[])
#else
int sum_args (argc, argv) int argc; int argv[];
#endif
{
  int sumval = 0;
  int idx;

  for (idx = 0; idx < argc; idx++)
    {
      sumval += argv[idx];
    }
  return (sumval);
}

/* Test that we can call functions that take structs and return
   members from that struct */

#ifdef PROTOTYPES
char   t_structs_c (struct struct1 tstruct) { return (tstruct.c); }
short  t_structs_s (struct struct1 tstruct) { return (tstruct.s); }
int    t_structs_i (struct struct1 tstruct) { return (tstruct.i); }
long   t_structs_l (struct struct1 tstruct) { return (tstruct.l); }
float  t_structs_f (struct struct1 tstruct) { return (tstruct.f); }
double t_structs_d (struct struct1 tstruct) { return (tstruct.d); }
char  *t_structs_a (struct struct1 tstruct)
{
  static char buf[8];
  strcpy (buf, tstruct.a);
  return buf;
}
#ifdef TEST_COMPLEX
float _Complex t_structs_fc (struct struct1 tstruct) { return tstruct.fc;}
double _Complex t_structs_dc (struct struct1 tstruct) { return tstruct.dc;}
long double _Complex t_structs_ldc (struct struct1 tstruct) { return tstruct.ldc;}
#endif
#else
char   t_structs_c (tstruct) struct struct1 tstruct; { return (tstruct.c); }
short  t_structs_s (tstruct) struct struct1 tstruct; { return (tstruct.s); }
int    t_structs_i (tstruct) struct struct1 tstruct; { return (tstruct.i); }
long   t_structs_l (tstruct) struct struct1 tstruct; { return (tstruct.l); }
float  t_structs_f (tstruct) struct struct1 tstruct; { return (tstruct.f); }
double t_structs_d (tstruct) struct struct1 tstruct; { return (tstruct.d); }
char  *t_structs_a (tstruct) struct struct1 tstruct;
{
  static char buf[8];
  strcpy (buf, tstruct.a);
  return buf;
}
#ifdef TEST_COMPLEX
float _Complex t_structs_fc (tstruct) struct struct1 tstruct; { return tstruct.fc;}
double _Complex t_structs_dc (tstruct) struct struct1 tstruct; { return tstruct.dc;}
long double _Complex t_structs_ldc (tstruct) struct struct1 tstruct; { return tstruct.ldc;}
#endif
#endif

/* Test that calling functions works if there are a lot of arguments.  */
#ifdef PROTOTYPES
int
sum10 (int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9)
#else
int
sum10 (i0, i1, i2, i3, i4, i5, i6, i7, i8, i9)
     int i0, i1, i2, i3, i4, i5, i6, i7, i8, i9;
#endif
{
  return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
}

/* Test that args are passed in the right order. */
#ifdef PROTOTYPES
int
cmp10 (int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9)
#else
int
cmp10 (i0, i1, i2, i3, i4, i5, i6, i7, i8, i9)
  int i0, i1, i2, i3, i4, i5, i6, i7, i8, i9;
#endif
{
  return
    (i0 == 0) && (i1 == 1) && (i2 == 2) && (i3 == 3) && (i4 == 4) &&
    (i5 == 5) && (i6 == 6) && (i7 == 7) && (i8 == 8) && (i9 == 9);
}

/* Functions that expect specific values to be passed and return 
   either 0 or 1, depending upon whether the values were
   passed incorrectly or correctly, respectively. */

#ifdef PROTOTYPES
int t_char_values (char char_arg1, char char_arg2)
#else
int t_char_values (char_arg1, char_arg2)
char char_arg1, char_arg2;
#endif
{
  return ((char_arg1 == char_val1) && (char_arg2 == char_val2));
}

int
#ifdef PROTOTYPES
t_small_values (char arg1, short arg2, int arg3, char arg4, short arg5,
		char arg6, short arg7, int arg8, short arg9, short arg10)
#else
t_small_values (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
     char arg1;
     short arg2;
     int arg3;
     char arg4;
     short arg5;
     char arg6;
     short arg7;
     int arg8;
     short arg9;
     short arg10;
#endif
{
  return arg1 + arg2 + arg3 + arg4 + arg5 + arg6 + arg7 + arg8 + arg9 + arg10;
}

#ifdef PROTOTYPES
int t_short_values (short short_arg1, short short_arg2)
#else
int t_short_values (short_arg1, short_arg2)
     short short_arg1, short_arg2;
#endif
{
  return ((short_arg1 == short_val1) && (short_arg2 == short_val2));
}

#ifdef PROTOTYPES
int t_int_values (int int_arg1, int int_arg2)
#else
int t_int_values (int_arg1, int_arg2)
int int_arg1, int_arg2;
#endif
{
  return ((int_arg1 == int_val1) && (int_arg2 == int_val2));
}

#ifdef PROTOTYPES
int t_long_values (long long_arg1, long long_arg2)
#else
int t_long_values (long_arg1, long_arg2)
long long_arg1, long_arg2;
#endif
{
  return ((long_arg1 == long_val1) && (long_arg2 == long_val2));
}

/* NOTE: THIS FUNCTION MUST NOT BE PROTOTYPED!!!!!
   There must be one version of "t_float_values" (this one)
   that is not prototyped, and one (if supported) that is (following).
   That way GDB can be tested against both cases.  */
   
int t_float_values (float_arg1, float_arg2)
float float_arg1, float_arg2;
{
  return ((float_arg1 - float_val1) < DELTA
	  && (float_arg1 - float_val1) > -DELTA
	  && (float_arg2 - float_val2) < DELTA
	  && (float_arg2 - float_val2) > -DELTA);
}

int
#ifdef NO_PROTOTYPES
/* In this case we are just duplicating t_float_values, but that is the
   easiest way to deal with either ANSI or non-ANSI.  */
t_float_values2 (float_arg1, float_arg2)
     float float_arg1, float_arg2;
#else
t_float_values2 (float float_arg1, float float_arg2)
#endif
{
  return ((float_arg1 - float_val1) < DELTA
	  && (float_arg1 - float_val1) > -DELTA
	  && (float_arg2 - float_val2) < DELTA
	  && (float_arg2 - float_val2) > -DELTA);
}

/* This function has many arguments to force some of them to be passed via
   the stack instead of registers, to test that GDB can construct correctly
   the parameter save area. Note that Linux/ppc32 has 8 float registers to use
   for float parameter passing and Linux/ppc64 has 13, so the number of
   arguments has to be at least 14 to contemplate these platforms.  */

float
#ifdef NO_PROTOTYPES
t_float_many_args (f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13,
		   f14, f15)
     float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15;
#else
t_float_many_args (float f1, float f2, float f3, float f4, float f5, float f6,
		   float f7, float f8, float f9, float f10, float f11,
		   float f12, float f13, float f14, float f15)
#endif
{
  float sum_args;
  float sum_values;

  sum_args = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12
	     + f13 + f14 + f15;
  sum_values = float_val1 + float_val2 + float_val3 + float_val4 + float_val5
	       + float_val6 + float_val7 + float_val8 + float_val9
	       + float_val10 + float_val11 + float_val12 + float_val13
	       + float_val14 + float_val15;

  return ((sum_args - sum_values) < DELTA
	  && (sum_args - sum_values) > -DELTA);
}

#ifdef PROTOTYPES
int t_double_values (double double_arg1, double double_arg2)
#else
int t_double_values (double_arg1, double_arg2)
double double_arg1, double_arg2;
#endif
{
  return ((double_arg1 - double_val1) < DELTA
	  && (double_arg1 - double_val1) > -DELTA
	  && (double_arg2 - double_val2) < DELTA
	  && (double_arg2 - double_val2) > -DELTA);
}

/* This function has many arguments to force some of them to be passed via
   the stack instead of registers, to test that GDB can construct correctly
   the parameter save area. Note that Linux/ppc32 has 8 float registers to use
   for float parameter passing and Linux/ppc64 has 13, so the number of
   arguments has to be at least 14 to contemplate these platforms.  */

double
#ifdef NO_PROTOTYPES
t_double_many_args (f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13,
		   f14, f15)
     double f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15;
#else
t_double_many_args (double f1, double f2, double f3, double f4, double f5,
		    double f6, double f7, double f8, double f9, double f10,
		    double f11, double f12, double f13, double f14, double f15)
#endif
{
  double sum_args;
  double sum_values;

  sum_args = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12
	     + f13 + f14 + f15;
  sum_values = double_val1 + double_val2 + double_val3 + double_val4
	       + double_val5 + double_val6 + double_val7 + double_val8
	       + double_val9 + double_val10 + double_val11 + double_val12
	       + double_val13 + double_val14 + double_val15;

  return ((sum_args - sum_values) < DELTA
	  && (sum_args - sum_values) > -DELTA);
}

/* Various functions for _Complex types.  */

#ifdef TEST_COMPLEX

#define COMPARE_WITHIN_RANGE(ARG1, ARG2, DEL, FUNC) \
  ((FUNC(ARG1) - FUNC(ARG2)) < DEL) && ((FUNC(ARG1) - FUNC(ARG2) > -DEL))

#define DEF_FUNC_MANY_ARGS_1(TYPE, NAME)			\
t_##NAME##_complex_many_args (f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, \
			      f12, f13, f14, f15, f16)			\
     TYPE _Complex f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, \
     f13, f14, f15, f16;

#define DEF_FUNC_MANY_ARGS_2(TYPE, NAME)		  \
t_##NAME##_complex_many_args (TYPE _Complex f1, TYPE _Complex f2, \
			      TYPE _Complex f3, TYPE _Complex f4, \
			      TYPE _Complex f5, TYPE _Complex f6, \
			      TYPE _Complex f7, TYPE _Complex f8, \
			      TYPE _Complex f9, TYPE _Complex f10,	\
			      TYPE _Complex f11, TYPE _Complex f12,	\
			      TYPE _Complex f13, TYPE _Complex f14,	\
			      TYPE _Complex f15, TYPE _Complex f16)

#define DEF_FUNC_MANY_ARGS_3(TYPE, CREAL, CIMAG) \
{ \
   TYPE _Complex expected = fc1 + fc2 + fc3 + fc4 + fc1 + fc2 + fc3 + fc4 \
    + fc1 + fc2 + fc3 + fc4 + fc1 + fc2 + fc3 + fc4; \
  TYPE _Complex actual = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 \
    + f11 + f12 + f13 + f14 + f15 + f16; \
  return (COMPARE_WITHIN_RANGE(expected, actual, DELTA, creal) \
	  && COMPARE_WITHIN_RANGE(expected, actual, DELTA, creal) \
	  && COMPARE_WITHIN_RANGE(expected, actual, DELTA, cimag)   \
	  && COMPARE_WITHIN_RANGE(expected, actual, DELTA, cimag)); \
}

int
#ifdef NO_PROTOTYPES
DEF_FUNC_MANY_ARGS_1(float, float)
#else
DEF_FUNC_MANY_ARGS_2(float, float)
#endif
DEF_FUNC_MANY_ARGS_3(float, crealf, cimagf)

int
#ifdef NO_PROTOTYPES
DEF_FUNC_MANY_ARGS_1(double, double)
#else
DEF_FUNC_MANY_ARGS_2(double, double)
#endif
DEF_FUNC_MANY_ARGS_3(double, creal, cimag)

int
#ifdef NO_PROTOTYPES
DEF_FUNC_MANY_ARGS_1(long double, long_double)
#else
DEF_FUNC_MANY_ARGS_2(long double, long_double)
#endif
DEF_FUNC_MANY_ARGS_3(long double, creall, cimagl)

#define DEF_FUNC_VALUES_1(TYPE, NAME)			\
  t_##NAME##_complex_values (f1, f2) TYPE _Complex f1, f2;

#define DEF_FUNC_VALUES_2(TYPE, NAME) \
  t_##NAME##_complex_values (TYPE _Complex f1, TYPE _Complex f2)

#define DEF_FUNC_VALUES_3(FORMAL_PARM, TYPE, CREAL, CIMAG)	\
{ \
  return (COMPARE_WITHIN_RANGE(f1, FORMAL_PARM##1, DELTA, CREAL)    \
	  && COMPARE_WITHIN_RANGE(f2, FORMAL_PARM##2, DELTA, CREAL)    \
	  && COMPARE_WITHIN_RANGE(f1,  FORMAL_PARM##1, DELTA, CIMAG)   \
	  && COMPARE_WITHIN_RANGE(f2,  FORMAL_PARM##2, DELTA, CIMAG)); \
}

int
#ifdef NO_PROTOTYPES
DEF_FUNC_VALUES_1(float, float)
#else
DEF_FUNC_VALUES_2(float, float)
#endif
DEF_FUNC_VALUES_3(fc, float, crealf, cimagf)

int
#ifdef NO_PROTOTYPES
DEF_FUNC_VALUES_1(double, double)
#else
DEF_FUNC_VALUES_2(double, double)
#endif
DEF_FUNC_VALUES_3(fc, double, creal, cimag)

int
#ifdef NO_PROTOTYPES
DEF_FUNC_VALUES_1(long double, long_double)
#else
DEF_FUNC_VALUES_2(long double, long_double)
#endif
DEF_FUNC_VALUES_3(fc, long double, creall, cimagl)

#endif /* TEST_COMPLEX */


#ifdef PROTOTYPES
int t_string_values (char *string_arg1, char *string_arg2)
#else
int t_string_values (string_arg1, string_arg2)
char *string_arg1, *string_arg2;
#endif
{
  return (!strcmp (string_arg1, string_val1) &&
	  !strcmp (string_arg2, string_val2));
}

#ifdef PROTOTYPES
int t_char_array_values (char char_array_arg1[], char char_array_arg2[])
#else
int t_char_array_values (char_array_arg1, char_array_arg2)
char char_array_arg1[], char_array_arg2[];
#endif
{
  return (!strcmp (char_array_arg1, char_array_val1) &&
	  !strcmp (char_array_arg2, char_array_val2));
}

#ifdef PROTOTYPES
int t_double_int (double double_arg1, int int_arg2)
#else
int t_double_int (double_arg1, int_arg2)
double double_arg1;
int int_arg2;
#endif
{
  return ((double_arg1 - int_arg2) < DELTA
	  && (double_arg1 - int_arg2) > -DELTA);
}

#ifdef PROTOTYPES
int t_int_double (int int_arg1, double double_arg2)
#else
int t_int_double (int_arg1, double_arg2)
int int_arg1;
double double_arg2;
#endif
{
  return ((int_arg1 - double_arg2) < DELTA
	  && (int_arg1 - double_arg2) > -DELTA);
}

/* This used to simply compare the function pointer arguments with
   known values for func_val1 and func_val2.  Doing so is valid ANSI
   code, but on some machines (RS6000, HPPA, others?) it may fail when
   called directly by GDB.

   In a nutshell, it's not possible for GDB to determine when the address
   of a function or the address of the function's stub/trampoline should
   be passed.

   So, to avoid GDB lossage in the common case, we perform calls through the
   various function pointers and compare the return values.  For the HPPA
   at least, this allows the common case to work.

   If one wants to try something more complicated, pass the address of
   a function accepting a "double" as one of its first 4 arguments.  Call
   that function indirectly through the function pointer.  This would fail
   on the HPPA.  */

#ifdef PROTOTYPES
int t_func_values (int (*func_arg1)(int, int), int (*func_arg2)(int))
#else
int t_func_values (func_arg1, func_arg2)
int (*func_arg1) PARAMS ((int, int));
int (*func_arg2) PARAMS ((int));
#endif
{
  return ((*func_arg1) (5,5)  == (*func_val1) (5,5)
          && (*func_arg2) (6) == (*func_val2) (6));
}

#ifdef PROTOTYPES
int t_call_add (int (*func_arg1)(int, int), int a, int b)
#else
int t_call_add (func_arg1, a, b)
int (*func_arg1) PARAMS ((int, int));
int a, b;
#endif
{
  return ((*func_arg1)(a, b));
}

struct struct_with_fnptr
{
  int (*func) PARAMS((int));
};

struct struct_with_fnptr function_struct = { doubleit };

struct struct_with_fnptr *function_struct_ptr = &function_struct;

int *
voidfunc (void)
{
  static int twentythree = 23;
  return &twentythree;
}

/* Gotta have a main to be able to generate a linked, runnable
   executable, and also provide a useful place to set a breakpoint. */

int main ()
{
  void *p = malloc (1);
  t_double_values(double_val1, double_val2);
  t_structs_c(struct_val1);
  free (p);
  return 0 ;
}

static int
Lcallfunc (int arg)
{
  return arg + 1;
}

int
callfunc (int (*func) (int value), int value)
{
  return Lcallfunc (0) * 0 + func (value) * 2;
}
