/* Program and address space management, for GDB, the GNU debugger.

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

#include "defs.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "arch-utils.h"
#include "gdbcore.h"
#include "solib.h"
#include "solist.h"
#include "gdbthread.h"
#include "inferior.h"
#include <algorithm>
#include "cli/cli-style.h"
#include "observable.h"

/* The last program space number assigned.  */
static int last_program_space_num = 0;

/* The head of the program spaces list.  */
std::vector<struct program_space *> program_spaces;

/* Pointer to the current program space.  */
struct program_space *current_program_space;

/* The last address space number assigned.  */
static int highest_address_space_num;



/* Create a new address space object, and add it to the list.  */

address_space::address_space ()
  : m_num (++highest_address_space_num)
{
}

/* Maybe create a new address space object, and add it to the list, or
   return a pointer to an existing address space, in case inferiors
   share an address space on this target system.  */

address_space_ref_ptr
maybe_new_address_space ()
{
  int shared_aspace
    = gdbarch_has_shared_address_space (current_inferior ()->arch ());

  if (shared_aspace)
    {
      /* Just return the first in the list.  */
      return program_spaces[0]->aspace;
    }

  return new_address_space ();
}

/* Start counting over from scratch.  */

static void
init_address_spaces (void)
{
  highest_address_space_num = 0;
}



/* Remove a program space from the program spaces list.  */

static void
remove_program_space (program_space *pspace)
{
  gdb_assert (pspace != NULL);

  auto iter = std::find (program_spaces.begin (), program_spaces.end (),
			 pspace);
  gdb_assert (iter != program_spaces.end ());
  program_spaces.erase (iter);
}

/* See progspace.h.  */

program_space::program_space (address_space_ref_ptr aspace_)
  : num (++last_program_space_num),
    aspace (std::move (aspace_))
{
  program_spaces.push_back (this);
  gdb::observers::new_program_space.notify (this);
}

/* See progspace.h.  */

program_space::~program_space ()
{
  gdb_assert (this != current_program_space);

  gdb::observers::free_program_space.notify (this);
  remove_program_space (this);

  scoped_restore_current_program_space restore_pspace;

  set_current_program_space (this);

  breakpoint_program_space_exit (this);
  no_shared_libraries (NULL, 0);
  free_all_objfiles ();
  /* Defer breakpoint re-set because we don't want to create new
     locations for this pspace which we're tearing down.  */
  clear_symtab_users (SYMFILE_DEFER_BP_RESET);
}

/* See progspace.h.  */

void
program_space::free_all_objfiles ()
{
  /* Any objfile reference would become stale.  */
  for (const shobj &so : current_program_space->solibs ())
    gdb_assert (so.objfile == NULL);

  while (!objfiles_list.empty ())
    objfiles_list.front ()->unlink ();
}

/* See progspace.h.  */

void
program_space::add_objfile (std::unique_ptr<objfile> &&objfile,
			    struct objfile *before)
{
  if (before == nullptr)
    objfiles_list.push_back (std::move (objfile));
  else
    {
      auto iter = std::find_if (objfiles_list.begin (), objfiles_list.end (),
				[=] (const std::unique_ptr<::objfile> &objf)
				{
				  return objf.get () == before;
				});
      gdb_assert (iter != objfiles_list.end ());
      objfiles_list.insert (iter, std::move (objfile));
    }
}

/* See progspace.h.  */

void
program_space::remove_objfile (struct objfile *objfile)
{
  /* Removing an objfile from the objfile list invalidates any frame
     that was built using frame info found in the objfile.  Reinit the
     frame cache to get rid of any frame that might otherwise
     reference stale info.  */
  reinit_frame_cache ();

  auto iter = std::find_if (objfiles_list.begin (), objfiles_list.end (),
			    [=] (const std::unique_ptr<::objfile> &objf)
			    {
			      return objf.get () == objfile;
			    });
  gdb_assert (iter != objfiles_list.end ());
  objfiles_list.erase (iter);

  if (objfile == symfile_object_file)
    symfile_object_file = NULL;
}

/* See progspace.h.  */

struct objfile *
program_space::objfile_for_address (CORE_ADDR address)
{
  for (auto iter : objfiles ())
    {
      /* Don't check separate debug objfiles.  */
      if (iter->separate_debug_objfile_backlink != nullptr)
	continue;
      if (is_addr_in_objfile (address, iter))
	return iter;
    }
  return nullptr;
}

/* See progspace.h.  */

void
program_space::exec_close ()
{
  if (ebfd != nullptr)
    {
      /* Removing target sections may close the exec_ops target.
	 Clear ebfd before doing so to prevent recursion.  */
      bfd *saved_ebfd = ebfd.get ();
      ebfd.reset (nullptr);
      ebfd_mtime = 0;

      remove_target_sections (saved_ebfd);

      exec_filename.reset (nullptr);
    }
}

/* Copies program space SRC to DEST.  Copies the main executable file,
   and the main symbol file.  Returns DEST.  */

struct program_space *
clone_program_space (struct program_space *dest, struct program_space *src)
{
  scoped_restore_current_program_space restore_pspace;

  set_current_program_space (dest);

  if (src->exec_filename != NULL)
    exec_file_attach (src->exec_filename.get (), 0);

  if (src->symfile_object_file != NULL)
    symbol_file_add_main (objfile_name (src->symfile_object_file),
			  SYMFILE_DEFER_BP_RESET);

  return dest;
}

/* Sets PSPACE as the current program space.  It is the caller's
   responsibility to make sure that the currently selected
   inferior/thread matches the selected program space.  */

