/* Simple iterators for GDB/Scheme.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* These are *simple* iterators, used to implement iterating over a collection
   of objects.  They are implemented as a smob containing three objects:

   1) the object being iterated over,
   2) an object to record the progress of the iteration,
   3) a procedure of one argument (the iterator object) that returns the next
      object in the iteration or a pre-determined end marker.

   Simple example:

   (define-public (make-list-iterator l end-marker)
     "Return a <gdb:iterator> object for a list."
     (let ((next! (lambda (iter)
		    (let ((l (iterator-progress iter)))
		      (if (eq? l '())
			  end-marker
			  (begin
			    (set-iterator-progress! iter (cdr l))
			    (car l)))))))
       (make-iterator l l next!)))

   (define l '(1 2))
   (define i (make-list-iterator l #:eoi))
   (iterator-next! i) -> 1
   (iterator-next! i) -> 2
   (iterator-next! i) -> #:eoi

   There is SRFI 41, Streams.  We might support that too eventually (not with
   this interface of course).  */

#include "defs.h"
#include "guile-internal.h"

/* A smob for iterating over something.
   Typically this is used when computing a list of everything is
   too expensive.  */

struct iterator_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The object being iterated over.  */
  SCM object;

  /* An arbitrary object describing the progress of the iteration.
     This is used by next_x to track progress.  */
  SCM progress;

  /* A procedure of one argument, the iterator.
     It returns the next object in the iteration.
     How to signal "end of iteration" is up to next_x.  */
  SCM next_x;
};

static const char iterator_smob_name[] = "gdb:iterator";

/* The tag Guile knows the iterator smob by.  */
static scm_t_bits iterator_smob_tag;

/* A unique-enough marker to denote "end of iteration".  */
static SCM end_of_iteration;

const char *
itscm_iterator_smob_name (void)
{
  return iterator_smob_name;
}

SCM
itscm_iterator_smob_object (iterator_smob *i_smob)
{
  return i_smob->object;
}

SCM
itscm_iterator_smob_progress (iterator_smob *i_smob)
{
  return i_smob->progress;
}

void
itscm_set_iterator_smob_progress_x (iterator_smob *i_smob, SCM progress)
{
  i_smob->progress = progress;
}

/* Administrivia for iterator smobs.  */

/* The smob "print" function for <gdb:iterator>.  */

