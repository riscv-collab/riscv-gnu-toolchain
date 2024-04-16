/* Copyright (C) 2018-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef GDBSERVER_X86_TDESC_H
#define GDBSERVER_X86_TDESC_H

/* The "expedite" registers for x86 targets.  Since whether the
   variable is used depends on host/configuration, we mark it
   ATTRIBUTE_UNUSED to keep it simple here.  */
static const char *i386_expedite_regs[] ATTRIBUTE_UNUSED
    = {"ebp", "esp", "eip", NULL};

#ifdef __x86_64__
/* The "expedite" registers for x86_64 targets.  */
static const char *amd64_expedite_regs[] = {"rbp", "rsp", "rip", NULL};
#endif

#endif /* GDBSERVER_X86_TDESC_H */
