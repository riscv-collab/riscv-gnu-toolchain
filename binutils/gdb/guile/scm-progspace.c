/* Guile interface to program spaces.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "charset.h"
#include "progspace.h"
#include "objfiles.h"
#include "language.h"
#include "arch-utils.h"
#include "guile-internal.h"

/* NOTE: Python exports the name "Progspace", so we export "progspace".
   Internally we shorten that to "pspace".  */

/* The <gdb:progspace> smob.  */

struct pspace_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The corresponding pspace.  */
  struct program_space *pspace;

  /* The pretty-printer list of functions.  */
  SCM pretty_printers;

  /* The <gdb:progspace> object we are contained in, needed to
     protect/unprotect the object since a reference to it comes from
     non-gc-managed space (the progspace).  */
  SCM containing_scm;
};

static const char pspace_smob_name[] = "gdb:progspace";

/* The tag Guile knows the pspace smob by.  */
static scm_t_bits pspace_smob_tag;

/* Progspace registry cleanup handler for when a progspace is deleted.  */
struct psscm_deleter
{
  void operator() (pspace_smob *p_smob)
  {
    p_smob->pspace = NULL;
    scm_gc_unprotect_object (p_smob->containing_scm);
  }
};

static const registry<program_space>::key<pspace_smob, psscm_deleter>
     psscm_pspace_data_key;

/* Return the list of pretty-printers registered with P_SMOB.  */

SCM
psscm_pspace_smob_pretty_printers (const pspace_smob *p_smob)
{
  return p_smob->pretty_printers;
}

/* Administrivia for progspace smobs.  */

/* The smob "print" function for <gdb:progspace>.  */

static int
psscm_print_pspace_smob (SCM self, SCM port, scm_print_state *pstate)
{
  pspace_smob *p_smob = (pspace_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", pspace_smob_name);
  if (p_smob->pspace != NULL)
    {
      struct objfile *objfile = p_smob->pspace->symfile_object_file;

      gdbscm_printf (port, "%s",
		     objfile != NULL
		     ? objfile_name (objfile)
		     : "{no symfile}");
    }
  else
    scm_puts ("{invalid}", port);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:progspace> object.
   It's empty in the sense that a progspace still needs to be associated
   with it.  */

static SCM
psscm_make_pspace_smob (void)
{
  pspace_smob *p_smob = (pspace_smob *)
    scm_gc_malloc (sizeof (pspace_smob), pspace_smob_name);
  SCM p_scm;

  p_smob->pspace = NULL;
  p_smob->pretty_printers = SCM_EOL;
  p_scm = scm_new_smob (pspace_smob_tag, (scm_t_bits) p_smob);
  p_smob->containing_scm = p_scm;
  gdbscm_init_gsmob (&p_smob->base);

  return p_scm;
}

/* Return non-zero if SCM is a <gdb:progspace> object.  */

static int
psscm_is_pspace (SCM scm)
{
  return SCM_SMOB_PREDICATE (pspace_smob_tag, scm);
}

/* (progspace? object) -> boolean */

static SCM
gdbscm_progspace_p (SCM scm)
{
  return scm_from_bool (psscm_is_pspace (scm));
}

/* Return a pointer to the progspace_smob that encapsulates PSPACE,
   creating one if necessary.
   The result is cached so that we have only one copy per objfile.  */

pspace_smob *
psscm_pspace_smob_from_pspace (struct program_space *pspace)
{
  pspace_smob *p_smob;

  p_smob = psscm_pspace_data_key.get (pspace);
  if (p_smob == NULL)
    {
      SCM p_scm = psscm_make_pspace_smob ();

      p_smob = (pspace_smob *) SCM_SMOB_DATA (p_scm);
      p_smob->pspace = pspace;

      psscm_pspace_data_key.set (pspace, p_smob);
      scm_gc_protect_object (p_smob->containing_scm);
    }

  return p_smob;
}

/* Return the <gdb:progspace> object that encapsulates PSPACE.  */

SCM
psscm_scm_from_pspace (struct program_space *pspace)
{
  pspace_smob *p_smob = psscm_pspace_smob_from_pspace (pspace);

  return p_smob->containing_scm;
}

/* Returns the <gdb:progspace> object in SELF.
   Throws an exception if SELF is not a <gdb:progspace> object.  */

static SCM
psscm_get_pspace_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (psscm_is_pspace (self), self, arg_pos, func_name,
		   pspace_smob_name);

  return self;
}

/* Returns a pointer to the pspace smob of SELF.
   Throws an exception if SELF is not a <gdb:progspace> object.  */

static pspace_smob *
psscm_get_pspace_smob_arg_unsafe (SCM self, int arg_pos,
				  const char *func_name)
{
  SCM p_scm = psscm_get_pspace_arg_unsafe (self, arg_pos, func_name);
  pspace_smob *p_smob = (pspace_smob *) SCM_SMOB_DATA (p_scm);

  return p_smob;
}

/* Return non-zero if pspace P_SMOB is valid.  */

static int
psscm_is_valid (pspace_smob *p_smob)
{
  return p_smob->pspace != NULL;
}

/* Return the pspace smob in SELF, verifying it's valid.
   Throws an exception if SELF is not a <gdb:progspace> object or is
   invalid.  */

static pspace_smob *
psscm_get_valid_pspace_smob_arg_unsafe (SCM self, int arg_pos,
					const char *func_name)
{
  pspace_smob *p_smob
    = psscm_get_pspace_smob_arg_unsafe (self, arg_pos, func_name);

  if (!psscm_is_valid (p_smob))
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("<gdb:progspace>"));
    }

  return p_smob;
}

