/* Tracepoint code for remote server for GDB.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "tracepoint.h"
#include "gdbthread.h"
#include "gdbsupport/rsp-low.h"

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <inttypes.h>
#include "ax.h"
#include "tdesc.h"

#define IPA_SYM_STRUCT_NAME ipa_sym_addresses
#include "gdbsupport/agent.h"

#define DEFAULT_TRACE_BUFFER_SIZE 5242880 /* 5*1024*1024 */

/* This file is built for both GDBserver, and the in-process
   agent (IPA), a shared library that includes a tracing agent that is
   loaded by the inferior to support fast tracepoints.  Fast
   tracepoints (or more accurately, jump based tracepoints) are
   implemented by patching the tracepoint location with a jump into a
   small trampoline function whose job is to save the register state,
   call the in-process tracing agent, and then execute the original
   instruction that was under the tracepoint jump (possibly adjusted,
   if PC-relative, or some such).

   The current synchronization design is pull based.  That means,
   GDBserver does most of the work, by peeking/poking at the inferior
   agent's memory directly for downloading tracepoint and associated
   objects, and for uploading trace frames.  Whenever the IPA needs
   something from GDBserver (trace buffer is full, tracing stopped for
   some reason, etc.) the IPA calls a corresponding hook function
   where GDBserver has placed a breakpoint.

   Each of the agents has its own trace buffer.  When browsing the
   trace frames built from slow and fast tracepoints from GDB (tfind
   mode), there's no guarantee the user is seeing the trace frames in
   strict chronological creation order, although, GDBserver tries to
   keep the order relatively reasonable, by syncing the trace buffers
   at appropriate times.

*/

#ifdef IN_PROCESS_AGENT

