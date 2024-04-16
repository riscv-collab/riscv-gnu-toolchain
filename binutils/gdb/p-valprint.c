/* Support for printing Pascal values for GDB, the GNU debugger.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

/* This file is derived from c-valprint.c */

#include "defs.h"
#include "gdbsupport/gdb_obstack.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "demangle.h"
#include "valprint.h"
#include "typeprint.h"
#include "language.h"
#include "target.h"
#include "annotate.h"
#include "p-lang.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "objfiles.h"
#include "gdbsupport/byte-vector.h"
#include "cli/cli-style.h"


static void pascal_object_print_value_fields (struct value *, struct ui_file *,
					      int,
					      const struct value_print_options *,
					      struct type **, int);

/* Decorations for Pascal.  */

static const struct generic_val_print_decorations p_decorations =
{
  "",
  " + ",
  " * I",
  "true",
  "false",
  "void",
  "{",
  "}"
};

/* See p-lang.h.  */

void
pascal_language::value_print_inner (struct value *val,
				    struct ui_file *stream, int recurse,
				    const struct value_print_options *options) const

{
  struct type *type = check_typedef (val->type ());
  struct gdbarch *gdbarch = type->arch ();
  enum bfd_endian byte_order = type_byte_order (type);
  unsigned int i = 0;	/* Number of characters printed */
  unsigned len;
  struct type *elttype;
  unsigned eltlen;
  int length_pos, length_size, string_pos;
  struct type *char_type;
  CORE_ADDR addr;
  int want_space = 0;
  const gdb_byte *valaddr = val->contents_for_printing ().data ();

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;

	if (get_array_bounds (type, &low_bound, &high_bound))
	  {
	    len = high_bound - low_bound + 1;
	    elttype = check_typedef (type->target_type ());
	    eltlen = elttype->length ();
	    /* If 's' format is used, try to print out as string.
	       If no format is given, print as string if element type
	       is of TYPE_CODE_CHAR and element size is 1,2 or 4.  */
	    if (options->format == 's'
		|| ((eltlen == 1 || eltlen == 2 || eltlen == 4)
		    && elttype->code () == TYPE_CODE_CHAR
		    && options->format == 0))
	      {
		/* If requested, look for the first null char and only print
		   elements up to it.  */
		if (options->stop_print_at_null)
		  {
		    unsigned int print_max_chars
		      = get_print_max_chars (options);
		    unsigned int temp_len;

		    /* Look for a NULL char.  */
		    for (temp_len = 0;
			 (extract_unsigned_integer
			    (valaddr + temp_len * eltlen, eltlen, byte_order)
			  && temp_len < len
			  && temp_len < print_max_chars);
			 temp_len++);
		    len = temp_len;
		  }

		printstr (stream, type->target_type (), valaddr, len,
			  NULL, 0, options);
		i = len;
	      }
	    else
	      {
		gdb_printf (stream, "{");
		/* If this is a virtual function table, print the 0th
		   entry specially, and the rest of the members normally.  */
		if (pascal_object_is_vtbl_ptr_type (elttype))
		  {
		    i = 1;
		    gdb_printf (stream, "%d vtable entries", len - 1);
		  }
		else
		  {
		    i = 0;
		  }
		value_print_array_elements (val, stream, recurse, options, i);
		gdb_printf (stream, "}");
	      }
	    break;
	  }
	/* Array of unspecified length: treat like pointer to first elt.  */
	addr = val->address ();
      }
      goto print_unpacked_pointer;

    case TYPE_CODE_PTR:
      if (options->format && options->format != 's')
	{
	  value_print_scalar_formatted (val, options, 0, stream);
	  break;
	}
      if (options->vtblprint && pascal_object_is_vtbl_ptr_type (type))
	{
	  /* Print the unmangled name if desired.  */
	  /* Print vtable entry - we only get here if we ARE using
	     -fvtable_thunks.  (Otherwise, look under TYPE_CODE_STRUCT.)  */
	  /* Extract the address, assume that it is unsigned.  */
	  addr = extract_unsigned_integer (valaddr,
					   type->length (), byte_order);
	  print_address_demangle (options, gdbarch, addr, stream, demangle);
	  break;
	}
      check_typedef (type->target_type ());

      addr = unpack_pointer (type, valaddr);
    print_unpacked_pointer:
      elttype = check_typedef (type->target_type ());

      if (elttype->code () == TYPE_CODE_FUNC)
	{
	  /* Try to print what function it points to.  */
	  print_address_demangle (options, gdbarch, addr, stream, demangle);
	  return;
	}

      if (options->addressprint && options->format != 's')
	{
	  gdb_puts (paddress (gdbarch, addr), stream);
	  want_space = 1;
	}

      /* For a pointer to char or unsigned char, also print the string
	 pointed to, unless pointer is null.  */
      if (((elttype->length () == 1
	   && (elttype->code () == TYPE_CODE_INT
	       || elttype->code () == TYPE_CODE_CHAR))
	   || ((elttype->length () == 2 || elttype->length () == 4)
	       && elttype->code () == TYPE_CODE_CHAR))
	  && (options->format == 0 || options->format == 's')
	  && addr != 0)
	{
	  if (want_space)
	    gdb_puts (" ", stream);
	  /* No wide string yet.  */
	  i = val_print_string (elttype, NULL, addr, -1, stream, options);
	}
      /* Also for pointers to pascal strings.  */
      /* Note: this is Free Pascal specific:
	 as GDB does not recognize stabs pascal strings
	 Pascal strings are mapped to records
	 with lowercase names PM.  */
      if (pascal_is_string_type (elttype, &length_pos, &length_size,
				 &string_pos, &char_type, NULL) > 0
	  && addr != 0)
	{
	  ULONGEST string_length;
	  gdb_byte *buffer;

	  if (want_space)
	    gdb_puts (" ", stream);
	  buffer = (gdb_byte *) xmalloc (length_size);
	  read_memory (addr + length_pos, buffer, length_size);
	  string_length = extract_unsigned_integer (buffer, length_size,
						    byte_order);
	  xfree (buffer);
	  i = val_print_string (char_type, NULL,
				addr + string_pos, string_length,
				stream, options);
	}
      else if (pascal_object_is_vtbl_member (type))
	{
	  /* Print vtbl's nicely.  */
	  CORE_ADDR vt_address = unpack_pointer (type, valaddr);
	  struct bound_minimal_symbol msymbol =
	    lookup_minimal_symbol_by_pc (vt_address);

	  /* If 'symbol_print' is set, we did the work above.  */
	  if (!options->symbol_print
	      && (msymbol.minsym != NULL)
	      && (vt_address == msymbol.value_address ()))
	    {
	      if (want_space)
		gdb_puts (" ", stream);
	      gdb_puts ("<", stream);
	      gdb_puts (msymbol.minsym->print_name (), stream);
	      gdb_puts (">", stream);
	      want_space = 1;
	    }
	  if (vt_address && options->vtblprint)
	    {
	      struct value *vt_val;
	      struct symbol *wsym = NULL;
	      struct type *wtype;

	      if (want_space)
		gdb_puts (" ", stream);

	      if (msymbol.minsym != NULL)
		{
		  const char *search_name = msymbol.minsym->search_name ();
		  wsym = lookup_symbol_search_name (search_name, NULL,
						    VAR_DOMAIN).symbol;
		}

	      if (wsym)
		{
		  wtype = wsym->type ();
		}
	      else
		{
		  wtype = type->target_type ();
		}
	      vt_val = value_at (wtype, vt_address);
	      common_val_print (vt_val, stream, recurse + 1, options,
				current_language);
	      if (options->prettyformat)
		{
		  gdb_printf (stream, "\n");
		  print_spaces (2 + 2 * recurse, stream);
		}
	    }
	}

      return;

    case TYPE_CODE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
      generic_value_print (val, stream, recurse, options, &p_decorations);
      break;

    case TYPE_CODE_UNION:
      if (recurse && !options->unionprint)
	{
	  gdb_printf (stream, "{...}");
	  break;
	}
      [[fallthrough]];
    case TYPE_CODE_STRUCT:
      if (options->vtblprint && pascal_object_is_vtbl_ptr_type (type))
	{
	  /* Print the unmangled name if desired.  */
	  /* Print vtable entry - we only get here if NOT using
	     -fvtable_thunks.  (Otherwise, look under TYPE_CODE_PTR.)  */
	  /* Extract the address, assume that it is unsigned.  */
	  print_address_demangle
	    (options, gdbarch,
	     extract_unsigned_integer
	       (valaddr + type->field (VTBL_FNADDR_OFFSET).loc_bitpos () / 8,
		type->field (VTBL_FNADDR_OFFSET).type ()->length (),
		byte_order),
	     stream, demangle);
	}
      else
	{
	  if (pascal_is_string_type (type, &length_pos, &length_size,
				     &string_pos, &char_type, NULL) > 0)
	    {
	      len = extract_unsigned_integer (valaddr + length_pos,
					      length_size, byte_order);
	      printstr (stream, char_type, valaddr + string_pos, len,
			NULL, 0, options);
	    }
	  else
	    pascal_object_print_value_fields (val, stream, recurse,
					      options, NULL, 0);
	}
      break;

    case TYPE_CODE_SET:
      elttype = type->index_type ();
      elttype = check_typedef (elttype);
      if (elttype->is_stub ())
	{
	  fprintf_styled (stream, metadata_style.style (), "<incomplete type>");
	  break;
	}
      else
	{
	  struct type *range = elttype;
	  LONGEST low_bound, high_bound;
	  int need_comma = 0;

	  gdb_puts ("[", stream);

	  int bound_info = (get_discrete_bounds (range, &low_bound, &high_bound)
			    ? 0 : -1);
	  if (low_bound == 0 && high_bound == -1 && type->length () > 0)
	    {
	      /* If we know the size of the set type, we can figure out the
	      maximum value.  */
	      bound_info = 0;
	      high_bound = type->length () * TARGET_CHAR_BIT - 1;
	      range->bounds ()->high.set_const_val (high_bound);
	    }
	maybe_bad_bstring:
	  if (bound_info < 0)
	    {
	      fputs_styled ("<error value>", metadata_style.style (), stream);
	      goto done;
	    }

	  for (i = low_bound; i <= high_bound; i++)
	    {
	      int element = value_bit_index (type, valaddr, i);

	      if (element < 0)
		{
		  i = element;
		  goto maybe_bad_bstring;
		}
	      if (element)
		{
		  if (need_comma)
		    gdb_puts (", ", stream);
		  print_type_scalar (range, i, stream);
		  need_comma = 1;

		  if (i + 1 <= high_bound
		      && value_bit_index (type, valaddr, ++i))
		    {
		      int j = i;

		      gdb_puts ("..", stream);
		      while (i + 1 <= high_bound
			     && value_bit_index (type, valaddr, ++i))
			j = i;
		      print_type_scalar (range, j, stream);
		    }
		}
	    }
	done:
	  gdb_puts ("]", stream);
	}
      break;

    default:
      error (_("Invalid pascal type code %d in symbol table."),
	     type->code ());
    }
}