/* Program space methods.  */

/* (progspace-valid? <gdb:progspace>) -> boolean
   Returns #t if this program space still exists in GDB.  */

static SCM
gdbscm_progspace_valid_p (SCM self)
{
  pspace_smob *p_smob
    = psscm_get_pspace_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (p_smob->pspace != NULL);
}

/* (progspace-filename <gdb:progspace>) -> string
   Returns the name of the main symfile associated with the progspace,
   or #f if there isn't one.
   Throw's an exception if the underlying pspace is invalid.  */

static SCM
gdbscm_progspace_filename (SCM self)
{
  pspace_smob *p_smob
    = psscm_get_valid_pspace_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct objfile *objfile = p_smob->pspace->symfile_object_file;

  if (objfile != NULL)
    return gdbscm_scm_from_c_string (objfile_name (objfile));
  return SCM_BOOL_F;
}

/* (progspace-objfiles <gdb:progspace>) -> list
   Return the list of objfiles in the progspace.
   Objfiles that are separate debug objfiles are *not* included in the result,
   only the "original/real" one appears in the result.
   The order of appearance of objfiles in the result is arbitrary.
   Throw's an exception if the underlying pspace is invalid.

   Some apps can have 1000s of shared libraries.  Seriously.
   A future extension here could be to provide, e.g., a regexp to select
   just the ones the caller is interested in (rather than building the list
   and then selecting the desired ones).  Another alternative is passing a
   predicate, then the filter criteria can be more general.  */

static SCM
gdbscm_progspace_objfiles (SCM self)
{
  pspace_smob *p_smob
    = psscm_get_valid_pspace_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  SCM result;

  result = SCM_EOL;

  for (objfile *objfile : p_smob->pspace->objfiles ())
    {
      if (objfile->separate_debug_objfile_backlink == NULL)
	{
	  SCM item = ofscm_scm_from_objfile (objfile);

	  result = scm_cons (item, result);
	}
    }

  /* We don't really have to return the list in the same order as recorded
     internally, but for consistency we do.  We still advertise that one
     cannot assume anything about the order.  */
  return scm_reverse_x (result, SCM_EOL);
}

/* (progspace-pretty-printers <gdb:progspace>) -> list
   Returns the list of pretty-printers for this program space.  */

static SCM
gdbscm_progspace_pretty_printers (SCM self)
{
  pspace_smob *p_smob
    = psscm_get_pspace_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return p_smob->pretty_printers;
}

/* (set-progspace-pretty-printers! <gdb:progspace> list) -> unspecified
   Set the pretty-printers for this program space.  */

static SCM
gdbscm_set_progspace_pretty_printers_x (SCM self, SCM printers)
{
  pspace_smob *p_smob
    = psscm_get_pspace_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  SCM_ASSERT_TYPE (gdbscm_is_true (scm_list_p (printers)), printers,
		   SCM_ARG2, FUNC_NAME, _("list"));

  p_smob->pretty_printers = printers;

  return SCM_UNSPECIFIED;
}

/* (current-progspace) -> <gdb:progspace>
   Return the current program space.  There always is one.  */

static SCM
gdbscm_current_progspace (void)
{
  SCM result;

  result = psscm_scm_from_pspace (current_program_space);

  return result;
}

/* (progspaces) -> list
   Return a list of all progspaces.  */

static SCM
gdbscm_progspaces (void)
{
  SCM result;

  result = SCM_EOL;

  for (struct program_space *ps : program_spaces)
    {
      SCM item = psscm_scm_from_pspace (ps);

      result = scm_cons (item, result);
    }

  return scm_reverse_x (result, SCM_EOL);
}

/* Initialize the Scheme program space support.  */

static const scheme_function pspace_functions[] =
{
  { "progspace?", 1, 0, 0, as_a_scm_t_subr (gdbscm_progspace_p),
    "\
Return #t if the object is a <gdb:objfile> object." },

  { "progspace-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_progspace_valid_p),
    "\
Return #t if the progspace is valid (hasn't been deleted from gdb)." },

  { "progspace-filename", 1, 0, 0, as_a_scm_t_subr (gdbscm_progspace_filename),
    "\
Return the name of the main symbol file of the progspace." },

  { "progspace-objfiles", 1, 0, 0, as_a_scm_t_subr (gdbscm_progspace_objfiles),
    "\
Return the list of objfiles associated with the progspace.\n\
Objfiles that are separate debug objfiles are not included in the result.\n\
The order of appearance of objfiles in the result is arbitrary." },

  { "progspace-pretty-printers", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_progspace_pretty_printers),
    "\
Return a list of pretty-printers of the progspace." },

  { "set-progspace-pretty-printers!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_progspace_pretty_printers_x),
    "\
Set the list of pretty-printers of the progspace." },

  { "current-progspace", 0, 0, 0, as_a_scm_t_subr (gdbscm_current_progspace),
    "\
Return the current program space if there is one or #f if there isn't one." },

  { "progspaces", 0, 0, 0, as_a_scm_t_subr (gdbscm_progspaces),
    "\
Return a list of all program spaces." },

  END_FUNCTIONS
};

void
gdbscm_initialize_pspaces (void)
{
  pspace_smob_tag
    = gdbscm_make_smob_type (pspace_smob_name, sizeof (pspace_smob));
  scm_set_smob_print (pspace_smob_tag, psscm_print_pspace_smob);

  gdbscm_define_functions (pspace_functions, 1);
}
