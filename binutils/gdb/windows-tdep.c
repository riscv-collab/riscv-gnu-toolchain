/* Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "windows-tdep.h"
#include "gdbsupport/gdb_obstack.h"
#include "xml-support.h"
#include "gdbarch.h"
#include "target.h"
#include "value.h"
#include "inferior.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "objfiles.h"
#include "symfile.h"
#include "coff-pe-read.h"
#include "gdb_bfd.h"
#include "solib.h"
#include "solib-target.h"
#include "gdbcore.h"
#include "coff/internal.h"
#include "libcoff.h"
#include "solist.h"

#define CYGWIN_DLL_NAME "cygwin1.dll"

/* Windows signal numbers differ between MinGW flavors and between
   those and Cygwin.  The below enumerations were gleaned from the
   respective headers.  */

/* Signal numbers for the various MinGW flavors.  The ones marked with
   MinGW-w64 are defined by MinGW-w64, not by mingw.org's MinGW.  */

enum
{
  WINDOWS_SIGHUP = 1,	/* MinGW-w64 */
  WINDOWS_SIGINT = 2,
  WINDOWS_SIGQUIT = 3,	/* MinGW-w64 */
  WINDOWS_SIGILL = 4,
  WINDOWS_SIGTRAP = 5,	/* MinGW-w64 */
  WINDOWS_SIGIOT = 6,	/* MinGW-w64 */
  WINDOWS_SIGEMT = 7,	/* MinGW-w64 */
  WINDOWS_SIGFPE = 8,
  WINDOWS_SIGKILL = 9,	/* MinGW-w64 */
  WINDOWS_SIGBUS = 10,	/* MinGW-w64 */
  WINDOWS_SIGSEGV = 11,
  WINDOWS_SIGSYS = 12,	/* MinGW-w64 */
  WINDOWS_SIGPIPE = 13,	/* MinGW-w64 */
  WINDOWS_SIGALRM = 14,	/* MinGW-w64 */
  WINDOWS_SIGTERM = 15,
  WINDOWS_SIGBREAK = 21,
  WINDOWS_SIGABRT = 22,
};

/* Signal numbers for Cygwin.  */

enum
{
  CYGWIN_SIGHUP = 1,
  CYGWIN_SIGINT = 2,
  CYGWIN_SIGQUIT = 3,
  CYGWIN_SIGILL = 4,
  CYGWIN_SIGTRAP = 5,
  CYGWIN_SIGABRT = 6,
  CYGWIN_SIGEMT = 7,
  CYGWIN_SIGFPE = 8,
  CYGWIN_SIGKILL = 9,
  CYGWIN_SIGBUS = 10,
  CYGWIN_SIGSEGV = 11,
  CYGWIN_SIGSYS = 12,
  CYGWIN_SIGPIPE = 13,
  CYGWIN_SIGALRM = 14,
  CYGWIN_SIGTERM = 15,
  CYGWIN_SIGURG = 16,
  CYGWIN_SIGSTOP = 17,
  CYGWIN_SIGTSTP = 18,
  CYGWIN_SIGCONT = 19,
  CYGWIN_SIGCHLD = 20,
  CYGWIN_SIGTTIN = 21,
  CYGWIN_SIGTTOU = 22,
  CYGWIN_SIGIO = 23,
  CYGWIN_SIGXCPU = 24,
  CYGWIN_SIGXFSZ = 25,
  CYGWIN_SIGVTALRM = 26,
  CYGWIN_SIGPROF = 27,
  CYGWIN_SIGWINCH = 28,
  CYGWIN_SIGLOST = 29,
  CYGWIN_SIGUSR1 = 30,
  CYGWIN_SIGUSR2 = 31,
};

/* These constants are defined by Cygwin's core_dump.h */
static constexpr unsigned int NOTE_INFO_MODULE = 3;
static constexpr unsigned int NOTE_INFO_MODULE64 = 4;

struct cmd_list_element *info_w32_cmdlist;

typedef struct thread_information_block_32
  {
    uint32_t current_seh;			/* %fs:0x0000 */
    uint32_t current_top_of_stack; 		/* %fs:0x0004 */
    uint32_t current_bottom_of_stack;		/* %fs:0x0008 */
    uint32_t sub_system_tib;			/* %fs:0x000c */
    uint32_t fiber_data;			/* %fs:0x0010 */
    uint32_t arbitrary_data_slot;		/* %fs:0x0014 */
    uint32_t linear_address_tib;		/* %fs:0x0018 */
    uint32_t environment_pointer;		/* %fs:0x001c */
    uint32_t process_id;			/* %fs:0x0020 */
    uint32_t current_thread_id;			/* %fs:0x0024 */
    uint32_t active_rpc_handle;			/* %fs:0x0028 */
    uint32_t thread_local_storage;		/* %fs:0x002c */
    uint32_t process_environment_block;		/* %fs:0x0030 */
    uint32_t last_error_number;			/* %fs:0x0034 */
  }
thread_information_32;

typedef struct thread_information_block_64
  {
    uint64_t current_seh;			/* %gs:0x0000 */
    uint64_t current_top_of_stack; 		/* %gs:0x0008 */
    uint64_t current_bottom_of_stack;		/* %gs:0x0010 */
    uint64_t sub_system_tib;			/* %gs:0x0018 */
    uint64_t fiber_data;			/* %gs:0x0020 */
    uint64_t arbitrary_data_slot;		/* %gs:0x0028 */
    uint64_t linear_address_tib;		/* %gs:0x0030 */
    uint64_t environment_pointer;		/* %gs:0x0038 */
    uint64_t process_id;			/* %gs:0x0040 */
    uint64_t current_thread_id;			/* %gs:0x0048 */
    uint64_t active_rpc_handle;			/* %gs:0x0050 */
    uint64_t thread_local_storage;		/* %gs:0x0058 */
    uint64_t process_environment_block;		/* %gs:0x0060 */
    uint64_t last_error_number;			/* %gs:0x0068 */
  }
