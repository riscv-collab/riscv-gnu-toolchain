/* GDB/Scheme smobs (gsmob is pronounced "jee smob")

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

/* Smobs are Guile's "small object".
   They are used to export C structs to Scheme.

   Note: There's only room in the encoding space for 256, and while we won't
   come close to that, mixed with other libraries maybe someday we could.
   We don't worry about it now, except to be aware of the issue.
   We could allocate just a few smobs and use the unused smob flags field to
   specify the gdb smob kind, that is left for another day if it ever is
   needed.

   Some GDB smobs are "chained gsmobs".  They are used to assist with life-time
   tracking of GDB objects vs Scheme objects.  Gsmobs can "subclass"
   chained_gdb_smob, which contains a doubly-linked list to assist with
   life-time tracking.

   Some other GDB smobs are "eqable gsmobs".  Gsmob implementations can
   "subclass" eqable_gdb_smob to make gsmobs eq?-able.  This is done by
   recording all gsmobs in a hash table and before creating a gsmob first
   seeing if it's already in the table.  Eqable gsmobs can also be used where
   lifetime-tracking is required.  */

#include "defs.h"
#include "hashtab.h"
#include "objfiles.h"
#include "guile-internal.h"

/* We need to call this.  Undo our hack to prevent others from calling it.  */
#undef scm_make_smob_type

static htab_t registered_gsmobs;

/* Hash function for registered_gsmobs hash table.  */

static hashval_t
hash_scm_t_bits (const void *item)
{
  uintptr_t v = (uintptr_t) item;

  return v;
}

/* Equality function for registered_gsmobs hash table.  */

static int
eq_scm_t_bits (const void *item_lhs, const void *item_rhs)
{
  return item_lhs == item_rhs;
}

/* Record GSMOB_CODE as being a gdb smob.
   GSMOB_CODE is the result of scm_make_smob_type.  */

static void
register_gsmob (scm_t_bits gsmob_code)
{
  void **slot;

  slot = htab_find_slot (registered_gsmobs, (void *) gsmob_code, INSERT);
  gdb_assert (*slot == NULL);
  *slot = (void *) gsmob_code;
}

/* Return non-zero if SCM is any registered gdb smob object.  */

static int
gdbscm_is_gsmob (SCM scm)
{
  void **slot;

  if (SCM_IMP (scm))
    return 0;
  slot = htab_find_slot (registered_gsmobs, (void *) SCM_TYP16 (scm),
			 NO_INSERT);
  return slot != NULL;
}

/* Call this to register a smob, instead of scm_make_smob_type.
   Exports the created smob type from the current module.  */

scm_t_bits
gdbscm_make_smob_type (const char *name, size_t size)
{
  scm_t_bits result = scm_make_smob_type (name, size);

  register_gsmob (result);

#if SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION == 0
  /* Prior to Guile 2.1.0, smob classes were only exposed via exports
     from the (oop goops) module.  */
  SCM bound_name = scm_string_append (scm_list_3 (scm_from_latin1_string ("<"),
						  scm_from_latin1_string (name),
						  scm_from_latin1_string (">")));
  bound_name = scm_string_to_symbol (bound_name);
  SCM smob_type = scm_public_ref (scm_list_2 (scm_from_latin1_symbol ("oop"),
					      scm_from_latin1_symbol ("goops")),
				  bound_name);
#elif SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION == 1 && SCM_MICRO_VERSION == 0
  /* Guile 2.1.0 doesn't provide any API for looking up smob classes.
     We could try allocating a fake instance and using scm_class_of,
     but it's probably not worth the trouble for the sake of a single
     development release.  */
#  error "Unsupported Guile version"
#else
  /* Guile 2.1.1 and above provides scm_smob_type_class.  */
  SCM smob_type = scm_smob_type_class (result);
#endif

  SCM smob_type_name = scm_class_name (smob_type);
  scm_define (smob_type_name, smob_type);
  scm_module_export (scm_current_module (), scm_list_1 (smob_type_name));

  return result;
}

/* Initialize a gsmob.  */

void
gdbscm_init_gsmob (gdb_smob *base)
{
  base->empty_base_class = 0;
}

/* Initialize a chained_gdb_smob.
   This is the same as gdbscm_init_gsmob except that it also sets prev,next
   to NULL.  */

