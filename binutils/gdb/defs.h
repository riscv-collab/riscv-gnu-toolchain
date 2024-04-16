/* Basic, host-specific, and target-specific definitions for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef DEFS_H
#define DEFS_H

#ifdef GDBSERVER
#  error gdbserver should not include gdb/defs.h
#endif

#include "gdbsupport/common-defs.h"

#undef PACKAGE
#undef PACKAGE_NAME
#undef PACKAGE_VERSION
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

#include <config.h>
#include "bfd.h"

#include <sys/types.h>
#include <climits>

/* The libdecnumber library, on which GDB depends, includes a header file
   called gstdint.h instead of relying directly on stdint.h.  GDB, on the
   other hand, includes stdint.h directly, relying on the fact that gnulib
   generates a copy if the system doesn't provide one or if it is missing
   some features.  Unfortunately, gstdint.h and stdint.h cannot be included
   at the same time, which may happen when we include a file from
   libdecnumber.

   The following macro definition effectively prevents the inclusion of
   gstdint.h, as all the definitions it provides are guarded against
   the GCC_GENERATED_STDINT_H macro.  We already have gnulib/stdint.h
   included, so it's ok to blank out gstdint.h.  */
#define GCC_GENERATED_STDINT_H 1

#include <unistd.h>

#include <fcntl.h>

#include "gdb_wchar.h"

#include "ui-file.h"

#include "gdbsupport/host-defs.h"
#include "gdbsupport/enum-flags.h"
#include "gdbsupport/array-view.h"

/* Scope types enumerator.  List the types of scopes the compiler will
   accept.  */

enum compile_i_scope_types
  {
    COMPILE_I_INVALID_SCOPE,

    /* A simple scope.  Wrap an expression into a simple scope that
       takes no arguments, returns no value, and uses the generic
       function name "_gdb_expr". */

    COMPILE_I_SIMPLE_SCOPE,

    /* Do not wrap the expression,
       it has to provide function "_gdb_expr" on its own.  */
    COMPILE_I_RAW_SCOPE,

    /* A printable expression scope.  Wrap an expression into a scope
       suitable for the "compile print" command.  It uses the generic
       function name "_gdb_expr".  COMPILE_I_PRINT_ADDRESS_SCOPE variant
       is the usual one, taking address of the object.
       COMPILE_I_PRINT_VALUE_SCOPE is needed for arrays where the array
       name already specifies its address.  See get_out_value_type.  */
    COMPILE_I_PRINT_ADDRESS_SCOPE,
    COMPILE_I_PRINT_VALUE_SCOPE,
  };


template<typename T>
using RequireLongest = gdb::Requires<gdb::Or<std::is_same<T, LONGEST>,
					     std::is_same<T, ULONGEST>>>;

/* Just in case they're not defined in stdio.h.  */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

/* The O_BINARY flag is defined in fcntl.h on some non-Posix platforms.
   It is used as an access modifier in calls to open(), where it acts
   similarly to the "b" character in fopen()'s MODE argument.  On Posix
   platforms it should be a no-op, so it is defined as 0 here.  This 
   ensures that the symbol may be used freely elsewhere in gdb.  */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include "hashtab.h"

/* * System root path, used to find libraries etc.  */
extern std::string gdb_sysroot;

/* * GDB datadir, used to store data files.  */
extern std::string gdb_datadir;

/* * If not empty, the possibly relocated path to python's "lib" directory
   specified with --with-python.  */
extern std::string python_libdir;

/* * Search path for separate debug files.  */
extern std::string debug_file_directory;

/* GDB's SIGINT handler basically sets a flag; code that might take a
   long time before it gets back to the event loop, and which ought to
   be interruptible, checks this flag using the QUIT macro, which, if
   GDB has the terminal, throws a quit exception.

   In addition to setting a flag, the SIGINT handler also marks a
   select/poll-able file descriptor as read-ready.  That is used by
   interruptible_select in order to support interrupting blocking I/O
   in a race-free manner.

   These functions use the extension_language_ops API to allow extension
   language(s) and GDB SIGINT handling to coexist seamlessly.  */

