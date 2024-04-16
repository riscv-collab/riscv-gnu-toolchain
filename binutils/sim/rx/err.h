/* err.h --- handle errors for RX simulator.

Copyright (C) 2008-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

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

enum execution_error {
  SIM_ERR_NONE,
  SIM_ERR_READ_UNWRITTEN_PAGES,
  SIM_ERR_READ_UNWRITTEN_BYTES,
  SIM_ERR_NULL_POINTER_DEREFERENCE,
  SIM_ERR_CORRUPT_STACK,
  SIM_ERR_NUM_ERRORS
};

enum execution_error_action {
  SIM_ERRACTION_EXIT,
  SIM_ERRACTION_WARN,
  SIM_ERRACTION_IGNORE,
  SIM_ERRACTION_DEBUG,
  SIM_ERRACTION_NUM_ACTIONS
};

void execution_error (enum execution_error num, unsigned long address);
void execution_error_init_standalone (void);
void execution_error_init_debugger (void);
void execution_error_error_all (void);
void execution_error_warn_all (void);
void execution_error_ignore_all (void);
enum execution_error execution_error_get_last_error (void);
void execution_error_clear_last_error (void);
void execution_error_set_action (enum execution_error num,
				 enum execution_error_action act);
