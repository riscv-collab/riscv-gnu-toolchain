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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

static volatile int v;

static void __attribute__((noinline, noclone))
e (int i, double j)
{
  v = 0;
}

static void __attribute__((noinline, noclone))
d (int i, double j)
{
  i++;
  j++;
  e (i, j);
  e (v, v);
asm ("breakhere:");
  e (v, v);
}

static void __attribute__((noinline, noclone))
locexpr (int i)
{
  i = i;
asm ("breakhere_locexpr:");
}

static void __attribute__((noinline, noclone))
c (int i, double j)
{
  d (i * 10, j * 10);
}

static void __attribute__((noinline, noclone))
a (int i, double j)
{
  c (i + 1, j + 1);
}

static void __attribute__((noinline, noclone))
b (int i, double j)
{
  c (i + 2, j + 2);
}

static void __attribute__((noinline, noclone))
amb_z (int i)
{
  d (i + 7, i + 7.5);
}

static void __attribute__((noinline, noclone))
amb_y (int i)
{
  amb_z (i + 6);
}

static void __attribute__((noinline, noclone))
amb_x (int i)
{
  amb_y (i + 5);
}

static void __attribute__((noinline, noclone))
amb (int i)
{
  if (i < 0)
    amb_x (i + 3);
  else
    amb_x (i + 4);
}

static void __attribute__((noinline, noclone))
amb_b (int i)
{
  amb (i + 2);
}

static void __attribute__((noinline, noclone))
amb_a (int i)
{
  amb_b (i + 1);
}

static void __attribute__((noinline, noclone)) self (int i);

static void __attribute__((noinline, noclone))
self2 (int i)
{
  self (i);
}

static void __attribute__((noinline, noclone))
self (int i)
{
  if (i == 200)
    {
      /* GCC would inline `self' as `cmovne' without the `self2' indirect.  */
      self2 (i + 1);
    }
  else
    {
      e (v, v);
      d (i + 2, i + 2.5);
    }
}

static void __attribute__((noinline, noclone))
stacktest (int r1, int r2, int r3, int r4, int r5, int r6, int s1, int s2,
	   double d1, double d2, double d3, double d4, double d5, double d6,
	   double d7, double d8, double d9, double da)
{
  s1 = 3;
  s2 = 4;
  d9 = 3.5;
  da = 4.5;
  e (v, v);
asm ("breakhere_stacktest:");
  e (v, v);
}

/* nodataparam has DW_AT_GNU_call_site_value but it does not have
   DW_AT_GNU_call_site_data_value.  GDB should not display dereferenced @entry
   value for it.  */

static void __attribute__((noinline, noclone))
reference (int &regparam, int &nodataparam, int r3, int r4, int r5, int r6,
	   int &stackparam1, int &stackparam2)
{
  int regcopy = regparam, nodatacopy = nodataparam;
  int stackcopy1 = stackparam1, stackcopy2 = stackparam2;

  regparam = 21;
  nodataparam = 22;
  stackparam1 = 31;
  stackparam2 = 32;
  e (v, v);
asm ("breakhere_reference:");
  e (v, v);
}

static int *__attribute__((noinline, noclone))
datap ()
{
  static int two = 2;

  return &two;
}

static void __attribute__((noinline, noclone))
datap_input (int *datap)
{
  (*datap)++;
}

static int __attribute__((noinline, noclone))
data (void)
{
  return 10;
}

static int __attribute__((noinline, noclone))
data2 (void)
{
  return 20;
}

static int __attribute__((noinline, noclone))
different (int val)
{
  val++;
  e (val, val);
asm ("breakhere_different:");
  return val;
}

static int __attribute__((noinline, noclone))
validity (int lost, int born)
{
  lost = data ();
  e (0, 0.0);
asm ("breakhere_validity:");
  return born;
}

static void __attribute__((noinline, noclone))
invalid (int inv)
{
  e (0, 0.0);
asm ("breakhere_invalid:");
}

int
main ()
{
  d (30, 30.5);
  locexpr (30);
  stacktest (1, 2, 3, 4, 5, 6, 11, 12,
	     1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 11.5, 12.5);
  different (5);
  validity (5, data ());
  invalid (data2 ());

  {
    int regvar = 1, *nodatavarp = datap (), stackvar1 = 11, stackvar2 = 12;
    reference (regvar, *nodatavarp, 3, 4, 5, 6, stackvar1, stackvar2);
    datap_input (nodatavarp);
  }

  if (v)
    a (1, 1.25);
  else
    b (5, 5.25);
  amb_a (100);
  self (200);
  return 0;
}
