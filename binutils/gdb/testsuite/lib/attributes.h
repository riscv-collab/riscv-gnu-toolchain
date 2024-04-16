/* This file is part of GDB, the GNU debugger.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

/* Compatibility macro for __attribute__((noclone)).  */

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __has_attribute
# if !__has_attribute (noclone)
#  define ATTRIBUTE_NOCLONE
# endif
#endif
#ifndef ATTRIBUTE_NOCLONE
# define ATTRIBUTE_NOCLONE __attribute__((noclone))
#endif

#ifdef __cplusplus
}
#endif

#endif /* ATTRIBUTES_H */
