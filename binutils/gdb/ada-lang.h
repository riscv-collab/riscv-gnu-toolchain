/* Ada language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#if !defined (ADA_LANG_H)
#define ADA_LANG_H 1

class frame_info_ptr;
struct inferior;
struct type_print_options;
struct parser_state;

#include "value.h"
#include "gdbtypes.h"
#include "breakpoint.h"

/* Names of specific files known to be part of the runtime
   system and that might consider (confusing) debugging information.
   Each name (a basic regular expression string) is followed by a
   comma.  FIXME: Should be part of a configuration file.  */
#if defined (__linux__)
#define ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS \
   "^[agis]-.*\\.ad[bs]$", \
   "/lib.*/libpthread\\.so[.0-9]*$", "/lib.*/libpthread\\.a$", \
   "/lib.*/libc\\.so[.0-9]*$", "/lib.*/libc\\.a$",
#endif

#if !defined (ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS)
#define ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS \
   "^unwind-seh.c$", \
   "^[agis]-.*\\.ad[bs]$",
#endif

/* Names of compiler-generated auxiliary functions probably of no
   interest to users.  Each name (a basic regular expression string)
   is followed by a comma.  */
#define ADA_KNOWN_AUXILIARY_FUNCTION_NAME_PATTERNS \
   "___clean[.$a-zA-Z0-9_]*$", \
   "___finalizer[.$a-zA-Z0-9_]*$",

/* The maximum number of frame levels searched for non-local,
   non-global symbols.  This limit exists as a precaution to prevent
   infinite search loops when the stack is screwed up.  */
#define MAX_ENCLOSING_FRAME_LEVELS 7

/* Maximum number of steps followed in looking for the ultimate
   referent of a renaming.  This prevents certain infinite loops that
   can otherwise result.  */
#define MAX_RENAMING_CHAIN_LENGTH 10

struct block;

/* Corresponding encoded/decoded names and opcodes for Ada user-definable
   operators.  */
struct ada_opname_map
{
  const char *encoded;
  const char *decoded;
  enum exp_opcode op;
};

/* Table of Ada operators in encoded and decoded forms.  */
/* Defined in ada-lang.c */
extern const struct ada_opname_map ada_opname_table[];

/* Denotes a type of renaming symbol (see ada_parse_renaming).  */
enum ada_renaming_category
  {
    /* Indicates a symbol that does not encode a renaming.  */
    ADA_NOT_RENAMING,

    /* For symbols declared
	 Foo : TYPE renamed OBJECT;  */
    ADA_OBJECT_RENAMING,

    /* For symbols declared
	 Foo : exception renames EXCEPTION;  */
    ADA_EXCEPTION_RENAMING,
    /* For packages declared
	  package Foo renames PACKAGE; */
    ADA_PACKAGE_RENAMING,
    /* For subprograms declared
	  SUBPROGRAM_SPEC renames SUBPROGRAM;
       (Currently not used).  */
    ADA_SUBPROGRAM_RENAMING
  };

/* The different types of catchpoints that we introduced for catching
   Ada exceptions.  */

enum ada_exception_catchpoint_kind
{
  ada_catch_exception,
  ada_catch_exception_unhandled,
  ada_catch_assert,
  ada_catch_handlers
};

/* Ada task structures.  */

struct ada_task_info
{
  /* The PTID of the thread that this task runs on.  This ptid is computed
     in a target-dependent way from the associated Task Control Block.  */
  ptid_t ptid;

  /* The ID of the task.  */
  CORE_ADDR task_id;

  /* The name of the task.  */
  char name[257];

  /* The current state of the task.  */
  int state;

  /* The priority associated to the task.  */
  int priority;

  /* If non-zero, the task ID of the parent task.  */
  CORE_ADDR parent;

  /* If the task is waiting on a task entry, this field contains
     the ID of the other task.  Zero otherwise.  */
  CORE_ADDR called_task;

  /* If the task is accepting a rendezvous with another task, this field
     contains the ID of the calling task.  Zero otherwise.  */
  CORE_ADDR caller_task;

  /* The CPU on which the task is running.  This is dependent on
     the runtime actually providing that info, which is not always
     the case.  Normally, we should be able to count on it on
     bare-metal targets.  */
  int base_cpu;
};

extern int ada_get_field_index (const struct type *type,
				const char *field_name,
				int maybe_missing);

extern int ada_parse (struct parser_state *);    /* Defined in ada-exp.y */

			/* Defined in ada-typeprint.c */
extern void ada_print_type (struct type *, const char *, struct ui_file *, int,
			    int, const struct type_print_options *);