static void trace_vdebug (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

static void
trace_vdebug (const char *fmt, ...)
{
  char buf[1024];
  va_list ap;

  va_start (ap, fmt);
  vsprintf (buf, fmt, ap);
  fprintf (stderr, PROG "/tracepoint: %s\n", buf);
  va_end (ap);
}

#define trace_debug(fmt, args...)	\
  do {						\
    if (debug_threads)				\
      trace_vdebug ((fmt), ##args);		\
  } while (0)

#else

#define trace_debug(fmt, args...)	\
  do {						\
      threads_debug_printf ((fmt), ##args);	\
  } while (0)

#endif

/* Prefix exported symbols, for good citizenship.  All the symbols
   that need exporting are defined in this module.  Note that all
   these symbols must be tagged with IP_AGENT_EXPORT_*.  */
#ifdef IN_PROCESS_AGENT
# define gdb_tp_heap_buffer IPA_SYM_EXPORTED_NAME (gdb_tp_heap_buffer)
# define gdb_jump_pad_buffer IPA_SYM_EXPORTED_NAME (gdb_jump_pad_buffer)
# define gdb_jump_pad_buffer_end IPA_SYM_EXPORTED_NAME (gdb_jump_pad_buffer_end)
# define gdb_trampoline_buffer IPA_SYM_EXPORTED_NAME (gdb_trampoline_buffer)
# define gdb_trampoline_buffer_end IPA_SYM_EXPORTED_NAME (gdb_trampoline_buffer_end)
# define gdb_trampoline_buffer_error IPA_SYM_EXPORTED_NAME (gdb_trampoline_buffer_error)
# define collecting IPA_SYM_EXPORTED_NAME (collecting)
# define gdb_collect_ptr IPA_SYM_EXPORTED_NAME (gdb_collect_ptr)
# define stop_tracing IPA_SYM_EXPORTED_NAME (stop_tracing)
# define flush_trace_buffer IPA_SYM_EXPORTED_NAME (flush_trace_buffer)
# define about_to_request_buffer_space IPA_SYM_EXPORTED_NAME (about_to_request_buffer_space)
# define trace_buffer_is_full IPA_SYM_EXPORTED_NAME (trace_buffer_is_full)
# define stopping_tracepoint IPA_SYM_EXPORTED_NAME (stopping_tracepoint)
# define expr_eval_result IPA_SYM_EXPORTED_NAME (expr_eval_result)
# define error_tracepoint IPA_SYM_EXPORTED_NAME (error_tracepoint)
# define tracepoints IPA_SYM_EXPORTED_NAME (tracepoints)
# define tracing IPA_SYM_EXPORTED_NAME (tracing)
# define trace_buffer_ctrl IPA_SYM_EXPORTED_NAME (trace_buffer_ctrl)
# define trace_buffer_ctrl_curr IPA_SYM_EXPORTED_NAME (trace_buffer_ctrl_curr)
# define trace_buffer_lo IPA_SYM_EXPORTED_NAME (trace_buffer_lo)
# define trace_buffer_hi IPA_SYM_EXPORTED_NAME (trace_buffer_hi)
# define traceframe_read_count IPA_SYM_EXPORTED_NAME (traceframe_read_count)
# define traceframe_write_count IPA_SYM_EXPORTED_NAME (traceframe_write_count)
# define traceframes_created IPA_SYM_EXPORTED_NAME (traceframes_created)
# define trace_state_variables IPA_SYM_EXPORTED_NAME (trace_state_variables)
# define get_raw_reg_ptr IPA_SYM_EXPORTED_NAME (get_raw_reg_ptr)
# define get_trace_state_variable_value_ptr \
  IPA_SYM_EXPORTED_NAME (get_trace_state_variable_value_ptr)
# define set_trace_state_variable_value_ptr \
  IPA_SYM_EXPORTED_NAME (set_trace_state_variable_value_ptr)
# define ust_loaded IPA_SYM_EXPORTED_NAME (ust_loaded)
# define helper_thread_id IPA_SYM_EXPORTED_NAME (helper_thread_id)
# define cmd_buf IPA_SYM_EXPORTED_NAME (cmd_buf)
# define ipa_tdesc_idx IPA_SYM_EXPORTED_NAME (ipa_tdesc_idx)
#endif

#ifndef IN_PROCESS_AGENT

/* Addresses of in-process agent's symbols GDBserver cares about.  */

struct ipa_sym_addresses
{
  CORE_ADDR addr_gdb_tp_heap_buffer;
  CORE_ADDR addr_gdb_jump_pad_buffer;
  CORE_ADDR addr_gdb_jump_pad_buffer_end;
  CORE_ADDR addr_gdb_trampoline_buffer;
  CORE_ADDR addr_gdb_trampoline_buffer_end;
  CORE_ADDR addr_gdb_trampoline_buffer_error;
  CORE_ADDR addr_collecting;
  CORE_ADDR addr_gdb_collect_ptr;
  CORE_ADDR addr_stop_tracing;
  CORE_ADDR addr_flush_trace_buffer;
  CORE_ADDR addr_about_to_request_buffer_space;
  CORE_ADDR addr_trace_buffer_is_full;
  CORE_ADDR addr_stopping_tracepoint;
  CORE_ADDR addr_expr_eval_result;
  CORE_ADDR addr_error_tracepoint;
  CORE_ADDR addr_tracepoints;
  CORE_ADDR addr_tracing;
  CORE_ADDR addr_trace_buffer_ctrl;
  CORE_ADDR addr_trace_buffer_ctrl_curr;
  CORE_ADDR addr_trace_buffer_lo;
  CORE_ADDR addr_trace_buffer_hi;
  CORE_ADDR addr_traceframe_read_count;
  CORE_ADDR addr_traceframe_write_count;
  CORE_ADDR addr_traceframes_created;
  CORE_ADDR addr_trace_state_variables;
  CORE_ADDR addr_get_raw_reg_ptr;
  CORE_ADDR addr_get_trace_state_variable_value_ptr;
  CORE_ADDR addr_set_trace_state_variable_value_ptr;
  CORE_ADDR addr_ust_loaded;
  CORE_ADDR addr_ipa_tdesc_idx;
};

static struct
{
  const char *name;
  int offset;
} symbol_list[] = {
  IPA_SYM(gdb_tp_heap_buffer),
  IPA_SYM(gdb_jump_pad_buffer),
  IPA_SYM(gdb_jump_pad_buffer_end),
  IPA_SYM(gdb_trampoline_buffer),
  IPA_SYM(gdb_trampoline_buffer_end),
  IPA_SYM(gdb_trampoline_buffer_error),
  IPA_SYM(collecting),
  IPA_SYM(gdb_collect_ptr),
  IPA_SYM(stop_tracing),
  IPA_SYM(flush_trace_buffer),
  IPA_SYM(about_to_request_buffer_space),
  IPA_SYM(trace_buffer_is_full),
  IPA_SYM(stopping_tracepoint),
  IPA_SYM(expr_eval_result),
  IPA_SYM(error_tracepoint),
  IPA_SYM(tracepoints),
  IPA_SYM(tracing),
  IPA_SYM(trace_buffer_ctrl),
  IPA_SYM(trace_buffer_ctrl_curr),
  IPA_SYM(trace_buffer_lo),
  IPA_SYM(trace_buffer_hi),
  IPA_SYM(traceframe_read_count),
  IPA_SYM(traceframe_write_count),
  IPA_SYM(traceframes_created),
  IPA_SYM(trace_state_variables),
  IPA_SYM(get_raw_reg_ptr),
  IPA_SYM(get_trace_state_variable_value_ptr),
  IPA_SYM(set_trace_state_variable_value_ptr),
  IPA_SYM(ust_loaded),
  IPA_SYM(ipa_tdesc_idx),
};

static struct ipa_sym_addresses ipa_sym_addrs;

static int read_inferior_integer (CORE_ADDR symaddr, int *val);

/* Returns true if both the in-process agent library and the static
   tracepoints libraries are loaded in the inferior, and agent has
   capability on static tracepoints.  */

static int
in_process_agent_supports_ust (void)
{
  int loaded = 0;

  if (!agent_loaded_p ())
    {
      warning ("In-process agent not loaded");
      return 0;
    }

  if (agent_capability_check (AGENT_CAPA_STATIC_TRACE))
    {
      /* Agent understands static tracepoint, then check whether UST is in
	 fact loaded in the inferior.  */
      if (read_inferior_integer (ipa_sym_addrs.addr_ust_loaded, &loaded))
	{
	  warning ("Error reading ust_loaded in lib");
	  return 0;
	}

      return loaded;
    }
  else
    return 0;
}

static void
write_e_ipa_not_loaded (char *buffer)
{
  sprintf (buffer,
	   "E.In-process agent library not loaded in process.  "
	   "Fast and static tracepoints unavailable.");
}

/* Write an error to BUFFER indicating that UST isn't loaded in the
   inferior.  */

static void
write_e_ust_not_loaded (char *buffer)
{
#ifdef HAVE_UST
  sprintf (buffer,
	   "E.UST library not loaded in process.  "
	   "Static tracepoints unavailable.");
#else
  sprintf (buffer, "E.GDBserver was built without static tracepoints support");
#endif
}

/* If the in-process agent library isn't loaded in the inferior, write
   an error to BUFFER, and return 1.  Otherwise, return 0.  */

static int
maybe_write_ipa_not_loaded (char *buffer)
{
  if (!agent_loaded_p ())
    {
      write_e_ipa_not_loaded (buffer);
      return 1;
    }
  return 0;
}

/* If the in-process agent library and the ust (static tracepoints)
   library aren't loaded in the inferior, write an error to BUFFER,
   and return 1.  Otherwise, return 0.  */

static int
maybe_write_ipa_ust_not_loaded (char *buffer)
{
  if (!agent_loaded_p ())
    {
      write_e_ipa_not_loaded (buffer);
      return 1;
    }
  else if (!in_process_agent_supports_ust ())
    {
      write_e_ust_not_loaded (buffer);
      return 1;
    }
  return 0;
}

/* Cache all future symbols that the tracepoints module might request.
   We can not request symbols at arbitrary states in the remote
   protocol, only when the client tells us that new symbols are
   available.  So when we load the in-process library, make sure to
   check the entire list.  */

void
tracepoint_look_up_symbols (void)
{
  int i;

  if (agent_loaded_p ())
    return;

  for (i = 0; i < sizeof (symbol_list) / sizeof (symbol_list[0]); i++)
    {
      CORE_ADDR *addrp =
	(CORE_ADDR *) ((char *) &ipa_sym_addrs + symbol_list[i].offset);

      if (look_up_one_symbol (symbol_list[i].name, addrp, 1) == 0)
	{
	  threads_debug_printf ("symbol `%s' not found", symbol_list[i].name);
	  return;
	}
    }

  agent_look_up_symbols (NULL);
}

#endif

/* GDBserver places a breakpoint on the IPA's version (which is a nop)
   of the "stop_tracing" function.  When this breakpoint is hit,
   tracing stopped in the IPA for some reason.  E.g., due to
   tracepoint reaching the pass count, hitting conditional expression
   evaluation error, etc.

   The IPA's trace buffer is never in circular tracing mode: instead,
   GDBserver's is, and whenever the in-process buffer fills, it calls
   "flush_trace_buffer", which triggers an internal breakpoint.
   GDBserver reacts to this breakpoint by pulling the meanwhile
   collected data.  Old frames discarding is always handled on the
   GDBserver side.  */

#ifdef IN_PROCESS_AGENT
/* See target.h.  */

int
read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  memcpy (myaddr, (void *) (uintptr_t) memaddr, len);
  return 0;
}

/* Call this in the functions where GDBserver places a breakpoint, so
   that the compiler doesn't try to be clever and skip calling the
   function at all.  This is necessary, even if we tell the compiler
   to not inline said functions.  */

#if defined(__GNUC__)
#  define UNKNOWN_SIDE_EFFECTS() asm ("")
#else
#  define UNKNOWN_SIDE_EFFECTS() do {} while (0)
#endif

/* This is needed for -Wmissing-declarations.  */
IP_AGENT_EXPORT_FUNC void stop_tracing (void);

IP_AGENT_EXPORT_FUNC void
stop_tracing (void)
{
  /* GDBserver places breakpoint here.  */
  UNKNOWN_SIDE_EFFECTS();
}

/* This is needed for -Wmissing-declarations.  */
IP_AGENT_EXPORT_FUNC void flush_trace_buffer (void);

IP_AGENT_EXPORT_FUNC void
flush_trace_buffer (void)
{
  /* GDBserver places breakpoint here.  */
  UNKNOWN_SIDE_EFFECTS();
}

#endif

#ifndef IN_PROCESS_AGENT
static int
tracepoint_handler (CORE_ADDR address)
{
  trace_debug ("tracepoint_handler: tracepoint at 0x%s hit",
	       paddress (address));
  return 0;
}

/* Breakpoint at "stop_tracing" in the inferior lib.  */
static struct breakpoint *stop_tracing_bkpt;
static int stop_tracing_handler (CORE_ADDR);

/* Breakpoint at "flush_trace_buffer" in the inferior lib.  */
static struct breakpoint *flush_trace_buffer_bkpt;
static int flush_trace_buffer_handler (CORE_ADDR);

static void download_trace_state_variables (void);
static void upload_fast_traceframes (void);

static int run_inferior_command (char *cmd, int len);

static int
read_inferior_integer (CORE_ADDR symaddr, int *val)
{
  return read_inferior_memory (symaddr, (unsigned char *) val,
			       sizeof (*val));
}

struct tracepoint;
static int tracepoint_send_agent (struct tracepoint *tpoint);

static int
read_inferior_uinteger (CORE_ADDR symaddr, unsigned int *val)
{
  return read_inferior_memory (symaddr, (unsigned char *) val,
			       sizeof (*val));
}

static int
read_inferior_data_pointer (CORE_ADDR symaddr, CORE_ADDR *val)
{
  void *pval = (void *) (uintptr_t) val;
  int ret;

  ret = read_inferior_memory (symaddr, (unsigned char *) &pval, sizeof (pval));
  *val = (uintptr_t) pval;
  return ret;
}

static int
write_inferior_data_pointer (CORE_ADDR symaddr, CORE_ADDR val)
{
  void *pval = (void *) (uintptr_t) val;
  return target_write_memory (symaddr,
				(unsigned char *) &pval, sizeof (pval));
}

static int
write_inferior_integer (CORE_ADDR symaddr, int val)
{
  return target_write_memory (symaddr, (unsigned char *) &val, sizeof (val));
}

static int
write_inferior_int8 (CORE_ADDR symaddr, int8_t val)
{
  return target_write_memory (symaddr, (unsigned char *) &val, sizeof (val));
}

static int
write_inferior_uinteger (CORE_ADDR symaddr, unsigned int val)
{
  return target_write_memory (symaddr, (unsigned char *) &val, sizeof (val));
}

static CORE_ADDR target_malloc (ULONGEST size);

#define COPY_FIELD_TO_BUF(BUF, OBJ, FIELD)	\
  do {							\
    memcpy (BUF, &(OBJ)->FIELD, sizeof ((OBJ)->FIELD)); \
    BUF += sizeof ((OBJ)->FIELD);			\
  } while (0)

#endif

/* Base action.  Concrete actions inherit this.  */

struct tracepoint_action
{
  char type;
};

/* An 'M' (collect memory) action.  */
struct collect_memory_action
{
  struct tracepoint_action base;

  ULONGEST addr;
  ULONGEST len;
  int32_t basereg;
};

/* An 'R' (collect registers) action.  */

struct collect_registers_action
{
  struct tracepoint_action base;
};

/* An 'X' (evaluate expression) action.  */

struct eval_expr_action
{
  struct tracepoint_action base;

  struct agent_expr *expr;
};

/* An 'L' (collect static trace data) action.  */
struct collect_static_trace_data_action
{
  struct tracepoint_action base;
};

#ifndef IN_PROCESS_AGENT
static CORE_ADDR
m_tracepoint_action_download (const struct tracepoint_action *action)
{
  CORE_ADDR ipa_action = target_malloc (sizeof (struct collect_memory_action));

  target_write_memory (ipa_action, (unsigned char *) action,
			 sizeof (struct collect_memory_action));

  return ipa_action;
}
static char *
m_tracepoint_action_send (char *buffer, const struct tracepoint_action *action)
{
  struct collect_memory_action *maction
    = (struct collect_memory_action *) action;

  COPY_FIELD_TO_BUF (buffer, maction, addr);
  COPY_FIELD_TO_BUF (buffer, maction, len);
  COPY_FIELD_TO_BUF (buffer, maction, basereg);

  return buffer;
}

static CORE_ADDR
r_tracepoint_action_download (const struct tracepoint_action *action)
{
  CORE_ADDR ipa_action = target_malloc (sizeof (struct collect_registers_action));

  target_write_memory (ipa_action, (unsigned char *) action,
			 sizeof (struct collect_registers_action));

  return ipa_action;
}

static char *
r_tracepoint_action_send (char *buffer, const struct tracepoint_action *action)
{
  return buffer;
}

static CORE_ADDR download_agent_expr (struct agent_expr *expr);

static CORE_ADDR
x_tracepoint_action_download (const struct tracepoint_action *action)
{
  CORE_ADDR ipa_action = target_malloc (sizeof (struct eval_expr_action));
  CORE_ADDR expr;

  target_write_memory (ipa_action, (unsigned char *) action,
			 sizeof (struct eval_expr_action));
  expr = download_agent_expr (((struct eval_expr_action *) action)->expr);
  write_inferior_data_pointer (ipa_action
			       + offsetof (struct eval_expr_action, expr),
			       expr);

  return ipa_action;
}

/* Copy agent expression AEXPR to buffer pointed by P.  If AEXPR is NULL,
   copy 0 to P.  Return updated header of buffer.  */

static char *
agent_expr_send (char *p, const struct agent_expr *aexpr)
{
  /* Copy the length of condition first, and then copy its
     content.  */
  if (aexpr == NULL)
    {
      memset (p, 0, 4);
      p += 4;
    }
  else
    {
      memcpy (p, &aexpr->length, 4);
      p +=4;

      memcpy (p, aexpr->bytes, aexpr->length);
      p += aexpr->length;
    }
  return p;
}

static char *
x_tracepoint_action_send ( char *buffer, const struct tracepoint_action *action)
{
  struct eval_expr_action *eaction = (struct eval_expr_action *) action;

  return agent_expr_send (buffer, eaction->expr);
}

static CORE_ADDR
l_tracepoint_action_download (const struct tracepoint_action *action)
{
  CORE_ADDR ipa_action
    = target_malloc (sizeof (struct collect_static_trace_data_action));

  target_write_memory (ipa_action, (unsigned char *) action,
			 sizeof (struct collect_static_trace_data_action));

  return ipa_action;
}

static char *
l_tracepoint_action_send (char *buffer, const struct tracepoint_action *action)
{
  return buffer;
}

static char *
tracepoint_action_send (char *buffer, const struct tracepoint_action *action)
{
  switch (action->type)
    {
    case 'M':
      return m_tracepoint_action_send (buffer, action);
    case 'R':
      return r_tracepoint_action_send (buffer, action);
    case 'X':
      return x_tracepoint_action_send (buffer, action);
    case 'L':
      return l_tracepoint_action_send (buffer, action);
    }
  error ("Unknown trace action '%c'.", action->type);
}

static CORE_ADDR
tracepoint_action_download (const struct tracepoint_action *action)
{
  switch (action->type)
    {
    case 'M':
      return m_tracepoint_action_download (action);
    case 'R':
      return r_tracepoint_action_download (action);
    case 'X':
      return x_tracepoint_action_download (action);
    case 'L':
      return l_tracepoint_action_download (action);
    }
  error ("Unknown trace action '%c'.", action->type);
}
#endif

/* This structure describes a piece of the source-level definition of
   the tracepoint.  The contents are not interpreted by the target,
   but preserved verbatim for uploading upon reconnection.  */

struct source_string
{
  /* The type of string, such as "cond" for a conditional.  */
  char *type;

  /* The source-level string itself.  For the sake of target
     debugging, we store it in plaintext, even though it is always
     transmitted in hex.  */
  char *str;

  /* Link to the next one in the list.  We link them in the order
     received, in case some make up an ordered list of commands or
     some such.  */
  struct source_string *next;
};

enum tracepoint_type
{
  /* Trap based tracepoint.  */
  trap_tracepoint,

  /* A fast tracepoint implemented with a jump instead of a trap.  */
  fast_tracepoint,

  /* A static tracepoint, implemented by a program call into a tracing
     library.  */
  static_tracepoint
};

struct tracepoint_hit_ctx;

typedef enum eval_result_type (*condfn) (unsigned char *,
					 ULONGEST *);

/* The definition of a tracepoint.  */

/* Tracepoints may have multiple locations, each at a different
   address.  This can occur with optimizations, template
   instantiation, etc.  Since the locations may be in different
   scopes, the conditions and actions may be different for each
   location.  Our target version of tracepoints is more like GDB's
   notion of "breakpoint locations", but we have almost nothing that
   is not per-location, so we bother having two kinds of objects.  The
   key consequence is that numbers are not unique, and that it takes
   both number and address to identify a tracepoint uniquely.  */

struct tracepoint
{
  /* The number of the tracepoint, as specified by GDB.  Several
     tracepoint objects here may share a number.  */
  uint32_t number;

  /* Address at which the tracepoint is supposed to trigger.  Several
     tracepoints may share an address.  */
  CORE_ADDR address;

  /* Tracepoint type.  */
  enum tracepoint_type type;

  /* True if the tracepoint is currently enabled.  */
  int8_t enabled;

  /* The number of single steps that will be performed after each
     tracepoint hit.  */
  uint64_t step_count;

  /* The number of times the tracepoint may be hit before it will
     terminate the entire tracing run.  */
  uint64_t pass_count;

  /* Pointer to the agent expression that is the tracepoint's
     conditional, or NULL if the tracepoint is unconditional.  */
  struct agent_expr *cond;

  /* The list of actions to take when the tracepoint triggers.  */
  uint32_t numactions;
  struct tracepoint_action **actions;

  /* Count of the times we've hit this tracepoint during the run.
     Note that while-stepping steps are not counted as "hits".  */
  uint64_t hit_count;

  /* Cached sum of the sizes of traceframes created by this point.  */
  uint64_t traceframe_usage;

  CORE_ADDR compiled_cond;

  /* Link to the next tracepoint in the list.  */
  struct tracepoint *next;

#ifndef IN_PROCESS_AGENT
  /* The list of actions to take when the tracepoint triggers, in
     string/packet form.  */
  char **actions_str;

  /* The collection of strings that describe the tracepoint as it was
     entered into GDB.  These are not used by the target, but are
     reported back to GDB upon reconnection.  */
  struct source_string *source_strings;

  /* The number of bytes displaced by fast tracepoints. It may subsume
     multiple instructions, for multi-byte fast tracepoints.  This
     field is only valid for fast tracepoints.  */
  uint32_t orig_size;

  /* Only for fast tracepoints.  */
  CORE_ADDR obj_addr_on_target;

  /* Address range where the original instruction under a fast
     tracepoint was relocated to.  (_end is actually one byte past
     the end).  */
  CORE_ADDR adjusted_insn_addr;
  CORE_ADDR adjusted_insn_addr_end;

  /* The address range of the piece of the jump pad buffer that was
     assigned to this fast tracepoint.  (_end is actually one byte
     past the end).*/
  CORE_ADDR jump_pad;
  CORE_ADDR jump_pad_end;

  /* The address range of the piece of the trampoline buffer that was
     assigned to this fast tracepoint.  (_end is actually one byte
     past the end).  */
  CORE_ADDR trampoline;
  CORE_ADDR trampoline_end;

  /* The list of actions to take while in a stepping loop.  These
     fields are only valid for patch-based tracepoints.  */
  int num_step_actions;
  struct tracepoint_action **step_actions;
  /* Same, but in string/packet form.  */
  char **step_actions_str;

  /* Handle returned by the breakpoint or tracepoint module when we
     inserted the trap or jump, or hooked into a static tracepoint.
     NULL if we haven't inserted it yet.  */
  void *handle;
#endif

};

#ifndef IN_PROCESS_AGENT

/* Given `while-stepping', a thread may be collecting data for more
   than one tracepoint simultaneously.  On the other hand, the same
   tracepoint with a while-stepping action may be hit by more than one
   thread simultaneously (but not quite, each thread could be handling
   a different step).  Each thread holds a list of these objects,
   representing the current step of each while-stepping action being
   collected.  */

struct wstep_state
{
  struct wstep_state *next;

  /* The tracepoint number.  */
  int tp_number;
  /* The tracepoint's address.  */
  CORE_ADDR tp_address;

  /* The number of the current step in this 'while-stepping'
     action.  */
  long current_step;
};

#endif

extern "C" {

/* The linked list of all tracepoints.  Marked explicitly as used as
   the in-process library doesn't use it for the fast tracepoints
   support.  */
IP_AGENT_EXPORT_VAR struct tracepoint *tracepoints;

/* The first tracepoint to exceed its pass count.  */

IP_AGENT_EXPORT_VAR struct tracepoint *stopping_tracepoint;

/* True if the trace buffer is full or otherwise no longer usable.  */

IP_AGENT_EXPORT_VAR int trace_buffer_is_full;

/* The first error that occurred during expression evaluation.  */

/* Stored as an int to avoid the IPA ABI being dependent on whatever
   the compiler decides to use for the enum's underlying type.  Holds
   enum eval_result_type values.  */
IP_AGENT_EXPORT_VAR int expr_eval_result = expr_eval_no_error;

}

#ifndef IN_PROCESS_AGENT

/* Pointer to the last tracepoint in the list, new tracepoints are
   linked in at the end.  */

static struct tracepoint *last_tracepoint;

static const char * const eval_result_names[] =
  {
#define AX_RESULT_TYPE(ENUM,STR) STR,
#include "ax-result-types.def"
#undef AX_RESULT_TYPE
  };

#endif

/* The tracepoint in which the error occurred.  */

extern "C" {
IP_AGENT_EXPORT_VAR struct tracepoint *error_tracepoint;
}

struct trace_state_variable
{
  /* This is the name of the variable as used in GDB.  The target
     doesn't use the name, but needs to have it for saving and
     reconnection purposes.  */
  char *name;

  /* This number identifies the variable uniquely.  Numbers may be
     assigned either by the target (in the case of builtin variables),
     or by GDB, and are presumed unique during the course of a trace
     experiment.  */
  int number;

  /* The variable's initial value, a 64-bit signed integer always.  */
  LONGEST initial_value;

  /* The variable's value, a 64-bit signed integer always.  */
  LONGEST value;

  /* Pointer to a getter function, used to supply computed values.  */
  LONGEST (*getter) (void);

  /* Link to the next variable.  */
  struct trace_state_variable *next;
};

/* Linked list of all trace state variables.  */

#ifdef IN_PROCESS_AGENT
static struct trace_state_variable *alloced_trace_state_variables;
#endif

IP_AGENT_EXPORT_VAR struct trace_state_variable *trace_state_variables;

/* The results of tracing go into a fixed-size space known as the
   "trace buffer".  Because usage follows a limited number of
   patterns, we manage it ourselves rather than with malloc.  Basic
   rules are that we create only one trace frame at a time, each is
   variable in size, they are never moved once created, and we only
   discard if we are doing a circular buffer, and then only the oldest
   ones.  Each trace frame includes its own size, so we don't need to
   link them together, and the trace frame number is relative to the
   first one, so we don't need to record numbers.  A trace frame also
   records the number of the tracepoint that created it.  The data
   itself is a series of blocks, each introduced by a single character
   and with a defined format.  Each type of block has enough
   type/length info to allow scanners to jump quickly from one block
   to the next without reading each byte in the block.  */

/* Trace buffer management would be simple - advance a free pointer
   from beginning to end, then stop - were it not for the circular
   buffer option, which is a useful way to prevent a trace run from
   stopping prematurely because the buffer filled up.  In the circular
   case, the location of the first trace frame (trace_buffer_start)
   moves as old trace frames are discarded.  Also, since we grow trace
   frames incrementally as actions are performed, we wrap around to
   the beginning of the trace buffer.  This is per-block, so each
   block within a trace frame remains contiguous.  Things get messy
   when the wrapped-around trace frame is the one being discarded; the
   free space ends up in two parts at opposite ends of the buffer.  */

#ifndef ATTR_PACKED
#  if defined(__GNUC__)
#    define ATTR_PACKED __attribute__ ((packed))
#  else
#    define ATTR_PACKED /* nothing */
#  endif
#endif

/* The data collected at a tracepoint hit.  This object should be as
   small as possible, since there may be a great many of them.  We do
   not need to keep a frame number, because they are all sequential
   and there are no deletions; so the Nth frame in the buffer is
   always frame number N.  */

struct traceframe
{
  /* Number of the tracepoint that collected this traceframe.  A value
     of 0 indicates the current end of the trace buffer.  We make this
     a 16-bit field because it's never going to happen that GDB's
     numbering of tracepoints reaches 32,000.  */
  int tpnum : 16;

  /* The size of the data in this trace frame.  We limit this to 32
     bits, even on a 64-bit target, because it's just implausible that
     one is validly going to collect 4 gigabytes of data at a single
     tracepoint hit.  */
  unsigned int data_size : 32;

  /* The base of the trace data, which is contiguous from this point.  */
  unsigned char data[0];

} ATTR_PACKED;

/* The size of the EOB marker, in bytes.  A traceframe with zeroed
   fields (and no data) marks the end of trace data.  */
#define TRACEFRAME_EOB_MARKER_SIZE offsetof (struct traceframe, data)

/* This flag is true if the trace buffer is circular, meaning that
   when it fills, the oldest trace frames are discarded in order to
   make room.  */

#ifndef IN_PROCESS_AGENT
static int circular_trace_buffer;
#endif

/* Size of the trace buffer.  */

static LONGEST trace_buffer_size;

extern "C" {

/* Pointer to the block of memory that traceframes all go into.  */

IP_AGENT_EXPORT_VAR unsigned char *trace_buffer_lo;

/* Pointer to the end of the trace buffer, more precisely to the byte
   after the end of the buffer.  */

IP_AGENT_EXPORT_VAR unsigned char *trace_buffer_hi;

}

/* Control structure holding the read/write/etc. pointers into the
   trace buffer.  We need more than one of these to implement a
   transaction-like mechanism to guarantees that both GDBserver and the
   in-process agent can try to change the trace buffer
   simultaneously.  */

struct trace_buffer_control
{
  /* Pointer to the first trace frame in the buffer.  In the
     non-circular case, this is equal to trace_buffer_lo, otherwise it
     moves around in the buffer.  */
  unsigned char *start;

  /* Pointer to the free part of the trace buffer.  Note that we clear
     several bytes at and after this pointer, so that traceframe
     scans/searches terminate properly.  */
  unsigned char *free;

  /* Pointer to the byte after the end of the free part.  Note that
     this may be smaller than trace_buffer_free in the circular case,
     and means that the free part is in two pieces.  Initially it is
     equal to trace_buffer_hi, then is generally equivalent to
     trace_buffer_start.  */
  unsigned char *end_free;

  /* Pointer to the wraparound.  If not equal to trace_buffer_hi, then
     this is the point at which the trace data breaks, and resumes at
     trace_buffer_lo.  */
  unsigned char *wrap;
};

/* Same as above, to be used by GDBserver when updating the in-process
   agent.  */
struct ipa_trace_buffer_control
{
  uintptr_t start;
  uintptr_t free;
  uintptr_t end_free;
  uintptr_t wrap;
};


/* We have possibly both GDBserver and an inferior thread accessing
   the same IPA trace buffer memory.  The IPA is the producer (tries
   to put new frames in the buffer), while GDBserver occasionally
   consumes them, that is, flushes the IPA's buffer into its own
   buffer.  Both sides need to update the trace buffer control
   pointers (current head, tail, etc.).  We can't use a global lock to
   synchronize the accesses, as otherwise we could deadlock GDBserver
   (if the thread holding the lock stops for a signal, say).  So
   instead of that, we use a transaction scheme where GDBserver writes
   always prevail over the IPAs writes, and, we have the IPA detect
   the commit failure/overwrite, and retry the whole attempt.  This is
   mainly implemented by having a global token object that represents
   who wrote last to the buffer control structure.  We need to freeze
   any inferior writing to the buffer while GDBserver touches memory,
   so that the inferior can correctly detect that GDBserver had been
   there, otherwise, it could mistakingly think its commit was
   successful; that's implemented by simply having GDBserver set a
   breakpoint the inferior hits if it is the critical region.

   There are three cycling trace buffer control structure copies
   (buffer head, tail, etc.), with the token object including an index
   indicating which is current live copy.  The IPA tentatively builds
   an updated copy in a non-current control structure, while GDBserver
   always clobbers the current version directly.  The IPA then tries
   to atomically "commit" its version; if GDBserver clobbered the
   structure meanwhile, that will fail, and the IPA restarts the
   allocation process.

   Listing the step in further detail, we have:

  In-process agent (producer):

  - passes by `about_to_request_buffer_space' breakpoint/lock

  - reads current token, extracts current trace buffer control index,
    and starts tentatively updating the rightmost one (0->1, 1->2,
    2->0).  Note that only one inferior thread is executing this code
    at any given time, due to an outer lock in the jump pads.

  - updates counters, and tries to commit the token.

  - passes by second `about_to_request_buffer_space' breakpoint/lock,
    leaving the sync region.

  - checks if the update was effective.

  - if trace buffer was found full, hits flush_trace_buffer
    breakpoint, and restarts later afterwards.

  GDBserver (consumer):

  - sets `about_to_request_buffer_space' breakpoint/lock.

  - updates the token unconditionally, using the current buffer
    control index, since it knows that the IP agent always writes to
    the rightmost, and due to the breakpoint, at most one IP thread
    can try to update the trace buffer concurrently to GDBserver, so
    there will be no danger of trace buffer control index wrap making
    the IPA write to the same index as GDBserver.

  - flushes the IP agent's trace buffer completely, and updates the
    current trace buffer control structure.  GDBserver *always* wins.

  - removes the `about_to_request_buffer_space' breakpoint.

The token is stored in the `trace_buffer_ctrl_curr' variable.
Internally, it's bits are defined as:

 |-------------+-----+-------------+--------+-------------+--------------|
 | Bit offsets |  31 |   30 - 20   |   19   |    18-8     |     7-0      |
 |-------------+-----+-------------+--------+-------------+--------------|
 | What        | GSB | PC (11-bit) | unused | CC (11-bit) | TBCI (8-bit) |
 |-------------+-----+-------------+--------+-------------+--------------|

 GSB  - GDBserver Stamp Bit
 PC   - Previous Counter
 CC   - Current Counter
 TBCI - Trace Buffer Control Index


An IPA update of `trace_buffer_ctrl_curr' does:

    - read CC from the current token, save as PC.
    - updates pointers
    - atomically tries to write PC+1,CC

A GDBserver update of `trace_buffer_ctrl_curr' does:

    - reads PC and CC from the current token.
    - updates pointers
    - writes GSB,PC,CC
*/

/* These are the bits of `trace_buffer_ctrl_curr' that are reserved
   for the counters described below.  The cleared bits are used to
   hold the index of the items of the `trace_buffer_ctrl' array that
   is "current".  */
#define GDBSERVER_FLUSH_COUNT_MASK        0xfffffff0

/* `trace_buffer_ctrl_curr' contains two counters.  The `previous'
   counter, and the `current' counter.  */

#define GDBSERVER_FLUSH_COUNT_MASK_PREV   0x7ff00000
#define GDBSERVER_FLUSH_COUNT_MASK_CURR   0x0007ff00

/* When GDBserver update the IP agent's `trace_buffer_ctrl_curr', it
   always stamps this bit as set.  */
#define GDBSERVER_UPDATED_FLUSH_COUNT_BIT 0x80000000

#ifdef IN_PROCESS_AGENT
IP_AGENT_EXPORT_VAR struct trace_buffer_control trace_buffer_ctrl[3];
IP_AGENT_EXPORT_VAR unsigned int trace_buffer_ctrl_curr;

# define TRACE_BUFFER_CTRL_CURR \
  (trace_buffer_ctrl_curr & ~GDBSERVER_FLUSH_COUNT_MASK)

#else

/* The GDBserver side agent only needs one instance of this object, as
   it doesn't need to sync with itself.  Define it as array anyway so
   that the rest of the code base doesn't need to care for the
   difference.  */
static trace_buffer_control trace_buffer_ctrl[1];
# define TRACE_BUFFER_CTRL_CURR 0
#endif

/* These are convenience macros used to access the current trace
   buffer control in effect.  */
#define trace_buffer_start (trace_buffer_ctrl[TRACE_BUFFER_CTRL_CURR].start)
#define trace_buffer_free (trace_buffer_ctrl[TRACE_BUFFER_CTRL_CURR].free)
#define trace_buffer_end_free \
  (trace_buffer_ctrl[TRACE_BUFFER_CTRL_CURR].end_free)
#define trace_buffer_wrap (trace_buffer_ctrl[TRACE_BUFFER_CTRL_CURR].wrap)


/* Macro that returns a pointer to the first traceframe in the buffer.  */

#define FIRST_TRACEFRAME() ((struct traceframe *) trace_buffer_start)

/* Macro that returns a pointer to the next traceframe in the buffer.
   If the computed location is beyond the wraparound point, subtract
   the offset of the wraparound.  */

#define NEXT_TRACEFRAME_1(TF) \
  (((unsigned char *) (TF)) + sizeof (struct traceframe) + (TF)->data_size)

#define NEXT_TRACEFRAME(TF) \
  ((struct traceframe *) (NEXT_TRACEFRAME_1 (TF)  \
			  - ((NEXT_TRACEFRAME_1 (TF) >= trace_buffer_wrap) \
			     ? (trace_buffer_wrap - trace_buffer_lo)	\
			     : 0)))

/* The difference between these counters represents the total number
   of complete traceframes present in the trace buffer.  The IP agent
   writes to the write count, GDBserver writes to read count.  */

IP_AGENT_EXPORT_VAR unsigned int traceframe_write_count;
IP_AGENT_EXPORT_VAR unsigned int traceframe_read_count;

/* Convenience macro.  */

#define traceframe_count \
  ((unsigned int) (traceframe_write_count - traceframe_read_count))

/* The count of all traceframes created in the current run, including
   ones that were discarded to make room.  */

IP_AGENT_EXPORT_VAR int traceframes_created;

#ifndef IN_PROCESS_AGENT

/* Read-only regions are address ranges whose contents don't change,
   and so can be read from target memory even while looking at a trace
   frame.  Without these, disassembly for instance will likely fail,
   because the program code is not usually collected into a trace
   frame.  This data structure does not need to be very complicated or
   particularly efficient, it's only going to be used occasionally,
   and only by some commands.  */

struct readonly_region
{
  /* The bounds of the region.  */
  CORE_ADDR start, end;

  /* Link to the next one.  */
  struct readonly_region *next;
};

/* Linked list of readonly regions.  This list stays in effect from
   one tstart to the next.  */

static struct readonly_region *readonly_regions;

#endif

/* The global that controls tracing overall.  */

IP_AGENT_EXPORT_VAR int tracing;

#ifndef IN_PROCESS_AGENT

/* Controls whether tracing should continue after GDB disconnects.  */

int disconnected_tracing;

/* The reason for the last tracing run to have stopped.  We initialize
   to a distinct string so that GDB can distinguish between "stopped
   after running" and "stopped because never run" cases.  */

static const char *tracing_stop_reason = "tnotrun";

static int tracing_stop_tpnum;

/* 64-bit timestamps for the trace run's start and finish, expressed
   in microseconds from the Unix epoch.  */

static LONGEST tracing_start_time;
static LONGEST tracing_stop_time;

/* The (optional) user-supplied name of the user that started the run.
   This is an arbitrary string, and may be NULL.  */

static char *tracing_user_name;

/* Optional user-supplied text describing the run.  This is
   an arbitrary string, and may be NULL.  */

static char *tracing_notes;

/* Optional user-supplied text explaining a tstop command.  This is an
   arbitrary string, and may be NULL.  */

static char *tracing_stop_note;

#endif

/* Functions local to this file.  */

/* Base "class" for tracepoint type specific data to be passed down to
   collect_data_at_tracepoint.  */
struct tracepoint_hit_ctx
{
  enum tracepoint_type type;
};

#ifdef IN_PROCESS_AGENT

/* Fast/jump tracepoint specific data to be passed down to
   collect_data_at_tracepoint.  */
struct fast_tracepoint_ctx
{
  struct tracepoint_hit_ctx base;

  struct regcache regcache;
  int regcache_initted;
  unsigned char *regspace;

  unsigned char *regs;
  struct tracepoint *tpoint;
};

/* Static tracepoint specific data to be passed down to
   collect_data_at_tracepoint.  */
struct static_tracepoint_ctx
{
  struct tracepoint_hit_ctx base;

  /* The regcache corresponding to the registers state at the time of
     the tracepoint hit.  Initialized lazily, from REGS.  */
  struct regcache regcache;
  int regcache_initted;

  /* The buffer space REGCACHE above uses.  We use a separate buffer
     instead of letting the regcache malloc for both signal safety and
     performance reasons; this is allocated on the stack instead.  */
  unsigned char *regspace;

  /* The register buffer as passed on by lttng/ust.  */
  struct registers *regs;

  /* The "printf" formatter and the args the user passed to the marker
     call.  We use this to be able to collect "static trace data"
     ($_sdata).  */
  const char *fmt;
  va_list *args;

  /* The GDB tracepoint matching the probed marker that was "hit".  */
  struct tracepoint *tpoint;
};

#else

/* Static tracepoint specific data to be passed down to
   collect_data_at_tracepoint.  */
struct trap_tracepoint_ctx
{
  struct tracepoint_hit_ctx base;

  struct regcache *regcache;
};

#endif

#ifndef IN_PROCESS_AGENT
static CORE_ADDR traceframe_get_pc (struct traceframe *tframe);
static int traceframe_read_tsv (int num, LONGEST *val);
#endif

static int condition_true_at_tracepoint (struct tracepoint_hit_ctx *ctx,
					 struct tracepoint *tpoint);

#ifndef IN_PROCESS_AGENT
static void clear_readonly_regions (void);
static void clear_installed_tracepoints (void);
#endif

static void collect_data_at_tracepoint (struct tracepoint_hit_ctx *ctx,
					CORE_ADDR stop_pc,
					struct tracepoint *tpoint);
#ifndef IN_PROCESS_AGENT
static void collect_data_at_step (struct tracepoint_hit_ctx *ctx,
				  CORE_ADDR stop_pc,
				  struct tracepoint *tpoint, int current_step);
static void compile_tracepoint_condition (struct tracepoint *tpoint,
					  CORE_ADDR *jump_entry);
#endif
static void do_action_at_tracepoint (struct tracepoint_hit_ctx *ctx,
				     CORE_ADDR stop_pc,
				     struct tracepoint *tpoint,
				     struct traceframe *tframe,
				     struct tracepoint_action *taction);

#ifndef IN_PROCESS_AGENT
static struct tracepoint *fast_tracepoint_from_ipa_tpoint_address (CORE_ADDR);

static void install_tracepoint (struct tracepoint *, char *own_buf);
static void download_tracepoint (struct tracepoint *);
static int install_fast_tracepoint (struct tracepoint *, char *errbuf);
static void clone_fast_tracepoint (struct tracepoint *to,
				   const struct tracepoint *from);
#endif

static LONGEST get_timestamp (void);

#if defined(__GNUC__)
#  define memory_barrier() asm volatile ("" : : : "memory")
#else
#  define memory_barrier() do {} while (0)
#endif

/* We only build the IPA if this builtin is supported, and there are
   no uses of this in GDBserver itself, so we're safe in defining this
   unconditionally.  */
#define cmpxchg(mem, oldval, newval) \
  __sync_val_compare_and_swap (mem, oldval, newval)

/* Record that an error occurred during expression evaluation.  */

static void
record_tracepoint_error (struct tracepoint *tpoint, const char *which,
			 enum eval_result_type rtype)
{
  trace_debug ("Tracepoint %d at %s %s eval reports error %d",
	       tpoint->number, paddress (tpoint->address), which, rtype);

#ifdef IN_PROCESS_AGENT
  /* Only record the first error we get.  */
  if (cmpxchg (&expr_eval_result,
	       expr_eval_no_error,
	       rtype) != expr_eval_no_error)
    return;
#else
  if (expr_eval_result != expr_eval_no_error)
    return;
#endif

  error_tracepoint = tpoint;
}

/* Trace buffer management.  */

static void
clear_trace_buffer (void)
{
  trace_buffer_start = trace_buffer_lo;
  trace_buffer_free = trace_buffer_lo;
  trace_buffer_end_free = trace_buffer_hi;
  trace_buffer_wrap = trace_buffer_hi;
  /* A traceframe with zeroed fields marks the end of trace data.  */
  ((struct traceframe *) trace_buffer_free)->tpnum = 0;
  ((struct traceframe *) trace_buffer_free)->data_size = 0;
  traceframe_read_count = traceframe_write_count = 0;
  traceframes_created = 0;
}

#ifndef IN_PROCESS_AGENT

static void
clear_inferior_trace_buffer (void)
{
  CORE_ADDR ipa_trace_buffer_lo;
  CORE_ADDR ipa_trace_buffer_hi;
  struct traceframe ipa_traceframe = { 0 };
  struct ipa_trace_buffer_control ipa_trace_buffer_ctrl;

  read_inferior_data_pointer (ipa_sym_addrs.addr_trace_buffer_lo,
			      &ipa_trace_buffer_lo);
  read_inferior_data_pointer (ipa_sym_addrs.addr_trace_buffer_hi,
			      &ipa_trace_buffer_hi);

  ipa_trace_buffer_ctrl.start = ipa_trace_buffer_lo;
  ipa_trace_buffer_ctrl.free = ipa_trace_buffer_lo;
  ipa_trace_buffer_ctrl.end_free = ipa_trace_buffer_hi;
  ipa_trace_buffer_ctrl.wrap = ipa_trace_buffer_hi;

  /* A traceframe with zeroed fields marks the end of trace data.  */
  target_write_memory (ipa_sym_addrs.addr_trace_buffer_ctrl,
			 (unsigned char *) &ipa_trace_buffer_ctrl,
			 sizeof (ipa_trace_buffer_ctrl));

  write_inferior_uinteger (ipa_sym_addrs.addr_trace_buffer_ctrl_curr, 0);

  /* A traceframe with zeroed fields marks the end of trace data.  */
  target_write_memory (ipa_trace_buffer_lo,
			 (unsigned char *) &ipa_traceframe,
			 sizeof (ipa_traceframe));

  write_inferior_uinteger (ipa_sym_addrs.addr_traceframe_write_count, 0);
  write_inferior_uinteger (ipa_sym_addrs.addr_traceframe_read_count, 0);
  write_inferior_integer (ipa_sym_addrs.addr_traceframes_created, 0);
}

#endif

static void
init_trace_buffer (LONGEST bufsize)
{
  size_t alloc_size;

  trace_buffer_size = bufsize;

  /* Make sure to internally allocate at least space for the EOB
     marker.  */
  alloc_size = (bufsize < TRACEFRAME_EOB_MARKER_SIZE
		? TRACEFRAME_EOB_MARKER_SIZE : bufsize);
  trace_buffer_lo = (unsigned char *) xrealloc (trace_buffer_lo, alloc_size);

  trace_buffer_hi = trace_buffer_lo + trace_buffer_size;

  clear_trace_buffer ();
}

#ifdef IN_PROCESS_AGENT

/* This is needed for -Wmissing-declarations.  */
IP_AGENT_EXPORT_FUNC void about_to_request_buffer_space (void);

IP_AGENT_EXPORT_FUNC void
about_to_request_buffer_space (void)
{
  /* GDBserver places breakpoint here while it goes about to flush
     data at random times.  */
  UNKNOWN_SIDE_EFFECTS();
}

#endif

/* Carve out a piece of the trace buffer, returning NULL in case of
   failure.  */

static void *
trace_buffer_alloc (size_t amt)
{
  unsigned char *rslt;
  struct trace_buffer_control *tbctrl;
  unsigned int curr;
#ifdef IN_PROCESS_AGENT
  unsigned int prev, prev_filtered;
  unsigned int commit_count;
  unsigned int commit;
  unsigned int readout;
#else
  struct traceframe *oldest;
  unsigned char *new_start;
#endif

  trace_debug ("Want to allocate %ld+%ld bytes in trace buffer",
	       (long) amt, (long) sizeof (struct traceframe));

  /* Account for the EOB marker.  */
  amt += TRACEFRAME_EOB_MARKER_SIZE;

#ifdef IN_PROCESS_AGENT
 again:
  memory_barrier ();

  /* Read the current token and extract the index to try to write to,
     storing it in CURR.  */
  prev = trace_buffer_ctrl_curr;
  prev_filtered = prev & ~GDBSERVER_FLUSH_COUNT_MASK;
  curr = prev_filtered + 1;
  if (curr > 2)
    curr = 0;

  about_to_request_buffer_space ();

  /* Start out with a copy of the current state.  GDBserver may be
     midway writing to the PREV_FILTERED TBC, but, that's OK, we won't
     be able to commit anyway if that happens.  */
  trace_buffer_ctrl[curr]
    = trace_buffer_ctrl[prev_filtered];
  trace_debug ("trying curr=%u", curr);
#else
  /* The GDBserver's agent doesn't need all that syncing, and always
     updates TCB 0 (there's only one, mind you).  */
  curr = 0;
#endif
  tbctrl = &trace_buffer_ctrl[curr];

  /* Offsets are easier to grok for debugging than raw addresses,
     especially for the small trace buffer sizes that are useful for
     testing.  */
  trace_debug ("Trace buffer [%d] start=%d free=%d endfree=%d wrap=%d hi=%d",
	       curr,
	       (int) (tbctrl->start - trace_buffer_lo),
	       (int) (tbctrl->free - trace_buffer_lo),
	       (int) (tbctrl->end_free - trace_buffer_lo),
	       (int) (tbctrl->wrap - trace_buffer_lo),
	       (int) (trace_buffer_hi - trace_buffer_lo));

  /* The algorithm here is to keep trying to get a contiguous block of
     the requested size, possibly discarding older traceframes to free
     up space.  Since free space might come in one or two pieces,
     depending on whether discarded traceframes wrapped around at the
     high end of the buffer, we test both pieces after each
     discard.  */
  while (1)
    {
      /* First, if we have two free parts, try the upper one first.  */
      if (tbctrl->end_free < tbctrl->free)
	{
	  if (tbctrl->free + amt <= trace_buffer_hi)
	    /* We have enough in the upper part.  */
	    break;
	  else
	    {
	      /* Our high part of free space wasn't enough.  Give up
		 on it for now, set wraparound.  We will recover the
		 space later, if/when the wrapped-around traceframe is
		 discarded.  */
	      trace_debug ("Upper part too small, setting wraparound");
	      tbctrl->wrap = tbctrl->free;
	      tbctrl->free = trace_buffer_lo;
	    }
	}

      /* The normal case.  */
      if (tbctrl->free + amt <= tbctrl->end_free)
	break;

#ifdef IN_PROCESS_AGENT
      /* The IP Agent's buffer is always circular.  It isn't used
	 currently, but `circular_trace_buffer' could represent
	 GDBserver's mode.  If we didn't find space, ask GDBserver to
	 flush.  */

      flush_trace_buffer ();
      memory_barrier ();
      if (tracing)
	{
	  trace_debug ("gdbserver flushed buffer, retrying");
	  goto again;
	}

      /* GDBserver cancelled the tracing.  Bail out as well.  */
      return NULL;
#else
      /* If we're here, then neither part is big enough, and
	 non-circular trace buffers are now full.  */
      if (!circular_trace_buffer)
	{
	  trace_debug ("Not enough space in the trace buffer");
	  return NULL;
	}

      trace_debug ("Need more space in the trace buffer");

      /* If we have a circular buffer, we can try discarding the
	 oldest traceframe and see if that helps.  */
      oldest = FIRST_TRACEFRAME ();
      if (oldest->tpnum == 0)
	{
	  /* Not good; we have no traceframes to free.  Perhaps we're
	     asking for a block that is larger than the buffer?  In
	     any case, give up.  */
	  trace_debug ("No traceframes to discard");
	  return NULL;
	}

      /* We don't run this code in the in-process agent currently.
	 E.g., we could leave the in-process agent in autonomous
	 circular mode if we only have fast tracepoints.  If we do
	 that, then this bit becomes racy with GDBserver, which also
	 writes to this counter.  */
      --traceframe_write_count;

      new_start = (unsigned char *) NEXT_TRACEFRAME (oldest);
      /* If we freed the traceframe that wrapped around, go back
	 to the non-wrap case.  */
      if (new_start < tbctrl->start)
	{
	  trace_debug ("Discarding past the wraparound");
	  tbctrl->wrap = trace_buffer_hi;
	}
      tbctrl->start = new_start;
      tbctrl->end_free = tbctrl->start;

      trace_debug ("Discarded a traceframe\n"
		   "Trace buffer [%d], start=%d free=%d "
		   "endfree=%d wrap=%d hi=%d",
		   curr,
		   (int) (tbctrl->start - trace_buffer_lo),
		   (int) (tbctrl->free - trace_buffer_lo),
		   (int) (tbctrl->end_free - trace_buffer_lo),
		   (int) (tbctrl->wrap - trace_buffer_lo),
		   (int) (trace_buffer_hi - trace_buffer_lo));

      /* Now go back around the loop.  The discard might have resulted
	 in either one or two pieces of free space, so we want to try
	 both before freeing any more traceframes.  */
#endif
    }

  /* If we get here, we know we can provide the asked-for space.  */

  rslt = tbctrl->free;

  /* Adjust the request back down, now that we know we have space for
     the marker, but don't commit to AMT yet, we may still need to
     restart the operation if GDBserver touches the trace buffer
     (obviously only important in the in-process agent's version).  */
  tbctrl->free += (amt - sizeof (struct traceframe));

  /* Or not.  If GDBserver changed the trace buffer behind our back,
     we get to restart a new allocation attempt.  */

#ifdef IN_PROCESS_AGENT
  /* Build the tentative token.  */
  commit_count = (((prev & GDBSERVER_FLUSH_COUNT_MASK_CURR) + 0x100)
		  & GDBSERVER_FLUSH_COUNT_MASK_CURR);
  commit = (((prev & GDBSERVER_FLUSH_COUNT_MASK_CURR) << 12)
	    | commit_count
	    | curr);

  /* Try to commit it.  */
  readout = cmpxchg (&trace_buffer_ctrl_curr, prev, commit);
  if (readout != prev)
    {
      trace_debug ("GDBserver has touched the trace buffer, restarting."
		   " (prev=%08x, commit=%08x, readout=%08x)",
		   prev, commit, readout);
      goto again;
    }

  /* Hold your horses here.  Even if that change was committed,
     GDBserver could come in, and clobber it.  We need to hold to be
     able to tell if GDBserver clobbers before or after we committed
     the change.  Whenever GDBserver goes about touching the IPA
     buffer, it sets a breakpoint in this routine, so we have a sync
     point here.  */
  about_to_request_buffer_space ();

  /* Check if the change has been effective, even if GDBserver stopped
     us at the breakpoint.  */

  {
    unsigned int refetch;

    memory_barrier ();

    refetch = trace_buffer_ctrl_curr;

    if (refetch == commit
	|| ((refetch & GDBSERVER_FLUSH_COUNT_MASK_PREV) >> 12) == commit_count)
      {
	/* effective */
	trace_debug ("change is effective: (prev=%08x, commit=%08x, "
		     "readout=%08x, refetch=%08x)",
		     prev, commit, readout, refetch);
      }
    else
      {
	trace_debug ("GDBserver has touched the trace buffer, not effective."
		     " (prev=%08x, commit=%08x, readout=%08x, refetch=%08x)",
		     prev, commit, readout, refetch);
	goto again;
      }
  }
#endif

  /* We have a new piece of the trace buffer.  Hurray!  */

  /* Add an EOB marker just past this allocation.  */
  ((struct traceframe *) tbctrl->free)->tpnum = 0;
  ((struct traceframe *) tbctrl->free)->data_size = 0;

  /* Adjust the request back down, now that we know we have space for
     the marker.  */
  amt -= sizeof (struct traceframe);

  if (debug_threads)
    {
      trace_debug ("Allocated %d bytes", (int) amt);
      trace_debug ("Trace buffer [%d] start=%d free=%d "
		   "endfree=%d wrap=%d hi=%d",
		   curr,
		   (int) (tbctrl->start - trace_buffer_lo),
		   (int) (tbctrl->free - trace_buffer_lo),
		   (int) (tbctrl->end_free - trace_buffer_lo),
		   (int) (tbctrl->wrap - trace_buffer_lo),
		   (int) (trace_buffer_hi - trace_buffer_lo));
    }

  return rslt;
}

#ifndef IN_PROCESS_AGENT

/* Return the total free space.  This is not necessarily the largest
   block we can allocate, because of the two-part case.  */

static int
free_space (void)
{
  if (trace_buffer_free <= trace_buffer_end_free)
    return trace_buffer_end_free - trace_buffer_free;
  else
    return ((trace_buffer_end_free - trace_buffer_lo)
	    + (trace_buffer_hi - trace_buffer_free));
}

/* An 'S' in continuation packets indicates remainder are for
   while-stepping.  */

static int seen_step_action_flag;

/* Create a tracepoint (location) with given number and address.  Add this
   new tracepoint to list and sort this list.  */

static struct tracepoint *
add_tracepoint (int num, CORE_ADDR addr)
{
  struct tracepoint *tpoint, **tp_next;

  tpoint = XNEW (struct tracepoint);
  tpoint->number = num;
  tpoint->address = addr;
  tpoint->numactions = 0;
  tpoint->actions = NULL;
  tpoint->actions_str = NULL;
  tpoint->cond = NULL;
  tpoint->num_step_actions = 0;
  tpoint->step_actions = NULL;
  tpoint->step_actions_str = NULL;
  /* Start all off as regular (slow) tracepoints.  */
  tpoint->type = trap_tracepoint;
  tpoint->orig_size = -1;
  tpoint->source_strings = NULL;
  tpoint->compiled_cond = 0;
  tpoint->handle = NULL;
  tpoint->next = NULL;

  /* Find a place to insert this tracepoint into list in order to keep
     the tracepoint list still in the ascending order.  There may be
     multiple tracepoints at the same address as TPOINT's, and this
     guarantees TPOINT is inserted after all the tracepoints which are
     set at the same address.  For example, fast tracepoints A, B, C are
     set at the same address, and D is to be insert at the same place as
     well,

     -->| A |--> | B |-->| C |->...

     One jump pad was created for tracepoint A, B, and C, and the target
     address of A is referenced/used in jump pad.  So jump pad will let
     inferior jump to A.  If D is inserted in front of A, like this,

     -->| D |-->| A |--> | B |-->| C |->...

     without updating jump pad, D is not reachable during collect, which
     is wrong.  As we can see, the order of B, C and D doesn't matter, but
     A should always be the `first' one.  */
  for (tp_next = &tracepoints;
       (*tp_next) != NULL && (*tp_next)->address <= tpoint->address;
       tp_next = &(*tp_next)->next)
    ;
  tpoint->next = *tp_next;
  *tp_next = tpoint;
  last_tracepoint = tpoint;

  seen_step_action_flag = 0;

  return tpoint;
}

#ifndef IN_PROCESS_AGENT

/* Return the tracepoint with the given number and address, or NULL.  */

static struct tracepoint *
find_tracepoint (int id, CORE_ADDR addr)
{
  struct tracepoint *tpoint;

  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    if (tpoint->number == id && tpoint->address == addr)
      return tpoint;

  return NULL;
}

/* Remove TPOINT from global list.  */

static void
remove_tracepoint (struct tracepoint *tpoint)
{
  struct tracepoint *tp, *tp_prev;

  for (tp = tracepoints, tp_prev = NULL; tp && tp != tpoint;
       tp_prev = tp, tp = tp->next)
    ;

  if (tp)
    {
      if (tp_prev)
	tp_prev->next = tp->next;
      else
	tracepoints = tp->next;

      xfree (tp);
    }
}

/* There may be several tracepoints with the same number (because they
   are "locations", in GDB parlance); return the next one after the
   given tracepoint, or search from the beginning of the list if the
   first argument is NULL.  */

static struct tracepoint *
find_next_tracepoint_by_number (struct tracepoint *prev_tp, int num)
{
  struct tracepoint *tpoint;

  if (prev_tp)
    tpoint = prev_tp->next;
  else
    tpoint = tracepoints;
  for (; tpoint; tpoint = tpoint->next)
    if (tpoint->number == num)
      return tpoint;

  return NULL;
}

#endif

/* Append another action to perform when the tracepoint triggers.  */

static void
add_tracepoint_action (struct tracepoint *tpoint, const char *packet)
{
  const char *act;

  if (*packet == 'S')
    {
      seen_step_action_flag = 1;
      ++packet;
    }

  act = packet;

  while (*act)
    {
      const char *act_start = act;
      struct tracepoint_action *action = NULL;

      switch (*act)
	{
	case 'M':
	  {
	    struct collect_memory_action *maction =
	      XNEW (struct collect_memory_action);
	    ULONGEST basereg;
	    int is_neg;

	    maction->base.type = *act;
	    action = &maction->base;

	    ++act;
	    is_neg = (*act == '-');
	    if (*act == '-')
	      ++act;
	    act = unpack_varlen_hex (act, &basereg);
	    ++act;
	    act = unpack_varlen_hex (act, &maction->addr);
	    ++act;
	    act = unpack_varlen_hex (act, &maction->len);
	    maction->basereg = (is_neg
				? - (int) basereg
				: (int) basereg);
	    trace_debug ("Want to collect %s bytes at 0x%s (basereg %d)",
			 pulongest (maction->len),
			 paddress (maction->addr), maction->basereg);
	    break;
	  }
	case 'R':
	  {
	    struct collect_registers_action *raction =
	      XNEW (struct collect_registers_action);

	    raction->base.type = *act;
	    action = &raction->base;

	    trace_debug ("Want to collect registers");
	    ++act;
	    /* skip past hex digits of mask for now */
	    while (isxdigit(*act))
	      ++act;
	    break;
	  }
	case 'L':
	  {
	    struct collect_static_trace_data_action *raction =
	      XNEW (struct collect_static_trace_data_action);

	    raction->base.type = *act;
	    action = &raction->base;

	    trace_debug ("Want to collect static trace data");
	    ++act;
	    break;
	  }
	case 'S':
	  trace_debug ("Unexpected step action, ignoring");
	  ++act;
	  break;
	case 'X':
	  {
	    struct eval_expr_action *xaction = XNEW (struct eval_expr_action);

	    xaction->base.type = *act;
	    action = &xaction->base;

	    trace_debug ("Want to evaluate expression");
	    xaction->expr = gdb_parse_agent_expr (&act);
	    break;
	  }
	default:
	  trace_debug ("unknown trace action '%c', ignoring...", *act);
	  break;
	case '-':
	  break;
	}

      if (action == NULL)
	break;

      if (seen_step_action_flag)
	{
	  tpoint->num_step_actions++;

	  tpoint->step_actions
	    = XRESIZEVEC (struct tracepoint_action *, tpoint->step_actions,
			  tpoint->num_step_actions);
	  tpoint->step_actions_str
	    = XRESIZEVEC (char *, tpoint->step_actions_str,
			  tpoint->num_step_actions);
	  tpoint->step_actions[tpoint->num_step_actions - 1] = action;
	  tpoint->step_actions_str[tpoint->num_step_actions - 1]
	    = savestring (act_start, act - act_start);
	}
      else
	{
	  tpoint->numactions++;
	  tpoint->actions
	    = XRESIZEVEC (struct tracepoint_action *, tpoint->actions,
			  tpoint->numactions);
	  tpoint->actions_str
	    = XRESIZEVEC (char *, tpoint->actions_str, tpoint->numactions);
	  tpoint->actions[tpoint->numactions - 1] = action;
	  tpoint->actions_str[tpoint->numactions - 1]
	    = savestring (act_start, act - act_start);
	}
    }
}

#endif

/* Find or create a trace state variable with the given number.  */

static struct trace_state_variable *
get_trace_state_variable (int num)
{
  struct trace_state_variable *tsv;

#ifdef IN_PROCESS_AGENT
  /* Search for an existing variable.  */
  for (tsv = alloced_trace_state_variables; tsv; tsv = tsv->next)
    if (tsv->number == num)
      return tsv;
#endif

  /* Search for an existing variable.  */
  for (tsv = trace_state_variables; tsv; tsv = tsv->next)
    if (tsv->number == num)
      return tsv;

  return NULL;
}

/* Find or create a trace state variable with the given number.  */

static struct trace_state_variable *
create_trace_state_variable (int num, int gdb)
{
  struct trace_state_variable *tsv;

  tsv = get_trace_state_variable (num);
  if (tsv != NULL)
    return tsv;

  /* Create a new variable.  */
  tsv = XNEW (struct trace_state_variable);
  tsv->number = num;
  tsv->initial_value = 0;
  tsv->value = 0;
  tsv->getter = NULL;
  tsv->name = NULL;
#ifdef IN_PROCESS_AGENT
  if (!gdb)
    {
      tsv->next = alloced_trace_state_variables;
      alloced_trace_state_variables = tsv;
    }
  else
#endif
    {
      tsv->next = trace_state_variables;
      trace_state_variables = tsv;
    }
  return tsv;
}

/* This is needed for -Wmissing-declarations.  */
IP_AGENT_EXPORT_FUNC LONGEST get_trace_state_variable_value (int num);

IP_AGENT_EXPORT_FUNC LONGEST
get_trace_state_variable_value (int num)
{
  struct trace_state_variable *tsv;

  tsv = get_trace_state_variable (num);

  if (!tsv)
    {
      trace_debug ("No trace state variable %d, skipping value get", num);
      return 0;
    }

  /* Call a getter function if we have one.  While it's tempting to
     set up something to only call the getter once per tracepoint hit,
     it could run afoul of thread races. Better to let the getter
     handle it directly, if necessary to worry about it.  */
  if (tsv->getter)
    tsv->value = (tsv->getter) ();

  trace_debug ("get_trace_state_variable_value(%d) ==> %s",
	       num, plongest (tsv->value));

  return tsv->value;
}

/* This is needed for -Wmissing-declarations.  */
IP_AGENT_EXPORT_FUNC void set_trace_state_variable_value (int num,
							  LONGEST val);

IP_AGENT_EXPORT_FUNC void
set_trace_state_variable_value (int num, LONGEST val)
{
  struct trace_state_variable *tsv;

  tsv = get_trace_state_variable (num);

  if (!tsv)
    {
      trace_debug ("No trace state variable %d, skipping value set", num);
      return;
    }

  tsv->value = val;
}

LONGEST
agent_get_trace_state_variable_value (int num)
{
  return get_trace_state_variable_value (num);
}

void
agent_set_trace_state_variable_value (int num, LONGEST val)
{
  set_trace_state_variable_value (num, val);
}

static void
set_trace_state_variable_name (int num, const char *name)
{
  struct trace_state_variable *tsv;

  tsv = get_trace_state_variable (num);

  if (!tsv)
    {
      trace_debug ("No trace state variable %d, skipping name set", num);
      return;
    }

  tsv->name = (char *) name;
}

static void
set_trace_state_variable_getter (int num, LONGEST (*getter) (void))
{
  struct trace_state_variable *tsv;

  tsv = get_trace_state_variable (num);

  if (!tsv)
    {
      trace_debug ("No trace state variable %d, skipping getter set", num);
      return;
    }

  tsv->getter = getter;
}

/* Add a raw traceframe for the given tracepoint.  */

static struct traceframe *
add_traceframe (struct tracepoint *tpoint)
{
  struct traceframe *tframe;

  tframe
    = (struct traceframe *) trace_buffer_alloc (sizeof (struct traceframe));

  if (tframe == NULL)
    return NULL;

  tframe->tpnum = tpoint->number;
  tframe->data_size = 0;

  return tframe;
}

/* Add a block to the traceframe currently being worked on.  */

static unsigned char *
add_traceframe_block (struct traceframe *tframe,
		      struct tracepoint *tpoint, int amt)
{
  unsigned char *block;

  if (!tframe)
    return NULL;

  block = (unsigned char *) trace_buffer_alloc (amt);

  if (!block)
    return NULL;

  gdb_assert (tframe->tpnum == tpoint->number);

  tframe->data_size += amt;
  tpoint->traceframe_usage += amt;

  return block;
}

/* Flag that the current traceframe is finished.  */

static void
finish_traceframe (struct traceframe *tframe)
{
  ++traceframe_write_count;
  ++traceframes_created;
}

#ifndef IN_PROCESS_AGENT

/* Given a traceframe number NUM, find the NUMth traceframe in the
   buffer.  */

static struct traceframe *
find_traceframe (int num)
{
  struct traceframe *tframe;
  int tfnum = 0;

  for (tframe = FIRST_TRACEFRAME ();
       tframe->tpnum != 0;
       tframe = NEXT_TRACEFRAME (tframe))
    {
      if (tfnum == num)
	return tframe;
      ++tfnum;
    }

  return NULL;
}

static CORE_ADDR
get_traceframe_address (struct traceframe *tframe)
{
  CORE_ADDR addr;
  struct tracepoint *tpoint;

  addr = traceframe_get_pc (tframe);

  if (addr)
    return addr;

  /* Fallback strategy, will be incorrect for while-stepping frames
     and multi-location tracepoints.  */
  tpoint = find_next_tracepoint_by_number (NULL, tframe->tpnum);
  return tpoint->address;
}

/* Search for the next traceframe whose address is inside or outside
   the given range.  */

static struct traceframe *
find_next_traceframe_in_range (CORE_ADDR lo, CORE_ADDR hi, int inside_p,
			       int *tfnump)
{
  client_state &cs = get_client_state ();
  struct traceframe *tframe;
  CORE_ADDR tfaddr;

  *tfnump = cs.current_traceframe + 1;
  tframe = find_traceframe (*tfnump);
  /* The search is not supposed to wrap around.  */
  if (!tframe)
    {
      *tfnump = -1;
      return NULL;
    }

  for (; tframe->tpnum != 0; tframe = NEXT_TRACEFRAME (tframe))
    {
      tfaddr = get_traceframe_address (tframe);
      if (inside_p
	  ? (lo <= tfaddr && tfaddr <= hi)
	  : (lo > tfaddr || tfaddr > hi))
	return tframe;
      ++*tfnump;
    }

  *tfnump = -1;
  return NULL;
}

/* Search for the next traceframe recorded by the given tracepoint.
   Note that for multi-location tracepoints, this will find whatever
   location appears first.  */

static struct traceframe *
find_next_traceframe_by_tracepoint (int num, int *tfnump)
{
  client_state &cs = get_client_state ();
  struct traceframe *tframe;

  *tfnump = cs.current_traceframe + 1;
  tframe = find_traceframe (*tfnump);
  /* The search is not supposed to wrap around.  */
  if (!tframe)
    {
      *tfnump = -1;
      return NULL;
    }

  for (; tframe->tpnum != 0; tframe = NEXT_TRACEFRAME (tframe))
    {
      if (tframe->tpnum == num)
	return tframe;
      ++*tfnump;
    }

  *tfnump = -1;
  return NULL;
}

#endif

#ifndef IN_PROCESS_AGENT

/* Clear all past trace state.  */

static void
cmd_qtinit (char *packet)
{
  client_state &cs = get_client_state ();
  struct trace_state_variable *tsv, *prev, *next;

  /* Can't do this command without a pid attached.  */
  if (current_thread == NULL)
    {
      write_enn (packet);
      return;
    }

  /* Make sure we don't try to read from a trace frame.  */
  cs.current_traceframe = -1;

  stop_tracing ();

  trace_debug ("Initializing the trace");

  clear_installed_tracepoints ();
  clear_readonly_regions ();

  tracepoints = NULL;
  last_tracepoint = NULL;

  /* Clear out any leftover trace state variables.  Ones with target
     defined getters should be kept however.  */
  prev = NULL;
  tsv = trace_state_variables;
  while (tsv)
    {
      trace_debug ("Looking at var %d", tsv->number);
      if (tsv->getter == NULL)
	{
	  next = tsv->next;
	  if (prev)
	    prev->next = next;
	  else
	    trace_state_variables = next;
	  trace_debug ("Deleting var %d", tsv->number);
	  free (tsv);
	  tsv = next;
	}
      else
	{
	  prev = tsv;
	  tsv = tsv->next;
	}
    }

  clear_trace_buffer ();
  clear_inferior_trace_buffer ();

  write_ok (packet);
}

/* Unprobe the UST marker at ADDRESS.  */

static void
unprobe_marker_at (CORE_ADDR address)
{
  char cmd[IPA_CMD_BUF_SIZE];

  sprintf (cmd, "unprobe_marker_at:%s", paddress (address));
  run_inferior_command (cmd, strlen (cmd) + 1);
}

/* Restore the program to its pre-tracing state.  This routine may be called
   in error situations, so it needs to be careful about only restoring
   from known-valid bits.  */

static void
clear_installed_tracepoints (void)
{
  struct tracepoint *tpoint;
  struct tracepoint *prev_stpoint;

  target_pause_all (true);

  prev_stpoint = NULL;

  /* Restore any bytes overwritten by tracepoints.  */
  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    {
      /* Catch the case where we might try to remove a tracepoint that
	 was never actually installed.  */
      if (tpoint->handle == NULL)
	{
	  trace_debug ("Tracepoint %d at 0x%s was "
		       "never installed, nothing to clear",
		       tpoint->number, paddress (tpoint->address));
	  continue;
	}

      switch (tpoint->type)
	{
	case trap_tracepoint:
	  {
	    struct breakpoint *bp
	      = (struct breakpoint *) tpoint->handle;

	    delete_breakpoint (bp);
	  }
	  break;
	case fast_tracepoint:
	  {
	    struct fast_tracepoint_jump *jump
	      = (struct fast_tracepoint_jump *) tpoint->handle;

	    delete_fast_tracepoint_jump (jump);
	  }
	  break;
	case static_tracepoint:
	  if (prev_stpoint != NULL
	      && prev_stpoint->address == tpoint->address)
	    /* Nothing to do.  We already unprobed a tracepoint set at
	       this marker address (and there can only be one probe
	       per marker).  */
	    ;
	  else
	    {
	      unprobe_marker_at (tpoint->address);
	      prev_stpoint = tpoint;
	    }
	  break;
	}

      tpoint->handle = NULL;
    }

  target_unpause_all (true);
}

/* Parse a packet that defines a tracepoint.  */

static void
cmd_qtdp (char *own_buf)
{
  int tppacket;
  /* Whether there is a trailing hyphen at the end of the QTDP packet.  */
  int trail_hyphen = 0;
  ULONGEST num;
  ULONGEST addr;
  ULONGEST count;
  struct tracepoint *tpoint;
  const char *packet = own_buf;

  packet += strlen ("QTDP:");

  /* A hyphen at the beginning marks a packet specifying actions for a
     tracepoint already supplied.  */
  tppacket = 1;
  if (*packet == '-')
    {
      tppacket = 0;
      ++packet;
    }
  packet = unpack_varlen_hex (packet, &num);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &addr);
  ++packet; /* skip a colon */

  /* See if we already have this tracepoint.  */
  tpoint = find_tracepoint (num, addr);

  if (tppacket)
    {
      /* Duplicate tracepoints are never allowed.  */
      if (tpoint)
	{
	  trace_debug ("Tracepoint error: tracepoint %d"
		       " at 0x%s already exists",
		       (int) num, paddress (addr));
	  write_enn (own_buf);
	  return;
	}

      tpoint = add_tracepoint (num, addr);

      tpoint->enabled = (*packet == 'E');
      ++packet; /* skip 'E' */
      ++packet; /* skip a colon */
      packet = unpack_varlen_hex (packet, &count);
      tpoint->step_count = count;
      ++packet; /* skip a colon */
      packet = unpack_varlen_hex (packet, &count);
      tpoint->pass_count = count;
      /* See if we have any of the additional optional fields.  */
      while (*packet == ':')
	{
	  ++packet;
	  if (*packet == 'F')
	    {
	      tpoint->type = fast_tracepoint;
	      ++packet;
	      packet = unpack_varlen_hex (packet, &count);
	      tpoint->orig_size = count;
	    }
	  else if (*packet == 'S')
	    {
	      tpoint->type = static_tracepoint;
	      ++packet;
	    }
	  else if (*packet == 'X')
	    {
	      tpoint->cond = gdb_parse_agent_expr (&packet);
	    }
	  else if (*packet == '-')
	    break;
	  else if (*packet == '\0')
	    break;
	  else
	    trace_debug ("Unknown optional tracepoint field");
	}
      if (*packet == '-')
	{
	  trail_hyphen = 1;
	  trace_debug ("Also has actions\n");
	}

      trace_debug ("Defined %stracepoint %d at 0x%s, "
		   "enabled %d step %" PRIu64 " pass %" PRIu64,
		   tpoint->type == fast_tracepoint ? "fast "
		   : tpoint->type == static_tracepoint ? "static " : "",
		   tpoint->number, paddress (tpoint->address), tpoint->enabled,
		   tpoint->step_count, tpoint->pass_count);
    }
  else if (tpoint)
    add_tracepoint_action (tpoint, packet);
  else
    {
      trace_debug ("Tracepoint error: tracepoint %d at 0x%s not found",
		   (int) num, paddress (addr));
      write_enn (own_buf);
      return;
    }

  /* Install tracepoint during tracing only once for each tracepoint location.
     For each tracepoint loc, GDB may send multiple QTDP packets, and we can
     determine the last QTDP packet for one tracepoint location by checking
     trailing hyphen in QTDP packet.  */
  if (tracing && !trail_hyphen)
    {
      struct tracepoint *tp = NULL;

      /* Pause all threads temporarily while we patch tracepoints.  */
      target_pause_all (false);

      /* download_tracepoint will update global `tracepoints'
	 list, so it is unsafe to leave threads in jump pad.  */
      target_stabilize_threads ();

      /* Freeze threads.  */
      target_pause_all (true);


      if (tpoint->type != trap_tracepoint)
	{
	  /* Find another fast or static tracepoint at the same address.  */
	  for (tp = tracepoints; tp; tp = tp->next)
	    {
	      if (tp->address == tpoint->address && tp->type == tpoint->type
		  && tp->number != tpoint->number)
		break;
	    }

	  /* TPOINT is installed at the same address as TP.  */
	  if (tp)
	    {
	      if (tpoint->type == fast_tracepoint)
		clone_fast_tracepoint (tpoint, tp);
	      else if (tpoint->type == static_tracepoint)
		tpoint->handle = (void *) -1;
	    }
	}

      if (use_agent && tpoint->type == fast_tracepoint
	  && agent_capability_check (AGENT_CAPA_FAST_TRACE))
	{
	  /* Download and install fast tracepoint by agent.  */
	  if (tracepoint_send_agent (tpoint) == 0)
	    write_ok (own_buf);
	  else
	    {
	      write_enn (own_buf);
	      remove_tracepoint (tpoint);
	    }
	}
      else
	{
	  download_tracepoint (tpoint);

	  if (tpoint->type == trap_tracepoint || tp == NULL)
	    {
	      install_tracepoint (tpoint, own_buf);
	      if (strcmp (own_buf, "OK") != 0)
		remove_tracepoint (tpoint);
	    }
	  else
	    write_ok (own_buf);
	}

      target_unpause_all (true);
      return;
    }

  write_ok (own_buf);
}

static void
cmd_qtdpsrc (char *own_buf)
{
  ULONGEST num, addr, start, slen;
  struct tracepoint *tpoint;
  const char *packet = own_buf;
  const char *saved;
  char *srctype, *src;
  size_t nbytes;
  struct source_string *last, *newlast;

  packet += strlen ("QTDPsrc:");

  packet = unpack_varlen_hex (packet, &num);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &addr);
  ++packet; /* skip a colon */

  /* See if we already have this tracepoint.  */
  tpoint = find_tracepoint (num, addr);

  if (!tpoint)
    {
      trace_debug ("Tracepoint error: tracepoint %d at 0x%s not found",
		   (int) num, paddress (addr));
      write_enn (own_buf);
      return;
    }

  saved = packet;
  packet = strchr (packet, ':');
  srctype = (char *) xmalloc (packet - saved + 1);
  memcpy (srctype, saved, packet - saved);
  srctype[packet - saved] = '\0';
  ++packet;
  packet = unpack_varlen_hex (packet, &start);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &slen);
  ++packet; /* skip a colon */
  src = (char *) xmalloc (slen + 1);
  nbytes = hex2bin (packet, (gdb_byte *) src, strlen (packet) / 2);
  src[nbytes] = '\0';

  newlast = XNEW (struct source_string);
  newlast->type = srctype;
  newlast->str = src;
  newlast->next = NULL;
  /* Always add a source string to the end of the list;
     this keeps sequences of actions/commands in the right
     order.  */
  if (tpoint->source_strings)
    {
      for (last = tpoint->source_strings; last->next; last = last->next)
	;
      last->next = newlast;
    }
  else
    tpoint->source_strings = newlast;

  write_ok (own_buf);
}

