/* Go language support routines for GDB, the GNU debugger.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

/* TODO:
   - split stacks
   - printing of native types
   - goroutines
   - lots more
   - gccgo mangling needs redoing
     It's too hard, for example, to know whether one is looking at a mangled
     Go symbol or not, and their are ambiguities, e.g., the demangler may
     get passed *any* symbol, including symbols from other languages
     and including symbols that are already demangled.
     One thought is to at least add an _G prefix.
   - 6g mangling isn't supported yet
*/

#include "defs.h"
#include "gdbsupport/gdb_obstack.h"
#include "block.h"
#include "symtab.h"
#include "language.h"
#include "varobj.h"
#include "go-lang.h"
#include "c-lang.h"
#include "parser-defs.h"
#include "gdbarch.h"

#include <ctype.h>

/* The main function in the main package.  */
static const char GO_MAIN_MAIN[] = "main.main";

/* Function returning the special symbol name used by Go for the main
   procedure in the main program if it is found in minimal symbol list.
   This function tries to find minimal symbols so that it finds them even
   if the program was compiled without debugging information.  */

const char *
go_main_name (void)
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol (GO_MAIN_MAIN, NULL, NULL);
  if (msym.minsym != NULL)
    return GO_MAIN_MAIN;

  /* No known entry procedure found, the main program is probably not Go.  */
  return NULL;
}

/* Return non-zero if TYPE is a gccgo string.
   We assume CHECK_TYPEDEF has already been done.  */

static int
gccgo_string_p (struct type *type)
{
  /* gccgo strings don't necessarily have a name we can use.  */

  if (type->num_fields () == 2)
    {
      struct type *type0 = type->field (0).type ();
      struct type *type1 = type->field (1).type ();

      type0 = check_typedef (type0);
      type1 = check_typedef (type1);

      if (type0->code () == TYPE_CODE_PTR
	  && strcmp (type->field (0).name (), "__data") == 0
	  && type1->code () == TYPE_CODE_INT
	  && strcmp (type->field (1).name (), "__length") == 0)
	{
	  struct type *target_type = type0->target_type ();

	  target_type = check_typedef (target_type);

	  if (target_type->code () == TYPE_CODE_INT
	      && target_type->length () == 1
	      && strcmp (target_type->name (), "uint8") == 0)
	    return 1;
	}
    }

  return 0;
}

/* Return non-zero if TYPE is a 6g string.
   We assume CHECK_TYPEDEF has already been done.  */

static int
sixg_string_p (struct type *type)
{
  if (type->num_fields () == 2
      && type->name () != NULL
      && strcmp (type->name (), "string") == 0)
    return 1;

  return 0;
}

/* Classify the kind of Go object that TYPE is.
   TYPE is a TYPE_CODE_STRUCT, used to represent a Go object.  */

enum go_type
go_classify_struct_type (struct type *type)
{
  type = check_typedef (type);

  /* Recognize strings as they're useful to be able to print without
     pretty-printers.  */
  if (gccgo_string_p (type)
      || sixg_string_p (type))
    return GO_TYPE_STRING;

  return GO_TYPE_NONE;
}

/* Subroutine of unpack_mangled_go_symbol to simplify it.
   Given "[foo.]bar.baz", store "bar" in *PACKAGEP and "baz" in *OBJECTP.
   We stomp on the last '.' to nul-terminate "bar".
   The caller is responsible for memory management.  */

static void
unpack_package_and_object (char *buf,
			   const char **packagep, const char **objectp)
{
  char *last_dot;

  last_dot = strrchr (buf, '.');
  gdb_assert (last_dot != NULL);
  *objectp = last_dot + 1;
  *last_dot = '\0';
  last_dot = strrchr (buf, '.');
  if (last_dot != NULL)
    *packagep = last_dot + 1;
  else
    *packagep = buf;
}