void
pascal_language::value_print (struct value *val, struct ui_file *stream,
			      const struct value_print_options *options) const
{
  struct type *type = val->type ();
  struct value_print_options opts = *options;

  opts.deref_ref = true;

  /* If it is a pointer, indicate what it points to.

     Print type also if it is a reference.

     Object pascal: if it is a member pointer, we will take care
     of that when we print it.  */
  if (type->code () == TYPE_CODE_PTR
      || type->code () == TYPE_CODE_REF)
    {
      /* Hack:  remove (char *) for char strings.  Their
	 type is indicated by the quoted string anyway.  */
      if (type->code () == TYPE_CODE_PTR
	  && type->name () == NULL
	  && type->target_type ()->name () != NULL
	  && strcmp (type->target_type ()->name (), "char") == 0)
	{
	  /* Print nothing.  */
	}
      else
	{
	  gdb_printf (stream, "(");
	  type_print (type, "", stream, -1);
	  gdb_printf (stream, ") ");
	}
    }
  common_val_print (val, stream, 0, &opts, current_language);
}


static void
show_pascal_static_field_print (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of pascal static members is %s.\n"),
	      value);
}

static struct obstack dont_print_vb_obstack;
static struct obstack dont_print_statmem_obstack;

static void pascal_object_print_static_field (struct value *,
					      struct ui_file *, int,
					      const struct value_print_options *);

