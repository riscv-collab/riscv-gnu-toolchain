/* Run a function on the main thread
   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef GDB_RUN_ON_MAIN_THREAD_H
#define GDB_RUN_ON_MAIN_THREAD_H

#include <functional>

/* Send a runnable to the main thread.  */

extern void run_on_main_thread (std::function<void ()> &&);

/* Return true on the main thread.  */

extern bool is_main_thread ();

#endif /* GDB_RUN_ON_MAIN_THREAD_H */
