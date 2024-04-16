/* The common simulator framework for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney and Red Hat.

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


#ifndef SIM_BASICS_H
#define SIM_BASICS_H


/* Basic configuration */

#include "defs.h"

/* Basic host dependant mess - hopefully <stdio.h> + <stdarg.h> will
   bring potential conflicts out in the open */

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>


#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


/* Global types that code manipulates */

struct hw;
struct _sim_fpu;


/* Generic address space (maps) and access attributes */

enum {
  read_map = 0,
  write_map = 1,
  exec_map = 2,
  io_map = 3,
  nr_maps = 32, /* something small */
};

enum {
  access_invalid = 0,
  access_read = 1 << read_map,
  access_write = 1 << write_map,
  access_exec = 1 << exec_map,
  access_io = 1 << io_map,
};

enum {
  access_read_write = (access_read | access_write),
  access_read_exec = (access_read | access_exec),
  access_write_exec = (access_write | access_exec),
  access_read_write_exec = (access_read | access_write | access_exec),
  access_read_io = (access_read | access_io),
  access_write_io = (access_write | access_io),
  access_read_write_io = (access_read | access_write | access_io),
  access_exec_io = (access_exec | access_io),
  access_read_exec_io = (access_read | access_exec | access_io),
  access_write_exec_io = (access_write | access_exec | access_io),
  access_read_write_exec_io = (access_read | access_write | access_exec | access_io),
};


/* disposition of an object when things are reset */

typedef enum {
  permanent_object,
  temporary_object,
} object_disposition;


/* Memory transfer types */

typedef enum _transfer_type {
  read_transfer,
  write_transfer,
} transfer_type;


/* directions */

typedef enum {
  bidirect_port,
  input_port,
  output_port,
} port_direction;



/* Basic definitions - ordered so that nothing calls what comes after it.  */

#include "sim/sim.h"

#include "sim-config.h"

#include "sim-inline.h"

#include "sim-types.h"
#include "sim-bits.h"
#include "sim-endian.h"

#include "sim-utils.h"

/* Note: Only the simpler interfaces are defined here.  More heavy
   weight objects, such as core and events, are defined in the more
   serious sim-base.h header. */

#endif /* SIM_BASICS_H */
