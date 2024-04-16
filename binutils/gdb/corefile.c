/* Core dump and executable file functions above target vector, for GDB.

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

#include "defs.h"
#include <signal.h>
#include <fcntl.h>
#include "inferior.h"
#include "symtab.h"
#include "command.h"
#include "gdbcmd.h"
#include "bfd.h"
#include "target.h"
#include "gdbcore.h"
#include "dis-asm.h"
#include <sys/stat.h>
#include "completer.h"
#include "observable.h"
#include "cli/cli-utils.h"
#include "gdbarch.h"
#include "interps.h"

/* You can have any number of hooks for `exec_file_command' command to
   call.  If there's only one hook, it is set in exec_file_display
   hook.  If there are two or more hooks, they are set in
   exec_file_extra_hooks[], and deprecated_exec_file_display_hook is
   set to a function that calls all of them.  This extra complexity is
   needed to preserve compatibility with old code that assumed that
   only one hook could be set, and which called
   deprecated_exec_file_display_hook directly.  */

typedef void (*hook_type) (const char *);

hook_type deprecated_exec_file_display_hook;	/* The original hook.  */
static hook_type *exec_file_extra_hooks;	/* Array of additional
						   hooks.  */
static int exec_file_hook_count = 0;		/* Size of array.  */



/* If there are two or more functions that wish to hook into
   exec_file_command, this function will call all of the hook
   functions.  */

static void
call_extra_exec_file_hooks (const char *filename)
{
  int i;

  for (i = 0; i < exec_file_hook_count; i++)
    (*exec_file_extra_hooks[i]) (filename);
}

/* Call this to specify the hook for exec_file_command to call back.
   This is called from the x-window display code.  */

void
specify_exec_file_hook (void (*hook) (const char *))
{
  hook_type *new_array;

  if (deprecated_exec_file_display_hook != NULL)
    {
      /* There's already a hook installed.  Arrange to have both it
	 and the subsequent hooks called.  */
      if (exec_file_hook_count == 0)
	{
	  /* If this is the first extra hook, initialize the hook
	     array.  */
	  exec_file_extra_hooks = XNEW (hook_type);
	  exec_file_extra_hooks[0] = deprecated_exec_file_display_hook;
	  deprecated_exec_file_display_hook = call_extra_exec_file_hooks;
	  exec_file_hook_count = 1;
	}

      /* Grow the hook array by one and add the new hook to the end.
	 Yes, it's inefficient to grow it by one each time but since
	 this is hardly ever called it's not a big deal.  */
      exec_file_hook_count++;
      new_array = (hook_type *)
	xrealloc (exec_file_extra_hooks,
		  exec_file_hook_count * sizeof (hook_type));
      exec_file_extra_hooks = new_array;
      exec_file_extra_hooks[exec_file_hook_count - 1] = hook;
    }
  else
    deprecated_exec_file_display_hook = hook;
}

void
reopen_exec_file (void)
{
  bfd *exec_bfd = current_program_space->exec_bfd ();

  /* Don't do anything if there isn't an exec file.  */
  if (exec_bfd == nullptr)
    return;

  /* The main executable can't be an in-memory BFD object.  If it was then
     the use of bfd_stat below would not work as expected.  */
  gdb_assert ((exec_bfd->flags & BFD_IN_MEMORY) == 0);

  /* If the timestamp of the exec file has changed, reopen it.  */
  struct stat st;
  int res = bfd_stat (exec_bfd, &st);

  if (res == 0
      && current_program_space->ebfd_mtime != 0
      && current_program_space->ebfd_mtime != st.st_mtime)
    exec_file_attach (bfd_get_filename (exec_bfd), 0);
}

/* If we have both a core file and an exec file,
   print a warning if they don't go together.  */

void
validate_files (void)
{
  if (current_program_space->exec_bfd () && core_bfd)
    {
      if (!core_file_matches_executable_p (core_bfd,
					   current_program_space->exec_bfd ()))
	warning (_("core file may not match specified executable file."));
      else if (bfd_get_mtime (current_program_space->exec_bfd ())
	       > bfd_get_mtime (core_bfd))
	warning (_("exec file is newer than core file."));
    }
}

/* See gdbsupport/common-inferior.h.  */

const char *
get_exec_file (int err)
{
  if (current_program_space->exec_filename != nullptr)
    return current_program_space->exec_filename.get ();
  if (!err)
    return NULL;

  error (_("No executable file specified.\n\
Use the \"file\" or \"exec-file\" command."));
}


