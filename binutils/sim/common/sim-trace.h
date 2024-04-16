/* Simulator tracing/debugging support.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.
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

/* This file is meant to be included by sim-basics.h.  */

#ifndef SIM_TRACE_H
#define SIM_TRACE_H

#include <stdarg.h>

#include "ansidecl.h"
#include "bfd.h"
#include "dis-asm.h"

/* Standard traceable entities.  */

enum {
  /* Trace insn execution.  The port itself is responsible for displaying what
     it thinks it is decoding.  */
  TRACE_INSN_IDX = 1,

  /* Disassemble code addresses.  Like insn tracing, but relies on the opcode
     framework for displaying code.  Can be slower, more accurate as to what
     the binary code actually is, but not how the sim is decoding it.  */
  TRACE_DISASM_IDX,

  /* Trace insn decoding.
     ??? This is more of a simulator debugging operation and might best be
     moved to --debug-decode.  */
  TRACE_DECODE_IDX,

  /* Trace insn extraction.
     ??? This is more of a simulator debugging operation and might best be
     moved to --debug-extract.  */
  TRACE_EXTRACT_IDX,

  /* Trace insn execution but include line numbers.  */
  TRACE_LINENUM_IDX,

  /* Trace memory operations.
     The difference between this and TRACE_CORE_IDX is (I think) that this
     is intended to apply to a higher level.  TRACE_CORE_IDX applies to the
     low level core operations.  */
  TRACE_MEMORY_IDX,

  /* Include model performance data in tracing output.  */
  TRACE_MODEL_IDX,

  /* Trace ALU (Arithmetic Logic Unit) operations.  */
  TRACE_ALU_IDX,

  /* Trace memory core operations.  */
  TRACE_CORE_IDX,

  /* Trace events.  */
  TRACE_EVENTS_IDX,

  /* Trace FPU (Floating Point Unit) operations.  */
  TRACE_FPU_IDX,

  /* Trace VPU (Vector Processing Unit) operations.  */
  TRACE_VPU_IDX,

  /* Trace branching.  */
  TRACE_BRANCH_IDX,

  /* Trace syscalls.  */
  TRACE_SYSCALL_IDX,

  /* Trace cpu register accesses.  Registers that are part of hardware devices
     should use the HW_TRACE macros instead.  */
  TRACE_REGISTER_IDX,

  /* Add information useful for debugging the simulator to trace output.  */
  TRACE_DEBUG_IDX,

  /* Simulator specific trace bits begin here.  */
  TRACE_NEXT_IDX,

};
/* Maximum number of traceable entities.  */
#ifndef MAX_TRACE_VALUES
#define MAX_TRACE_VALUES 32
#endif

/* The -t option only prints useful values.  It's easy to type and shouldn't
   splat on the screen everything under the sun making nothing easy to
   find.  */
#define TRACE_USEFUL_MASK \
  (TRACE_insn | TRACE_linenum | TRACE_memory | TRACE_model)

/* Masks so WITH_TRACE can have symbolic values.
   The case choice here is on purpose.  The lowercase parts are args to
   --with-trace.  */
#define TRACE_insn     (1 << TRACE_INSN_IDX)
#define TRACE_disasm   (1 << TRACE_DISASM_IDX)
#define TRACE_decode   (1 << TRACE_DECODE_IDX)
#define TRACE_extract  (1 << TRACE_EXTRACT_IDX)
#define TRACE_linenum  (1 << TRACE_LINENUM_IDX)
#define TRACE_memory   (1 << TRACE_MEMORY_IDX)
#define TRACE_model    (1 << TRACE_MODEL_IDX)
#define TRACE_alu      (1 << TRACE_ALU_IDX)
#define TRACE_core     (1 << TRACE_CORE_IDX)
#define TRACE_events   (1 << TRACE_EVENTS_IDX)
#define TRACE_fpu      (1 << TRACE_FPU_IDX)
#define TRACE_vpu      (1 << TRACE_VPU_IDX)
#define TRACE_branch   (1 << TRACE_BRANCH_IDX)
#define TRACE_syscall  (1 << TRACE_SYSCALL_IDX)
#define TRACE_register (1 << TRACE_REGISTER_IDX)
#define TRACE_debug    (1 << TRACE_DEBUG_IDX)