void
set_current_program_space (struct program_space *pspace)
{
  if (current_program_space == pspace)
    return;

  gdb_assert (pspace != NULL);

  current_program_space = pspace;

  /* Different symbols change our view of the frame chain.  */
  reinit_frame_cache ();
}

/* Returns true iff there's no inferior bound to PSPACE.  */

bool
program_space::empty ()
{
  return find_inferior_for_program_space (this) == nullptr;
}

/* Prints the list of program spaces and their details on UIOUT.  If
   REQUESTED is not -1, it's the ID of the pspace that should be
   printed.  Otherwise, all spaces are printed.  */

static void
print_program_space (struct ui_out *uiout, int requested)
{
  int count = 0;

  /* Start with a minimum width of 17 for the executable name column.  */
  size_t longest_exec_name = 17;

  /* Compute number of pspaces we will print.  */
  for (struct program_space *pspace : program_spaces)
    {
      if (requested != -1 && pspace->num != requested)
	continue;

      if (pspace->exec_filename != nullptr)
	longest_exec_name = std::max (strlen (pspace->exec_filename.get ()),
				      longest_exec_name);

      ++count;
    }

  /* There should always be at least one.  */
  gdb_assert (count > 0);

  ui_out_emit_table table_emitter (uiout, 4, count, "pspaces");
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (4, ui_left, "id", "Id");
  uiout->table_header (longest_exec_name, ui_left, "exec", "Executable");
  uiout->table_header (17, ui_left, "core", "Core File");
  uiout->table_body ();

  for (struct program_space *pspace : program_spaces)
    {
      int printed_header;

      if (requested != -1 && requested != pspace->num)
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      if (pspace == current_program_space)
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      uiout->field_signed ("id", pspace->num);

      if (pspace->exec_filename != nullptr)
	uiout->field_string ("exec", pspace->exec_filename.get (),
			     file_name_style.style ());
      else
	uiout->field_skip ("exec");

      if (pspace->cbfd != nullptr)
	uiout->field_string ("core", bfd_get_filename (pspace->cbfd.get ()),
			     file_name_style.style ());
      else
	uiout->field_skip ("core");

      /* Print extra info that doesn't really fit in tabular form.
	 Currently, we print the list of inferiors bound to a pspace.
	 There can be more than one inferior bound to the same pspace,
	 e.g., both parent/child inferiors in a vfork, or, on targets
	 that share pspaces between inferiors.  */
      printed_header = 0;

      /* We're going to switch inferiors.  */
      scoped_restore_current_thread restore_thread;

      for (inferior *inf : all_inferiors ())
	if (inf->pspace == pspace)
	  {
	    /* Switch to inferior in order to call target methods.  */
	    switch_to_inferior_no_thread (inf);

	    if (!printed_header)
	      {
		printed_header = 1;
		gdb_printf ("\n\tBound inferiors: ID %d (%s)",
			    inf->num,
			    target_pid_to_str (ptid_t (inf->pid)).c_str ());
	      }
	    else
	      gdb_printf (", ID %d (%s)",
			  inf->num,
			  target_pid_to_str (ptid_t (inf->pid)).c_str ());
	  }

      uiout->text ("\n");
    }
}

/* Boolean test for an already-known program space id.  */

static int
valid_program_space_id (int num)
{
  for (struct program_space *pspace : program_spaces)
    if (pspace->num == num)
      return 1;

  return 0;
}

/* If ARGS is NULL or empty, print information about all program
   spaces.  Otherwise, ARGS is a text representation of a LONG
   indicating which the program space to print information about.  */

static void
maintenance_info_program_spaces_command (const char *args, int from_tty)
{
  int requested = -1;

  if (args && *args)
    {
      requested = parse_and_eval_long (args);
      if (!valid_program_space_id (requested))
	error (_("program space ID %d not known."), requested);
    }

  print_program_space (current_uiout, requested);
}

/* Update all program spaces matching to address spaces.  The user may
   have created several program spaces, and loaded executables into
   them before connecting to the target interface that will create the
   inferiors.  All that happens before GDB has a chance to know if the
   inferiors will share an address space or not.  Call this after
   having connected to the target interface and having fetched the
   target description, to fixup the program/address spaces mappings.

   It is assumed that there are no bound inferiors yet, otherwise,
   they'd be left with stale referenced to released aspaces.  */

void
update_address_spaces (void)
{
  int shared_aspace
    = gdbarch_has_shared_address_space (current_inferior ()->arch ());

  init_address_spaces ();

  if (shared_aspace)
    {
      address_space_ref_ptr aspace = new_address_space ();

      for (struct program_space *pspace : program_spaces)
	pspace->aspace = aspace;
    }
  else
    for (struct program_space *pspace : program_spaces)
      pspace->aspace = new_address_space ();

  for (inferior *inf : all_inferiors ())
    if (gdbarch_has_global_solist (current_inferior ()->arch ()))
      inf->aspace = maybe_new_address_space ();
    else
      inf->aspace = inf->pspace->aspace;
}



/* See progspace.h.  */

void
program_space::clear_solib_cache ()
{
  added_solibs.clear ();
  deleted_solibs.clear ();
}



void
initialize_progspace (void)
{
  add_cmd ("program-spaces", class_maintenance,
	   maintenance_info_program_spaces_command,
	   _("Info about currently known program spaces."),
	   &maintenanceinfolist);

  /* There's always one program space.  Note that this function isn't
     an automatic _initialize_foo function, since other
     _initialize_foo routines may need to install their per-pspace
     data keys.  We can only allocate a progspace when all those
     modules have done that.  Do this before
     initialize_current_architecture, because that accesses the ebfd
     of current_program_space.  */
  current_program_space = new program_space (new_address_space ());
}
