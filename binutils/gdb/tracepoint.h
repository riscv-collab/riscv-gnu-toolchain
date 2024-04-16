/* Data structures associated with tracepoints in GDB.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.

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

#if !defined (TRACEPOINT_H)
#define TRACEPOINT_H 1

#include "breakpoint.h"
#include "memrange.h"
#include "gdbsupport/gdb_vecs.h"

#include <vector>
#include <string>

/* An object describing the contents of a traceframe.  */

struct traceframe_info
{
  /* Collected memory.  */
  std::vector<mem_range> memory;

  /* Collected trace state variables.  */
  std::vector<int> tvars;
};

typedef std::unique_ptr<traceframe_info> traceframe_info_up;

/* A trace state variable is a value managed by a target being
   traced.  A trace state variable (or tsv for short) can be accessed
   and assigned to by tracepoint actions and conditionals, but is not
   part of the program being traced, and it doesn't have to be
   collected.  Effectively the variables are scratch space for
   tracepoints.  */

struct trace_state_variable
{
  trace_state_variable (std::string &&name_, int number_)
  : name (name_), number (number_)
  {}

  /* The variable's name.  The user has to prefix with a dollar sign,
     but we don't store that internally.  */
  std::string name;

  /* An id number assigned by GDB, and transmitted to targets.  */
  int number = 0;

  /* The initial value of a variable is a 64-bit signed integer.  */
  LONGEST initial_value = 0;

  /* 1 if the value is known, else 0.  The value is known during a
     trace run, or in tfind mode if the variable was collected into
     the current trace frame.  */
  int value_known = 0;

  /* The value of a variable is a 64-bit signed integer.  */
  LONGEST value = 0;

  /* This is true for variables that are predefined and built into
     the target.  */
  int builtin = 0;
 };

/* The trace status encompasses various info about the general state
   of the tracing run.  */

enum trace_stop_reason
  {
    trace_stop_reason_unknown,
    trace_never_run,
    trace_stop_command,
    trace_buffer_full,
    trace_disconnected,
    tracepoint_passcount,
    tracepoint_error
  };

struct trace_status
{
  /* If the status is coming from a file rather than a live target,
     this points at the file's filename.  Otherwise, this is NULL.  */
  const char *filename;

  /* This is true if the value of the running field is known.  */
  int running_known;

  /* This is true when the trace experiment is actually running.  */
  int running;

  enum trace_stop_reason stop_reason;

  /* If stop_reason is tracepoint_passcount or tracepoint_error, this
     is the (on-target) number of the tracepoint which caused the
     stop.  */
  int stopping_tracepoint;

  /* If stop_reason is tstop_command or tracepoint_error, this is an
     arbitrary string that may describe the reason for the stop in
     more detail.  */

  char *stop_desc;

  /* Number of traceframes currently in the buffer.  */

  int traceframe_count;

  /* Number of traceframes created since start of run.  */

  int traceframes_created;

  /* Total size of the target's trace buffer.  */

  int buffer_size;

  /* Unused bytes left in the target's trace buffer.  */

  int buffer_free;

  /* 1 if the target will continue tracing after disconnection, else
     0.  If the target does not report a value, assume 0.  */

  int disconnected_tracing;

  /* 1 if the target is using a circular trace buffer, else 0.  If the
     target does not report a value, assume 0.  */

  int circular_buffer;

  /* The "name" of the person running the trace.  This is an
     arbitrary string.  */

  char *user_name;

  /* "Notes" about the trace.  This is an arbitrary string not
     interpreted by GDBserver in any special way.  */

  char *notes;

  /* The calendar times at which the trace run started and stopped,
     both expressed in microseconds of Unix time.  */

  LONGEST start_time;
  LONGEST stop_time;
};

struct trace_status *current_trace_status (void);

extern std::string default_collect;

extern int trace_regblock_size;

extern const char *stop_reason_names[];

/* Struct to collect random info about tracepoints on the target.  */

struct uploaded_tp
{
  int number = 0;
  enum bptype type = bp_none;
  ULONGEST addr = 0;
  int enabled = 0;
  int step = 0;
  int pass = 0;
  int orig_size = 0;

