/* Inferior iterators and ranges for GDB, the GNU debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef INFERIOR_ITER_H
#define INFERIOR_ITER_H

#include "gdbsupport/filtered-iterator.h"
#include "gdbsupport/safe-iterator.h"

/* A forward iterator that iterates over all inferiors.  */

class all_inferiors_iterator
{
public:
  typedef all_inferiors_iterator self_type;
  typedef struct inferior *value_type;
  typedef struct inferior *&reference;
  typedef struct inferior **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  /* Create an iterator pointing at HEAD.  */
  all_inferiors_iterator (process_stratum_target *proc_target,
			  const intrusive_list<inferior> &list)
    : m_proc_target (proc_target), m_inf_iter (list.begin ())
  {
    intrusive_list<inferior>::iterator end;

    /* Advance M_INF to the first inferior's position.  */
    for (; m_inf_iter != end; ++m_inf_iter)
      if (m_inf_matches ())
	return;
  }

  /* Create a one-past-end iterator.  */
  all_inferiors_iterator ()
    : m_proc_target (nullptr)
  {}

  all_inferiors_iterator &operator++ ()
  {
    advance ();
    return *this;
  }

  inferior *operator* () const
  { return &*m_inf_iter; }

  bool operator!= (const all_inferiors_iterator &other) const
  { return m_inf_iter != other.m_inf_iter; }

private:
  /* Advance to next inferior, skipping filtered inferiors.  */
  void advance ()
  {
    intrusive_list<inferior>::iterator end;

    /* The loop below is written in the natural way as-if we'd always
       start at the beginning of the inferior list.  This
       fast-forwards the algorithm to the actual current position.  */
    goto start;

    while (m_inf_iter != end)
      {
	if (m_inf_matches ())
	  return;
      start:
	++m_inf_iter;
      }
  }

  bool m_inf_matches ()
  {
    return (m_proc_target == nullptr
	    || m_proc_target == m_inf_iter->process_target ());
  }

  process_stratum_target *m_proc_target;
  intrusive_list<inferior>::iterator m_inf_iter;
};

/* A range adapter that makes it possible to iterate over all
   inferiors with range-for.  */

using all_inferiors_range = iterator_range<all_inferiors_iterator>;

/* Filter for filtered_iterator.  Filters out exited inferiors.  */

struct exited_inferior_filter
{
  bool operator() (inferior *inf)
  {
    return inf->pid != 0;
  }
};

/* Iterate over all non-exited inferiors.  */

using all_non_exited_inferiors_iterator
  = filtered_iterator<all_inferiors_iterator, exited_inferior_filter>;

/* A range adapter that makes it possible to iterate over all
   non-exited inferiors with range-for.  */

using all_non_exited_inferiors_range
  = iterator_range<all_non_exited_inferiors_iterator>;

/* Iterate over all inferiors, safely.  */

using all_inferiors_safe_iterator
  = basic_safe_iterator<all_inferiors_iterator>;

/* A range adapter that makes it possible to iterate over all
   inferiors with range-for "safely".  I.e., it is safe to delete the
   currently-iterated inferior.  */

using all_inferiors_safe_range = iterator_range<all_inferiors_safe_iterator>;

#endif /* !defined (INFERIOR_ITER_H) */
