/* Support for printing Fortran values for GDB, the GNU debugger.

   Copyright (C) 1993-2024 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C definitions by Farooq Butt
   (fmbutt@engage.sps.mot.com), additionally worked over by Stan Shebs.

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
#include "annotate.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "f-lang.h"
#include "frame.h"
#include "gdbcore.h"
#include "command.h"
#include "block.h"
#include "dictionary.h"
#include "cli/cli-style.h"
#include "gdbarch.h"
#include "f-array-walker.h"

static void f77_get_dynamic_length_of_aggregate (struct type *);

LONGEST
f77_get_lowerbound (struct type *type)
{
  if (!type->bounds ()->low.is_constant ())
    error (_("Lower bound may not be '*' in F77"));

  return type->bounds ()->low.const_val ();
}

LONGEST
f77_get_upperbound (struct type *type)
{
  if (!type->bounds ()->high.is_constant ())
    {
      /* We have an assumed size array on our hands.  Assume that
	 upper_bound == lower_bound so that we show at least 1 element.
	 If the user wants to see more elements, let him manually ask for 'em
	 and we'll subscript the array and show him.  */

      return f77_get_lowerbound (type);
    }

  return type->bounds ()->high.const_val ();
}

/* Obtain F77 adjustable array dimensions.  */

static void
f77_get_dynamic_length_of_aggregate (struct type *type)
{
  int upper_bound = -1;
  int lower_bound = 1;

  /* Recursively go all the way down into a possibly multi-dimensional
     F77 array and get the bounds.  For simple arrays, this is pretty
     easy but when the bounds are dynamic, we must be very careful 
     to add up all the lengths correctly.  Not doing this right 
     will lead to horrendous-looking arrays in parameter lists.

     This function also works for strings which behave very 
     similarly to arrays.  */

  if (type->target_type ()->code () == TYPE_CODE_ARRAY
      || type->target_type ()->code () == TYPE_CODE_STRING)
    f77_get_dynamic_length_of_aggregate (type->target_type ());

  /* Recursion ends here, start setting up lengths.  */
  lower_bound = f77_get_lowerbound (type);
  upper_bound = f77_get_upperbound (type);

  /* Patch in a valid length value.  */
  type->set_length ((upper_bound - lower_bound + 1)
		    * check_typedef (type->target_type ())->length ());
}

/* Per-dimension statistics.  */

struct dimension_stats
{
  /* The type of the index used to address elements in the dimension.  */
  struct type *index_type;

  /* Total number of elements in the dimension, counted as we go.  */
  int nelts;
};

/* A class used by FORTRAN_PRINT_ARRAY as a specialisation of the array
   walking template.  This specialisation prints Fortran arrays.  */

class fortran_array_printer_impl : public fortran_array_walker_base_impl
{
public:
  /* Constructor.  TYPE is the array type being printed, ADDRESS is the
     address in target memory for the object of TYPE being printed.  VAL is
     the GDB value (of TYPE) being printed.  STREAM is where to print to,
     RECOURSE is passed through (and prevents infinite recursion), and
     OPTIONS are the printing control options.  */
  explicit fortran_array_printer_impl (struct type *type,
				       CORE_ADDR address,
				       struct value *val,
				       struct ui_file *stream,
				       int recurse,
				       const struct value_print_options *options)
    : m_elts (0),
      m_val (val),
      m_stream (stream),
      m_recurse (recurse),
      m_options (options),
      m_dimension (0),
      m_nrepeats (0),
      m_stats (0)
  { /* Nothing.  */ }

  /* Called while iterating over the array bounds.  When SHOULD_CONTINUE is
     false then we must return false, as we have reached the end of the
     array bounds for this dimension.  However, we also return false if we
     have printed too many elements (after printing '...').  In all other
     cases, return true.  */
  bool continue_walking (bool should_continue)
  {
    bool cont = should_continue && (m_elts < m_options->print_max);
    if (!cont && should_continue)
      gdb_puts ("...", m_stream);
    return cont;
  }

  /* Called when we start iterating over a dimension.  If it's not the
     inner most dimension then print an opening '(' character.  */
  void start_dimension (struct type *index_type, LONGEST nelts, bool inner_p)
  {
    size_t dim_indx = m_dimension++;

    m_elt_type_prev = nullptr;
    if (m_stats.size () < m_dimension)
      {
	m_stats.resize (m_dimension);
	m_stats[dim_indx].index_type = index_type;
	m_stats[dim_indx].nelts = nelts;
      }

    gdb_puts ("(", m_stream);
  }