extern void ada_print_typedef (struct type *type, struct symbol *new_symbol,
			       struct ui_file *stream);

/* Implement la_value_print_inner for Ada.  */

extern void ada_value_print_inner (struct value *, struct ui_file *, int,
				   const struct value_print_options *);

extern void ada_value_print (struct value *, struct ui_file *,
			     const struct value_print_options *);

				/* Defined in ada-lang.c */

extern void ada_emit_char (int, struct type *, struct ui_file *, int, int);

extern void ada_printchar (int, struct type *, struct ui_file *);

extern void ada_printstr (struct ui_file *, struct type *, const gdb_byte *,
			  unsigned int, const char *, int,
			  const struct value_print_options *);

struct value *ada_convert_actual (struct value *actual,
				  struct type *formal_type0);

extern bool ada_is_access_to_unconstrained_array (struct type *type);

extern struct value *ada_value_subscript (struct value *, int,
					  struct value **);

extern void ada_fixup_array_indexes_type (struct type *index_desc_type);

extern struct type *ada_array_element_type (struct type *, int);

extern int ada_array_arity (struct type *);

extern struct value *ada_coerce_to_simple_array_ptr (struct value *);

struct value *ada_coerce_to_simple_array (struct value *);

extern int ada_is_simple_array_type (struct type *);

extern int ada_is_array_descriptor_type (struct type *);

extern LONGEST ada_discrete_type_low_bound (struct type *);

extern LONGEST ada_discrete_type_high_bound (struct type *);

extern struct value *ada_get_decoded_value (struct value *value);

extern struct type *ada_get_decoded_type (struct type *type);

extern const char *ada_decode_symbol (const struct general_symbol_info *);

/* Decode the GNAT-encoded name NAME, returning the decoded name.  If
   the name does not appear to be GNAT-encoded, then the result
   depends on WRAP.  If WRAP is true (the default), then the result is
   simply wrapped in <...>.  If WRAP is false, then the empty string
   will be returned.

   When OPERATORS is false, operator names will not be decoded.  By
   default, they are decoded, e.g., 'Oadd' will be transformed to
   '"+"'.

   When WIDE is false, wide characters will be left as-is.  By
   default, they converted from their hex encoding to the host
   charset.  */
extern std::string ada_decode (const char *name, bool wrap = true,
			       bool operators = true,
			       bool wide = true);

extern std::vector<struct block_symbol> ada_lookup_symbol_list
     (const char *, const struct block *, domain_enum);

extern struct block_symbol ada_lookup_symbol (const char *,
					      const struct block *,
					      domain_enum);

extern void ada_lookup_encoded_symbol
  (const char *name, const struct block *block, domain_enum domain,
   struct block_symbol *symbol_info);

extern struct bound_minimal_symbol ada_lookup_simple_minsym (const char *,
							     objfile *);

extern int ada_scan_number (const char *, int, LONGEST *, int *);

extern struct value *ada_value_primitive_field (struct value *arg1,
						int offset,
						int fieldno,
						struct type *arg_type);

extern struct type *ada_parent_type (struct type *);

extern int ada_is_ignored_field (struct type *, int);

extern int ada_is_constrained_packed_array_type (struct type *);

extern struct value *ada_value_primitive_packed_val (struct value *,
						     const gdb_byte *,
						     long, int, int,
						     struct type *);

extern struct type *ada_coerce_to_simple_array_type (struct type *);

extern bool ada_is_character_type (struct type *);

extern bool ada_is_string_type (struct type *);

extern int ada_is_tagged_type (struct type *, int);

extern int ada_is_tag_type (struct type *);

extern gdb::unique_xmalloc_ptr<char> ada_tag_name (struct value *);

extern struct value *ada_tag_value_at_base_address (struct value *obj);

extern int ada_is_parent_field (struct type *, int);

extern int ada_is_wrapper_field (struct type *, int);

extern int ada_is_variant_part (struct type *, int);

extern struct type *ada_variant_discrim_type (struct type *, struct type *);

extern const char *ada_variant_discrim_name (struct type *);

extern int ada_is_aligner_type (struct type *);

extern struct type *ada_aligned_type (struct type *);

extern const gdb_byte *ada_aligned_value_addr (struct type *,
					       const gdb_byte *);

extern int ada_is_system_address_type (struct type *);

extern int ada_which_variant_applies (struct type *, struct value *);

extern struct type *ada_to_fixed_type (struct type *, const gdb_byte *,
				       CORE_ADDR, struct value *,
				       int check_tag);

extern struct value *ada_to_fixed_value (struct value *val);