thread_information_64;


static const char* TIB_NAME[] =
  {
    " current_seh                 ",	/* %fs:0x0000 */
    " current_top_of_stack        ", 	/* %fs:0x0004 */
    " current_bottom_of_stack     ",	/* %fs:0x0008 */
    " sub_system_tib              ",	/* %fs:0x000c */
    " fiber_data                  ",	/* %fs:0x0010 */
    " arbitrary_data_slot         ",	/* %fs:0x0014 */
    " linear_address_tib          ",	/* %fs:0x0018 */
    " environment_pointer         ",	/* %fs:0x001c */
    " process_id                  ",	/* %fs:0x0020 */
    " current_thread_id           ",	/* %fs:0x0024 */
    " active_rpc_handle           ",	/* %fs:0x0028 */
    " thread_local_storage        ",	/* %fs:0x002c */
    " process_environment_block   ",	/* %fs:0x0030 */
    " last_error_number           "	/* %fs:0x0034 */
  };

static const int MAX_TIB32 =
  sizeof (thread_information_32) / sizeof (uint32_t);
static const int MAX_TIB64 =
  sizeof (thread_information_64) / sizeof (uint64_t);
static const int FULL_TIB_SIZE = 0x1000;

static bool maint_display_all_tib = false;

struct windows_gdbarch_data
{
  struct type *siginfo_type = nullptr;
  /* Type of thread information block.  */
  struct type *tib_ptr_type = nullptr;
};

static const registry<gdbarch>::key<windows_gdbarch_data>
     windows_gdbarch_data_handle;

/* Get windows_gdbarch_data of an arch.  */

static struct windows_gdbarch_data *
get_windows_gdbarch_data (struct gdbarch *gdbarch)
{
  windows_gdbarch_data *result = windows_gdbarch_data_handle.get (gdbarch);
  if (result == nullptr)
    result = windows_gdbarch_data_handle.emplace (gdbarch);
  return result;
}

/* Define Thread Local Base pointer type.  */

