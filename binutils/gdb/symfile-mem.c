/* Reading symbol files from memory.

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

/* This file defines functions (and commands to exercise those
   functions) for reading debugging information from object files
   whose images are mapped directly into the inferior's memory.  For
   example, the Linux kernel maps a "syscall DSO" into each process's
   address space; this DSO provides kernel-specific code for some
   system calls.

   At the moment, BFD only has functions for parsing object files from
   memory for the ELF format, even though the general idea isn't
   ELF-specific.  This means that BFD only provides the functions GDB
   needs when configured for ELF-based targets.  So these functions
   may only be compiled on ELF-based targets.

   GDB has no idea whether it has been configured for an ELF-based
   target or not: it just tries to handle whatever files it is given.
   But this means there are no preprocessor symbols on which we could
   make these functions' compilation conditional.

   So, for the time being, we put these functions alone in this file,
   and have .mt files reference them as appropriate.  In the future, I
   hope BFD will provide a format-independent bfd_from_remote_memory
   entry point.  */


#include "defs.h"
#include "symtab.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "target.h"
#include "value.h"
#include "symfile.h"
#include "observable.h"
#include "auxv.h"
#include "elf/common.h"
#include "gdb_bfd.h"
#include "inferior.h"

/* Verify parameters of target_read_memory_bfd and target_read_memory are
   compatible.  */

static_assert (sizeof (CORE_ADDR) >= sizeof (bfd_vma));
static_assert (sizeof (gdb_byte) == sizeof (bfd_byte));
static_assert (sizeof (ssize_t) <= sizeof (bfd_size_type));

/* Provide bfd/ compatible prototype for target_read_memory.  Casting would not
   be enough as LEN width may differ.  */

static int
target_read_memory_bfd (bfd_vma memaddr, bfd_byte *myaddr, bfd_size_type len)
{
  /* MYADDR must be already allocated for the LEN size so it has to fit in
     ssize_t.  */
  gdb_assert ((ssize_t) len == len);

  return target_read_memory (memaddr, myaddr, len);
}

/* Read inferior memory at ADDR to find the header of a loaded object file
   and read its in-core symbols out of inferior memory.  SIZE, if
   non-zero, is the known size of the object.  TEMPL is a bfd
   representing the target's format.  NAME is the name to use for this
   symbol file in messages; it can be NULL.  */
static struct objfile *
symbol_file_add_from_memory (struct bfd *templ, CORE_ADDR addr,
			     size_t size, const char *name, int from_tty)
{
  struct objfile *objf;
  struct bfd *nbfd;
  struct bfd_section *sec;
  bfd_vma loadbase;
  symfile_add_flags add_flags = SYMFILE_NOT_FILENAME;

  if (bfd_get_flavour (templ) != bfd_target_elf_flavour)
    error (_("add-symbol-file-from-memory not supported for this target"));

  nbfd = bfd_elf_bfd_from_remote_memory (templ, addr, size, &loadbase,
					 target_read_memory_bfd);
  if (nbfd == NULL)
    error (_("Failed to read a valid object file image from memory."));

  /* Manage the new reference for the duration of this function.  */
  gdb_bfd_ref_ptr nbfd_holder = gdb_bfd_ref_ptr::new_reference (nbfd);

  if (name == NULL)
    name = "shared object read from target memory";
  bfd_set_filename (nbfd, name);

  if (!bfd_check_format (nbfd, bfd_object))
    error (_("Got object file from memory but can't read symbols: %s."),
	   bfd_errmsg (bfd_get_error ()));

  section_addr_info sai;
  for (sec = nbfd->sections; sec != NULL; sec = sec->next)
    if ((bfd_section_flags (sec) & (SEC_ALLOC|SEC_LOAD)) != 0)
      sai.emplace_back (bfd_section_vma (sec) + loadbase,
			bfd_section_name (sec),
			sec->index);

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  objf = symbol_file_add_from_bfd (nbfd_holder, bfd_get_filename (nbfd),
				   add_flags, &sai, OBJF_SHARED, NULL);

  current_program_space->add_target_sections (objf);

  /* This might change our ideas about frames already looked at.  */
  reinit_frame_cache ();

  return objf;
}


static void
add_symbol_file_from_memory_command (const char *args, int from_tty)
{
  CORE_ADDR addr;
  struct bfd *templ;

  if (args == NULL)
    error (_("add-symbol-file-from-memory requires an expression argument"));

  addr = parse_and_eval_address (args);

  /* We need some representative bfd to know the target we are looking at.  */
  if (current_program_space->symfile_object_file != NULL)
    templ = current_program_space->symfile_object_file->obfd.get ();
  else
    templ = current_program_space->exec_bfd ();
  if (templ == NULL)
    error (_("Must use symbol-file or exec-file "
	     "before add-symbol-file-from-memory."));

  symbol_file_add_from_memory (templ, addr, 0, NULL, from_tty);
}

/* Try to add the symbols for the vsyscall page, if there is one.
   This function is called via the inferior_created observer.  */

static void
add_vsyscall_page (inferior *inf)
{
  struct mem_range vsyscall_range;

  if (gdbarch_vsyscall_range (inf->arch (), &vsyscall_range))
    {
      struct bfd *bfd;

      if (core_bfd != NULL)
	bfd = core_bfd;
      else if (current_program_space->exec_bfd () != NULL)
	bfd = current_program_space->exec_bfd ();
      else
       /* FIXME: cagney/2004-05-06: Should not require an existing
	  BFD when trying to create a run-time BFD of the VSYSCALL
	  page in the inferior.  Unfortunately that's the current
	  interface so for the moment bail.  Introducing a
	  ``bfd_runtime'' (a BFD created using the loaded image) file
	  format should fix this.  */
	{
	  warning (_("Could not load vsyscall page "
		     "because no executable was specified"));
	  return;
	}

      std::string name = string_printf ("system-supplied DSO at %s",
					paddress (current_inferior ()->arch (),
						  vsyscall_range.start));
      try
	{
	  /* Pass zero for FROM_TTY, because the action of loading the
	     vsyscall DSO was not triggered by the user, even if the
	     user typed "run" at the TTY.  */
	  symbol_file_add_from_memory (bfd,
				       vsyscall_range.start,
				       vsyscall_range.length,
				       name.c_str (),
				       0 /* from_tty */);
	}
      catch (const gdb_exception_error &ex)
	{
	  exception_print (gdb_stderr, ex);
	}
    }
}

void _initialize_symfile_mem ();
void
_initialize_symfile_mem ()
{
  add_cmd ("add-symbol-file-from-memory", class_files,
	   add_symbol_file_from_memory_command,
	   _("Load the symbols out of memory from a "
	     "dynamically loaded object file.\n"
	     "Give an expression for the address "
	     "of the file's shared object file header."),
	   &cmdlist);

  /* Want to know of each new inferior so that its vsyscall info can
     be extracted.  */
  gdb::observers::inferior_created.attach (add_vsyscall_page, "symfile-mem");
}