static void pascal_object_print_value (struct value *, struct ui_file *, int,
				       const struct value_print_options *,
				       struct type **);

/* It was changed to this after 2.4.5.  */
const char pascal_vtbl_ptr_name[] =
{'_', '_', 'v', 't', 'b', 'l', '_', 'p', 't', 'r', '_', 't', 'y', 'p', 'e', 0};

/* Return truth value for assertion that TYPE is of the type
   "pointer to virtual function".  */

int
pascal_object_is_vtbl_ptr_type (struct type *type)
{
  const char *type_name = type->name ();

  return (type_name != NULL
	  && strcmp (type_name, pascal_vtbl_ptr_name) == 0);
}

/* Return truth value for the assertion that TYPE is of the type
   "pointer to virtual function table".  */

int
pascal_object_is_vtbl_member (struct type *type)
{
  if (type->code () == TYPE_CODE_PTR)
    {
      type = type->target_type ();
      if (type->code () == TYPE_CODE_ARRAY)
	{
	  type = type->target_type ();
	  if (type->code () == TYPE_CODE_STRUCT	/* If not using
							   thunks.  */
	      || type->code () == TYPE_CODE_PTR)	/* If using thunks.  */
	    {
	      /* Virtual functions tables are full of pointers
		 to virtual functions.  */
	      return pascal_object_is_vtbl_ptr_type (type);
	    }
	}
    }
  return 0;
}