/* Return non-zero if tracing of idx is enabled (compiled in).  */
#define WITH_TRACE_P(idx)	((WITH_TRACE & (1 << idx)) != 0)

/* Preprocessor macros to simplify tests of WITH_TRACE.  */
#define WITH_TRACE_ANY_P	(WITH_TRACE != 0)
#define WITH_TRACE_INSN_P	WITH_TRACE_P (TRACE_INSN_IDX)
#define WITH_TRACE_DISASM_P	WITH_TRACE_P (TRACE_DISASM_IDX)
#define WITH_TRACE_DECODE_P	WITH_TRACE_P (TRACE_DECODE_IDX)
#define WITH_TRACE_EXTRACT_P	WITH_TRACE_P (TRACE_EXTRACT_IDX)
#define WITH_TRACE_LINENUM_P	WITH_TRACE_P (TRACE_LINENUM_IDX)
#define WITH_TRACE_MEMORY_P	WITH_TRACE_P (TRACE_MEMORY_IDX)
#define WITH_TRACE_MODEL_P	WITH_TRACE_P (TRACE_MODEL_IDX)
#define WITH_TRACE_ALU_P	WITH_TRACE_P (TRACE_ALU_IDX)
#define WITH_TRACE_CORE_P	WITH_TRACE_P (TRACE_CORE_IDX)
#define WITH_TRACE_EVENTS_P	WITH_TRACE_P (TRACE_EVENTS_IDX)
#define WITH_TRACE_FPU_P	WITH_TRACE_P (TRACE_FPU_IDX)
#define WITH_TRACE_VPU_P	WITH_TRACE_P (TRACE_VPU_IDX)
#define WITH_TRACE_BRANCH_P	WITH_TRACE_P (TRACE_BRANCH_IDX)
#define WITH_TRACE_SYSCALL_P	WITH_TRACE_P (TRACE_SYSCALL_IDX)
#define WITH_TRACE_REGISTER_P	WITH_TRACE_P (TRACE_REGISTER_IDX)
#define WITH_TRACE_DEBUG_P	WITH_TRACE_P (TRACE_DEBUG_IDX)

/* Struct containing all system and cpu trace data.

   System trace data is stored with the associated module.
   System and cpu tracing must share the same space of bitmasks as they
   are arguments to --with-trace.  One could have --with-trace and
   --with-cpu-trace or some such but that's an over-complication at this point
   in time.  Also, there may be occasions where system and cpu tracing may
   wish to share a name.  */

typedef struct _trace_data {

  /* Global summary of all the current trace options */
  char trace_any_p;

  /* Boolean array of specified tracing flags.  */
  /* ??? It's not clear that using an array vs a bit mask is faster.
     Consider the case where one wants to test whether any of several bits
     are set.  */
  char trace_flags[MAX_TRACE_VALUES];
#define TRACE_FLAGS(t) ((t)->trace_flags)

  /* Tracing output goes to this or stderr if NULL.
     We can't store `stderr' here as stderr goes through a callback.  */
  FILE *trace_file;
#define TRACE_FILE(t) ((t)->trace_file)

  /* Buffer to store the prefix to be printed before any trace line.  */
  char trace_prefix[256];
#define TRACE_PREFIX(t) ((t)->trace_prefix)

  /* Buffer to save the inputs for the current instruction.  Use a
     union to force the buffer into correct alignment */
  union {
    uint8_t i8;
    uint16_t i16;
    uint32_t i32;
    uint64_t i64;
  } trace_input_data[16];
  uint8_t trace_input_fmt[16];
  uint8_t trace_input_size[16];
  int trace_input_idx;
#define TRACE_INPUT_DATA(t) ((t)->trace_input_data)
#define TRACE_INPUT_FMT(t) ((t)->trace_input_fmt)
#define TRACE_INPUT_SIZE(t) ((t)->trace_input_size)
#define TRACE_INPUT_IDX(t) ((t)->trace_input_idx)

  /* Category of trace being performed */
  int trace_idx;
#define TRACE_IDX(t) ((t)->trace_idx)

  /* Trace range.
     ??? Not all cpu's support this.  */
  ADDR_RANGE range;
#define TRACE_RANGE(t) (& (t)->range)

  /* The bfd used to disassemble code.  Should compare against STATE_PROG_BFD
     before using the disassembler helper.
     Meant for use by the internal trace module only.  */
  struct bfd *dis_bfd;

  /* The function used to actually disassemble code.
     Meant for use by the internal trace module only.  */
  disassembler_ftype disassembler;

  /* State used with the disassemble function.
     Meant for use by the internal trace module only.  */
  disassemble_info dis_info;
} TRACE_DATA;