static struct type *
windows_get_tlb_type (struct gdbarch *gdbarch)
{
  struct type *dword_ptr_type, *dword32_type, *void_ptr_type;
  struct type *peb_ldr_type, *peb_ldr_ptr_type;
  struct type *peb_type, *peb_ptr_type, *list_type;
  struct type *module_list_ptr_type;
  struct type *tib_type, *seh_type, *tib_ptr_type, *seh_ptr_type;
  struct type *word_type, *wchar_type, *wchar_ptr_type;
  struct type *uni_str_type, *rupp_type, *rupp_ptr_type;

  windows_gdbarch_data *windows_gdbarch_data
    = get_windows_gdbarch_data (gdbarch);
  if (windows_gdbarch_data->tib_ptr_type != nullptr)
    return windows_gdbarch_data->tib_ptr_type;

  type_allocator alloc (gdbarch);

  dword_ptr_type = init_integer_type (alloc, gdbarch_ptr_bit (gdbarch),
				 1, "DWORD_PTR");
  dword32_type = init_integer_type (alloc, 32,
				 1, "DWORD32");
  word_type = init_integer_type (alloc, 16,
				 1, "WORD");
  wchar_type = init_integer_type (alloc, 16,
				  1, "wchar_t");
  void_ptr_type = lookup_pointer_type (builtin_type (gdbarch)->builtin_void);
  wchar_ptr_type = init_pointer_type (alloc, gdbarch_ptr_bit (gdbarch),
				      nullptr, wchar_type);

  /* list entry */

  list_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  list_type->set_name (xstrdup ("list"));

  module_list_ptr_type = void_ptr_type;

  append_composite_type_field (list_type, "forward_list",
			       module_list_ptr_type);
  append_composite_type_field (list_type, "backward_list",
			       module_list_ptr_type);

  /* Structured Exception Handler */

  seh_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  seh_type->set_name (xstrdup ("seh"));

  seh_ptr_type = alloc.new_type (TYPE_CODE_PTR,
				 void_ptr_type->length () * TARGET_CHAR_BIT,
				 NULL);
  seh_ptr_type->set_target_type (seh_type);

  append_composite_type_field (seh_type, "next_seh", seh_ptr_type);
  append_composite_type_field (seh_type, "handler",
			       builtin_type (gdbarch)->builtin_func_ptr);

  /* struct _PEB_LDR_DATA */
  peb_ldr_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  peb_ldr_type->set_name (xstrdup ("peb_ldr_data"));

  append_composite_type_field (peb_ldr_type, "length", dword32_type);
  append_composite_type_field (peb_ldr_type, "initialized", dword32_type);
  append_composite_type_field (peb_ldr_type, "ss_handle", void_ptr_type);
  append_composite_type_field (peb_ldr_type, "in_load_order", list_type);
  append_composite_type_field (peb_ldr_type, "in_memory_order", list_type);
  append_composite_type_field (peb_ldr_type, "in_init_order", list_type);
  append_composite_type_field (peb_ldr_type, "entry_in_progress",
			       void_ptr_type);
  peb_ldr_ptr_type = alloc.new_type (TYPE_CODE_PTR,
				     void_ptr_type->length () * TARGET_CHAR_BIT,
				     NULL);
  peb_ldr_ptr_type->set_target_type (peb_ldr_type);

  /* struct UNICODE_STRING */
  uni_str_type = arch_composite_type (gdbarch, "unicode_string",
				      TYPE_CODE_STRUCT);

  append_composite_type_field (uni_str_type, "length", word_type);
  append_composite_type_field (uni_str_type, "maximum_length", word_type);
  append_composite_type_field_aligned (uni_str_type, "buffer",
				       wchar_ptr_type,
				       wchar_ptr_type->length ());

  /* struct _RTL_USER_PROCESS_PARAMETERS */
  rupp_type = arch_composite_type (gdbarch, "rtl_user_process_parameters",
				   TYPE_CODE_STRUCT);

  append_composite_type_field (rupp_type, "maximum_length", dword32_type);
  append_composite_type_field (rupp_type, "length", dword32_type);
  append_composite_type_field (rupp_type, "flags", dword32_type);
  append_composite_type_field (rupp_type, "debug_flags", dword32_type);
  append_composite_type_field (rupp_type, "console_handle", void_ptr_type);
  append_composite_type_field (rupp_type, "console_flags", dword32_type);
  append_composite_type_field_aligned (rupp_type, "standard_input",
				       void_ptr_type,
				       void_ptr_type->length ());
  append_composite_type_field (rupp_type, "standard_output", void_ptr_type);
  append_composite_type_field (rupp_type, "standard_error", void_ptr_type);
  append_composite_type_field (rupp_type, "current_directory", uni_str_type);
  append_composite_type_field (rupp_type, "current_directory_handle",
			       void_ptr_type);
  append_composite_type_field (rupp_type, "dll_path", uni_str_type);
  append_composite_type_field (rupp_type, "image_path_name", uni_str_type);
  append_composite_type_field (rupp_type, "command_line", uni_str_type);
  append_composite_type_field (rupp_type, "environment", void_ptr_type);
  append_composite_type_field (rupp_type, "starting_x", dword32_type);
  append_composite_type_field (rupp_type, "starting_y", dword32_type);
  append_composite_type_field (rupp_type, "count_x", dword32_type);
  append_composite_type_field (rupp_type, "count_y", dword32_type);
  append_composite_type_field (rupp_type, "count_chars_x", dword32_type);
  append_composite_type_field (rupp_type, "count_chars_y", dword32_type);
  append_composite_type_field (rupp_type, "fill_attribute", dword32_type);
  append_composite_type_field (rupp_type, "window_flags", dword32_type);
  append_composite_type_field (rupp_type, "show_window_flags", dword32_type);
  append_composite_type_field_aligned (rupp_type, "window_title",
				       uni_str_type,
				       void_ptr_type->length ());
  append_composite_type_field (rupp_type, "desktop_info", uni_str_type);
  append_composite_type_field (rupp_type, "shell_info", uni_str_type);
  append_composite_type_field (rupp_type, "runtime_data", uni_str_type);

  rupp_ptr_type = init_pointer_type (alloc, gdbarch_ptr_bit (gdbarch),
				     nullptr, rupp_type);


  /* struct process environment block */
  peb_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  peb_type->set_name (xstrdup ("peb"));

  /* First bytes contain several flags.  */
  append_composite_type_field (peb_type, "flags", dword_ptr_type);
  append_composite_type_field (peb_type, "mutant", void_ptr_type);
  append_composite_type_field (peb_type, "image_base_address", void_ptr_type);
  append_composite_type_field (peb_type, "ldr", peb_ldr_ptr_type);
  append_composite_type_field (peb_type, "process_parameters", rupp_ptr_type);
  append_composite_type_field (peb_type, "sub_system_data", void_ptr_type);
  append_composite_type_field (peb_type, "process_heap", void_ptr_type);
  append_composite_type_field (peb_type, "fast_peb_lock", void_ptr_type);
  peb_ptr_type = alloc.new_type (TYPE_CODE_PTR,
				 void_ptr_type->length () * TARGET_CHAR_BIT,
				 NULL);
  peb_ptr_type->set_target_type (peb_type);


  /* struct thread information block */
  tib_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  tib_type->set_name (xstrdup ("tib"));

  /* uint32_t current_seh;			%fs:0x0000 */
  append_composite_type_field (tib_type, "current_seh", seh_ptr_type);
  /* uint32_t current_top_of_stack; 		%fs:0x0004 */
  append_composite_type_field (tib_type, "current_top_of_stack",
			       void_ptr_type);
  /* uint32_t current_bottom_of_stack;		%fs:0x0008 */
  append_composite_type_field (tib_type, "current_bottom_of_stack",
			       void_ptr_type);
  /* uint32_t sub_system_tib;			%fs:0x000c */
  append_composite_type_field (tib_type, "sub_system_tib", void_ptr_type);

  /* uint32_t fiber_data;			%fs:0x0010 */
  append_composite_type_field (tib_type, "fiber_data", void_ptr_type);
  /* uint32_t arbitrary_data_slot;		%fs:0x0014 */
  append_composite_type_field (tib_type, "arbitrary_data_slot", void_ptr_type);
  /* uint32_t linear_address_tib;		%fs:0x0018 */
  append_composite_type_field (tib_type, "linear_address_tib", void_ptr_type);
  /* uint32_t environment_pointer;		%fs:0x001c */
  append_composite_type_field (tib_type, "environment_pointer", void_ptr_type);
  /* uint32_t process_id;			%fs:0x0020 */
  append_composite_type_field (tib_type, "process_id", dword_ptr_type);
  /* uint32_t current_thread_id;		%fs:0x0024 */
  append_composite_type_field (tib_type, "thread_id", dword_ptr_type);
  /* uint32_t active_rpc_handle;		%fs:0x0028 */
  append_composite_type_field (tib_type, "active_rpc_handle", dword_ptr_type);
  /* uint32_t thread_local_storage;		%fs:0x002c */
  append_composite_type_field (tib_type, "thread_local_storage",
			       void_ptr_type);
  /* uint32_t process_environment_block;	%fs:0x0030 */
  append_composite_type_field (tib_type, "process_environment_block",
			       peb_ptr_type);
  /* uint32_t last_error_number;		%fs:0x0034 */
  append_composite_type_field (tib_type, "last_error_number", dword_ptr_type);

  tib_ptr_type = alloc.new_type (TYPE_CODE_PTR,
				 void_ptr_type->length () * TARGET_CHAR_BIT,
				 NULL);
  tib_ptr_type->set_target_type (tib_type);

  windows_gdbarch_data->tib_ptr_type = tib_ptr_type;

  return tib_ptr_type;
}