  /* String that is the encoded form of the tracepoint's condition.  */
  gdb::unique_xmalloc_ptr<char[]> cond;

  /* Vectors of strings that are the encoded forms of a tracepoint's
     actions.  */
  std::vector<gdb::unique_xmalloc_ptr<char[]>> actions;
  std::vector<gdb::unique_xmalloc_ptr<char[]>> step_actions;

  /* The original string defining the location of the tracepoint.  */
  gdb::unique_xmalloc_ptr<char[]> at_string;

  /* The original string defining the tracepoint's condition.  */
  gdb::unique_xmalloc_ptr<char[]> cond_string;

  /* List of original strings defining the tracepoint's actions.  */
  std::vector<gdb::unique_xmalloc_ptr<char[]>> cmd_strings;

  /* The tracepoint's current hit count.  */
  int hit_count = 0;

  /* The tracepoint's current traceframe usage.  */
  ULONGEST traceframe_usage = 0;

  struct uploaded_tp *next = nullptr;
};

/* Struct recording info about trace state variables on the target.  */

struct uploaded_tsv
{
  const char *name;
  int number;
  LONGEST initial_value;
  int builtin;
  struct uploaded_tsv *next;
};

/* Struct recording info about a target static tracepoint marker.  */

struct static_tracepoint_marker
{
  DISABLE_COPY_AND_ASSIGN (static_tracepoint_marker);

  static_tracepoint_marker () = default;
  static_tracepoint_marker (static_tracepoint_marker &&) = default;
  static_tracepoint_marker &operator= (static_tracepoint_marker &&) = default;

  struct gdbarch *gdbarch = NULL;
  CORE_ADDR address = 0;

  /* The string ID of the marker.  */
  std::string str_id;

  /* Extra target reported info associated with the marker.  */
  std::string extra;
};

struct memrange
{
  memrange (int type_, bfd_signed_vma start_, bfd_signed_vma end_)
    : type (type_), start (start_), end (end_)
  {}

  memrange ()
  {}

  /* memrange_absolute for absolute memory range, else basereg
     number.  */
  int type;
  bfd_signed_vma start;
  bfd_signed_vma end;
};

class collection_list
{
public:
  collection_list ();

  void add_wholly_collected (const char *print_name);

  void append_exp (std::string &&exp);

  /* Add AEXPR to the list, taking ownership.  */
  void add_aexpr (agent_expr_up aexpr);

  void add_remote_register (unsigned int regno);
  void add_ax_registers (struct agent_expr *aexpr);
  void add_local_register (struct gdbarch *gdbarch,
			   unsigned int regno,
			   CORE_ADDR scope);
  void add_memrange (struct gdbarch *gdbarch,
		     int type, bfd_signed_vma base,
		     unsigned long len, CORE_ADDR scope);
  void collect_symbol (struct symbol *sym,
		       struct gdbarch *gdbarch,
		       long frame_regno, long frame_offset,
		       CORE_ADDR scope,
		       int trace_string);

  void add_local_symbols (struct gdbarch *gdbarch, CORE_ADDR pc,
			  long frame_regno, long frame_offset, int type,
			  int trace_string);
  void add_static_trace_data ();

  void finish ();

  std::vector<std::string> stringify ();

  const std::vector<std::string> &wholly_collected ()
  { return m_wholly_collected; }

  const std::vector<std::string> &computed ()
  { return m_computed; }

private:
  /* We need the allocator zero-initialize the mask, so we don't use
     gdb::byte_vector.  */
  std::vector<unsigned char> m_regs_mask;

  std::vector<memrange> m_memranges;

  std::vector<agent_expr_up> m_aexprs;

  /* True is the user requested a collection of "$_sdata", "static
     tracepoint data".  */
  bool m_strace_data;

  /* A set of names of wholly collected objects.  */
  std::vector<std::string> m_wholly_collected;
  /* A set of computed expressions.  */
  std::vector<std::string> m_computed;
};