static void
cmd_qtdv (char *own_buf)
{
  ULONGEST num, val, builtin;
  char *varname;
  size_t nbytes;
  struct trace_state_variable *tsv;
  const char *packet = own_buf;

  packet += strlen ("QTDV:");

  packet = unpack_varlen_hex (packet, &num);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &val);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &builtin);
  ++packet; /* skip a colon */

  nbytes = strlen (packet) / 2;
  varname = (char *) xmalloc (nbytes + 1);
  nbytes = hex2bin (packet, (gdb_byte *) varname, nbytes);
  varname[nbytes] = '\0';

  tsv = create_trace_state_variable (num, 1);
  tsv->initial_value = (LONGEST) val;
  tsv->name = varname;

  set_trace_state_variable_value (num, (LONGEST) val);

  write_ok (own_buf);
}

static void
cmd_qtenable_disable (char *own_buf, int enable)
{
  const char *packet = own_buf;
  ULONGEST num, addr;
  struct tracepoint *tp;

  packet += strlen (enable ? "QTEnable:" : "QTDisable:");
  packet = unpack_varlen_hex (packet, &num);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &addr);

  tp = find_tracepoint (num, addr);

  if (tp)
    {
      if ((enable && tp->enabled) || (!enable && !tp->enabled))
	{
	  trace_debug ("Tracepoint %d at 0x%s is already %s",
		       (int) num, paddress (addr),
		       enable ? "enabled" : "disabled");
	  write_ok (own_buf);
	  return;
	}

      trace_debug ("%s tracepoint %d at 0x%s",
		   enable ? "Enabling" : "Disabling",
		   (int) num, paddress (addr));

      tp->enabled = enable;

      if (tp->type == fast_tracepoint || tp->type == static_tracepoint)
	{
	  int offset = offsetof (struct tracepoint, enabled);
	  CORE_ADDR obj_addr = tp->obj_addr_on_target + offset;

	  int ret = write_inferior_int8 (obj_addr, enable);
	  if (ret)
	    {
	      trace_debug ("Cannot write enabled flag into "
			   "inferior process memory");
	      write_enn (own_buf);
	      return;
	    }
	}

      write_ok (own_buf);
    }
  else
    {
      trace_debug ("Tracepoint %d at 0x%s not found",
		   (int) num, paddress (addr));
      write_enn (own_buf);
    }
}