/* The $_tlb convenience variable is a bit special.  We don't know
   for sure the type of the value until we actually have a chance to
   fetch the data.  The type can change depending on gdbarch, so it is
   also dependent on which thread you have selected.  */

/* This function implements the lval_computed support for reading a
   $_tlb value.  */

static void
tlb_value_read (struct value *val)
{
  CORE_ADDR tlb;
  struct type *type = check_typedef (val->type ());

  if (!target_get_tib_address (inferior_ptid, &tlb))
    error (_("Unable to read tlb"));
  store_typed_address (val->contents_raw ().data (), type, tlb);
}

/* This function implements the lval_computed support for writing a
   $_tlb value.  */

static void
tlb_value_write (struct value *v, struct value *fromval)
{
  error (_("Impossible to change the Thread Local Base"));
}

static const struct lval_funcs tlb_value_funcs =
  {
    tlb_value_read,
    tlb_value_write
  };


/* Return a new value with the correct type for the tlb object of
   the current thread using architecture GDBARCH.  Return a void value
   if there's no object available.  */

static struct value *
tlb_make_value (struct gdbarch *gdbarch, struct internalvar *var, void *ignore)
{
  if (target_has_stack () && inferior_ptid != null_ptid)
    {
      struct type *type = windows_get_tlb_type (gdbarch);
      return value::allocate_computed (type, &tlb_value_funcs, NULL);
    }

  return value::allocate (builtin_type (gdbarch)->builtin_void);
}


/* Display thread information block of a given thread.  */

static int
display_one_tib (ptid_t ptid)
{
  gdb_byte *tib = NULL;
  gdb_byte *index;
  CORE_ADDR thread_local_base;
  ULONGEST i, val, max, max_name, size, tib_size;
  ULONGEST sizeof_ptr = gdbarch_ptr_bit (current_inferior ()->arch ());
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());

  if (sizeof_ptr == 64)
    {
      size = sizeof (uint64_t);
      tib_size = sizeof (thread_information_64);
      max = MAX_TIB64;
    }
  else
    {
      size = sizeof (uint32_t);
      tib_size = sizeof (thread_information_32);
      max = MAX_TIB32;
    }

  max_name = max;

  if (maint_display_all_tib)
    {
      tib_size = FULL_TIB_SIZE;
      max = tib_size / size;
    }
  
  tib = (gdb_byte *) alloca (tib_size);

  if (target_get_tib_address (ptid, &thread_local_base) == 0)
    {
      gdb_printf (_("Unable to get thread local base for %s\n"),
		  target_pid_to_str (ptid).c_str ());
      return -1;
    }

  if (target_read (current_inferior ()->top_target (), TARGET_OBJECT_MEMORY,
		   NULL, tib, thread_local_base, tib_size) != tib_size)
    {
      gdb_printf (_("Unable to read thread information "
		    "block for %s at address %s\n"),
		  target_pid_to_str (ptid).c_str (), 
		  paddress (current_inferior ()->arch (), thread_local_base));
      return -1;
    }

  gdb_printf (_("Thread Information Block %s at %s\n"),
	      target_pid_to_str (ptid).c_str (),
	      paddress (current_inferior ()->arch (), thread_local_base));

  index = (gdb_byte *) tib;

  /* All fields have the size of a pointer, this allows to iterate 
     using the same for loop for both layouts.  */
  for (i = 0; i < max; i++)
    {
      val = extract_unsigned_integer (index, size, byte_order);
      if (i < max_name)
	gdb_printf (_("%s is 0x%s\n"), TIB_NAME[i], phex (val, size));
      else if (val != 0)
	gdb_printf (_("TIB[0x%s] is 0x%s\n"), phex (i * size, 2),
		    phex (val, size));
      index += size;
    } 
  return 1;  
}

/* Display thread information block of the current thread.  */

static void
display_tib (const char * args, int from_tty)
{
  if (inferior_ptid != null_ptid)
    display_one_tib (inferior_ptid);
}

void
windows_xfer_shared_library (const char* so_name, CORE_ADDR load_addr,
			     CORE_ADDR *text_offset_cached,
			     struct gdbarch *gdbarch, std::string &xml)
{
  CORE_ADDR text_offset = text_offset_cached ? *text_offset_cached : 0;

  xml += "<library name=\"";
  xml_escape_text_append (xml, so_name);
  xml += "\"><segment address=\"";

  if (!text_offset)
    {
      gdb_bfd_ref_ptr dll (gdb_bfd_open (so_name, gnutarget));
      /* The following calls are OK even if dll is NULL.
	 The default value 0x1000 is returned by pe_text_section_offset
	 in that case.  */
      text_offset = pe_text_section_offset (dll.get ());
      if (text_offset_cached)
	*text_offset_cached = text_offset;
    }

  xml += paddress (gdbarch, load_addr + text_offset);
  xml += "\"/></library>";
}