/* Helper function for print pascal objects.

   VAL, STREAM, RECURSE, and OPTIONS have the same meanings as in
   pascal_object_print_value and c_value_print.

   DONT_PRINT is an array of baseclass types that we
   should not print, or zero if called from top level.  */

static void
pascal_object_print_value_fields (struct value *val, struct ui_file *stream,
				  int recurse,
				  const struct value_print_options *options,
				  struct type **dont_print_vb,
				  int dont_print_statmem)
{
  int i, len, n_baseclasses;
  char *last_dont_print
    = (char *) obstack_next_free (&dont_print_statmem_obstack);

  struct type *type = check_typedef (val->type ());

  gdb_printf (stream, "{");
  len = type->num_fields ();
  n_baseclasses = TYPE_N_BASECLASSES (type);

  /* Print out baseclasses such that we don't print
     duplicates of virtual baseclasses.  */
  if (n_baseclasses > 0)
    pascal_object_print_value (val, stream, recurse + 1,
			       options, dont_print_vb);

  if (!len && n_baseclasses == 1)
    fprintf_styled (stream, metadata_style.style (), "<No data fields>");
  else
    {
      struct obstack tmp_obstack = dont_print_statmem_obstack;
      int fields_seen = 0;
      const gdb_byte *valaddr = val->contents_for_printing ().data ();

      if (dont_print_statmem == 0)
	{
	  /* If we're at top level, carve out a completely fresh
	     chunk of the obstack and use that until this particular
	     invocation returns.  */
	  obstack_finish (&dont_print_statmem_obstack);
	}

      for (i = n_baseclasses; i < len; i++)
	{
	  /* If requested, skip printing of static fields.  */
	  if (!options->pascal_static_field_print
	      && type->field (i).is_static ())
	    continue;
	  if (fields_seen)
	    gdb_printf (stream, ", ");
	  else if (n_baseclasses > 0)
	    {
	      if (options->prettyformat)
		{
		  gdb_printf (stream, "\n");
		  print_spaces (2 + 2 * recurse, stream);
		  gdb_puts ("members of ", stream);
		  gdb_puts (type->name (), stream);
		  gdb_puts (": ", stream);
		}
	    }
	  fields_seen = 1;

	  if (options->prettyformat)
	    {
	      gdb_printf (stream, "\n");
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  else
	    {
	      stream->wrap_here (2 + 2 * recurse);
	    }

	  annotate_field_begin (type->field (i).type ());

	  if (type->field (i).is_static ())
	    {
	      gdb_puts ("static ", stream);
	      fprintf_symbol (stream,
			      type->field (i).name (),
			      current_language->la_language,
			      DMGL_PARAMS | DMGL_ANSI);
	    }
	  else
	    fputs_styled (type->field (i).name (),
			  variable_name_style.style (), stream);
	  annotate_field_name_end ();
	  gdb_puts (" = ", stream);
	  annotate_field_value ();

	  if (!type->field (i).is_static ()
	      && type->field (i).is_packed ())
	    {
	      struct value *v;

	      /* Bitfields require special handling, especially due to byte
		 order problems.  */
	      if (type->field (i).is_ignored ())
		{
		  fputs_styled ("<optimized out or zero length>",
				metadata_style.style (), stream);
		}
	      else if (val->bits_synthetic_pointer
		       (type->field (i).loc_bitpos (),
			type->field (i).bitsize ()))
		{
		  fputs_styled (_("<synthetic pointer>"),
				metadata_style.style (), stream);
		}
	      else
		{
		  struct value_print_options opts = *options;

		  v = value_field_bitfield (type, i, valaddr, 0, val);

		  opts.deref_ref = false;
		  common_val_print (v, stream, recurse + 1, &opts,
				    current_language);
		}
	    }
	  else
	    {
	      if (type->field (i).is_ignored ())
		{
		  fputs_styled ("<optimized out or zero length>",
				metadata_style.style (), stream);
		}
	      else if (type->field (i).is_static ())
		{
		  /* struct value *v = value_static_field (type, i);
		     v4.17 specific.  */
		  struct value *v;

		  v = value_field_bitfield (type, i, valaddr, 0, val);

		  if (v == NULL)
		    val_print_optimized_out (NULL, stream);
		  else
		    pascal_object_print_static_field (v, stream, recurse + 1,
						      options);
		}
	      else
		{
		  struct value_print_options opts = *options;

		  opts.deref_ref = false;

		  struct value *v = val->primitive_field (0, i,
							  val->type ());
		  common_val_print (v, stream, recurse + 1, &opts,
				    current_language);
		}
	    }
	  annotate_field_end ();
	}

      if (dont_print_statmem == 0)
	{
	  /* Free the space used to deal with the printing
	     of the members from top level.  */
	  obstack_free (&dont_print_statmem_obstack, last_dont_print);
	  dont_print_statmem_obstack = tmp_obstack;
	}

      if (options->prettyformat)
	{
	  gdb_printf (stream, "\n");
	  print_spaces (2 * recurse, stream);
	}
    }
  gdb_printf (stream, "}");
}

/* Special val_print routine to avoid printing multiple copies of virtual
   baseclasses.  */

static void
pascal_object_print_value (struct value *val, struct ui_file *stream,
			   int recurse,
			   const struct value_print_options *options,
			   struct type **dont_print_vb)
{
  struct type **last_dont_print
    = (struct type **) obstack_next_free (&dont_print_vb_obstack);
  struct obstack tmp_obstack = dont_print_vb_obstack;
  struct type *type = check_typedef (val->type ());
  int i, n_baseclasses = TYPE_N_BASECLASSES (type);

  if (dont_print_vb == 0)
    {
      /* If we're at top level, carve out a completely fresh
	 chunk of the obstack and use that until this particular
	 invocation returns.  */
      /* Bump up the high-water mark.  Now alpha is omega.  */
      obstack_finish (&dont_print_vb_obstack);
    }

  for (i = 0; i < n_baseclasses; i++)
    {
      LONGEST boffset = 0;
      struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
      const char *basename = baseclass->name ();
      int skip = 0;

      if (BASETYPE_VIA_VIRTUAL (type, i))
	{
	  struct type **first_dont_print
	    = (struct type **) obstack_base (&dont_print_vb_obstack);

	  int j = (struct type **) obstack_next_free (&dont_print_vb_obstack)
	    - first_dont_print;

	  while (--j >= 0)
	    if (baseclass == first_dont_print[j])
	      goto flush_it;

	  obstack_ptr_grow (&dont_print_vb_obstack, baseclass);
	}

      struct value *base_value;
      try
	{
	  base_value = val->primitive_field (0, i, type);
	}
      catch (const gdb_exception_error &ex)
	{
	  base_value = nullptr;
	  if (ex.error == NOT_AVAILABLE_ERROR)
	    skip = -1;
	  else
	    skip = 1;
	}

      if (skip == 0)
	{
	  /* The virtual base class pointer might have been clobbered by the
	     user program. Make sure that it still points to a valid memory
	     location.  */

	  if (boffset < 0 || boffset >= type->length ())
	    {
	      CORE_ADDR address= val->address ();
	      gdb::byte_vector buf (baseclass->length ());

	      if (target_read_memory (address + boffset, buf.data (),
				      baseclass->length ()) != 0)
		skip = 1;
	      base_value = value_from_contents_and_address (baseclass,
							    buf.data (),
							    address + boffset);
	      baseclass = base_value->type ();
	      boffset = 0;
	    }
	}

      if (options->prettyformat)
	{
	  gdb_printf (stream, "\n");
	  print_spaces (2 * recurse, stream);
	}
      gdb_puts ("<", stream);
      /* Not sure what the best notation is in the case where there is no
	 baseclass name.  */

      gdb_puts (basename ? basename : "", stream);
      gdb_puts ("> = ", stream);

      if (skip < 0)
	val_print_unavailable (stream);
      else if (skip > 0)
	val_print_invalid_address (stream);
      else
	pascal_object_print_value_fields
	  (base_value, stream, recurse, options,
	   (struct type **) obstack_base (&dont_print_vb_obstack),
	   0);
      gdb_puts (", ", stream);

    flush_it:
      ;
    }

  if (dont_print_vb == 0)
    {
      /* Free the space used to deal with the printing
	 of this type from top level.  */
      obstack_free (&dont_print_vb_obstack, last_dont_print);
      /* Reset watermark so that we can continue protecting
	 ourselves from whatever we were protecting ourselves.  */
      dont_print_vb_obstack = tmp_obstack;
    }
}

/* Print value of a static member.
   To avoid infinite recursion when printing a class that contains
   a static instance of the class, we keep the addresses of all printed
   static member classes in an obstack and refuse to print them more
   than once.

   VAL contains the value to print, STREAM, RECURSE, and OPTIONS
   have the same meanings as in c_val_print.  */

static void
pascal_object_print_static_field (struct value *val,
				  struct ui_file *stream,
				  int recurse,
				  const struct value_print_options *options)
{
  struct type *type = val->type ();
  struct value_print_options opts;

  if (val->entirely_optimized_out ())
    {
      val_print_optimized_out (val, stream);
      return;
    }

  if (type->code () == TYPE_CODE_STRUCT)
    {
      CORE_ADDR *first_dont_print, addr;
      int i;

      first_dont_print
	= (CORE_ADDR *) obstack_base (&dont_print_statmem_obstack);
      i = (CORE_ADDR *) obstack_next_free (&dont_print_statmem_obstack)
	- first_dont_print;

      while (--i >= 0)
	{
	  if (val->address () == first_dont_print[i])
	    {
	      fputs_styled (_("\
<same as static member of an already seen type>"),
			    metadata_style.style (), stream);
	      return;
	    }
	}

      addr = val->address ();
      obstack_grow (&dont_print_statmem_obstack, (char *) &addr,
		    sizeof (CORE_ADDR));

      type = check_typedef (type);
      pascal_object_print_value_fields (val, stream, recurse,
					options, NULL, 1);
      return;
    }

  opts = *options;
  opts.deref_ref = false;
  common_val_print (val, stream, recurse, &opts, current_language);
}

void _initialize_pascal_valprint ();
void
_initialize_pascal_valprint ()
{
  add_setshow_boolean_cmd ("pascal_static-members", class_support,
			   &user_print_options.pascal_static_field_print, _("\
Set printing of pascal static members."), _("\
Show printing of pascal static members."), NULL,
			   NULL,
			   show_pascal_static_field_print,
			   &setprintlist, &showprintlist);
}
