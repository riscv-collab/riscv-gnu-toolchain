/*  This file is part of the program psim.

    Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au>
    Copyright (C) 1997-2024 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
    */


#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#define SIM_ENGINE_HALT_HOOK(SD,LAST_CPU,CIA) /* disable this hook */

#include "sim-basics.h"

#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR)  \
mn10300_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), (TRANSFER), (ERROR))

#include "sim-base.h"

/**
 * TODO: Move these includes to the igen files that need them.
 * This requires extending the igen syntax to support header includes.
 *
 * For now only include them in the igen generated support.c,
 * semantics.c, idecode.c and engine.c files.
 */
#if defined(SUPPORT_C) \
    || defined(SEMANTICS_C) \
    || defined(IDECODE_C) \
    || defined(ENGINE_C)
#include "sim-fpu.h"
#include "sim-signal.h"

#include "mn10300-sim.h"
#endif

extern SIM_CORE_SIGNAL_FN mn10300_core_signal ATTRIBUTE_NORETURN;

#endif