extern struct type *ada_template_to_fixed_record_type_1 (struct type *type,
							 const gdb_byte *valaddr,
							 CORE_ADDR address,
							 struct value *dval0,
							 int keep_dynamic_fields);

extern int ada_name_prefix_len (const char *);

extern const char *ada_type_name (struct type *);

extern struct type *ada_find_parallel_type (struct type *,
					    const char *suffix);

extern bool get_int_var_value (const char *, LONGEST &value);

extern int ada_prefer_type (struct type *, struct type *);

extern struct type *ada_get_base_type (struct type *);

extern struct type *ada_check_typedef (struct type *);

extern std::string ada_encode (const char *, bool fold = true);

extern const char *ada_enum_name (const char *);

extern int ada_is_modular_type (struct type *);

extern ULONGEST ada_modulus (struct type *);

extern struct value *ada_value_ind (struct value *);

extern void ada_print_scalar (struct type *, LONGEST, struct ui_file *);

extern int ada_is_range_type_name (const char *);

extern enum ada_renaming_category ada_parse_renaming (struct symbol *,
						      const char **,
						      int *, const char **);

extern void ada_find_printable_frame (frame_info_ptr fi);

extern const char *ada_main_name ();

extern void create_ada_exception_catchpoint
  (struct gdbarch *gdbarch, enum ada_exception_catchpoint_kind ex_kind,
   std::string &&excep_string, const std::string &cond_string, int tempflag,
   int enabled, int from_tty);

/* Return true if BP is an Ada catchpoint.  */

extern bool is_ada_exception_catchpoint (breakpoint *bp);

/* Some information about a given Ada exception.  */

struct ada_exc_info
{
  /* The name of the exception.  */
  const char *name;

  /* The address of the symbol corresponding to that exception.  */
  CORE_ADDR addr;

  bool operator< (const ada_exc_info &) const;
  bool operator== (const ada_exc_info &) const;
};

extern std::vector<ada_exc_info> ada_exceptions_list (const char *regexp);

/* Tasking-related: ada-tasks.c */

extern int valid_task_id (int);

extern struct ada_task_info *ada_get_task_info_from_ptid (ptid_t ptid);

extern int ada_get_task_number (thread_info *thread);

typedef gdb::function_view<void (struct ada_task_info *task)>
  ada_task_list_iterator_ftype;
extern void iterate_over_live_ada_tasks
  (ada_task_list_iterator_ftype iterator);

extern const char *ada_get_tcb_types_info (void);

extern void print_ada_task_info (struct ui_out *uiout,
				 const char *taskno_str,
				 struct inferior *inf);

/* Look for a symbol for an overloaded operator for the operation OP.
   PARSE_COMPLETION is true if currently parsing for completion.
   NARGS and ARGVEC describe the arguments to the call.  Returns a
   "null" block_symbol if no such operator is found.  */

extern block_symbol ada_find_operator_symbol (enum exp_opcode op,
					      bool parse_completion,
					      int nargs, value *argvec[]);

/* Resolve a function call, selecting among possible function symbols.
   SYM and BLOCK are passed to ada_lookup_symbol_list.  CONTEXT_TYPE
   describes the calling context.  PARSE_COMPLETION is true if
   currently parsing for completion.  NARGS and ARGVEC describe the
   arguments to the call.  This returns the chosen symbol and will
   update TRACKER accordingly.  */

extern block_symbol ada_resolve_funcall (struct symbol *sym,
					 const struct block *block,
					 struct type *context_type,
					 bool parse_completion,
					 int nargs, value *argvec[],
					 innermost_block_tracker *tracker);

/* Resolve a symbol reference, selecting among possible values.  SYM
   and BLOCK are passed to ada_lookup_symbol_list.  CONTEXT_TYPE
   describes the calling context.  PARSE_COMPLETION is true if
   currently parsing for completion.  If DEPROCEDURE_P is nonzero,
   then a symbol that names a zero-argument function will be passed
   through ada_resolve_function.  This returns the chosen symbol and
   will update TRACKER accordingly.  */

extern block_symbol ada_resolve_variable (struct symbol *sym,
					  const struct block *block,
					  struct type *context_type,
					  bool parse_completion,
					  int deprocedure_p,
					  innermost_block_tracker *tracker);

/* The type of nth index in arrays of given type (n numbering from 1).
   Does not examine memory.  Throws an error if N is invalid or TYPE
   is not an array type.  NAME is the name of the Ada attribute being
   evaluated ('range, 'first, 'last, or 'length); it is used in building
   the error message.  */
extern struct type *ada_index_type (struct type *type, int n,
				    const char *name);

#endif