static void
cmd_qtv (char *own_buf)
{
  client_state &cs = get_client_state ();
  ULONGEST num;
  LONGEST val = 0;
  int err;
  char *packet = own_buf;

  packet += strlen ("qTV:");
  unpack_varlen_hex (packet, &num);

  if (cs.current_traceframe >= 0)
    {
      err = traceframe_read_tsv ((int) num, &val);
      if (err)
	{
	  strcpy (own_buf, "U");
	  return;
	}
    }
  /* Only make tsv's be undefined before the first trace run.  After a
     trace run is over, the user might want to see the last value of
     the tsv, and it might not be available in a traceframe.  */
  else if (!tracing && strcmp (tracing_stop_reason, "tnotrun") == 0)
    {
      strcpy (own_buf, "U");
      return;
    }
  else
    val = get_trace_state_variable_value (num);

  sprintf (own_buf, "V%s", phex_nz (val, 0));
}

/* Clear out the list of readonly regions.  */

static void
clear_readonly_regions (void)
{
  struct readonly_region *roreg;

  while (readonly_regions)
    {
      roreg = readonly_regions;
      readonly_regions = readonly_regions->next;
      free (roreg);
    }
}

/* Parse the collection of address ranges whose contents GDB believes
   to be unchanging and so can be read directly from target memory
   even while looking at a traceframe.  */

static void
cmd_qtro (char *own_buf)
{
  ULONGEST start, end;
  struct readonly_region *roreg;
  const char *packet = own_buf;

  trace_debug ("Want to mark readonly regions");

  clear_readonly_regions ();

  packet += strlen ("QTro");

  while (*packet == ':')
    {
      ++packet;  /* skip a colon */
      packet = unpack_varlen_hex (packet, &start);
      ++packet;  /* skip a comma */
      packet = unpack_varlen_hex (packet, &end);

      roreg = XNEW (struct readonly_region);
      roreg->start = start;
      roreg->end = end;
      roreg->next = readonly_regions;
      readonly_regions = roreg;
      trace_debug ("Added readonly region from 0x%s to 0x%s",
		   paddress (roreg->start), paddress (roreg->end));
    }

  write_ok (own_buf);
}

/* Test to see if the given range is in our list of readonly ranges.
   We only test for being entirely within a range, GDB is not going to
   send a single memory packet that spans multiple regions.  */

int
in_readonly_region (CORE_ADDR addr, ULONGEST length)
{
  struct readonly_region *roreg;

  for (roreg = readonly_regions; roreg; roreg = roreg->next)
    if (roreg->start <= addr && (addr + length - 1) <= roreg->end)
      return 1;

  return 0;
}

static CORE_ADDR gdb_jump_pad_head;

/* Return the address of the next free jump space.  */

static CORE_ADDR
get_jump_space_head (void)
{
  if (gdb_jump_pad_head == 0)
    {
      if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_jump_pad_buffer,
				      &gdb_jump_pad_head))
	{
	  internal_error ("error extracting jump_pad_buffer");
	}
    }

  return gdb_jump_pad_head;
}

/* Reserve USED bytes from the jump space.  */

static void
claim_jump_space (ULONGEST used)
{
  trace_debug ("claim_jump_space reserves %s bytes at %s",
	       pulongest (used), paddress (gdb_jump_pad_head));
  gdb_jump_pad_head += used;
}

static CORE_ADDR trampoline_buffer_head = 0;
static CORE_ADDR trampoline_buffer_tail;

/* Reserve USED bytes from the trampoline buffer and return the
   address of the start of the reserved space in TRAMPOLINE.  Returns
   non-zero if the space is successfully claimed.  */

int
claim_trampoline_space (ULONGEST used, CORE_ADDR *trampoline)
{
  if (!trampoline_buffer_head)
    {
      if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_trampoline_buffer,
				      &trampoline_buffer_tail))
	{
	  internal_error ("error extracting trampoline_buffer");
	}

      if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_trampoline_buffer_end,
				      &trampoline_buffer_head))
	{
	  internal_error ("error extracting trampoline_buffer_end");
	}
    }

  /* Start claiming space from the top of the trampoline space.  If
     the space is located at the bottom of the virtual address space,
     this reduces the possibility that corruption will occur if a null
     pointer is used to write to memory.  */
  if (trampoline_buffer_head - trampoline_buffer_tail < used)
    {
      trace_debug ("claim_trampoline_space failed to reserve %s bytes",
		   pulongest (used));
      return 0;
    }

  trampoline_buffer_head -= used;

  trace_debug ("claim_trampoline_space reserves %s bytes at %s",
	       pulongest (used), paddress (trampoline_buffer_head));

  *trampoline = trampoline_buffer_head;
  return 1;
}

/* Returns non-zero if there is space allocated for use in trampolines
   for fast tracepoints.  */

int
have_fast_tracepoint_trampoline_buffer (char *buf)
{
  CORE_ADDR trampoline_end, errbuf;

  if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_trampoline_buffer_end,
				  &trampoline_end))
    {
      internal_error ("error extracting trampoline_buffer_end");
    }
  
  if (buf)
    {
      buf[0] = '\0';
      strcpy (buf, "was claiming");
      if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_trampoline_buffer_error,
				  &errbuf))
	{
	  internal_error ("error extracting errbuf");
	}

      read_inferior_memory (errbuf, (unsigned char *) buf, 100);
    }

  return trampoline_end != 0;
}

/* Ask the IPA to probe the marker at ADDRESS.  Returns -1 if running
   the command fails, or 0 otherwise.  If the command ran
   successfully, but probing the marker failed, ERROUT will be filled
   with the error to reply to GDB, and -1 is also returned.  This
   allows directly passing IPA errors to GDB.  */

static int
probe_marker_at (CORE_ADDR address, char *errout)
{
  char cmd[IPA_CMD_BUF_SIZE];
  int err;

  sprintf (cmd, "probe_marker_at:%s", paddress (address));
  err = run_inferior_command (cmd, strlen (cmd) + 1);

  if (err == 0)
    {
      if (*cmd == 'E')
	{
	  strcpy (errout, cmd);
	  return -1;
	}
    }

  return err;
}

static void
clone_fast_tracepoint (struct tracepoint *to, const struct tracepoint *from)
{
  to->jump_pad = from->jump_pad;
  to->jump_pad_end = from->jump_pad_end;
  to->trampoline = from->trampoline;
  to->trampoline_end = from->trampoline_end;
  to->adjusted_insn_addr = from->adjusted_insn_addr;
  to->adjusted_insn_addr_end = from->adjusted_insn_addr_end;
  to->handle = from->handle;

  gdb_assert (from->handle);
  inc_ref_fast_tracepoint_jump ((struct fast_tracepoint_jump *) from->handle);
}

#define MAX_JUMP_SIZE 20

/* Install fast tracepoint.  Return 0 if successful, otherwise return
   non-zero.  */

static int
install_fast_tracepoint (struct tracepoint *tpoint, char *errbuf)
{
  CORE_ADDR jentry, jump_entry;
  CORE_ADDR trampoline;
  CORE_ADDR collect;
  ULONGEST trampoline_size;
  int err = 0;
  /* The jump to the jump pad of the last fast tracepoint
     installed.  */
  unsigned char fjump[MAX_JUMP_SIZE];
  ULONGEST fjump_size;

  if (tpoint->orig_size < target_get_min_fast_tracepoint_insn_len ())
    {
      trace_debug ("Requested a fast tracepoint on an instruction "
		   "that is of less than the minimum length.");
      return 0;
    }

  if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_collect_ptr,
				  &collect))
    {
      error ("error extracting gdb_collect_ptr");
      return 1;
    }

  jentry = jump_entry = get_jump_space_head ();

  trampoline = 0;
  trampoline_size = 0;

  /* Install the jump pad.  */
  err = target_install_fast_tracepoint_jump_pad
    (tpoint->obj_addr_on_target, tpoint->address, collect,
     ipa_sym_addrs.addr_collecting, tpoint->orig_size, &jentry,
     &trampoline, &trampoline_size, fjump, &fjump_size,
     &tpoint->adjusted_insn_addr, &tpoint->adjusted_insn_addr_end, errbuf);

  if (err)
    return 1;

  /* Wire it in.  */
  tpoint->handle = set_fast_tracepoint_jump (tpoint->address, fjump,
					     fjump_size);

  if (tpoint->handle != NULL)
    {
      tpoint->jump_pad = jump_entry;
      tpoint->jump_pad_end = jentry;
      tpoint->trampoline = trampoline;
      tpoint->trampoline_end = trampoline + trampoline_size;

      /* Pad to 8-byte alignment.  */
      jentry = ((jentry + 7) & ~0x7);
      claim_jump_space (jentry - jump_entry);
    }

  return 0;
}


/* Install tracepoint TPOINT, and write reply message in OWN_BUF.  */

static void
install_tracepoint (struct tracepoint *tpoint, char *own_buf)
{
  tpoint->handle = NULL;
  *own_buf = '\0';

  if (tpoint->type == trap_tracepoint)
    {
      /* Tracepoints are installed as memory breakpoints.  Just go
	 ahead and install the trap.  The breakpoints module
	 handles duplicated breakpoints, and the memory read
	 routine handles un-patching traps from memory reads.  */
      tpoint->handle = set_breakpoint_at (tpoint->address,
					  tracepoint_handler);
    }
  else if (tpoint->type == fast_tracepoint || tpoint->type == static_tracepoint)
    {
      if (!agent_loaded_p ())
	{
	  trace_debug ("Requested a %s tracepoint, but fast "
		       "tracepoints aren't supported.",
		       tpoint->type == static_tracepoint ? "static" : "fast");
	  write_e_ipa_not_loaded (own_buf);
	  return;
	}
      if (tpoint->type == static_tracepoint
	  && !in_process_agent_supports_ust ())
	{
	  trace_debug ("Requested a static tracepoint, but static "
		       "tracepoints are not supported.");
	  write_e_ust_not_loaded (own_buf);
	  return;
	}

      if (tpoint->type == fast_tracepoint)
	install_fast_tracepoint (tpoint, own_buf);
      else
	{
	  if (probe_marker_at (tpoint->address, own_buf) == 0)
	    tpoint->handle = (void *) -1;
	}

    }
  else
    internal_error ("Unknown tracepoint type");

  if (tpoint->handle == NULL)
    {
      if (*own_buf == '\0')
	write_enn (own_buf);
    }
  else
    write_ok (own_buf);
}

static void download_tracepoint_1 (struct tracepoint *tpoint);

static void
cmd_qtstart (char *packet)
{
  struct tracepoint *tpoint, *prev_ftpoint, *prev_stpoint;
  CORE_ADDR tpptr = 0, prev_tpptr = 0;

  trace_debug ("Starting the trace");

  /* Pause all threads temporarily while we patch tracepoints.  */
  target_pause_all (false);

  /* Get threads out of jump pads.  Safe to do here, since this is a
     top level command.  And, required to do here, since we're
     deleting/rewriting jump pads.  */

  target_stabilize_threads ();

  /* Freeze threads.  */
  target_pause_all (true);

  /* Sync the fast tracepoints list in the inferior ftlib.  */
  if (agent_loaded_p ())
    download_trace_state_variables ();

  /* No previous fast tpoint yet.  */
  prev_ftpoint = NULL;

  /* No previous static tpoint yet.  */
  prev_stpoint = NULL;

  *packet = '\0';

  if (agent_loaded_p ())
    {
      /* Tell IPA about the correct tdesc.  */
      if (write_inferior_integer (ipa_sym_addrs.addr_ipa_tdesc_idx,
				  target_get_ipa_tdesc_idx ()))
	error ("Error setting ipa_tdesc_idx variable in lib");
    }

  /* Start out empty.  */
  if (agent_loaded_p ())
    write_inferior_data_pointer (ipa_sym_addrs.addr_tracepoints, 0);

  /* Download and install tracepoints.  */
  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    {
      /* Ensure all the hit counts start at zero.  */
      tpoint->hit_count = 0;
      tpoint->traceframe_usage = 0;

      if (tpoint->type == trap_tracepoint)
	{
	  /* Tracepoints are installed as memory breakpoints.  Just go
	     ahead and install the trap.  The breakpoints module
	     handles duplicated breakpoints, and the memory read
	     routine handles un-patching traps from memory reads.  */
	  tpoint->handle = set_breakpoint_at (tpoint->address,
					      tracepoint_handler);
	}
      else if (tpoint->type == fast_tracepoint
	       || tpoint->type == static_tracepoint)
	{
	  if (maybe_write_ipa_not_loaded (packet))
	    {
	      trace_debug ("Requested a %s tracepoint, but fast "
			   "tracepoints aren't supported.",
			   tpoint->type == static_tracepoint
			   ? "static" : "fast");
	      break;
	    }

	  if (tpoint->type == fast_tracepoint)
	    {
	      int use_agent_p
		= use_agent && agent_capability_check (AGENT_CAPA_FAST_TRACE);

	      if (prev_ftpoint != NULL
		  && prev_ftpoint->address == tpoint->address)
		{
		  if (use_agent_p)
		    tracepoint_send_agent (tpoint);
		  else
		    download_tracepoint_1 (tpoint);

		  clone_fast_tracepoint (tpoint, prev_ftpoint);
		}
	      else
		{
		  /* Tracepoint is installed successfully?  */
		  int installed = 0;

		  /* Download and install fast tracepoint by agent.  */
		  if (use_agent_p)
		    installed = !tracepoint_send_agent (tpoint);
		  else
		    {
		      download_tracepoint_1 (tpoint);
		      installed = !install_fast_tracepoint (tpoint, packet);
		    }

		  if (installed)
		    prev_ftpoint = tpoint;
		}
	    }
	  else
	    {
	      if (!in_process_agent_supports_ust ())
		{
		  trace_debug ("Requested a static tracepoint, but static "
			       "tracepoints are not supported.");
		  break;
		}

	      download_tracepoint_1 (tpoint);
	      /* Can only probe a given marker once.  */
	      if (prev_stpoint != NULL
		  && prev_stpoint->address == tpoint->address)
		tpoint->handle = (void *) -1;
	      else
		{
		  if (probe_marker_at (tpoint->address, packet) == 0)
		    {
		      tpoint->handle = (void *) -1;

		      /* So that we can handle multiple static tracepoints
			 at the same address easily.  */
		      prev_stpoint = tpoint;
		    }
		}
	    }

	  prev_tpptr = tpptr;
	  tpptr = tpoint->obj_addr_on_target;

	  if (tpoint == tracepoints)
	    /* First object in list, set the head pointer in the
	       inferior.  */
	    write_inferior_data_pointer (ipa_sym_addrs.addr_tracepoints, tpptr);
	  else
	    write_inferior_data_pointer (prev_tpptr
					 + offsetof (struct tracepoint, next),
					 tpptr);
	}

      /* Any failure in the inner loop is sufficient cause to give
	 up.  */
      if (tpoint->handle == NULL)
	break;
    }

  /* Any error in tracepoint insertion is unacceptable; better to
     address the problem now, than end up with a useless or misleading
     trace run.  */
  if (tpoint != NULL)
    {
      clear_installed_tracepoints ();
      if (*packet == '\0')
	write_enn (packet);
      target_unpause_all (true);
      return;
    }

  stopping_tracepoint = NULL;
  trace_buffer_is_full = 0;
  expr_eval_result = expr_eval_no_error;
  error_tracepoint = NULL;
  tracing_start_time = get_timestamp ();

  /* Tracing is now active, hits will now start being logged.  */
  tracing = 1;

  if (agent_loaded_p ())
    {
      if (write_inferior_integer (ipa_sym_addrs.addr_tracing, 1))
	{
	  internal_error ("Error setting tracing variable in lib");
	}

      if (write_inferior_data_pointer (ipa_sym_addrs.addr_stopping_tracepoint,
				       0))
	{
	  internal_error ("Error clearing stopping_tracepoint variable"
			  " in lib");
	}

      if (write_inferior_integer (ipa_sym_addrs.addr_trace_buffer_is_full, 0))
	{
	  internal_error ("Error clearing trace_buffer_is_full variable"
			  " in lib");
	}

      stop_tracing_bkpt = set_breakpoint_at (ipa_sym_addrs.addr_stop_tracing,
					     stop_tracing_handler);
      if (stop_tracing_bkpt == NULL)
	error ("Error setting stop_tracing breakpoint");

      flush_trace_buffer_bkpt
	= set_breakpoint_at (ipa_sym_addrs.addr_flush_trace_buffer,
			     flush_trace_buffer_handler);
      if (flush_trace_buffer_bkpt == NULL)
	error ("Error setting flush_trace_buffer breakpoint");
    }

  target_unpause_all (true);

  write_ok (packet);
}

/* End a tracing run, filling in a stop reason to report back to GDB,
   and removing the tracepoints from the code.  */

void
stop_tracing (void)
{
  if (!tracing)
    {
      trace_debug ("Tracing is already off, ignoring");
      return;
    }

  trace_debug ("Stopping the trace");

  /* Pause all threads before removing fast jumps from memory,
     breakpoints, and touching IPA state variables (inferior memory).
     Some thread may hit the internal tracing breakpoints, or be
     collecting this moment, but that's ok, we don't release the
     tpoint object's memory or the jump pads here (we only do that
     when we're sure we can move all threads out of the jump pads).
     We can't now, since we may be getting here due to the inferior
     agent calling us.  */
  target_pause_all (true);

  /* Stop logging. Tracepoints can still be hit, but they will not be
     recorded.  */
  tracing = 0;
  if (agent_loaded_p ())
    {
      if (write_inferior_integer (ipa_sym_addrs.addr_tracing, 0))
	{
	  internal_error ("Error clearing tracing variable in lib");
	}
    }

  tracing_stop_time = get_timestamp ();
  tracing_stop_reason = "t???";
  tracing_stop_tpnum = 0;
  if (stopping_tracepoint)
    {
      trace_debug ("Stopping the trace because "
		   "tracepoint %d was hit %" PRIu64 " times",
		   stopping_tracepoint->number,
		   stopping_tracepoint->pass_count);
      tracing_stop_reason = "tpasscount";
      tracing_stop_tpnum = stopping_tracepoint->number;
    }
  else if (trace_buffer_is_full)
    {
      trace_debug ("Stopping the trace because the trace buffer is full");
      tracing_stop_reason = "tfull";
    }
  else if (expr_eval_result != expr_eval_no_error)
    {
      trace_debug ("Stopping the trace because of an expression eval error");
      tracing_stop_reason = eval_result_names[expr_eval_result];
      tracing_stop_tpnum = error_tracepoint->number;
    }
#ifndef IN_PROCESS_AGENT
  else if (!gdb_connected ())
    {
      trace_debug ("Stopping the trace because GDB disconnected");
      tracing_stop_reason = "tdisconnected";
    }
#endif
  else
    {
      trace_debug ("Stopping the trace because of a tstop command");
      tracing_stop_reason = "tstop";
    }

  stopping_tracepoint = NULL;
  error_tracepoint = NULL;

  /* Clear out the tracepoints.  */
  clear_installed_tracepoints ();

  if (agent_loaded_p ())
    {
      /* Pull in fast tracepoint trace frames from the inferior lib
	 buffer into our buffer, even if our buffer is already full,
	 because we want to present the full number of created frames
	 in addition to what fit in the trace buffer.  */
      upload_fast_traceframes ();
    }

  if (stop_tracing_bkpt != NULL)
    {
      delete_breakpoint (stop_tracing_bkpt);
      stop_tracing_bkpt = NULL;
    }

  if (flush_trace_buffer_bkpt != NULL)
    {
      delete_breakpoint (flush_trace_buffer_bkpt);
      flush_trace_buffer_bkpt = NULL;
    }

  target_unpause_all (true);
}

static int
stop_tracing_handler (CORE_ADDR addr)
{
  trace_debug ("lib hit stop_tracing");

  /* Don't actually handle it here.  When we stop tracing we remove
     breakpoints from the inferior, and that is not allowed in a
     breakpoint handler (as the caller is walking the breakpoint
     list).  */
  return 0;
}

static int
flush_trace_buffer_handler (CORE_ADDR addr)
{
  trace_debug ("lib hit flush_trace_buffer");
  return 0;
}

static void
cmd_qtstop (char *packet)
{
  stop_tracing ();
  write_ok (packet);
}

static void
cmd_qtdisconnected (char *own_buf)
{
  ULONGEST setting;
  char *packet = own_buf;

  packet += strlen ("QTDisconnected:");

  unpack_varlen_hex (packet, &setting);

  write_ok (own_buf);

  disconnected_tracing = setting;
}

static void
cmd_qtframe (char *own_buf)
{
  client_state &cs = get_client_state ();
  ULONGEST frame, pc, lo, hi, num;
  int tfnum, tpnum;
  struct traceframe *tframe;
  const char *packet = own_buf;

  packet += strlen ("QTFrame:");

  if (startswith (packet, "pc:"))
    {
      packet += strlen ("pc:");
      unpack_varlen_hex (packet, &pc);
      trace_debug ("Want to find next traceframe at pc=0x%s", paddress (pc));
      tframe = find_next_traceframe_in_range (pc, pc, 1, &tfnum);
    }
  else if (startswith (packet, "range:"))
    {
      packet += strlen ("range:");
      packet = unpack_varlen_hex (packet, &lo);
      ++packet;
      unpack_varlen_hex (packet, &hi);
      trace_debug ("Want to find next traceframe in the range 0x%s to 0x%s",
		   paddress (lo), paddress (hi));
      tframe = find_next_traceframe_in_range (lo, hi, 1, &tfnum);
    }
  else if (startswith (packet, "outside:"))
    {
      packet += strlen ("outside:");
      packet = unpack_varlen_hex (packet, &lo);
      ++packet;
      unpack_varlen_hex (packet, &hi);
      trace_debug ("Want to find next traceframe "
		   "outside the range 0x%s to 0x%s",
		   paddress (lo), paddress (hi));
      tframe = find_next_traceframe_in_range (lo, hi, 0, &tfnum);
    }
  else if (startswith (packet, "tdp:"))
    {
      packet += strlen ("tdp:");
      unpack_varlen_hex (packet, &num);
      tpnum = (int) num;
      trace_debug ("Want to find next traceframe for tracepoint %d", tpnum);
      tframe = find_next_traceframe_by_tracepoint (tpnum, &tfnum);
    }
  else
    {
      unpack_varlen_hex (packet, &frame);
      tfnum = (int) frame;
      if (tfnum == -1)
	{
	  trace_debug ("Want to stop looking at traceframes");
	  cs.current_traceframe = -1;
	  write_ok (own_buf);
	  return;
	}
      trace_debug ("Want to look at traceframe %d", tfnum);
      tframe = find_traceframe (tfnum);
    }

  if (tframe)
    {
      cs.current_traceframe = tfnum;
      sprintf (own_buf, "F%xT%x", tfnum, tframe->tpnum);
    }
  else
    sprintf (own_buf, "F-1");
}

static void
cmd_qtstatus (char *packet)
{
  char *stop_reason_rsp = NULL;
  char *buf1, *buf2, *buf3;
  const char *str;
  int slen;

  /* Translate the plain text of the notes back into hex for
     transmission.  */

  str = (tracing_user_name ? tracing_user_name : "");
  slen = strlen (str);
  buf1 = (char *) alloca (slen * 2 + 1);
  bin2hex ((gdb_byte *) str, buf1, slen);

  str = (tracing_notes ? tracing_notes : "");
  slen = strlen (str);
  buf2 = (char *) alloca (slen * 2 + 1);
  bin2hex ((gdb_byte *) str, buf2, slen);

  str = (tracing_stop_note ? tracing_stop_note : "");
  slen = strlen (str);
  buf3 = (char *) alloca (slen * 2 + 1);
  bin2hex ((gdb_byte *) str, buf3, slen);

  trace_debug ("Returning trace status as %d, stop reason %s",
	       tracing, tracing_stop_reason);

  if (agent_loaded_p ())
    {
      target_pause_all (true);

      upload_fast_traceframes ();

      target_unpause_all (true);
   }

  stop_reason_rsp = (char *) tracing_stop_reason;

  /* The user visible error string in terror needs to be hex encoded.
     We leave it as plain string in `tracing_stop_reason' to ease
     debugging.  */
  if (startswith (stop_reason_rsp, "terror:"))
    {
      const char *result_name;
      int hexstr_len;
      char *p;

      result_name = stop_reason_rsp + strlen ("terror:");
      hexstr_len = strlen (result_name) * 2;
      p = stop_reason_rsp
	= (char *) alloca (strlen ("terror:") + hexstr_len + 1);
      strcpy (p, "terror:");
      p += strlen (p);
      bin2hex ((gdb_byte *) result_name, p, strlen (result_name));
    }

  /* If this was a forced stop, include any stop note that was supplied.  */
  if (strcmp (stop_reason_rsp, "tstop") == 0)
    {
      stop_reason_rsp = (char *) alloca (strlen ("tstop:") + strlen (buf3) + 1);
      strcpy (stop_reason_rsp, "tstop:");
      strcat (stop_reason_rsp, buf3);
    }

  sprintf (packet,
	   "T%d;"
	   "%s:%x;"
	   "tframes:%x;tcreated:%x;"
	   "tfree:%x;tsize:%s;"
	   "circular:%d;"
	   "disconn:%d;"
	   "starttime:%s;stoptime:%s;"
	   "username:%s;notes:%s:",
	   tracing ? 1 : 0,
	   stop_reason_rsp, tracing_stop_tpnum,
	   traceframe_count, traceframes_created,
	   free_space (), phex_nz (trace_buffer_hi - trace_buffer_lo, 0),
	   circular_trace_buffer,
	   disconnected_tracing,
	   phex_nz (tracing_start_time, sizeof (tracing_start_time)),
	   phex_nz (tracing_stop_time, sizeof (tracing_stop_time)),
	   buf1, buf2);
}

static void
cmd_qtp (char *own_buf)
{
  ULONGEST num, addr;
  struct tracepoint *tpoint;
  const char *packet = own_buf;

  packet += strlen ("qTP:");

  packet = unpack_varlen_hex (packet, &num);
  ++packet; /* skip a colon */
  packet = unpack_varlen_hex (packet, &addr);

  /* See if we already have this tracepoint.  */
  tpoint = find_tracepoint (num, addr);

  if (!tpoint)
    {
      trace_debug ("Tracepoint error: tracepoint %d at 0x%s not found",
		   (int) num, paddress (addr));
      write_enn (own_buf);
      return;
    }

  sprintf (own_buf, "V%" PRIu64 ":%" PRIu64 "", tpoint->hit_count,
	   tpoint->traceframe_usage);
}

/* State variables to help return all the tracepoint bits.  */
static struct tracepoint *cur_tpoint;
static unsigned int cur_action;
static unsigned int cur_step_action;
static struct source_string *cur_source_string;
static struct trace_state_variable *cur_tsv;

/* Compose a response that is an imitation of the syntax by which the
   tracepoint was originally downloaded.  */