static int
itscm_print_iterator_smob (SCM self, SCM port, scm_print_state *pstate)
{
  iterator_smob *i_smob = (iterator_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", iterator_smob_name);
  scm_write (i_smob->object, port);
  scm_puts (" ", port);
  scm_write (i_smob->progress, port);
  scm_puts (" ", port);
  scm_write (i_smob->next_x, port);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to make a <gdb:iterator> object.
   Caller must verify correctness of arguments.
   No exceptions are thrown.  */

static SCM
itscm_make_iterator_smob (SCM object, SCM progress, SCM next)
{
  iterator_smob *i_smob = (iterator_smob *)
    scm_gc_malloc (sizeof (iterator_smob), iterator_smob_name);
  SCM i_scm;

  i_smob->object = object;
  i_smob->progress = progress;
  i_smob->next_x = next;
  i_scm = scm_new_smob (iterator_smob_tag, (scm_t_bits) i_smob);
  gdbscm_init_gsmob (&i_smob->base);

  return i_scm;
}

/* (make-iterator object object procedure) -> <gdb:iterator> */

SCM
gdbscm_make_iterator (SCM object, SCM progress, SCM next)
{
  SCM i_scm;

  SCM_ASSERT_TYPE (gdbscm_is_procedure (next), next, SCM_ARG3, FUNC_NAME,
		   _("procedure"));

  i_scm = itscm_make_iterator_smob (object, progress, next);

  return i_scm;
}

/* Return non-zero if SCM is a <gdb:iterator> object.  */

int
itscm_is_iterator (SCM scm)
{
  return SCM_SMOB_PREDICATE (iterator_smob_tag, scm);
}

/* (iterator? object) -> boolean */

static SCM
gdbscm_iterator_p (SCM scm)
{
  return scm_from_bool (itscm_is_iterator (scm));
}

/* (end-of-iteration) -> an "end-of-iteration" marker
   We rely on this not being used as a data result of an iterator.  */

SCM
gdbscm_end_of_iteration (void)
{
  return end_of_iteration;
}

/* Return non-zero if OBJ is the end-of-iteration marker.  */

int
itscm_is_end_of_iteration (SCM obj)
{
  return scm_is_eq (obj, end_of_iteration);
}

/* (end-of-iteration? obj) -> boolean */

static SCM
gdbscm_end_of_iteration_p (SCM obj)
{
  return scm_from_bool (itscm_is_end_of_iteration (obj));
}

/* Call the next! method on ITER, which must be a <gdb:iterator> object.
   Returns a <gdb:exception> object if an exception is thrown.
   OK_EXCPS is passed to gdbscm_safe_call_1.  */

SCM
itscm_safe_call_next_x (SCM iter, excp_matcher_func *ok_excps)
{
  iterator_smob *i_smob;

  gdb_assert (itscm_is_iterator (iter));

  i_smob = (iterator_smob *) SCM_SMOB_DATA (iter);
  return gdbscm_safe_call_1 (i_smob->next_x, iter, ok_excps);
}

/* Iterator methods.  */

/* Returns the <gdb:iterator> smob in SELF.
   Throws an exception if SELF is not an iterator smob.  */

SCM
itscm_get_iterator_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (itscm_is_iterator (self), self, arg_pos, func_name,
		   iterator_smob_name);

  return self;
}

/* (iterator-object <gdb:iterator>) -> object */

static SCM
gdbscm_iterator_object (SCM self)
{
  SCM i_scm;
  iterator_smob *i_smob;

  i_scm = itscm_get_iterator_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  i_smob = (iterator_smob *) SCM_SMOB_DATA (i_scm);

  return i_smob->object;
}

/* (iterator-progress <gdb:iterator>) -> object */

static SCM
gdbscm_iterator_progress (SCM self)
{
  SCM i_scm;
  iterator_smob *i_smob;

  i_scm = itscm_get_iterator_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  i_smob = (iterator_smob *) SCM_SMOB_DATA (i_scm);

  return i_smob->progress;
}

/* (set-iterator-progress! <gdb:iterator> object) -> unspecified */

static SCM
gdbscm_set_iterator_progress_x (SCM self, SCM value)
{
  SCM i_scm;
  iterator_smob *i_smob;

  i_scm = itscm_get_iterator_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  i_smob = (iterator_smob *) SCM_SMOB_DATA (i_scm);

  i_smob->progress = value;
  return SCM_UNSPECIFIED;
}

/* (iterator-next! <gdb:iterator>) -> object
   The result is the next value in the iteration or some "end" marker.
   It is up to each iterator's next! function to specify what its end
   marker is.  */

static SCM
gdbscm_iterator_next_x (SCM self)
{
  SCM i_scm;
  iterator_smob *i_smob;

  i_scm = itscm_get_iterator_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  i_smob = (iterator_smob *) SCM_SMOB_DATA (i_scm);
  /* We leave type-checking of the procedure to gdbscm_safe_call_1.  */

  return gdbscm_safe_call_1 (i_smob->next_x, self, NULL);
}

/* Initialize the Scheme iterator code.  */

static const scheme_function iterator_functions[] =
{
  { "make-iterator", 3, 0, 0, as_a_scm_t_subr (gdbscm_make_iterator),
    "\
Create a <gdb:iterator> object.\n\
\n\
  Arguments: object progress next!\n\
    object:   The object to iterate over.\n\
    progress: An object to use to track progress of the iteration.\n\
    next!:    A procedure of one argument, the iterator.\n\
      Returns the next element in the iteration or an implementation-chosen\n\
      value to signify iteration is complete.\n\
      By convention end-of-iteration should be marked with (end-of-iteration)\n\
      from module (gdb iterator)." },

  { "iterator?", 1, 0, 0, as_a_scm_t_subr (gdbscm_iterator_p),
    "\
Return #t if the object is a <gdb:iterator> object." },

  { "iterator-object", 1, 0, 0, as_a_scm_t_subr (gdbscm_iterator_object),
    "\
Return the object being iterated over." },

  { "iterator-progress", 1, 0, 0, as_a_scm_t_subr (gdbscm_iterator_progress),
    "\
Return the progress object of the iterator." },

  { "set-iterator-progress!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_iterator_progress_x),
    "\
Set the progress object of the iterator." },

  { "iterator-next!", 1, 0, 0, as_a_scm_t_subr (gdbscm_iterator_next_x),
    "\
Invoke the next! procedure of the iterator and return its result." },

  { "end-of-iteration", 0, 0, 0, as_a_scm_t_subr (gdbscm_end_of_iteration),
    "\
Return the end-of-iteration marker." },

  { "end-of-iteration?", 1, 0, 0, as_a_scm_t_subr (gdbscm_end_of_iteration_p),
    "\
Return #t if the object is the end-of-iteration marker." },

  END_FUNCTIONS
};

void
gdbscm_initialize_iterators (void)
{
  iterator_smob_tag = gdbscm_make_smob_type (iterator_smob_name,
					     sizeof (iterator_smob));
  scm_set_smob_print (iterator_smob_tag, itscm_print_iterator_smob);

  gdbscm_define_functions (iterator_functions, 1);

  /* We can make this more unique if it's necessary,
     but this is good enough for now.  */
  end_of_iteration = scm_from_latin1_keyword ("end-of-iteration");
}