/* * Evaluate to non-zero if the quit flag is set, zero otherwise.  This
   will clear the quit flag as a side effect.  */
extern int check_quit_flag (void);
/* * Set the quit flag.  */
extern void set_quit_flag (void);

/* The current quit handler (and its type).  This is called from the
   QUIT macro.  See default_quit_handler below for default behavior.
   Parts of GDB temporarily override this to e.g., completely suppress
   Ctrl-C because it would not be safe to throw.  E.g., normally, you
   wouldn't want to quit between a RSP command and its response, as
   that would break the communication with the target, but you may
   still want to intercept the Ctrl-C and offer to disconnect if the
   user presses Ctrl-C multiple times while the target is stuck
   waiting for the wedged remote stub.  */
typedef void (quit_handler_ftype) (void);
extern quit_handler_ftype *quit_handler;

/* The default quit handler.  Checks whether Ctrl-C was pressed, and
   if so:

     - If GDB owns the terminal, throws a quit exception.

     - If GDB does not own the terminal, forwards the Ctrl-C to the
       target.
*/
extern void default_quit_handler (void);

/* Flag that function quit should call quit_force.  */
extern volatile bool sync_quit_force_run;

/* Set sync_quit_force_run and also call set_quit_flag().  */
extern void set_force_quit_flag ();

extern void quit (void);

/* Helper for the QUIT macro.  */

extern void maybe_quit (void);

/* Check whether a Ctrl-C was typed, and if so, call the current quit
   handler.  */
#define QUIT maybe_quit ()

/* Set the serial event associated with the quit flag.  */
extern void quit_serial_event_set (void);

/* Clear the serial event associated with the quit flag.  */
extern void quit_serial_event_clear (void);

/* * Languages represented in the symbol table and elsewhere.
   This should probably be in language.h, but since enum's can't
   be forward declared to satisfy opaque references before their
   actual definition, needs to be here.

   The constants here are in priority order.  In particular,
   demangling is attempted according to this order.

   Note that there's ambiguity between the mangling schemes of some of
   these languages, so some symbols could be successfully demangled by
   several languages.  For that reason, the constants here are sorted
   in the order we'll attempt demangling them.  For example: Rust uses
   a C++-compatible mangling, so must come before C++; Ada must come
   last (see ada_sniff_from_mangled_name).  */

enum language
  {
    language_unknown,		/* Language not known */
    language_c,			/* C */
    language_objc,		/* Objective-C */
    language_rust,		/* Rust */
    language_cplus,		/* C++ */
    language_d,			/* D */
    language_go,		/* Go */
    language_fortran,		/* Fortran */
    language_m2,		/* Modula-2 */
    language_asm,		/* Assembly language */
    language_pascal,		/* Pascal */
    language_opencl,		/* OpenCL */
    language_minimal,		/* All other languages, minimal support only */
    language_ada,		/* Ada */
    nr_languages
  };

/* The number of bits needed to represent all languages, with enough
   padding to allow for reasonable growth.  */
#define LANGUAGE_BITS 5
static_assert (nr_languages <= (1 << LANGUAGE_BITS));

/* The number of bytes needed to represent all languages.  */
#define LANGUAGE_BYTES ((LANGUAGE_BITS + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT)

enum precision_type
  {
    single_precision,
    double_precision,
    unspecified_precision
  };

/* * A generic, not quite boolean, enumeration.  This is used for
   set/show commands in which the options are on/off/automatic.  */
enum auto_boolean
{
  AUTO_BOOLEAN_TRUE,
  AUTO_BOOLEAN_FALSE,
  AUTO_BOOLEAN_AUTO
};

/* * Potential ways that a function can return a value of a given
   type.  */