/* Given a mangled Go symbol, find its package name, object name, and
   method type (if present).
   E.g., for "libgo_net.textproto.String.N33_libgo_net.textproto.ProtocolError"
   *PACKAGEP = "textproto"
   *OBJECTP = "String"
   *METHOD_TYPE_PACKAGEP = "textproto"
   *METHOD_TYPE_OBJECTP = "ProtocolError"

   Space for the resulting strings is malloc'd in one buffer.
   PACKAGEP,OBJECTP,METHOD_TYPE* will (typically) point into this buffer.
   A pointer to this buffer is returned, or NULL if symbol isn't a
   mangled Go symbol.

   *METHOD_TYPE_IS_POINTERP is set to a boolean indicating if
   the method type is a pointer.

   There may be value in returning the outer container,
   i.e., "net" in the above example, but for now it's not needed.
   Plus it's currently not straightforward to compute,
   it comes from -fgo-prefix, and there's no algorithm to compute it.

   If we ever need to unpack the method type, this routine should work
   for that too.  */

static gdb::unique_xmalloc_ptr<char>
unpack_mangled_go_symbol (const char *mangled_name,
			  const char **packagep,
			  const char **objectp,
			  const char **method_type_packagep,
			  const char **method_type_objectp,
			  int *method_type_is_pointerp)
{
  char *buf;
  char *p;
  int len = strlen (mangled_name);
  /* Pointer to last digit in "N<digit(s)>_".  */
  char *saw_digit;
  /* Pointer to "N" if valid "N<digit(s)>_" found.  */
  char *method_type;
  /* Pointer to the first '.'.  */
  const char *first_dot;
  /* Pointer to the last '.'.  */
  const char *last_dot;
  /* Non-zero if we saw a pointer indicator.  */
  int saw_pointer;

  *packagep = *objectp = NULL;
  *method_type_packagep = *method_type_objectp = NULL;
  *method_type_is_pointerp = 0;

  /* main.init is mangled specially.  */
  if (strcmp (mangled_name, "__go_init_main") == 0)
    {
      gdb::unique_xmalloc_ptr<char> package
	= make_unique_xstrdup ("main");

      *packagep = package.get ();
      *objectp = "init";
      return package;
    }

  /* main.main is mangled specially (missing prefix).  */
  if (strcmp (mangled_name, "main.main") == 0)
    {
      gdb::unique_xmalloc_ptr<char> package
	= make_unique_xstrdup ("main");

      *packagep = package.get ();
      *objectp = "main";
      return package;
    }

  /* We may get passed, e.g., "main.T.Foo", which is *not* mangled.
     Alas it looks exactly like "prefix.package.object."
     To cope for now we only recognize the following prefixes:

     go: the default
     libgo_.*: used by gccgo's runtime

     Thus we don't support -fgo-prefix (except as used by the runtime).  */
  bool v3;
  if (startswith (mangled_name, "go_0"))
    /* V3 mangling detected, see
       https://go-review.googlesource.com/c/gofrontend/+/271726 .  */
    v3 = true;
  else if (startswith (mangled_name, "go.")
	   || startswith (mangled_name, "libgo_"))
    v3 = false;
  else
    return NULL;

  /* Quick check for whether a search may be fruitful.  */
  /* Ignore anything with @plt, etc. in it.  */
  if (strchr (mangled_name, '@') != NULL)
    return NULL;

  /* It must have at least two dots.  */
  if (v3)
    first_dot = strchr (mangled_name, '0');
  else
    first_dot = strchr (mangled_name, '.');

  if (first_dot == NULL)
    return NULL;
  /* Treat "foo.bar" as unmangled.  It can collide with lots of other
     languages and it's not clear what the consequences are.
     And except for main.main, all gccgo symbols are at least
     prefix.package.object.  */
  last_dot = strrchr (mangled_name, '.');
  if (last_dot == first_dot)
    return NULL;

  /* More quick checks.  */
  if (last_dot[1] == '\0' /* foo. */
      || last_dot[-1] == '.') /* foo..bar */
    return NULL;

  /* At this point we've decided we have a mangled Go symbol.  */

  gdb::unique_xmalloc_ptr<char> result = make_unique_xstrdup (mangled_name);
  buf = result.get ();

  if (v3)
    {
      /* Replace "go_0" with "\0go.".  */
      buf[0] = '\0';
      buf[1] = 'g';
      buf[2] = 'o';
      buf[3] = '.';

      /* Skip the '\0'.  */
      buf++;
    }

  /* Search backwards looking for "N<digit(s)>".  */
  p = buf + len;
  saw_digit = method_type = NULL;
  saw_pointer = 0;
  while (p > buf)
    {
      int current = *(const unsigned char *) --p;
      int current_is_digit = isdigit (current);

      if (saw_digit)
	{
	  if (current_is_digit)
	    continue;
	  if (current == 'N'
	      && ((p > buf && p[-1] == '.')
		  || (p > buf + 1 && p[-1] == 'p' && p[-2] == '.')))
	    {
	      if (atoi (p + 1) == strlen (saw_digit + 2))
		{
		  if (p[-1] == '.')
		    method_type = p - 1;
		  else
		    {
		      gdb_assert (p[-1] == 'p');
		      saw_pointer = 1;
		      method_type = p - 2;
		    }
		  break;
		}
	    }
	  /* Not what we're looking for, reset and keep looking.  */
	  saw_digit = NULL;
	  saw_pointer = 0;
	  continue;
	}
      if (current_is_digit && p[1] == '_')
	{
	  /* Possible start of method "this" [sic] type.  */
	  saw_digit = p;
	  continue;
	}
    }

  if (method_type != NULL
      /* Ensure not something like "..foo".  */
      && (method_type > buf && method_type[-1] != '.'))
    {
      unpack_package_and_object (saw_digit + 2,
				 method_type_packagep, method_type_objectp);
      *method_type = '\0';
      *method_type_is_pointerp = saw_pointer;
    }

  unpack_package_and_object (buf, packagep, objectp);
  return result;
}