  /* Called when we finish processing a batch of items within a dimension
     of the array.  Depending on whether this is the inner most dimension
     or not we print different things, but this is all about adding
     separators between elements, and dimensions of the array.  */
  void finish_dimension (bool inner_p, bool last_p)
  {
    gdb_puts (")", m_stream);
    if (!last_p)
      gdb_puts (" ", m_stream);

    m_dimension--;
  }

  /* Called when processing dimensions of the array other than the
     innermost one.  WALK_1 is the walker to normally call, ELT_TYPE is
     the type of the element being extracted, and ELT_OFF is the offset
     of the element from the start of array being walked, INDEX_TYPE
     and INDEX is the type and the value respectively of the element's
     index in the dimension currently being walked and LAST_P is true
     only when this is the last element that will be processed in this
     dimension.  */
  void process_dimension (gdb::function_view<void (struct type *,
						   int, bool)> walk_1,
			  struct type *elt_type, LONGEST elt_off,
			  LONGEST index, bool last_p)
  {
    size_t dim_indx = m_dimension - 1;
    struct type *elt_type_prev = m_elt_type_prev;
    LONGEST elt_off_prev = m_elt_off_prev;
    bool repeated = (m_options->repeat_count_threshold < UINT_MAX
		     && elt_type_prev != nullptr
		     && (m_elts + ((m_nrepeats + 1)
				   * m_stats[dim_indx + 1].nelts)
			 <= m_options->print_max)
		     && dimension_contents_eq (m_val, elt_type,
					       elt_off_prev, elt_off));

    if (repeated)
      m_nrepeats++;
    if (!repeated || last_p)
      {
	LONGEST nrepeats = m_nrepeats;

	m_nrepeats = 0;
	if (nrepeats >= m_options->repeat_count_threshold)
	  {
	    annotate_elt_rep (nrepeats + 1);
	    gdb_printf (m_stream, "%p[<repeats %s times>%p]",
			metadata_style.style ().ptr (),
			plongest (nrepeats + 1),
			nullptr);
	    annotate_elt_rep_end ();
	    if (!repeated)
	      gdb_puts (" ", m_stream);
	    m_elts += nrepeats * m_stats[dim_indx + 1].nelts;
	  }
	else
	  for (LONGEST i = nrepeats; i > 0; i--)
	    {
	      maybe_print_array_index (m_stats[dim_indx].index_type,
				       index - nrepeats + repeated,
				       m_stream, m_options);
	      walk_1 (elt_type_prev, elt_off_prev, repeated && i == 1);
	    }

	if (!repeated)
	  {
	    /* We need to specially handle the case of hitting `print_max'
	       exactly as recursing would cause lone `(...)' to be printed.
	       And we need to print `...' by hand if the skipped element
	       would be the last one processed, because the subsequent call
	       to `continue_walking' from our caller won't do that.  */
	    if (m_elts < m_options->print_max)
	      {
		maybe_print_array_index (m_stats[dim_indx].index_type, index,
					 m_stream, m_options);
		walk_1 (elt_type, elt_off, last_p);
		nrepeats++;
	      }
	    else if (last_p)
	      gdb_puts ("...", m_stream);
	  }
      }

    m_elt_type_prev = elt_type;
    m_elt_off_prev = elt_off;
  }