/* System tracing support.  */

#define STATE_TRACE_FLAGS(sd) TRACE_FLAGS (STATE_TRACE_DATA (sd))

/* Return non-zero if tracing of IDX is enabled for non-cpu specific
   components.  The "S" in "STRACE" refers to "System".  */
#define STRACE_P(sd,idx) \
  (WITH_TRACE_P (idx) && STATE_TRACE_FLAGS (sd)[idx] != 0)

/* Non-zero if --trace-<xxxx> was specified for SD.  */
#define STRACE_ANY_P(sd)	(WITH_TRACE_ANY_P && (STATE_TRACE_DATA (sd)->trace_any_p))
#define STRACE_INSN_P(sd)	STRACE_P (sd, TRACE_INSN_IDX)
#define STRACE_DISASM_P(sd)	STRACE_P (sd, TRACE_DISASM_IDX)
#define STRACE_DECODE_P(sd)	STRACE_P (sd, TRACE_DECODE_IDX)
#define STRACE_EXTRACT_P(sd)	STRACE_P (sd, TRACE_EXTRACT_IDX)
#define STRACE_LINENUM_P(sd)	STRACE_P (sd, TRACE_LINENUM_IDX)
#define STRACE_MEMORY_P(sd)	STRACE_P (sd, TRACE_MEMORY_IDX)
#define STRACE_MODEL_P(sd)	STRACE_P (sd, TRACE_MODEL_IDX)
#define STRACE_ALU_P(sd)	STRACE_P (sd, TRACE_ALU_IDX)
#define STRACE_CORE_P(sd)	STRACE_P (sd, TRACE_CORE_IDX)
#define STRACE_EVENTS_P(sd)	STRACE_P (sd, TRACE_EVENTS_IDX)
#define STRACE_FPU_P(sd)	STRACE_P (sd, TRACE_FPU_IDX)
#define STRACE_VPU_P(sd)	STRACE_P (sd, TRACE_VPU_IDX)
#define STRACE_BRANCH_P(sd)	STRACE_P (sd, TRACE_BRANCH_IDX)
#define STRACE_SYSCALL_P(sd)	STRACE_P (sd, TRACE_SYSCALL_IDX)
#define STRACE_REGISTER_P(sd)	STRACE_P (sd, TRACE_REGISTER_IDX)
#define STRACE_DEBUG_P(sd)	STRACE_P (sd, TRACE_DEBUG_IDX)