std::string
memory_error_message (enum target_xfer_status err,
		      struct gdbarch *gdbarch, CORE_ADDR memaddr)
{
  switch (err)
    {
    case TARGET_XFER_E_IO:
      /* Actually, address between memaddr and memaddr + len was out of
	 bounds.  */
      return string_printf (_("Cannot access memory at address %s"),
			    paddress (gdbarch, memaddr));
    case TARGET_XFER_UNAVAILABLE:
      return string_printf (_("Memory at address %s unavailable."),
			    paddress (gdbarch, memaddr));
    default:
      internal_error ("unhandled target_xfer_status: %s (%s)",
		      target_xfer_status_to_string (err),
		      plongest (err));
    }
}

/* Report a memory error by throwing a suitable exception.  */

void
memory_error (enum target_xfer_status err, CORE_ADDR memaddr)
{
  enum errors exception = GDB_NO_ERROR;

  /* Build error string.  */
  std::string str
    = memory_error_message (err, current_inferior ()->arch (), memaddr);

  /* Choose the right error to throw.  */
  switch (err)
    {
    case TARGET_XFER_E_IO:
      exception = MEMORY_ERROR;
      break;
    case TARGET_XFER_UNAVAILABLE:
      exception = NOT_AVAILABLE_ERROR;
      break;
    }

  /* Throw it.  */
  throw_error (exception, ("%s"), str.c_str ());
}

/* Helper function.  */

static void
read_memory_object (enum target_object object, CORE_ADDR memaddr,
		    gdb_byte *myaddr, ssize_t len)
{
  ULONGEST xfered = 0;

  while (xfered < len)
    {
      enum target_xfer_status status;
      ULONGEST xfered_len;

      status = target_xfer_partial (current_inferior ()->top_target (), object,
				    NULL, myaddr + xfered, NULL,
				    memaddr + xfered, len - xfered,
				    &xfered_len);

      if (status != TARGET_XFER_OK)
	memory_error (status == TARGET_XFER_EOF ? TARGET_XFER_E_IO : status,
		      memaddr + xfered);

      xfered += xfered_len;
      QUIT;
    }
}

/* Same as target_read_memory, but report an error if can't read.  */

void
read_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  read_memory_object (TARGET_OBJECT_MEMORY, memaddr, myaddr, len);
}

/* Same as target_read_stack, but report an error if can't read.  */

void
read_stack (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  read_memory_object (TARGET_OBJECT_STACK_MEMORY, memaddr, myaddr, len);
}

/* Same as target_read_code, but report an error if can't read.  */

void
read_code (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  read_memory_object (TARGET_OBJECT_CODE_MEMORY, memaddr, myaddr, len);
}

/* Read memory at MEMADDR of length LEN and put the contents in
   RETURN_VALUE.  Return 0 if MEMADDR couldn't be read and non-zero
   if successful.  */

int
safe_read_memory_integer (CORE_ADDR memaddr, int len, 
			  enum bfd_endian byte_order,
			  LONGEST *return_value)
{
  gdb_byte buf[sizeof (LONGEST)];

  if (target_read_memory (memaddr, buf, len))
    return 0;

  *return_value = extract_signed_integer (buf, len, byte_order);
  return 1;
}

/* Read memory at MEMADDR of length LEN and put the contents in
   RETURN_VALUE.  Return 0 if MEMADDR couldn't be read and non-zero
   if successful.  */

int
safe_read_memory_unsigned_integer (CORE_ADDR memaddr, int len,
				   enum bfd_endian byte_order,
				   ULONGEST *return_value)
{
  gdb_byte buf[sizeof (ULONGEST)];

  if (target_read_memory (memaddr, buf, len))
    return 0;

  *return_value = extract_unsigned_integer (buf, len, byte_order);
  return 1;
}

LONGEST
read_memory_integer (CORE_ADDR memaddr, int len,
		     enum bfd_endian byte_order)
{
  gdb_byte buf[sizeof (LONGEST)];

  read_memory (memaddr, buf, len);
  return extract_signed_integer (buf, len, byte_order);
}

ULONGEST
read_memory_unsigned_integer (CORE_ADDR memaddr, int len,
			      enum bfd_endian byte_order)
{
  gdb_byte buf[sizeof (ULONGEST)];

  read_memory (memaddr, buf, len);
  return extract_unsigned_integer (buf, len, byte_order);
}

LONGEST
read_code_integer (CORE_ADDR memaddr, int len,
		   enum bfd_endian byte_order)
{
  gdb_byte buf[sizeof (LONGEST)];

  read_code (memaddr, buf, len);
  return extract_signed_integer (buf, len, byte_order);
}

ULONGEST
read_code_unsigned_integer (CORE_ADDR memaddr, int len,
			    enum bfd_endian byte_order)
{
  gdb_byte buf[sizeof (ULONGEST)];

  read_code (memaddr, buf, len);
  return extract_unsigned_integer (buf, len, byte_order);
}

CORE_ADDR
read_memory_typed_address (CORE_ADDR addr, struct type *type)
{
  gdb_byte *buf = (gdb_byte *) alloca (type->length ());

  read_memory (addr, buf, type->length ());
  return extract_typed_address (buf, type);
}