  /* Called to process an element of ELT_TYPE at offset ELT_OFF from the
     start of the parent object, where INDEX is the value of the element's
     index in the dimension currently being walked and LAST_P is true only
     when this is the last element to be processed in this dimension.  */
  void process_element (struct type *elt_type, LONGEST elt_off,
			LONGEST index, bool last_p)
  {
    size_t dim_indx = m_dimension - 1;
    struct type *elt_type_prev = m_elt_type_prev;
    LONGEST elt_off_prev = m_elt_off_prev;
    bool repeated = false;

    if (m_options->repeat_count_threshold < UINT_MAX
	&& elt_type_prev != nullptr)
      {
	/* When printing large arrays this spot is called frequently, so clean
	   up temporary values asap to prevent allocating a large amount of
	   them.  */
	scoped_value_mark free_values;
	struct value *e_val = value_from_component (m_val, elt_type, elt_off);
	struct value *e_prev = value_from_component (m_val, elt_type,
						     elt_off_prev);
	repeated = ((e_prev->entirely_available ()
		     && e_val->entirely_available ()
		     && e_prev->contents_eq (e_val))
		    || (e_prev->entirely_unavailable ()
			&& e_val->entirely_unavailable ()));
      }

    if (repeated)
      m_nrepeats++;
    if (!repeated || last_p || m_elts + 1 == m_options->print_max)
      {
	LONGEST nrepeats = m_nrepeats;
	bool printed = false;

	if (nrepeats != 0)
	  {
	    m_nrepeats = 0;
	    if (nrepeats >= m_options->repeat_count_threshold)
	      {
		annotate_elt_rep (nrepeats + 1);
		gdb_printf (m_stream, "%p[<repeats %s times>%p]",
			    metadata_style.style ().ptr (),
			    plongest (nrepeats + 1),
			    nullptr);
		annotate_elt_rep_end ();
	      }
	    else
	      {
		/* Extract the element value from the parent value.  */
		struct value *e_val
		  = value_from_component (m_val, elt_type, elt_off_prev);

		for (LONGEST i = nrepeats; i > 0; i--)
		  {
		    maybe_print_array_index (m_stats[dim_indx].index_type,
					     index - i + 1,
					     m_stream, m_options);
		    common_val_print (e_val, m_stream, m_recurse, m_options,
				      current_language);
		    if (i > 1)
		      gdb_puts (", ", m_stream);
		  }
	      }
	    printed = true;
	  }

	if (!repeated)
	  {
	    /* Extract the element value from the parent value.  */
	    struct value *e_val
	      = value_from_component (m_val, elt_type, elt_off);

	    if (printed)
	      gdb_puts (", ", m_stream);
	    maybe_print_array_index (m_stats[dim_indx].index_type, index,
				     m_stream, m_options);
	    common_val_print (e_val, m_stream, m_recurse, m_options,
			      current_language);
	  }
	if (!last_p)
	  gdb_puts (", ", m_stream);
      }

    m_elt_type_prev = elt_type;
    m_elt_off_prev = elt_off;
    ++m_elts;
  }

private:
  /* Called to compare two VAL elements of ELT_TYPE at offsets OFFSET1
     and OFFSET2 each.  Handle subarrays recursively, because they may
     have been sliced and we do not want to compare any memory contents
     present between the slices requested.  */
  bool
  dimension_contents_eq (struct value *val, struct type *type,
			 LONGEST offset1, LONGEST offset2)
  {
    if (type->code () == TYPE_CODE_ARRAY
	&& type->target_type ()->code () != TYPE_CODE_CHAR)
      {
	/* Extract the range, and get lower and upper bounds.  */
	struct type *range_type = check_typedef (type)->index_type ();
	LONGEST lowerbound, upperbound;
	if (!get_discrete_bounds (range_type, &lowerbound, &upperbound))
	  error ("failed to get range bounds");

	/* CALC is used to calculate the offsets for each element.  */
	fortran_array_offset_calculator calc (type);

	struct type *subarray_type = check_typedef (type->target_type ());
	for (LONGEST i = lowerbound; i < upperbound + 1; i++)
	  {
	    /* Use the index and the stride to work out a new offset.  */
	    LONGEST index_offset = calc.index_offset (i);

	    if (!dimension_contents_eq (val, subarray_type,
					offset1 + index_offset,
					offset2 + index_offset))
	      return false;
	  }
	return true;
      }
    else
      {
	struct value *e_val1 = value_from_component (val, type, offset1);
	struct value *e_val2 = value_from_component (val, type, offset2);

	return ((e_val1->entirely_available ()
		 && e_val2->entirely_available ()
		 && e_val1->contents_eq (e_val2))
		|| (e_val1->entirely_unavailable ()
		    && e_val2->entirely_unavailable ()));
      }
  }

  /* The number of elements printed so far.  */
  int m_elts;

  /* The value from which we are printing elements.  */
  struct value *m_val;

  /* The stream we should print too.  */
  struct ui_file *m_stream;

  /* The recursion counter, passed through when we print each element.  */
  int m_recurse;