/* Implement the "iterate_over_objfiles_in_search_order" gdbarch
   method.  It searches all objfiles, starting with CURRENT_OBJFILE
   first (if not NULL).

   On Windows, the system behaves a little differently when two
   objfiles each define a global symbol using the same name, compared
   to other platforms such as GNU/Linux for instance.  On GNU/Linux,
   all instances of the symbol effectively get merged into a single
   one, but on Windows, they remain distinct.

   As a result, it usually makes sense to start global symbol searches
   with the current objfile before expanding it to all other objfiles.
   This helps for instance when a user debugs some code in a DLL that
   refers to a global variable defined inside that DLL.  When trying
   to print the value of that global variable, it would be unhelpful
   to print the value of another global variable defined with the same
   name, but in a different DLL.  */

static void
windows_iterate_over_objfiles_in_search_order
  (gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype cb,
   objfile *current_objfile)
{
  if (current_objfile)
    {
      if (cb (current_objfile))
	return;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    if (objfile != current_objfile)
      {
	if (cb (objfile))
	  return;
      }
}

static void
show_maint_show_all_tib (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Show all non-zero elements of "
		      "Thread Information Block is %s.\n"), value);
}


static int w32_prefix_command_valid = 0;
void
init_w32_command_list (void)
{
  if (!w32_prefix_command_valid)
    {
      add_basic_prefix_cmd
	("w32", class_info,
	 _("Print information specific to Win32 debugging."),
	 &info_w32_cmdlist, 0, &infolist);
      w32_prefix_command_valid = 1;
    }
}

/* Implementation of `gdbarch_gdb_signal_to_target' for Windows.  */

static int
windows_gdb_signal_to_target (struct gdbarch *gdbarch, enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;
    case GDB_SIGNAL_HUP:
      return WINDOWS_SIGHUP;
    case GDB_SIGNAL_INT:
      return WINDOWS_SIGINT;
    case GDB_SIGNAL_QUIT:
      return WINDOWS_SIGQUIT;
    case GDB_SIGNAL_ILL:
      return WINDOWS_SIGILL;
    case GDB_SIGNAL_TRAP:
      return WINDOWS_SIGTRAP;
    case GDB_SIGNAL_ABRT:
      return WINDOWS_SIGABRT;
    case GDB_SIGNAL_EMT:
      return WINDOWS_SIGEMT;
    case GDB_SIGNAL_FPE:
      return WINDOWS_SIGFPE;
    case GDB_SIGNAL_KILL:
      return WINDOWS_SIGKILL;
    case GDB_SIGNAL_BUS:
      return WINDOWS_SIGBUS;
    case GDB_SIGNAL_SEGV:
      return WINDOWS_SIGSEGV;
    case GDB_SIGNAL_SYS:
      return WINDOWS_SIGSYS;
    case GDB_SIGNAL_PIPE:
      return WINDOWS_SIGPIPE;
    case GDB_SIGNAL_ALRM:
      return WINDOWS_SIGALRM;
    case GDB_SIGNAL_TERM:
      return WINDOWS_SIGTERM;
    }
  return -1;
}

/* Implementation of `gdbarch_gdb_signal_to_target' for Cygwin.  */

static int
cygwin_gdb_signal_to_target (struct gdbarch *gdbarch, enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;
    case GDB_SIGNAL_HUP:
      return CYGWIN_SIGHUP;
    case GDB_SIGNAL_INT:
      return CYGWIN_SIGINT;
    case GDB_SIGNAL_QUIT:
      return CYGWIN_SIGQUIT;
    case GDB_SIGNAL_ILL:
      return CYGWIN_SIGILL;
    case GDB_SIGNAL_TRAP:
      return CYGWIN_SIGTRAP;
    case GDB_SIGNAL_ABRT:
      return CYGWIN_SIGABRT;
    case GDB_SIGNAL_EMT:
      return CYGWIN_SIGEMT;
    case GDB_SIGNAL_FPE:
      return CYGWIN_SIGFPE;
    case GDB_SIGNAL_KILL:
      return CYGWIN_SIGKILL;
    case GDB_SIGNAL_BUS:
      return CYGWIN_SIGBUS;
    case GDB_SIGNAL_SEGV:
      return CYGWIN_SIGSEGV;
    case GDB_SIGNAL_SYS:
      return CYGWIN_SIGSYS;
    case GDB_SIGNAL_PIPE:
      return CYGWIN_SIGPIPE;
    case GDB_SIGNAL_ALRM:
      return CYGWIN_SIGALRM;
    case GDB_SIGNAL_TERM:
      return CYGWIN_SIGTERM;
    case GDB_SIGNAL_URG:
      return CYGWIN_SIGURG;
    case GDB_SIGNAL_STOP:
      return CYGWIN_SIGSTOP;
    case GDB_SIGNAL_TSTP:
      return CYGWIN_SIGTSTP;
    case GDB_SIGNAL_CONT:
      return CYGWIN_SIGCONT;
    case GDB_SIGNAL_CHLD:
      return CYGWIN_SIGCHLD;
    case GDB_SIGNAL_TTIN:
      return CYGWIN_SIGTTIN;
    case GDB_SIGNAL_TTOU:
      return CYGWIN_SIGTTOU;
    case GDB_SIGNAL_IO:
      return CYGWIN_SIGIO;
    case GDB_SIGNAL_XCPU:
      return CYGWIN_SIGXCPU;
    case GDB_SIGNAL_XFSZ:
      return CYGWIN_SIGXFSZ;
    case GDB_SIGNAL_VTALRM:
      return CYGWIN_SIGVTALRM;
    case GDB_SIGNAL_PROF:
      return CYGWIN_SIGPROF;
    case GDB_SIGNAL_WINCH:
      return CYGWIN_SIGWINCH;
    case GDB_SIGNAL_PWR:
      return CYGWIN_SIGLOST;
    case GDB_SIGNAL_USR1:
      return CYGWIN_SIGUSR1;
    case GDB_SIGNAL_USR2:
      return CYGWIN_SIGUSR2;
    }
  return -1;
}

struct enum_value_name
{
  uint32_t value;
  const char *name;
};

