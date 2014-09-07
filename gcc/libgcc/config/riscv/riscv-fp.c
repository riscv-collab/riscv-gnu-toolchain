/* Functions needed for soft-float on riscv-linux.  Based on
   rs6000/ppc64-fp.c with TF types removed.
 
   Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002, 2003, 2004, 2006, 2009  Free Software Foundation,
   Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#if defined(__riscv64)
#include "fp-bit.h"

extern DItype __fixdfdi (DFtype);
extern DItype __fixsfdi (SFtype);
extern USItype __fixunsdfsi (DFtype);
extern USItype __fixunssfsi (SFtype);
extern DFtype __floatdidf (DItype);
extern DFtype __floatundidf (UDItype);
extern SFtype __floatdisf (DItype);
extern SFtype __floatundisf (UDItype);

static DItype local_fixunssfdi (SFtype);
static DItype local_fixunsdfdi (DFtype);

DItype
__fixdfdi (DFtype a)
{
  if (a < 0)
    return - local_fixunsdfdi (-a);
  return local_fixunsdfdi (a);
}

DItype
__fixsfdi (SFtype a)
{
  if (a < 0)
    return - local_fixunssfdi (-a);
  return local_fixunssfdi (a);
}

USItype
__fixunsdfsi (DFtype a)
{
  if (a >= - (DFtype) (- ((SItype)(((USItype)1 << ((4 * 8) - 1)) - 1)) - 1))
    return (SItype) (a + (- ((SItype)(((USItype)1 << ((4 * 8) - 1)) - 1)) - 1))
                       - (- ((SItype)(((USItype)1 << ((4 * 8) - 1)) - 1)) - 1);
  return (SItype) a;
}

USItype
__fixunssfsi (SFtype a)
{
  if (a >= - (SFtype) (- ((SItype)(((USItype)1 << ((4 * 8) - 1)) - 1)) - 1))
    return (SItype) (a + (- ((SItype)(((USItype)1 << ((4 * 8) - 1)) - 1)) - 1))
                       - (- ((SItype)(((USItype)1 << ((4 * 8) - 1)) - 1)) - 1);
  return (SItype) a;
}

DFtype
__floatdidf (DItype u)
{
  DFtype d;

  d = (SItype) (u >> (sizeof (SItype) * 8));
  d *= 2.0 * (((UDItype) 1) << ((sizeof (SItype) * 8) - 1));
  d += (USItype) (u & ((((UDItype) 1) << (sizeof (SItype) * 8)) - 1));

  return d;
}

DFtype
__floatundidf (UDItype u)
{
  DFtype d;

  d = (USItype) (u >> (sizeof (SItype) * 8));
  d *= 2.0 * (((UDItype) 1) << ((sizeof (SItype) * 8) - 1));
  d += (USItype) (u & ((((UDItype) 1) << (sizeof (SItype) * 8)) - 1));

  return d;
}

SFtype
__floatdisf (DItype u)
{
  DFtype f;

  if (53 < (sizeof (DItype) * 8)
      && 53 > ((sizeof (DItype) * 8) - 53 + 24))
    {
      if (! (- ((DItype) 1 << 53) < u
             && u < ((DItype) 1 << 53)))
        {
          if ((UDItype) u & (((UDItype) 1 << ((sizeof (DItype) * 8) - 53)) - 1))
            {
              u &= ~ (((UDItype) 1 << ((sizeof (DItype) * 8) - 53)) - 1);
              u |= ((UDItype) 1 << ((sizeof (DItype) * 8) - 53));
            }
        }
    }
  f = (SItype) (u >> (sizeof (SItype) * 8));
  f *= 2.0 * (((UDItype) 1) << ((sizeof (SItype) * 8) - 1));
  f += (USItype) (u & ((((UDItype) 1) << (sizeof (SItype) * 8)) - 1));

  return (SFtype) f;
}

SFtype
__floatundisf (UDItype u)
{
  DFtype f;

  if (53 < (sizeof (DItype) * 8)
      && 53 > ((sizeof (DItype) * 8) - 53 + 24))
    {
      if (u >= ((UDItype) 1 << 53))
        {
          if ((UDItype) u & (((UDItype) 1 << ((sizeof (DItype) * 8) - 53)) - 1))
            {
              u &= ~ (((UDItype) 1 << ((sizeof (DItype) * 8) - 53)) - 1);
              u |= ((UDItype) 1 << ((sizeof (DItype) * 8) - 53));
            }
        }
    }
  f = (USItype) (u >> (sizeof (SItype) * 8));
  f *= 2.0 * (((UDItype) 1) << ((sizeof (SItype) * 8) - 1));
  f += (USItype) (u & ((((UDItype) 1) << (sizeof (SItype) * 8)) - 1));

  return (SFtype) f;
}

/* This version is needed to prevent recursion; fixunsdfdi in libgcc
   calls fixdfdi, which in turn calls calls fixunsdfdi.  */

static DItype
local_fixunsdfdi (DFtype a)
{
  USItype hi, lo;

  hi = a / (((UDItype) 1) << (sizeof (SItype) * 8));
  lo = (a - ((DFtype) hi) * (((UDItype) 1) << (sizeof (SItype) * 8)));
  return ((UDItype) hi << (sizeof (SItype) * 8)) | lo;
}

/* This version is needed to prevent recursion; fixunssfdi in libgcc
   calls fixsfdi, which in turn calls calls fixunssfdi.  */

static DItype
local_fixunssfdi (SFtype original_a)
{
  DFtype a = original_a;
  USItype hi, lo;

  hi = a / (((UDItype) 1) << (sizeof (SItype) * 8));
  lo = (a - ((DFtype) hi) * (((UDItype) 1) << (sizeof (SItype) * 8)));
  return ((UDItype) hi << (sizeof (SItype) * 8)) | lo;
}

#endif
