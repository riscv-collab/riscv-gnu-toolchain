/* Thread iterators and ranges for GDB, the GNU debugger.
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

#ifndef THREAD_ITER_H
#define THREAD_ITER_H

#include "gdbsupport/filtered-iterator.h"
#include "gdbsupport/iterator-range.h"
#include "gdbsupport/next-iterator.h"
#include "gdbsupport/reference-to-pointer-iterator.h"
#include "gdbsupport/safe-iterator.h"

/* A forward iterator that iterates over a given inferior's
   threads.  */

using inf_threads_iterator
  = reference_to_pointer_iterator<intrusive_list<thread_info>::iterator>;

/* A forward iterator that iterates over all threads of all
   inferiors.  */

class all_threads_iterator
{
public:
  typedef all_threads_iterator self_type;
  typedef struct thread_info *value_type;
  typedef struct thread_info *&reference;
  typedef struct thread_info **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  /* Tag type.  */
  struct begin_t {};

  /* Create an iterator that points to the first thread of the first
     inferior.  */
  explicit all_threads_iterator (begin_t);

  /* Create a one-past-end iterator.  */
  all_threads_iterator ()
    : m_thr (nullptr)
  {}

  thread_info *operator* () const { return m_thr; }

  all_threads_iterator &operator++ ()
  {
    advance ();
    return *this;
  }

  bool operator== (const all_threads_iterator &other) const
  { return m_thr == other.m_thr; }

  bool operator!= (const all_threads_iterator &other) const
  { return m_thr != other.m_thr; }

private:
  /* Advance to the next thread.  */
  void advance ();

private:
  /* The current inferior and thread.  M_THR is NULL if we reached the
     end of the threads list of the last inferior.  */
  inferior *m_inf;
  thread_info *m_thr;
};

/* Iterate over all threads that match a given PTID.  */

class all_matching_threads_iterator
{
public:
  typedef all_matching_threads_iterator self_type;
  typedef struct thread_info *value_type;
  typedef struct thread_info *&reference;
  typedef struct thread_info **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  /* Creates an iterator that iterates over all threads that match
     FILTER_PTID.  */
  all_matching_threads_iterator (process_stratum_target *filter_target,
				 ptid_t filter_ptid);

  /* Create a one-past-end iterator.  */
  all_matching_threads_iterator () = default;

  thread_info *operator* () const { return m_thr; }

  all_matching_threads_iterator &operator++ ()
  {
    advance ();
    return *this;
  }

  bool operator== (const all_matching_threads_iterator &other) const
  { return m_thr == other.m_thr; }

  bool operator!= (const all_matching_threads_iterator &other) const
  { return m_thr != other.m_thr; }

private:
  /* Advance to next thread, skipping filtered threads.  */
  void advance ();

  /* True if M_INF has the process target M_FILTER_TARGET.  */
  bool m_inf_matches ();

private:
  enum class mode
  {
    /* All threads, possibly filtered down to a single target.  */
    ALL_THREADS,

    /* All threads of the given inferior.  */
    ALL_THREADS_OF_INFERIOR,

    /* A specific thread.  */
    SINGLE_THREAD,
  } m_mode;

  /* The current inferior.  */
  inferior *m_inf = nullptr;

  /* The current thread.  */
  thread_info *m_thr = nullptr;

  /* The target we filter on (may be nullptr).  */
  process_stratum_target *m_filter_target;
};

/* Filter for filtered_iterator.  Filters out exited threads.  */

struct non_exited_thread_filter
{
  bool operator() (struct thread_info *thr) const
  {
    return thr->state != THREAD_EXITED;
  }
};

/* Iterate over all non-exited threads that match a given PTID.  */

using all_non_exited_threads_iterator
  = filtered_iterator<all_matching_threads_iterator, non_exited_thread_filter>;

/* Iterate over all non-exited threads of an inferior.  */

using inf_non_exited_threads_iterator
  = filtered_iterator<inf_threads_iterator, non_exited_thread_filter>;

/* Iterate over all threads of all inferiors, safely.  */

using all_threads_safe_iterator
  = basic_safe_iterator<all_threads_iterator>;

/* Iterate over all threads of an inferior, safely.  */

using safe_inf_threads_iterator
  = basic_safe_iterator<inf_threads_iterator>;

/* A range adapter that makes it possible to iterate over all threads
   of an inferior with range-for.  */

using inf_threads_range = iterator_range<inf_threads_iterator>;

/* A range adapter that makes it possible to iterate over all
   non-exited threads of an inferior with range-for.  */

using inf_non_exited_threads_range
  = iterator_range<inf_non_exited_threads_iterator>;

/* A range adapter that makes it possible to iterate over all threads
   of an inferior with range-for, safely.  */

using safe_inf_threads_range = iterator_range<safe_inf_threads_iterator>;

/* A range adapter that makes it possible to iterate over all threads
   with range-for "safely".  I.e., it is safe to delete the
   currently-iterated thread.  */

using all_threads_safe_range = iterator_range<all_threads_safe_iterator>;

/* A range adapter that makes it possible to iterate over all threads
   that match a PTID filter with range-for.  */

struct all_matching_threads_range
{
public:
  all_matching_threads_range (process_stratum_target *filter_target,
			      ptid_t filter_ptid)
    : m_filter_target (filter_target), m_filter_ptid (filter_ptid)
  {}
  all_matching_threads_range ()
    : m_filter_target (nullptr), m_filter_ptid (minus_one_ptid)
  {}

  all_matching_threads_iterator begin () const
  { return all_matching_threads_iterator (m_filter_target, m_filter_ptid); }
  all_matching_threads_iterator end () const
  { return all_matching_threads_iterator (); }

private:
  /* The filter.  */
  process_stratum_target *m_filter_target;
  ptid_t m_filter_ptid;
};

/* A range adapter that makes it possible to iterate over all
   non-exited threads of all inferiors, with range-for.
   Threads/inferiors that do not match FILTER_PTID are filtered
   out.  */

class all_non_exited_threads_range
{
public:
  all_non_exited_threads_range (process_stratum_target *filter_target,
				ptid_t filter_ptid)
    : m_filter_target (filter_target), m_filter_ptid (filter_ptid)
  {}

  all_non_exited_threads_range ()
    : m_filter_target (nullptr), m_filter_ptid (minus_one_ptid)
  {}

  all_non_exited_threads_iterator begin () const
  { return all_non_exited_threads_iterator (m_filter_target, m_filter_ptid); }
  all_non_exited_threads_iterator end () const
  { return all_non_exited_threads_iterator (); }

private:
  process_stratum_target *m_filter_target;
  ptid_t m_filter_ptid;
};

#endif /* THREAD_ITER_H */