/* Allocate a TYPE_CODE_ENUM type structure with its named values.  */

static struct type *
create_enum (struct gdbarch *gdbarch, int bit, const char *name,
	     const struct enum_value_name *values, int count)
{
  struct type *type;
  int i;

  type = type_allocator (gdbarch).new_type (TYPE_CODE_ENUM, bit, name);
  type->alloc_fields (count);
  type->set_is_unsigned (true);

  for (i = 0; i < count; i++)
    {
      type->field (i).set_name (values[i].name);
      type->field (i).set_loc_enumval (values[i].value);
    }

  return type;
}

static const struct enum_value_name exception_values[] =
{
  { 0x40000015, "FATAL_APP_EXIT" },
  { 0x4000001E, "WX86_SINGLE_STEP" },
  { 0x4000001F, "WX86_BREAKPOINT" },
  { 0x40010005, "DBG_CONTROL_C" },
  { 0x40010008, "DBG_CONTROL_BREAK" },
  { 0x80000002, "DATATYPE_MISALIGNMENT" },
  { 0x80000003, "BREAKPOINT" },
  { 0x80000004, "SINGLE_STEP" },
  { 0xC0000005, "ACCESS_VIOLATION" },
  { 0xC0000006, "IN_PAGE_ERROR" },
  { 0xC000001D, "ILLEGAL_INSTRUCTION" },
  { 0xC0000025, "NONCONTINUABLE_EXCEPTION" },
  { 0xC0000026, "INVALID_DISPOSITION" },
  { 0xC000008C, "ARRAY_BOUNDS_EXCEEDED" },
  { 0xC000008D, "FLOAT_DENORMAL_OPERAND" },
  { 0xC000008E, "FLOAT_DIVIDE_BY_ZERO" },
  { 0xC000008F, "FLOAT_INEXACT_RESULT" },
  { 0xC0000090, "FLOAT_INVALID_OPERATION" },
  { 0xC0000091, "FLOAT_OVERFLOW" },
  { 0xC0000092, "FLOAT_STACK_CHECK" },
  { 0xC0000093, "FLOAT_UNDERFLOW" },
  { 0xC0000094, "INTEGER_DIVIDE_BY_ZERO" },
  { 0xC0000095, "INTEGER_OVERFLOW" },
  { 0xC0000096, "PRIV_INSTRUCTION" },
  { 0xC00000FD, "STACK_OVERFLOW" },
  { 0xC0000409, "FAST_FAIL" },
};

static const struct enum_value_name violation_values[] =
{
  { 0, "READ_ACCESS_VIOLATION" },
  { 1, "WRITE_ACCESS_VIOLATION" },
  { 8, "DATA_EXECUTION_PREVENTION_VIOLATION" },
};

/* Implement the "get_siginfo_type" gdbarch method.  */

static struct type *
windows_get_siginfo_type (struct gdbarch *gdbarch)
{
  struct windows_gdbarch_data *windows_gdbarch_data;
  struct type *dword_type, *pvoid_type, *ulongptr_type;
  struct type *code_enum, *violation_enum;
  struct type *violation_type, *para_type, *siginfo_ptr_type, *siginfo_type;

  windows_gdbarch_data = get_windows_gdbarch_data (gdbarch);
  if (windows_gdbarch_data->siginfo_type != NULL)
    return windows_gdbarch_data->siginfo_type;

  type_allocator alloc (gdbarch);
  dword_type = init_integer_type (alloc, gdbarch_int_bit (gdbarch),
				  1, "DWORD");
  pvoid_type = init_pointer_type (alloc, gdbarch_ptr_bit (gdbarch), "PVOID",
				  builtin_type (gdbarch)->builtin_void);
  ulongptr_type = init_integer_type (alloc, gdbarch_ptr_bit (gdbarch),
				     1, "ULONG_PTR");

  /* ExceptionCode value names */
  code_enum = create_enum (gdbarch, gdbarch_int_bit (gdbarch),
			   "ExceptionCode", exception_values,
			   ARRAY_SIZE (exception_values));

  /* ACCESS_VIOLATION type names */
  violation_enum = create_enum (gdbarch, gdbarch_ptr_bit (gdbarch),
				"ViolationType", violation_values,
				ARRAY_SIZE (violation_values));

  /* ACCESS_VIOLATION information */
  violation_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (violation_type, "Type", violation_enum);
  append_composite_type_field (violation_type, "Address", pvoid_type);

  /* Unnamed union of the documented field ExceptionInformation,
     and the alternative AccessViolationInformation (which displays
     human-readable values for ExceptionCode ACCESS_VIOLATION).  */
  para_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);
  append_composite_type_field (para_type, "ExceptionInformation",
			       lookup_array_range_type (ulongptr_type, 0, 14));
  append_composite_type_field (para_type, "AccessViolationInformation",
			       violation_type);

  siginfo_type = arch_composite_type (gdbarch, "EXCEPTION_RECORD",
				      TYPE_CODE_STRUCT);
  siginfo_ptr_type = init_pointer_type (alloc, gdbarch_ptr_bit (gdbarch),
					nullptr, siginfo_type);

  /* ExceptionCode is documented as type DWORD, but here a helper
     enum type is used instead to display a human-readable value.  */
  append_composite_type_field (siginfo_type, "ExceptionCode", code_enum);
  append_composite_type_field (siginfo_type, "ExceptionFlags", dword_type);
  append_composite_type_field (siginfo_type, "ExceptionRecord",
			       siginfo_ptr_type);
  append_composite_type_field (siginfo_type, "ExceptionAddress",
			       pvoid_type);
  append_composite_type_field (siginfo_type, "NumberParameters", dword_type);
  /* The 64-bit variant needs some padding.  */
  append_composite_type_field_aligned (siginfo_type, "",
				       para_type, ulongptr_type->length ());

  windows_gdbarch_data->siginfo_type = siginfo_type;

  return siginfo_type;
}

