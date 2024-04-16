/* Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_SIGNALS_STATE_SAVE_RESTORE_H
#define COMMON_SIGNALS_STATE_SAVE_RESTORE_H

/* Save/restore the signal actions of all signals, and the signal
   mask.

   Since the exec family of functions does not reset the signal
   disposition of signals set to SIG_IGN, nor does it reset the signal
   mask, in order to be transparent, when spawning new child processes
   to debug (with "run", etc.), we must reset signal actions and mask
   back to what was originally inherited from gdb/gdbserver's parent,
   just before execing the target program to debug.  */

/* Save the signal state of all signals.  If !QUIET, warn if we detect
   a custom signal handler preinstalled.  */

extern void save_original_signals_state (bool quiet);

/* Restore the signal state of all signals.  */

extern void restore_original_signals_state (void);

#endif /* COMMON_SIGNALS_STATE_SAVE_RESTORE_H */