/* Helper functions for printing messages.  */
#define STRACE(sd, idx, fmt, args...) \
  do { \
    if (STRACE_P (sd, idx)) \
      trace_generic (sd, NULL, idx, fmt, ## args); \
  } while (0)
#define STRACE_INSN(sd, fmt, args...)		STRACE (sd, TRACE_INSN_IDX, fmt, ## args)
#define STRACE_DISASM(sd, fmt, args...)		STRACE (sd, TRACE_DISASM_IDX, fmt, ## args)
#define STRACE_DECODE(sd, fmt, args...)		STRACE (sd, TRACE_DECODE_IDX, fmt, ## args)
#define STRACE_EXTRACT(sd, fmt, args...)	STRACE (sd, TRACE_EXTRACT_IDX, fmt, ## args)
#define STRACE_LINENUM(sd, fmt, args...)	STRACE (sd, TRACE_LINENUM_IDX, fmt, ## args)
#define STRACE_MEMORY(sd, fmt, args...)		STRACE (sd, TRACE_MEMORY_IDX, fmt, ## args)
#define STRACE_MODEL(sd, fmt, args...)		STRACE (sd, TRACE_MODEL_IDX, fmt, ## args)
#define STRACE_ALU(sd, fmt, args...)		STRACE (sd, TRACE_ALU_IDX, fmt, ## args)
#define STRACE_CORE(sd, fmt, args...)		STRACE (sd, TRACE_CORE_IDX, fmt, ## args)
#define STRACE_EVENTS(sd, fmt, args...)		STRACE (sd, TRACE_EVENTS_IDX, fmt, ## args)
#define STRACE_FPU(sd, fmt, args...)		STRACE (sd, TRACE_FPU_IDX, fmt, ## args)
#define STRACE_VPU(sd, fmt, args...)		STRACE (sd, TRACE_VPU_IDX, fmt, ## args)
#define STRACE_BRANCH(sd, fmt, args...)		STRACE (sd, TRACE_BRANCH_IDX, fmt, ## args)
#define STRACE_SYSCALL(sd, fmt, args...)	STRACE (sd, TRACE_SYSCALL_IDX, fmt, ## args)
#define STRACE_REGISTER(sd, fmt, args...)	STRACE (sd, TRACE_REGISTER_IDX, fmt, ## args)
#define STRACE_DEBUG(sd, fmt, args...)		STRACE (sd, TRACE_DEBUG_IDX, fmt, ## args)

/* CPU tracing support.  */

#define CPU_TRACE_FLAGS(cpu) TRACE_FLAGS (CPU_TRACE_DATA (cpu))

/* Return non-zero if tracing of IDX is enabled for CPU.  */
#define TRACE_P(cpu,idx) \
  (WITH_TRACE_P (idx) && CPU_TRACE_FLAGS (cpu)[idx] != 0)

/* Non-zero if --trace-<xxxx> was specified for CPU.  */
#define TRACE_ANY_P(cpu)	(WITH_TRACE_ANY_P && (CPU_TRACE_DATA (cpu)->trace_any_p))
#define TRACE_INSN_P(cpu)	TRACE_P (cpu, TRACE_INSN_IDX)
#define TRACE_DISASM_P(cpu)	TRACE_P (cpu, TRACE_DISASM_IDX)
#define TRACE_DECODE_P(cpu)	TRACE_P (cpu, TRACE_DECODE_IDX)
#define TRACE_EXTRACT_P(cpu)	TRACE_P (cpu, TRACE_EXTRACT_IDX)
#define TRACE_LINENUM_P(cpu)	TRACE_P (cpu, TRACE_LINENUM_IDX)
#define TRACE_MEMORY_P(cpu)	TRACE_P (cpu, TRACE_MEMORY_IDX)
#define TRACE_MODEL_P(cpu)	TRACE_P (cpu, TRACE_MODEL_IDX)
#define TRACE_ALU_P(cpu)	TRACE_P (cpu, TRACE_ALU_IDX)
#define TRACE_CORE_P(cpu)	TRACE_P (cpu, TRACE_CORE_IDX)
#define TRACE_EVENTS_P(cpu)	TRACE_P (cpu, TRACE_EVENTS_IDX)
#define TRACE_FPU_P(cpu)	TRACE_P (cpu, TRACE_FPU_IDX)
#define TRACE_VPU_P(cpu)	TRACE_P (cpu, TRACE_VPU_IDX)
#define TRACE_BRANCH_P(cpu)	TRACE_P (cpu, TRACE_BRANCH_IDX)
#define TRACE_SYSCALL_P(cpu)	TRACE_P (cpu, TRACE_SYSCALL_IDX)
#define TRACE_REGISTER_P(cpu)	TRACE_P (cpu, TRACE_REGISTER_IDX)
#define TRACE_DEBUG_P(cpu)	TRACE_P (cpu, TRACE_DEBUG_IDX)
#define TRACE_DISASM_P(cpu)	TRACE_P (cpu, TRACE_DISASM_IDX)

/* Helper functions for printing messages.  */
#define TRACE(cpu, idx, fmt, args...) \
  do { \
    if (TRACE_P (cpu, idx)) \
      trace_generic (CPU_STATE (cpu), cpu, idx, fmt, ## args); \
  } while (0)
#define TRACE_INSN(cpu, fmt, args...)		TRACE (cpu, TRACE_INSN_IDX, fmt, ## args)
#define TRACE_DECODE(cpu, fmt, args...)		TRACE (cpu, TRACE_DECODE_IDX, fmt, ## args)
#define TRACE_EXTRACT(cpu, fmt, args...)	TRACE (cpu, TRACE_EXTRACT_IDX, fmt, ## args)
#define TRACE_LINENUM(cpu, fmt, args...)	TRACE (cpu, TRACE_LINENUM_IDX, fmt, ## args)
#define TRACE_MEMORY(cpu, fmt, args...)		TRACE (cpu, TRACE_MEMORY_IDX, fmt, ## args)
#define TRACE_MODEL(cpu, fmt, args...)		TRACE (cpu, TRACE_MODEL_IDX, fmt, ## args)
#define TRACE_ALU(cpu, fmt, args...)		TRACE (cpu, TRACE_ALU_IDX, fmt, ## args)
#define TRACE_CORE(cpu, fmt, args...)		TRACE (cpu, TRACE_CORE_IDX, fmt, ## args)
#define TRACE_EVENTS(cpu, fmt, args...)		TRACE (cpu, TRACE_EVENTS_IDX, fmt, ## args)
#define TRACE_FPU(cpu, fmt, args...)		TRACE (cpu, TRACE_FPU_IDX, fmt, ## args)
#define TRACE_VPU(cpu, fmt, args...)		TRACE (cpu, TRACE_VPU_IDX, fmt, ## args)
#define TRACE_BRANCH(cpu, fmt, args...)		TRACE (cpu, TRACE_BRANCH_IDX, fmt, ## args)
#define TRACE_SYSCALL(cpu, fmt, args...)	TRACE (cpu, TRACE_SYSCALL_IDX, fmt, ## args)
#define TRACE_REGISTER(cpu, fmt, args...)	TRACE (cpu, TRACE_REGISTER_IDX, fmt, ## args)
#define TRACE_DEBUG(cpu, fmt, args...)		TRACE (cpu, TRACE_DEBUG_IDX, fmt, ## args)
#define TRACE_DISASM(cpu, addr) \
  do { \
    if (TRACE_DISASM_P (cpu)) \
      trace_disasm (CPU_STATE (cpu), cpu, addr); \
  } while (0)

/* Tracing functions.  */

/* Prime the trace buffers ready for any trace output.
   Must be called prior to any other trace operation */
extern void trace_prefix (SIM_DESC sd,
			  sim_cpu *cpu,
			  sim_cia cia,
			  address_word pc,
			  int print_linenum_p,
			  const char *file_name,
			  int line_nr,
			  const char *fmt,
			  ...) ATTRIBUTE_PRINTF (8, 9);

/* Generic trace print, assumes trace_prefix() has been called */

extern void trace_generic (SIM_DESC sd,
			   sim_cpu *cpu,
			   int trace_idx,
			   const char *fmt,
			   ...) ATTRIBUTE_PRINTF (4, 5);

/* Disassemble the specified address.  */

extern void trace_disasm (SIM_DESC sd, sim_cpu *cpu, address_word addr);

typedef enum {
  trace_fmt_invalid,
  trace_fmt_word,
  trace_fmt_fp,
  trace_fmt_fpu,
  trace_fmt_string,
  trace_fmt_bool,
  trace_fmt_addr,
  trace_fmt_instruction_incomplete,
} data_fmt;

/* Trace a varying number of word sized inputs/outputs.  trace_result*
   must be called to close the trace operation. */

extern void save_data (SIM_DESC sd,
		       TRACE_DATA *data,
		       data_fmt fmt,
		       long size,
		       const void *buf);

extern void trace_input0 (SIM_DESC sd,
			  sim_cpu *cpu,
			  int trace_idx);

extern void trace_input_word1 (SIM_DESC sd,
			       sim_cpu *cpu,
			       int trace_idx,
			       unsigned_word d0);

extern void trace_input_word2 (SIM_DESC sd,
			       sim_cpu *cpu,
			       int trace_idx,
			       unsigned_word d0,
			       unsigned_word d1);

extern void trace_input_word3 (SIM_DESC sd,
				       sim_cpu *cpu,
				       int trace_idx,
				       unsigned_word d0,
				       unsigned_word d1,
				       unsigned_word d2);

extern void trace_input_word4 (SIM_DESC sd,
			       sim_cpu *cpu,
			       int trace_idx,
			       unsigned_word d0,
			       unsigned_word d1,
			       unsigned_word d2,
			       unsigned_word d3);

extern void trace_input_addr1 (SIM_DESC sd,
			       sim_cpu *cpu,
			       int trace_idx,
			       address_word d0);

extern void trace_input_bool1 (SIM_DESC sd,
			       sim_cpu *cpu,
			       int trace_idx,
			       int d0);

extern void trace_input_fp1 (SIM_DESC sd,
			     sim_cpu *cpu,
			     int trace_idx,
			     fp_word f0);

extern void trace_input_fp2 (SIM_DESC sd,
			     sim_cpu *cpu,
			     int trace_idx,
			     fp_word f0,
			     fp_word f1);

extern void trace_input_fp3 (SIM_DESC sd,
			     sim_cpu *cpu,
			     int trace_idx,
			     fp_word f0,
			     fp_word f1,
			     fp_word f2);

extern void trace_input_fpu1 (SIM_DESC sd,
			      sim_cpu *cpu,
			      int trace_idx,
			      struct _sim_fpu *f0);

extern void trace_input_fpu2 (SIM_DESC sd,
			      sim_cpu *cpu,
			      int trace_idx,
			      struct _sim_fpu *f0,
			      struct _sim_fpu *f1);

extern void trace_input_fpu3 (SIM_DESC sd,
			      sim_cpu *cpu,
			      int trace_idx,
			      struct _sim_fpu *f0,
			      struct _sim_fpu *f1,
			      struct _sim_fpu *f2);

/* Other trace_input{_<fmt><nr-inputs>} functions can go here */

extern void trace_result0 (SIM_DESC sd,
			   sim_cpu *cpu,
			   int trace_idx);

extern void trace_result_word1 (SIM_DESC sd,
				sim_cpu *cpu,
				int trace_idx,
				unsigned_word r0);

extern void trace_result_word2 (SIM_DESC sd,
				sim_cpu *cpu,
				int trace_idx,
				unsigned_word r0,
				unsigned_word r1);

extern void trace_result_word4 (SIM_DESC sd,
				sim_cpu *cpu,
				int trace_idx,
				unsigned_word r0,
				unsigned_word r1,
				unsigned_word r2,
				unsigned_word r3);

extern void trace_result_bool1 (SIM_DESC sd,
				sim_cpu *cpu,
				int trace_idx,
				int r0);

extern void trace_result_addr1 (SIM_DESC sd,
				sim_cpu *cpu,
				int trace_idx,
				address_word r0);

extern void trace_result_fp1 (SIM_DESC sd,
			      sim_cpu *cpu,
			      int trace_idx,
			      fp_word f0);

extern void trace_result_fp2 (SIM_DESC sd,
			      sim_cpu *cpu,
			      int trace_idx,
			      fp_word f0,
			      fp_word f1);

extern void trace_result_fpu1 (SIM_DESC sd,
			       sim_cpu *cpu,
			       int trace_idx,
			       struct _sim_fpu *f0);

extern void trace_result_string1 (SIM_DESC sd,
				  sim_cpu *cpu,
				  int trace_idx,
				  char *str0);

extern void trace_result_word1_string1 (SIM_DESC sd,
					sim_cpu *cpu,
					int trace_idx,
					unsigned_word r0,
					char *s0);

/* Other trace_result{_<type><nr-results>} */


/* Macros for tracing ALU instructions */

#define TRACE_ALU_INPUT0() \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_input0 (SD, CPU, TRACE_ALU_IDX); \
} while (0)

#define TRACE_ALU_INPUT1(V0) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_input_word1 (SD, CPU, TRACE_ALU_IDX, (V0)); \
} while (0)