enum return_value_convention
{
  /* * Where the return value has been squeezed into one or more
     registers.  */
  RETURN_VALUE_REGISTER_CONVENTION,
  /* * Commonly known as the "struct return convention".  The caller
     passes an additional hidden first parameter to the caller.  That
     parameter contains the address at which the value being returned
     should be stored.  While typically, and historically, used for
     large structs, this is convention is applied to values of many
     different types.  */
  RETURN_VALUE_STRUCT_CONVENTION,
  /* * Like the "struct return convention" above, but where the ABI
     guarantees that the called function stores the address at which
     the value being returned is stored in a well-defined location,
     such as a register or memory slot in the stack frame.  Don't use
     this if the ABI doesn't explicitly guarantees this.  */
  RETURN_VALUE_ABI_RETURNS_ADDRESS,
  /* * Like the "struct return convention" above, but where the ABI
     guarantees that the address at which the value being returned is
     stored will be available in a well-defined location, such as a
     register or memory slot in the stack frame.  Don't use this if
     the ABI doesn't explicitly guarantees this.  */
  RETURN_VALUE_ABI_PRESERVES_ADDRESS,
};

/* Needed for various prototypes */

struct symtab;
struct breakpoint;
class frame_info_ptr;
struct gdbarch;
struct value;

/* From main.c.  */

/* This really belong in utils.c (path-utils.c?), but it references some
   globals that are currently only available to main.c.  */
extern std::string relocate_gdb_directory (const char *initial, bool relocatable);


/* Annotation stuff.  */

extern int annotation_level;	/* in stack.c */


/* From symfile.c */

extern void symbol_file_command (const char *, int);

/* From top.c */

typedef void initialize_file_ftype (void);

extern char *gdb_readline_wrapper (const char *);

extern const char *command_line_input (std::string &cmd_line_buffer,
				       const char *, const char *);

extern void print_prompt (void);

struct ui;

extern bool info_verbose;

/* From printcmd.c */

extern void set_next_address (struct gdbarch *, CORE_ADDR);

extern int print_address_symbolic (struct gdbarch *, CORE_ADDR,
				   struct ui_file *, int,
				   const char *);

extern void print_address (struct gdbarch *, CORE_ADDR, struct ui_file *);
extern const char *pc_prefix (CORE_ADDR);

/* From exec.c */

/* * Process memory area starting at ADDR with length SIZE.  Area is
   readable iff READ is non-zero, writable if WRITE is non-zero,
   executable if EXEC is non-zero.  Area is possibly changed against
   its original file based copy if MODIFIED is non-zero.

   MEMORY_TAGGED is true if the memory region contains memory tags, false
   otherwise.

   DATA is passed without changes from a caller.  */

typedef int (*find_memory_region_ftype) (CORE_ADDR addr, unsigned long size,
					 int read, int write, int exec,
					 int modified, bool memory_tagged,
					 void *data);

/* * Possible lvalue types.  Like enum language, this should be in
   value.h, but needs to be here for the same reason.  */

enum lval_type
  {
    /* * Not an lval.  */
    not_lval,
    /* * In memory.  */
    lval_memory,
    /* * In a register.  Registers are relative to a frame.  */
    lval_register,
    /* * In a gdb internal variable.  */
    lval_internalvar,
    /* * Value encapsulates a callable defined in an extension language.  */
    lval_xcallable,
    /* * Part of a gdb internal variable (structure field).  */
    lval_internalvar_component,
    /* * Value's bits are fetched and stored using functions provided
       by its creator.  */
    lval_computed
  };

/* * Parameters of the "info proc" command.  */

enum info_proc_what
  {
    /* * Display the default cmdline, cwd and exe outputs.  */
    IP_MINIMAL,

    /* * Display `info proc mappings'.  */
    IP_MAPPINGS,

    /* * Display `info proc status'.  */
    IP_STATUS,

    /* * Display `info proc stat'.  */
    IP_STAT,

    /* * Display `info proc cmdline'.  */
    IP_CMDLINE,

    /* * Display `info proc exe'.  */
    IP_EXE,

    /* * Display `info proc cwd'.  */
    IP_CWD,

    /* * Display `info proc files'.  */
    IP_FILES,

    /* * Display all of the above.  */
    IP_ALL
  };

