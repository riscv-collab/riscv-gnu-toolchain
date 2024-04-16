/* Scheme interface to objfiles.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "objfiles.h"
#include "language.h"
#include "guile-internal.h"

/* The <gdb:objfile> smob.  */

struct objfile_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The corresponding objfile.  */
  struct objfile *objfile;

  /* The pretty-printer list of functions.  */
  SCM pretty_printers;

  /* The <gdb:objfile> object we are contained in, needed to protect/unprotect
     the object since a reference to it comes from non-gc-managed space
     (the objfile).  */
  SCM containing_scm;
};

static const char objfile_smob_name[] = "gdb:objfile";

/* The tag Guile knows the objfile smob by.  */
static scm_t_bits objfile_smob_tag;

/* Objfile registry cleanup handler for when an objfile is deleted.  */
struct ofscm_deleter
{
  void operator() (objfile_smob *o_smob)
  {
    o_smob->objfile = NULL;
    scm_gc_unprotect_object (o_smob->containing_scm);
  }
};

static const registry<objfile>::key<objfile_smob, ofscm_deleter>
     ofscm_objfile_data_key;

/* Return the list of pretty-printers registered with O_SMOB.  */

SCM
ofscm_objfile_smob_pretty_printers (objfile_smob *o_smob)
{
  return o_smob->pretty_printers;
}

/* Administrivia for objfile smobs.  */

/* The smob "print" function for <gdb:objfile>.  */

static int
ofscm_print_objfile_smob (SCM self, SCM port, scm_print_state *pstate)
{
  objfile_smob *o_smob = (objfile_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", objfile_smob_name);
  gdbscm_printf (port, "%s",
		 o_smob->objfile != NULL
		 ? objfile_name (o_smob->objfile)
		 : "{invalid}");
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:objfile> object.
   It's empty in the sense that an OBJFILE still needs to be associated
   with it.  */

static SCM
ofscm_make_objfile_smob (void)
{
  objfile_smob *o_smob = (objfile_smob *)
    scm_gc_malloc (sizeof (objfile_smob), objfile_smob_name);
  SCM o_scm;

  o_smob->objfile = NULL;
  o_smob->pretty_printers = SCM_EOL;
  o_scm = scm_new_smob (objfile_smob_tag, (scm_t_bits) o_smob);
  o_smob->containing_scm = o_scm;
  gdbscm_init_gsmob (&o_smob->base);

  return o_scm;
}

/* Return non-zero if SCM is a <gdb:objfile> object.  */

static int
ofscm_is_objfile (SCM scm)
{
  return SCM_SMOB_PREDICATE (objfile_smob_tag, scm);
}

/* (objfile? object) -> boolean */

static SCM
gdbscm_objfile_p (SCM scm)
{
  return scm_from_bool (ofscm_is_objfile (scm));
}

/* Return a pointer to the objfile_smob that encapsulates OBJFILE,
   creating one if necessary.
   The result is cached so that we have only one copy per objfile.  */

objfile_smob *
ofscm_objfile_smob_from_objfile (struct objfile *objfile)
{
  objfile_smob *o_smob;

  o_smob = ofscm_objfile_data_key.get (objfile);
  if (o_smob == NULL)
    {
      SCM o_scm = ofscm_make_objfile_smob ();

      o_smob = (objfile_smob *) SCM_SMOB_DATA (o_scm);
      o_smob->objfile = objfile;

      ofscm_objfile_data_key.set (objfile, o_smob);
      scm_gc_protect_object (o_smob->containing_scm);
    }

  return o_smob;
}

/* Return the <gdb:objfile> object that encapsulates OBJFILE.  */

SCM
ofscm_scm_from_objfile (struct objfile *objfile)
{
  objfile_smob *o_smob = ofscm_objfile_smob_from_objfile (objfile);

  return o_smob->containing_scm;
}

/* Returns the <gdb:objfile> object in SELF.
   Throws an exception if SELF is not a <gdb:objfile> object.  */

static SCM
ofscm_get_objfile_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (ofscm_is_objfile (self), self, arg_pos, func_name,
		   objfile_smob_name);

  return self;
}

/* Returns a pointer to the objfile smob of SELF.
   Throws an exception if SELF is not a <gdb:objfile> object.  */

static objfile_smob *
ofscm_get_objfile_smob_arg_unsafe (SCM self, int arg_pos,
				   const char *func_name)
{
  SCM o_scm = ofscm_get_objfile_arg_unsafe (self, arg_pos, func_name);
  objfile_smob *o_smob = (objfile_smob *) SCM_SMOB_DATA (o_scm);

  return o_smob;
}

/* Return non-zero if objfile O_SMOB is valid.  */

static int
ofscm_is_valid (objfile_smob *o_smob)
{
  return o_smob->objfile != NULL;
}

/* Return the objfile smob in SELF, verifying it's valid.
   Throws an exception if SELF is not a <gdb:objfile> object or is invalid.  */

static objfile_smob *
ofscm_get_valid_objfile_smob_arg_unsafe (SCM self, int arg_pos,
					 const char *func_name)
{
  objfile_smob *o_smob
    = ofscm_get_objfile_smob_arg_unsafe (self, arg_pos, func_name);

  if (!ofscm_is_valid (o_smob))
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("<gdb:objfile>"));
    }

  return o_smob;
}

/* Objfile methods.  */

/* (objfile-valid? <gdb:objfile>) -> boolean
   Returns #t if this object file still exists in GDB.  */

static SCM
gdbscm_objfile_valid_p (SCM self)
{
  objfile_smob *o_smob
    = ofscm_get_objfile_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (o_smob->objfile != NULL);
}

/* (objfile-filename <gdb:objfile>) -> string
   Returns the objfile's file name.
   Throw's an exception if the underlying objfile is invalid.  */

