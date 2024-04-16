/* Python interface to btrace record targets.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

#ifndef PYTHON_PY_RECORD_BTRACE_H
#define PYTHON_PY_RECORD_BTRACE_H

#include "python-internal.h"

/* Implementation of record.method [str].  */
extern PyObject *recpy_bt_method (PyObject *self, void *closure);

/* Implementation of record.format [str].  */
extern PyObject *recpy_bt_format (PyObject *self, void *closure);

/* Implementation of record.goto (instruction) -> None.  */
extern PyObject *recpy_bt_goto (PyObject *self, PyObject *value);

/* Implementation of record.instruction_history [list].  */
extern PyObject *recpy_bt_instruction_history (PyObject *self, void *closure);

/* Implementation of record.function_call_history [list].  */
extern PyObject *recpy_bt_function_call_history (PyObject *self, void *closure);

/* Implementation of record.replay_position [instruction].  */
extern PyObject *recpy_bt_replay_position (PyObject *self, void *closure);

/* Implementation of record.begin [instruction].  */
extern PyObject *recpy_bt_begin (PyObject *self, void *closure);

/* Implementation of record.end [instruction].  */
extern PyObject *recpy_bt_end (PyObject *self, void *closure);

/* Implementation of RecordInstruction.number [int].  */
extern PyObject *recpy_bt_insn_number (PyObject *self, void *closure);

/* Implementation of RecordInstruction.sal [gdb.Symtab_and_line].  */
extern PyObject *recpy_bt_insn_sal (PyObject *self, void *closure);

/* Implementation of RecordInstruction.pc [int].  */
extern PyObject *recpy_bt_insn_pc (PyObject *self, void *closure);

/* Implementation of RecordInstruction.data [buffer].  */
extern PyObject *recpy_bt_insn_data (PyObject *self, void *closure);

/* Implementation of RecordInstruction.decoded [str].  */
extern PyObject *recpy_bt_insn_decoded (PyObject *self, void *closure);

/* Implementation of RecordInstruction.size [int].  */
extern PyObject *recpy_bt_insn_size (PyObject *self, void *closure);

/* Implementation of RecordInstruction.is_speculative [bool].  */
extern PyObject *recpy_bt_insn_is_speculative (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.number [int].  */
extern PyObject *recpy_bt_func_number (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.number [int].  */
extern PyObject *recpy_bt_func_level (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.symbol [gdb.Symbol].  */
extern PyObject *recpy_bt_func_symbol (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.instructions [list].  */
extern PyObject *recpy_bt_func_instructions (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.up [RecordFunctionSegment].  */
extern PyObject *recpy_bt_func_up (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.prev [RecordFunctionSegment].  */
extern PyObject *recpy_bt_func_prev (PyObject *self, void *closure);

/* Implementation of RecordFunctionSegment.next [RecordFunctionSegment].  */
extern PyObject *recpy_bt_func_next (PyObject *self, void *closure);

#endif /* PYTHON_PY_RECORD_BTRACE_H */