/* * Default radixes for input and output.  Only some values supported.  */
extern unsigned input_radix;
extern unsigned output_radix;

/* * Optional native machine support.  Non-native (and possibly pure
   multi-arch) targets do not need a "nm.h" file.  This will be a
   symlink to one of the nm-*.h files, built by the `configure'
   script.  */

#ifdef GDB_NM_FILE
#include "nm.h"
#endif

/* Assume that fopen accepts the letter "b" in the mode string.
   It is demanded by ISO C9X, and should be supported on all
   platforms that claim to have a standard-conforming C library.  On
   true POSIX systems it will be ignored and have no effect.  There
   may still be systems without a standard-conforming C library where
   an ISO C9X compiler (GCC) is available.  Known examples are SunOS
   4.x and 4.3BSD.  This assumption means these systems are no longer
   supported.  */
#ifndef FOPEN_RB
# include "fopen-bin.h"
#endif

/* * Convert a LONGEST to an int.  This is used in contexts (e.g. number of
   arguments to a function, number in a value history, register number, etc.)
   where the value must not be larger than can fit in an int.  */

extern int longest_to_int (LONGEST);

/* Enumerate the requirements a symbol has in order to be evaluated.
   These are listed in order of "strength" -- a later entry subsumes
   earlier ones.  This fine-grained distinction is important because
   it allows for the evaluation of a TLS symbol during unwinding --
   when unwinding one has access to registers, but not the frame
   itself, because that is being constructed.  */

enum symbol_needs_kind
{
  /* No special requirements -- just memory.  */
  SYMBOL_NEEDS_NONE,

  /* The symbol needs registers.  */
  SYMBOL_NEEDS_REGISTERS,

  /* The symbol needs a frame.  */
  SYMBOL_NEEDS_FRAME
};

/* In findvar.c.  */

template<typename T, typename = RequireLongest<T>>
T extract_integer (gdb::array_view<const gdb_byte>, enum bfd_endian byte_order);

static inline LONGEST
extract_signed_integer (gdb::array_view<const gdb_byte> buf,
			enum bfd_endian byte_order)
{
  return extract_integer<LONGEST> (buf, byte_order);
}

static inline LONGEST
extract_signed_integer (const gdb_byte *addr, int len,
			enum bfd_endian byte_order)
{
  return extract_signed_integer (gdb::array_view<const gdb_byte> (addr, len),
				 byte_order);
}

static inline ULONGEST
extract_unsigned_integer (gdb::array_view<const gdb_byte> buf,
			  enum bfd_endian byte_order)
{
  return extract_integer<ULONGEST> (buf, byte_order);
}

static inline ULONGEST
extract_unsigned_integer (const gdb_byte *addr, int len,
			  enum bfd_endian byte_order)
{
  return extract_unsigned_integer (gdb::array_view<const gdb_byte> (addr, len),
				   byte_order);
}

extern int extract_long_unsigned_integer (const gdb_byte *, int,
					  enum bfd_endian, LONGEST *);

extern CORE_ADDR extract_typed_address (const gdb_byte *buf,
					struct type *type);

/* All 'store' functions accept a host-format integer and store a
   target-format integer at ADDR which is LEN bytes long.  */

template<typename T, typename = RequireLongest<T>>
extern void store_integer (gdb::array_view<gdb_byte> dst,
			   bfd_endian byte_order, T val);

template<typename T>
static inline void
store_integer (gdb_byte *addr, int len, bfd_endian byte_order, T val)
{
  return store_integer (gdb::make_array_view (addr, len), byte_order, val);
}