  /* The print control options.  Gives us the maximum number of elements to
     print, and is passed through to each element that we print.  */
  const struct value_print_options *m_options = nullptr;

  /* The number of the current dimension being handled.  */
  LONGEST m_dimension;

  /* The number of element repetitions in the current series.  */
  LONGEST m_nrepeats;

  /* The type and offset from M_VAL of the element handled in the previous
     iteration over the current dimension.  */
  struct type *m_elt_type_prev;
  LONGEST m_elt_off_prev;

  /* Per-dimension stats.  */
  std::vector<struct dimension_stats> m_stats;
};

/* This function gets called to print a Fortran array.  */

static void
fortran_print_array (struct type *type, CORE_ADDR address,
		     struct ui_file *stream, int recurse,
		     const struct value *val,
		     const struct value_print_options *options)
{
  fortran_array_walker<fortran_array_printer_impl> p
    (type, address, (struct value *) val, stream, recurse, options);
  p.walk ();
}


/* Decorations for Fortran.  */

static const struct generic_val_print_decorations f_decorations =
{
  "(",
  ",",
  ")",
  ".TRUE.",
  ".FALSE.",
  "void",
  "{",
  "}"
};

/* See f-lang.h.  */

void
f_language::value_print_inner (struct value *val, struct ui_file *stream,
			       int recurse,
			       const struct value_print_options *options) const
{
  struct type *type = check_typedef (val->type ());
  struct gdbarch *gdbarch = type->arch ();
  int printed_field = 0; /* Number of fields printed.  */
  struct type *elttype;
  CORE_ADDR addr;
  int index;
  const gdb_byte *valaddr = val->contents_for_printing ().data ();
  const CORE_ADDR address = val->address ();

  switch (type->code ())
    {
    case TYPE_CODE_STRING:
      f77_get_dynamic_length_of_aggregate (type);
      printstr (stream, builtin_type (gdbarch)->builtin_char, valaddr,
		type->length (), NULL, 0, options);
      break;

    case TYPE_CODE_ARRAY:
      if (type->target_type ()->code () != TYPE_CODE_CHAR)
	fortran_print_array (type, address, stream, recurse, val, options);
      else
	{
	  struct type *ch_type = type->target_type ();

	  f77_get_dynamic_length_of_aggregate (type);
	  printstr (stream, ch_type, valaddr,
		    type->length () / ch_type->length (), NULL, 0,
		    options);
	}
      break;

    case TYPE_CODE_PTR:
      if (options->format && options->format != 's')
	{
	  value_print_scalar_formatted (val, options, 0, stream);
	  break;
	}
      else
	{
	  int want_space = 0;

	  addr = unpack_pointer (type, valaddr);
	  elttype = check_typedef (type->target_type ());

	  if (elttype->code () == TYPE_CODE_FUNC)
	    {
	      /* Try to print what function it points to.  */
	      print_function_pointer_address (options, gdbarch, addr, stream);
	      return;
	    }

	  if (options->symbol_print)
	    want_space = print_address_demangle (options, gdbarch, addr,
						 stream, demangle);
	  else if (options->addressprint && options->format != 's')
	    {
	      gdb_puts (paddress (gdbarch, addr), stream);
	      want_space = 1;
	    }

	  /* For a pointer to char or unsigned char, also print the string
	     pointed to, unless pointer is null.  */
	  if (elttype->length () == 1
	      && elttype->code () == TYPE_CODE_INT
	      && (options->format == 0 || options->format == 's')
	      && addr != 0)
	    {
	      if (want_space)
		gdb_puts (" ", stream);
	      val_print_string (type->target_type (), NULL, addr, -1,
				stream, options);
	    }
	  return;
	}
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_NAMELIST:
      /* Starting from the Fortran 90 standard, Fortran supports derived
	 types.  */
      gdb_printf (stream, "( ");
      for (index = 0; index < type->num_fields (); index++)
	{
	  struct type *field_type
	    = check_typedef (type->field (index).type ());

	  if (field_type->code () != TYPE_CODE_FUNC)
	    {
	      const char *field_name = type->field (index).name ();
	      struct value *field;

	      if (type->code () == TYPE_CODE_NAMELIST)
		{
		  /* While printing namelist items, fetch the appropriate
		     value field before printing its value.  */
		  struct block_symbol sym
		    = lookup_symbol (field_name, get_selected_block (nullptr),
				     VAR_DOMAIN, nullptr);
		  if (sym.symbol == nullptr)
		    error (_("failed to find symbol for name list component %s"),
			   field_name);
		  field = value_of_variable (sym.symbol, sym.block);
		}
	      else
		field = value_field (val, index);

	      if (printed_field > 0)
		gdb_puts (", ", stream);

	      if (field_name != NULL)
		{
		  fputs_styled (field_name, variable_name_style.style (),
				stream);
		  gdb_puts (" = ", stream);
		}

	      common_val_print (field, stream, recurse + 1,
				options, current_language);

	      ++printed_field;
	    }
	 }
      gdb_printf (stream, " )");
      break;     

    case TYPE_CODE_BOOL:
      if (options->format || options->output_format)
	{
	  struct value_print_options opts = *options;
	  opts.format = (options->format ? options->format
			 : options->output_format);
	  value_print_scalar_formatted (val, &opts, 0, stream);
	}
      else
	{
	  LONGEST longval = value_as_long (val);
	  /* The Fortran standard doesn't specify how logical types are
	     represented.  Different compilers use different non zero
	     values to represent logical true.  */
	  if (longval == 0)
	    gdb_puts (f_decorations.false_name, stream);
	  else
	    gdb_puts (f_decorations.true_name, stream);
	}
      break;

    case TYPE_CODE_INT:
    case TYPE_CODE_REF:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_CHAR:
    default:
      generic_value_print (val, stream, recurse, options, &f_decorations);
      break;
    }
}

