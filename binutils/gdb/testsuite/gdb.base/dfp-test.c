/* This testcase is part of GDB, the GNU debugger.

   Copyright 2007-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <stdlib.h>

#define DELTA (0.0001df)
#define DELTA_B (0.001)

double double_val1 = 45.125;
double double_val2 = -67.75;
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

_Decimal32 dec32_val1 = 3.14159df;
_Decimal32 dec32_val2 = -2.3765df;
_Decimal32 dec32_val3 = 0.2df;
_Decimal32 dec32_val4 = 1.2df;
_Decimal32 dec32_val5 = 2.2df;
_Decimal32 dec32_val6 = 3.2df;
_Decimal32 dec32_val7 = 4.2df;
_Decimal32 dec32_val8 = 5.2df;
_Decimal32 dec32_val9 = 6.2df;
_Decimal32 dec32_val10 = 7.2df;
_Decimal32 dec32_val11 = 8.2df;
_Decimal32 dec32_val12 = 9.2df;
_Decimal32 dec32_val13 = 10.2df;
_Decimal32 dec32_val14 = 11.2df;
_Decimal32 dec32_val15 = 12.2df;
_Decimal32 dec32_val16 = 13.2df;

_Decimal64 dec64_val1 = 3.14159dd;
_Decimal64 dec64_val2 = -2.3765dd;
_Decimal64 dec64_val3 = 0.2dd;
_Decimal64 dec64_val4 = 1.2dd;
_Decimal64 dec64_val5 = 2.2dd;
_Decimal64 dec64_val6 = 3.2dd;
_Decimal64 dec64_val7 = 4.2dd;
_Decimal64 dec64_val8 = 5.2dd;
_Decimal64 dec64_val9 = 6.2dd;
_Decimal64 dec64_val10 = 7.2dd;
_Decimal64 dec64_val11 = 8.2dd;
_Decimal64 dec64_val12 = 9.2dd;
_Decimal64 dec64_val13 = 10.2dd;
_Decimal64 dec64_val14 = 11.2dd;
_Decimal64 dec64_val15 = 12.2dd;
_Decimal64 dec64_val16 = 13.2dd;

_Decimal128 dec128_val1 = 3.14159dl;
_Decimal128 dec128_val2 = -2.3765dl;
_Decimal128 dec128_val3 = 0.2dl;
_Decimal128 dec128_val4 = 1.2dl;
_Decimal128 dec128_val5 = 2.2dl;
_Decimal128 dec128_val6 = 3.2dl;
_Decimal128 dec128_val7 = 4.2dl;
_Decimal128 dec128_val8 = 5.2dl;
_Decimal128 dec128_val9 = 6.2dl;
_Decimal128 dec128_val10 = 7.2dl;
_Decimal128 dec128_val11 = 8.2dl;
_Decimal128 dec128_val12 = 9.2dl;
_Decimal128 dec128_val13 = 10.2dl;
_Decimal128 dec128_val14 = 11.2dl;
_Decimal128 dec128_val15 = 12.2dl;
_Decimal128 dec128_val16 = 13.2dl;

volatile _Decimal32 d32;
volatile _Decimal64 d64;
volatile _Decimal128 d128;

/* Typedefs and typedefs of typedefs, for ptype/whatis testing.  */
typedef _Decimal32 d32_t;
typedef _Decimal64 d64_t;
typedef _Decimal128 d128_t;

typedef d32_t d32_t2;
typedef d64_t d64_t2;
typedef d128_t d128_t2;

d32_t v_d32_t;
d64_t v_d64_t;
d128_t v_d128_t;

d32_t2 v_d32_t2;
d64_t2 v_d64_t2;
d128_t2 v_d128_t2;

struct decstruct
{
  int int4;
  long long8;
  float float4;
  double double8;
  _Decimal32 dec32;
  _Decimal64 dec64;
  _Decimal128 dec128;
} ds;

static _Decimal32
arg0_32 (_Decimal32 arg0, _Decimal32 arg1, _Decimal32 arg2,
         _Decimal32 arg3, _Decimal32 arg4, _Decimal32 arg5)
{
  return arg0;
}

static _Decimal64
arg0_64 (_Decimal64 arg0, _Decimal64 arg1, _Decimal64 arg2,
         _Decimal64 arg3, _Decimal64 arg4, _Decimal64 arg5)
{
  return arg0;
}

static _Decimal128
arg0_128 (_Decimal128 arg0, _Decimal128 arg1, _Decimal128 arg2,
         _Decimal128 arg3, _Decimal128 arg4, _Decimal128 arg5)
{
  return arg0;
}

