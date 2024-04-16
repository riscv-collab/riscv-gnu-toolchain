/* Simulation code for the CR16 processor.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "bfd.h"

#include "sim-main.h"
#include "sim-signal.h"
#include "simops.h"
#include "target-newlib-syscall.h"

#include "cr16-sim.h"

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#include <sys/wait.h>

#define EXCEPTION(sig) sim_engine_halt (sd, cpu, NULL, PC, sim_stopped, sig)

enum op_types {
  OP_VOID,
  OP_CONSTANT3,
  OP_UCONSTANT3,
  OP_CONSTANT4,
  OP_CONSTANT4_1,
  OP_CONSTANT5,
  OP_CONSTANT6,
  OP_CONSTANT16,
  OP_UCONSTANT16,
  OP_CONSTANT20,
  OP_UCONSTANT20,
  OP_CONSTANT32,
  OP_UCONSTANT32,
  OP_MEMREF,
  OP_MEMREF2,
  OP_MEMREF3,

  OP_DISP5,
  OP_DISP17,
  OP_DISP25,
  OP_DISPE9,
  //OP_ABS20,
  OP_ABS20_OUTPUT,
  //OP_ABS24,
  OP_ABS24_OUTPUT,

  OP_R_BASE_DISPS16,
  OP_R_BASE_DISP20,
  OP_R_BASE_DISPS20,
  OP_R_BASE_DISPE20,

  OP_RP_BASE_DISPE0,
  OP_RP_BASE_DISP4,
  OP_RP_BASE_DISPE4,
  OP_RP_BASE_DISP14,
  OP_RP_BASE_DISP16,
  OP_RP_BASE_DISP20,
  OP_RP_BASE_DISPS20,
  OP_RP_BASE_DISPE20,

  OP_R_INDEX7_ABS20,
  OP_R_INDEX8_ABS20,

  OP_RP_INDEX_DISP0,
  OP_RP_INDEX_DISP14,
  OP_RP_INDEX_DISP20,
  OP_RP_INDEX_DISPS20, 

  OP_REG,
  OP_REGP,
  OP_PROC_REG,
  OP_PROC_REGP,
  OP_COND,
  OP_RA
};


enum {
  PSR_MASK = (PSR_I_BIT
	      | PSR_P_BIT
	      | PSR_E_BIT
	      | PSR_N_BIT
	      | PSR_Z_BIT
	      | PSR_F_BIT
	      | PSR_U_BIT
	      | PSR_L_BIT
	      | PSR_T_BIT
	      | PSR_C_BIT),
  /* The following bits in the PSR _can't_ be set by instructions such
     as mvtc.  */
  PSR_HW_MASK = (PSR_MASK)
};

/* cond    Code Condition            True State
 * EQ      Equal                     Z flag is 1
 * NE      Not Equal                 Z flag is 0
 * CS      Carry Set                 C flag is 1
 * CC      Carry Clear               C flag is 0
 * HI      Higher                    L flag is 1
 * LS      Lower or Same             L flag is 0
 * GT      Greater Than              N flag is 1 
 * LE      Less Than or Equal To     N flag is 0
 * FS      Flag Set                  F flag is 1
 * FC      Flag Clear                F flag is 0
 * LO      Lower                     Z and L flags are 0
 * HS      Higher or Same            Z or L flag is 1
 * LT      Less Than                 Z and N flags are 0
 * GE      Greater Than or Equal To  Z or N flag is 1.  */

static int cond_stat(int cc)
{
  switch (cc) 
    {
      case 0: return  PSR_Z; break;
      case 1: return !PSR_Z; break;
      case 2: return  PSR_C; break;
      case 3: return !PSR_C; break;
      case 4: return  PSR_L; break;
      case 5: return !PSR_L; break;
      case 6: return  PSR_N; break;
      case 7: return !PSR_N; break;
      case 8: return  PSR_F; break;
      case 9: return !PSR_F; break;
      case 10: return !PSR_Z && !PSR_L; break;
      case 11: return  PSR_Z ||  PSR_L; break;
      case 12: return !PSR_Z && !PSR_N; break;
      case 13: return  PSR_Z ||  PSR_N; break;
      case 14: return 1; break; /*ALWAYS.  */
      default:
     // case NEVER:  return false; break;
      //case NO_COND_CODE:
      //panic("Shouldn't have NO_COND_CODE in an actual instruction!");
      return 0; break;
     }
   return 0;
}


creg_t
move_to_cr (SIM_DESC sd, SIM_CPU *cpu, int cr, creg_t mask, creg_t val, int psw_hw_p)
{
  /* A MASK bit is set when the corresponding bit in the CR should
     be left alone.  */
  /* This assumes that (VAL & MASK) == 0.  */
  switch (cr)
    {
    case PSR_CR:
      if (psw_hw_p)
	val &= PSR_HW_MASK;
#if 0
      else
	val &= PSR_MASK;
	  sim_io_printf
		(sd,
		 "ERROR at PC 0x%x: ST can only be set when FX is set.\n", PC);
	  EXCEPTION (SIM_SIGILL);
#endif
      /* keep an up-to-date psw around for tracing.  */
      State.trace.psw = (State.trace.psw & mask) | val;
      break;
    default:
      break;
    }
  /* only issue an update if the register is being changed.  */
  if ((State.cregs[cr] & ~mask) != val)
   SLOT_PEND_MASK (State.cregs[cr], mask, val);

  return val;
}

#ifdef DEBUG
static void trace_input_func (SIM_DESC sd,
			      const char *name,
			      enum op_types in1,
			      enum op_types in2,
			      enum op_types in3);

#define trace_input(name, in1, in2, in3) do { if (cr16_debug) trace_input_func (sd, name, in1, in2, in3); } while (0)

#ifndef SIZE_INSTRUCTION
#define SIZE_INSTRUCTION 8
#endif

#ifndef SIZE_OPERANDS
#define SIZE_OPERANDS 18
#endif

#ifndef SIZE_VALUES
#define SIZE_VALUES 13
#endif

#ifndef SIZE_LOCATION
#define SIZE_LOCATION 20
#endif

#ifndef SIZE_PC
#define SIZE_PC 4
#endif

#ifndef SIZE_LINE_NUMBER
#define SIZE_LINE_NUMBER 2
#endif

static void
trace_input_func (SIM_DESC sd, const char *name, enum op_types in1, enum op_types in2, enum op_types in3)
{
  char *comma;
  enum op_types in[3];
  int i;
  char buf[1024];
  char *p;
  long tmp;
  char *type;
  const char *filename;
  const char *functionname;
  unsigned int linenumber;
  bfd_vma byte_pc;

  if ((cr16_debug & DEBUG_TRACE) == 0)
    return;

  switch (State.ins_type)
    {
    default:
    case INS_UNKNOWN:		type = " ?"; break;
    }

  if ((cr16_debug & DEBUG_LINE_NUMBER) == 0)
    sim_io_printf (sd,
				       "0x%.*x %s: %-*s ",
				       SIZE_PC, (unsigned)PC,
				       type,
				       SIZE_INSTRUCTION, name);

  else
    {
      buf[0] = '\0';
      byte_pc = PC;
      if (STATE_TEXT_SECTION (sd)
	  && byte_pc >= STATE_TEXT_START (sd)
	  && byte_pc < STATE_TEXT_END (sd))
	{
	  filename = (const char *)0;
	  functionname = (const char *)0;
	  linenumber = 0;
	  if (bfd_find_nearest_line (STATE_PROG_BFD (sd),
				     STATE_TEXT_SECTION (sd),
				     (struct bfd_symbol **)0,
				     byte_pc - STATE_TEXT_START (sd),
				     &filename, &functionname, &linenumber))
	    {
	      p = buf;
	      if (linenumber)
		{
		  sprintf (p, "#%-*d ", SIZE_LINE_NUMBER, linenumber);
		  p += strlen (p);
		}
	      else
		{
		  sprintf (p, "%-*s ", SIZE_LINE_NUMBER+1, "---");
		  p += SIZE_LINE_NUMBER+2;
		}

	      if (functionname)
		{
		  sprintf (p, "%s ", functionname);
		  p += strlen (p);
		}
	      else if (filename)
		{
		  char *q = strrchr (filename, '/');
		  sprintf (p, "%s ", (q) ? q+1 : filename);
		  p += strlen (p);
		}

	      if (*p == ' ')
		*p = '\0';
	    }
	}

      sim_io_printf (sd,
					 "0x%.*x %s: %-*.*s %-*s ",
					 SIZE_PC, (unsigned)PC,
					 type,
					 SIZE_LOCATION, SIZE_LOCATION, buf,
					 SIZE_INSTRUCTION, name);
    }

  in[0] = in1;
  in[1] = in2;
  in[2] = in3;
  comma = "";
  p = buf;
  for (i = 0; i < 3; i++)
    {
      switch (in[i])
	{
	case OP_VOID:
	  break;

	case OP_REG:
	case OP_REGP:
	  sprintf (p, "%sr%d", comma, OP[i]);
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_PROC_REG:
	  sprintf (p, "%scr%d", comma, OP[i]);
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_CONSTANT16:
	  sprintf (p, "%s%d", comma, OP[i]);
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_CONSTANT4:
	  sprintf (p, "%s%d", comma, SEXT4(OP[i]));
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_CONSTANT3:
	  sprintf (p, "%s%d", comma, SEXT3(OP[i]));
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_MEMREF:
	  sprintf (p, "%s@r%d", comma, OP[i]);
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_MEMREF2:
	  sprintf (p, "%s@(%d,r%d)", comma, (int16_t)OP[i], OP[i+1]);
	  p += strlen (p);
	  comma = ",";
	  break;

	case OP_MEMREF3:
	  sprintf (p, "%s@%d", comma, OP[i]);
	  p += strlen (p);
	  comma = ",";
	  break;
	}
    }

  if ((cr16_debug & DEBUG_VALUES) == 0)
    {
      *p++ = '\n';
      *p = '\0';
      sim_io_printf (sd, "%s", buf);
    }
  else
    {
      *p = '\0';
      sim_io_printf (sd, "%-*s", SIZE_OPERANDS, buf);

      p = buf;
      for (i = 0; i < 3; i++)
	{
	  buf[0] = '\0';
	  switch (in[i])
	    {
	    case OP_VOID:
	      sim_io_printf (sd, "%*s", SIZE_VALUES, "");
	      break;

	    case OP_REG:
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t) GPR (OP[i]));
	      break;

	    case OP_REGP:
	      tmp = (long)((((uint32_t) GPR (OP[i])) << 16) | ((uint32_t) GPR (OP[i] + 1)));
	      sim_io_printf (sd, "%*s0x%.8lx", SIZE_VALUES-10, "", tmp);
	      break;

	    case OP_PROC_REG:
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t) CREG (OP[i]));
	      break;

	    case OP_CONSTANT16:
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t)OP[i]);
	      break;

	    case OP_CONSTANT4:
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t)SEXT4(OP[i]));
	      break;

	    case OP_CONSTANT3:
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t)SEXT3(OP[i]));
	      break;

	    case OP_MEMREF2:
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t)OP[i]);
	      sim_io_printf (sd, "%*s0x%.4x", SIZE_VALUES-6, "",
						 (uint16_t)GPR (OP[i + 1]));
	      i++;
	      break;
	    }
	}
    }

  sim_io_flush_stdout (sd);
}

static void
do_trace_output_flush (SIM_DESC sd)
{
  sim_io_flush_stdout (sd);
}

static void
do_trace_output_finish (SIM_DESC sd)
{
  sim_io_printf (sd,
				     " F0=%d F1=%d C=%d\n",
				     (State.trace.psw & PSR_F_BIT) != 0,
				     (State.trace.psw & PSR_F_BIT) != 0,
				     (State.trace.psw & PSR_C_BIT) != 0);
  sim_io_flush_stdout (sd);
}

#if 0
static void
trace_output_40 (SIM_DESC sd, uint64_t val)
{
  if ((cr16_debug & (DEBUG_TRACE | DEBUG_VALUES)) == (DEBUG_TRACE | DEBUG_VALUES))
    {
      sim_io_printf (sd,
					 " :: %*s0x%.2x%.8lx",
					 SIZE_VALUES - 12,
					 "",
					 ((int)(val >> 32) & 0xff),
					 ((unsigned long) val) & 0xffffffff);
      do_trace_output_finish ();
    }
}
#endif

static void
trace_output_32 (SIM_DESC sd, uint32_t val)
{
  if ((cr16_debug & (DEBUG_TRACE | DEBUG_VALUES)) == (DEBUG_TRACE | DEBUG_VALUES))
    {
      sim_io_printf (sd,
					 " :: %*s0x%.8x",
					 SIZE_VALUES - 10,
					 "",
					 (int) val);
      do_trace_output_finish (sd);
    }
}

static void
trace_output_16 (SIM_DESC sd, uint16_t val)
{
  if ((cr16_debug & (DEBUG_TRACE | DEBUG_VALUES)) == (DEBUG_TRACE | DEBUG_VALUES))
    {
      sim_io_printf (sd,
					 " :: %*s0x%.4x",
					 SIZE_VALUES - 6,
					 "",
					 (int) val);
      do_trace_output_finish (sd);
    }
}

static void
trace_output_void (SIM_DESC sd)
{
  if ((cr16_debug & (DEBUG_TRACE | DEBUG_VALUES)) == (DEBUG_TRACE | DEBUG_VALUES))
    {
      sim_io_printf (sd, "\n");
      do_trace_output_flush (sd);
    }
}

