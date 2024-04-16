/* Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

/* Support classes to wrap up the process of iterating over a
   multi-dimensional Fortran array.  */

#ifndef F_ARRAY_WALKER_H
#define F_ARRAY_WALKER_H

#include "defs.h"
#include "gdbtypes.h"
#include "f-lang.h"

/* Class for calculating the byte offset for elements within a single
   dimension of a Fortran array.  */
class fortran_array_offset_calculator
{
public:
  /* Create a new offset calculator for TYPE, which is either an array or a
     string.  */
  explicit fortran_array_offset_calculator (struct type *type)
  {
    /* Validate the type.  */
    type = check_typedef (type);
    if (type->code () != TYPE_CODE_ARRAY
	&& (type->code () != TYPE_CODE_STRING))
      error (_("can only compute offsets for arrays and strings"));

    /* Get the range, and extract the bounds.  */
    struct type *range_type = type->index_type ();
    if (!get_discrete_bounds (range_type, &m_lowerbound, &m_upperbound))
      error ("unable to read array bounds");

    /* Figure out the stride for this array.  */
    struct type *elt_type = check_typedef (type->target_type ());
    m_stride = type->index_type ()->bounds ()->bit_stride ();
    if (m_stride == 0)
      m_stride = type_length_units (elt_type);
    else
      {
	int unit_size
	  = gdbarch_addressable_memory_unit_size (elt_type->arch ());
	m_stride /= (unit_size * 8);
      }
  };

  /* Get the byte offset for element INDEX within the type we are working
     on.  There is no bounds checking done on INDEX.  If the stride is
     negative then we still assume that the base address (for the array
     object) points to the element with the lowest memory address, we then
     calculate an offset assuming that index 0 will be the element at the
     highest address, index 1 the next highest, and so on.  This is not
     quite how Fortran works in reality; in reality the base address of
     the object would point at the element with the highest address, and
     we would index backwards from there in the "normal" way, however,
     GDB's current value contents model doesn't support having the base
     address be near to the end of the value contents, so we currently
     adjust the base address of Fortran arrays with negative strides so
     their base address points at the lowest memory address.  This code
     here is part of working around this weirdness.  */
  LONGEST index_offset (LONGEST index)
  {
    LONGEST offset;
    if (m_stride < 0)
      offset = std::abs (m_stride) * (m_upperbound - index);
    else
      offset = std::abs (m_stride) * (index - m_lowerbound);
    return offset;
  }

private:

  /* The stride for the type we are working with.  */
  LONGEST m_stride;

  /* The upper bound for the type we are working with.  */
  LONGEST m_upperbound;

  /* The lower bound for the type we are working with.  */
  LONGEST m_lowerbound;
};

/* A base class used by fortran_array_walker.  There's no virtual methods
   here, sub-classes should just override the functions they want in order
   to specialise the behaviour to their needs.  The functionality
   provided in these default implementations will visit every array
   element, but do nothing for each element.  */

struct fortran_array_walker_base_impl
{
  /* Called when iterating between the lower and upper bounds of each
     dimension of the array.  Return true if GDB should continue iterating,
     otherwise, return false.

     SHOULD_CONTINUE indicates if GDB is going to stop anyway, and should
     be taken into consideration when deciding what to return.  If
     SHOULD_CONTINUE is false then this function must also return false,
     the function is still called though in case extra work needs to be
     done as part of the stopping process.  */
  bool continue_walking (bool should_continue)
  { return should_continue; }

  /* Called when GDB starts iterating over a dimension of the array.  The
     argument INDEX_TYPE is the type of the index used to address elements
     in the dimension, NELTS holds the number of the elements there, and
     INNER_P is true for the inner most dimension (the dimension containing
     the actual elements of the array), and false for more outer dimensions.
     For a concrete example of how this function is called see the comment
     on process_element below.  */
  void start_dimension (struct type *index_type, LONGEST nelts, bool inner_p)
  { /* Nothing.  */ }

  /* Called when GDB finishes iterating over a dimension of the array.  The
     argument INNER_P is true for the inner most dimension (the dimension
     containing the actual elements of the array), and false for more outer
     dimensions.  LAST_P is true for the last call at a particular
     dimension.  For a concrete example of how this function is called
     see the comment on process_element below.  */
  void finish_dimension (bool inner_p, bool last_p)
  { /* Nothing.  */ }

  /* Called when processing dimensions of the array other than the
     innermost one.  WALK_1 is the walker to normally call, ELT_TYPE is
     the type of the element being extracted, and ELT_OFF is the offset
     of the element from the start of array being walked.  INDEX is the
     value of the index the current element is at in the upper dimension.
     Finally LAST_P is true only when this is the last element that will
     be processed in this dimension.  */
  void process_dimension (gdb::function_view<void (struct type *,
						   int, bool)> walk_1,
			  struct type *elt_type, LONGEST elt_off,
			  LONGEST index, bool last_p)
  {
    walk_1 (elt_type, elt_off, last_p);
  }

