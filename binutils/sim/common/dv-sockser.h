/* Serial port emulation via sockets.
   Copyright (C) 1998-2024 Free Software Foundation, Inc.

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

#ifndef DV_SOCKSER_H
#define DV_SOCKSER_H

#include "sim-inline.h"

/* bits in result of dev_sockser_status */
#define DV_SOCKSER_INPUT_EMPTY  0x1
#define DV_SOCKSER_OUTPUT_EMPTY 0x2
#define DV_SOCKSER_DISCONNECTED 0x4

#ifdef HAVE_DV_SOCKSER

/* FIXME: later add a device ptr arg */
extern int dv_sockser_status (SIM_DESC);
int dv_sockser_write (SIM_DESC, unsigned char);
int dv_sockser_write_buffer (SIM_DESC, const unsigned char *, unsigned);
int dv_sockser_read (SIM_DESC);

#else

/* If dv-sockser isn't available, provide stub functions.  */

STATIC_INLINE int
dv_sockser_status (SIM_DESC sd)
{
  return (DV_SOCKSER_INPUT_EMPTY  |
	  DV_SOCKSER_OUTPUT_EMPTY |
	  DV_SOCKSER_DISCONNECTED);
}

STATIC_INLINE int
dv_sockser_write (SIM_DESC sd, unsigned char c)
{
  return -1;
}

STATIC_INLINE int
dv_sockser_write_buffer (SIM_DESC sd, const unsigned char *buffer,
			 unsigned nr_bytes)
{
  return -1;
}

STATIC_INLINE int
dv_sockser_read (SIM_DESC sd)
{
  return -1;
}

#endif /* HAVE_DV_SOCKSER */

#endif /* DV_SOCKSER_H */
