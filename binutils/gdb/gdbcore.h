/* Machine independent variables that describe the core file under GDB.

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

/* Interface routines for core, executable, etc.  */

#if !defined (GDBCORE_H)
#define GDBCORE_H 1

struct type;
struct regcache;

#include "bfd.h"
#include "exec.h"
#include "target.h"

/* Nonzero if there is a core file.  */

extern int have_core_file_p (void);

/* Report a memory error with error().  */

extern void memory_error (enum target_xfer_status status, CORE_ADDR memaddr);

/* The string 'memory_error' would use as exception message.  */

extern std::string memory_error_message (enum target_xfer_status err,
					 struct gdbarch *gdbarch,
					 CORE_ADDR memaddr);

/* Like target_read_memory, but report an error if can't read.  */

extern void read_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len);

/* Like target_read_stack, but report an error if can't read.  */

extern void read_stack (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len);

/* Like target_read_code, but report an error if can't read.  */

extern void read_code (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len);

/* Read an integer from debugged memory, given address and number of
   bytes.  */

extern LONGEST read_memory_integer (CORE_ADDR memaddr,
				    int len, enum bfd_endian byte_order);
extern int safe_read_memory_integer (CORE_ADDR memaddr, int len,
				     enum bfd_endian byte_order,
				     LONGEST *return_value);

/* Read an unsigned integer from debugged memory, given address and
   number of bytes.  */

extern ULONGEST read_memory_unsigned_integer (CORE_ADDR memaddr,
					      int len,
					      enum bfd_endian byte_order);
extern int safe_read_memory_unsigned_integer (CORE_ADDR memaddr, int len,
					      enum bfd_endian byte_order,
					      ULONGEST *return_value);

/* Read an integer from debugged code memory, given address,
   number of bytes, and byte order for code.  */

extern LONGEST read_code_integer (CORE_ADDR memaddr, int len,
				  enum bfd_endian byte_order);

/* Read an unsigned integer from debugged code memory, given address,
   number of bytes, and byte order for code.  */

extern ULONGEST read_code_unsigned_integer (CORE_ADDR memaddr,
					    int len,
					    enum bfd_endian byte_order);

/* Read the pointer of type TYPE at ADDR, and return the address it
   represents.  */

CORE_ADDR read_memory_typed_address (CORE_ADDR addr, struct type *type);

/* Same as target_write_memory, but report an error if can't
   write.  */

extern void write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr,
			  ssize_t len);

/* Same as write_memory, but notify 'memory_changed' observers.  */

extern void write_memory_with_notification (CORE_ADDR memaddr,
					    const bfd_byte *myaddr,
					    ssize_t len);

/* Store VALUE at ADDR in the inferior as a LEN-byte unsigned integer.  */
extern void write_memory_unsigned_integer (CORE_ADDR addr, int len,
					   enum bfd_endian byte_order,
					   ULONGEST value);

/* Store VALUE at ADDR in the inferior as a LEN-byte unsigned integer.  */
extern void write_memory_signed_integer (CORE_ADDR addr, int len,
					 enum bfd_endian byte_order,
					 LONGEST value);

/* Hook for `exec_file_command' command to call.  */

extern void (*deprecated_exec_file_display_hook) (const char *filename);

/* Hook for "file_command", which is more useful than above
   (because it is invoked AFTER symbols are read, not before).  */

extern void (*deprecated_file_changed_hook) (const char *filename);

extern void specify_exec_file_hook (void (*hook) (const char *filename));

/* Binary File Diddler for the core file.  */

#define core_bfd (current_program_space->cbfd.get ())

/* Whether to open exec and core files read-only or read-write.  */

extern bool write_files;

/* Open and set up the core file bfd.  */

extern void core_target_open (const char *arg, int from_tty);

extern void core_file_command (const char *filename, int from_tty);

extern void exec_file_attach (const char *filename, int from_tty);

/* If the filename of the main executable is unknown, attempt to
   determine it.  If a filename is determined, proceed as though
   it was just specified with the "file" command.  Do nothing if
   the filename of the main executable is already known.
   DEFER_BP_RESET uses SYMFILE_DEFER_BP_RESET for the main symbol file.  */

extern void exec_file_locate_attach (int pid, int defer_bp_reset, int from_tty);

extern void validate_files (void);

/* Give the user a message if the current exec file does not match the exec
   file determined from the target.  In case of mismatch, ask the user
   if the exec file determined from target must be loaded.  */
extern void validate_exec_file (int from_tty);

/* The current default bfd target.  */

extern const char *gnutarget;

extern void set_gnutarget (const char *);

/* Build either a single-thread or multi-threaded section name for
   PTID.

   If ptid's lwp member is zero, we want to do the single-threaded
   thing: look for a section named NAME (as passed to the
   constructor).  If ptid's lwp member is non-zero, we'll want do the
   multi-threaded thing: look for a section named "NAME/LWP", where
   LWP is the shortest ASCII decimal representation of ptid's lwp
   member.  */

class thread_section_name
{
public:
  /* NAME is the single-threaded section name.  If PTID represents an
     LWP, then the build section name is "NAME/LWP", otherwise it's
     just "NAME" unmodified.  */
  thread_section_name (const char *name, ptid_t ptid)
  {
    if (ptid.lwp_p ())
      {
	m_storage = string_printf ("%s/%ld", name, ptid.lwp ());
	m_section_name = m_storage.c_str ();
      }
    else
      m_section_name = name;
  }

  /* Return the computed section name.  The result is valid as long as
     this thread_section_name object is live.  */
  const char *c_str () const
  { return m_section_name; }

  DISABLE_COPY_AND_ASSIGN (thread_section_name);

private:
  /* Either a pointer into M_STORAGE, or a pointer to the name passed
     as parameter to the constructor.  */
  const char *m_section_name;
  /* If we need to build a new section name, this is where we store
     it.  */
  std::string m_storage;
};

#endif /* !defined (GDBCORE_H) */