void
gdbscm_init_chained_gsmob (chained_gdb_smob *base)
{
  gdbscm_init_gsmob ((gdb_smob *) base);
  base->prev = NULL;
  base->next = NULL;
}

/* Initialize an eqable_gdb_smob.
   This is the same as gdbscm_init_gsmob except that it also sets
   BASE->containing_scm to CONTAINING_SCM.  */

void
gdbscm_init_eqable_gsmob (eqable_gdb_smob *base, SCM containing_scm)
{
  gdbscm_init_gsmob ((gdb_smob *) base);
  base->containing_scm = containing_scm;
}


/* gsmob accessors */

/* Return the gsmob in SELF.
   Throws an exception if SELF is not a gsmob.  */

static SCM
gsscm_get_gsmob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (gdbscm_is_gsmob (self), self, arg_pos, func_name,
		   _("any gdb smob"));

  return self;
}

/* (gdb-object-kind gsmob) -> symbol

   Note: While one might want to name this gdb-object-class-name, it is named
   "-kind" because smobs aren't real GOOPS classes.  */

static SCM
gdbscm_gsmob_kind (SCM self)
{
  SCM smob, result;
  scm_t_bits smobnum;
  const char *name;

  smob = gsscm_get_gsmob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  smobnum = SCM_SMOBNUM (smob);
  name = SCM_SMOBNAME (smobnum);
  gdb::unique_xmalloc_ptr<char> kind = xstrprintf ("<%s>", name);
  result = scm_from_latin1_symbol (kind.get ());
  return result;
}


/* When underlying gdb data structures are deleted, we need to update any
   smobs with references to them.  There are several smobs that reference
   objfile-based data, so we provide helpers to manage this.  */

/* Create a hash table for mapping a pointer to a gdb data structure to the
   gsmob that wraps it.  */

htab_t
gdbscm_create_eqable_gsmob_ptr_map (htab_hash hash_fn, htab_eq eq_fn)
{
  htab_t htab = htab_create_alloc (7, hash_fn, eq_fn,
				   NULL, xcalloc, xfree);

  return htab;
}

/* Return a pointer to the htab entry for the eq?-able gsmob BASE.
   If the entry is found, *SLOT is non-NULL.
   Otherwise *slot is NULL.  */

eqable_gdb_smob **
gdbscm_find_eqable_gsmob_ptr_slot (htab_t htab, eqable_gdb_smob *base)
{
  void **slot = htab_find_slot (htab, base, INSERT);

  return (eqable_gdb_smob **) slot;
}

/* Record BASE in SLOT.  SLOT must be the result of calling
   gdbscm_find_eqable_gsmob_ptr_slot on BASE (or equivalent for lookup).  */

void
gdbscm_fill_eqable_gsmob_ptr_slot (eqable_gdb_smob **slot,
				   eqable_gdb_smob *base)
{
  *slot = base;
}

/* Remove BASE from HTAB.
   BASE is a pointer to a gsmob that wraps a pointer to a GDB datum.
   This is used, for example, when an object is freed.

   It is an error to call this if PTR is not in HTAB (only because it allows
   for some consistency checking).  */

void
gdbscm_clear_eqable_gsmob_ptr_slot (htab_t htab, eqable_gdb_smob *base)
{
  void **slot = htab_find_slot (htab, base, NO_INSERT);

  gdb_assert (slot != NULL);
  htab_clear_slot (htab, slot);
}

/* Initialize the Scheme gsmobs code.  */

static const scheme_function gsmob_functions[] =
{
  /* N.B. There is a general rule of not naming symbols in gdb-guile with a
     "gdb" prefix.  This symbol does not violate this rule because it is to
     be read as "gdb-object-foo", not "gdb-foo".  */
  { "gdb-object-kind", 1, 0, 0, as_a_scm_t_subr (gdbscm_gsmob_kind),
    "\
Return the kind of the GDB object, e.g., <gdb:breakpoint>, as a symbol." },

  END_FUNCTIONS
};

void
gdbscm_initialize_smobs (void)
{
  registered_gsmobs = htab_create_alloc (10,
					 hash_scm_t_bits, eq_scm_t_bits,
					 NULL, xcalloc, xfree);

  gdbscm_define_functions (gsmob_functions, 1);
}