  /* Called when processing the inner most dimension of the array, for
     every element in the array.  ELT_TYPE is the type of the element being
     extracted, and ELT_OFF is the offset of the element from the start of
     array being walked.  INDEX is the value of the index the current
     element is at in the upper dimension.  Finally LAST_P is true only
     when this is the last element that will be processed in this dimension.

     Given this two dimensional array ((1, 2) (3, 4) (5, 6)), the calls to
     start_dimension, process_element, and finish_dimension look like this:

     start_dimension (INDEX_TYPE, 3, false);
       start_dimension (INDEX_TYPE, 2, true);
	 process_element (TYPE, OFFSET, false);
	 process_element (TYPE, OFFSET, true);
       finish_dimension (true, false);
       start_dimension (INDEX_TYPE, 2, true);
	 process_element (TYPE, OFFSET, false);
	 process_element (TYPE, OFFSET, true);
       finish_dimension (true, true);
       start_dimension (INDEX_TYPE, 2, true);
	 process_element (TYPE, OFFSET, false);
	 process_element (TYPE, OFFSET, true);
       finish_dimension (true, true);
     finish_dimension (false, true);  */
  void process_element (struct type *elt_type, LONGEST elt_off,
			LONGEST index, bool last_p)
  { /* Nothing.  */ }
};

/* A class to wrap up the process of iterating over a multi-dimensional
   Fortran array.  IMPL is used to specialise what happens as we walk over
   the array.  See class FORTRAN_ARRAY_WALKER_BASE_IMPL (above) for the
   methods than can be used to customise the array walk.  */
template<typename Impl>
class fortran_array_walker
{
  /* Ensure that Impl is derived from the required base class.  This just
     ensures that all of the required API methods are available and have a
     sensible default implementation.  */
  static_assert ((std::is_base_of<fortran_array_walker_base_impl,Impl>::value));

public:
  /* Create a new array walker.  TYPE is the type of the array being walked
     over, and ADDRESS is the base address for the object of TYPE in
     memory.  All other arguments are forwarded to the constructor of the
     template parameter class IMPL.  */
  template <typename ...Args>
  fortran_array_walker (struct type *type, CORE_ADDR address,
			Args... args)
    : m_type (type),
      m_address (address),
      m_impl (type, address, args...),
      m_ndimensions (calc_f77_array_dims (m_type)),
      m_nss (0)
  { /* Nothing.  */ }

  /* Walk the array.  */
  void
  walk ()
  {
    walk_1 (m_type, 0, false);
  }

private:
  /* The core of the array walking algorithm.  TYPE is the type of
     the current dimension being processed and OFFSET is the offset
     (in bytes) for the start of this dimension.  */
  void
  walk_1 (struct type *type, int offset, bool last_p)
  {
    /* Extract the range, and get lower and upper bounds.  */
    struct type *range_type = check_typedef (type)->index_type ();
    LONGEST lowerbound, upperbound;
    if (!get_discrete_bounds (range_type, &lowerbound, &upperbound))
      error ("failed to get range bounds");

    /* CALC is used to calculate the offsets for each element in this
       dimension.  */
    fortran_array_offset_calculator calc (type);

    m_nss++;
    gdb_assert (range_type->code () == TYPE_CODE_RANGE);
    m_impl.start_dimension (range_type->target_type (),
			    upperbound - lowerbound + 1,
			    m_nss == m_ndimensions);

    if (m_nss != m_ndimensions)
      {
	struct type *subarray_type = check_typedef (type)->target_type ();

	/* For dimensions other than the inner most, walk each element and
	   recurse while peeling off one more dimension of the array.  */
	for (LONGEST i = lowerbound;
	     m_impl.continue_walking (i < upperbound + 1);
	     i++)
	  {
	    /* Use the index and the stride to work out a new offset.  */
	    LONGEST new_offset = offset + calc.index_offset (i);

	    /* Now print the lower dimension.  */
	    m_impl.process_dimension
	      ([this] (struct type *w_type, int w_offset, bool w_last_p) -> void
		{
		  this->walk_1 (w_type, w_offset, w_last_p);
		},
	       subarray_type, new_offset, i, i == upperbound);
	  }
      }
    else
      {
	struct type *elt_type = check_typedef (type)->target_type ();

	/* For the inner most dimension of the array, process each element
	   within this dimension.  */
	for (LONGEST i = lowerbound;
	     m_impl.continue_walking (i < upperbound + 1);
	     i++)
	  {
	    LONGEST elt_off = offset + calc.index_offset (i);

	    if (is_dynamic_type (elt_type))
	      {
		CORE_ADDR e_address = m_address + elt_off;
		elt_type = resolve_dynamic_type (elt_type, {}, e_address);
	      }

	    m_impl.process_element (elt_type, elt_off, i, i == upperbound);
	  }
      }

    m_impl.finish_dimension (m_nss == m_ndimensions, last_p || m_nss == 1);
    m_nss--;
  }

  /* The array type being processed.  */
  struct type *m_type;

  /* The address in target memory for the object of M_TYPE being
     processed.  This is required in order to resolve dynamic types.  */
  CORE_ADDR m_address;

  /* An instance of the template specialisation class.  */
  Impl m_impl;

  /* The total number of dimensions in M_TYPE.  */
  int m_ndimensions;

  /* The current dimension number being processed.  */
  int m_nss;
};

#endif /* F_ARRAY_WALKER_H */