static SCM
gdbscm_objfile_filename (SCM self)
{
  objfile_smob *o_smob
    = ofscm_get_valid_objfile_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return gdbscm_scm_from_c_string (objfile_name (o_smob->objfile));
}

/* (objfile-progspace <gdb:objfile>) -> <gdb:progspace>
   Returns the objfile's progspace.
   Throw's an exception if the underlying objfile is invalid.  */

static SCM
gdbscm_objfile_progspace (SCM self)
{
  objfile_smob *o_smob
    = ofscm_get_valid_objfile_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return psscm_scm_from_pspace (o_smob->objfile->pspace);
}

/* (objfile-pretty-printers <gdb:objfile>) -> list
   Returns the list of pretty-printers for this objfile.  */

static SCM
gdbscm_objfile_pretty_printers (SCM self)
{
  objfile_smob *o_smob
    = ofscm_get_objfile_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return o_smob->pretty_printers;
}

/* (set-objfile-pretty-printers! <gdb:objfile> list) -> unspecified
   Set the pretty-printers for this objfile.  */

static SCM
gdbscm_set_objfile_pretty_printers_x (SCM self, SCM printers)
{
  objfile_smob *o_smob
    = ofscm_get_objfile_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  SCM_ASSERT_TYPE (gdbscm_is_true (scm_list_p (printers)), printers,
		   SCM_ARG2, FUNC_NAME, _("list"));

  o_smob->pretty_printers = printers;

  return SCM_UNSPECIFIED;
}

/* The "current" objfile.  This is set when gdb detects that a new
   objfile has been loaded.  It is only set for the duration of a call to
   gdbscm_source_objfile_script and gdbscm_execute_objfile_script; it is NULL
   at other times.  */
static struct objfile *ofscm_current_objfile;

/* Set the current objfile to OBJFILE and then read FILE named FILENAME
   as Guile code.  This does not throw any errors.  If an exception
   occurs Guile will print the backtrace.
   This is the extension_language_script_ops.objfile_script_sourcer
   "method".  */

void
gdbscm_source_objfile_script (const struct extension_language_defn *extlang,
			      struct objfile *objfile, FILE *file,
			      const char *filename)
{
  ofscm_current_objfile = objfile;

  gdb::unique_xmalloc_ptr<char> msg = gdbscm_safe_source_script (filename);
  if (msg != NULL)
    gdb_printf (gdb_stderr, "%s", msg.get ());

  ofscm_current_objfile = NULL;
}

/* Set the current objfile to OBJFILE and then read FILE named FILENAME
   as Guile code.  This does not throw any errors.  If an exception
   occurs Guile will print the backtrace.
   This is the extension_language_script_ops.objfile_script_sourcer
   "method".  */

void
gdbscm_execute_objfile_script (const struct extension_language_defn *extlang,
			       struct objfile *objfile, const char *name,
			       const char *script)
{
  ofscm_current_objfile = objfile;

  gdb::unique_xmalloc_ptr<char> msg
    = gdbscm_safe_eval_string (script, 0 /* display_result */);
  if (msg != NULL)
    gdb_printf (gdb_stderr, "%s", msg.get ());

  ofscm_current_objfile = NULL;
}

/* (current-objfile) -> <gdb:objfile>
   Return the current objfile, or #f if there isn't one.
   Ideally this would be named ofscm_current_objfile, but that name is
   taken by the variable recording the current objfile.  */

static SCM
gdbscm_get_current_objfile (void)
{
  if (ofscm_current_objfile == NULL)
    return SCM_BOOL_F;

  return ofscm_scm_from_objfile (ofscm_current_objfile);
}

/* (objfiles) -> list
   Return a list of all objfiles in the current program space.  */

static SCM
gdbscm_objfiles (void)
{
  SCM result;

  result = SCM_EOL;

  for (objfile *objf : current_program_space->objfiles ())
    {
      SCM item = ofscm_scm_from_objfile (objf);

      result = scm_cons (item, result);
    }

  return scm_reverse_x (result, SCM_EOL);
}

/* Initialize the Scheme objfile support.  */

static const scheme_function objfile_functions[] =
{
  { "objfile?", 1, 0, 0, as_a_scm_t_subr (gdbscm_objfile_p),
    "\
Return #t if the object is a <gdb:objfile> object." },

  { "objfile-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_objfile_valid_p),
    "\
Return #t if the objfile is valid (hasn't been deleted from gdb)." },

  { "objfile-filename", 1, 0, 0, as_a_scm_t_subr (gdbscm_objfile_filename),
    "\
Return the file name of the objfile." },

  { "objfile-progspace", 1, 0, 0, as_a_scm_t_subr (gdbscm_objfile_progspace),
    "\
Return the progspace that the objfile lives in." },

  { "objfile-pretty-printers", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_objfile_pretty_printers),
    "\
Return a list of pretty-printers of the objfile." },

  { "set-objfile-pretty-printers!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_objfile_pretty_printers_x),
    "\
Set the list of pretty-printers of the objfile." },

  { "current-objfile", 0, 0, 0, as_a_scm_t_subr (gdbscm_get_current_objfile),
    "\
Return the current objfile if there is one or #f if there isn't one." },

  { "objfiles", 0, 0, 0, as_a_scm_t_subr (gdbscm_objfiles),
    "\
Return a list of all objfiles in the current program space." },

  END_FUNCTIONS
};

void
gdbscm_initialize_objfiles (void)
{
  objfile_smob_tag
    = gdbscm_make_smob_type (objfile_smob_name, sizeof (objfile_smob));
  scm_set_smob_print (objfile_smob_tag, ofscm_print_objfile_smob);

  gdbscm_define_functions (objfile_functions, 1);
}
