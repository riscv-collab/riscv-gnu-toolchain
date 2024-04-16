/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

extern volatile int gnu_ifunc_initialized;
extern volatile unsigned long resolver_hwcap;
extern int init_stub (int arg);
extern int final (int arg);

typedef int (*final_t) (int arg);

#ifndef IFUNC_RESOLVER_ATTR
asm (".type gnu_ifunc, %gnu_indirect_function");
final_t
gnu_ifunc (unsigned long hwcap)
#else
final_t
gnu_ifunc_resolver (unsigned long hwcap)
#endif
{
  resolver_hwcap = hwcap;
  if (! gnu_ifunc_initialized)
    return init_stub;
  else
    return final;
}

#ifdef IFUNC_RESOLVER_ATTR
extern int gnu_ifunc (int arg);

__typeof (gnu_ifunc) gnu_ifunc __attribute__ ((ifunc ("gnu_ifunc_resolver")));
#endif
