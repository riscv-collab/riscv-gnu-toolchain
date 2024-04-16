/* The configure generated header settings.

   Copyright 2002-2024 Free Software Foundation, Inc.

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

/* This file should be included by every .c file before any other header.  */

#ifndef DEFS_H
#define DEFS_H

#ifdef HAVE_CONFIG_H

/* Include gnulib's various configure tests.  */
#include "gnulib/config.h"

/* This comes from gnulib.  Export it until ansidecl.h handles it.  */
#define ATTRIBUTE_FALLTHROUGH _GL_ATTRIBUTE_FALLTHROUGH

/* Reset macros that our config.h will provide.  */
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION

/* Include common sim's various configure tests.  */
#ifndef SIM_TOPDIR_BUILD
#include "../config.h"
#else
#include "config.h"
#endif

#endif

#endif