/* Function to test if _Decimal128 argument interferes with stack slots
   because of alignment.  */
int
decimal_dec128_align (double arg0, _Decimal128 arg1, double arg2, double arg3,
		      double arg4, double arg5, double arg6, double arg7,
		      double arg8, double arg9, double arg10, double arg11,
		      double arg12, double arg13)
{
  return ((arg0 - double_val1) < DELTA_B
	  && (arg0 - double_val1) > -DELTA_B
	  && (arg1 - dec128_val2) < DELTA
	  && (arg1 - dec128_val2) > -DELTA
	  && (arg2 - double_val3) < DELTA_B
	  && (arg2 - double_val3) > -DELTA_B
	  && (arg3 - double_val4) < DELTA_B
	  && (arg3 - double_val4) > -DELTA_B
	  && (arg4 - double_val5) < DELTA_B
	  && (arg4 - double_val5) > -DELTA_B
	  && (arg5 - double_val6) < DELTA_B
	  && (arg5 - double_val6) > -DELTA_B
	  && (arg6 - double_val7) < DELTA_B
	  && (arg6 - double_val7) > -DELTA_B
	  && (arg7 - double_val8) < DELTA_B
	  && (arg7 - double_val8) > -DELTA_B
	  && (arg8 - double_val9) < DELTA_B
	  && (arg8 - double_val9) > -DELTA_B
	  && (arg9 - double_val10) < DELTA_B
	  && (arg9 - double_val10) > -DELTA_B
	  && (arg10 - double_val11) < DELTA_B
	  && (arg10 - double_val11) > -DELTA_B
	  && (arg11 - double_val12) < DELTA_B
	  && (arg11 - double_val12) > -DELTA_B
	  && (arg12 - double_val13) < DELTA_B
	  && (arg12 - double_val13) > -DELTA_B
	  && (arg13 - double_val14) < DELTA_B
	  && (arg13 - double_val14) > -DELTA_B);
}

int
decimal_mixed (_Decimal32 arg0, _Decimal64 arg1, _Decimal128 arg2)
{
  return ((arg0 - dec32_val1) < DELTA
	  && (arg0 - dec32_val1) > -DELTA
	  && (arg1 - dec64_val1) < DELTA
	  && (arg1 - dec64_val1) > -DELTA
	  && (arg2 - dec128_val1) < DELTA
	  && (arg2 - dec128_val1) > -DELTA);
}

/* These functions have many arguments to force some of them to be passed via
   the stack instead of registers, to test that GDB can construct correctly
   the parameter save area. Note that Linux/ppc32 has 8 float registers to use
   for float parameter passing and Linux/ppc64 has 13, so the number of
   arguments has to be at least 14 to contemplate these platforms.  */

int
decimal_many_args_dec32 (_Decimal32 f1, _Decimal32 f2, _Decimal32 f3,
			    _Decimal32 f4, _Decimal32 f5, _Decimal32 f6,
			    _Decimal32 f7, _Decimal32 f8, _Decimal32 f9,
			    _Decimal32 f10, _Decimal32 f11, _Decimal32 f12,
			    _Decimal32 f13, _Decimal32 f14, _Decimal32 f15,
			    _Decimal32 f16)
{
  _Decimal32 sum_args;
  _Decimal32 sum_values;

  sum_args = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12
	     + f13 + f14 + f15 + f16;
  sum_values = dec32_val1 + dec32_val2 + dec32_val3 + dec32_val4 + dec32_val5
	       + dec32_val6 + dec32_val7 + dec32_val8 + dec32_val9
	       + dec32_val10 + dec32_val11 + dec32_val12 + dec32_val13
	       + dec32_val14 + dec32_val15 + dec32_val16;

  return ((sum_args - sum_values) < DELTA
	  && (sum_args - sum_values) > -DELTA);
}

int
decimal_many_args_dec64 (_Decimal64 f1, _Decimal64 f2, _Decimal64 f3,
			    _Decimal64 f4, _Decimal64 f5, _Decimal64 f6,
			    _Decimal64 f7, _Decimal64 f8, _Decimal64 f9,
			    _Decimal64 f10, _Decimal64 f11, _Decimal64 f12,
			    _Decimal64 f13, _Decimal64 f14, _Decimal64 f15,
			    _Decimal64 f16)
{
  _Decimal64 sum_args;
  _Decimal64 sum_values;

  sum_args = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12
	     + f13 + f14 + f15 + f16;
  sum_values = dec64_val1 + dec64_val2 + dec64_val3 + dec64_val4 + dec64_val5
	       + dec64_val6 + dec64_val7 + dec64_val8 + dec64_val9
	       + dec64_val10 + dec64_val11 + dec64_val12 + dec64_val13
	       + dec64_val14 + dec64_val15 + dec64_val16;

  return ((sum_args - sum_values) < DELTA
	  && (sum_args - sum_values) > -DELTA);
}