static inline void
store_signed_integer (gdb::array_view<gdb_byte> dst, bfd_endian byte_order,
		      LONGEST val)
{
  return store_integer (dst, byte_order, val);
}

static inline void
store_signed_integer (gdb_byte *addr, int len, bfd_endian byte_order,
		      LONGEST val)
{
  return store_signed_integer (gdb::make_array_view (addr, len), byte_order,
			       val);
}

static inline void
store_unsigned_integer (gdb::array_view<gdb_byte> dst, bfd_endian byte_order,
			ULONGEST val)
{
  return store_integer (dst, byte_order, val);
}

static inline void
store_unsigned_integer (gdb_byte *addr, int len, bfd_endian byte_order,
			ULONGEST val)
{
  return store_unsigned_integer (gdb::make_array_view (addr, len), byte_order,
				 val);
}

extern void store_typed_address (gdb_byte *buf, struct type *type,
				 CORE_ADDR addr);

extern void copy_integer_to_size (gdb_byte *dest, int dest_size,
				  const gdb_byte *source, int source_size,
				  bool is_signed, enum bfd_endian byte_order);

/* Hooks for alternate command interfaces.  */

struct target_waitstatus;
struct cmd_list_element;

extern void (*deprecated_pre_add_symbol_hook) (const char *);
extern void (*deprecated_post_add_symbol_hook) (void);
extern void (*selected_frame_level_changed_hook) (int);
extern int (*deprecated_ui_loop_hook) (int signo);
extern void (*deprecated_show_load_progress) (const char *section,
					      unsigned long section_sent, 
					      unsigned long section_size, 
					      unsigned long total_sent, 
					      unsigned long total_size);
extern void (*deprecated_print_frame_info_listing_hook) (struct symtab * s,
							 int line,
							 int stopline,
							 int noerror);
extern int (*deprecated_query_hook) (const char *, va_list)
     ATTRIBUTE_FPTR_PRINTF(1,0);
extern thread_local void (*deprecated_warning_hook) (const char *, va_list)
     ATTRIBUTE_FPTR_PRINTF(1,0);
extern void (*deprecated_readline_begin_hook) (const char *, ...)
     ATTRIBUTE_FPTR_PRINTF_1;
extern char *(*deprecated_readline_hook) (const char *);
extern void (*deprecated_readline_end_hook) (void);
extern void (*deprecated_context_hook) (int);
extern ptid_t (*deprecated_target_wait_hook) (ptid_t ptid,
					      struct target_waitstatus *status,
					      int options);

extern void (*deprecated_attach_hook) (void);
extern void (*deprecated_detach_hook) (void);
extern void (*deprecated_call_command_hook) (struct cmd_list_element * c,
					     const char *cmd, int from_tty);

extern int (*deprecated_ui_load_progress_hook) (const char *section,
						unsigned long num);

/* If this definition isn't overridden by the header files, assume
   that isatty and fileno exist on this system.  */
#ifndef ISATTY
#define ISATTY(FP)	(isatty (fileno (FP)))
#endif

/* * A width that can achieve a better legibility for GDB MI mode.  */
#define GDB_MI_MSG_WIDTH  80

/* From progspace.c */

extern void initialize_progspace (void);
extern void initialize_inferiors (void);

/* * Special block numbers */

enum block_enum
{
  GLOBAL_BLOCK = 0,
  STATIC_BLOCK = 1,
  FIRST_LOCAL_BLOCK = 2
};

/* User selection used in observable.h and multiple print functions.  */

enum user_selected_what_flag
  {
    /* Inferior selected.  */
    USER_SELECTED_INFERIOR = 1 << 1,

    /* Thread selected.  */
    USER_SELECTED_THREAD = 1 << 2,

    /* Frame selected.  */
    USER_SELECTED_FRAME = 1 << 3
  };
DEF_ENUM_FLAGS_TYPE (enum user_selected_what_flag, user_selected_what);

#include "utils.h"

#endif /* #ifndef DEFS_H */
