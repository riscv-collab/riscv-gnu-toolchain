/* Scheme interface to architecture.

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

#include "defs.h"
#include "charset.h"
#include "gdbarch.h"
#include "arch-utils.h"
#include "guile-internal.h"

/* The <gdb:arch> smob.  */

struct arch_smob
{
  /* This always appears first.  */
  gdb_smob base;

  struct gdbarch *gdbarch;
};

static const char arch_smob_name[] = "gdb:arch";

/* The tag Guile knows the arch smob by.  */
static scm_t_bits arch_smob_tag;

/* Use a 'void *' here because it isn't guaranteed that SCM is a
   pointer.  */
static const registry<gdbarch>::key<void, gdb::noop_deleter<void>>
     arch_object_data;

static int arscm_is_arch (SCM);

/* Administrivia for arch smobs.  */

/* The smob "print" function for <gdb:arch>.  */

static int
arscm_print_arch_smob (SCM self, SCM port, scm_print_state *pstate)
{
  arch_smob *a_smob = (arch_smob *) SCM_SMOB_DATA (self);
  struct gdbarch *gdbarch = a_smob->gdbarch;

  gdbscm_printf (port, "#<%s", arch_smob_name);
  gdbscm_printf (port, " %s", gdbarch_bfd_arch_info (gdbarch)->printable_name);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:arch> object for GDBARCH.  */

static SCM
arscm_make_arch_smob (struct gdbarch *gdbarch)
{
  arch_smob *a_smob = (arch_smob *)
    scm_gc_malloc (sizeof (arch_smob), arch_smob_name);
  SCM a_scm;

  a_smob->gdbarch = gdbarch;
  a_scm = scm_new_smob (arch_smob_tag, (scm_t_bits) a_smob);
  gdbscm_init_gsmob (&a_smob->base);

  return a_scm;
}

/* Return the gdbarch field of A_SMOB.  */

struct gdbarch *
arscm_get_gdbarch (arch_smob *a_smob)
{
  return a_smob->gdbarch;
}

/* Return non-zero if SCM is an architecture smob.  */

static int
arscm_is_arch (SCM scm)
{
  return SCM_SMOB_PREDICATE (arch_smob_tag, scm);
}

/* (arch? object) -> boolean */

static SCM
gdbscm_arch_p (SCM scm)
{
  return scm_from_bool (arscm_is_arch (scm));
}

/* Return the <gdb:arch> object corresponding to GDBARCH.
   The object is cached in GDBARCH so this is simple.  */

SCM
arscm_scm_from_arch (struct gdbarch *gdbarch)
{
  SCM arch_scm;
  void *data = arch_object_data.get (gdbarch);
  if (data == nullptr)
    {
      arch_scm = arscm_make_arch_smob (gdbarch);

      /* This object lasts the duration of the GDB session, so there
	 is no call to scm_gc_unprotect_object for it.  */
      scm_gc_protect_object (arch_scm);

      arch_object_data.set (gdbarch, (void *) arch_scm);
    }
  else
    arch_scm = (SCM) data;

  return arch_scm;
}

/* Return the <gdb:arch> smob in SELF.
   Throws an exception if SELF is not a <gdb:arch> object.  */

static SCM
arscm_get_arch_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (arscm_is_arch (self), self, arg_pos, func_name,
		   arch_smob_name);

  return self;
}

/* Return a pointer to the arch smob of SELF.
   Throws an exception if SELF is not a <gdb:arch> object.  */

arch_smob *
arscm_get_arch_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM a_scm = arscm_get_arch_arg_unsafe (self, arg_pos, func_name);
  arch_smob *a_smob = (arch_smob *) SCM_SMOB_DATA (a_scm);

  return a_smob;
}

/* Arch methods.  */

/* (current-arch) -> <gdb:arch>
   Return the architecture of the currently selected stack frame,
   if there is one, or the current target if there isn't.  */

static SCM
gdbscm_current_arch (void)
{
  return arscm_scm_from_arch (get_current_arch ());
}

/* (arch-name <gdb:arch>) -> string
   Return the name of the architecture as a string value.  */

