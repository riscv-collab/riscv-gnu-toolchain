/* GDB/Scheme pretty-printing.

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
#include "top.h"
#include "charset.h"
#include "symtab.h"
#include "language.h"
#include "objfiles.h"
#include "value.h"
#include "valprint.h"
#include "guile-internal.h"

/* Return type of print_string_repr.  */

enum guile_string_repr_result
{
  /* The string method returned None.  */
  STRING_REPR_NONE,
  /* The string method had an error.  */
  STRING_REPR_ERROR,
  /* Everything ok.  */
  STRING_REPR_OK
};

/* Display hints.  */

enum display_hint
{
  /* No display hint.  */
  HINT_NONE,
  /* The display hint has a bad value.  */
  HINT_ERROR,
  /* Print as an array.  */
  HINT_ARRAY,
  /* Print as a map.  */
  HINT_MAP,
  /* Print as a string.  */
  HINT_STRING
};

/* The <gdb:pretty-printer> smob.  */

struct pretty_printer_smob
{
  /* This must appear first.  */
  gdb_smob base;

  /* A string representing the name of the printer.  */
  SCM name;

  /* A boolean indicating whether the printer is enabled.  */
  SCM enabled;

  /* A procedure called to look up the printer for the given value.
     The procedure is called as (lookup gdb:pretty-printer value).
     The result should either be a gdb:pretty-printer object that will print
     the value, or #f if the value is not recognized.  */     
  SCM lookup;

  /* Note: Attaching subprinters to this smob is left to Scheme.  */
};

/* The <gdb:pretty-printer-worker> smob.  */

struct pretty_printer_worker_smob
{
  /* This must appear first.  */
  gdb_smob base;

  /* Either #f or one of the supported display hints: map, array, string.
     If neither of those then the display hint is ignored (treated as #f).  */
  SCM display_hint;

  /* A procedure called to pretty-print the value.
     (lambda (printer) ...) -> string | <gdb:lazy-string> | <gdb:value>  */
  SCM to_string;

  /* A procedure called to print children of the value.
     (lambda (printer) ...) -> <gdb:iterator>
     The iterator returns a pair for each iteration: (name . value),
     where "value" can have the same types as to_string.  */
  SCM children;
};

static const char pretty_printer_smob_name[] =
  "gdb:pretty-printer";
static const char pretty_printer_worker_smob_name[] =
  "gdb:pretty-printer-worker";

/* The tag Guile knows the pretty-printer smobs by.  */
static scm_t_bits pretty_printer_smob_tag;
static scm_t_bits pretty_printer_worker_smob_tag;

/* The global pretty-printer list.  */
static SCM pretty_printer_list;

/* gdb:pp-type-error.  */
static SCM pp_type_error_symbol;

/* Pretty-printer display hints are specified by strings.  */
static SCM ppscm_map_string;
static SCM ppscm_array_string;
static SCM ppscm_string_string;

/* Administrivia for pretty-printer matcher smobs.  */

/* The smob "print" function for <gdb:pretty-printer>.  */