static void
response_tracepoint (char *packet, struct tracepoint *tpoint)
{
  char *buf;

  sprintf (packet, "T%x:%s:%c:%" PRIx64 ":%" PRIx64, tpoint->number,
	   paddress (tpoint->address),
	   (tpoint->enabled ? 'E' : 'D'), tpoint->step_count,
	   tpoint->pass_count);
  if (tpoint->type == fast_tracepoint)
    sprintf (packet + strlen (packet), ":F%x", tpoint->orig_size);
  else if (tpoint->type == static_tracepoint)
    sprintf (packet + strlen (packet), ":S");

  if (tpoint->cond)
    {
      buf = gdb_unparse_agent_expr (tpoint->cond);
      sprintf (packet + strlen (packet), ":X%x,%s",
	       tpoint->cond->length, buf);
      free (buf);
    }
}

/* Compose a response that is an imitation of the syntax by which the
   tracepoint action was originally downloaded (with the difference
   that due to the way we store the actions, this will output a packet
   per action, while GDB could have combined more than one action
   per-packet.  */

static void
response_action (char *packet, struct tracepoint *tpoint,
		 char *taction, int step)
{
  sprintf (packet, "%c%x:%s:%s",
	   (step ? 'S' : 'A'), tpoint->number, paddress (tpoint->address),
	   taction);
}

/* Compose a response that is an imitation of the syntax by which the
   tracepoint source piece was originally downloaded.  */

static void
response_source (char *packet,
		 struct tracepoint *tpoint, struct source_string *src)
{
  char *buf;
  int len;

  len = strlen (src->str);
  buf = (char *) alloca (len * 2 + 1);
  bin2hex ((gdb_byte *) src->str, buf, len);

  sprintf (packet, "Z%x:%s:%s:%x:%x:%s",
	   tpoint->number, paddress (tpoint->address),
	   src->type, 0, len, buf);
}

/* Return the first piece of tracepoint definition, and initialize the
   state machine that will iterate through all the tracepoint
   bits.  */

static void
cmd_qtfp (char *packet)
{
  trace_debug ("Returning first tracepoint definition piece");

  cur_tpoint = tracepoints;
  cur_action = cur_step_action = 0;
  cur_source_string = NULL;

  if (cur_tpoint)
    response_tracepoint (packet, cur_tpoint);
  else
    strcpy (packet, "l");
}

/* Return additional pieces of tracepoint definition.  Each action and
   stepping action must go into its own packet, because of packet size
   limits, and so we use state variables to deliver one piece at a
   time.  */

static void
cmd_qtsp (char *packet)
{
  trace_debug ("Returning subsequent tracepoint definition piece");

  if (!cur_tpoint)
    {
      /* This case would normally never occur, but be prepared for
	 GDB misbehavior.  */
      strcpy (packet, "l");
    }
  else if (cur_action < cur_tpoint->numactions)
    {
      response_action (packet, cur_tpoint,
		       cur_tpoint->actions_str[cur_action], 0);
      ++cur_action;
    }
  else if (cur_step_action < cur_tpoint->num_step_actions)
    {
      response_action (packet, cur_tpoint,
		       cur_tpoint->step_actions_str[cur_step_action], 1);
      ++cur_step_action;
    }
  else if ((cur_source_string
	    ? cur_source_string->next
	    : cur_tpoint->source_strings))
    {
      if (cur_source_string)
	cur_source_string = cur_source_string->next;
      else
	cur_source_string = cur_tpoint->source_strings;
      response_source (packet, cur_tpoint, cur_source_string);
    }
  else
    {
      cur_tpoint = cur_tpoint->next;
      cur_action = cur_step_action = 0;
      cur_source_string = NULL;
      if (cur_tpoint)
	response_tracepoint (packet, cur_tpoint);
      else
	strcpy (packet, "l");
    }
}

/* Compose a response that is an imitation of the syntax by which the
   trace state variable was originally downloaded.  */

static void
response_tsv (char *packet, struct trace_state_variable *tsv)
{
  char *buf = (char *) "";
  int namelen;

  if (tsv->name)
    {
      namelen = strlen (tsv->name);
      buf = (char *) alloca (namelen * 2 + 1);
      bin2hex ((gdb_byte *) tsv->name, buf, namelen);
    }

  sprintf (packet, "%x:%s:%x:%s", tsv->number, phex_nz (tsv->initial_value, 0),
	   tsv->getter ? 1 : 0, buf);
}

/* Return the first trace state variable definition, and initialize
   the state machine that will iterate through all the tsv bits.  */

static void
cmd_qtfv (char *packet)
{
  trace_debug ("Returning first trace state variable definition");

  cur_tsv = trace_state_variables;

  if (cur_tsv)
    response_tsv (packet, cur_tsv);
  else
    strcpy (packet, "l");
}

/* Return additional trace state variable definitions. */

static void
cmd_qtsv (char *packet)
{
  trace_debug ("Returning additional trace state variable definition");

  if (cur_tsv)
    {
      cur_tsv = cur_tsv->next;
      if (cur_tsv)
	response_tsv (packet, cur_tsv);
      else
	strcpy (packet, "l");
    }
  else
    strcpy (packet, "l");
}

/* Return the first static tracepoint marker, and initialize the state
   machine that will iterate through all the static tracepoints
   markers.  */

static void
cmd_qtfstm (char *packet)
{
  if (!maybe_write_ipa_ust_not_loaded (packet))
    run_inferior_command (packet, strlen (packet) + 1);
}

/* Return additional static tracepoints markers.  */

static void
cmd_qtsstm (char *packet)
{
  if (!maybe_write_ipa_ust_not_loaded (packet))
    run_inferior_command (packet, strlen (packet) + 1);
}

/* Return the definition of the static tracepoint at a given address.
   Result packet is the same as qTsST's.  */

static void
cmd_qtstmat (char *packet)
{
  if (!maybe_write_ipa_ust_not_loaded (packet))
    run_inferior_command (packet, strlen (packet) + 1);
}

/* Sent the agent a command to close it.  */

void
gdb_agent_about_to_close (int pid)
{
  char buf[IPA_CMD_BUF_SIZE];

  if (!maybe_write_ipa_not_loaded (buf))
    {
      scoped_restore_current_thread restore_thread;

      /* Find any thread which belongs to process PID.  */
      switch_to_thread (find_any_thread_of_pid (pid));

      strcpy (buf, "close");

      run_inferior_command (buf, strlen (buf) + 1);
    }
}

/* Return the minimum instruction size needed for fast tracepoints as a
   hexadecimal number.  */

static void
cmd_qtminftpilen (char *packet)
{
  if (current_thread == NULL)
    {
      /* Indicate that the minimum length is currently unknown.  */
      strcpy (packet, "0");
      return;
    }

  sprintf (packet, "%x", target_get_min_fast_tracepoint_insn_len ());
}

/* Respond to qTBuffer packet with a block of raw data from the trace
   buffer.  GDB may ask for a lot, but we are allowed to reply with
   only as much as will fit within packet limits or whatever.  */

static void
cmd_qtbuffer (char *own_buf)
{
  ULONGEST offset, num, tot;
  unsigned char *tbp;
  const char *packet = own_buf;

  packet += strlen ("qTBuffer:");

  packet = unpack_varlen_hex (packet, &offset);
  ++packet; /* skip a comma */
  unpack_varlen_hex (packet, &num);

  trace_debug ("Want to get trace buffer, %d bytes at offset 0x%s",
	       (int) num, phex_nz (offset, 0));

  tot = (trace_buffer_hi - trace_buffer_lo) - free_space ();

  /* If we're right at the end, reply specially that we're done.  */
  if (offset == tot)
    {
      strcpy (own_buf, "l");
      return;
    }

  /* Object to any other out-of-bounds request.  */
  if (offset > tot)
    {
      write_enn (own_buf);
      return;
    }

  /* Compute the pointer corresponding to the given offset, accounting
     for wraparound.  */
  tbp = trace_buffer_start + offset;
  if (tbp >= trace_buffer_wrap)
    tbp -= (trace_buffer_wrap - trace_buffer_lo);

  /* Trim to the remaining bytes if we're close to the end.  */
  if (num > tot - offset)
    num = tot - offset;

  /* Trim to available packet size.  */
  if (num >= (PBUFSIZ - 16) / 2 )
    num = (PBUFSIZ - 16) / 2;

  bin2hex (tbp, own_buf, num);
}

static void
cmd_bigqtbuffer_circular (char *own_buf)
{
  ULONGEST val;
  char *packet = own_buf;

  packet += strlen ("QTBuffer:circular:");

  unpack_varlen_hex (packet, &val);
  circular_trace_buffer = val;
  trace_debug ("Trace buffer is now %s",
	       circular_trace_buffer ? "circular" : "linear");
  write_ok (own_buf);
}

static void
cmd_bigqtbuffer_size (char *own_buf)
{
  ULONGEST val;
  LONGEST sval;
  char *packet = own_buf;

  /* Can't change the size during a tracing run.  */
  if (tracing)
    {
      write_enn (own_buf);
      return;
    }

  packet += strlen ("QTBuffer:size:");

  /* -1 is sent as literal "-1".  */
  if (strcmp (packet, "-1") == 0)
    sval = DEFAULT_TRACE_BUFFER_SIZE;
  else
    {
      unpack_varlen_hex (packet, &val);
      sval = (LONGEST) val;
    }

  init_trace_buffer (sval);
  trace_debug ("Trace buffer is now %s bytes",
	       plongest (trace_buffer_size));
  write_ok (own_buf);
}

static void
cmd_qtnotes (char *own_buf)
{
  size_t nbytes;
  char *saved, *user, *notes, *stopnote;
  char *packet = own_buf;

  packet += strlen ("QTNotes:");

  while (*packet)
    {
      if (startswith (packet, "user:"))
	{
	  packet += strlen ("user:");
	  saved = packet;
	  packet = strchr (packet, ';');
	  nbytes = (packet - saved) / 2;
	  user = (char *) xmalloc (nbytes + 1);
	  nbytes = hex2bin (saved, (gdb_byte *) user, nbytes);
	  user[nbytes] = '\0';
	  ++packet; /* skip the semicolon */
	  trace_debug ("User is '%s'", user);
	  xfree (tracing_user_name);
	  tracing_user_name = user;
	}
      else if (startswith (packet, "notes:"))
	{
	  packet += strlen ("notes:");
	  saved = packet;
	  packet = strchr (packet, ';');
	  nbytes = (packet - saved) / 2;
	  notes = (char *) xmalloc (nbytes + 1);
	  nbytes = hex2bin (saved, (gdb_byte *) notes, nbytes);
	  notes[nbytes] = '\0';
	  ++packet; /* skip the semicolon */
	  trace_debug ("Notes is '%s'", notes);
	  xfree (tracing_notes);
	  tracing_notes = notes;
	}
      else if (startswith (packet, "tstop:"))
	{
	  packet += strlen ("tstop:");
	  saved = packet;
	  packet = strchr (packet, ';');
	  nbytes = (packet - saved) / 2;
	  stopnote = (char *) xmalloc (nbytes + 1);
	  nbytes = hex2bin (saved, (gdb_byte *) stopnote, nbytes);
	  stopnote[nbytes] = '\0';
	  ++packet; /* skip the semicolon */
	  trace_debug ("tstop note is '%s'", stopnote);
	  xfree (tracing_stop_note);
	  tracing_stop_note = stopnote;
	}
      else
	break;
    }

  write_ok (own_buf);
}

int
handle_tracepoint_general_set (char *packet)
{
  if (strcmp ("QTinit", packet) == 0)
    {
      cmd_qtinit (packet);
      return 1;
    }
  else if (startswith (packet, "QTDP:"))
    {
      cmd_qtdp (packet);
      return 1;
    }
  else if (startswith (packet, "QTDPsrc:"))
    {
      cmd_qtdpsrc (packet);
      return 1;
    }
  else if (startswith (packet, "QTEnable:"))
    {
      cmd_qtenable_disable (packet, 1);
      return 1;
    }
  else if (startswith (packet, "QTDisable:"))
    {
      cmd_qtenable_disable (packet, 0);
      return 1;
    }
  else if (startswith (packet, "QTDV:"))
    {
      cmd_qtdv (packet);
      return 1;
    }
  else if (startswith (packet, "QTro:"))
    {
      cmd_qtro (packet);
      return 1;
    }
  else if (strcmp ("QTStart", packet) == 0)
    {
      cmd_qtstart (packet);
      return 1;
    }
  else if (strcmp ("QTStop", packet) == 0)
    {
      cmd_qtstop (packet);
      return 1;
    }
  else if (startswith (packet, "QTDisconnected:"))
    {
      cmd_qtdisconnected (packet);
      return 1;
    }
  else if (startswith (packet, "QTFrame:"))
    {
      cmd_qtframe (packet);
      return 1;
    }
  else if (startswith (packet, "QTBuffer:circular:"))
    {
      cmd_bigqtbuffer_circular (packet);
      return 1;
    }
  else if (startswith (packet, "QTBuffer:size:"))
    {
      cmd_bigqtbuffer_size (packet);
      return 1;
    }
  else if (startswith (packet, "QTNotes:"))
    {
      cmd_qtnotes (packet);
      return 1;
    }

  return 0;
}

int
handle_tracepoint_query (char *packet)
{
  if (strcmp ("qTStatus", packet) == 0)
    {
      cmd_qtstatus (packet);
      return 1;
    }
  else if (startswith (packet, "qTP:"))
    {
      cmd_qtp (packet);
      return 1;
    }
  else if (strcmp ("qTfP", packet) == 0)
    {
      cmd_qtfp (packet);
      return 1;
    }
  else if (strcmp ("qTsP", packet) == 0)
    {
      cmd_qtsp (packet);
      return 1;
    }
  else if (strcmp ("qTfV", packet) == 0)
    {
      cmd_qtfv (packet);
      return 1;
    }
  else if (strcmp ("qTsV", packet) == 0)
    {
      cmd_qtsv (packet);
      return 1;
    }
  else if (startswith (packet, "qTV:"))
    {
      cmd_qtv (packet);
      return 1;
    }
  else if (startswith (packet, "qTBuffer:"))
    {
      cmd_qtbuffer (packet);
      return 1;
    }
  else if (strcmp ("qTfSTM", packet) == 0)
    {
      cmd_qtfstm (packet);
      return 1;
    }
  else if (strcmp ("qTsSTM", packet) == 0)
    {
      cmd_qtsstm (packet);
      return 1;
    }
  else if (startswith (packet, "qTSTMat:"))
    {
      cmd_qtstmat (packet);
      return 1;
    }
  else if (strcmp ("qTMinFTPILen", packet) == 0)
    {
      cmd_qtminftpilen (packet);
      return 1;
    }

  return 0;
}

#endif
#ifndef IN_PROCESS_AGENT

/* Call this when thread TINFO has hit the tracepoint defined by
   TP_NUMBER and TP_ADDRESS, and that tracepoint has a while-stepping
   action.  This adds a while-stepping collecting state item to the
   threads' collecting state list, so that we can keep track of
   multiple simultaneous while-stepping actions being collected by the
   same thread.  This can happen in cases like:

    ff0001  INSN1 <-- TP1, while-stepping 10 collect $regs
    ff0002  INSN2
    ff0003  INSN3 <-- TP2, collect $regs
    ff0004  INSN4 <-- TP3, while-stepping 10 collect $regs
    ff0005  INSN5

   Notice that when instruction INSN5 is reached, the while-stepping
   actions of both TP1 and TP3 are still being collected, and that TP2
   had been collected meanwhile.  The whole range of ff0001-ff0005
   should be single-stepped, due to at least TP1's while-stepping
   action covering the whole range.  */

static void
add_while_stepping_state (struct thread_info *tinfo,
			  int tp_number, CORE_ADDR tp_address)
{
  struct wstep_state *wstep = XNEW (struct wstep_state);

  wstep->next = tinfo->while_stepping;

  wstep->tp_number = tp_number;
  wstep->tp_address = tp_address;
  wstep->current_step = 0;

  tinfo->while_stepping = wstep;
}

/* Release the while-stepping collecting state WSTEP.  */

static void
release_while_stepping_state (struct wstep_state *wstep)
{
  free (wstep);
}

/* Release all while-stepping collecting states currently associated
   with thread TINFO.  */

void
release_while_stepping_state_list (struct thread_info *tinfo)
{
  struct wstep_state *head;

  while (tinfo->while_stepping)
    {
      head = tinfo->while_stepping;
      tinfo->while_stepping = head->next;
      release_while_stepping_state (head);
    }
}

/* If TINFO was handling a 'while-stepping' action, the step has
   finished, so collect any step data needed, and check if any more
   steps are required.  Return true if the thread was indeed
   collecting tracepoint data, false otherwise.  */

int
tracepoint_finished_step (struct thread_info *tinfo, CORE_ADDR stop_pc)
{
  struct tracepoint *tpoint;
  struct wstep_state *wstep;
  struct wstep_state **wstep_link;
  struct trap_tracepoint_ctx ctx;

  /* Pull in fast tracepoint trace frames from the inferior lib buffer into
     our buffer.  */
  if (agent_loaded_p ())
    upload_fast_traceframes ();

  /* Check if we were indeed collecting data for one of more
     tracepoints with a 'while-stepping' count.  */
  if (tinfo->while_stepping == NULL)
    return 0;

  if (!tracing)
    {
      /* We're not even tracing anymore.  Stop this thread from
	 collecting.  */
      release_while_stepping_state_list (tinfo);

      /* The thread had stopped due to a single-step request indeed
	 explained by a tracepoint.  */
      return 1;
    }

  wstep = tinfo->while_stepping;
  wstep_link = &tinfo->while_stepping;

  trace_debug ("Thread %s finished a single-step for tracepoint %d at 0x%s",
	       target_pid_to_str (tinfo->id).c_str (),
	       wstep->tp_number, paddress (wstep->tp_address));

  ctx.base.type = trap_tracepoint;
  ctx.regcache = get_thread_regcache (tinfo, 1);

  while (wstep != NULL)
    {
      tpoint = find_tracepoint (wstep->tp_number, wstep->tp_address);
      if (tpoint == NULL)
	{
	  trace_debug ("NO TRACEPOINT %d at 0x%s FOR THREAD %s!",
		       wstep->tp_number, paddress (wstep->tp_address),
		       target_pid_to_str (tinfo->id).c_str ());

	  /* Unlink.  */
	  *wstep_link = wstep->next;
	  release_while_stepping_state (wstep);
	  wstep = *wstep_link;
	  continue;
	}

      /* We've just finished one step.  */
      ++wstep->current_step;

      /* Collect data.  */
      collect_data_at_step ((struct tracepoint_hit_ctx *) &ctx,
			    stop_pc, tpoint, wstep->current_step);

      if (wstep->current_step >= tpoint->step_count)
	{
	  /* The requested numbers of steps have occurred.  */
	  trace_debug ("Thread %s done stepping for tracepoint %d at 0x%s",
		       target_pid_to_str (tinfo->id).c_str (),
		       wstep->tp_number, paddress (wstep->tp_address));

	  /* Unlink the wstep.  */
	  *wstep_link = wstep->next;
	  release_while_stepping_state (wstep);
	  wstep = *wstep_link;

	  /* Only check the hit count now, which ensure that we do all
	     our stepping before stopping the run.  */
	  if (tpoint->pass_count > 0
	      && tpoint->hit_count >= tpoint->pass_count
	      && stopping_tracepoint == NULL)
	    stopping_tracepoint = tpoint;
	}
      else
	{
	  /* Keep single-stepping until the requested numbers of steps
	     have occurred.  */
	  wstep_link = &wstep->next;
	  wstep = *wstep_link;
	}

      if (stopping_tracepoint
	  || trace_buffer_is_full
	  || expr_eval_result != expr_eval_no_error)
	{
	  stop_tracing ();
	  break;
	}
    }

  return 1;
}

/* Handle any internal tracing control breakpoint hits.  That means,
   pull traceframes from the IPA to our buffer, and syncing both
   tracing agents when the IPA's tracing stops for some reason.  */

int
handle_tracepoint_bkpts (struct thread_info *tinfo, CORE_ADDR stop_pc)
{
  /* Pull in fast tracepoint trace frames from the inferior in-process
     agent's buffer into our buffer.  */

  if (!agent_loaded_p ())
    return 0;

  upload_fast_traceframes ();

  /* Check if the in-process agent had decided we should stop
     tracing.  */
  if (stop_pc == ipa_sym_addrs.addr_stop_tracing)
    {
      int ipa_trace_buffer_is_full;
      CORE_ADDR ipa_stopping_tracepoint;
      int ipa_expr_eval_result;
      CORE_ADDR ipa_error_tracepoint;

      trace_debug ("lib stopped at stop_tracing");

      read_inferior_integer (ipa_sym_addrs.addr_trace_buffer_is_full,
			     &ipa_trace_buffer_is_full);

      read_inferior_data_pointer (ipa_sym_addrs.addr_stopping_tracepoint,
				  &ipa_stopping_tracepoint);
      write_inferior_data_pointer (ipa_sym_addrs.addr_stopping_tracepoint, 0);

      read_inferior_data_pointer (ipa_sym_addrs.addr_error_tracepoint,
				  &ipa_error_tracepoint);
      write_inferior_data_pointer (ipa_sym_addrs.addr_error_tracepoint, 0);

      read_inferior_integer (ipa_sym_addrs.addr_expr_eval_result,
			     &ipa_expr_eval_result);
      write_inferior_integer (ipa_sym_addrs.addr_expr_eval_result, 0);

      trace_debug ("lib: trace_buffer_is_full: %d, "
		   "stopping_tracepoint: %s, "
		   "ipa_expr_eval_result: %d, "
		   "error_tracepoint: %s, ",
		   ipa_trace_buffer_is_full,
		   paddress (ipa_stopping_tracepoint),
		   ipa_expr_eval_result,
		   paddress (ipa_error_tracepoint));

      if (ipa_trace_buffer_is_full)
	trace_debug ("lib stopped due to full buffer.");

      if (ipa_stopping_tracepoint)
	trace_debug ("lib stopped due to tpoint");

      if (ipa_error_tracepoint)
	trace_debug ("lib stopped due to error");

      if (ipa_stopping_tracepoint != 0)
	{
	  stopping_tracepoint
	    = fast_tracepoint_from_ipa_tpoint_address (ipa_stopping_tracepoint);
	}
      else if (ipa_expr_eval_result != expr_eval_no_error)
	{
	  expr_eval_result = ipa_expr_eval_result;
	  error_tracepoint
	    = fast_tracepoint_from_ipa_tpoint_address (ipa_error_tracepoint);
	}
      stop_tracing ();
      return 1;
    }
  else if (stop_pc == ipa_sym_addrs.addr_flush_trace_buffer)
    {
      trace_debug ("lib stopped at flush_trace_buffer");
      return 1;
    }

  return 0;
}

/* Return true if TINFO just hit a tracepoint.  Collect data if
   so.  */

int
tracepoint_was_hit (struct thread_info *tinfo, CORE_ADDR stop_pc)
{
  struct tracepoint *tpoint;
  int ret = 0;
  struct trap_tracepoint_ctx ctx;

  /* Not tracing, don't handle.  */
  if (!tracing)
    return 0;

  ctx.base.type = trap_tracepoint;
  ctx.regcache = get_thread_regcache (tinfo, 1);

  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    {
      /* Note that we collect fast tracepoints here as well.  We'll
	 step over the fast tracepoint jump later, which avoids the
	 double collect.  However, we don't collect for static
	 tracepoints here, because UST markers are compiled in program,
	 and probes will be executed in program.  So static tracepoints
	 are collected there.   */
      if (tpoint->enabled && stop_pc == tpoint->address
	  && tpoint->type != static_tracepoint)
	{
	  trace_debug ("Thread %s at address of tracepoint %d at 0x%s",
		       target_pid_to_str (tinfo->id).c_str (),
		       tpoint->number, paddress (tpoint->address));

	  /* Test the condition if present, and collect if true.  */
	  if (!tpoint->cond
	      || (condition_true_at_tracepoint
		  ((struct tracepoint_hit_ctx *) &ctx, tpoint)))
	    collect_data_at_tracepoint ((struct tracepoint_hit_ctx *) &ctx,
					stop_pc, tpoint);

	  if (stopping_tracepoint
	      || trace_buffer_is_full
	      || expr_eval_result != expr_eval_no_error)
	    {
	      stop_tracing ();
	    }
	  /* If the tracepoint had a 'while-stepping' action, then set
	     the thread to collect this tracepoint on the following
	     single-steps.  */
	  else if (tpoint->step_count > 0)
	    {
	      add_while_stepping_state (tinfo,
					tpoint->number, tpoint->address);
	    }

	  ret = 1;
	}
    }

  return ret;
}

#endif

#if defined IN_PROCESS_AGENT && defined HAVE_UST
struct ust_marker_data;
static void collect_ust_data_at_tracepoint (struct tracepoint_hit_ctx *ctx,
					    struct traceframe *tframe);
#endif

/* Create a trace frame for the hit of the given tracepoint in the
   given thread.  */

static void
collect_data_at_tracepoint (struct tracepoint_hit_ctx *ctx, CORE_ADDR stop_pc,
			    struct tracepoint *tpoint)
{
  struct traceframe *tframe;
  int acti;

  /* Only count it as a hit when we actually collect data.  */
  tpoint->hit_count++;

  /* If we've exceeded a defined pass count, record the event for
     later, and finish the collection for this hit.  This test is only
     for nonstepping tracepoints, stepping tracepoints test at the end
     of their while-stepping loop.  */
  if (tpoint->pass_count > 0
      && tpoint->hit_count >= tpoint->pass_count
      && tpoint->step_count == 0
      && stopping_tracepoint == NULL)
    stopping_tracepoint = tpoint;

  trace_debug ("Making new traceframe for tracepoint %d at 0x%s, hit %" PRIu64,
	       tpoint->number, paddress (tpoint->address), tpoint->hit_count);

  tframe = add_traceframe (tpoint);

  if (tframe)
    {
      for (acti = 0; acti < tpoint->numactions; ++acti)
	{
#ifndef IN_PROCESS_AGENT
	  trace_debug ("Tracepoint %d at 0x%s about to do action '%s'",
		       tpoint->number, paddress (tpoint->address),
		       tpoint->actions_str[acti]);
#endif

	  do_action_at_tracepoint (ctx, stop_pc, tpoint, tframe,
				   tpoint->actions[acti]);
	}

      finish_traceframe (tframe);
    }

  if (tframe == NULL && tracing)
    trace_buffer_is_full = 1;
}

#ifndef IN_PROCESS_AGENT

static void
collect_data_at_step (struct tracepoint_hit_ctx *ctx,
		      CORE_ADDR stop_pc,
		      struct tracepoint *tpoint, int current_step)
{
  struct traceframe *tframe;
  int acti;

  trace_debug ("Making new step traceframe for "
	       "tracepoint %d at 0x%s, step %d of %" PRIu64 ", hit %" PRIu64,
	       tpoint->number, paddress (tpoint->address),
	       current_step, tpoint->step_count,
	       tpoint->hit_count);

  tframe = add_traceframe (tpoint);

  if (tframe)
    {
      for (acti = 0; acti < tpoint->num_step_actions; ++acti)
	{
	  trace_debug ("Tracepoint %d at 0x%s about to do step action '%s'",
		       tpoint->number, paddress (tpoint->address),
		       tpoint->step_actions_str[acti]);

	  do_action_at_tracepoint (ctx, stop_pc, tpoint, tframe,
				   tpoint->step_actions[acti]);
	}

      finish_traceframe (tframe);
    }

  if (tframe == NULL && tracing)
    trace_buffer_is_full = 1;
}

#endif

#ifdef IN_PROCESS_AGENT
/* The target description index for IPA.  Passed from gdbserver, used
   to select ipa_tdesc.  */
extern "C" {
IP_AGENT_EXPORT_VAR int ipa_tdesc_idx;
}
#endif