#define TRACE_ALU_INPUT2(V0,V1) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_input_word2 (SD, CPU, TRACE_ALU_IDX, (V0), (V1)); \
} while (0)

#define TRACE_ALU_INPUT3(V0,V1,V2) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_input_word3 (SD, CPU, TRACE_ALU_IDX, (V0), (V1), (V2)); \
} while (0)

#define TRACE_ALU_INPUT4(V0,V1,V2,V3) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_input_word4 (SD, CPU, TRACE_ALU_IDX, (V0), (V1), (V2), (V3)); \
} while (0)

#define TRACE_ALU_RESULT(R0) TRACE_ALU_RESULT1(R0)

#define TRACE_ALU_RESULT0() \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_result0 (SD, CPU, TRACE_ALU_IDX); \
} while (0)

#define TRACE_ALU_RESULT1(R0) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_result_word1 (SD, CPU, TRACE_ALU_IDX, (R0)); \
} while (0)

#define TRACE_ALU_RESULT2(R0,R1) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_result_word2 (SD, CPU, TRACE_ALU_IDX, (R0), (R1)); \
} while (0)

#define TRACE_ALU_RESULT4(R0,R1,R2,R3) \
do { \
  if (TRACE_ALU_P (CPU)) \
    trace_result_word4 (SD, CPU, TRACE_ALU_IDX, (R0), (R1), (R2), (R3)); \
} while (0)

