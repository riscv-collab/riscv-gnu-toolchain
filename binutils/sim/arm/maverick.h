/*  maverick.h -- Cirrus/DSP co-processor interface header
    Copyright (C) 2003-2024 Free Software Foundation, Inc.
    Contributed by Aldy Hernandez (aldyh@redhat.com).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* Define Co-Processor instruction handlers here.  */

/* Here's ARMulator's DSP definition.  A few things to note:
   1) it has 16 64-bit registers and 4 72-bit accumulators
   2) you can only access its registers with MCR and MRC.  */

struct maverick_regs
{
  union
  {
    int i;
    float f;
  } upper;

  union
  {
    int i;
    float f;
  } lower;
};

union maverick_acc_regs
{
  long double ld;		/* Acc registers are 72-bits.  */
};

extern struct maverick_regs DSPregs[16];
extern union maverick_acc_regs DSPacc[4];
extern ARMword DSPsc;