static void
trace_output_flag (SIM_DESC sd)
{
  if ((cr16_debug & (DEBUG_TRACE | DEBUG_VALUES)) == (DEBUG_TRACE | DEBUG_VALUES))
    {
      sim_io_printf (sd,
					 " :: %*s",
					 SIZE_VALUES,
					 "");
      do_trace_output_finish (sd);
    }
}




#else
#define trace_input(NAME, IN1, IN2, IN3)
#define trace_output(RESULT)
#endif

/* addub.  */
void
OP_2C_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp;
  uint8_t a = OP[0] & 0xff;
  uint16_t b = (GPR (OP[1])) & 0xff;
  trace_input ("addub", OP_CONSTANT4_1, OP_REG, OP_VOID);
  tmp = (a + b) & 0xff;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* addub.  */
void
OP_2CB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp;
  uint8_t a = ((OP[0]) & 0xff), b = (GPR (OP[1])) & 0xff;
  trace_input ("addub", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = (a + b) & 0xff;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* addub.  */
void
OP_2D_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (GPR (OP[0])) & 0xff;
  uint8_t b = (GPR (OP[1])) & 0xff;
  uint16_t tmp = (a + b) & 0xff;
  trace_input ("addub", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* adduw.  */
void
OP_2E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (a + b);
  trace_input ("adduw", OP_CONSTANT4_1, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* adduw.  */
void
OP_2EB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (a + b);
  trace_input ("adduw", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* adduw.  */
void
OP_2F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (a + b);
  trace_input ("adduw", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* addb.  */
void
OP_30_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0];
  uint8_t b = (GPR (OP[1]) & 0xff);
  uint16_t tmp = (a + b) & 0xff;
  trace_input ("addb", OP_CONSTANT4_1, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  SET_PSR_C (tmp > 0xFF);
  SET_PSR_F (((a & 0x80) == (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  trace_output_16 (sd, tmp);
}

/* addb.  */
void
OP_30B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint8_t b = (GPR (OP[1]) & 0xff);
  uint16_t tmp = (a + b) & 0xff;
  trace_input ("addb", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  SET_PSR_C (tmp > 0xFF);
  SET_PSR_F (((a & 0x80) == (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  trace_output_16 (sd, tmp);
}

/* addb.  */
void
OP_31_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (GPR (OP[0]) & 0xff);
  uint8_t b = (GPR (OP[1]) & 0xff);
  uint16_t tmp = (a + b) & 0xff;
  trace_input ("addb", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  SET_PSR_C (tmp > 0xFF);
  SET_PSR_F (((a & 0x80) == (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  trace_output_16 (sd, tmp);
}

/* addw.  */
void
OP_32_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t a = OP[0];
  uint16_t tmp, b = GPR (OP[1]);
  tmp = (a + b);
  trace_input ("addw", OP_CONSTANT4_1, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFF);
  SET_PSR_F (((a & 0x8000) == (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  trace_output_16 (sd, tmp);
}

/* addw.  */
void
OP_32B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t a = OP[0];
  uint16_t tmp, b = GPR (OP[1]);
  tmp = (a + b);
  trace_input ("addw", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFF);
  SET_PSR_F (((a & 0x8000) == (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  trace_output_16 (sd, tmp);
}

/* addw.  */
void
OP_33_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (GPR (OP[0])), b = (GPR (OP[1]));
  trace_input ("addw", OP_REG, OP_REG, OP_VOID);
  tmp = (a + b);
  SET_GPR (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFF);
  SET_PSR_F (((a & 0x8000) == (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  trace_output_16 (sd, tmp);
}

/* addcb.  */
void
OP_34_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = OP[0] & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("addcb", OP_CONSTANT4_1, OP_REG, OP_REG);
  tmp = (a + b + PSR_C) & 0xff;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  SET_PSR_C (tmp > 0xFF);
  SET_PSR_F (((a & 0x80) == (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  trace_output_16 (sd, tmp);
}

/* addcb.  */
void
OP_34B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int8_t a = OP[0] & 0xff;
  uint8_t b = (GPR (OP[1])) & 0xff;
  uint8_t tmp = (a + b + PSR_C) & 0xff;
  trace_input ("addcb", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  SET_PSR_C (tmp > 0xFF);
  SET_PSR_F (((a & 0x80) == (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  trace_output_16 (sd, tmp);
}

/* addcb.  */
void
OP_35_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (GPR (OP[0])) & 0xff;
  uint8_t b = (GPR (OP[1])) & 0xff;
  uint8_t tmp = (a + b + PSR_C) & 0xff;
  trace_input ("addcb", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  SET_PSR_C (tmp > 0xFF);
  SET_PSR_F (((a & 0x80) == (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  trace_output_16 (sd, tmp);
}

/* addcw.  */
void
OP_36_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (a + b + PSR_C);
  trace_input ("addcw", OP_CONSTANT4_1, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFF);
  SET_PSR_F (((a & 0x8000) == (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  trace_output_16 (sd, tmp);
}

/* addcw.  */
void
OP_36B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (a + b + PSR_C);
  trace_input ("addcw", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFF);
  SET_PSR_F (((a & 0x8000) == (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  trace_output_16 (sd, tmp);
}

/* addcw.  */
void
OP_37_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[1]);
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (a + b + PSR_C);
  trace_input ("addcw", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFF);
  SET_PSR_F (((a & 0x8000) == (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  trace_output_16 (sd, tmp);
}

/* addd.  */
void
OP_60_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t a = (OP[0]);
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp = (a + b);
  trace_input ("addd", OP_CONSTANT4_1, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFFFFFF);
  SET_PSR_F (((a & 0x80000000) == (b & 0x80000000)) && ((b & 0x80000000) != (tmp & 0x80000000)));
  trace_output_32 (sd, tmp);
}

/* addd.  */
void
OP_60B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t a = (SEXT16(OP[0]));
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp = (a + b);
  trace_input ("addd", OP_CONSTANT16, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFFFFFF);
  SET_PSR_F (((a & 0x80000000) == (b & 0x80000000)) && ((b & 0x80000000) != (tmp & 0x80000000)));
  trace_output_32 (sd, tmp);
}

/* addd.  */
void
OP_61_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]);
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp = (a + b);
  trace_input ("addd", OP_REGP, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
  SET_PSR_C (tmp > 0xFFFFFFFF);
  SET_PSR_F (((a & 0x80000000) == (b & 0x80000000)) && ((b & 0x80000000) != (tmp & 0x80000000)));
}

/* addd.  */
void
OP_4_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = OP[0];
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp;
  trace_input ("addd", OP_CONSTANT20, OP_REGP, OP_VOID);
  tmp = (a + b);
  SET_GPR32 (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFFFFFF);
  SET_PSR_F (((a & 0x80000000) == (b & 0x80000000)) && ((b & 0x80000000) != (tmp & 0x80000000)));
  trace_output_32 (sd, tmp);
}

/* addd.  */
void
OP_2_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t a = OP[0];
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp;
  trace_input ("addd", OP_CONSTANT32, OP_REGP, OP_VOID);
  tmp = (a + b);
  SET_GPR32 (OP[1], tmp);
  SET_PSR_C (tmp > 0xFFFFFFFF);
  SET_PSR_F (((a & 0x80000000) == (b & 0x80000000)) && ((b & 0x80000000) != (tmp & 0x80000000)));
  trace_output_32 (sd, tmp);
}

/* andb.  */
void
OP_20_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("andb", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = a & b;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* andb.  */
void
OP_20B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("andb", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = a & b;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* andb.  */
void
OP_21_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (GPR (OP[0])) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("andb", OP_REG, OP_REG, OP_VOID);
  tmp = a & b;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* andw.  */
void
OP_22_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = OP[0], b = GPR (OP[1]);
  trace_input ("andw", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = a & b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* andw.  */
void
OP_22B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = OP[0], b = GPR (OP[1]);
  trace_input ("andw", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = a & b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* andw.  */
void
OP_23_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = GPR (OP[0]), b = GPR (OP[1]);
  trace_input ("andw", OP_REG, OP_REG, OP_VOID);
  tmp = a & b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* andd.  */
void
OP_4_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, a = OP[0],  b = GPR32 (OP[1]);
  trace_input ("andd", OP_CONSTANT32, OP_REGP, OP_VOID);
  tmp = a & b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* andd.  */
void
OP_14B_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, a = (GPR32 (OP[0])), b = (GPR32 (OP[1]));
  trace_input ("andd", OP_REGP, OP_REGP, OP_VOID);
  tmp = a & b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* ord.  */
void
OP_5_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, a = (OP[0]), b = GPR32 (OP[1]);
  trace_input ("ord", OP_CONSTANT32, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* ord.  */
void
OP_149_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, a = GPR32 (OP[0]), b = GPR32 (OP[1]);
  trace_input ("ord", OP_REGP, OP_REGP, OP_VOID);
  tmp = a | b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* xord.  */
void
OP_6_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, a = (OP[0]), b = GPR32 (OP[1]);
  trace_input ("xord", OP_CONSTANT32, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* xord.  */
void
OP_14A_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, a = GPR32 (OP[0]), b = GPR32 (OP[1]);
  trace_input ("xord", OP_REGP, OP_REGP, OP_VOID);
  tmp = a ^ b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}


/* b.  */
void
OP_1_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0, cc = cond_stat (OP[0]);
  trace_input ("b", OP_CONSTANT4, OP_DISPE9, OP_VOID);
  if  (cc)
    {
      if (sign_flag)
	tmp =  (PC - (OP[1]));
      else
        tmp =  (PC + (OP[1]));
      /* If the resulting PC value is less than 0x00_0000 or greater 
         than 0xFF_FFFF, this instruction causes an IAD trap.*/

      if ((tmp < 0x000000) || (tmp > 0xFFFFFF))
        {
          trace_output_void (sd);
          EXCEPTION (SIM_SIGBUS);
        }
      else
        JMP (tmp);
    }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_32 (sd, tmp);
}

/* b.  */
void
OP_18_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0, cc = cond_stat (OP[0]);
  trace_input ("b", OP_CONSTANT4, OP_DISP17, OP_VOID);
  if (cc)
    {
      if (sign_flag)
	tmp =  (PC - OP[1]);
      else
        tmp =  (PC + OP[1]);
      /* If the resulting PC value is less than 0x00_0000 or greater 
         than 0xFF_FFFF, this instruction causes an IAD trap.*/

      if ((tmp < 0x000000) || (tmp > 0xFFFFFF))
        {
          trace_output_void (sd);
          EXCEPTION (SIM_SIGBUS);
        }
      else
        JMP (tmp);
    }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_32 (sd, tmp);
}

/* b.  */
void
OP_10_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0, cc = cond_stat (OP[0]);
  trace_input ("b", OP_CONSTANT4, OP_DISP25, OP_VOID);
  if (cc)
    {
      if (sign_flag)
	tmp =  (PC - (OP[1]));
      else
        tmp =  (PC + (OP[1]));
      /* If the resulting PC value is less than 0x00_0000 or greater 
         than 0xFF_FFFF, this instruction causes an IAD trap.*/

      if ((tmp < 0x000000) || (tmp > 0xFFFFFF))
        {
          trace_output_void (sd);
          EXCEPTION (SIM_SIGBUS);
        }
      else
        JMP (tmp);
    }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_32 (sd, tmp);
}

/* bal.  */
void
OP_C0_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("bal", OP_REG, OP_DISP17, OP_VOID);
  tmp =  ((PC + 4) >> 1); /* Store PC in RA register. */
  SET_GPR32 (14, tmp);
  if (sign_flag)
    tmp =  (PC - (OP[1]));
  else
    tmp =  (PC + (OP[1]));

  /* If the resulting PC value is less than 0x00_0000 or greater 
     than 0xFF_FFFF, this instruction causes an IAD trap.  */

  if ((tmp < 0x000000) || (tmp > 0xFFFFFF))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }
  else
    JMP (tmp);
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_32 (sd, tmp);
}


/* bal.  */
void
OP_102_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("bal", OP_REGP, OP_DISP25, OP_VOID);
  tmp = (((PC) + 4) >> 1); /* Store PC in reg pair.  */
  SET_GPR32 (OP[0], tmp);
  if (sign_flag)
    tmp =  ((PC) - (OP[1]));
  else
    tmp =  ((PC) + (OP[1]));
  /* If the resulting PC value is less than 0x00_0000 or greater 
     than 0xFF_FFFF, this instruction causes an IAD trap.*/

  if ((tmp < 0x000000) || (tmp > 0xFFFFFF))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }
  else
    JMP (tmp);
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_32 (sd, tmp);
}

/* jal.  */
void
OP_148_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("jal", OP_REGP, OP_REGP, OP_VOID);
  SET_GPR32 (OP[0], (((PC) + 4) >> 1)); /* Store next PC in RA */
  tmp = GPR32 (OP[1]);
  tmp = SEXT24(tmp << 1);
  /* If the resulting PC value is less than 0x00_0000 or greater 
     than 0xFF_FFFF, this instruction causes an IAD trap.*/

  if ((tmp < 0x0) || (tmp > 0xFFFFFF))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }
  else
    JMP (tmp);

  trace_output_32 (sd, tmp);
}


/* jal.  */
void
OP_D_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("jal", OP_REGP, OP_VOID, OP_VOID);
  SET_GPR32 (14, (((PC) + 2) >> 1)); /* Store next PC in RA */
  tmp = GPR32 (OP[0]);
  tmp = SEXT24(tmp << 1);
  /* If the resulting PC value is less than 0x00_0000 or greater 
     than 0xFF_FFFF, this instruction causes an IAD trap.*/

  if ((tmp < 0x0) || (tmp > 0xFFFFFF))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }
  else
    JMP (tmp);

  trace_output_32 (sd, tmp);
}


/* beq0b.  */
void
OP_C_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint8_t a = (GPR (OP[0]) & 0xFF);
  trace_input ("beq0b", OP_REG, OP_DISP5, OP_VOID);
  addr = OP[1];
  if (a == 0) 
  {
    if (sign_flag)
      addr = (PC - OP[1]);
    else
      addr = (PC + OP[1]);

    JMP (addr);
  }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_void (sd);
}

/* bne0b.  */
void
OP_D_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint8_t a = (GPR (OP[0]) & 0xFF);
  trace_input ("bne0b", OP_REG, OP_DISP5, OP_VOID);
  addr = OP[1];
  if (a != 0) 
  {
    if (sign_flag)
      addr = (PC - OP[1]);
    else
      addr = (PC + OP[1]);

    JMP (addr);
  }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_void (sd);
}

/* beq0w.  */
void
OP_E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = GPR (OP[0]);
  trace_input ("beq0w", OP_REG, OP_DISP5, OP_VOID);
  addr = OP[1];
  if (a == 0) 
  {
    if (sign_flag)
      addr = (PC - OP[1]);
    else
      addr = (PC + OP[1]);

    JMP (addr);
  }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_void (sd);
}

/* bne0w.  */
void
OP_F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = GPR (OP[0]);
  trace_input ("bne0w", OP_REG, OP_DISP5, OP_VOID);
  addr = OP[1];
  if (a != 0) 
  {
    if (sign_flag)
      addr = (PC - OP[1]);
    else
      addr = (PC + OP[1]);

    JMP (addr);
  }
  sign_flag = 0; /* Reset sign_flag.  */
  trace_output_void (sd);
}


/* jeq.  */
void
OP_A0_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jeq", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_Z) == 1)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits.  */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit. */
  }
  trace_output_32 (sd, tmp);
}

/* jne.  */
void
OP_A1_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jne", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_Z) == 0)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits.  */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit. */
  }
  trace_output_32 (sd, tmp);
}

/* jcs.  */
void
OP_A2_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jcs", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_C) == 1)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jcc.  */
void
OP_A3_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jcc", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_C) == 0)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jhi.  */
void
OP_A4_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jhi", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_L) == 1)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jls.  */
void
OP_A5_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jls", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_L) == 0)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jgt.  */
void
OP_A6_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jgt", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_N) == 1)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jle.  */
void
OP_A7_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jle", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_N) == 0)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}


/* jfs.  */
void
OP_A8_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jfs", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_F) == 1)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jfc.  */
void
OP_A9_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jfc", OP_REGP, OP_VOID, OP_VOID);
  if ((PSR_F) == 0)
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jlo.  */
void
OP_AA_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jlo", OP_REGP, OP_VOID, OP_VOID);
  if (((PSR_Z) == 0) & ((PSR_L) == 0))
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jhs.  */
void
OP_AB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jhs", OP_REGP, OP_VOID, OP_VOID);
  if (((PSR_Z) == 1) | ((PSR_L) == 1))
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jlt.  */
void
OP_AC_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jlt", OP_REGP, OP_VOID, OP_VOID);
  if (((PSR_Z) == 0) & ((PSR_N) == 0))
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jge.  */
void
OP_AD_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = 0;
  trace_input ("jge", OP_REGP, OP_VOID, OP_VOID);
  if (((PSR_Z) == 1) | ((PSR_N) == 1))
  {
     tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
     JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  }
  trace_output_32 (sd, tmp);
}

/* jump.  */
void
OP_AE_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("jump", OP_REGP, OP_VOID, OP_VOID);
  tmp = GPR32 (OP[0]) /*& 0x3fffff*/; /* Use only 0 - 22 bits */
  JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  trace_output_32 (sd, tmp);
}

/* jusr.  */
void
OP_AF_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("jusr", OP_REGP, OP_VOID, OP_VOID);
  tmp = (GPR32 (OP[0])) & 0x3fffff; /* Use only 0 - 22 bits */
  JMP (tmp << 1); /* Set PC's 1 - 23 bits and clear 0th bit*/
  SET_PSR_U(1);
  trace_output_32 (sd, tmp);
}

/* seq.  */
void
OP_80_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("seq", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_Z) == 1)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}
/* sne.  */
void
OP_81_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sne", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_Z) == 0)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* scs.  */
void
OP_82_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("scs", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_C) == 1)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* scc.  */
void
OP_83_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("scc", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_C) == 0)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* shi.  */
void
OP_84_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("shi", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_L) == 1)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* sls.  */
void
OP_85_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sls", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_L) == 0)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* sgt.  */
void
OP_86_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sgt", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_N) == 1)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* sle.  */
void
OP_87_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sle", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_N) == 0)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* sfs.  */
void
OP_88_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sfs", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_F) == 1)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* sfc.  */
void
OP_89_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sfc", OP_REG, OP_VOID, OP_VOID);
  if ((PSR_F) == 0)
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}


/* slo.  */
void
OP_8A_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("slo", OP_REG, OP_VOID, OP_VOID);
  if (((PSR_Z) == 0) & ((PSR_L) == 0))
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* shs.  */
void
OP_8B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("shs", OP_REG, OP_VOID, OP_VOID);
  if ( ((PSR_Z) == 1) | ((PSR_L) == 1))
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* slt.  */
void
OP_8C_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("slt", OP_REG, OP_VOID, OP_VOID);
  if (((PSR_Z) == 0) & ((PSR_N) == 0))
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* sge.  */
void
OP_8D_C (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("sge", OP_REG, OP_VOID, OP_VOID);
  if (((PSR_Z) == 1) | ((PSR_N) == 1))
     SET_GPR (OP[0], 1);
  else
     SET_GPR (OP[0], 0);
  trace_output_void (sd);
}

/* cbitb.  */
void
OP_D7_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* cbitb.  */
void
OP_107_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* cbitb.  */
void
OP_68_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_R_INDEX7_ABS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitb.  */
void
OP_1AA_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_RP_INDEX_DISP14, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitb.  */
void
OP_104_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitb.  */
void
OP_D4_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitb.  */
void
OP_D6_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);

}

/* cbitb.  */
void
OP_105_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitb.  */
void
OP_106_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitb", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, addr);
}