static struct regcache *
get_context_regcache (struct tracepoint_hit_ctx *ctx)
{
  struct regcache *regcache = NULL;
#ifdef IN_PROCESS_AGENT
  const struct target_desc *ipa_tdesc = get_ipa_tdesc (ipa_tdesc_idx);

  if (ctx->type == fast_tracepoint)
    {
      struct fast_tracepoint_ctx *fctx = (struct fast_tracepoint_ctx *) ctx;
      if (!fctx->regcache_initted)
	{
	  fctx->regcache_initted = 1;
	  init_register_cache (&fctx->regcache, ipa_tdesc, fctx->regspace);
	  supply_regblock (&fctx->regcache, NULL);
	  supply_fast_tracepoint_registers (&fctx->regcache, fctx->regs);
	}
      regcache = &fctx->regcache;
    }
#ifdef HAVE_UST
  if (ctx->type == static_tracepoint)
    {
      struct static_tracepoint_ctx *sctx
	= (struct static_tracepoint_ctx *) ctx;

      if (!sctx->regcache_initted)
	{
	  sctx->regcache_initted = 1;
	  init_register_cache (&sctx->regcache, ipa_tdesc, sctx->regspace);
	  supply_regblock (&sctx->regcache, NULL);
	  /* Pass down the tracepoint address, because REGS doesn't
	     include the PC, but we know what it must have been.  */
	  supply_static_tracepoint_registers (&sctx->regcache,
					      (const unsigned char *)
					      sctx->regs,
					      sctx->tpoint->address);
	}
      regcache = &sctx->regcache;
    }
#endif
#else
  if (ctx->type == trap_tracepoint)
    {
      struct trap_tracepoint_ctx *tctx = (struct trap_tracepoint_ctx *) ctx;
      regcache = tctx->regcache;
    }
#endif

  gdb_assert (regcache != NULL);

  return regcache;
}

static void
do_action_at_tracepoint (struct tracepoint_hit_ctx *ctx,
			 CORE_ADDR stop_pc,
			 struct tracepoint *tpoint,
			 struct traceframe *tframe,
			 struct tracepoint_action *taction)
{
  enum eval_result_type err;

  switch (taction->type)
    {
    case 'M':
      {
	struct collect_memory_action *maction;
	struct eval_agent_expr_context ax_ctx;

	maction = (struct collect_memory_action *) taction;
	ax_ctx.regcache = NULL;
	ax_ctx.tframe = tframe;
	ax_ctx.tpoint = tpoint;

	trace_debug ("Want to collect %s bytes at 0x%s (basereg %d)",
		     pulongest (maction->len),
		     paddress (maction->addr), maction->basereg);
	/* (should use basereg) */
	agent_mem_read (&ax_ctx, NULL, (CORE_ADDR) maction->addr,
			maction->len);
	break;
      }
    case 'R':
      {
	unsigned char *regspace;
	struct regcache tregcache;
	struct regcache *context_regcache;
	int regcache_size;

	trace_debug ("Want to collect registers");

	context_regcache = get_context_regcache (ctx);
	regcache_size = register_cache_size (context_regcache->tdesc);

	/* Collect all registers for now.  */
	regspace = add_traceframe_block (tframe, tpoint, 1 + regcache_size);
	if (regspace == NULL)
	  {
	    trace_debug ("Trace buffer block allocation failed, skipping");
	    break;
	  }
	/* Identify a register block.  */
	*regspace = 'R';

	/* Wrap the regblock in a register cache (in the stack, we
	   don't want to malloc here).  */
	init_register_cache (&tregcache, context_regcache->tdesc,
			     regspace + 1);

	/* Copy the register data to the regblock.  */
	regcache_cpy (&tregcache, context_regcache);

#ifndef IN_PROCESS_AGENT
	/* On some platforms, trap-based tracepoints will have the PC
	   pointing to the next instruction after the trap, but we
	   don't want the user or GDB trying to guess whether the
	   saved PC needs adjusting; so always record the adjusted
	   stop_pc.  Note that we can't use tpoint->address instead,
	   since it will be wrong for while-stepping actions.  This
	   adjustment is a nop for fast tracepoints collected from the
	   in-process lib (but not if GDBserver is collecting one
	   preemptively), since the PC had already been adjusted to
	   contain the tracepoint's address by the jump pad.  */
	trace_debug ("Storing stop pc (0x%s) in regblock",
		     paddress (stop_pc));

	/* This changes the regblock, not the thread's
	   regcache.  */
	regcache_write_pc (&tregcache, stop_pc);
#endif
      }
      break;
    case 'X':
      {
	struct eval_expr_action *eaction;
	struct eval_agent_expr_context ax_ctx;

	eaction = (struct eval_expr_action *) taction;
	ax_ctx.regcache = get_context_regcache (ctx);
	ax_ctx.tframe = tframe;
	ax_ctx.tpoint = tpoint;

	trace_debug ("Want to evaluate expression");

	err = gdb_eval_agent_expr (&ax_ctx, eaction->expr, NULL);

	if (err != expr_eval_no_error)
	  {
	    record_tracepoint_error (tpoint, "action expression", err);
	    return;
	  }
      }
      break;
    case 'L':
      {
#if defined IN_PROCESS_AGENT && defined HAVE_UST
	trace_debug ("Want to collect static trace data");
	collect_ust_data_at_tracepoint (ctx, tframe);
#else
	trace_debug ("warning: collecting static trace data, "
		     "but static tracepoints are not supported");
#endif
      }
      break;
    default:
      trace_debug ("unknown trace action '%c', ignoring", taction->type);
      break;
    }
}

static int
condition_true_at_tracepoint (struct tracepoint_hit_ctx *ctx,
			      struct tracepoint *tpoint)
{
  ULONGEST value = 0;
  enum eval_result_type err;

  /* Presently, gdbserver doesn't run compiled conditions, only the
     IPA does.  If the program stops at a fast tracepoint's address
     (e.g., due to a breakpoint, trap tracepoint, or stepping),
     gdbserver preemptively collect the fast tracepoint.  Later, on
     resume, gdbserver steps over the fast tracepoint like it steps
     over breakpoints, so that the IPA doesn't see that fast
     tracepoint.  This avoids double collects of fast tracepoints in
     that stopping scenario.  Having gdbserver itself handle the fast
     tracepoint gives the user a consistent view of when fast or trap
     tracepoints are collected, compared to an alternative where only
     trap tracepoints are collected on stop, and fast tracepoints on
     resume.  When a fast tracepoint is being processed by gdbserver,
     it is always the non-compiled condition expression that is
     used.  */
#ifdef IN_PROCESS_AGENT
  if (tpoint->compiled_cond)
    {
      struct fast_tracepoint_ctx *fctx = (struct fast_tracepoint_ctx *) ctx;
      err = ((condfn) (uintptr_t) (tpoint->compiled_cond)) (fctx->regs, &value);
    }
  else
#endif
    {
      struct eval_agent_expr_context ax_ctx;

      ax_ctx.regcache = get_context_regcache (ctx);
      ax_ctx.tframe = NULL;
      ax_ctx.tpoint = tpoint;

      err = gdb_eval_agent_expr (&ax_ctx, tpoint->cond, &value);
    }
  if (err != expr_eval_no_error)
    {
      record_tracepoint_error (tpoint, "condition", err);
      /* The error case must return false.  */
      return 0;
    }

  trace_debug ("Tracepoint %d at 0x%s condition evals to %s",
	       tpoint->number, paddress (tpoint->address),
	       pulongest (value));
  return (value ? 1 : 0);
}

/* See tracepoint.h.  */

int
agent_mem_read (struct eval_agent_expr_context *ctx,
		unsigned char *to, CORE_ADDR from, ULONGEST len)
{
  unsigned char *mspace;
  ULONGEST remaining = len;
  unsigned short blocklen;

  /* If a 'to' buffer is specified, use it.  */
  if (to != NULL)
    return read_inferior_memory (from, to, len);

  /* Otherwise, create a new memory block in the trace buffer.  */
  while (remaining > 0)
    {
      size_t sp;

      blocklen = (remaining > 65535 ? 65535 : remaining);
      sp = 1 + sizeof (from) + sizeof (blocklen) + blocklen;
      mspace = add_traceframe_block (ctx->tframe, ctx->tpoint, sp);
      if (mspace == NULL)
	return 1;
      /* Identify block as a memory block.  */
      *mspace = 'M';
      ++mspace;
      /* Record address and size.  */
      memcpy (mspace, &from, sizeof (from));
      mspace += sizeof (from);
      memcpy (mspace, &blocklen, sizeof (blocklen));
      mspace += sizeof (blocklen);
      /* Record the memory block proper.  */
      if (read_inferior_memory (from, mspace, blocklen) != 0)
	return 1;
      trace_debug ("%d bytes recorded", blocklen);
      remaining -= blocklen;
      from += blocklen;
    }
  return 0;
}

int
agent_mem_read_string (struct eval_agent_expr_context *ctx,
		       unsigned char *to, CORE_ADDR from, ULONGEST len)
{
  unsigned char *buf, *mspace;
  ULONGEST remaining = len;
  unsigned short blocklen, i;

  /* To save a bit of space, block lengths are 16-bit, so break large
     requests into multiple blocks.  Bordering on overkill for strings,
     but it could happen that someone specifies a large max length.  */
  while (remaining > 0)
    {
      size_t sp;

      blocklen = (remaining > 65535 ? 65535 : remaining);
      /* We want working space to accumulate nonzero bytes, since
	 traceframes must have a predecided size (otherwise it gets
	 harder to wrap correctly for the circular case, etc).  */
      buf = (unsigned char *) xmalloc (blocklen + 1);
      for (i = 0; i < blocklen; ++i)
	{
	  /* Read the string one byte at a time, in case the string is
	     at the end of a valid memory area - we don't want a
	     correctly-terminated string to engender segvio
	     complaints.  */
	  read_inferior_memory (from + i, buf + i, 1);

	  if (buf[i] == '\0')
	    {
	      blocklen = i + 1;
	      /* Make sure outer loop stops now too.  */
	      remaining = blocklen;
	      break;
	    }
	}
      sp = 1 + sizeof (from) + sizeof (blocklen) + blocklen;
      mspace = add_traceframe_block (ctx->tframe, ctx->tpoint, sp);
      if (mspace == NULL)
	{
	  xfree (buf);
	  return 1;
	}
      /* Identify block as a memory block.  */
      *mspace = 'M';
      ++mspace;
      /* Record address and size.  */
      memcpy ((void *) mspace, (void *) &from, sizeof (from));
      mspace += sizeof (from);
      memcpy ((void *) mspace, (void *) &blocklen, sizeof (blocklen));
      mspace += sizeof (blocklen);
      /* Copy the string contents.  */
      memcpy ((void *) mspace, (void *) buf, blocklen);
      remaining -= blocklen;
      from += blocklen;
      xfree (buf);
    }
  return 0;
}

/* Record the value of a trace state variable.  */

int
agent_tsv_read (struct eval_agent_expr_context *ctx, int n)
{
  unsigned char *vspace;
  LONGEST val;

  vspace = add_traceframe_block (ctx->tframe, ctx->tpoint,
				 1 + sizeof (n) + sizeof (LONGEST));
  if (vspace == NULL)
    return 1;
  /* Identify block as a variable.  */
  *vspace = 'V';
  /* Record variable's number and value.  */
  memcpy (vspace + 1, &n, sizeof (n));
  val = get_trace_state_variable_value (n);
  memcpy (vspace + 1 + sizeof (n), &val, sizeof (val));
  trace_debug ("Variable %d recorded", n);
  return 0;
}

#ifndef IN_PROCESS_AGENT

/* Callback for traceframe_walk_blocks, used to find a given block
   type in a traceframe.  */

static int
match_blocktype (char blocktype, unsigned char *dataptr, void *data)
{
  char *wantedp = (char *) data;

  if (*wantedp == blocktype)
    return 1;

  return 0;
}

/* Walk over all traceframe blocks of the traceframe buffer starting
   at DATABASE, of DATASIZE bytes long, and call CALLBACK for each
   block found, passing in DATA unmodified.  If CALLBACK returns true,
   this returns a pointer to where the block is found.  Returns NULL
   if no callback call returned true, indicating that all blocks have
   been walked.  */

static unsigned char *
traceframe_walk_blocks (unsigned char *database, unsigned int datasize,
			int tfnum,
			int (*callback) (char blocktype,
					 unsigned char *dataptr,
					 void *data),
			void *data)
{
  unsigned char *dataptr;

  if (datasize == 0)
    {
      trace_debug ("traceframe %d has no data", tfnum);
      return NULL;
    }

  /* Iterate through a traceframe's blocks, looking for a block of the
     requested type.  */
  for (dataptr = database;
       dataptr < database + datasize;
       /* nothing */)
    {
      char blocktype;
      unsigned short mlen;

      if (dataptr == trace_buffer_wrap)
	{
	  /* Adjust to reflect wrapping part of the frame around to
	     the beginning.  */
	  datasize = dataptr - database;
	  dataptr = database = trace_buffer_lo;
	}

      blocktype = *dataptr++;

      if ((*callback) (blocktype, dataptr, data))
	return dataptr;

      switch (blocktype)
	{
	case 'R':
	  /* Skip over the registers block.  */
	  dataptr += current_target_desc ()->registers_size;
	  break;
	case 'M':
	  /* Skip over the memory block.  */
	  dataptr += sizeof (CORE_ADDR);
	  memcpy (&mlen, dataptr, sizeof (mlen));
	  dataptr += (sizeof (mlen) + mlen);
	  break;
	case 'V':
	  /* Skip over the TSV block.  */
	  dataptr += (sizeof (int) + sizeof (LONGEST));
	  break;
	case 'S':
	  /* Skip over the static trace data block.  */
	  memcpy (&mlen, dataptr, sizeof (mlen));
	  dataptr += (sizeof (mlen) + mlen);
	  break;
	default:
	  trace_debug ("traceframe %d has unknown block type 0x%x",
		       tfnum, blocktype);
	  return NULL;
	}
    }

  return NULL;
}

/* Look for the block of type TYPE_WANTED in the traceframe starting
   at DATABASE of DATASIZE bytes long.  TFNUM is the traceframe
   number.  */

static unsigned char *
traceframe_find_block_type (unsigned char *database, unsigned int datasize,
			    int tfnum, char type_wanted)
{
  return traceframe_walk_blocks (database, datasize, tfnum,
				 match_blocktype, &type_wanted);
}

static unsigned char *
traceframe_find_regblock (struct traceframe *tframe, int tfnum)
{
  unsigned char *regblock;

  regblock = traceframe_find_block_type (tframe->data,
					 tframe->data_size,
					 tfnum, 'R');

  if (regblock == NULL)
    trace_debug ("traceframe %d has no register data", tfnum);

  return regblock;
}

/* Get registers from a traceframe.  */

int
fetch_traceframe_registers (int tfnum, struct regcache *regcache, int regnum)
{
  unsigned char *dataptr;
  struct tracepoint *tpoint;
  struct traceframe *tframe;

  tframe = find_traceframe (tfnum);

  if (tframe == NULL)
    {
      trace_debug ("traceframe %d not found", tfnum);
      return 1;
    }

  dataptr = traceframe_find_regblock (tframe, tfnum);
  if (dataptr == NULL)
    {
      /* Mark registers unavailable.  */
      supply_regblock (regcache, NULL);

      /* We can generally guess at a PC, although this will be
	 misleading for while-stepping frames and multi-location
	 tracepoints.  */
      tpoint = find_next_tracepoint_by_number (NULL, tframe->tpnum);
      if (tpoint != NULL)
	regcache_write_pc (regcache, tpoint->address);
    }
  else
    supply_regblock (regcache, dataptr);

  return 0;
}

static CORE_ADDR
traceframe_get_pc (struct traceframe *tframe)
{
  struct regcache regcache;
  unsigned char *dataptr;
  const struct target_desc *tdesc = current_target_desc ();

  dataptr = traceframe_find_regblock (tframe, -1);
  if (dataptr == NULL)
    return 0;

  init_register_cache (&regcache, tdesc, dataptr);
  return regcache_read_pc (&regcache);
}

/* Read a requested block of memory from a trace frame.  */

int
traceframe_read_mem (int tfnum, CORE_ADDR addr,
		     unsigned char *buf, ULONGEST length,
		     ULONGEST *nbytes)
{
  struct traceframe *tframe;
  unsigned char *database, *dataptr;
  unsigned int datasize;
  CORE_ADDR maddr;
  unsigned short mlen;

  trace_debug ("traceframe_read_mem");

  tframe = find_traceframe (tfnum);

  if (!tframe)
    {
      trace_debug ("traceframe %d not found", tfnum);
      return 1;
    }

  datasize = tframe->data_size;
  database = dataptr = &tframe->data[0];

  /* Iterate through a traceframe's blocks, looking for memory.  */
  while ((dataptr = traceframe_find_block_type (dataptr,
						datasize
						- (dataptr - database),
						tfnum, 'M')) != NULL)
    {
      memcpy (&maddr, dataptr, sizeof (maddr));
      dataptr += sizeof (maddr);
      memcpy (&mlen, dataptr, sizeof (mlen));
      dataptr += sizeof (mlen);
      trace_debug ("traceframe %d has %d bytes at %s",
		   tfnum, mlen, paddress (maddr));

      /* If the block includes the first part of the desired range,
	 return as much it has; GDB will re-request the remainder,
	 which might be in a different block of this trace frame.  */
      if (maddr <= addr && addr < (maddr + mlen))
	{
	  ULONGEST amt = (maddr + mlen) - addr;
	  if (amt > length)
	    amt = length;

	  memcpy (buf, dataptr + (addr - maddr), amt);
	  *nbytes = amt;
	  return 0;
	}

      /* Skip over this block.  */
      dataptr += mlen;
    }

  trace_debug ("traceframe %d has no memory data for the desired region",
	       tfnum);

  *nbytes = 0;
  return 0;
}

static int
traceframe_read_tsv (int tsvnum, LONGEST *val)
{
  client_state &cs = get_client_state ();
  int tfnum;
  struct traceframe *tframe;
  unsigned char *database, *dataptr;
  unsigned int datasize;
  int vnum;
  int found = 0;

  trace_debug ("traceframe_read_tsv");

  tfnum = cs.current_traceframe;

  if (tfnum < 0)
    {
      trace_debug ("no current traceframe");
      return 1;
    }

  tframe = find_traceframe (tfnum);

  if (tframe == NULL)
    {
      trace_debug ("traceframe %d not found", tfnum);
      return 1;
    }

  datasize = tframe->data_size;
  database = dataptr = &tframe->data[0];

  /* Iterate through a traceframe's blocks, looking for the last
     matched tsv.  */
  while ((dataptr = traceframe_find_block_type (dataptr,
						datasize
						- (dataptr - database),
						tfnum, 'V')) != NULL)
    {
      memcpy (&vnum, dataptr, sizeof (vnum));
      dataptr += sizeof (vnum);

      trace_debug ("traceframe %d has variable %d", tfnum, vnum);

      /* Check that this is the variable we want.  */
      if (tsvnum == vnum)
	{
	  memcpy (val, dataptr, sizeof (*val));
	  found = 1;
	}

      /* Skip over this block.  */
      dataptr += sizeof (LONGEST);
    }

  if (!found)
    trace_debug ("traceframe %d has no data for variable %d",
		 tfnum, tsvnum);
  return !found;
}

/* Read a requested block of static tracepoint data from a trace
   frame.  */

int
traceframe_read_sdata (int tfnum, ULONGEST offset,
		       unsigned char *buf, ULONGEST length,
		       ULONGEST *nbytes)
{
  struct traceframe *tframe;
  unsigned char *database, *dataptr;
  unsigned int datasize;
  unsigned short mlen;

  trace_debug ("traceframe_read_sdata");

  tframe = find_traceframe (tfnum);

  if (!tframe)
    {
      trace_debug ("traceframe %d not found", tfnum);
      return 1;
    }

  datasize = tframe->data_size;
  database = &tframe->data[0];

  /* Iterate through a traceframe's blocks, looking for static
     tracepoint data.  */
  dataptr = traceframe_find_block_type (database, datasize,
					tfnum, 'S');
  if (dataptr != NULL)
    {
      memcpy (&mlen, dataptr, sizeof (mlen));
      dataptr += sizeof (mlen);
      if (offset < mlen)
	{
	  if (offset + length > mlen)
	    length = mlen - offset;

	  memcpy (buf, dataptr, length);
	  *nbytes = length;
	}
      else
	*nbytes = 0;
      return 0;
    }

  trace_debug ("traceframe %d has no static trace data", tfnum);

  *nbytes = 0;
  return 0;
}

/* Callback for traceframe_walk_blocks.  Builds a traceframe-info
   object.  DATA is pointer to a string holding the traceframe-info
   object being built.  */

static int
build_traceframe_info_xml (char blocktype, unsigned char *dataptr, void *data)
{
  std::string *buffer = (std::string *) data;

  switch (blocktype)
    {
    case 'M':
      {
	unsigned short mlen;
	CORE_ADDR maddr;

	memcpy (&maddr, dataptr, sizeof (maddr));
	dataptr += sizeof (maddr);
	memcpy (&mlen, dataptr, sizeof (mlen));
	dataptr += sizeof (mlen);
	string_xml_appendf (*buffer,
			    "<memory start=\"0x%s\" length=\"0x%s\"/>\n",
			    paddress (maddr), phex_nz (mlen, sizeof (mlen)));
	break;
      }
    case 'V':
      {
	int vnum;

	memcpy (&vnum, dataptr, sizeof (vnum));
	string_xml_appendf (*buffer, "<tvar id=\"%d\"/>\n", vnum);
	break;
      }
    case 'R':
    case 'S':
      {
	break;
      }
    default:
      warning ("Unhandled trace block type (%d) '%c ' "
	       "while building trace frame info.",
	       blocktype, blocktype);
      break;
    }

  return 0;
}

/* Build a traceframe-info object for traceframe number TFNUM into
   BUFFER.  */

int
traceframe_read_info (int tfnum, std::string *buffer)
{
  struct traceframe *tframe;

  trace_debug ("traceframe_read_info");

  tframe = find_traceframe (tfnum);

  if (!tframe)
    {
      trace_debug ("traceframe %d not found", tfnum);
      return 1;
    }

  *buffer += "<traceframe-info>\n";
  traceframe_walk_blocks (tframe->data, tframe->data_size,
			  tfnum, build_traceframe_info_xml, buffer);
  *buffer += "</traceframe-info>\n";
  return 0;
}

/* Return the first fast tracepoint whose jump pad contains PC.  */

static struct tracepoint *
fast_tracepoint_from_jump_pad_address (CORE_ADDR pc)
{
  struct tracepoint *tpoint;

  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    if (tpoint->type == fast_tracepoint)
      if (tpoint->jump_pad <= pc && pc < tpoint->jump_pad_end)
	return tpoint;

  return NULL;
}

/* Return the first fast tracepoint whose trampoline contains PC.  */

static struct tracepoint *
fast_tracepoint_from_trampoline_address (CORE_ADDR pc)
{
  struct tracepoint *tpoint;

  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    {
      if (tpoint->type == fast_tracepoint
	  && tpoint->trampoline <= pc && pc < tpoint->trampoline_end)
	return tpoint;
    }

  return NULL;
}

/* Return GDBserver's tracepoint that matches the IP Agent's
   tracepoint object that lives at IPA_TPOINT_OBJ in the IP Agent's
   address space.  */

static struct tracepoint *
fast_tracepoint_from_ipa_tpoint_address (CORE_ADDR ipa_tpoint_obj)
{
  struct tracepoint *tpoint;

  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    if (tpoint->type == fast_tracepoint)
      if (tpoint->obj_addr_on_target == ipa_tpoint_obj)
	return tpoint;

  return NULL;
}

#endif

/* The type of the object that is used to synchronize fast tracepoint
   collection.  */

typedef struct collecting_t
{
  /* The fast tracepoint number currently collecting.  */
  uintptr_t tpoint;

  /* A number that GDBserver can use to identify the thread that is
     presently holding the collect lock.  This need not (and usually
     is not) the thread id, as getting the current thread ID usually
     requires a system call, which we want to avoid like the plague.
     Usually this is thread's TCB, found in the TLS (pseudo-)
     register, which is readable with a single insn on several
     architectures.  */
  uintptr_t thread_area;
} collecting_t;

#ifndef IN_PROCESS_AGENT

void
force_unlock_trace_buffer (void)
{
  write_inferior_data_pointer (ipa_sym_addrs.addr_collecting, 0);
}

/* Check if the thread identified by THREAD_AREA which is stopped at
   STOP_PC, is presently locking the fast tracepoint collection, and
   if so, gather some status of said collection.  Returns 0 if the
   thread isn't collecting or in the jump pad at all.  1, if in the
   jump pad (or within gdb_collect) and hasn't executed the adjusted
   original insn yet (can set a breakpoint there and run to it).  2,
   if presently executing the adjusted original insn --- in which
   case, if we want to move the thread out of the jump pad, we need to
   single-step it until this function returns 0.  */

fast_tpoint_collect_result
fast_tracepoint_collecting (CORE_ADDR thread_area,
			    CORE_ADDR stop_pc,
			    struct fast_tpoint_collect_status *status)
{
  CORE_ADDR ipa_collecting;
  CORE_ADDR ipa_gdb_jump_pad_buffer, ipa_gdb_jump_pad_buffer_end;
  CORE_ADDR ipa_gdb_trampoline_buffer;
  CORE_ADDR ipa_gdb_trampoline_buffer_end;
  struct tracepoint *tpoint;
  int needs_breakpoint;

  /* The thread THREAD_AREA is either:

      0. not collecting at all, not within the jump pad, or within
	 gdb_collect or one of its callees.

      1. in the jump pad and haven't reached gdb_collect

      2. within gdb_collect (out of the jump pad) (collect is set)

      3. we're in the jump pad, after gdb_collect having returned,
	 possibly executing the adjusted insns.

      For cases 1 and 3, `collecting' may or not be set.  The jump pad
      doesn't have any complicated jump logic, so we can tell if the
      thread is executing the adjust original insn or not by just
      matching STOP_PC with known jump pad addresses.  If we it isn't
      yet executing the original insn, set a breakpoint there, and let
      the thread run to it, so to quickly step over a possible (many
      insns) gdb_collect call.  Otherwise, or when the breakpoint is
      hit, only a few (small number of) insns are left to be executed
      in the jump pad.  Single-step the thread until it leaves the
      jump pad.  */

 again:
  tpoint = NULL;
  needs_breakpoint = 0;
  trace_debug ("fast_tracepoint_collecting");

