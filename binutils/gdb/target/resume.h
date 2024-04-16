/* Target resumption definitions and prototypes.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#ifndef TARGET_RESUME_H
#define TARGET_RESUME_H

/* Ways to "resume" a thread.  */

enum resume_kind
{
  /* Thread should continue.  */
  resume_continue,

  /* Thread should single-step.  */
  resume_step,

  /* Thread should be stopped.  */
  resume_stop
};

#endif /* TARGET_RESUME_H */