static SCM
gdbscm_arch_name (SCM self)
{
  arch_smob *a_smob
    = arscm_get_arch_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct gdbarch *gdbarch = a_smob->gdbarch;
  const char *name;

  name = (gdbarch_bfd_arch_info (gdbarch))->printable_name;

  return gdbscm_scm_from_c_string (name);
}

/* (arch-charset <gdb:arch>) -> string */

static SCM
gdbscm_arch_charset (SCM self)
{
  arch_smob *a_smob
    =arscm_get_arch_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct gdbarch *gdbarch = a_smob->gdbarch;

  return gdbscm_scm_from_c_string (target_charset (gdbarch));
}

/* (arch-wide-charset <gdb:arch>) -> string */

static SCM
gdbscm_arch_wide_charset (SCM self)
{
  arch_smob *a_smob
    = arscm_get_arch_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct gdbarch *gdbarch = a_smob->gdbarch;

  return gdbscm_scm_from_c_string (target_wide_charset (gdbarch));
}

/* Builtin types.

   The order the types are defined here follows the order in
   struct builtin_type.  */

/* Helper routine to return a builtin type for <gdb:arch> object SELF.
   OFFSET is offsetof (builtin_type, the_type).
   Throws an exception if SELF is not a <gdb:arch> object.  */

static const struct builtin_type *
gdbscm_arch_builtin_type (SCM self, const char *func_name)
{
  arch_smob *a_smob
    = arscm_get_arch_smob_arg_unsafe (self, SCM_ARG1, func_name);
  struct gdbarch *gdbarch = a_smob->gdbarch;

  return builtin_type (gdbarch);
}

/* (arch-void-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_void_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_void;

  return tyscm_scm_from_type (type);
}

/* (arch-char-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_char_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_char;

  return tyscm_scm_from_type (type);
}

/* (arch-short-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_short_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_short;

  return tyscm_scm_from_type (type);
}

/* (arch-int-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_int_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_int;

  return tyscm_scm_from_type (type);
}

/* (arch-long-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_long_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_long;

  return tyscm_scm_from_type (type);
}

/* (arch-schar-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_schar_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_signed_char;

  return tyscm_scm_from_type (type);
}

/* (arch-uchar-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_uchar_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_unsigned_char;

  return tyscm_scm_from_type (type);
}

/* (arch-ushort-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_ushort_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_unsigned_short;

  return tyscm_scm_from_type (type);
}

/* (arch-uint-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_uint_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_unsigned_int;

  return tyscm_scm_from_type (type);
}

/* (arch-ulong-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_ulong_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_unsigned_long;

  return tyscm_scm_from_type (type);
}

/* (arch-float-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_float_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_float;

  return tyscm_scm_from_type (type);
}

/* (arch-double-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_double_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_double;

  return tyscm_scm_from_type (type);
}

/* (arch-longdouble-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_longdouble_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_long_double;

  return tyscm_scm_from_type (type);
}

/* (arch-bool-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_bool_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_bool;

  return tyscm_scm_from_type (type);
}

/* (arch-longlong-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_longlong_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_long_long;

  return tyscm_scm_from_type (type);
}

/* (arch-ulonglong-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_ulonglong_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_unsigned_long_long;

  return tyscm_scm_from_type (type);
}

/* (arch-int8-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_int8_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_int8;

  return tyscm_scm_from_type (type);
}

/* (arch-uint8-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_uint8_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_uint8;

  return tyscm_scm_from_type (type);
}

/* (arch-int16-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_int16_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_int16;

  return tyscm_scm_from_type (type);
}

/* (arch-uint16-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_uint16_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_uint16;

  return tyscm_scm_from_type (type);
}

/* (arch-int32-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_int32_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_int32;

  return tyscm_scm_from_type (type);
}

/* (arch-uint32-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_uint32_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_uint32;

  return tyscm_scm_from_type (type);
}

/* (arch-int64-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_int64_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_int64;

  return tyscm_scm_from_type (type);
}

/* (arch-uint64-type <gdb:arch>) -> <gdb:type> */

static SCM
gdbscm_arch_uint64_type (SCM self)
{
  struct type *type
    = gdbscm_arch_builtin_type (self, FUNC_NAME)->builtin_uint64;

  return tyscm_scm_from_type (type);
}

/* Initialize the Scheme architecture support.  */