/* Implements the la_demangle language_defn routine for language Go.

   N.B. This may get passed *any* symbol, including symbols from other
   languages and including symbols that are already demangled.
   Both of these situations are kinda unfortunate, but that's how things
   are today.

   N.B. This currently only supports gccgo's mangling.

   N.B. gccgo's mangling needs, I think, changing.
   This demangler can't work in all situations,
   thus not too much effort is currently put into it.  */

gdb::unique_xmalloc_ptr<char>
go_language::demangle_symbol (const char *mangled_name, int options) const
{
  const char *package_name;
  const char *object_name;
  const char *method_type_package_name;
  const char *method_type_object_name;
  int method_type_is_pointer;

  if (mangled_name == NULL)
    return NULL;

  gdb::unique_xmalloc_ptr<char> name_buf
    (unpack_mangled_go_symbol (mangled_name,
			       &package_name, &object_name,
			       &method_type_package_name,
			       &method_type_object_name,
			       &method_type_is_pointer));
  if (name_buf == NULL)
    return NULL;

  auto_obstack tempbuf;

  /* Print methods as they appear in "method expressions".  */
  if (method_type_package_name != NULL)
    {
      /* FIXME: Seems like we should include package_name here somewhere.  */
      if (method_type_is_pointer)
	  obstack_grow_str (&tempbuf, "(*");
      obstack_grow_str (&tempbuf, method_type_package_name);
      obstack_grow_str (&tempbuf, ".");
      obstack_grow_str (&tempbuf, method_type_object_name);
      if (method_type_is_pointer)
	obstack_grow_str (&tempbuf, ")");
      obstack_grow_str (&tempbuf, ".");
      obstack_grow_str (&tempbuf, object_name);
    }
  else
    {
      obstack_grow_str (&tempbuf, package_name);
      obstack_grow_str (&tempbuf, ".");
      obstack_grow_str (&tempbuf, object_name);
    }
  obstack_grow_str0 (&tempbuf, "");

  return make_unique_xstrdup ((const char *) obstack_finish (&tempbuf));
}

/* See go-lang.h.  */

gdb::unique_xmalloc_ptr<char>
go_symbol_package_name (const struct symbol *sym)
{
  const char *mangled_name = sym->linkage_name ();
  const char *package_name;
  const char *object_name;
  const char *method_type_package_name;
  const char *method_type_object_name;
  int method_type_is_pointer;
  gdb::unique_xmalloc_ptr<char> name_buf;

  if (sym->language () != language_go)
    return nullptr;
  name_buf = unpack_mangled_go_symbol (mangled_name,
				       &package_name, &object_name,
				       &method_type_package_name,
				       &method_type_object_name,
				       &method_type_is_pointer);
  /* Some Go symbols don't have mangled form we interpret (yet).  */
  if (name_buf == NULL)
    return NULL;
  return make_unique_xstrdup (package_name);
}

/* See go-lang.h.  */

