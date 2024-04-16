/* Type stack for GDB parser.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef TYPE_STACK_H
#define TYPE_STACK_H

#include "gdbtypes.h"
#include <vector>

struct type;
struct expr_builder;

/* For parsing of complicated types.
   An array should be preceded in the list by the size of the array.  */
enum type_pieces
  {
    tp_end = -1, 
    tp_pointer, 
    tp_reference, 
    tp_rvalue_reference,
    tp_array, 
    tp_function,
    tp_function_with_arguments,
    tp_const, 
    tp_volatile, 
    tp_space_identifier,
    tp_atomic,
    tp_restrict,
    tp_type_stack,
    tp_kind
  };

/* The stack can contain either an enum type_pieces or an int.  */
union type_stack_elt
  {
    enum type_pieces piece;
    int int_val;
    struct type_stack *stack_val;
    std::vector<struct type *> *typelist_val;
  };

/* The type stack is an instance of this structure.  */

struct type_stack
{
public:

  type_stack () = default;

  DISABLE_COPY_AND_ASSIGN (type_stack);

  type_stack *create ()
  {
    type_stack *result = new type_stack ();
    result->m_elements = std::move (m_elements);
    return result;
  }

  /* Insert a new type, TP, at the bottom of the type stack.  If TP is
     tp_pointer, tp_reference or tp_rvalue_reference, it is inserted at the
     bottom.  If TP is a qualifier, it is inserted at slot 1 (just above a
     previous tp_pointer) if there is anything on the stack, or simply pushed
     if the stack is empty.  Other values for TP are invalid.  */

  void insert (enum type_pieces tp);

  void push (enum type_pieces tp)
  {
    type_stack_elt elt;
    elt.piece = tp;
    m_elements.push_back (elt);
  }

  void push (int n)
  {
    type_stack_elt elt;
    elt.int_val = n;
    m_elements.push_back (elt);
  }

  /* Push the type stack STACK as an element on this type stack.  */

  void push (struct type_stack *stack)
  {
    type_stack_elt elt;
    elt.stack_val = stack;
    m_elements.push_back (elt);
    push (tp_type_stack);
  }

  /* Push a function type with arguments onto the global type stack.
     LIST holds the argument types.  If the final item in LIST is NULL,
     then the function will be varargs.  */

  void push (std::vector<struct type *> *list)
  {
    type_stack_elt elt;
    elt.typelist_val = list;
    m_elements.push_back (elt);
    push (tp_function_with_arguments);
  }

  enum type_pieces pop ()
  {
    if (m_elements.empty ())
      return tp_end;
    type_stack_elt elt = m_elements.back ();
    m_elements.pop_back ();
    return elt.piece;
  }

  int pop_int ()
  {
    if (m_elements.empty ())
      {
	/* "Can't happen".  */
	return 0;
      }
    type_stack_elt elt = m_elements.back ();
    m_elements.pop_back ();
    return elt.int_val;
  }

  std::vector<struct type *> *pop_typelist ()
  {
    gdb_assert (!m_elements.empty ());
    type_stack_elt elt = m_elements.back ();
    m_elements.pop_back ();
    return elt.typelist_val;
  }

  /* Pop a type_stack element.  */

  struct type_stack *pop_type_stack ()
  {
    gdb_assert (!m_elements.empty ());
    type_stack_elt elt = m_elements.back ();
    m_elements.pop_back ();
    return elt.stack_val;
  }

  /* Insert a tp_space_identifier and the corresponding address space
     value into the stack.  STRING is the name of an address space, as
     recognized by address_space_name_to_type_instance_flags.  If the
     stack is empty, the new elements are simply pushed.  If the stack
     is not empty, this function assumes that the first item on the
     stack is a tp_pointer, and the new values are inserted above the
     first item.  */

  void insert (struct expr_builder *pstate, const char *string);

  /* Append the elements of the type stack FROM to the type stack
     THIS.  Always returns THIS.  */

  struct type_stack *append (struct type_stack *from)
  {
    m_elements.insert (m_elements.end (), from->m_elements.begin (),
		       from->m_elements.end ());
    return this;
  }

  /* Pop the type stack and return a type_instance_flags that
     corresponds the const/volatile qualifiers on the stack.  This is
     called by the C++ parser when parsing methods types, and as such no
     other kind of type in the type stack is expected.  */

  type_instance_flags follow_type_instance_flags ();

  /* Pop the type stack and return the type which corresponds to
     FOLLOW_TYPE as modified by all the stuff on the stack.  */
  struct type *follow_types (struct type *follow_type);

private:

  /* A helper function for insert_type and insert_type_address_space.
     This does work of expanding the type stack and inserting the new
     element, ELEMENT, into the stack at location SLOT.  */

  void insert_into (int slot, union type_stack_elt element)
  {
    gdb_assert (slot <= m_elements.size ());
    m_elements.insert (m_elements.begin () + slot, element);
  }


  /* Elements on the stack.  */
  std::vector<union type_stack_elt> m_elements;
};

#endif /* TYPE_STACK_H */