static const scheme_function arch_functions[] =
{
  { "arch?", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_p),
    "\
Return #t if the object is a <gdb:arch> object." },

  { "current-arch", 0, 0, 0, as_a_scm_t_subr (gdbscm_current_arch),
    "\
Return the <gdb:arch> object representing the architecture of the\n\
currently selected stack frame, if there is one, or the architecture of the\n\
current target if there isn't.\n\
\n\
  Arguments: none" },

  { "arch-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_name),
    "\
Return the name of the architecture." },

  { "arch-charset", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_charset),
  "\
Return name of target character set as a string." },

  { "arch-wide-charset", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_wide_charset),
  "\
Return name of target wide character set as a string." },

  { "arch-void-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_void_type),
    "\
Return the <gdb:type> object for the \"void\" type\n\
of the architecture." },

  { "arch-char-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_char_type),
    "\
Return the <gdb:type> object for the \"char\" type\n\
of the architecture." },

  { "arch-short-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_short_type),
    "\
Return the <gdb:type> object for the \"short\" type\n\
of the architecture." },

  { "arch-int-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_int_type),
    "\
Return the <gdb:type> object for the \"int\" type\n\
of the architecture." },

  { "arch-long-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_long_type),
    "\
Return the <gdb:type> object for the \"long\" type\n\
of the architecture." },

  { "arch-schar-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_schar_type),
    "\
Return the <gdb:type> object for the \"signed char\" type\n\
of the architecture." },

  { "arch-uchar-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_uchar_type),
    "\
Return the <gdb:type> object for the \"unsigned char\" type\n\
of the architecture." },

  { "arch-ushort-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_ushort_type),
    "\
Return the <gdb:type> object for the \"unsigned short\" type\n\
of the architecture." },

  { "arch-uint-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_uint_type),
    "\
Return the <gdb:type> object for the \"unsigned int\" type\n\
of the architecture." },

  { "arch-ulong-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_ulong_type),
    "\
Return the <gdb:type> object for the \"unsigned long\" type\n\
of the architecture." },

  { "arch-float-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_float_type),
    "\
Return the <gdb:type> object for the \"float\" type\n\
of the architecture." },

  { "arch-double-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_double_type),
    "\
Return the <gdb:type> object for the \"double\" type\n\
of the architecture." },

  { "arch-longdouble-type", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_arch_longdouble_type),
    "\
Return the <gdb:type> object for the \"long double\" type\n\
of the architecture." },

  { "arch-bool-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_bool_type),
    "\
Return the <gdb:type> object for the \"bool\" type\n\
of the architecture." },

  { "arch-longlong-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_longlong_type),
    "\
Return the <gdb:type> object for the \"long long\" type\n\
of the architecture." },

  { "arch-ulonglong-type", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_arch_ulonglong_type),
    "\
Return the <gdb:type> object for the \"unsigned long long\" type\n\
of the architecture." },

  { "arch-int8-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_int8_type),
    "\
Return the <gdb:type> object for the \"int8\" type\n\
of the architecture." },

  { "arch-uint8-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_uint8_type),
    "\
Return the <gdb:type> object for the \"uint8\" type\n\
of the architecture." },

  { "arch-int16-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_int16_type),
    "\
Return the <gdb:type> object for the \"int16\" type\n\
of the architecture." },

  { "arch-uint16-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_uint16_type),
    "\
Return the <gdb:type> object for the \"uint16\" type\n\
of the architecture." },

  { "arch-int32-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_int32_type),
    "\
Return the <gdb:type> object for the \"int32\" type\n\
of the architecture." },

  { "arch-uint32-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_uint32_type),
    "\
Return the <gdb:type> object for the \"uint32\" type\n\
of the architecture." },

  { "arch-int64-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_int64_type),
    "\
Return the <gdb:type> object for the \"int64\" type\n\
of the architecture." },

  { "arch-uint64-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_arch_uint64_type),
    "\
Return the <gdb:type> object for the \"uint64\" type\n\
of the architecture." },

  END_FUNCTIONS
};

void
gdbscm_initialize_arches (void)
{
  arch_smob_tag = gdbscm_make_smob_type (arch_smob_name, sizeof (arch_smob));
  scm_set_smob_print (arch_smob_tag, arscm_print_arch_smob);

  gdbscm_define_functions (arch_functions, 1);
}
