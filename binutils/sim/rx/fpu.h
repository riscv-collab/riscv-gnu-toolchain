/* fpu.h --- FPU emulator for stand-alone RX simulator.

Copyright (C) 2008-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

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

typedef unsigned int fp_t;

extern fp_t rxfp_add (fp_t fa, fp_t fb);
extern fp_t rxfp_sub (fp_t fa, fp_t fb);
extern fp_t rxfp_mul (fp_t fa, fp_t fb);
extern fp_t rxfp_div (fp_t fa, fp_t fb);
extern void rxfp_cmp (fp_t fa, fp_t fb);
extern long rxfp_ftoi (fp_t fa, int round_mode);
extern fp_t rxfp_itof (long fa, int round_mode);