/* Macros for tracing inputs to comparative branch instructions. */

#define TRACE_BRANCH_INPUT1(V0) \
do { \
  if (TRACE_BRANCH_P (CPU)) \
    trace_input_word1 (SD, CPU, TRACE_BRANCH_IDX, (V0)); \
} while (0)

#define TRACE_BRANCH_INPUT2(V0,V1) \
do { \
  if (TRACE_BRANCH_P (CPU)) \
    trace_input_word2 (SD, CPU, TRACE_BRANCH_IDX, (V0), (V1)); \
} while (0)

/* Macros for tracing FPU instructions */

#define TRACE_FP_INPUT0() \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_input0 (SD, CPU, TRACE_FPU_IDX); \
} while (0)

#define TRACE_FP_INPUT1(V0) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_input_fp1 (SD, CPU, TRACE_FPU_IDX, (V0)); \
} while (0)

#define TRACE_FP_INPUT2(V0,V1) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_input_fp2 (SD, CPU, TRACE_FPU_IDX, (V0), (V1)); \
} while (0)

#define TRACE_FP_INPUT3(V0,V1,V2) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_input_fp3 (SD, CPU, TRACE_FPU_IDX, (V0), (V1), (V2)); \
} while (0)

#define TRACE_FP_INPUT_WORD1(V0) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_input_word1 (SD, CPU, TRACE_FPU_IDX, (V0)); \
} while (0)