int
decimal_many_args_dec128 (_Decimal128 f1, _Decimal128 f2, _Decimal128 f3,
			    _Decimal128 f4, _Decimal128 f5, _Decimal128 f6,
			    _Decimal128 f7, _Decimal128 f8, _Decimal128 f9,
			    _Decimal128 f10, _Decimal128 f11, _Decimal128 f12,
			    _Decimal128 f13, _Decimal128 f14, _Decimal128 f15,
			    _Decimal128 f16)
{
  _Decimal128 sum_args;
  _Decimal128 sum_values;

  sum_args = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12
	     + f13 + f14 + f15 + f16;
  sum_values = dec128_val1 + dec128_val2 + dec128_val3 + dec128_val4 + dec128_val5
	       + dec128_val6 + dec128_val7 + dec128_val8 + dec128_val9
	       + dec128_val10 + dec128_val11 + dec128_val12 + dec128_val13
	       + dec128_val14 + dec128_val15 + dec128_val16;

  return ((sum_args - sum_values) < DELTA
	  && (sum_args - sum_values) > -DELTA);
}

int
decimal_many_args_mixed (_Decimal32 f1, _Decimal32 f2, _Decimal32 f3,
			   _Decimal64 f4, _Decimal64 f5, _Decimal64 f6,
			   _Decimal64 f7, _Decimal128 f8, _Decimal128 f9,
			   _Decimal128 f10, _Decimal32 f11, _Decimal64 f12,
			   _Decimal32 f13, _Decimal64 f14, _Decimal128 f15)
{
  _Decimal128 sum_args;
  _Decimal128 sum_values;

  sum_args = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9 + f10 + f11 + f12
	     + f13 + f14 + f15;
  sum_values = dec32_val1 + dec32_val2 + dec32_val3 + dec64_val4 + dec64_val5
	       + dec64_val6 + dec64_val7 + dec128_val8 + dec128_val9
	       + dec128_val10 + dec32_val11 + dec64_val12 + dec32_val13
	       + dec64_val14 + dec128_val15;

  return ((sum_args - sum_values) < DELTA
	  && (sum_args - sum_values) > -DELTA);
}

int main()
{
  /* An finite 32-bits decimal floating point.  */
  d32 = 1.2345df;		/* Initialize d32.  */

  /* Non-finite 32-bits decimal floating point: infinity and NaN.  */
  d32 = __builtin_infd32();	/* Positive infd32.  */
  d32 = -__builtin_infd32();	/* Negative infd32.  */
  d32 = __builtin_nand32("");

  /* An finite 64-bits decimal floating point.  */
  d64 = 1.2345dd;		/* Initialize d64.  */

  /* Non-finite 64-bits decimal floating point: infinity and NaN.  */
  d64 = __builtin_infd64();	/* Positive infd64.  */
  d64 = -__builtin_infd64();	/* Negative infd64.  */
  d64 = __builtin_nand64("");

  /* An finite 128-bits decimal floating point.  */
  d128 = 1.2345dl;		/* Initialize d128.  */

  /* Non-finite 128-bits decimal floating point: infinity and NaN.  */
  d128 = __builtin_infd128();	/* Positive infd128.  */
  d128 = -__builtin_infd128();	/* Negative infd128.  */
  d128 = __builtin_nand128("");

  /* Functions with decimal floating point as parameter and return value. */
  d32 = arg0_32 (0.1df, 1.0df, 2.0df, 3.0df, 4.0df, 5.0df);
  d64 = arg0_64 (0.1dd, 1.0dd, 2.0dd, 3.0dd, 4.0dd, 5.0dd);
  d128 = arg0_128 (0.1dl, 1.0dl, 2.0dl, 3.0dl, 4.0dl, 5.0dl);

  ds.int4 = 1;
  ds.long8 = 2;
  ds.float4 = 3.1;
  ds.double8 = 4.2;
  ds.dec32 = 1.2345df;
  ds.dec64 = 1.2345dd;
  ds.dec128 = 1.2345dl;

  return 0;	/* Exit point.  */
}