  if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_jump_pad_buffer,
				  &ipa_gdb_jump_pad_buffer))
    {
      internal_error ("error extracting `gdb_jump_pad_buffer'");
    }
  if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_jump_pad_buffer_end,
				  &ipa_gdb_jump_pad_buffer_end))
    {
      internal_error ("error extracting `gdb_jump_pad_buffer_end'");
    }

  if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_trampoline_buffer,
				  &ipa_gdb_trampoline_buffer))
    {
      internal_error ("error extracting `gdb_trampoline_buffer'");
    }
  if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_trampoline_buffer_end,
				  &ipa_gdb_trampoline_buffer_end))
    {
      internal_error ("error extracting `gdb_trampoline_buffer_end'");
    }

  if (ipa_gdb_jump_pad_buffer <= stop_pc
      && stop_pc < ipa_gdb_jump_pad_buffer_end)
    {
      /* We can tell which tracepoint(s) the thread is collecting by
	 matching the jump pad address back to the tracepoint.  */
      tpoint = fast_tracepoint_from_jump_pad_address (stop_pc);
      if (tpoint == NULL)
	{
	  warning ("in jump pad, but no matching tpoint?");
	  return fast_tpoint_collect_result::not_collecting;
	}
      else
	{
	  trace_debug ("in jump pad of tpoint (%d, %s); jump_pad(%s, %s); "
		       "adj_insn(%s, %s)",
		       tpoint->number, paddress (tpoint->address),
		       paddress (tpoint->jump_pad),
		       paddress (tpoint->jump_pad_end),
		       paddress (tpoint->adjusted_insn_addr),
		       paddress (tpoint->adjusted_insn_addr_end));
	}

      /* Definitely in the jump pad.  May or may not need
	 fast-exit-jump-pad breakpoint.  */
      if (tpoint->jump_pad <= stop_pc
	  && stop_pc < tpoint->adjusted_insn_addr)
	needs_breakpoint =  1;
    }
  else if (ipa_gdb_trampoline_buffer <= stop_pc
	   && stop_pc < ipa_gdb_trampoline_buffer_end)
    {
      /* We can tell which tracepoint(s) the thread is collecting by
	 matching the trampoline address back to the tracepoint.  */
      tpoint = fast_tracepoint_from_trampoline_address (stop_pc);
      if (tpoint == NULL)
	{
	  warning ("in trampoline, but no matching tpoint?");
	  return fast_tpoint_collect_result::not_collecting;
	}
      else
	{
	  trace_debug ("in trampoline of tpoint (%d, %s); trampoline(%s, %s)",
		       tpoint->number, paddress (tpoint->address),
		       paddress (tpoint->trampoline),
		       paddress (tpoint->trampoline_end));
	}

      /* Have not reached jump pad yet, but treat the trampoline as a
	 part of the jump pad that is before the adjusted original
	 instruction.  */
      needs_breakpoint = 1;
    }
  else
    {
      collecting_t ipa_collecting_obj;

      /* If `collecting' is set/locked, then the THREAD_AREA thread
	 may or not be the one holding the lock.  We have to read the
	 lock to find out.  */

      if (read_inferior_data_pointer (ipa_sym_addrs.addr_collecting,
				      &ipa_collecting))
	{
	  trace_debug ("fast_tracepoint_collecting:"
		       " failed reading 'collecting' in the inferior");
	  return fast_tpoint_collect_result::not_collecting;
	}

      if (!ipa_collecting)
	{
	  trace_debug ("fast_tracepoint_collecting: not collecting"
		       " (and nobody is).");
	  return fast_tpoint_collect_result::not_collecting;
	}

      /* Some thread is collecting.  Check which.  */
      if (read_inferior_memory (ipa_collecting,
				(unsigned char *) &ipa_collecting_obj,
				sizeof (ipa_collecting_obj)) != 0)
	goto again;

      if (ipa_collecting_obj.thread_area != thread_area)
	{
	  trace_debug ("fast_tracepoint_collecting: not collecting "
		       "(another thread is)");
	  return fast_tpoint_collect_result::not_collecting;
	}

      tpoint
	= fast_tracepoint_from_ipa_tpoint_address (ipa_collecting_obj.tpoint);
      if (tpoint == NULL)
	{
	  warning ("fast_tracepoint_collecting: collecting, "
		   "but tpoint %s not found?",
		   paddress ((CORE_ADDR) ipa_collecting_obj.tpoint));
	  return fast_tpoint_collect_result::not_collecting;
	}

      /* The thread is within `gdb_collect', skip over the rest of
	 fast tracepoint collection quickly using a breakpoint.  */
      needs_breakpoint = 1;
    }

  /* The caller wants a bit of status detail.  */
  if (status != NULL)
    {
      status->tpoint_num = tpoint->number;
      status->tpoint_addr = tpoint->address;
      status->adjusted_insn_addr = tpoint->adjusted_insn_addr;
      status->adjusted_insn_addr_end = tpoint->adjusted_insn_addr_end;
    }

  if (needs_breakpoint)
    {
      /* Hasn't executed the original instruction yet.  Set breakpoint
	 there, and wait till it's hit, then single-step until exiting
	 the jump pad.  */

      trace_debug ("\
fast_tracepoint_collecting, returning continue-until-break at %s",
		   paddress (tpoint->adjusted_insn_addr));

      return fast_tpoint_collect_result::before_insn; /* continue */
    }
  else
    {
      /* Just single-step until exiting the jump pad.  */

      trace_debug ("fast_tracepoint_collecting, returning "
		   "need-single-step (%s-%s)",
		   paddress (tpoint->adjusted_insn_addr),
		   paddress (tpoint->adjusted_insn_addr_end));

      return fast_tpoint_collect_result::at_insn; /* single-step */
    }
}

#endif

#ifdef IN_PROCESS_AGENT

/* The global fast tracepoint collect lock.  Points to a collecting_t
   object built on the stack by the jump pad, if presently locked;
   NULL if it isn't locked.  Note that this lock *must* be set while
   executing any *function other than the jump pad.  See
   fast_tracepoint_collecting.  */
extern "C" {
IP_AGENT_EXPORT_VAR collecting_t *collecting;
}

/* This is needed for -Wmissing-declarations.  */
IP_AGENT_EXPORT_FUNC void gdb_collect (struct tracepoint *tpoint,
				       unsigned char *regs);

/* This routine, called from the jump pad (in asm) is designed to be
   called from the jump pads of fast tracepoints, thus it is on the
   critical path.  */

IP_AGENT_EXPORT_FUNC void
gdb_collect (struct tracepoint *tpoint, unsigned char *regs)
{
  struct fast_tracepoint_ctx ctx;
  const struct target_desc *ipa_tdesc;

  /* Don't do anything until the trace run is completely set up.  */
  if (!tracing)
    return;

  ipa_tdesc = get_ipa_tdesc (ipa_tdesc_idx);
  ctx.base.type = fast_tracepoint;
  ctx.regs = regs;
  ctx.regcache_initted = 0;
  /* Wrap the regblock in a register cache (in the stack, we don't
     want to malloc here).  */
  ctx.regspace = (unsigned char *) alloca (ipa_tdesc->registers_size);
  if (ctx.regspace == NULL)
    {
      trace_debug ("Trace buffer block allocation failed, skipping");
      return;
    }

  for (ctx.tpoint = tpoint;
       ctx.tpoint != NULL && ctx.tpoint->address == tpoint->address;
       ctx.tpoint = ctx.tpoint->next)
    {
      if (!ctx.tpoint->enabled)
	continue;

      /* Multiple tracepoints of different types, such as fast tracepoint and
	 static tracepoint, can be set at the same address.  */
      if (ctx.tpoint->type != tpoint->type)
	continue;

      /* Test the condition if present, and collect if true.  */
      if (ctx.tpoint->cond == NULL
	  || condition_true_at_tracepoint ((struct tracepoint_hit_ctx *) &ctx,
					   ctx.tpoint))
	{
	  collect_data_at_tracepoint ((struct tracepoint_hit_ctx *) &ctx,
				      ctx.tpoint->address, ctx.tpoint);

	  /* Note that this will cause original insns to be written back
	     to where we jumped from, but that's OK because we're jumping
	     back to the next whole instruction.  This will go badly if
	     instruction restoration is not atomic though.  */
	  if (stopping_tracepoint
	      || trace_buffer_is_full
	      || expr_eval_result != expr_eval_no_error)
	    {
	      stop_tracing ();
	      break;
	    }
	}
      else
	{
	  /* If there was a condition and it evaluated to false, the only
	     way we would stop tracing is if there was an error during
	     condition expression evaluation.  */
	  if (expr_eval_result != expr_eval_no_error)
	    {
	      stop_tracing ();
	      break;
	    }
	}
    }
}

/* These global variables points to the corresponding functions.  This is
   necessary on powerpc64, where asking for function symbol address from gdb
   results in returning the actual code pointer, instead of the descriptor
   pointer.  */

typedef void (*gdb_collect_ptr_type) (struct tracepoint *, unsigned char *);
typedef ULONGEST (*get_raw_reg_ptr_type) (const unsigned char *, int);
typedef LONGEST (*get_trace_state_variable_value_ptr_type) (int);
typedef void (*set_trace_state_variable_value_ptr_type) (int, LONGEST);

extern "C" {
IP_AGENT_EXPORT_VAR gdb_collect_ptr_type gdb_collect_ptr = gdb_collect;
IP_AGENT_EXPORT_VAR get_raw_reg_ptr_type get_raw_reg_ptr = get_raw_reg;
IP_AGENT_EXPORT_VAR get_trace_state_variable_value_ptr_type
  get_trace_state_variable_value_ptr = get_trace_state_variable_value;
IP_AGENT_EXPORT_VAR set_trace_state_variable_value_ptr_type
  set_trace_state_variable_value_ptr = set_trace_state_variable_value;
}

#endif

#ifndef IN_PROCESS_AGENT

CORE_ADDR
get_raw_reg_func_addr (void)
{
  CORE_ADDR res;
  if (read_inferior_data_pointer (ipa_sym_addrs.addr_get_raw_reg_ptr, &res))
    {
      error ("error extracting get_raw_reg_ptr");
      return 0;
    }
  return res;
}

CORE_ADDR
get_get_tsv_func_addr (void)
{
  CORE_ADDR res;
  if (read_inferior_data_pointer (
	ipa_sym_addrs.addr_get_trace_state_variable_value_ptr, &res))
    {
      error ("error extracting get_trace_state_variable_value_ptr");
      return 0;
    }
  return res;
}

CORE_ADDR
get_set_tsv_func_addr (void)
{
  CORE_ADDR res;
  if (read_inferior_data_pointer (
	ipa_sym_addrs.addr_set_trace_state_variable_value_ptr, &res))
    {
      error ("error extracting set_trace_state_variable_value_ptr");
      return 0;
    }
  return res;
}

static void
compile_tracepoint_condition (struct tracepoint *tpoint,
			      CORE_ADDR *jump_entry)
{
  CORE_ADDR entry_point = *jump_entry;
  enum eval_result_type err;

  trace_debug ("Starting condition compilation for tracepoint %d\n",
	       tpoint->number);

  /* Initialize the global pointer to the code being built.  */
  current_insn_ptr = *jump_entry;

  emit_prologue ();

  err = compile_bytecodes (tpoint->cond);

  if (err == expr_eval_no_error)
    {
      emit_epilogue ();

      /* Record the beginning of the compiled code.  */
      tpoint->compiled_cond = entry_point;

      trace_debug ("Condition compilation for tracepoint %d complete\n",
		   tpoint->number);
    }
  else
    {
      /* Leave the unfinished code in situ, but don't point to it.  */

      tpoint->compiled_cond = 0;

      trace_debug ("Condition compilation for tracepoint %d failed, "
		   "error code %d",
		   tpoint->number, err);
    }

  /* Update the code pointer passed in.  Note that we do this even if
     the compile fails, so that we can look at the partial results
     instead of letting them be overwritten.  */
  *jump_entry = current_insn_ptr;

  /* Leave a gap, to aid dump decipherment.  */
  *jump_entry += 16;
}

/* The base pointer of the IPA's heap.  This is the only memory the
   IPA is allowed to use.  The IPA should _not_ call the inferior's
   `malloc' during operation.  That'd be slow, and, most importantly,
   it may not be safe.  We may be collecting a tracepoint in a signal
   handler, for example.  */
static CORE_ADDR target_tp_heap;

/* Allocate at least SIZE bytes of memory from the IPA heap, aligned
   to 8 bytes.  */

static CORE_ADDR
target_malloc (ULONGEST size)
{
  CORE_ADDR ptr;

  if (target_tp_heap == 0)
    {
      /* We have the pointer *address*, need what it points to.  */
      if (read_inferior_data_pointer (ipa_sym_addrs.addr_gdb_tp_heap_buffer,
				      &target_tp_heap))
	{
	  internal_error ("couldn't get target heap head pointer");
	}
    }

  ptr = target_tp_heap;
  target_tp_heap += size;

  /* Pad to 8-byte alignment.  */
  target_tp_heap = ((target_tp_heap + 7) & ~0x7);

  return ptr;
}

static CORE_ADDR
download_agent_expr (struct agent_expr *expr)
{
  CORE_ADDR expr_addr;
  CORE_ADDR expr_bytes;

  expr_addr = target_malloc (sizeof (*expr));
  target_write_memory (expr_addr, (unsigned char *) expr, sizeof (*expr));

  expr_bytes = target_malloc (expr->length);
  write_inferior_data_pointer (expr_addr + offsetof (struct agent_expr, bytes),
			       expr_bytes);
  target_write_memory (expr_bytes, expr->bytes, expr->length);

  return expr_addr;
}

/* Align V up to N bits.  */
#define UALIGN(V, N) (((V) + ((N) - 1)) & ~((N) - 1))

/* Sync tracepoint with IPA, but leave maintenance of linked list to caller.  */

static void
download_tracepoint_1 (struct tracepoint *tpoint)
{
  struct tracepoint target_tracepoint;
  CORE_ADDR tpptr = 0;

  gdb_assert (tpoint->type == fast_tracepoint
	      || tpoint->type == static_tracepoint);

  if (tpoint->cond != NULL && target_emit_ops () != NULL)
    {
      CORE_ADDR jentry, jump_entry;

      jentry = jump_entry = get_jump_space_head ();

      if (tpoint->cond != NULL)
	{
	  /* Pad to 8-byte alignment. (needed?)  */
	  /* Actually this should be left for the target to
	     decide.  */
	  jentry = UALIGN (jentry, 8);

	  compile_tracepoint_condition (tpoint, &jentry);
	}

      /* Pad to 8-byte alignment.  */
      jentry = UALIGN (jentry, 8);
      claim_jump_space (jentry - jump_entry);
    }

  target_tracepoint = *tpoint;

  tpptr = target_malloc (sizeof (*tpoint));
  tpoint->obj_addr_on_target = tpptr;

  /* Write the whole object.  We'll fix up its pointers in a bit.
     Assume no next for now.  This is fixed up above on the next
     iteration, if there's any.  */
  target_tracepoint.next = NULL;
  /* Need to clear this here too, since we're downloading the
     tracepoints before clearing our own copy.  */
  target_tracepoint.hit_count = 0;

  target_write_memory (tpptr, (unsigned char *) &target_tracepoint,
			 sizeof (target_tracepoint));

  if (tpoint->cond)
    write_inferior_data_pointer (tpptr
				 + offsetof (struct tracepoint, cond),
				 download_agent_expr (tpoint->cond));

  if (tpoint->numactions)
    {
      int i;
      CORE_ADDR actions_array;

      /* The pointers array.  */
      actions_array
	= target_malloc (sizeof (*tpoint->actions) * tpoint->numactions);
      write_inferior_data_pointer (tpptr + offsetof (struct tracepoint,
						     actions),
				   actions_array);

      /* Now for each pointer, download the action.  */
      for (i = 0; i < tpoint->numactions; i++)
	{
	  struct tracepoint_action *action = tpoint->actions[i];
	  CORE_ADDR ipa_action = tracepoint_action_download (action);

	  if (ipa_action != 0)
	    write_inferior_data_pointer (actions_array
					 + i * sizeof (*tpoint->actions),
					 ipa_action);
	}
    }
}

#define IPA_PROTO_FAST_TRACE_FLAG 0
#define IPA_PROTO_FAST_TRACE_ADDR_ON_TARGET 2
#define IPA_PROTO_FAST_TRACE_JUMP_PAD 10
#define IPA_PROTO_FAST_TRACE_FJUMP_SIZE 18
#define IPA_PROTO_FAST_TRACE_FJUMP_INSN 22

/* Send a command to agent to download and install tracepoint TPOINT.  */

static int
tracepoint_send_agent (struct tracepoint *tpoint)
{
  char buf[IPA_CMD_BUF_SIZE];
  char *p;
  int i, ret;

  p = buf;
  strcpy (p, "FastTrace:");
  p += 10;

  COPY_FIELD_TO_BUF (p, tpoint, number);
  COPY_FIELD_TO_BUF (p, tpoint, address);
  COPY_FIELD_TO_BUF (p, tpoint, type);
  COPY_FIELD_TO_BUF (p, tpoint, enabled);
  COPY_FIELD_TO_BUF (p, tpoint, step_count);
  COPY_FIELD_TO_BUF (p, tpoint, pass_count);
  COPY_FIELD_TO_BUF (p, tpoint, numactions);
  COPY_FIELD_TO_BUF (p, tpoint, hit_count);
  COPY_FIELD_TO_BUF (p, tpoint, traceframe_usage);
  COPY_FIELD_TO_BUF (p, tpoint, compiled_cond);
  COPY_FIELD_TO_BUF (p, tpoint, orig_size);

  /* condition */
  p = agent_expr_send (p, tpoint->cond);

  /* tracepoint_action */
  for (i = 0; i < tpoint->numactions; i++)
    {
      struct tracepoint_action *action = tpoint->actions[i];

      p[0] = action->type;
      p = tracepoint_action_send (&p[1], action);
    }

  get_jump_space_head ();
  /* Copy the value of GDB_JUMP_PAD_HEAD to command buffer, so that
     agent can use jump pad from it.  */
  if (tpoint->type == fast_tracepoint)
    {
      memcpy (p, &gdb_jump_pad_head, 8);
      p += 8;
    }

  ret = run_inferior_command (buf, (int) (ptrdiff_t) (p - buf));
  if (ret)
    return ret;

  if (!startswith (buf, "OK"))
    return 1;

  /* The value of tracepoint's target address is stored in BUF.  */
  memcpy (&tpoint->obj_addr_on_target,
	  &buf[IPA_PROTO_FAST_TRACE_ADDR_ON_TARGET], 8);

  if (tpoint->type == fast_tracepoint)
    {
      unsigned char *insn
	= (unsigned char *) &buf[IPA_PROTO_FAST_TRACE_FJUMP_INSN];
      int fjump_size;

     trace_debug ("agent: read from cmd_buf 0x%x 0x%x\n",
		  (unsigned int) tpoint->obj_addr_on_target,
		  (unsigned int) gdb_jump_pad_head);

      memcpy (&gdb_jump_pad_head, &buf[IPA_PROTO_FAST_TRACE_JUMP_PAD], 8);

      /* This has been done in agent.  We should also set up record for it.  */
      memcpy (&fjump_size, &buf[IPA_PROTO_FAST_TRACE_FJUMP_SIZE], 4);
      /* Wire it in.  */
      tpoint->handle
	= set_fast_tracepoint_jump (tpoint->address, insn, fjump_size);
    }

  return 0;
}

static void
download_tracepoint (struct tracepoint *tpoint)
{
  struct tracepoint *tp, *tp_prev;

  if (tpoint->type != fast_tracepoint
      && tpoint->type != static_tracepoint)
    return;

  download_tracepoint_1 (tpoint);

  /* Find the previous entry of TPOINT, which is fast tracepoint or
     static tracepoint.  */
  tp_prev = NULL;
  for (tp = tracepoints; tp != tpoint; tp = tp->next)
    {
      if (tp->type == fast_tracepoint || tp->type == static_tracepoint)
	tp_prev = tp;
    }

  if (tp_prev)
    {
      CORE_ADDR tp_prev_target_next_addr;

      /* Insert TPOINT after TP_PREV in IPA.  */
      if (read_inferior_data_pointer (tp_prev->obj_addr_on_target
				      + offsetof (struct tracepoint, next),
				      &tp_prev_target_next_addr))
	{
	  internal_error ("error reading `tp_prev->next'");
	}

      /* tpoint->next = tp_prev->next */
      write_inferior_data_pointer (tpoint->obj_addr_on_target
				   + offsetof (struct tracepoint, next),
				   tp_prev_target_next_addr);
      /* tp_prev->next = tpoint */
      write_inferior_data_pointer (tp_prev->obj_addr_on_target
				   + offsetof (struct tracepoint, next),
				   tpoint->obj_addr_on_target);
    }
  else
    /* First object in list, set the head pointer in the
       inferior.  */
    write_inferior_data_pointer (ipa_sym_addrs.addr_tracepoints,
				 tpoint->obj_addr_on_target);

}

static void
download_trace_state_variables (void)
{
  CORE_ADDR ptr = 0, prev_ptr = 0;
  struct trace_state_variable *tsv;

  /* Start out empty.  */
  write_inferior_data_pointer (ipa_sym_addrs.addr_trace_state_variables, 0);

  for (tsv = trace_state_variables; tsv != NULL; tsv = tsv->next)
    {
      struct trace_state_variable target_tsv;

      /* TSV's with a getter have been initialized equally in both the
	 inferior and GDBserver.  Skip them.  */
      if (tsv->getter != NULL)
	continue;

      target_tsv = *tsv;

      prev_ptr = ptr;
      ptr = target_malloc (sizeof (*tsv));

      if (tsv == trace_state_variables)
	{
	  /* First object in list, set the head pointer in the
	     inferior.  */

	  write_inferior_data_pointer (ipa_sym_addrs.addr_trace_state_variables,
				       ptr);
	}
      else
	{
	  write_inferior_data_pointer (prev_ptr
				       + offsetof (struct trace_state_variable,
						   next),
				       ptr);
	}

      /* Write the whole object.  We'll fix up its pointers in a bit.
	 Assume no next, fixup when needed.  */
      target_tsv.next = NULL;

      target_write_memory (ptr, (unsigned char *) &target_tsv,
			     sizeof (target_tsv));

      if (tsv->name != NULL)
	{
	  size_t size = strlen (tsv->name) + 1;
	  CORE_ADDR name_addr = target_malloc (size);
	  target_write_memory (name_addr,
				 (unsigned char *) tsv->name, size);
	  write_inferior_data_pointer (ptr
				       + offsetof (struct trace_state_variable,
						   name),
				       name_addr);
	}

      gdb_assert (tsv->getter == NULL);
    }

  if (prev_ptr != 0)
    {
      /* Fixup the next pointer in the last item in the list.  */
      write_inferior_data_pointer (prev_ptr
				   + offsetof (struct trace_state_variable,
					       next), 0);
    }
}

/* Upload complete trace frames out of the IP Agent's trace buffer
   into GDBserver's trace buffer.  This always uploads either all or
   no trace frames.  This is the counter part of
   `trace_alloc_trace_buffer'.  See its description of the atomic
   syncing mechanism.  */

static void
upload_fast_traceframes (void)
{
  unsigned int ipa_traceframe_read_count, ipa_traceframe_write_count;
  unsigned int ipa_traceframe_read_count_racy, ipa_traceframe_write_count_racy;
  CORE_ADDR tf;
  struct ipa_trace_buffer_control ipa_trace_buffer_ctrl;
  unsigned int curr_tbctrl_idx;
  unsigned int ipa_trace_buffer_ctrl_curr;
  unsigned int ipa_trace_buffer_ctrl_curr_old;
  CORE_ADDR ipa_trace_buffer_ctrl_addr;
  struct breakpoint *about_to_request_buffer_space_bkpt;
  CORE_ADDR ipa_trace_buffer_lo;
  CORE_ADDR ipa_trace_buffer_hi;

  if (read_inferior_uinteger (ipa_sym_addrs.addr_traceframe_read_count,
			      &ipa_traceframe_read_count_racy))
    {
      /* This will happen in most targets if the current thread is
	 running.  */
      return;
    }

  if (read_inferior_uinteger (ipa_sym_addrs.addr_traceframe_write_count,
			      &ipa_traceframe_write_count_racy))
    return;

  trace_debug ("ipa_traceframe_count (racy area): %d (w=%d, r=%d)",
	       ipa_traceframe_write_count_racy
	       - ipa_traceframe_read_count_racy,
	       ipa_traceframe_write_count_racy,
	       ipa_traceframe_read_count_racy);

  if (ipa_traceframe_write_count_racy == ipa_traceframe_read_count_racy)
    return;

  about_to_request_buffer_space_bkpt
    = set_breakpoint_at (ipa_sym_addrs.addr_about_to_request_buffer_space,
			 NULL);

  if (read_inferior_uinteger (ipa_sym_addrs.addr_trace_buffer_ctrl_curr,
			      &ipa_trace_buffer_ctrl_curr))
    return;

  ipa_trace_buffer_ctrl_curr_old = ipa_trace_buffer_ctrl_curr;

  curr_tbctrl_idx = ipa_trace_buffer_ctrl_curr & ~GDBSERVER_FLUSH_COUNT_MASK;

  {
    unsigned int prev, counter;

    /* Update the token, with new counters, and the GDBserver stamp
       bit.  Always reuse the current TBC index.  */
    prev = ipa_trace_buffer_ctrl_curr & GDBSERVER_FLUSH_COUNT_MASK_CURR;
    counter = (prev + 0x100) & GDBSERVER_FLUSH_COUNT_MASK_CURR;

    ipa_trace_buffer_ctrl_curr = (GDBSERVER_UPDATED_FLUSH_COUNT_BIT
				  | (prev << 12)
				  | counter
				  | curr_tbctrl_idx);
  }

  if (write_inferior_uinteger (ipa_sym_addrs.addr_trace_buffer_ctrl_curr,
			       ipa_trace_buffer_ctrl_curr))
    return;

  trace_debug ("Lib: Committed %08x -> %08x",
	       ipa_trace_buffer_ctrl_curr_old,
	       ipa_trace_buffer_ctrl_curr);

  /* Re-read these, now that we've installed the
     `about_to_request_buffer_space' breakpoint/lock.  A thread could
     have finished a traceframe between the last read of these
     counters and setting the breakpoint above.  If we start
     uploading, we never want to leave this function with
     traceframe_read_count != 0, otherwise, GDBserver could end up
     incrementing the counter tokens more than once (due to event loop
     nesting), which would break the IP agent's "effective" detection
     (see trace_alloc_trace_buffer).  */
  if (read_inferior_uinteger (ipa_sym_addrs.addr_traceframe_read_count,
			      &ipa_traceframe_read_count))
    return;
  if (read_inferior_uinteger (ipa_sym_addrs.addr_traceframe_write_count,
			      &ipa_traceframe_write_count))
    return;

  if (debug_threads)
    {
      trace_debug ("ipa_traceframe_count (blocked area): %d (w=%d, r=%d)",
		   ipa_traceframe_write_count - ipa_traceframe_read_count,
		   ipa_traceframe_write_count, ipa_traceframe_read_count);

      if (ipa_traceframe_write_count != ipa_traceframe_write_count_racy
	  || ipa_traceframe_read_count != ipa_traceframe_read_count_racy)
	trace_debug ("note that ipa_traceframe_count's parts changed");
    }

  /* Get the address of the current TBC object (the IP agent has an
     array of 3 such objects).  The index is stored in the TBC
     token.  */
  ipa_trace_buffer_ctrl_addr = ipa_sym_addrs.addr_trace_buffer_ctrl;
  ipa_trace_buffer_ctrl_addr
    += sizeof (struct ipa_trace_buffer_control) * curr_tbctrl_idx;

  if (read_inferior_memory (ipa_trace_buffer_ctrl_addr,
			    (unsigned char *) &ipa_trace_buffer_ctrl,
			    sizeof (struct ipa_trace_buffer_control)))
    return;

  if (read_inferior_data_pointer (ipa_sym_addrs.addr_trace_buffer_lo,
				  &ipa_trace_buffer_lo))
    return;
  if (read_inferior_data_pointer (ipa_sym_addrs.addr_trace_buffer_hi,
				  &ipa_trace_buffer_hi))
    return;

  /* Offsets are easier to grok for debugging than raw addresses,
     especially for the small trace buffer sizes that are useful for
     testing.  */
  trace_debug ("Lib: Trace buffer [%d] start=%d free=%d "
	       "endfree=%d wrap=%d hi=%d",
	       curr_tbctrl_idx,
	       (int) (ipa_trace_buffer_ctrl.start - ipa_trace_buffer_lo),
	       (int) (ipa_trace_buffer_ctrl.free - ipa_trace_buffer_lo),
	       (int) (ipa_trace_buffer_ctrl.end_free - ipa_trace_buffer_lo),
	       (int) (ipa_trace_buffer_ctrl.wrap - ipa_trace_buffer_lo),
	       (int) (ipa_trace_buffer_hi - ipa_trace_buffer_lo));

  /* Note that the IPA's buffer is always circular.  */

#define IPA_FIRST_TRACEFRAME() (ipa_trace_buffer_ctrl.start)

#define IPA_NEXT_TRACEFRAME_1(TF, TFOBJ)		\
  ((TF) + sizeof (struct traceframe) + (TFOBJ)->data_size)

#define IPA_NEXT_TRACEFRAME(TF, TFOBJ)					\
  (IPA_NEXT_TRACEFRAME_1 (TF, TFOBJ)					\
   - ((IPA_NEXT_TRACEFRAME_1 (TF, TFOBJ) >= ipa_trace_buffer_ctrl.wrap) \
      ? (ipa_trace_buffer_ctrl.wrap - ipa_trace_buffer_lo)		\
      : 0))

  tf = IPA_FIRST_TRACEFRAME ();

  while (ipa_traceframe_write_count - ipa_traceframe_read_count)
    {
      struct tracepoint *tpoint;
      struct traceframe *tframe;
      unsigned char *block;
      struct traceframe ipa_tframe;

      if (read_inferior_memory (tf, (unsigned char *) &ipa_tframe,
				offsetof (struct traceframe, data)))
	error ("Uploading: couldn't read traceframe at %s\n", paddress (tf));

      if (ipa_tframe.tpnum == 0)
	{
	  internal_error ("Uploading: No (more) fast traceframes, but"
			  " ipa_traceframe_count == %u??\n",
			  ipa_traceframe_write_count
			  - ipa_traceframe_read_count);
	}

      /* Note that this will be incorrect for multi-location
	 tracepoints...  */
      tpoint = find_next_tracepoint_by_number (NULL, ipa_tframe.tpnum);

      tframe = add_traceframe (tpoint);
      if (tframe == NULL)
	{
	  trace_buffer_is_full = 1;
	  trace_debug ("Uploading: trace buffer is full");
	}
      else
	{
	  /* Copy the whole set of blocks in one go for now.  FIXME:
	     split this in smaller blocks.  */
	  block = add_traceframe_block (tframe, tpoint,
					ipa_tframe.data_size);
	  if (block != NULL)
	    {
	      if (read_inferior_memory (tf
					+ offsetof (struct traceframe, data),
					block, ipa_tframe.data_size))
		error ("Uploading: Couldn't read traceframe data at %s\n",
		       paddress (tf + offsetof (struct traceframe, data)));
	    }

	  trace_debug ("Uploading: traceframe didn't fit");
	  finish_traceframe (tframe);
	}

      tf = IPA_NEXT_TRACEFRAME (tf, &ipa_tframe);

      /* If we freed the traceframe that wrapped around, go back
	 to the non-wrap case.  */
      if (tf < ipa_trace_buffer_ctrl.start)
	{
	  trace_debug ("Lib: Discarding past the wraparound");
	  ipa_trace_buffer_ctrl.wrap = ipa_trace_buffer_hi;
	}
      ipa_trace_buffer_ctrl.start = tf;
      ipa_trace_buffer_ctrl.end_free = ipa_trace_buffer_ctrl.start;
      ++ipa_traceframe_read_count;

      if (ipa_trace_buffer_ctrl.start == ipa_trace_buffer_ctrl.free
	  && ipa_trace_buffer_ctrl.start == ipa_trace_buffer_ctrl.end_free)
	{
	  trace_debug ("Lib: buffer is fully empty.  "
		       "Trace buffer [%d] start=%d free=%d endfree=%d",
		       curr_tbctrl_idx,
		       (int) (ipa_trace_buffer_ctrl.start
			      - ipa_trace_buffer_lo),
		       (int) (ipa_trace_buffer_ctrl.free
			      - ipa_trace_buffer_lo),
		       (int) (ipa_trace_buffer_ctrl.end_free
			      - ipa_trace_buffer_lo));

	  ipa_trace_buffer_ctrl.start = ipa_trace_buffer_lo;
	  ipa_trace_buffer_ctrl.free = ipa_trace_buffer_lo;
	  ipa_trace_buffer_ctrl.end_free = ipa_trace_buffer_hi;
	  ipa_trace_buffer_ctrl.wrap = ipa_trace_buffer_hi;
	}

      trace_debug ("Uploaded a traceframe\n"
		   "Lib: Trace buffer [%d] start=%d free=%d "
		   "endfree=%d wrap=%d hi=%d",
		   curr_tbctrl_idx,
		   (int) (ipa_trace_buffer_ctrl.start - ipa_trace_buffer_lo),
		   (int) (ipa_trace_buffer_ctrl.free - ipa_trace_buffer_lo),
		   (int) (ipa_trace_buffer_ctrl.end_free
			  - ipa_trace_buffer_lo),
		   (int) (ipa_trace_buffer_ctrl.wrap - ipa_trace_buffer_lo),
		   (int) (ipa_trace_buffer_hi - ipa_trace_buffer_lo));
    }

  if (target_write_memory (ipa_trace_buffer_ctrl_addr,
			     (unsigned char *) &ipa_trace_buffer_ctrl,
			     sizeof (struct ipa_trace_buffer_control)))
    return;

  write_inferior_integer (ipa_sym_addrs.addr_traceframe_read_count,
			  ipa_traceframe_read_count);

  trace_debug ("Done uploading traceframes [%d]\n", curr_tbctrl_idx);

  target_pause_all (true);

  delete_breakpoint (about_to_request_buffer_space_bkpt);
  about_to_request_buffer_space_bkpt = NULL;

  target_unpause_all (true);

  if (trace_buffer_is_full)
    stop_tracing ();
}
#endif