gdb::unique_xmalloc_ptr<char>
go_block_package_name (const struct block *block)
{
  while (block != NULL)
    {
      struct symbol *function = block->function ();

      if (function != NULL)
	{
	  gdb::unique_xmalloc_ptr<char> package_name
	    = go_symbol_package_name (function);

	  if (package_name != NULL)
	    return package_name;

	  /* Stop looking if we find a function without a package name.
	     We're most likely outside of Go and thus the concept of the
	     "current" package is gone.  */
	  return NULL;
	}

      block = block->superblock ();
    }

  return NULL;
}

/* See language.h.  */

void
go_language::language_arch_info (struct gdbarch *gdbarch,
				 struct language_arch_info *lai) const
{
  const struct builtin_go_type *builtin = builtin_go_type (gdbarch);

  /* Helper function to allow shorter lines below.  */
  auto add  = [&] (struct type * t) -> struct type *
  {
    lai->add_primitive_type (t);
    return t;
  };

  add (builtin->builtin_void);
  add (builtin->builtin_char);
  add (builtin->builtin_bool);
  add (builtin->builtin_int);
  add (builtin->builtin_uint);
  add (builtin->builtin_uintptr);
  add (builtin->builtin_int8);
  add (builtin->builtin_int16);
  add (builtin->builtin_int32);
  add (builtin->builtin_int64);
  add (builtin->builtin_uint8);
  add (builtin->builtin_uint16);
  add (builtin->builtin_uint32);
  add (builtin->builtin_uint64);
  add (builtin->builtin_float32);
  add (builtin->builtin_float64);
  add (builtin->builtin_complex64);
  add (builtin->builtin_complex128);

  lai->set_string_char_type (builtin->builtin_char);
  lai->set_bool_type (builtin->builtin_bool, "bool");
}

/* Single instance of the Go language class.  */

static go_language go_language_defn;

static struct builtin_go_type *
build_go_types (struct gdbarch *gdbarch)
{
  struct builtin_go_type *builtin_go_type = new struct builtin_go_type;

  type_allocator alloc (gdbarch);
  builtin_go_type->builtin_void = builtin_type (gdbarch)->builtin_void;
  builtin_go_type->builtin_char
    = init_character_type (alloc, 8, 1, "char");
  builtin_go_type->builtin_bool
    = init_boolean_type (alloc, 8, 0, "bool");
  builtin_go_type->builtin_int
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch), 0, "int");
  builtin_go_type->builtin_uint
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch), 1, "uint");
  builtin_go_type->builtin_uintptr
    = init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 1, "uintptr");
  builtin_go_type->builtin_int8
    = init_integer_type (alloc, 8, 0, "int8");
  builtin_go_type->builtin_int16
    = init_integer_type (alloc, 16, 0, "int16");
  builtin_go_type->builtin_int32
    = init_integer_type (alloc, 32, 0, "int32");
  builtin_go_type->builtin_int64
    = init_integer_type (alloc, 64, 0, "int64");
  builtin_go_type->builtin_uint8
    = init_integer_type (alloc, 8, 1, "uint8");
  builtin_go_type->builtin_uint16
    = init_integer_type (alloc, 16, 1, "uint16");
  builtin_go_type->builtin_uint32
    = init_integer_type (alloc, 32, 1, "uint32");
  builtin_go_type->builtin_uint64
    = init_integer_type (alloc, 64, 1, "uint64");
  builtin_go_type->builtin_float32
    = init_float_type (alloc, 32, "float32", floatformats_ieee_single);
  builtin_go_type->builtin_float64
    = init_float_type (alloc, 64, "float64", floatformats_ieee_double);
  builtin_go_type->builtin_complex64
    = init_complex_type ("complex64", builtin_go_type->builtin_float32);
  builtin_go_type->builtin_complex128
    = init_complex_type ("complex128", builtin_go_type->builtin_float64);

  return builtin_go_type;
}

static const registry<gdbarch>::key<struct builtin_go_type> go_type_data;

const struct builtin_go_type *
builtin_go_type (struct gdbarch *gdbarch)
{
  struct builtin_go_type *result = go_type_data.get (gdbarch);
  if (result == nullptr)
    {
      result = build_go_types (gdbarch);
      go_type_data.set (gdbarch, result);
    }

  return result;
}
