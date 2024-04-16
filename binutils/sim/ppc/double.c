/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

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


#ifndef _DOUBLE_C_
#define _DOUBLE_C_

#include "basics.h"
#include "ansidecls.h"

#define SFtype uint32_t
#define DFtype uint64_t

#define HItype int16_t
#define SItype int32_t
#define DItype int64_t

#define UHItype uint16_t
#define USItype uint32_t
#define UDItype uint64_t


#define US_SOFTWARE_GOFAST
#include "dp-bit.c"

#endif