/* See gdbcore.h.  */

void
write_memory (CORE_ADDR memaddr, 
	      const bfd_byte *myaddr, ssize_t len)
{
  int status;

  status = target_write_memory (memaddr, myaddr, len);
  if (status != 0)
    memory_error (TARGET_XFER_E_IO, memaddr);
}

/* Notify interpreters and observers that INF's memory was changed.  */

static void
notify_memory_changed (inferior *inf, CORE_ADDR addr, ssize_t len,
		       const bfd_byte *data)
{
  interps_notify_memory_changed (inf, addr, len, data);
  gdb::observers::memory_changed.notify (inf, addr, len, data);
}

/* Same as write_memory, but notify 'memory_changed' observers.  */

void
write_memory_with_notification (CORE_ADDR memaddr, const bfd_byte *myaddr,
				ssize_t len)
{
  write_memory (memaddr, myaddr, len);
  notify_memory_changed (current_inferior (), memaddr, len, myaddr);
}

/* Store VALUE at ADDR in the inferior as a LEN-byte unsigned
   integer.  */
void
write_memory_unsigned_integer (CORE_ADDR addr, int len, 
			       enum bfd_endian byte_order,
			       ULONGEST value)
{
  gdb_byte *buf = (gdb_byte *) alloca (len);

  store_unsigned_integer (buf, len, byte_order, value);
  write_memory (addr, buf, len);
}

/* Store VALUE at ADDR in the inferior as a LEN-byte signed
   integer.  */
void
write_memory_signed_integer (CORE_ADDR addr, int len, 
			     enum bfd_endian byte_order,
			     LONGEST value)
{
  gdb_byte *buf = (gdb_byte *) alloca (len);

  store_signed_integer (buf, len, byte_order, value);
  write_memory (addr, buf, len);
}

/* The current default bfd target.  Points to storage allocated for
   gnutarget_string.  */
const char *gnutarget;

/* Same thing, except it is "auto" not NULL for the default case.  */
static std::string gnutarget_string;
static void
show_gnutarget_string (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c,
		       const char *value)
{
  gdb_printf (file,
	      _("The current BFD target is \"%s\".\n"), value);
}

static void
set_gnutarget_command (const char *ignore, int from_tty,
		       struct cmd_list_element *c)
{
  const char *gend = gnutarget_string.c_str () + gnutarget_string.size ();
  gend = remove_trailing_whitespace (gnutarget_string.c_str (), gend);
  gnutarget_string
    = gnutarget_string.substr (0, gend - gnutarget_string.data ());

  if (gnutarget_string == "auto")
    gnutarget = NULL;
  else
    gnutarget = gnutarget_string.c_str ();
}

/* A completion function for "set gnutarget".  */

static void
complete_set_gnutarget (struct cmd_list_element *cmd,
			completion_tracker &tracker,
			const char *text, const char *word)
{
  static const char **bfd_targets;

  if (bfd_targets == NULL)
    {
      int last;

      bfd_targets = bfd_target_list ();
      for (last = 0; bfd_targets[last] != NULL; ++last)
	;

      bfd_targets = XRESIZEVEC (const char *, bfd_targets, last + 2);
      bfd_targets[last] = "auto";
      bfd_targets[last + 1] = NULL;
    }

  complete_on_enum (tracker, bfd_targets, text, word);
}

/* Set the gnutarget.  */
void
set_gnutarget (const char *newtarget)
{
  gnutarget_string = newtarget;
  set_gnutarget_command (NULL, 0, NULL);
}

void _initialize_core ();
void
_initialize_core ()
{
  cmd_list_element *core_file_cmd
    = add_cmd ("core-file", class_files, core_file_command, _("\
Use FILE as core dump for examining memory and registers.\n\
Usage: core-file FILE\n\
No arg means have no core file.  This command has been superseded by the\n\
`target core' and `detach' commands."), &cmdlist);
  set_cmd_completer (core_file_cmd, filename_completer);

  
  set_show_commands set_show_gnutarget
    = add_setshow_string_noescape_cmd ("gnutarget", class_files,
				       &gnutarget_string, _("\
Set the current BFD target."), _("\
Show the current BFD target."), _("\
Use `set gnutarget auto' to specify automatic detection."),
				       set_gnutarget_command,
				       show_gnutarget_string,
				       &setlist, &showlist);
  set_cmd_completer (set_show_gnutarget.set, complete_set_gnutarget);

  add_alias_cmd ("g", set_show_gnutarget.set, class_files, 1, &setlist);

  if (getenv ("GNUTARGET"))
    set_gnutarget (getenv ("GNUTARGET"));
  else
    set_gnutarget ("auto");
}
