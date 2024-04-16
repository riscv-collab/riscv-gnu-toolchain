/* Simulator tracing support for Cpu tools GENerated simulators.
   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

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

#ifndef CGEN_TRACE_H
#define CGEN_TRACE_H

#include "ansidecl.h"
#include "bfd.h"

void cgen_trace_insn_init (SIM_CPU *, int);
void cgen_trace_insn_fini (SIM_CPU *, const struct argbuf *, int);
void cgen_trace_insn (SIM_CPU *, const struct cgen_insn *,
		      const struct argbuf *, IADDR);
void cgen_trace_extract (SIM_CPU *, IADDR, const char *, ...);
void cgen_trace_result (SIM_CPU *, const char *, int, ...);
void cgen_trace_printf (SIM_CPU *, const char *fmt, ...) ATTRIBUTE_PRINTF_2;

/* Trace instruction results.  */
#define CGEN_TRACE_RESULT_P(cpu, abuf) \
  (TRACE_INSN_P (cpu) && ARGBUF_TRACE_P (abuf))

#define CGEN_TRACE_INSN_INIT(cpu, abuf, first_p) \
do { \
  if (TRACE_INSN_P (cpu)) \
    cgen_trace_insn_init ((cpu), (first_p)); \
} while (0)
#define CGEN_TRACE_INSN_FINI(cpu, abuf, last_p) \
do { \
  if (TRACE_INSN_P (cpu)) \
    cgen_trace_insn_fini ((cpu), (abuf), (last_p)); \
} while (0)
#define CGEN_TRACE_PRINTF(cpu, what, args) \
do { \
  if (TRACE_P ((cpu), (what))) \
    cgen_trace_printf args ; \
} while (0)
#define CGEN_TRACE_INSN(cpu, insn, abuf, pc) \
do { \
  if (TRACE_INSN_P (cpu) && ARGBUF_TRACE_P (abuf)) \
    cgen_trace_insn ((cpu), (insn), (abuf), (pc)) ; \
} while (0)
#define CGEN_TRACE_EXTRACT(cpu, abuf, args) \
do { \
  if (TRACE_EXTRACT_P (cpu)) \
    cgen_trace_extract args ; \
} while (0)
#define CGEN_TRACE_RESULT(cpu, abuf, name, type, val) \
do { \
  if (CGEN_TRACE_RESULT_P ((cpu), (abuf))) \
    cgen_trace_result ((cpu), (name), (type), (val)) ; \
} while (0)

/* Disassembly support.  */

/* Function to use for cgen-based disassemblers.  */
extern CGEN_DISASSEMBLER sim_cgen_disassemble_insn;

/* Pseudo FILE object for strings.  */
typedef struct {
  char *buffer;
  char *current;
} SFILE;

/* String printer for the disassembler.  */
extern int sim_disasm_sprintf (SFILE *, const char *, ...) ATTRIBUTE_PRINTF_2;
extern int sim_disasm_styled_sprintf (SFILE *, enum disassembler_style, const char *, ...) ATTRIBUTE_PRINTF_3;

/* For opcodes based disassemblers.  */
#ifdef __BFD_H_SEEN__
struct disassemble_info;
extern int
sim_disasm_read_memory (bfd_vma memaddr_, bfd_byte *myaddr_, unsigned int length_,
			struct disassemble_info *info_);
extern void
sim_disasm_perror_memory (int status_, bfd_vma memaddr_,
			  struct disassemble_info *info_);
#endif

#endif /* CGEN_TRACE_H */
