/* Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_PREPROCESSOR_H
#define COMMON_PREPROCESSOR_H

/* Generally useful preprocessor bits.  */

/* Concatenate two tokens.  */
#define CONCAT_1(a, b) a ## b
#define CONCAT(a, b) CONCAT_1 (a, b)

/* Stringification.  */
#define STRINGIFY_1(x) #x
#define STRINGIFY(x) STRINGIFY_1 (x)

/* Escape parens out.  Useful if you need to pass an argument that
   includes commas to another macro.  */
#define ESC_PARENS(...) __VA_ARGS__

#endif /* COMMON_PREPROCESSOR_H */