/* Implement the "solib_create_inferior_hook" target_so_ops method.  */

static void
windows_solib_create_inferior_hook (int from_tty)
{
  CORE_ADDR exec_base = 0;

  /* Find base address of main executable in
     TIB->process_environment_block->image_base_address.  */
  gdbarch *gdbarch = current_inferior ()->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int ptr_bytes;
  int peb_offset;  /* Offset of process_environment_block in TIB.  */
  int base_offset; /* Offset of image_base_address in PEB.  */
  if (gdbarch_ptr_bit (gdbarch) == 32)
    {
      ptr_bytes = 4;
      peb_offset = 48;
      base_offset = 8;
    }
  else
    {
      ptr_bytes = 8;
      peb_offset = 96;
      base_offset = 16;
    }
  CORE_ADDR tlb;
  gdb_byte buf[8];
  if (target_has_execution ()
      && target_get_tib_address (inferior_ptid, &tlb)
      && !target_read_memory (tlb + peb_offset, buf, ptr_bytes))
    {
      CORE_ADDR peb = extract_unsigned_integer (buf, ptr_bytes, byte_order);
      if (!target_read_memory (peb + base_offset, buf, ptr_bytes))
	exec_base = extract_unsigned_integer (buf, ptr_bytes, byte_order);
    }

  /* Rebase executable if the base address changed because of ASLR.  */
  if (current_program_space->symfile_object_file != nullptr && exec_base != 0)
    {
      CORE_ADDR vmaddr
	= pe_data (current_program_space->exec_bfd ())->pe_opthdr.ImageBase;
      if (vmaddr != exec_base)
	objfile_rebase (current_program_space->symfile_object_file,
			exec_base - vmaddr);
    }
}

static struct target_so_ops windows_so_ops;

/* Common parts for gdbarch initialization for the Windows and Cygwin OS
   ABIs.  */

static void
windows_init_abi_common (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_wchar_bit (gdbarch, 16);
  set_gdbarch_wchar_signed (gdbarch, 0);

  /* Canonical paths on this target look like
     `c:\Program Files\Foo App\mydll.dll', for example.  */
  set_gdbarch_has_dos_based_file_system (gdbarch, 1);

  set_gdbarch_iterate_over_objfiles_in_search_order
    (gdbarch, windows_iterate_over_objfiles_in_search_order);

  windows_so_ops = solib_target_so_ops;
  windows_so_ops.solib_create_inferior_hook
    = windows_solib_create_inferior_hook;
  set_gdbarch_so_ops (gdbarch, &windows_so_ops);

  set_gdbarch_get_siginfo_type (gdbarch, windows_get_siginfo_type);
}

/* See windows-tdep.h.  */
void
windows_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  windows_init_abi_common (info, gdbarch);
  set_gdbarch_gdb_signal_to_target (gdbarch, windows_gdb_signal_to_target);
}

/* See windows-tdep.h.  */

void
cygwin_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  windows_init_abi_common (info, gdbarch);
  set_gdbarch_gdb_signal_to_target (gdbarch, cygwin_gdb_signal_to_target);
}

/* Implementation of `tlb' variable.  */

static const struct internalvar_funcs tlb_funcs =
{
  tlb_make_value,
  NULL,
};

/* Layout of an element of a PE's Import Directory Table.  Based on:

     https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#import-directory-table
 */

struct pe_import_directory_entry
{
  uint32_t import_lookup_table_rva;
  uint32_t timestamp;
  uint32_t forwarder_chain;
  uint32_t name_rva;
  uint32_t import_address_table_rva;
};

static_assert (sizeof (pe_import_directory_entry) == 20);

/* See windows-tdep.h.  */

bool
is_linked_with_cygwin_dll (bfd *abfd)
{
  /* The list of DLLs a PE is linked to is in the .idata section.  See:

     https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#the-idata-section
   */
  asection *idata_section = bfd_get_section_by_name (abfd, ".idata");
  if (idata_section == nullptr)
    return false;

  bfd_size_type idata_section_size = bfd_section_size (idata_section);
  internal_extra_pe_aouthdr *pe_extra = &pe_data (abfd)->pe_opthdr;
  bfd_vma import_table_va = pe_extra->DataDirectory[PE_IMPORT_TABLE].VirtualAddress;
  bfd_vma idata_section_va = bfd_section_vma (idata_section);

  /* The section's virtual address as reported by BFD has the image base applied,
     remove it.  */
  gdb_assert (idata_section_va >= pe_extra->ImageBase);
  idata_section_va -= pe_extra->ImageBase;

  bfd_vma idata_section_end_va = idata_section_va + idata_section_size;

  /* Make sure that the import table is indeed within the .idata section's range.  */
  if (import_table_va < idata_section_va
      || import_table_va >= idata_section_end_va)
    {
      warning (_("\
%s: import table's virtual address (%s) is outside .idata \
section's range [%s, %s]."),
	       bfd_get_filename (abfd), hex_string (import_table_va),
	       hex_string (idata_section_va),
	       hex_string (idata_section_end_va));
      return false;
    }

  /* The import table starts at this offset into the .idata section.  */
  bfd_vma import_table_offset_in_sect = import_table_va - idata_section_va;

  /* Get the section's data.  */
  gdb::byte_vector idata_contents;
  if (!gdb_bfd_get_full_section_contents (abfd, idata_section, &idata_contents))
    {
      warning (_("%s: failed to get contents of .idata section."),
	       bfd_get_filename (abfd));
      return false;
    }

  gdb_assert (idata_contents.size () == idata_section_size);

  const gdb_byte *iter = idata_contents.data () + import_table_offset_in_sect;
  const gdb_byte *end = idata_contents.data () + idata_section_size;
  const pe_import_directory_entry null_dir_entry = { 0 };

  /* Iterate through all directory entries.  */
  while (true)
    {
      /* Is there enough space left in the section for another entry?  */
      if (iter + sizeof (pe_import_directory_entry) > end)
	{
	  warning (_("%s: unexpected end of .idata section."),
		   bfd_get_filename (abfd));
	  break;
	}

      pe_import_directory_entry *dir_entry = (pe_import_directory_entry *) iter;

      /* Is it the end of list marker?  */
      if (memcmp (dir_entry, &null_dir_entry,
		  sizeof (pe_import_directory_entry)) == 0)
	break;

      bfd_vma name_va = dir_entry->name_rva;

      /* If the name's virtual address is smaller than the section's virtual
	 address, there's a problem.  */
      if (name_va < idata_section_va || name_va >= idata_section_end_va)
	{
	  warning (_("\
%s: name's virtual address (%s) is outside .idata section's \
range [%s, %s]."),
		   bfd_get_filename (abfd), hex_string (name_va),
		   hex_string (idata_section_va),
		   hex_string (idata_section_end_va));
	  break;
	}

      const gdb_byte *name = &idata_contents[name_va - idata_section_va];

      /* Make sure we don't overshoot the end of the section with the
	 streq.  */
      if (name + sizeof (CYGWIN_DLL_NAME) <= end)
	{
	  /* Finally, check if this is the dll name we are looking for.  */
	  if (streq ((const char *) name, CYGWIN_DLL_NAME))
	    return true;
	}

      iter += sizeof (pe_import_directory_entry);
    }

  return false;
}

