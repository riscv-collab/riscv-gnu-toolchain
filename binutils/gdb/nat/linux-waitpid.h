/* Wrapper for waitpid for GNU/Linux (LWP layer).

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef NAT_LINUX_WAITPID_H
#define NAT_LINUX_WAITPID_H

/* Wrapper function for waitpid which handles EINTR.  */
extern int my_waitpid (int pid, int *status, int flags);

/* Convert wait status STATUS to a string.  Used for printing debug
   messages only.  */
extern std::string status_to_str (int status);

#endif /* NAT_LINUX_WAITPID_H */