static void
info_common_command_for_block (const struct block *block, const char *comname,
			       int *any_printed)
{
  struct value_print_options opts;

  get_user_print_options (&opts);

  for (struct symbol *sym : block_iterator_range (block))
    if (sym->domain () == COMMON_BLOCK_DOMAIN)
      {
	const struct common_block *common = sym->value_common_block ();
	size_t index;

	gdb_assert (sym->aclass () == LOC_COMMON_BLOCK);

	if (comname && (!sym->linkage_name ()
			|| strcmp (comname, sym->linkage_name ()) != 0))
	  continue;

	if (*any_printed)
	  gdb_putc ('\n');
	else
	  *any_printed = 1;
	if (sym->print_name ())
	  gdb_printf (_("Contents of F77 COMMON block '%s':\n"),
		      sym->print_name ());
	else
	  gdb_printf (_("Contents of blank COMMON block:\n"));
	
	for (index = 0; index < common->n_entries; index++)
	  {
	    struct value *val = NULL;

	    gdb_printf ("%s = ",
			common->contents[index]->print_name ());

	    try
	      {
		val = value_of_variable (common->contents[index], block);
		value_print (val, gdb_stdout, &opts);
	      }

	    catch (const gdb_exception_error &except)
	      {
		fprintf_styled (gdb_stdout, metadata_style.style (),
				"<error reading variable: %s>",
				except.what ());
	      }

	    gdb_putc ('\n');
	  }
      }
}

/* This function is used to print out the values in a given COMMON 
   block.  It will always use the most local common block of the 
   given name.  */

static void
info_common_command (const char *comname, int from_tty)
{
  frame_info_ptr fi;
  const struct block *block;
  int values_printed = 0;

  /* We have been told to display the contents of F77 COMMON 
     block supposedly visible in this function.  Let us 
     first make sure that it is visible and if so, let 
     us display its contents.  */

  fi = get_selected_frame (_("No frame selected"));

  /* The following is generally ripped off from stack.c's routine 
     print_frame_info().  */

  block = get_frame_block (fi, 0);
  if (block == NULL)
    {
      gdb_printf (_("No symbol table info available.\n"));
      return;
    }

  while (block)
    {
      info_common_command_for_block (block, comname, &values_printed);
      /* After handling the function's top-level block, stop.  Don't
	 continue to its superblock, the block of per-file symbols.  */
      if (block->function ())
	break;
      block = block->superblock ();
    }

  if (!values_printed)
    {
      if (comname)
	gdb_printf (_("No common block '%s'.\n"), comname);
      else
	gdb_printf (_("No common blocks.\n"));
    }
}

void _initialize_f_valprint ();
void
_initialize_f_valprint ()
{
  add_info ("common", info_common_command,
	    _("Print out the values contained in a Fortran COMMON block."));
}