/* cbitw.  */
void
OP_6F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* cbitw.  */
void
OP_117_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* cbitw.  */
void
OP_36_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = (OP[0]), tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);

}

/* cbitw.  */
void
OP_1AB_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_RP_INDEX_DISP14, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitw.  */
void
OP_114_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}


/* cbitw.  */
void
OP_6E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitw.  */
void
OP_69_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}


/* cbitw.  */
void
OP_115_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* cbitw.  */
void
OP_116_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("cbitw", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp & ~(1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitb.  */
void
OP_E7_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitb.  */
void
OP_10B_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitb.  */
void
OP_70_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_R_INDEX7_ABS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitb.  */
void
OP_1CA_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_RP_INDEX_DISP14, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitb.  */
void
OP_108_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}


/* sbitb.  */
void
OP_E4_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitb.  */
void
OP_E6_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}


/* sbitb.  */
void
OP_109_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}


/* sbitb.  */
void
OP_10A_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitb", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SB (addr, tmp);
  trace_output_32 (sd, tmp);
}


/* sbitw.  */
void
OP_77_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitw.  */
void
OP_11B_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, tmp);
}

/* sbitw.  */
void
OP_3A_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = (OP[0]), tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitw.  */
void
OP_1CB_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_RP_INDEX_DISP14, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitw.  */
void
OP_118_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitw.  */
void
OP_76_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitw.  */
void
OP_71_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitw.  */
void
OP_119_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}

/* sbitw.  */
void
OP_11A_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("sbitw", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  tmp = tmp | (1 << a);
  SW (addr, tmp);
  trace_output_32 (sd, addr);
}


/* tbitb.  */
void
OP_F7_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, tmp);
}

/* tbitb.  */
void
OP_10F_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xff;
  uint32_t addr = OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, tmp);
}

/* tbitb.  */
void
OP_78_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_R_INDEX7_ABS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitb.  */
void
OP_1EA_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_RP_INDEX_DISP14, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitb.  */
void
OP_10C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitb.  */
void
OP_F4_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitb.  */
void
OP_F6_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitb.  */
void
OP_10D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitb.  */
void
OP_10E_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitb", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  tmp = RB (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}


/* tbitw.  */
void
OP_7F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, tmp);
}

/* tbitw.  */
void
OP_11F_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, tmp);
}


/* tbitw.  */
void
OP_3E_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = (OP[0]), tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitw.  */
void
OP_1EB_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_RP_INDEX_DISP14, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitw.  */
void
OP_11C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR (OP[2])) + OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitw.  */
void
OP_7E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitw.  */
void
OP_79_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}

/* tbitw.  */
void
OP_11D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}


/* tbitw.  */
void
OP_11E_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1], tmp;
  trace_input ("tbitw", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  tmp = RW (addr);
  SET_PSR_F (tmp & (1 << a));
  trace_output_32 (sd, addr);
}


/* tbit.  */
void
OP_6_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = (GPR (OP[1]));
  trace_input ("tbit", OP_CONSTANT4, OP_REG, OP_VOID);
  SET_PSR_F (b & (1 << a));
  trace_output_16 (sd, b);
}

/* tbit.  */
void
OP_7_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint16_t b = (GPR (OP[1]));
  trace_input ("tbit", OP_REG, OP_REG, OP_VOID);
  SET_PSR_F (b & (1 << a));
  trace_output_16 (sd, b);
}


/* cmpb.  */
void
OP_50_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xFF; 
  uint8_t b = (GPR (OP[1])) & 0xFF;
  trace_input ("cmpb", OP_CONSTANT4, OP_REG, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int8_t)a > (int8_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpb.  */
void
OP_50B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xFF; 
  uint8_t b = (GPR (OP[1])) & 0xFF;
  trace_input ("cmpb", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int8_t)a > (int8_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpb.  */
void
OP_51_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (GPR (OP[0])) & 0xFF; 
  uint8_t b = (GPR (OP[1])) & 0xFF;
  trace_input ("cmpb", OP_REG, OP_REG, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int8_t)a > (int8_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpw.  */
void
OP_52_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]); 
  uint16_t b = GPR (OP[1]);
  trace_input ("cmpw", OP_CONSTANT4, OP_REG, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int16_t)a > (int16_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpw.  */
void
OP_52B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (OP[0]); 
  uint16_t b = GPR (OP[1]);
  trace_input ("cmpw", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int16_t)a > (int16_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpw.  */
void
OP_53_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]) ; 
  uint16_t b = GPR (OP[1]) ;
  trace_input ("cmpw", OP_REG, OP_REG, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int16_t)a > (int16_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpd.  */
void
OP_56_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = (OP[0]); 
  uint32_t b = GPR32 (OP[1]);
  trace_input ("cmpd", OP_CONSTANT4, OP_REGP, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int32_t)a > (int32_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpd.  */
void
OP_56B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = (SEXT16(OP[0])); 
  uint32_t b = GPR32 (OP[1]);
  trace_input ("cmpd", OP_CONSTANT16, OP_REGP, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int32_t)a > (int32_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpd.  */
void
OP_57_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]) ; 
  uint32_t b = GPR32 (OP[1]) ;
  trace_input ("cmpd", OP_REGP, OP_REGP, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int32_t)a > (int32_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}

/* cmpd.  */
void
OP_9_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = (OP[0]); 
  uint32_t b = GPR32 (OP[1]);
  trace_input ("cmpd", OP_CONSTANT32, OP_REGP, OP_VOID);
  SET_PSR_Z (a == b);
  SET_PSR_N ((int32_t)a > (int32_t)b);
  SET_PSR_L (a > b);
  trace_output_flag (sd);
}


/* movb.  */
void
OP_58_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp = OP[0] & 0xFF;
  uint16_t a = (GPR (OP[1])) & 0xFF00;
  trace_input ("movb", OP_CONSTANT4, OP_REG, OP_VOID);
  SET_GPR (OP[1], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* movb.  */
void
OP_58B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp = OP[0] & 0xFF;
  uint16_t a = (GPR (OP[1])) & 0xFF00;
  trace_input ("movb", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* movb.  */
void
OP_59_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp = (GPR (OP[0])) & 0xFF;
  uint16_t a = (GPR (OP[1])) & 0xFF00;
  trace_input ("movb", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* movw.  */
void
OP_5A_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp = OP[0];
  trace_input ("movw", OP_CONSTANT4_1, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* movw.  */
void
OP_5AB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp = OP[0];
  trace_input ("movw", OP_CONSTANT16, OP_REG, OP_VOID);
  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* movw.  */
void
OP_5B_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp = GPR (OP[0]);
  uint32_t a = GPR32 (OP[1]);
  trace_input ("movw", OP_REG, OP_REGP, OP_VOID);
  a = (a & 0xffff0000) | tmp;
  SET_GPR32 (OP[1], a);
  trace_output_16 (sd, tmp);
}

/* movxb.  */
void
OP_5C_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp = (GPR (OP[0])) & 0xFF;
  trace_input ("movxb", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], ((SEXT8(tmp)) & 0xffff));
  trace_output_16 (sd, tmp);
}

/* movzb.  */
void
OP_5D_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp = (GPR (OP[0])) & 0xFF;
  trace_input ("movzb", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1],  tmp);
  trace_output_16 (sd, tmp);
}

