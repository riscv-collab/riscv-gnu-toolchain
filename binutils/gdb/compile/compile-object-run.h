/* Header file to call module for 'compile' command.
   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#ifndef COMPILE_COMPILE_OBJECT_RUN_H
#define COMPILE_COMPILE_OBJECT_RUN_H

#include "compile-object-load.h"

extern void compile_object_run (compile_module_up &&module);

#endif /* COMPILE_COMPILE_OBJECT_RUN_H */
