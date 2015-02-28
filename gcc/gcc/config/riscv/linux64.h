/* Definitions for 64-bit RISC-V GNU/Linux systems with ELF format.
   Copyright 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* Force the default ABI flags onto the command line
   in order to make the other specs easier to write.  */
#undef LIB_SPEC
#define LIB_SPEC "\
%{pthread:-lpthread} \
%{shared:-lc} \
%{!shared: \
  %{profile:-lc_p} %{!profile:-lc}}"

#define GLIBC_DYNAMIC_LINKER32 "/lib32/ld.so.1"
#define GLIBC_DYNAMIC_LINKER64 "/lib/ld.so.1"

#undef LINK_SPEC
#define LINK_SPEC "\
%{shared} \
  %{!shared: \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{" OPT_ARCH64 ": -dynamic-linker " GNU_USER_DYNAMIC_LINKER64 "} \
      %{" OPT_ARCH32 ": -dynamic-linker " GNU_USER_DYNAMIC_LINKER32 "}} \
    %{static:-static}} \
%{" OPT_ARCH64 ":-melf64lriscv} \
%{" OPT_ARCH32 ":-melf32lriscv}"