/* movxw.  */
void
OP_5E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp = GPR (OP[0]);
  trace_input ("movxw", OP_REG, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], SEXT16(tmp));
  trace_output_16 (sd, tmp);
}

/* movzw.  */
void
OP_5F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp = GPR (OP[0]);
  trace_input ("movzw", OP_REG, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], (tmp & 0x0000FFFF));
  trace_output_16 (sd, tmp);
}

/* movd.  */
void
OP_54_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp = OP[0];
  trace_input ("movd", OP_CONSTANT4, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* movd.  */
void
OP_54B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp = SEXT16(OP[0]);
  trace_input ("movd", OP_CONSTANT16, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* movd.  */
void
OP_55_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = GPR32 (OP[0]);
  trace_input ("movd", OP_REGP, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* movd.  */
void
OP_5_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp = OP[0];
  trace_input ("movd", OP_CONSTANT20, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* movd.  */
void
OP_7_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp = OP[0];
  trace_input ("movd", OP_CONSTANT32, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* loadm.  */
void
OP_14_D (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr = GPR (0);
  uint16_t count = OP[0], reg = 2, tmp;
  trace_input ("loadm", OP_CONSTANT4, OP_VOID, OP_VOID);
  if ((addr & 1))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }

  while (count)
    {
      tmp = RW (addr);
      SET_GPR (reg, tmp);
      addr +=2;
      --count;
      reg++;
      if (reg == 6) reg = 8;
    };

  SET_GPR (0, addr);
  trace_output_void (sd);
}


/* loadmp.  */
void
OP_15_D (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr = GPR32 (0);
  uint16_t count = OP[0], reg = 2, tmp;
  trace_input ("loadm", OP_CONSTANT4, OP_VOID, OP_VOID);
  if ((addr & 1))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }

  while (count)
    {
      tmp = RW (addr);
      SET_GPR (reg, tmp);
      addr +=2;
      --count;
      reg++;
      if (reg == 6) reg = 8;
    };

  SET_GPR32 (0, addr);
  trace_output_void (sd);
}


/* loadb.  */
void
OP_88_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb ABS20, REG 
   * ADDR = zext24(abs20) | remap (ie 0xF00000)
   * REG  = [ADDR] 
   * NOTE: remap is 
   * If (abs20 > 0xEFFFF) the resulting address is logically ORed 
   * with 0xF00000 i.e. addresses from 1M-64k to 1M are re-mapped 
   * by the core to 16M-64k to 16M. */

  uint16_t tmp, a = (GPR (OP[1])) & 0xFF00;
  uint32_t addr = OP[0];
  trace_input ("loadb", OP_ABS20, OP_REG, OP_VOID);
  if (addr > 0xEFFFF) addr |= 0xF00000;
  tmp = (RB (addr));
  SET_GPR (OP[1], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_127_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb ABS24, REG 
   * ADDR = abs24
   * REGR = [ADDR].   */

  uint16_t tmp, a = (GPR (OP[1])) & 0xFF00;
  uint32_t addr = OP[0];
  trace_input ("loadb", OP_ABS24, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[1], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_45_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb [Rindex]ABS20   REG
   * ADDR = Rindex + zext24(disp20)
   * REGR = [ADDR].   */

  uint32_t addr;
  uint16_t tmp, a = (GPR (OP[2])) & 0xFF00;
  trace_input ("loadb", OP_R_INDEX8_ABS20, OP_REG, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + OP[1];
  else
     addr = (GPR32 (13)) + OP[1];

  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}


/* loadb.  */
void
OP_B_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb DIPS4(REGP)   REG 
   * ADDR = RPBASE + zext24(DISP4)
   * REG = [ADDR].  */
  uint16_t tmp, a = (GPR (OP[2])) & 0xFF00;
  uint32_t addr = (GPR32 (OP[1])) + OP[0];
  trace_input ("loadb", OP_RP_BASE_DISP4, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_BE_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb [Rindex]disp0(RPbasex) REG
   * ADDR = Rpbasex + Rindex
   * REGR = [ADDR]   */

  uint32_t addr;
  uint16_t tmp, a = (GPR (OP[3])) & 0xFF00;
  trace_input ("loadb", OP_RP_INDEX_DISP0, OP_REG, OP_VOID);

  addr =  (GPR32 (OP[2])) + OP[1];

  if (OP[0] == 0)
     addr = (GPR32 (12)) + addr;
  else
     addr = (GPR32 (13)) + addr;

  tmp = (RB (addr));
  SET_GPR (OP[3], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_219_A (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb [Rindex]disp14(RPbasex) REG
   * ADDR = Rpbasex + Rindex + zext24(disp14)
   * REGR = [ADDR]   */

  uint32_t addr;
  uint16_t tmp, a = (GPR (OP[3])) & 0xFF00;

  addr =  (GPR32 (OP[2])) + OP[1];

  if (OP[0] == 0)
     addr = (GPR32 (12)) + addr;
  else
     addr = (GPR32 (13)) + addr;

  trace_input ("loadb", OP_RP_INDEX_DISP14, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[3], (a | tmp));
  trace_output_16 (sd, tmp);
}


/* loadb.  */
void
OP_184_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb DISPE20(REG)   REG
   * zext24(Rbase) + zext24(dispe20)
   * REG = [ADDR]   */

  uint16_t tmp,a = (GPR (OP[2])) & 0xFF00;
  uint32_t addr = OP[0] + (GPR (OP[1]));
  trace_input ("loadb", OP_R_BASE_DISPE20, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_124_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb DISP20(REG)   REG
   * ADDR = zext24(Rbase) + zext24(disp20)
   * REG = [ADDR]                          */

  uint16_t tmp,a = (GPR (OP[2])) & 0xFF00;
  uint32_t addr = OP[0] + (GPR (OP[1]));
  trace_input ("loadb", OP_R_BASE_DISP20, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_BF_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb disp16(REGP)   REG
   * ADDR = RPbase + zext24(disp16)
   * REGR = [ADDR]   */

  uint16_t tmp,a = (GPR (OP[2])) & 0xFF00;
  uint32_t addr = (GPR32 (OP[1])) + OP[0];
  trace_input ("loadb", OP_RP_BASE_DISP16, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_125_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb disp20(REGP)   REG
   * ADDR = RPbase + zext24(disp20)
   * REGR = [ADDR]   */
  uint16_t tmp,a = (GPR (OP[2])) & 0xFF00;
  uint32_t addr =  (GPR32 (OP[1])) + OP[0];
  trace_input ("loadb", OP_RP_BASE_DISP20, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}


/* loadb.  */
void
OP_185_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb -disp20(REGP)   REG
   * ADDR = RPbase + zext24(-disp20)
   * REGR = [ADDR]   */
  uint16_t tmp,a = (GPR (OP[2])) & 0xFF00;
  uint32_t addr =  (GPR32 (OP[1])) + OP[1];
  trace_input ("loadb", OP_RP_BASE_DISPE20, OP_REG, OP_VOID);
  tmp = (RB (addr));
  SET_GPR (OP[2], (a | tmp));
  trace_output_16 (sd, tmp);
}

/* loadb.  */
void
OP_126_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadb [Rindex]disp20(RPbasexb) REG
   * ADDR = RPbasex + Rindex + zext24(disp20)
   * REGR = [ADDR]   */

  uint32_t addr;
  uint16_t tmp, a = (GPR (OP[3])) & 0xFF00;
  trace_input ("loadb", OP_RP_INDEX_DISP20, OP_REG, OP_VOID);

  addr = (GPR32 (OP[2])) + OP[1];

  if (OP[0] == 0)
     addr = (GPR32 (12)) + addr;
  else
     addr = (GPR32 (13)) + addr;

  tmp = (RB (addr));
  SET_GPR (OP[3], (a | tmp));
  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_89_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw ABS20, REG 
   * ADDR = zext24(abs20) | remap
   * REGR = [ADDR] 
   * NOTE: remap is 
   * If (abs20 > 0xEFFFF) the resulting address is logically ORed 
   * with 0xF00000 i.e. addresses from 1M-64k to 1M are re-mapped 
   * by the core to 16M-64k to 16M. */

  uint16_t tmp; 
  uint32_t addr = OP[0];
  trace_input ("loadw", OP_ABS20, OP_REG, OP_VOID);
  if (addr > 0xEFFFF) addr |= 0xF00000;
  tmp = (RW (addr));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_12F_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw ABS24, REG 
   * ADDR = abs24
   * REGR = [ADDR]  */
  uint16_t tmp; 
  uint32_t addr = OP[0];
  trace_input ("loadw", OP_ABS24, OP_REG, OP_VOID);
  tmp = (RW (addr));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* loadw.  */
void
OP_47_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw [Rindex]ABS20   REG
   * ADDR = Rindex + zext24(disp20)
   * REGR = [ADDR]  */

  uint32_t addr;
  uint16_t tmp; 
  trace_input ("loadw", OP_R_INDEX8_ABS20, OP_REG, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + OP[1];
  else
     addr = (GPR32 (13)) + OP[1];

  tmp = (RW (addr));
  SET_GPR (OP[2], tmp);
  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_9_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw DIPS4(REGP)   REGP
   * ADDR = RPBASE + zext24(DISP4)
   * REGP = [ADDR].  */
  uint16_t tmp;
  uint32_t addr, a;
  trace_input ("loadw", OP_RP_BASE_DISP4, OP_REG, OP_VOID);
  addr = (GPR32 (OP[1])) + OP[0];
  tmp =  (RW (addr));
  if (OP[2] > 11)
   {
    a = (GPR32 (OP[2])) & 0xffff0000;
    SET_GPR32 (OP[2], (a | tmp));
   }
  else
    SET_GPR (OP[2], tmp);

  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_9E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw [Rindex]disp0(RPbasex) REG
   * ADDR = Rpbasex + Rindex
   * REGR = [ADDR]   */

  uint32_t addr;
  uint16_t tmp;
  trace_input ("loadw", OP_RP_INDEX_DISP0, OP_REG, OP_VOID);

  addr = (GPR32 (OP[2])) + OP[1];

  if (OP[0] == 0)
    addr = (GPR32 (12)) + addr;
  else
    addr = (GPR32 (13)) + addr;

  tmp = RW (addr);
  SET_GPR (OP[3], tmp);
  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_21B_A (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw [Rindex]disp14(RPbasex) REG
   * ADDR = Rpbasex + Rindex + zext24(disp14)
   * REGR = [ADDR]   */

  uint32_t addr;
  uint16_t tmp;
  trace_input ("loadw", OP_RP_INDEX_DISP14, OP_REG, OP_VOID);
  addr =  (GPR32 (OP[2])) + OP[1];

  if (OP[0] == 0)
     addr = (GPR32 (12)) + addr;
  else
     addr = (GPR32 (13)) + addr;

  tmp = (RW (addr));
  SET_GPR (OP[3], tmp);
  trace_output_16 (sd, tmp);
}

/* loadw.  */
void
OP_18C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw dispe20(REG)   REGP
   * REGP = [DISPE20+[REG]]   */

  uint16_t tmp;
  uint32_t addr, a; 
  trace_input ("loadw", OP_R_BASE_DISPE20, OP_REGP, OP_VOID);
  addr = OP[0] + (GPR (OP[1]));
  tmp = (RW (addr));
  if (OP[2] > 11)
   {
    a = (GPR32 (OP[2])) & 0xffff0000;
    SET_GPR32 (OP[2], (a | tmp));
   }
  else
    SET_GPR (OP[2], tmp);
   
  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_12C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw DISP20(REG)   REGP
   * ADDR = zext24(Rbase) + zext24(disp20)
   * REGP = [ADDR]                          */

  uint16_t tmp;
  uint32_t addr, a;
  trace_input ("loadw", OP_R_BASE_DISP20, OP_REGP, OP_VOID);
  addr = OP[0] + (GPR (OP[1]));
  tmp = (RW (addr));
  if (OP[2] > 11)
   {
    a = (GPR32 (OP[2])) & 0xffff0000;
    SET_GPR32 (OP[2], (a | tmp));
   }
  else
    SET_GPR (OP[2], tmp);

  trace_output_16 (sd, tmp);
}

/* loadw.  */
void
OP_9F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw disp16(REGP)   REGP
   * ADDR = RPbase + zext24(disp16)
   * REGP = [ADDR]   */
  uint16_t tmp;
  uint32_t addr, a;
  trace_input ("loadw", OP_RP_BASE_DISP16, OP_REGP, OP_VOID);
  addr = (GPR32 (OP[1])) + OP[0];
  tmp = (RW (addr));
  if (OP[2] > 11)
   {
    a = (GPR32 (OP[2])) & 0xffff0000;
    SET_GPR32 (OP[2], (a | tmp));
   }
  else
    SET_GPR (OP[2], tmp);

  trace_output_16 (sd, tmp);
}

/* loadw.  */
void
OP_12D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw disp20(REGP)   REGP
   * ADDR = RPbase + zext24(disp20)
   * REGP = [ADDR]   */
  uint16_t tmp;
  uint32_t addr, a;
  trace_input ("loadw", OP_RP_BASE_DISP20, OP_REG, OP_VOID);
  addr = (GPR32 (OP[1])) + OP[0];
  tmp = (RW (addr));
  if (OP[2] > 11)
   {
    a = (GPR32 (OP[2])) & 0xffff0000;
    SET_GPR32 (OP[2], (a | tmp));
   }
  else
    SET_GPR (OP[2], tmp);

  trace_output_16 (sd, tmp);
}

/* loadw.  */
void
OP_18D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw -disp20(REGP)   REG
   * ADDR = RPbase + zext24(-disp20)
   * REGR = [ADDR]   */

  uint16_t tmp;
  uint32_t addr, a;
  trace_input ("loadw", OP_RP_BASE_DISPE20, OP_REG, OP_VOID);
  addr = (GPR32 (OP[1])) + OP[0];
  tmp = (RB (addr));
  if (OP[2] > 11)
   {
    a = (GPR32 (OP[2])) & 0xffff0000;
    SET_GPR32 (OP[2], (a | tmp));
   }
  else
    SET_GPR (OP[2], tmp);

  trace_output_16 (sd, tmp);
}


/* loadw.  */
void
OP_12E_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadw [Rindex]disp20(RPbasexb) REG
   * ADDR = RPbasex + Rindex + zext24(disp20)
   * REGR = [ADDR]   */

  uint32_t addr;
  uint16_t tmp;
  trace_input ("loadw", OP_RP_INDEX_DISP20, OP_REG, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + OP[1] + (GPR32 (OP[2]));
  else
     addr = (GPR32 (13)) + OP[1] + (GPR32 (OP[2]));

  tmp = (RW (addr));
  SET_GPR (OP[3], tmp);
  trace_output_16 (sd, tmp);
}


/* loadd.  */
void
OP_87_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd ABS20, REGP
   * ADDR = zext24(abs20) | remap
   * REGP = [ADDR] 
   * NOTE: remap is 
   * If (abs20 > 0xEFFFF) the resulting address is logically ORed 
   * with 0xF00000 i.e. addresses from 1M-64k to 1M are re-mapped 
   * by the core to 16M-64k to 16M. */

  uint32_t addr, tmp;
  addr = OP[0];
  trace_input ("loadd", OP_ABS20, OP_REGP, OP_VOID);
  if (addr > 0xEFFFF) addr |= 0xF00000;
  tmp = RLW (addr);
  tmp = ((tmp << 16) & 0xffff)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* loadd.  */
void
OP_12B_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd ABS24, REGP
   * ADDR = abs24
   * REGP = [ADDR]  */

  uint32_t addr = OP[0];
  uint32_t tmp;
  trace_input ("loadd", OP_ABS24, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[1],tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_46_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd [Rindex]ABS20   REGP
   * ADDR = Rindex + zext24(disp20)
   * REGP = [ADDR]  */

  uint32_t addr, tmp;
  trace_input ("loadd", OP_R_INDEX8_ABS20, OP_REGP, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + OP[1];
  else
     addr = (GPR32 (13)) + OP[1];

  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_A_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd dips4(regp)   REGP 
   * ADDR = Rpbase + zext24(disp4)
   * REGP = [ADDR] */

  uint32_t tmp, addr = (GPR32 (OP[1])) + OP[0];
  trace_input ("loadd", OP_RP_BASE_DISP4, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_AE_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd [Rindex]disp0(RPbasex) REGP
   * ADDR = Rpbasex + Rindex
   * REGP = [ADDR]   */

  uint32_t addr, tmp;
  trace_input ("loadd", OP_RP_INDEX_DISP0, OP_REGP, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + (GPR32 (OP[2])) + OP[1];
  else
     addr = (GPR32 (13)) + (GPR32 (OP[2])) + OP[1];

  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[3], tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_21A_A (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd [Rindex]disp14(RPbasex) REGP
   * ADDR = Rpbasex + Rindex + zext24(disp14)
   * REGR = [ADDR]   */

  uint32_t addr, tmp;
  trace_input ("loadd", OP_RP_INDEX_DISP14, OP_REGP, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + OP[1] + (GPR32 (OP[2]));
  else
     addr = (GPR32 (13)) + OP[1] + (GPR32 (OP[2]));

  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR (OP[3],tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_188_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd dispe20(REG)   REG
   * zext24(Rbase) + zext24(dispe20)
   * REG = [ADDR]   */

  uint32_t tmp, addr = OP[0] + (GPR (OP[1]));
  trace_input ("loadd", OP_R_BASE_DISPE20, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_128_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd DISP20(REG)   REG
   * ADDR = zext24(Rbase) + zext24(disp20)
   * REG = [ADDR]                          */

  uint32_t tmp, addr = OP[0] + (GPR (OP[1]));
  trace_input ("loadd", OP_R_BASE_DISP20, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}

/* loadd.  */
void
OP_AF_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd disp16(REGP)   REGP
   * ADDR = RPbase + zext24(disp16)
   * REGR = [ADDR]   */
  uint32_t tmp, addr = OP[0] + (GPR32 (OP[1]));
  trace_input ("loadd", OP_RP_BASE_DISP16, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}


/* loadd.  */
void
OP_129_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd disp20(REGP)   REGP
   * ADDR = RPbase + zext24(disp20)
   * REGP = [ADDR]   */
  uint32_t tmp, addr = OP[0] + (GPR32 (OP[1]));
  trace_input ("loadd", OP_RP_BASE_DISP20, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}

/* loadd.  */
void
OP_189_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd -disp20(REGP)   REGP
   * ADDR = RPbase + zext24(-disp20)
   * REGP = [ADDR]   */

  uint32_t tmp, addr = OP[0] + (GPR32 (OP[1]));
  trace_input ("loadd", OP_RP_BASE_DISPE20, OP_REGP, OP_VOID);
  tmp = RLW (addr);
  tmp = ((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}

/* loadd.  */
void
OP_12A_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* loadd [Rindex]disp20(RPbasexb) REGP
   * ADDR = RPbasex + Rindex + zext24(disp20)
   * REGP = [ADDR]   */

  uint32_t addr, tmp;
  trace_input ("loadd", OP_RP_INDEX_DISP20, OP_REGP, OP_VOID);

  if (OP[0] == 0)
     addr = (GPR32 (12)) + OP[1] + (GPR32 (OP[2]));
  else
     addr = (GPR32 (13)) + OP[1] + (GPR32 (OP[2])); 

  tmp = RLW (addr);
  tmp = ((tmp << 16) & 0xffff)| ((tmp >> 16) & 0xffff);
  SET_GPR32 (OP[3], tmp);
  trace_output_32 (sd, tmp);
}


/* storb.  */
void
OP_C8_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, ABS20
   * ADDR = zext24(abs20) | remap
   * [ADDR] = REGR 
   * NOTE: remap is
   * If (abs20 > 0xEFFFF) the resulting address is logically ORed
   * with 0xF00000 i.e. addresses from 1M-64k to 1M are re-mapped
   * by the core to 16M-64k to 16M. */

  uint8_t a = ((GPR (OP[0])) & 0xff);
  uint32_t addr =  OP[1];
  trace_input ("storb", OP_REG, OP_ABS20_OUTPUT, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_137_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, ABS24
   * ADDR = abs24
   * [ADDR] = REGR.  */

  uint8_t a = ((GPR (OP[0])) & 0xff);
  uint32_t addr =  OP[1];
  trace_input ("storb", OP_REG, OP_ABS24_OUTPUT, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_65_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, [Rindex]ABS20 
   * ADDR = Rindex + zext24(disp20)
   * [ADDR] = REGR  */

  uint32_t addr;
  uint8_t a = ((GPR (OP[0])) & 0xff);
  trace_input ("storb", OP_REG, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_F_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, DIPS4(REGP)
   * ADDR = RPBASE + zext24(DISP4)
   * [ADDR]  = REG.  */

  uint16_t a = ((GPR (OP[0])) & 0xff);
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_REG, OP_RP_BASE_DISPE4, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_FE_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb [Rindex]disp0(RPbasex) REG
   * ADDR = Rpbasex + Rindex
   * [ADDR] = REGR   */

  uint32_t addr;
  uint8_t a = ((GPR (OP[0])) & 0xff);
  trace_input ("storb", OP_REG, OP_RP_INDEX_DISP0, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + (GPR32 (OP[3])) + OP[2];
  else
     addr = (GPR32 (13)) + (GPR32 (OP[3])) + OP[2];

  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_319_A (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, [Rindex]disp14(RPbasex)
   * ADDR = Rpbasex + Rindex + zext24(disp14)
   * [ADDR] = REGR  */

  uint8_t a = ((GPR (OP[0])) & 0xff);
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_REG, OP_RP_INDEX_DISP14, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_194_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, DISPE20(REG) 
   * zext24(Rbase) + zext24(dispe20)
   * [ADDR] = REG  */

  uint8_t a = ((GPR (OP[0])) & 0xff);
  uint32_t addr = OP[1] + (GPR (OP[2]));
  trace_input ("storb", OP_REG, OP_R_BASE_DISPE20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_134_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, DISP20(REG)
   * ADDR = zext24(Rbase) + zext24(disp20)
   * [ADDR] = REG                          */

  uint8_t a = (GPR (OP[0]) & 0xff);
  uint32_t addr = OP[1] + (GPR (OP[2]));
  trace_input ("storb", OP_REG, OP_R_BASE_DISPS20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_FF_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, disp16(REGP)
   * ADDR = RPbase + zext24(disp16)
   * [ADDR] = REGP   */

  uint8_t a = ((GPR (OP[0])) & 0xff);
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_REG, OP_RP_BASE_DISP16, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_135_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, disp20(REGP)
   * ADDR = RPbase + zext24(disp20)
   * [ADDR] = REGP   */

  uint8_t a = ((GPR (OP[0])) & 0xff); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_REG, OP_RP_BASE_DISPS20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_195_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, -disp20(REGP)
   * ADDR = RPbase + zext24(-disp20)
   * [ADDR] = REGP  */

  uint8_t a = (GPR (OP[0]) & 0xff); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_REG, OP_RP_BASE_DISPE20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_136_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* storb REG, [Rindex]disp20(RPbase)
   * ADDR = RPbasex + Rindex + zext24(disp20)
   * [ADDR] = REGP   */

  uint8_t a = (GPR (OP[0])) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_REG, OP_RP_INDEX_DISPS20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* STR_IMM instructions.  */
/* storb . */
void
OP_81_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_123_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_42_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint8_t a = (OP[0]) & 0xff;
  trace_input ("storb", OP_CONSTANT4, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_218_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_RP_BASE_DISP14, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_82_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_120_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR (OP[2])) + OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_83_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_121_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}

/* storb.  */
void
OP_122_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (OP[0]) & 0xff;
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storb", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  SB (addr, a);
  trace_output_32 (sd, addr);
}
/* endif for STR_IMM.  */

/* storw . */
void
OP_C9_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint32_t addr =  OP[1];
  trace_input ("storw", OP_REG, OP_ABS20_OUTPUT, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_13F_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint32_t addr =  OP[1];
  trace_input ("storw", OP_REG, OP_ABS24_OUTPUT, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_67_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = GPR (OP[0]);
  trace_input ("storw", OP_REG, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  SW (addr, a);
  trace_output_32 (sd, addr);
}


/* storw.  */
void
OP_D_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REGP, OP_RP_BASE_DISPE4, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_DE_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_RP_INDEX_DISP0, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_31B_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_RP_INDEX_DISP14, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_19C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REGP, OP_RP_BASE_DISPE20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_13C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_R_BASE_DISPS20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_DF_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_RP_BASE_DISP16, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_13D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_RP_BASE_DISPS20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_19D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_RP_BASE_DISPE20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_13E_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0]));
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_REG, OP_RP_INDEX_DISPS20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* STORE-w IMM instruction *****/
/* storw . */
void
OP_C1_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_ABS20_OUTPUT, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_133_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_ABS24_OUTPUT, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_62_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr;
  uint16_t a = OP[0];
  trace_input ("storw", OP_CONSTANT4, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_318_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_RP_BASE_DISP14, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_C2_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_RP_INDEX_DISP0, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_130_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_R_BASE_DISPS20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_C3_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_RP_BASE_DISP16, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}


/* storw.  */
void
OP_131_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_RP_BASE_DISPS20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}

/* storw.  */
void
OP_132_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("storw", OP_CONSTANT4, OP_RP_INDEX_DISPS20, OP_VOID);
  SW (addr, a);
  trace_output_32 (sd, addr);
}


/* stord.  */
void
OP_C7_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = OP[1];
  trace_input ("stord", OP_REGP, OP_ABS20_OUTPUT, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_13B_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = OP[1];
  trace_input ("stord", OP_REGP, OP_ABS24_OUTPUT, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_66_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr, a = GPR32 (OP[0]); 
  trace_input ("stord", OP_REGP, OP_R_INDEX8_ABS20, OP_VOID);

  if (OP[1] == 0)
     addr = (GPR32 (12)) + OP[2];
  else
     addr = (GPR32 (13)) + OP[2];

  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_E_4 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_BASE_DISPE4, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_EE_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_INDEX_DISP0, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_31A_A (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_INDEX_DISP14, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_198_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_R_BASE_DISPE20, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_138_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_R_BASE_DISPS20, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_EF_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_BASE_DISP16, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_139_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_BASE_DISPS20, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_199_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_BASE_DISPE20, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* stord.  */
void
OP_13A_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]); 
  uint32_t addr = (GPR32 (OP[2])) + OP[1];
  trace_input ("stord", OP_REGP, OP_RP_INDEX_DISPS20, OP_VOID);
  SLW (addr, a);
  trace_output_32 (sd, addr);
}

/* macqu.  */
void
OP_14D_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  int16_t src1, src2;
  trace_input ("macuw", OP_REG, OP_REG, OP_REGP);
  src1 = GPR (OP[0]);
  src2 = GPR (OP[1]);
  tmp = src1 * src2;
  /*REVISIT FOR SATURATION and Q FORMAT. */
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}

/* macuw.  */
void
OP_14E_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  uint16_t src1, src2;
  trace_input ("macuw", OP_REG, OP_REG, OP_REGP);
  src1 = GPR (OP[0]);
  src2 = GPR (OP[1]);
  tmp = src1 * src2;
  /*REVISIT FOR SATURATION. */
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}

/* macsw.  */
void
OP_14F_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  int16_t src1, src2;
  trace_input ("macsw", OP_REG, OP_REG, OP_REGP);
  src1 = GPR (OP[0]);
  src2 = GPR (OP[1]);
  tmp = src1 * src2;
  /*REVISIT FOR SATURATION. */
  SET_GPR32 (OP[2], tmp);
  trace_output_32 (sd, tmp);
}


/* mulb.  */
void
OP_64_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp;
  int8_t a = (OP[0]) & 0xff;
  int8_t b = (GPR (OP[1])) & 0xff;
  trace_input ("mulb", OP_CONSTANT4_1, OP_REG, OP_VOID);
  tmp = (a * b) & 0xff;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* mulb.  */
void
OP_64B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp;
  int8_t a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("mulb", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = (a * b) & 0xff;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}


/* mulb.  */
void
OP_65_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp;
  int8_t a = (GPR (OP[0])) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("mulb", OP_REG, OP_REG, OP_VOID);
  tmp = (a * b) & 0xff;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}


/* mulw.  */
void
OP_66_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  uint16_t a = OP[0];
  int16_t b = (GPR (OP[1]));
  trace_input ("mulw", OP_CONSTANT4_1, OP_REG, OP_VOID);
  tmp = (a * b) & 0xffff;
  SET_GPR (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* mulw.  */
void
OP_66B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  int16_t a = OP[0], b = (GPR (OP[1]));
  trace_input ("mulw", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = (a * b) & 0xffff;
  SET_GPR (OP[1], tmp);
  trace_output_32 (sd, tmp);
}


/* mulw.  */
void
OP_67_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  int16_t a = (GPR (OP[0])), b = (GPR (OP[1]));
  trace_input ("mulw", OP_REG, OP_REG, OP_VOID);
  tmp = (a * b) & 0xffff;
  SET_GPR (OP[1], tmp);
  trace_output_32 (sd, tmp);
}


/* mulsb.  */
void
OP_B_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp;
  int8_t a = (GPR (OP[0])) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("mulsb", OP_REG, OP_REG, OP_VOID);
  tmp = a * b;
  SET_GPR (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* mulsw.  */
void
OP_62_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp; 
  int16_t a = (GPR (OP[0])), b = (GPR (OP[1]));
  trace_input ("mulsw", OP_REG, OP_REGP, OP_VOID);
  tmp = a * b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* muluw.  */
void
OP_63_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  uint16_t a = (GPR (OP[0])), b = (GPR (OP[1]));
  trace_input ("muluw", OP_REG, OP_REGP, OP_VOID);
  tmp = a * b;
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}


/* nop.  */
void
OP_2C00_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("nop", OP_VOID, OP_VOID, OP_VOID);

#if 0
  ins_type_counters[ (int)State.ins_type ]--;	/* don't count nops as normal instructions */
  switch (State.ins_type)
    {
    default:
      ins_type_counters[ (int)INS_UNKNOWN ]++;
      break;

    }
  EXCEPTION (SIM_SIGTRAP);
#endif
  trace_output_void (sd);
}


/* orb.  */
void
OP_24_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("orb", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR (OP[1], ((GPR (OP[1]) | tmp)));
  trace_output_16 (sd, tmp);
}

/* orb.  */
void
OP_24B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("orb", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR (OP[1], ((GPR (OP[1]) | tmp)));
  trace_output_16 (sd, tmp);
}

/* orb.  */
void
OP_25_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (GPR (OP[0])) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("orb", OP_REG, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR (OP[1], ((GPR (OP[1]) | tmp)));
  trace_output_16 (sd, tmp);
}

/* orw.  */
void
OP_26_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (OP[0]), b = (GPR (OP[1]));
  trace_input ("orw", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}


/* orw.  */
void
OP_26B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (OP[0]), b = (GPR (OP[1]));
  trace_input ("orw", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* orw.  */
void
OP_27_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (GPR (OP[0])), b = (GPR (OP[1]));
  trace_input ("orw", OP_REG, OP_REG, OP_VOID);
  tmp = a | b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}


/* lshb.  */
void
OP_13_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t tmp, b = (GPR (OP[1])) & 0xFF;
  trace_input ("lshb", OP_CONSTANT4, OP_REG, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign_flag.  */

  SET_GPR (OP[1], ((tmp & 0xFF) | ((GPR (OP[1])) & 0xFF00)));
  trace_output_16 (sd, tmp);
}

/* lshb.  */
void
OP_44_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0])) & 0xff;
  uint16_t tmp, b = (GPR (OP[1])) & 0xFF;
  trace_input ("lshb", OP_REG, OP_REG, OP_VOID);
  if (a & ((long)1 << 3))
    {
      sign_flag = 1;
      a = ~(a) + 1;
    }
  a = (unsigned int) (a & 0x7);

  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign_flag.  */
  SET_GPR (OP[1], ((tmp & 0xFF) | ((GPR (OP[1])) & 0xFF00)));
  trace_output_16 (sd, tmp);
}

/* lshw.  */
void
OP_46_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, b = GPR (OP[1]);
  int16_t a = GPR (OP[0]);
  trace_input ("lshw", OP_REG, OP_REG, OP_VOID);
  if (a & ((long)1 << 4))
    {
      sign_flag = 1;
      a = ~(a) + 1;
    }
  a = (unsigned int) (a & 0xf);

  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign_flag.  */
  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* lshw.  */
void
OP_49_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, b = GPR (OP[1]);
  uint16_t a = OP[0];
  trace_input ("lshw", OP_CONSTANT5, OP_REG, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign_flag.  */
  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* lshd.  */
void
OP_25_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, b = GPR32 (OP[1]);
  uint16_t a = OP[0];
  trace_input ("lshd", OP_CONSTANT6, OP_REGP, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */

  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* lshd.  */
void
OP_47_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp, b = GPR32 (OP[1]);
  uint16_t a = GPR (OP[0]);
  trace_input ("lshd", OP_REG, OP_REGP, OP_VOID);
  if (a & ((long)1 << 5))
    {
      sign_flag = 1;
      a = ~(a) + 1;
    }
  a = (unsigned int) (a & 0x1f);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */

  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* ashub.  */
void
OP_80_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0]; 
  int8_t tmp, b = (GPR (OP[1])) & 0xFF;
  trace_input ("ashub", OP_CONSTANT4, OP_REG, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */

  SET_GPR (OP[1], ((tmp & 0xFF) | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* ashub.  */
void
OP_81_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0]; 
  int8_t tmp, b = (GPR (OP[1])) & 0xFF;
  trace_input ("ashub", OP_CONSTANT4, OP_REG, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */

  SET_GPR (OP[1], ((tmp & 0xFF) | ((GPR (OP[1])) & 0xFF00)));
  trace_output_16 (sd, tmp);
}


/* ashub.  */
void
OP_41_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t a = (GPR (OP[0]));
  int8_t tmp, b = (GPR (OP[1])) & 0xFF;
  trace_input ("ashub", OP_REG, OP_REG, OP_VOID);

  if (a & ((long)1 << 3))
    {
      sign_flag = 1;
      a = ~(a) + 1;
    }
  a = (unsigned int) (a & 0x7);

  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */

  SET_GPR (OP[1], ((tmp & 0xFF) | ((GPR (OP[1])) & 0xFF00)));
  trace_output_16 (sd, tmp);
}


/* ashuw.  */
void
OP_42_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp, b = GPR (OP[1]);
  uint16_t a = OP[0];
  trace_input ("ashuw", OP_CONSTANT5, OP_REG, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */

  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* ashuw.  */
void
OP_43_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp, b = GPR (OP[1]);
  uint16_t a = OP[0];
  trace_input ("ashuw", OP_CONSTANT5, OP_REG, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */
  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* ashuw.  */
void
OP_45_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t tmp;
  int16_t a = GPR (OP[0]), b = GPR (OP[1]);
  trace_input ("ashuw", OP_REG, OP_REG, OP_VOID);

  if (a & ((long)1 << 4))
    {
      sign_flag = 1;
      a = ~(a) + 1;
    }
  a = (unsigned int) (a & 0xf);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */

  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */
  SET_GPR (OP[1], (tmp & 0xffff));
  trace_output_16 (sd, tmp);
}

/* ashud.  */
void
OP_26_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp,b = GPR32 (OP[1]);
  uint32_t a = OP[0];
  trace_input ("ashud", OP_CONSTANT6, OP_REGP, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* ashud.  */
void
OP_27_7 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  int32_t a = OP[0], b = GPR32 (OP[1]);
  trace_input ("ashud", OP_CONSTANT6, OP_REGP, OP_VOID);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* ashud.  */
void
OP_48_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t tmp;
  int32_t a = GPR32 (OP[0]), b = GPR32 (OP[1]);
  trace_input ("ashud", OP_REGP, OP_REGP, OP_VOID);

  if (a & ((long)1 << 5))
    {
      sign_flag = 1;
      a = ~(a) + 1;
    }
  a = (unsigned int) (a & 0x1f);
  /* A positive count specifies a shift to the left;
   * A negative count specifies a shift to the right. */
  if (sign_flag)
    tmp = b >> a;
  else
    tmp = b << a;

  sign_flag = 0; /* Reset sign flag.  */
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}


/* storm.  */
void
OP_16_D (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr = GPR (1);
  uint16_t count = OP[0], reg = 2;
  trace_input ("storm", OP_CONSTANT4, OP_VOID, OP_VOID);
  if ((addr & 1))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }

  while (count)
    {
      SW (addr, (GPR (reg)));
      addr +=2;
      --count;
      reg++;
      if (reg == 6) reg = 8;
    };

  SET_GPR (1, addr);

  trace_output_void (sd);
}


/* stormp.  */
void
OP_17_D (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t addr = GPR32 (6);
  uint16_t count = OP[0], reg = 2;
  trace_input ("stormp", OP_CONSTANT4, OP_VOID, OP_VOID);
  if ((addr & 1))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }

  while (count)
    {
      SW (addr, (GPR (reg)));
      addr +=2;
      --count;
      reg++;
      if (reg == 6) reg = 8;
    };

  SET_GPR32 (6, addr);
  trace_output_void (sd);
}

/* subb.  */
void
OP_38_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0];
  uint8_t b = (GPR (OP[1])) & 0xff;
  uint16_t tmp = (~a + 1 + b) & 0xff;
  trace_input ("subb", OP_CONSTANT4, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xff);
  SET_PSR_F (((a & 0x80) != (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* subb.  */
void
OP_38B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0] & 0xFF;
  uint8_t b = (GPR (OP[1])) & 0xFF;
  uint16_t tmp = (~a + 1 + b) & 0xFF;
  trace_input ("subb", OP_CONSTANT16, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xff);
  SET_PSR_F (((a & 0x80) != (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* subb.  */
void
OP_39_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = (GPR (OP[0])) & 0xFF;
  uint8_t b = (GPR (OP[1])) & 0xFF;
  uint16_t tmp = (~a + 1 + b) & 0xff;
  trace_input ("subb", OP_REG, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xff);
  SET_PSR_F (((a & 0x80) != (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* subw.  */
void
OP_3A_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint16_t tmp = (~a + 1 + b);
  trace_input ("subw", OP_CONSTANT4, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffff);
  SET_PSR_F (((a & 0x8000) != (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subw.  */
void
OP_3AB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint32_t tmp = (~a + 1 + b);
  trace_input ("subw", OP_CONSTANT16, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffff);
  SET_PSR_F (((a & 0x8000) != (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  SET_GPR (OP[1], tmp & 0xffff);
  trace_output_16 (sd, tmp);
}

/* subw.  */
void
OP_3B_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  uint16_t b = GPR (OP[1]);
  uint32_t tmp = (~a + 1 + b);
  trace_input ("subw", OP_REG, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffff);
  SET_PSR_F (((a & 0x8000) != (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  SET_GPR (OP[1], tmp & 0xffff);
  trace_output_16 (sd, tmp);
}

/* subcb.  */
void
OP_3C_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t a = OP[0];
  uint8_t b = (GPR (OP[1])) & 0xff;
  //uint16_t tmp1 = a + 1;
  uint16_t tmp1 = a + (PSR_C);
  uint16_t tmp = (~tmp1 + 1 + b);
  trace_input ("subcb", OP_CONSTANT4, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xff);
  SET_PSR_F (((a & 0x80) != (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subcb.  */
void
OP_3CB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0];
  uint16_t b = (GPR (OP[1])) & 0xff;
  //uint16_t tmp1 = a + 1;
  uint16_t tmp1 = a + (PSR_C);
  uint16_t tmp = (~tmp1 + 1 + b);
  trace_input ("subcb", OP_CONSTANT16, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xff);
  SET_PSR_F (((a & 0x80) != (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subcb.  */
void
OP_3D_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0])) & 0xff;
  uint16_t b = (GPR (OP[1])) & 0xff;
  uint16_t tmp1 = a + (PSR_C);
  uint16_t tmp = (~tmp1 + 1 + b);
  trace_input ("subcb", OP_REG, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xff);
  SET_PSR_F (((a & 0x80) != (b & 0x80)) && ((b & 0x80) != (tmp & 0x80)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subcw.  */
void
OP_3E_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0], b = (GPR (OP[1]));
  uint16_t tmp1 = a + (PSR_C);
  uint16_t tmp = (~tmp1 + 1  + b);
  trace_input ("subcw", OP_CONSTANT4, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffff);
  SET_PSR_F (((a & 0x8000) != (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subcw.  */
void
OP_3EB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int16_t a = OP[0];
  uint16_t b = GPR (OP[1]);
  uint16_t tmp1 = a + (PSR_C);
  uint16_t tmp = (~tmp1 + 1  + b);
  trace_input ("subcw", OP_CONSTANT16, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffff);
  SET_PSR_F (((a & 0x8000) != (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subcw.  */
void
OP_3F_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = (GPR (OP[0])), b = (GPR (OP[1]));
  uint16_t tmp1 = a + (PSR_C);
  uint16_t tmp = (~tmp1 + 1  + b);
  trace_input ("subcw", OP_REG, OP_REG, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffff);
  SET_PSR_F (((a & 0x8000) != (b & 0x8000)) && ((b & 0x8000) != (tmp & 0x8000)));
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* subd.  */
void
OP_3_C (SIM_DESC sd, SIM_CPU *cpu)
{
  int32_t a = OP[0];
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp = (~a + 1 + b);
  trace_input ("subd", OP_CONSTANT32, OP_REGP, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffffffff);
  SET_PSR_F (((a & 0x80000000) != (b & 0x80000000)) && 
	     ((b & 0x80000000) != (tmp & 0x80000000)));
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* subd.  */
void
OP_14C_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]);
  uint32_t b = GPR32 (OP[1]);
  uint32_t tmp = (~a + 1 + b);
  trace_input ("subd", OP_REGP, OP_REGP, OP_VOID);
  /* see ../common/sim-alu.h for a more extensive discussion on how to
     compute the carry/overflow bits. */
  SET_PSR_C (tmp > 0xffffffff);
  SET_PSR_F (((a & 0x80000000) != (b & 0x80000000)) && 
	     ((b & 0x80000000) != (tmp & 0x80000000)));
  SET_GPR32 (OP[1], tmp);
  trace_output_32 (sd, tmp);
}

/* excp.  */
void
OP_C_C (SIM_DESC sd, SIM_CPU *cpu)
{
  host_callback *cb = STATE_CALLBACK (sd);
  uint32_t tmp;
  uint16_t a;
  trace_input ("excp", OP_CONSTANT4, OP_VOID, OP_VOID);
  switch (OP[0])
    {
    default:
#if (DEBUG & DEBUG_TRAP) == 0
      {
#if 0
	uint16_t vec = OP[0] + TRAP_VECTOR_START;
	SET_BPC (PC + 1);
	SET_BPSR (PSR);
	SET_PSR (PSR & PSR_SM_BIT);
	JMP (vec);
	break;
#endif
      }
#else			/* if debugging use trap to print registers */
      {
	int i;
	static int first_time = 1;

	if (first_time)
	  {
	    first_time = 0;
	    sim_io_printf (sd, "Trap  #     PC ");
	    for (i = 0; i < 16; i++)
	      sim_io_printf (sd, "  %sr%d", (i > 9) ? "" : " ", i);
	    sim_io_printf (sd, "         a0         a1 f0 f1 c\n");
	  }

	sim_io_printf (sd, "Trap %2d 0x%.4x:", (int)OP[0], (int)PC);

	for (i = 0; i < 16; i++)
	  sim_io_printf (sd, " %.4x", (int) GPR (i));

	for (i = 0; i < 2; i++)
	  sim_io_printf (sd, " %.2x%.8lx",
					     ((int)(ACC (i) >> 32) & 0xff),
					     ((unsigned long) ACC (i)) & 0xffffffff);

	sim_io_printf (sd, "  %d  %d %d\n",
					   PSR_F != 0, PSR_F != 0, PSR_C != 0);
	sim_io_flush_stdout (sd);
	break;
      }
#endif
    case 8:			/* new system call trap */
      /* Trap 8 is used for simulating low-level I/O */
      {
	uint32_t result = 0;
	errno = 0;

/* Registers passed to trap 0.  */

#define FUNC   GPR (0)	/* function number.  */
#define PARM1  GPR (2)	/* optional parm 1.  */
#define PARM2  GPR (3)	/* optional parm 2.  */
#define PARM3  GPR (4)	/* optional parm 3.  */
#define PARM4  GPR (5)	/* optional parm 4.  */

/* Registers set by trap 0 */

#define RETVAL(X)   do { result = (0xffff & (X));SET_GPR (0, result);} while (0)
#define RETVAL32(X) do { result = (X); SET_GPR32 (0, result);} while (0)
#define RETERR(X) SET_GPR (4, (X))		/* return error code.  */

/* Turn a pointer in a register into a pointer into real memory. */

#define MEMPTR(x) sim_core_trans_addr (sd, cpu, read_map, x)

	switch (FUNC)
	  {
#if !defined(__GO32__) && !defined(_WIN32)
	  case TARGET_NEWLIB_CR16_SYS_fork:
	    trace_input ("<fork>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (fork ());
	    trace_output_16 (sd, result);
	    break;

#define getpid() 47
	  case TARGET_NEWLIB_CR16_SYS_getpid:
	    trace_input ("<getpid>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (getpid ());
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_kill:
	    trace_input ("<kill>", OP_REG, OP_REG, OP_VOID);
	    if (PARM1 == getpid ())
	      {
		trace_output_void (sd);
		EXCEPTION (PARM2);
	      }
	    else
	      {
		int os_sig = -1;
		switch (PARM2)
		  {
#ifdef SIGHUP
		  case 1: os_sig = SIGHUP;	break;
#endif
#ifdef SIGINT
		  case 2: os_sig = SIGINT;	break;
#endif
#ifdef SIGQUIT
		  case 3: os_sig = SIGQUIT;	break;
#endif
#ifdef SIGILL
		  case 4: os_sig = SIGILL;	break;
#endif
#ifdef SIGTRAP
		  case 5: os_sig = SIGTRAP;	break;
#endif
#ifdef SIGABRT
		  case 6: os_sig = SIGABRT;	break;
#elif defined(SIGIOT)
		  case 6: os_sig = SIGIOT;	break;
#endif
#ifdef SIGEMT
		  case 7: os_sig = SIGEMT;	break;
#endif
#ifdef SIGFPE
		  case 8: os_sig = SIGFPE;	break;
#endif
#ifdef SIGKILL
		  case 9: os_sig = SIGKILL;	break;
#endif
#ifdef SIGBUS
		  case 10: os_sig = SIGBUS;	break;
#endif
#ifdef SIGSEGV
		  case 11: os_sig = SIGSEGV;	break;
#endif
#ifdef SIGSYS
		  case 12: os_sig = SIGSYS;	break;
#endif
#ifdef SIGPIPE
		  case 13: os_sig = SIGPIPE;	break;
#endif
#ifdef SIGALRM
		  case 14: os_sig = SIGALRM;	break;
#endif
#ifdef SIGTERM
		  case 15: os_sig = SIGTERM;	break;
#endif
#ifdef SIGURG
		  case 16: os_sig = SIGURG;	break;
#endif
#ifdef SIGSTOP
		  case 17: os_sig = SIGSTOP;	break;
#endif
#ifdef SIGTSTP
		  case 18: os_sig = SIGTSTP;	break;
#endif
#ifdef SIGCONT
		  case 19: os_sig = SIGCONT;	break;
#endif
#ifdef SIGCHLD
		  case 20: os_sig = SIGCHLD;	break;
#elif defined(SIGCLD)
		  case 20: os_sig = SIGCLD;	break;
#endif
#ifdef SIGTTIN
		  case 21: os_sig = SIGTTIN;	break;
#endif
#ifdef SIGTTOU
		  case 22: os_sig = SIGTTOU;	break;
#endif
#ifdef SIGIO
		  case 23: os_sig = SIGIO;	break;
#elif defined (SIGPOLL)
		  case 23: os_sig = SIGPOLL;	break;
#endif
#ifdef SIGXCPU
		  case 24: os_sig = SIGXCPU;	break;
#endif
#ifdef SIGXFSZ
		  case 25: os_sig = SIGXFSZ;	break;
#endif
#ifdef SIGVTALRM
		  case 26: os_sig = SIGVTALRM;	break;
#endif
#ifdef SIGPROF
		  case 27: os_sig = SIGPROF;	break;
#endif
#ifdef SIGWINCH
		  case 28: os_sig = SIGWINCH;	break;
#endif
#ifdef SIGLOST
		  case 29: os_sig = SIGLOST;	break;
#endif
#ifdef SIGUSR1
		  case 30: os_sig = SIGUSR1;	break;
#endif
#ifdef SIGUSR2
		  case 31: os_sig = SIGUSR2;	break;
#endif
		  }

		if (os_sig == -1)
		  {
		    trace_output_void (sd);
		    sim_io_printf (sd, "Unknown signal %d\n", PARM2);
		    sim_io_flush_stdout (sd);
		    EXCEPTION (SIM_SIGILL);
		  }
		else
		  {
		    RETVAL (kill (PARM1, PARM2));
		    trace_output_16 (sd, result);
		  }
	      }
	    break;

	  case TARGET_NEWLIB_CR16_SYS_execve:
	    trace_input ("<execve>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (execve (MEMPTR (PARM1), (char **) MEMPTR (PARM2<<16|PARM3),
			     (char **)MEMPTR (PARM4)));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_execv:
	    trace_input ("<execv>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (execve (MEMPTR (PARM1), (char **) MEMPTR (PARM2), NULL));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_pipe:
	    {
	      reg_t buf;
	      int host_fd[2];

	      trace_input ("<pipe>", OP_VOID, OP_VOID, OP_VOID);
	      buf = PARM1;
	      RETVAL (pipe (host_fd));
	      SW (buf, host_fd[0]);
	      buf += sizeof(uint16_t);
	      SW (buf, host_fd[1]);
	      trace_output_16 (sd, result);
	    }
	  break;

	  case TARGET_NEWLIB_CR16_SYS_wait:
	    {
	      int status;
	      trace_input ("<wait>", OP_REG, OP_VOID, OP_VOID);
	      RETVAL (wait (&status));
	      if (PARM1)
		SW (PARM1, status);
	      trace_output_16 (sd, result);
	    }
	  break;
#else
	  case TARGET_NEWLIB_CR16_SYS_getpid:
	    trace_input ("<getpid>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (1);
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_kill:
	    trace_input ("<kill>", OP_REG, OP_REG, OP_VOID);
	    trace_output_void (sd);
	    EXCEPTION (PARM2);
	    break;
#endif

	  case TARGET_NEWLIB_CR16_SYS_read:
	    trace_input ("<read>", OP_REG, OP_MEMREF, OP_REG);
	    RETVAL (cb->read (cb, PARM1,
			      MEMPTR (((unsigned long)PARM3 << 16)
				      | ((unsigned long)PARM2)), PARM4));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_write:
	    trace_input ("<write>", OP_REG, OP_MEMREF, OP_REG);
	    RETVAL ((int)cb->write (cb, PARM1,
				    MEMPTR (((unsigned long)PARM3 << 16)
					    | PARM2), PARM4));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_lseek:
	    trace_input ("<lseek>", OP_REG, OP_REGP, OP_REG);
	    RETVAL32 (cb->lseek (cb, PARM1, ((((long) PARM3) << 16) | PARM2),
				 PARM4));
	    trace_output_32 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_close:
	    trace_input ("<close>", OP_REG, OP_VOID, OP_VOID);
	    RETVAL (cb->close (cb, PARM1));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_open:
	    trace_input ("<open>", OP_MEMREF, OP_REG, OP_VOID);
	    RETVAL32 (cb->open (cb, MEMPTR ((((unsigned long)PARM2) << 16)
					    | PARM1), PARM3));
	    trace_output_32 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_rename:
	    trace_input ("<rename>", OP_MEMREF, OP_MEMREF, OP_VOID);
	    RETVAL (cb->rename (cb, MEMPTR ((((unsigned long)PARM2) << 16) | PARM1),
				    MEMPTR ((((unsigned long)PARM4) << 16) | PARM3)));
	    trace_output_16 (sd, result);
	    break;

	  case 0x408: /* REVISIT: Added a dummy getenv call. */
	    trace_input ("<getenv>", OP_MEMREF, OP_MEMREF, OP_VOID);
	    RETVAL32 (0);
	    trace_output_32 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_exit:
	    trace_input ("<exit>", OP_VOID, OP_VOID, OP_VOID);
	    trace_output_void (sd);
	    sim_engine_halt (sd, cpu, NULL, PC, sim_exited, GPR (2));
	    break;

	  case TARGET_NEWLIB_CR16_SYS_unlink:
	    trace_input ("<unlink>", OP_MEMREF, OP_VOID, OP_VOID);
	    RETVAL (cb->unlink (cb, MEMPTR (((unsigned long)PARM2 << 16) | PARM1)));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_stat:
	    trace_input ("<stat>", OP_VOID, OP_VOID, OP_VOID);
	    /* stat system call.  */
	    {
	      struct stat host_stat;
	      reg_t buf;

	      RETVAL (stat (MEMPTR ((((unsigned long)PARM2) << 16)|PARM1), &host_stat));

	      buf = PARM2;

	      /* The hard-coded offsets and sizes were determined by using
	       * the CR16 compiler on a test program that used struct stat.
	       */
	      SW  (buf,    host_stat.st_dev);
	      SW  (buf+2,  host_stat.st_ino);
	      SW  (buf+4,  host_stat.st_mode);
	      SW  (buf+6,  host_stat.st_nlink);
	      SW  (buf+8,  host_stat.st_uid);
	      SW  (buf+10, host_stat.st_gid);
	      SW  (buf+12, host_stat.st_rdev);
	      SLW (buf+16, host_stat.st_size);
	      SLW (buf+20, host_stat.st_atime);
	      SLW (buf+28, host_stat.st_mtime);
	      SLW (buf+36, host_stat.st_ctime);
	    }
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_chown:
	    trace_input ("<chown>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (chown (MEMPTR (PARM1), PARM2, PARM3));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_chmod:
	    trace_input ("<chmod>", OP_VOID, OP_VOID, OP_VOID);
	    RETVAL (chmod (MEMPTR (PARM1), PARM2));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_utime:
	    trace_input ("<utime>", OP_REG, OP_REG, OP_REG);
	    /* Cast the second argument to void *, to avoid type mismatch
	       if a prototype is present.  */
	    RETVAL (utime (MEMPTR (PARM1), (void *) MEMPTR (PARM2)));
	    trace_output_16 (sd, result);
	    break;

	  case TARGET_NEWLIB_CR16_SYS_time:
	    trace_input ("<time>", OP_VOID, OP_VOID, OP_REG);
	    RETVAL32 (time (NULL));
	    trace_output_32 (sd, result);
	    break;

	  default:
	    a = OP[0];
	    switch (a)
	    {
	      case TRAP_BREAKPOINT:
		tmp = (PC);
		JMP(tmp);
		trace_output_void (sd);
		EXCEPTION (SIM_SIGTRAP);
		break;
	      case SIGTRAP:  /* supervisor call ?  */
		trace_output_void (sd);
		sim_engine_halt (sd, cpu, NULL, PC, sim_exited, GPR (2));
		break;
	      default:
		cb->error (cb, "Unknown syscall %d", FUNC);
		break;
	    }
	  }
	if ((uint16_t) result == (uint16_t) -1)
	  RETERR (cb->get_errno (cb));
	else
	  RETERR (0);
	break;
      }
    }
}


/* push.  */
void
OP_3_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0] + 1, b = OP[1], i = 0;
  uint32_t tmp, sp_addr = (GPR32 (15)) - (a * 2) - 4, is_regp = 0;
  trace_input ("push", OP_CONSTANT3, OP_REG, OP_REG);

  for (; i < a; ++i)
    {
      if ((b+i) <= 11)
        {
          SW (sp_addr, (GPR (b+i)));
          sp_addr +=2;
	}
       else
	{
	  if (is_regp == 0)
	    tmp = (GPR32 (b+i));
	  else
	    tmp = (GPR32 (b+i-1));

	  if ((a-i) > 1)
	    {
              SLW (sp_addr, tmp);
              sp_addr +=4;
	    }
	  else
	    {
              SW (sp_addr, tmp);
              sp_addr +=2;
	    }
	  ++i;
	  is_regp = 1;
	}
    }

  sp_addr +=4;

  /* Store RA address.  */
  tmp = (GPR32 (14)); 
  SLW(sp_addr,tmp);

  sp_addr = (GPR32 (15)) - (a * 2) - 4;
  SET_GPR32 (15, sp_addr);     /* Update SP address.  */

  trace_output_void (sd);
}

/* push.  */
void
OP_1_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t sp_addr, tmp, is_regp = 0;
  uint16_t a = OP[0] + 1, b = OP[1], c = OP[2], i = 0;
  trace_input ("push", OP_CONSTANT3, OP_REG, OP_VOID);

  if (c == 1)
    sp_addr = (GPR32 (15)) - (a * 2) - 4;
  else
    sp_addr = (GPR32 (15)) - (a * 2);

  for (; i < a; ++i)
    {
      if ((b+i) <= 11)
        {
          SW (sp_addr, (GPR (b+i)));
          sp_addr +=2;
	}
       else
	{
	  if (is_regp == 0)
	    tmp = (GPR32 (b+i));
	  else
	    tmp = (GPR32 (b+i-1));

	  if ((a-i) > 1)
	    {
              SLW (sp_addr, tmp);
              sp_addr +=4;
	    }
	  else
	    {
              SW (sp_addr, tmp);
              sp_addr +=2;
	    }
	  ++i;
	  is_regp = 1;
	}
    }

  if (c == 1)
   {
      /* Store RA address.  */
      tmp = (GPR32 (14)); 
      SLW(sp_addr,tmp);
      sp_addr = (GPR32 (15)) - (a * 2) - 4;
    }
  else
     sp_addr = (GPR32 (15)) - (a * 2);

  SET_GPR32 (15, sp_addr);     /* Update SP address.  */

  trace_output_void (sd);
}


/* push.   */
void
OP_11E_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t sp_addr = (GPR32 (15)), tmp;
  trace_input ("push", OP_VOID, OP_VOID, OP_VOID);
  tmp = (GPR32 (14)); 
  SLW(sp_addr-4,tmp);                /* Store RA address.   */
  SET_GPR32 (15, (sp_addr - 4));     /* Update SP address.   */
  trace_output_void (sd);
}


/* pop.   */
void
OP_5_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0] + 1, b = OP[1], i = 0;
  uint32_t tmp, sp_addr = (GPR32 (15)), is_regp = 0;;
  trace_input ("pop", OP_CONSTANT3, OP_REG, OP_REG);

  for (; i < a; ++i)
    {
      if ((b+i) <= 11)
	{
          SET_GPR ((b+i), RW(sp_addr));
          sp_addr +=2;
	}
      else
	{
	  if ((a-i) > 1)
	    {
              tmp =  RLW(sp_addr); 
              sp_addr +=4;
	    }
	  else
	    {
              tmp =  RW(sp_addr); 
              sp_addr +=2;

	      if (is_regp == 0)
		tmp = (tmp << 16) | (GPR32 (b+i));
	      else
		tmp = (tmp << 16) | (GPR32 (b+i-1));
	    }

	    if (is_regp == 0)
              SET_GPR32 ((b+i), (((tmp & 0xffff) << 16)
			         | ((tmp >> 16) & 0xffff)));
	     else
              SET_GPR32 ((b+i-1), (((tmp & 0xffff) << 16)
			           | ((tmp >> 16) & 0xffff)));

	  ++i;
	  is_regp = 1;
	}
    }

  tmp =  RLW(sp_addr);                /* store RA also.   */
  SET_GPR32 (14, (((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff)));

  SET_GPR32 (15, (sp_addr + 4));     /* Update SP address.  */

  trace_output_void (sd);
}

/* pop.  */
void
OP_2_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = OP[0] + 1, b = OP[1], c = OP[2], i = 0;
  uint32_t tmp, sp_addr = (GPR32 (15)), is_regp = 0;
  trace_input ("pop", OP_CONSTANT3, OP_REG, OP_VOID);

  for (; i < a; ++i)
    {
      if ((b+i) <= 11)
	{
          SET_GPR ((b+i), RW(sp_addr));
          sp_addr +=2;
	}
      else
	{
	  if ((a-i) > 1)
	    {
              tmp =  RLW(sp_addr); 
              sp_addr +=4;
	    }
	  else
	    {
              tmp =  RW(sp_addr); 
              sp_addr +=2;

	      if (is_regp == 0)
		tmp = ((tmp << 16) & 0xffffffff) | (GPR32 (b+i));
	      else
		tmp = ((tmp << 16) & 0xffffffff) | (GPR32 (b+i-1));
	    }

	  if (is_regp == 0)
          SET_GPR32 ((b+i), (((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff)));
	  else
          SET_GPR32 ((b+i-1), (((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff)));
	  ++i;
	  is_regp = 1;
	}
    }

  if (c == 1)
    {
      tmp =  RLW(sp_addr);    /* Store RA Reg.  */
      SET_GPR32 (14, (((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff)));
      sp_addr +=4;
    }

  SET_GPR32 (15, sp_addr); /* Update SP address.  */

  trace_output_void (sd);
}

/* pop.  */
void
OP_21E_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t sp_addr = GPR32 (15);
  uint32_t tmp;
  trace_input ("pop", OP_VOID, OP_VOID, OP_VOID);

  tmp =  RLW(sp_addr); 
  SET_GPR32 (14, (((tmp & 0xffff) << 16)| ((tmp >> 16) & 0xffff)));
  SET_GPR32 (15, (sp_addr+4));    /* Update SP address.  */

  trace_output_void (sd);
}

/* popret.  */
void
OP_7_9 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("popret", OP_CONSTANT3, OP_REG, OP_REG);
  OP_5_9 (sd, cpu);
  JMP(((GPR32(14)) << 1) & 0xffffff);

  trace_output_void (sd);
}

/* popret.  */
void
OP_3_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("popret", OP_CONSTANT3, OP_REG, OP_VOID);
  OP_2_8 (sd, cpu);
  JMP(((GPR32(14)) << 1) & 0xffffff);

  trace_output_void (sd);
}

/* popret.  */
void
OP_31E_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t tmp;
  trace_input ("popret", OP_VOID, OP_VOID, OP_VOID);
  OP_21E_10 (sd, cpu);
  tmp = (((GPR32(14)) << 1) & 0xffffff);
  /* If the resulting PC value is less than 0x00_0000 or greater 
     than 0xFF_FFFF, this instruction causes an IAD trap.*/

  if ((tmp < 0x0) || (tmp > 0xFFFFFF))
    {
      trace_output_void (sd);
      EXCEPTION (SIM_SIGBUS);
    }
  else
    JMP (tmp);

  trace_output_32 (sd, tmp);
}


/* cinv[i].  */
void
OP_A_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("cinv[i]", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* cinv[i,u].  */
void
OP_B_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("cinv[i,u]", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* cinv[d].  */
void
OP_C_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("cinv[d]", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* cinv[d,u].  */
void
OP_D_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("cinv[i,u]", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* cinv[d,i].  */
void
OP_E_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("cinv[d,i]", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* cinv[d,i,u].  */
void
OP_F_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("cinv[d,i,u]", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* retx.  */
void
OP_3_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("retx", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* di.  */
void
OP_4_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("di", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* ei.  */
void
OP_5_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("ei", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* wait.  */
void
OP_6_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("wait", OP_VOID, OP_VOID, OP_VOID);
  trace_output_void (sd);
  EXCEPTION (SIM_SIGTRAP);
}

/* ewait.  */
void
OP_7_10 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("ewait", OP_VOID, OP_VOID, OP_VOID);
  SET_PSR_I (1);
  trace_output_void (sd);
}

/* xorb. */
void
OP_28_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("xorb", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* xorb.  */
void
OP_28B_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (OP[0]) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("xorb", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* xorb.  */
void
OP_29_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint8_t tmp, a = (GPR (OP[0])) & 0xff, b = (GPR (OP[1])) & 0xff;
  trace_input ("xorb", OP_REG, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR (OP[1], (tmp | ((GPR (OP[1])) & 0xff00)));
  trace_output_16 (sd, tmp);
}

/* xorw.  */
void
OP_2A_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (OP[0]), b = (GPR (OP[1]));
  trace_input ("xorw", OP_CONSTANT4, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* xorw.  */
void
OP_2AB_C (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (OP[0]), b = (GPR (OP[1]));
  trace_input ("xorw", OP_CONSTANT16, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/* xorw.  */
void
OP_2B_8 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t tmp, a = (GPR (OP[0])), b = (GPR (OP[1]));
  trace_input ("xorw", OP_REG, OP_REG, OP_VOID);
  tmp = a ^ b;
  SET_GPR (OP[1], tmp);
  trace_output_16 (sd, tmp);
}

/*REVISIT FOR LPR/SPR . */

/* lpr.  */
void
OP_140_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = GPR (OP[0]);
  trace_input ("lpr", OP_REG, OP_REG, OP_VOID);
  SET_CREG (OP[1], a);
  trace_output_16 (sd, a);
}

/* lprd.  */
void
OP_141_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = GPR32 (OP[0]);
  trace_input ("lprd", OP_REGP, OP_REG, OP_VOID);
  SET_CREG (OP[1], a);
  trace_output_flag (sd);
}

/* spr.  */
void
OP_142_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint16_t a = CREG (OP[0]);
  trace_input ("spr", OP_REG, OP_REG, OP_VOID);
  SET_GPR (OP[1], a);
  trace_output_16 (sd, a);
}

/* sprd.  */
void
OP_143_14 (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t a = CREG (OP[0]);
  trace_input ("sprd", OP_REGP, OP_REGP, OP_VOID);
  SET_GPR32 (OP[1], a);
  trace_output_32 (sd, a);
}

/* null.  */
void
OP_0_20 (SIM_DESC sd, SIM_CPU *cpu)
{
  trace_input ("null", OP_VOID, OP_VOID, OP_VOID);
  sim_engine_halt (sd, cpu, NULL, PC, sim_exited, 0);
}