#ifdef IN_PROCESS_AGENT

IP_AGENT_EXPORT_VAR int ust_loaded;
IP_AGENT_EXPORT_VAR char cmd_buf[IPA_CMD_BUF_SIZE];

#ifdef HAVE_UST

/* Static tracepoints.  */

/* UST puts a "struct tracepoint" in the global namespace, which
   conflicts with our tracepoint.  Arguably, being a library, it
   shouldn't take ownership of such a generic name.  We work around it
   here.  */
#define tracepoint ust_tracepoint
#include <ust/ust.h>
#undef tracepoint

extern int serialize_to_text (char *outbuf, int bufsize,
			      const char *fmt, va_list ap);

#define GDB_PROBE_NAME "gdb"

/* We dynamically search for the UST symbols instead of linking them
   in.  This lets the user decide if the application uses static
   tracepoints, instead of always pulling libust.so in.  This vector
   holds pointers to all functions we care about.  */

static struct
{
  int (*serialize_to_text) (char *outbuf, int bufsize,
			    const char *fmt, va_list ap);

  int (*ltt_probe_register) (struct ltt_available_probe *pdata);
  int (*ltt_probe_unregister) (struct ltt_available_probe *pdata);

  int (*ltt_marker_connect) (const char *channel, const char *mname,
			     const char *pname);
  int (*ltt_marker_disconnect) (const char *channel, const char *mname,
				const char *pname);

  void (*marker_iter_start) (struct marker_iter *iter);
  void (*marker_iter_next) (struct marker_iter *iter);
  void (*marker_iter_stop) (struct marker_iter *iter);
  void (*marker_iter_reset) (struct marker_iter *iter);
} ust_ops;

#include <dlfcn.h>

/* Cast through typeof to catch incompatible API changes.  Since UST
   only builds with gcc, we can freely use gcc extensions here
   too.  */
#define GET_UST_SYM(SYM)					\
  do								\
    {								\
      if (ust_ops.SYM == NULL)					\
	ust_ops.SYM = (typeof (&SYM)) dlsym (RTLD_DEFAULT, #SYM);	\
      if (ust_ops.SYM == NULL)					\
	return 0;						\
    } while (0)

#define USTF(SYM) ust_ops.SYM

/* Get pointers to all libust.so functions we care about.  */

static int
dlsym_ust (void)
{
  GET_UST_SYM (serialize_to_text);

  GET_UST_SYM (ltt_probe_register);
  GET_UST_SYM (ltt_probe_unregister);
  GET_UST_SYM (ltt_marker_connect);
  GET_UST_SYM (ltt_marker_disconnect);

  GET_UST_SYM (marker_iter_start);
  GET_UST_SYM (marker_iter_next);
  GET_UST_SYM (marker_iter_stop);
  GET_UST_SYM (marker_iter_reset);

  ust_loaded = 1;
  return 1;
}

/* Given an UST marker, return the matching gdb static tracepoint.
   The match is done by address.  */

static struct tracepoint *
ust_marker_to_static_tracepoint (const struct marker *mdata)
{
  struct tracepoint *tpoint;

  for (tpoint = tracepoints; tpoint; tpoint = tpoint->next)
    {
      if (tpoint->type != static_tracepoint)
	continue;

      if (tpoint->address == (uintptr_t) mdata->location)
	return tpoint;
    }

  return NULL;
}

/* The probe function we install on lttng/ust markers.  Whenever a
   probed ust marker is hit, this function is called.  This is similar
   to gdb_collect, only for static tracepoints, instead of fast
   tracepoints.  */

static void
gdb_probe (const struct marker *mdata, void *probe_private,
	   struct registers *regs, void *call_private,
	   const char *fmt, va_list *args)
{
  struct tracepoint *tpoint;
  struct static_tracepoint_ctx ctx;
  const struct target_desc *ipa_tdesc;

  /* Don't do anything until the trace run is completely set up.  */
  if (!tracing)
    {
      trace_debug ("gdb_probe: not tracing\n");
      return;
    }

  ipa_tdesc = get_ipa_tdesc (ipa_tdesc_idx);
  ctx.base.type = static_tracepoint;
  ctx.regcache_initted = 0;
  ctx.regs = regs;
  ctx.fmt = fmt;
  ctx.args = args;

  /* Wrap the regblock in a register cache (in the stack, we don't
     want to malloc here).  */
  ctx.regspace = alloca (ipa_tdesc->registers_size);
  if (ctx.regspace == NULL)
    {
      trace_debug ("Trace buffer block allocation failed, skipping");
      return;
    }

  tpoint = ust_marker_to_static_tracepoint (mdata);
  if (tpoint == NULL)
    {
      trace_debug ("gdb_probe: marker not known: "
		   "loc:0x%p, ch:\"%s\",n:\"%s\",f:\"%s\"",
		   mdata->location, mdata->channel,
		   mdata->name, mdata->format);
      return;
    }

  if (!tpoint->enabled)
    {
      trace_debug ("gdb_probe: tracepoint disabled");
      return;
    }

  ctx.tpoint = tpoint;

  trace_debug ("gdb_probe: collecting marker: "
	       "loc:0x%p, ch:\"%s\",n:\"%s\",f:\"%s\"",
	       mdata->location, mdata->channel,
	       mdata->name, mdata->format);

  /* Test the condition if present, and collect if true.  */
  if (tpoint->cond == NULL
      || condition_true_at_tracepoint ((struct tracepoint_hit_ctx *) &ctx,
				       tpoint))
    {
      collect_data_at_tracepoint ((struct tracepoint_hit_ctx *) &ctx,
				  tpoint->address, tpoint);

      if (stopping_tracepoint
	  || trace_buffer_is_full
	  || expr_eval_result != expr_eval_no_error)
	stop_tracing ();
    }
  else
    {
      /* If there was a condition and it evaluated to false, the only
	 way we would stop tracing is if there was an error during
	 condition expression evaluation.  */
      if (expr_eval_result != expr_eval_no_error)
	stop_tracing ();
    }
}

/* Called if the gdb static tracepoint requested collecting "$_sdata",
   static tracepoint string data.  This is a string passed to the
   tracing library by the user, at the time of the tracepoint marker
   call.  E.g., in the UST marker call:

     trace_mark (ust, bar33, "str %s", "FOOBAZ");

   the collected data is "str FOOBAZ".
*/

static void
collect_ust_data_at_tracepoint (struct tracepoint_hit_ctx *ctx,
				struct traceframe *tframe)
{
  struct static_tracepoint_ctx *umd = (struct static_tracepoint_ctx *) ctx;
  unsigned char *bufspace;
  int size;
  va_list copy;
  unsigned short blocklen;

  if (umd == NULL)
    {
      trace_debug ("Wanted to collect static trace data, "
		   "but there's no static trace data");
      return;
    }

  va_copy (copy, *umd->args);
  size = USTF(serialize_to_text) (NULL, 0, umd->fmt, copy);
  va_end (copy);

  trace_debug ("Want to collect ust data");

  /* 'S' + size + string */
  bufspace = add_traceframe_block (tframe, umd->tpoint,
				   1 + sizeof (blocklen) + size + 1);
  if (bufspace == NULL)
    {
      trace_debug ("Trace buffer block allocation failed, skipping");
      return;
    }

  /* Identify a static trace data block.  */
  *bufspace = 'S';

  blocklen = size + 1;
  memcpy (bufspace + 1, &blocklen, sizeof (blocklen));

  va_copy (copy, *umd->args);
  USTF(serialize_to_text) ((char *) bufspace + 1 + sizeof (blocklen),
			   size + 1, umd->fmt, copy);
  va_end (copy);

  trace_debug ("Storing static tracepoint data in regblock: %s",
	       bufspace + 1 + sizeof (blocklen));
}

/* The probe to register with lttng/ust.  */
static struct ltt_available_probe gdb_ust_probe =
  {
    GDB_PROBE_NAME,
    NULL,
    gdb_probe,
  };

#endif /* HAVE_UST */
#endif /* IN_PROCESS_AGENT */

#ifndef IN_PROCESS_AGENT

/* Ask the in-process agent to run a command.  Since we don't want to
   have to handle the IPA hitting breakpoints while running the
   command, we pause all threads, remove all breakpoints, and then set
   the helper thread re-running.  We communicate with the helper
   thread by means of direct memory xfering, and a socket for
   synchronization.  */

static int
run_inferior_command (char *cmd, int len)
{
  int err = -1;
  int pid = current_ptid.pid ();

  trace_debug ("run_inferior_command: running: %s", cmd);

  target_pause_all (false);
  uninsert_all_breakpoints ();

  err = agent_run_command (pid, cmd, len);

  reinsert_all_breakpoints ();
  target_unpause_all (false);

  return err;
}

#else /* !IN_PROCESS_AGENT */

#include <sys/socket.h>
#include <sys/un.h>

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(((struct sockaddr_un *) NULL)->sun_path)
#endif

/* Where we put the socked used for synchronization.  */
#define SOCK_DIR P_tmpdir

/* Thread ID of the helper thread.  GDBserver reads this to know which
   is the help thread.  This is an LWP id on Linux.  */
extern "C" {
IP_AGENT_EXPORT_VAR int helper_thread_id;
}

static int
init_named_socket (const char *name)
{
  int result, fd;
  struct sockaddr_un addr;

  result = fd = socket (PF_UNIX, SOCK_STREAM, 0);
  if (result == -1)
    {
      warning ("socket creation failed: %s", safe_strerror (errno));
      return -1;
    }

  addr.sun_family = AF_UNIX;

  if (strlen (name) >= ARRAY_SIZE (addr.sun_path))
    {
      warning ("socket name too long for sockaddr_un::sun_path field: %s", name);
      return -1;
    }

  strcpy (addr.sun_path, name);

  result = access (name, F_OK);
  if (result == 0)
    {
      /* File exists.  */
      result = unlink (name);
      if (result == -1)
	{
	  warning ("unlink failed: %s", safe_strerror (errno));
	  close (fd);
	  return -1;
	}
      warning ("socket %s already exists; overwriting", name);
    }

  result = bind (fd, (struct sockaddr *) &addr, sizeof (addr));
  if (result == -1)
    {
      warning ("bind failed: %s", safe_strerror (errno));
      close (fd);
      return -1;
    }

  result = listen (fd, 1);
  if (result == -1)
    {
      warning ("listen: %s", safe_strerror (errno));
      close (fd);
      return -1;
    }

  return fd;
}

static char agent_socket_name[UNIX_PATH_MAX];

static int
gdb_agent_socket_init (void)
{
  int result, fd;

  result = snprintf (agent_socket_name, UNIX_PATH_MAX, "%s/gdb_ust%d",
		     SOCK_DIR, getpid ());
  if (result >= UNIX_PATH_MAX)
    {
      trace_debug ("string overflow allocating socket name");
      return -1;
    }

  fd = init_named_socket (agent_socket_name);
  if (fd < 0)
    warning ("Error initializing named socket (%s) for communication with the "
	     "ust helper thread. Check that directory exists and that it "
	     "is writable.", agent_socket_name);

  return fd;
}

#ifdef HAVE_UST

/* The next marker to be returned on a qTsSTM command.  */
static const struct marker *next_st;

/* Returns the first known marker.  */

struct marker *
first_marker (void)
{
  struct marker_iter iter;

  USTF(marker_iter_reset) (&iter);
  USTF(marker_iter_start) (&iter);

  return iter.marker;
}

/* Returns the marker following M.  */

const struct marker *
next_marker (const struct marker *m)
{
  struct marker_iter iter;

  USTF(marker_iter_reset) (&iter);
  USTF(marker_iter_start) (&iter);

  for (; iter.marker != NULL; USTF(marker_iter_next) (&iter))
    {
      if (iter.marker == m)
	{
	  USTF(marker_iter_next) (&iter);
	  return iter.marker;
	}
    }

  return NULL;
}

/* Return an hexstr version of the STR C string, fit for sending to
   GDB.  */

static char *
cstr_to_hexstr (const char *str)
{
  int len = strlen (str);
  char *hexstr = xmalloc (len * 2 + 1);
  bin2hex ((gdb_byte *) str, hexstr, len);
  return hexstr;
}

/* Compose packet that is the response to the qTsSTM/qTfSTM/qTSTMat
   packets.  */

static void
response_ust_marker (char *packet, const struct marker *st)
{
  char *strid, *format, *tmp;

  next_st = next_marker (st);

  tmp = xmalloc (strlen (st->channel) + 1 +
		 strlen (st->name) + 1);
  sprintf (tmp, "%s/%s", st->channel, st->name);

  strid = cstr_to_hexstr (tmp);
  free (tmp);

  format = cstr_to_hexstr (st->format);

  sprintf (packet, "m%s:%s:%s",
	   paddress ((uintptr_t) st->location),
	   strid,
	   format);

  free (strid);
  free (format);
}

/* Return the first static tracepoint, and initialize the state
   machine that will iterate through all the static tracepoints.  */

static void
cmd_qtfstm (char *packet)
{
  trace_debug ("Returning first trace state variable definition");

  if (first_marker ())
    response_ust_marker (packet, first_marker ());
  else
    strcpy (packet, "l");
}

/* Return additional trace state variable definitions. */

static void
cmd_qtsstm (char *packet)
{
  trace_debug ("Returning static tracepoint");

  if (next_st)
    response_ust_marker (packet, next_st);
  else
    strcpy (packet, "l");
}

/* Disconnect the GDB probe from a marker at a given address.  */

static void
unprobe_marker_at (char *packet)
{
  char *p = packet;
  ULONGEST address;
  struct marker_iter iter;

  p += sizeof ("unprobe_marker_at:") - 1;

  p = unpack_varlen_hex (p, &address);

  USTF(marker_iter_reset) (&iter);
  USTF(marker_iter_start) (&iter);
  for (; iter.marker != NULL; USTF(marker_iter_next) (&iter))
    if ((uintptr_t ) iter.marker->location == address)
      {
	int result;

	result = USTF(ltt_marker_disconnect) (iter.marker->channel,
					      iter.marker->name,
					      GDB_PROBE_NAME);
	if (result < 0)
	  warning ("could not disable marker %s/%s",
		   iter.marker->channel, iter.marker->name);
	break;
      }
}

/* Connect the GDB probe to a marker at a given address.  */

static int
probe_marker_at (char *packet)
{
  char *p = packet;
  ULONGEST address;
  struct marker_iter iter;
  struct marker *m;

  p += sizeof ("probe_marker_at:") - 1;

  p = unpack_varlen_hex (p, &address);

  USTF(marker_iter_reset) (&iter);

  for (USTF(marker_iter_start) (&iter), m = iter.marker;
       m != NULL;
       USTF(marker_iter_next) (&iter), m = iter.marker)
    if ((uintptr_t ) m->location == address)
      {
	int result;

	trace_debug ("found marker for address.  "
		     "ltt_marker_connect (marker = %s/%s)",
		     m->channel, m->name);

	result = USTF(ltt_marker_connect) (m->channel, m->name,
					   GDB_PROBE_NAME);
	if (result && result != -EEXIST)
	  trace_debug ("ltt_marker_connect (marker = %s/%s, errno = %d)",
		       m->channel, m->name, -result);

	if (result < 0)
	  {
	    sprintf (packet, "E.could not connect marker: channel=%s, name=%s",
		     m->channel, m->name);
	    return -1;
	  }

	strcpy (packet, "OK");
	return 0;
      }

  sprintf (packet, "E.no marker found at 0x%s", paddress (address));
  return -1;
}

static int
cmd_qtstmat (char *packet)
{
  char *p = packet;
  ULONGEST address;
  struct marker_iter iter;
  struct marker *m;

  p += sizeof ("qTSTMat:") - 1;

  p = unpack_varlen_hex (p, &address);

  USTF(marker_iter_reset) (&iter);

  for (USTF(marker_iter_start) (&iter), m = iter.marker;
       m != NULL;
       USTF(marker_iter_next) (&iter), m = iter.marker)
    if ((uintptr_t ) m->location == address)
      {
	response_ust_marker (packet, m);
	return 0;
      }

  strcpy (packet, "l");
  return -1;
}

static void
gdb_ust_init (void)
{
  if (!dlsym_ust ())
    return;

  USTF(ltt_probe_register) (&gdb_ust_probe);
}

#endif /* HAVE_UST */

#include <sys/syscall.h>

static void
gdb_agent_remove_socket (void)
{
  unlink (agent_socket_name);
}

/* Helper thread of agent.  */

static void *
gdb_agent_helper_thread (void *arg)
{
  int listen_fd;

  atexit (gdb_agent_remove_socket);

  while (1)
    {
      listen_fd = gdb_agent_socket_init ();

      if (helper_thread_id == 0)
	helper_thread_id = syscall (SYS_gettid);

      if (listen_fd == -1)
	{
	  warning ("could not create sync socket");
	  break;
	}

      while (1)
	{
	  socklen_t tmp;
	  struct sockaddr_un sockaddr;
	  int fd;
	  char buf[1];
	  int ret;
	  int stop_loop = 0;

	  tmp = sizeof (sockaddr);

	  do
	    {
	      fd = accept (listen_fd, (struct sockaddr *) &sockaddr, &tmp);
	    }
	  /* It seems an ERESTARTSYS can escape out of accept.  */
	  while (fd == -512 || (fd == -1 && errno == EINTR));

	  if (fd < 0)
	    {
	      warning ("Accept returned %d, error: %s",
		       fd, safe_strerror (errno));
	      break;
	    }

	  do
	    {
	      ret = read (fd, buf, 1);
	    } while (ret == -1 && errno == EINTR);

	  if (ret == -1)
	    {
	      warning ("reading socket (fd=%d) failed with %s",
		       fd, safe_strerror (errno));
	      close (fd);
	      break;
	    }

	  if (cmd_buf[0])
	    {
	      if (startswith (cmd_buf, "close"))
		{
		  stop_loop = 1;
		}
#ifdef HAVE_UST
	      else if (strcmp ("qTfSTM", cmd_buf) == 0)
		{
		  cmd_qtfstm (cmd_buf);
		}
	      else if (strcmp ("qTsSTM", cmd_buf) == 0)
		{
		  cmd_qtsstm (cmd_buf);
		}
	      else if (startswith (cmd_buf, "unprobe_marker_at:"))
		{
		  unprobe_marker_at (cmd_buf);
		}
	      else if (startswith (cmd_buf, "probe_marker_at:"))
		{
		  probe_marker_at (cmd_buf);
		}
	      else if (startswith (cmd_buf, "qTSTMat:"))
		{
		  cmd_qtstmat (cmd_buf);
		}
#endif /* HAVE_UST */
	    }

	  /* Fix compiler's warning: ignoring return value of 'write'.  */
	  ret = write (fd, buf, 1);
	  close (fd);

	  if (stop_loop)
	    {
	      close (listen_fd);
	      unlink (agent_socket_name);

	      /* Sleep endlessly to wait the whole inferior stops.  This
		 thread can not exit because GDB or GDBserver may still need
		 'current_thread' (representing this thread) to access
		 inferior memory.  Otherwise, this thread exits earlier than
		 other threads, and 'current_thread' is set to NULL.  */
	      while (1)
		sleep (10);
	    }
	}
    }

  return NULL;
}

#include <signal.h>
#include <pthread.h>

extern "C" {
IP_AGENT_EXPORT_VAR int gdb_agent_capability = AGENT_CAPA_STATIC_TRACE;
}

static void
gdb_agent_init (void)
{
  int res;
  pthread_t thread;
  sigset_t new_mask;
  sigset_t orig_mask;

  /* We want the helper thread to be as transparent as possible, so
     have it inherit an all-signals-blocked mask.  */

  sigfillset (&new_mask);
  res = pthread_sigmask (SIG_SETMASK, &new_mask, &orig_mask);
  if (res)
    perror_with_name ("pthread_sigmask (1)");

  res = pthread_create (&thread,
			NULL,
			gdb_agent_helper_thread,
			NULL);

  res = pthread_sigmask (SIG_SETMASK, &orig_mask, NULL);
  if (res)
    perror_with_name ("pthread_sigmask (2)");

  while (helper_thread_id == 0)
    usleep (1);

#ifdef HAVE_UST
  gdb_ust_init ();
#endif
}

#include <sys/mman.h>

IP_AGENT_EXPORT_VAR char *gdb_tp_heap_buffer;
IP_AGENT_EXPORT_VAR char *gdb_jump_pad_buffer;
IP_AGENT_EXPORT_VAR char *gdb_jump_pad_buffer_end;
IP_AGENT_EXPORT_VAR char *gdb_trampoline_buffer;
IP_AGENT_EXPORT_VAR char *gdb_trampoline_buffer_end;
IP_AGENT_EXPORT_VAR char *gdb_trampoline_buffer_error;

/* Record the result of getting buffer space for fast tracepoint
   trampolines.  Any error message is copied, since caller may not be
   using persistent storage.  */

void
set_trampoline_buffer_space (CORE_ADDR begin, CORE_ADDR end, char *errmsg)
{
  gdb_trampoline_buffer = (char *) (uintptr_t) begin;
  gdb_trampoline_buffer_end = (char *) (uintptr_t) end;
  if (errmsg)
    strncpy (gdb_trampoline_buffer_error, errmsg, 99);
  else
    strcpy (gdb_trampoline_buffer_error, "no buffer passed");
}

static void __attribute__ ((constructor))
initialize_tracepoint_ftlib (void)
{
  initialize_tracepoint ();

  gdb_agent_init ();
}

#ifndef HAVE_GETAUXVAL
/* Retrieve the value of TYPE from the auxiliary vector.  If TYPE is not
   found, 0 is returned.  This function is provided if glibc is too old.  */

unsigned long
getauxval (unsigned long type)
{
  unsigned long data[2];
  FILE *f = fopen ("/proc/self/auxv", "r");
  unsigned long value = 0;

  if (f == NULL)
    return 0;

  while (fread (data, sizeof (data), 1, f) > 0)
    {
      if (data[0] == type)
	{
	  value = data[1];
	  break;
	}
    }

  fclose (f);
  return value;
}
#endif

#endif /* IN_PROCESS_AGENT */

/* Return a timestamp, expressed as microseconds of the usual Unix
   time.  (As the result is a 64-bit number, it will not overflow any
   time soon.)  */

static LONGEST
get_timestamp (void)
{
  using namespace std::chrono;

  steady_clock::time_point now = steady_clock::now ();
  return duration_cast<microseconds> (now.time_since_epoch ()).count ();
}

void
initialize_tracepoint (void)
{
  /* Start with the default size.  */
  init_trace_buffer (DEFAULT_TRACE_BUFFER_SIZE);

  /* Wire trace state variable 1 to be the timestamp.  This will be
     uploaded to GDB upon connection and become one of its trace state
     variables.  (In case you're wondering, if GDB already has a trace
     variable numbered 1, it will be renumbered.)  */
  create_trace_state_variable (1, 0);
  set_trace_state_variable_name (1, "trace_timestamp");
  set_trace_state_variable_getter (1, get_timestamp);

#ifdef IN_PROCESS_AGENT
  {
    int pagesize;
    size_t jump_pad_size;

    pagesize = sysconf (_SC_PAGE_SIZE);
    if (pagesize == -1)
      perror_with_name ("sysconf");

#define SCRATCH_BUFFER_NPAGES 20

    jump_pad_size = pagesize * SCRATCH_BUFFER_NPAGES;

    gdb_tp_heap_buffer = (char *) xmalloc (5 * 1024 * 1024);
    gdb_jump_pad_buffer = (char *) alloc_jump_pad_buffer (jump_pad_size);
    if (gdb_jump_pad_buffer == NULL)
      perror_with_name ("mmap");
    gdb_jump_pad_buffer_end = gdb_jump_pad_buffer + jump_pad_size;
  }

  gdb_trampoline_buffer = gdb_trampoline_buffer_end = 0;

  /* It's not a fatal error for something to go wrong with trampoline
     buffer setup, but it can be mysterious, so create a channel to
     report back on what went wrong, using a fixed size since we may
     not be able to allocate space later when the problem occurs.  */
  gdb_trampoline_buffer_error = (char *) xmalloc (IPA_BUFSIZ);

  strcpy (gdb_trampoline_buffer_error, "No errors reported");

  initialize_low_tracepoint ();
#endif
}