extern void
  parse_static_tracepoint_marker_definition (const char *line, const char **pp,
					     static_tracepoint_marker *marker);

/* Returns the current traceframe number.  */
extern int get_traceframe_number (void);

/* Returns the tracepoint number for current traceframe.  */
extern int get_tracepoint_number (void);

/* Make the traceframe NUM be the current trace frame, all the way to
   the target, and flushes all global state (register/frame caches,
   etc.).  */
extern void set_current_traceframe (int num);

struct scoped_restore_current_traceframe
{
  scoped_restore_current_traceframe ();

  ~scoped_restore_current_traceframe ()
  {
    set_current_traceframe (m_traceframe_number);
  }

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_traceframe);

private:

  /* The traceframe we were inspecting.  */
  int m_traceframe_number;
};

extern const char *decode_agent_options (const char *exp, int *trace_string);

extern void encode_actions (struct bp_location *tloc,
			    struct collection_list *tracepoint_list,
			    struct collection_list *stepping_list);

extern void encode_actions_rsp (struct bp_location *tloc,
				std::vector<std::string> *tdp_actions,
				std::vector<std::string> *stepping_actions);

extern void validate_actionline (const char *, tracepoint *);
extern void validate_trace_state_variable_name (const char *name);

extern struct trace_state_variable *find_trace_state_variable (const char *name);
extern struct trace_state_variable *
  find_trace_state_variable_by_number (int number);

extern struct trace_state_variable *create_trace_state_variable (const char *name);

extern int encode_source_string (int num, ULONGEST addr,
				 const char *srctype, const char *src,
				 char *buf, int buf_size);

extern void parse_trace_status (const char *line, struct trace_status *ts);

extern void parse_tracepoint_status (const char *p, tracepoint *tp,
				     struct uploaded_tp *utp);

extern void parse_tracepoint_definition (const char *line,
					 struct uploaded_tp **utpp);
extern void parse_tsv_definition (const char *line, struct uploaded_tsv **utsvp);

extern struct uploaded_tp *get_uploaded_tp (int num, ULONGEST addr,
					    struct uploaded_tp **utpp);
extern void free_uploaded_tps (struct uploaded_tp **utpp);

extern struct uploaded_tsv *get_uploaded_tsv (int num,
					      struct uploaded_tsv **utsvp);
extern void free_uploaded_tsvs (struct uploaded_tsv **utsvp);
extern struct tracepoint *create_tracepoint_from_upload (struct uploaded_tp *utp);
extern void merge_uploaded_tracepoints (struct uploaded_tp **utpp);
extern void merge_uploaded_trace_state_variables (struct uploaded_tsv **utsvp);

extern void query_if_trace_running (int from_tty);
extern void disconnect_tracing (void);
extern void trace_reset_local_state (void);

extern void check_trace_running (struct trace_status *);

extern void start_tracing (const char *notes);
extern void stop_tracing (const char *notes);

extern void trace_status_mi (int on_stop);

extern void tvariables_info_1 (void);
extern void save_trace_state_variables (struct ui_file *fp);

/* Enumeration of the kinds of traceframe searches that a target may
   be able to perform.  */

enum trace_find_type
{
  tfind_number,
  tfind_pc,
  tfind_tp,
  tfind_range,
  tfind_outside,
};

extern void tfind_1 (enum trace_find_type type, int num,
		     CORE_ADDR addr1, CORE_ADDR addr2,
		     int from_tty);

extern void trace_save_tfile (const char *filename,
			      int target_does_save);
extern void trace_save_ctf (const char *dirname,
			    int target_does_save);

extern traceframe_info_up parse_traceframe_info (const char *tframe_info);

extern int traceframe_available_memory (std::vector<mem_range> *result,
					CORE_ADDR memaddr, ULONGEST len);

extern struct traceframe_info *get_traceframe_info (void);

extern struct bp_location *get_traceframe_location (int *stepping_frame_p);

/* Command element for the 'while-stepping' command.  */
extern cmd_list_element *while_stepping_cmd_element;

#endif	/* TRACEPOINT_H */
