/*  This file is part of the program psim.

    Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.
 
    */


#ifndef _BASICS_H_
#define _BASICS_H_

/* This must come before any other includes.  */
#include "defs.h"


/* many things pass around the cpu and psim object with out knowing
   what it is */

typedef struct _cpu cpu;
typedef struct _psim psim;
typedef struct _device device;
typedef struct _device_instance device_instance;
typedef struct _event_queue event_queue;
typedef struct _event_entry_tag *event_entry_tag;


/* many things are moving data between the host and target */

typedef enum {
  cooked_transfer,
  raw_transfer,
} transfer_mode;


/* possible exit statuses */

typedef enum {
  was_continuing, was_trap, was_exited, was_signalled
} stop_reason;


/* disposition of an object when things are next restarted */

typedef enum {
  permanent_object,
  tempoary_object,
} object_disposition;


/* directions */

typedef enum {
  bidirect_port,
  input_port,
  output_port,
} port_direction;



/* Basic configuration */

#include "std-config.h"
#include "inline.h"


/* Basic host dependant mess - hopefully <stdio.h> + <stdarg.h> will
   bring potential conflicts out in the open */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/* Basic definitions - ordered so that nothing calls what comes after
   it */

#include "sim_callbacks.h"

#include "debug.h"

#include "words.h"
#include "bits.h"
#include "sim-endian.h"

#endif /* _BASICS_H_ */