static int
ppscm_print_pretty_printer_smob (SCM self, SCM port, scm_print_state *pstate)
{
  pretty_printer_smob *pp_smob = (pretty_printer_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", pretty_printer_smob_name);
  scm_write (pp_smob->name, port);
  scm_puts (gdbscm_is_true (pp_smob->enabled) ? " enabled" : " disabled",
	    port);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* (make-pretty-printer string procedure) -> <gdb:pretty-printer> */

static SCM
gdbscm_make_pretty_printer (SCM name, SCM lookup)
{
  pretty_printer_smob *pp_smob = (pretty_printer_smob *)
    scm_gc_malloc (sizeof (pretty_printer_smob),
		   pretty_printer_smob_name);
  SCM smob;

  SCM_ASSERT_TYPE (scm_is_string (name), name, SCM_ARG1, FUNC_NAME,
		   _("string"));
  SCM_ASSERT_TYPE (gdbscm_is_procedure (lookup), lookup, SCM_ARG2, FUNC_NAME,
		   _("procedure"));

  pp_smob->name = name;
  pp_smob->lookup = lookup;
  pp_smob->enabled = SCM_BOOL_T;
  smob = scm_new_smob (pretty_printer_smob_tag, (scm_t_bits) pp_smob);
  gdbscm_init_gsmob (&pp_smob->base);

  return smob;
}

/* Return non-zero if SCM is a <gdb:pretty-printer> object.  */

static int
ppscm_is_pretty_printer (SCM scm)
{
  return SCM_SMOB_PREDICATE (pretty_printer_smob_tag, scm);
}

/* (pretty-printer? object) -> boolean */

static SCM
gdbscm_pretty_printer_p (SCM scm)
{
  return scm_from_bool (ppscm_is_pretty_printer (scm));
}

/* Returns the <gdb:pretty-printer> object in SELF.
   Throws an exception if SELF is not a <gdb:pretty-printer> object.  */

static SCM
ppscm_get_pretty_printer_arg_unsafe (SCM self, int arg_pos,
				     const char *func_name)
{
  SCM_ASSERT_TYPE (ppscm_is_pretty_printer (self), self, arg_pos, func_name,
		   pretty_printer_smob_name);

  return self;
}

/* Returns a pointer to the pretty-printer smob of SELF.
   Throws an exception if SELF is not a <gdb:pretty-printer> object.  */

static pretty_printer_smob *
ppscm_get_pretty_printer_smob_arg_unsafe (SCM self, int arg_pos,
					  const char *func_name)
{
  SCM pp_scm = ppscm_get_pretty_printer_arg_unsafe (self, arg_pos, func_name);
  pretty_printer_smob *pp_smob
    = (pretty_printer_smob *) SCM_SMOB_DATA (pp_scm);

  return pp_smob;
}

/* Pretty-printer methods.  */

/* (pretty-printer-enabled? <gdb:pretty-printer>) -> boolean */

static SCM
gdbscm_pretty_printer_enabled_p (SCM self)
{
  pretty_printer_smob *pp_smob
    = ppscm_get_pretty_printer_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return pp_smob->enabled;
}

/* (set-pretty-printer-enabled! <gdb:pretty-printer> boolean)
     -> unspecified */

static SCM
gdbscm_set_pretty_printer_enabled_x (SCM self, SCM enabled)
{
  pretty_printer_smob *pp_smob
    = ppscm_get_pretty_printer_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  pp_smob->enabled = scm_from_bool (gdbscm_is_true (enabled));

  return SCM_UNSPECIFIED;
}

/* (pretty-printers) -> list
   Returns the list of global pretty-printers.  */

static SCM
gdbscm_pretty_printers (void)
{
  return pretty_printer_list;
}

/* (set-pretty-printers! list) -> unspecified
   Set the global pretty-printers list.  */

static SCM
gdbscm_set_pretty_printers_x (SCM printers)
{
  SCM_ASSERT_TYPE (gdbscm_is_true (scm_list_p (printers)), printers,
		   SCM_ARG1, FUNC_NAME, _("list"));

  pretty_printer_list = printers;

  return SCM_UNSPECIFIED;
}

/* Administrivia for pretty-printer-worker smobs.
   These are created when a matcher recognizes a value.  */

/* The smob "print" function for <gdb:pretty-printer-worker>.  */

static int
ppscm_print_pretty_printer_worker_smob (SCM self, SCM port,
					scm_print_state *pstate)
{
  pretty_printer_worker_smob *w_smob
    = (pretty_printer_worker_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", pretty_printer_worker_smob_name);
  scm_write (w_smob->display_hint, port);
  scm_puts (" ", port);
  scm_write (w_smob->to_string, port);
  scm_puts (" ", port);
  scm_write (w_smob->children, port);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* (make-pretty-printer-worker string procedure procedure)
     -> <gdb:pretty-printer-worker> */

static SCM
gdbscm_make_pretty_printer_worker (SCM display_hint, SCM to_string,
				   SCM children)
{
  pretty_printer_worker_smob *w_smob = (pretty_printer_worker_smob *)
    scm_gc_malloc (sizeof (pretty_printer_worker_smob),
		   pretty_printer_worker_smob_name);
  SCM w_scm;

  w_smob->display_hint = display_hint;
  w_smob->to_string = to_string;
  w_smob->children = children;
  w_scm = scm_new_smob (pretty_printer_worker_smob_tag, (scm_t_bits) w_smob);
  gdbscm_init_gsmob (&w_smob->base);
  return w_scm;
}

/* Return non-zero if SCM is a <gdb:pretty-printer-worker> object.  */

static int
ppscm_is_pretty_printer_worker (SCM scm)
{
  return SCM_SMOB_PREDICATE (pretty_printer_worker_smob_tag, scm);
}

/* (pretty-printer-worker? object) -> boolean */

static SCM
gdbscm_pretty_printer_worker_p (SCM scm)
{
  return scm_from_bool (ppscm_is_pretty_printer_worker (scm));
}

/* Helper function to create a <gdb:exception> object indicating that the
   type of some value returned from a pretty-printer is invalid.  */

static SCM
ppscm_make_pp_type_error_exception (const char *message, SCM object)
{
  std::string msg = string_printf ("%s: ~S", message);
  return gdbscm_make_error (pp_type_error_symbol,
			    NULL /* func */, msg.c_str (),
			    scm_list_1 (object), scm_list_1 (object));
}

/* Print MESSAGE as an exception (meaning it is controlled by
   "guile print-stack").
   Called from the printer code when the Scheme code returns an invalid type
   for something.  */

static void
ppscm_print_pp_type_error (const char *message, SCM object)
{
  SCM exception = ppscm_make_pp_type_error_exception (message, object);

  gdbscm_print_gdb_exception (SCM_BOOL_F, exception);
}

/* Helper function for find_pretty_printer which iterates over a list,
   calls each function and inspects output.  This will return a
   <gdb:pretty-printer> object if one recognizes VALUE.  If no printer is
   found, it will return #f.  On error, it will return a <gdb:exception>
   object.

   Note: This has to be efficient and careful.
   We don't want to excessively slow down printing of values, but any kind of
   random crud can appear in the pretty-printer list, and we can't crash
   because of it.  */

static SCM
ppscm_search_pp_list (SCM list, SCM value)
{
  SCM orig_list = list;

  if (scm_is_null (list))
    return SCM_BOOL_F;
  if (gdbscm_is_false (scm_list_p (list))) /* scm_is_pair? */
    {
      return ppscm_make_pp_type_error_exception
	(_("pretty-printer list is not a list"), list);
    }

  for ( ; scm_is_pair (list); list = scm_cdr (list))
    {
      SCM matcher = scm_car (list);
      SCM worker;
      pretty_printer_smob *pp_smob;

      if (!ppscm_is_pretty_printer (matcher))
	{
	  return ppscm_make_pp_type_error_exception
	    (_("pretty-printer list contains non-pretty-printer object"),
	     matcher);
	}

      pp_smob = (pretty_printer_smob *) SCM_SMOB_DATA (matcher);

      /* Skip if disabled.  */
      if (gdbscm_is_false (pp_smob->enabled))
	continue;

      if (!gdbscm_is_procedure (pp_smob->lookup))
	{
	  return ppscm_make_pp_type_error_exception
	    (_("invalid lookup object in pretty-printer matcher"),
	     pp_smob->lookup);
	}

      worker = gdbscm_safe_call_2 (pp_smob->lookup, matcher,
				   value, gdbscm_memory_error_p);
      if (!gdbscm_is_false (worker))
	{
	  if (gdbscm_is_exception (worker))
	    return worker;
	  if (ppscm_is_pretty_printer_worker (worker))
	    return worker;
	  return ppscm_make_pp_type_error_exception
	    (_("invalid result from pretty-printer lookup"), worker);
	}
    }

  if (!scm_is_null (list))
    {
      return ppscm_make_pp_type_error_exception
	(_("pretty-printer list is not a list"), orig_list);
    }

  return SCM_BOOL_F;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in all objfiles.
   If there's an error an exception smob is returned.
   The result is #f, if no pretty-printer was found.
   Otherwise the result is the pretty-printer smob.  */

static SCM
ppscm_find_pretty_printer_from_objfiles (SCM value)
{
  for (objfile *objfile : current_program_space->objfiles ())
    {
      objfile_smob *o_smob = ofscm_objfile_smob_from_objfile (objfile);
      SCM pp
	= ppscm_search_pp_list (ofscm_objfile_smob_pretty_printers (o_smob),
				value);

      /* Note: This will return if pp is a <gdb:exception> object,
	 which is what we want.  */
      if (gdbscm_is_true (pp))
	return pp;
    }

  return SCM_BOOL_F;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the current program space.
   If there's an error an exception smob is returned.
   The result is #f, if no pretty-printer was found.
   Otherwise the result is the pretty-printer smob.  */

static SCM
ppscm_find_pretty_printer_from_progspace (SCM value)
{
  pspace_smob *p_smob = psscm_pspace_smob_from_pspace (current_program_space);
  SCM pp
    = ppscm_search_pp_list (psscm_pspace_smob_pretty_printers (p_smob), value);

  return pp;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the gdb module.
   If there's an error a Scheme exception is returned.
   The result is #f, if no pretty-printer was found.
   Otherwise the result is the pretty-printer smob.  */

static SCM
ppscm_find_pretty_printer_from_gdb (SCM value)
{
  SCM pp = ppscm_search_pp_list (pretty_printer_list, value);

  return pp;
}

/* Find the pretty-printing constructor function for VALUE.  If no
   pretty-printer exists, return #f.  If one exists, return the
   gdb:pretty-printer smob that implements it.  On error, an exception smob
   is returned.

   Note: In the end it may be better to call out to Scheme once, and then
   do all of the lookup from Scheme.  TBD.  */

static SCM
ppscm_find_pretty_printer (SCM value)
{
  SCM pp;

  /* Look at the pretty-printer list for each objfile
     in the current program-space.  */
  pp = ppscm_find_pretty_printer_from_objfiles (value);
  /* Note: This will return if function is a <gdb:exception> object,
     which is what we want.  */
  if (gdbscm_is_true (pp))
    return pp;

  /* Look at the pretty-printer list for the current program-space.  */
  pp = ppscm_find_pretty_printer_from_progspace (value);
  /* Note: This will return if function is a <gdb:exception> object,
     which is what we want.  */
  if (gdbscm_is_true (pp))
    return pp;

  /* Look at the pretty-printer list in the gdb module.  */
  pp = ppscm_find_pretty_printer_from_gdb (value);
  return pp;
}

/* Pretty-print a single value, via the PRINTER, which must be a
   <gdb:pretty-printer-worker> object.
   The caller is responsible for ensuring PRINTER is valid.
   If the function returns a string, an SCM containing the string
   is returned.  If the function returns #f that means the pretty
   printer returned #f as a value.  Otherwise, if the function returns a
   <gdb:value> object, *OUT_VALUE is set to the value and #t is returned.
   It is an error if the printer returns #t.
   On error, an exception smob is returned.  */

static SCM
ppscm_pretty_print_one_value (SCM printer, struct value **out_value,
			      struct gdbarch *gdbarch,
			      const struct language_defn *language)
{
  SCM result = SCM_BOOL_F;

  *out_value = NULL;
  try
    {
      pretty_printer_worker_smob *w_smob
	= (pretty_printer_worker_smob *) SCM_SMOB_DATA (printer);

      result = gdbscm_safe_call_1 (w_smob->to_string, printer,
				   gdbscm_memory_error_p);
      if (gdbscm_is_false (result))
	; /* Done.  */
      else if (scm_is_string (result)
	       || lsscm_is_lazy_string (result))
	; /* Done.  */
      else if (vlscm_is_value (result))
	{
	  SCM except_scm;

	  *out_value
	    = vlscm_convert_value_from_scheme (FUNC_NAME, GDBSCM_ARG_NONE,
					       result, &except_scm,
					       gdbarch, language);
	  if (*out_value != NULL)
	    result = SCM_BOOL_T;
	  else
	    result = except_scm;
	}
      else if (gdbscm_is_exception (result))
	; /* Done.  */
      else
	{
	  /* Invalid result from to-string.  */
	  result = ppscm_make_pp_type_error_exception
	    (_("invalid result from pretty-printer to-string"), result);
	}
    }
  catch (const gdb_exception_forced_quit &except)
    {
      quit_force (NULL, 0);
    }
  catch (const gdb_exception &except)
    {
    }

  return result;
}

/* Return the display hint for PRINTER as a Scheme object.
   The caller is responsible for ensuring PRINTER is a
   <gdb:pretty-printer-worker> object.  */
 
static SCM
ppscm_get_display_hint_scm (SCM printer)
{
  pretty_printer_worker_smob *w_smob
    = (pretty_printer_worker_smob *) SCM_SMOB_DATA (printer);

  return w_smob->display_hint;
}

/* Return the display hint for the pretty-printer PRINTER.
   The caller is responsible for ensuring PRINTER is a
   <gdb:pretty-printer-worker> object.
   Returns the display hint or #f if the hint is not a string.  */

static enum display_hint
ppscm_get_display_hint_enum (SCM printer)
{
  SCM hint = ppscm_get_display_hint_scm (printer);

  if (gdbscm_is_false (hint))
    return HINT_NONE;
  if (scm_is_string (hint))
    {
      if (gdbscm_is_true (scm_string_equal_p (hint, ppscm_array_string)))
	return HINT_STRING;
      if (gdbscm_is_true (scm_string_equal_p (hint, ppscm_map_string)))
	return HINT_STRING;
      if (gdbscm_is_true (scm_string_equal_p (hint, ppscm_string_string)))
	return HINT_STRING;
      return HINT_ERROR;
    }
  return HINT_ERROR;
}

/* A wrapper for gdbscm_print_gdb_exception that ignores memory errors.
   EXCEPTION is a <gdb:exception> object.  */

static void
ppscm_print_exception_unless_memory_error (SCM exception,
					   struct ui_file *stream)
{
  if (gdbscm_memory_error_p (gdbscm_exception_key (exception)))
    {
      gdb::unique_xmalloc_ptr<char> msg
	= gdbscm_exception_message_to_string (exception);

      /* This "shouldn't happen", but play it safe.  */
      if (msg == NULL || msg.get ()[0] == '\0')
	gdb_printf (stream, _("<error reading variable>"));
      else
	{
	  /* Remove the trailing newline.  We could instead call a special
	     routine for printing memory error messages, but this is easy
	     enough for now.  */
	  char *msg_text = msg.get ();
	  size_t len = strlen (msg_text);

	  if (msg_text[len - 1] == '\n')
	    msg_text[len - 1] = '\0';
	  gdb_printf (stream, _("<error reading variable: %s>"), msg_text);
	}
    }
  else
    gdbscm_print_gdb_exception (SCM_BOOL_F, exception);
}

/* Helper for gdbscm_apply_val_pretty_printer which calls to_string and
   formats the result.  */

static enum guile_string_repr_result
ppscm_print_string_repr (SCM printer, enum display_hint hint,
			 struct ui_file *stream, int recurse,
			 const struct value_print_options *options,
			 struct gdbarch *gdbarch,
			 const struct language_defn *language)
{
  struct value *replacement = NULL;
  SCM str_scm;
  enum guile_string_repr_result result = STRING_REPR_ERROR;

  str_scm = ppscm_pretty_print_one_value (printer, &replacement,
					  gdbarch, language);
  if (gdbscm_is_false (str_scm))
    {
      result = STRING_REPR_NONE;
    }
  else if (scm_is_eq (str_scm, SCM_BOOL_T))
    {
      struct value_print_options opts = *options;

      gdb_assert (replacement != NULL);
      opts.addressprint = false;
      common_val_print (replacement, stream, recurse, &opts, language);
      result = STRING_REPR_OK;
    }
  else if (scm_is_string (str_scm))
    {
      size_t length;
      gdb::unique_xmalloc_ptr<char> string
	= gdbscm_scm_to_string (str_scm, &length,
				target_charset (gdbarch), 0 /*!strict*/, NULL);

      if (hint == HINT_STRING)
	{
	  struct type *type = builtin_type (gdbarch)->builtin_char;
	  
	  language->printstr (stream, type, (gdb_byte *) string.get (),
			      length, NULL, 0, options);
	}
      else
	{
	  /* Alas scm_to_stringn doesn't nul-terminate the string if we
	     ask for the length.  */
	  size_t i;

	  for (i = 0; i < length; ++i)
	    {
	      if (string.get ()[i] == '\0')
		gdb_puts ("\\000", stream);
	      else
		gdb_putc (string.get ()[i], stream);
	    }
	}
      result = STRING_REPR_OK;
    }
  else if (lsscm_is_lazy_string (str_scm))
    {
      struct value_print_options local_opts = *options;

      local_opts.addressprint = false;
      lsscm_val_print_lazy_string (str_scm, stream, &local_opts);
      result = STRING_REPR_OK;
    }
  else
    {
      gdb_assert (gdbscm_is_exception (str_scm));
      ppscm_print_exception_unless_memory_error (str_scm, stream);
      result = STRING_REPR_ERROR;
    }

  return result;
}

/* Helper for gdbscm_apply_val_pretty_printer that formats children of the
   printer, if any exist.
   The caller is responsible for ensuring PRINTER is a printer smob.
   If PRINTED_NOTHING is true, then nothing has been printed by to_string,
   and format output accordingly. */

static void
ppscm_print_children (SCM printer, enum display_hint hint,
		      struct ui_file *stream, int recurse,
		      const struct value_print_options *options,
		      struct gdbarch *gdbarch,
		      const struct language_defn *language,
		      int printed_nothing)
{
  pretty_printer_worker_smob *w_smob
    = (pretty_printer_worker_smob *) SCM_SMOB_DATA (printer);
  int is_map, is_array, done_flag, pretty;
  unsigned int i;
  SCM children;
  SCM iter = SCM_BOOL_F; /* -Wall */

  if (gdbscm_is_false (w_smob->children))
    return;
  if (!gdbscm_is_procedure (w_smob->children))
    {
      ppscm_print_pp_type_error
	(_("pretty-printer \"children\" object is not a procedure or #f"),
	 w_smob->children);
      return;
    }

  /* If we are printing a map or an array, we want special formatting.  */
  is_map = hint == HINT_MAP;
  is_array = hint == HINT_ARRAY;

  children = gdbscm_safe_call_1 (w_smob->children, printer,
				 gdbscm_memory_error_p);
  if (gdbscm_is_exception (children))
    {
      ppscm_print_exception_unless_memory_error (children, stream);
      goto done;
    }
  /* We combine two steps here: get children, make an iterator out of them.
     This simplifies things because there's no language means of creating
     iterators, and it's the printer object that knows how it will want its
     children iterated over.  */
  if (!itscm_is_iterator (children))
    {
      ppscm_print_pp_type_error
	(_("result of pretty-printer \"children\" procedure is not"
	   " a <gdb:iterator> object"), children);
      goto done;
    }
  iter = children;

  /* Use the prettyformat_arrays option if we are printing an array,
     and the pretty option otherwise.  */
  if (is_array)
    pretty = options->prettyformat_arrays;
  else
    {
      if (options->prettyformat == Val_prettyformat)
	pretty = 1;
      else
	pretty = options->prettyformat_structs;
    }

  done_flag = 0;
  for (i = 0; i < options->print_max; ++i)
    {
      SCM scm_name, v_scm;
      SCM item = itscm_safe_call_next_x (iter, gdbscm_memory_error_p);

      if (gdbscm_is_exception (item))
	{
	  ppscm_print_exception_unless_memory_error (item, stream);
	  break;
	}
      if (itscm_is_end_of_iteration (item))
	{
	  /* Set a flag so we can know whether we printed all the
	     available elements.  */
	  done_flag = 1;
	  break;
	}

      if (! scm_is_pair (item))
	{
	  ppscm_print_pp_type_error
	    (_("result of pretty-printer children iterator is not a pair"
	       " or (end-of-iteration)"),
	     item);
	  continue;
	}
      scm_name = scm_car (item);
      v_scm = scm_cdr (item);
      if (!scm_is_string (scm_name))
	{
	  ppscm_print_pp_type_error
	    (_("first element of pretty-printer children iterator is not"
	       " a string"), item);
	  continue;
	}
      gdb::unique_xmalloc_ptr<char> name
	= gdbscm_scm_to_c_string (scm_name);

      /* Print initial "=" to separate print_string_repr output and
	 children.  For other elements, there are three cases:
	 1. Maps.  Print a "," after each value element.
	 2. Arrays.  Always print a ",".
	 3. Other.  Always print a ",".  */
      if (i == 0)
	{
	  if (!printed_nothing)
	    gdb_puts (" = ", stream);
	}
      else if (! is_map || i % 2 == 0)
	gdb_puts (pretty ? "," : ", ", stream);

      /* Skip printing children if max_depth has been reached.  This check
	 is performed after print_string_repr and the "=" separator so that
	 these steps are not skipped if the variable is located within the
	 permitted depth.  */
      if (val_print_check_max_depth (stream, recurse, options, language))
	goto done;
      else if (i == 0)
	/* Print initial "{" to bookend children.  */
	gdb_puts ("{", stream);

      /* In summary mode, we just want to print "= {...}" if there is
	 a value.  */
      if (options->summary)
	{
	  /* This increment tricks the post-loop logic to print what
	     we want.  */
	  ++i;
	  /* Likewise.  */
	  pretty = 0;
	  break;
	}

      if (! is_map || i % 2 == 0)
	{
	  if (pretty)
	    {
	      gdb_puts ("\n", stream);
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  else
	    stream->wrap_here (2 + 2 *recurse);
	}

      if (is_map && i % 2 == 0)
	gdb_puts ("[", stream);
      else if (is_array)
	{
	  /* We print the index, not whatever the child method
	     returned as the name.  */
	  if (options->print_array_indexes)
	    gdb_printf (stream, "[%d] = ", i);
	}
      else if (! is_map)
	{
	  gdb_puts (name.get (), stream);
	  gdb_puts (" = ", stream);
	}

      if (lsscm_is_lazy_string (v_scm))
	{
	  struct value_print_options local_opts = *options;

	  local_opts.addressprint = false;
	  lsscm_val_print_lazy_string (v_scm, stream, &local_opts);
	}
      else if (scm_is_string (v_scm))
	{
	  gdb::unique_xmalloc_ptr<char> output
	    = gdbscm_scm_to_c_string (v_scm);
	  gdb_puts (output.get (), stream);
	}
      else
	{
	  SCM except_scm;
	  struct value *value
	    = vlscm_convert_value_from_scheme (FUNC_NAME, GDBSCM_ARG_NONE,
					       v_scm, &except_scm,
					       gdbarch, language);

	  if (value == NULL)
	    {
	      ppscm_print_exception_unless_memory_error (except_scm, stream);
	      break;
	    }
	  else
	    {
	      /* When printing the key of a map we allow one additional
		 level of depth.  This means the key will print before the
		 value does.  */
	      struct value_print_options opt = *options;
	      if (is_map && i % 2 == 0
		  && opt.max_depth != -1
		  && opt.max_depth < INT_MAX)
		++opt.max_depth;
	      common_val_print (value, stream, recurse + 1, &opt, language);
	    }
	}

      if (is_map && i % 2 == 0)
	gdb_puts ("] = ", stream);
    }

  if (i)
    {
      if (!done_flag)
	{
	  if (pretty)
	    {
	      gdb_puts ("\n", stream);
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  gdb_puts ("...", stream);
	}
      if (pretty)
	{
	  gdb_puts ("\n", stream);
	  print_spaces (2 * recurse, stream);
	}
      gdb_puts ("}", stream);
    }

 done:
  /* Play it safe, make sure ITER doesn't get GC'd.  */
  scm_remember_upto_here_1 (iter);
}

/* This is the extension_language_ops.apply_val_pretty_printer "method".  */

enum ext_lang_rc
gdbscm_apply_val_pretty_printer (const struct extension_language_defn *extlang,
				 struct value *value,
				 struct ui_file *stream, int recurse,
				 const struct value_print_options *options,
				 const struct language_defn *language)
{
  struct type *type = value->type ();
  struct gdbarch *gdbarch = type->arch ();
  SCM exception = SCM_BOOL_F;
  SCM printer = SCM_BOOL_F;
  SCM val_obj = SCM_BOOL_F;
  enum display_hint hint;
  enum ext_lang_rc result = EXT_LANG_RC_NOP;
  enum guile_string_repr_result print_result;

  if (value->lazy ())
    value->fetch_lazy ();

  /* No pretty-printer support for unavailable values.  */
  if (!value->bytes_available (0, type->length ()))
    return EXT_LANG_RC_NOP;

  if (!gdb_scheme_initialized)
    return EXT_LANG_RC_NOP;

  /* Instantiate the printer.  */
  val_obj = vlscm_scm_from_value_no_release (value);
  if (gdbscm_is_exception (val_obj))
    {
      exception = val_obj;
      result = EXT_LANG_RC_ERROR;
      goto done;
    }

  printer = ppscm_find_pretty_printer (val_obj);

  if (gdbscm_is_exception (printer))
    {
      exception = printer;
      result = EXT_LANG_RC_ERROR;
      goto done;
    }
  if (gdbscm_is_false (printer))
    {
      result = EXT_LANG_RC_NOP;
      goto done;
    }
  gdb_assert (ppscm_is_pretty_printer_worker (printer));

  /* If we are printing a map, we want some special formatting.  */
  hint = ppscm_get_display_hint_enum (printer);
  if (hint == HINT_ERROR)
    {
      /* Print the error as an exception for consistency.  */
      SCM hint_scm = ppscm_get_display_hint_scm (printer);

      ppscm_print_pp_type_error ("Invalid display hint", hint_scm);
      /* Fall through.  A bad hint doesn't stop pretty-printing.  */
      hint = HINT_NONE;
    }

  /* Print the section.  */
  print_result = ppscm_print_string_repr (printer, hint, stream, recurse,
					  options, gdbarch, language);
  if (print_result != STRING_REPR_ERROR)
    {
      ppscm_print_children (printer, hint, stream, recurse, options,
			    gdbarch, language,
			    print_result == STRING_REPR_NONE);
    }

  result = EXT_LANG_RC_OK;

 done:
  if (gdbscm_is_exception (exception))
    ppscm_print_exception_unless_memory_error (exception, stream);
  return result;
}

/* Initialize the Scheme pretty-printer code.  */

static const scheme_function pretty_printer_functions[] =
{
  { "make-pretty-printer", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_make_pretty_printer),
    "\
Create a <gdb:pretty-printer> object.\n\
\n\
  Arguments: name lookup\n\
    name:   a string naming the matcher\n\
    lookup: a procedure:\n\
      (pretty-printer <gdb:value>) -> <gdb:pretty-printer-worker> | #f." },

  { "pretty-printer?", 1, 0, 0, as_a_scm_t_subr (gdbscm_pretty_printer_p),
    "\
Return #t if the object is a <gdb:pretty-printer> object." },

  { "pretty-printer-enabled?", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_pretty_printer_enabled_p),
    "\
Return #t if the pretty-printer is enabled." },

  { "set-pretty-printer-enabled!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_pretty_printer_enabled_x),
    "\
Set the enabled flag of the pretty-printer.\n\
Returns \"unspecified\"." },

  { "make-pretty-printer-worker", 3, 0, 0,
    as_a_scm_t_subr (gdbscm_make_pretty_printer_worker),
    "\
Create a <gdb:pretty-printer-worker> object.\n\
\n\
  Arguments: display-hint to-string children\n\
    display-hint: either #f or one of \"array\", \"map\", or \"string\"\n\
    to-string:    a procedure:\n\
      (pretty-printer) -> string | #f | <gdb:value>\n\
    children:     either #f or a procedure:\n\
      (pretty-printer) -> <gdb:iterator>" },

  { "pretty-printer-worker?", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_pretty_printer_worker_p),
    "\
Return #t if the object is a <gdb:pretty-printer-worker> object." },

  { "pretty-printers", 0, 0, 0, as_a_scm_t_subr (gdbscm_pretty_printers),
    "\
Return the list of global pretty-printers." },

  { "set-pretty-printers!", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_set_pretty_printers_x),
    "\
Set the list of global pretty-printers." },

  END_FUNCTIONS
};

void
gdbscm_initialize_pretty_printers (void)
{
  pretty_printer_smob_tag
    = gdbscm_make_smob_type (pretty_printer_smob_name,
			     sizeof (pretty_printer_smob));
  scm_set_smob_print (pretty_printer_smob_tag,
		      ppscm_print_pretty_printer_smob);

  pretty_printer_worker_smob_tag
    = gdbscm_make_smob_type (pretty_printer_worker_smob_name,
			     sizeof (pretty_printer_worker_smob));
  scm_set_smob_print (pretty_printer_worker_smob_tag,
		      ppscm_print_pretty_printer_worker_smob);

  gdbscm_define_functions (pretty_printer_functions, 1);

  pretty_printer_list = SCM_EOL;

  pp_type_error_symbol = scm_from_latin1_symbol ("gdb:pp-type-error");

  ppscm_map_string = scm_from_latin1_string ("map");
  ppscm_array_string = scm_from_latin1_string ("array");
  ppscm_string_string = scm_from_latin1_string ("string");
}