#define TRACE_FP_RESULT(R0) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_result_fp1 (SD, CPU, TRACE_FPU_IDX, (R0)); \
} while (0)

#define TRACE_FP_RESULT2(R0,R1) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_result_fp2 (SD, CPU, TRACE_FPU_IDX, (R0), (R1)); \
} while (0)

#define TRACE_FP_RESULT_BOOL(R0) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_result_bool1 (SD, CPU, TRACE_FPU_IDX, (R0)); \
} while (0)

#define TRACE_FP_RESULT_WORD(R0) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_result_word1 (SD, CPU, TRACE_FPU_IDX, (R0)); \
} while (0)


/* Macros for tracing branches */

#define TRACE_BRANCH_INPUT(COND) \
do { \
  if (TRACE_BRANCH_P (CPU)) \
    trace_input_bool1 (SD, CPU, TRACE_BRANCH_IDX, (COND)); \
} while (0)

#define TRACE_BRANCH_RESULT(DEST) \
do { \
  if (TRACE_BRANCH_P (CPU)) \
    trace_result_addr1 (SD, CPU, TRACE_BRANCH_IDX, (DEST)); \
} while (0)


extern void trace_printf (SIM_DESC, sim_cpu *, const char *, ...)
    ATTRIBUTE_PRINTF (3, 4);

extern void trace_vprintf (SIM_DESC, sim_cpu *, const char *, va_list)
    ATTRIBUTE_PRINTF (3, 0);

/* Debug support.
   This is included here because there isn't enough of it to justify
   a sim-debug.h.  */

/* Return non-zero if debugging of IDX for CPU is enabled.  */
#define DEBUG_P(cpu, idx) \
((WITH_DEBUG & (1 << (idx))) != 0 \
 && CPU_DEBUG_FLAGS (cpu)[idx] != 0)

/* Non-zero if "--debug-insn" specified.  */
#define DEBUG_INSN_P(cpu) DEBUG_P (cpu, DEBUG_INSN_IDX)

/* Symbol related helpers.  */
int trace_load_symbols (SIM_DESC);
bfd_vma trace_sym_value (SIM_DESC, const char *name);

extern void sim_debug_printf (sim_cpu *, const char *, ...)
    ATTRIBUTE_PRINTF (2, 3);

#endif /* SIM_TRACE_H */