struct cpms_data
{
  struct gdbarch *gdbarch;
  std::string xml;
  int module_count;
};

static void
core_process_module_section (bfd *abfd, asection *sect, void *obj)
{
  struct cpms_data *data = (struct cpms_data *) obj;
  enum bfd_endian byte_order = gdbarch_byte_order (data->gdbarch);

  unsigned int data_type;
  char *module_name;
  size_t module_name_size;
  size_t module_name_offset;
  CORE_ADDR base_addr;

  if (!startswith (sect->name, ".module"))
    return;

  gdb::byte_vector buf (bfd_section_size (sect) + 1);
  if (!bfd_get_section_contents (abfd, sect,
				 buf.data (), 0, bfd_section_size (sect)))
    return;
  /* We're going to treat part of the buffer as a string, so make sure
     it is NUL-terminated.  */
  buf.back () = 0;

  /* A DWORD (data_type) followed by struct windows_core_module_info.  */
  if (bfd_section_size (sect) < 4)
    return;
  data_type = extract_unsigned_integer (buf.data (), 4, byte_order);

  if (data_type == NOTE_INFO_MODULE)
    {
      module_name_offset = 12;
      if (bfd_section_size (sect) < module_name_offset)
	return;
      base_addr = extract_unsigned_integer (&buf[4], 4, byte_order);
      module_name_size = extract_unsigned_integer (&buf[8], 4, byte_order);
    }
  else if (data_type == NOTE_INFO_MODULE64)
    {
      module_name_offset = 16;
      if (bfd_section_size (sect) < module_name_offset)
	return;
      base_addr = extract_unsigned_integer (&buf[4], 8, byte_order);
      module_name_size = extract_unsigned_integer (&buf[12], 4, byte_order);
    }
  else
    return;

  if (module_name_offset + module_name_size > bfd_section_size (sect))
    return;
  module_name = (char *) buf.data () + module_name_offset;

  /* The first module is the .exe itself.  */
  if (data->module_count != 0)
    windows_xfer_shared_library (module_name, base_addr,
				 NULL, data->gdbarch, data->xml);
  data->module_count++;
}

ULONGEST
windows_core_xfer_shared_libraries (struct gdbarch *gdbarch,
				    gdb_byte *readbuf,
				    ULONGEST offset, ULONGEST len)
{
  cpms_data data { gdbarch, "<library-list>\n", 0 };
  bfd_map_over_sections (core_bfd,
			 core_process_module_section,
			 &data);
  data.xml += "</library-list>\n";

  ULONGEST len_avail = data.xml.length ();
  if (offset >= len_avail)
    return 0;

  if (len > len_avail - offset)
    len = len_avail - offset;

  memcpy (readbuf, data.xml.data () + offset, len);

  return len;
}

/* This is how we want PTIDs from core files to be printed.  */

std::string
windows_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("Thread 0x%lx", ptid.lwp ());

  return normal_pid_to_str (ptid);
}

void _initialize_windows_tdep ();
void
_initialize_windows_tdep ()
{
  init_w32_command_list ();
  cmd_list_element *info_w32_thread_information_block_cmd
    = add_cmd ("thread-information-block", class_info, display_tib,
	       _("Display thread information block."),
	       &info_w32_cmdlist);
  add_alias_cmd ("tib", info_w32_thread_information_block_cmd, class_info, 1,
		 &info_w32_cmdlist);

  add_setshow_boolean_cmd ("show-all-tib", class_maintenance,
			   &maint_display_all_tib, _("\
Set whether to display all non-zero fields of thread information block."), _("\
Show whether to display all non-zero fields of thread information block."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, all non-zero fields of thread information block are displayed,\n\
even if their meaning is unknown."),
			   NULL,
			   show_maint_show_all_tib,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  /* Explicitly create without lookup, since that tries to create a
     value with a void typed value, and when we get here, gdbarch
     isn't initialized yet.  At this point, we're quite sure there
     isn't another convenience variable of the same name.  */
  create_internalvar_type_lazy ("_tlb", &tlb_funcs, NULL);
}
