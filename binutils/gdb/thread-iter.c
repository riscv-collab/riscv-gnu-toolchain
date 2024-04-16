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

#include "defs.h"
#include "gdbthread.h"
#include "inferior.h"

/* See thread-iter.h.  */

all_threads_iterator::all_threads_iterator (begin_t)
{
  /* Advance M_INF/M_THR to the first thread's position.  */

  for (inferior &inf : inferior_list)
    {
      auto thr_iter = inf.thread_list.begin ();
      if (thr_iter != inf.thread_list.end ())
	{
	  m_inf = &inf;
	  m_thr = &*thr_iter;
	  return;
	}
    }
  m_inf = nullptr;
  m_thr = nullptr;
}

/* See thread-iter.h.  */

void
all_threads_iterator::advance ()
{
  intrusive_list<inferior>::iterator inf_iter (m_inf);
  intrusive_list<thread_info>::iterator thr_iter (m_thr);

  /* The loop below is written in the natural way as-if we'd always
     start at the beginning of the inferior list.  This fast forwards
     the algorithm to the actual current position.  */
  goto start;

  for (; inf_iter != inferior_list.end (); ++inf_iter)
    {
      m_inf = &*inf_iter;
      thr_iter = m_inf->thread_list.begin ();
      while (thr_iter != m_inf->thread_list.end ())
	{
	  m_thr = &*thr_iter;
	  return;
	start:
	  ++thr_iter;
	}
    }

  m_thr = nullptr;
}

/* See thread-iter.h.  */

bool
all_matching_threads_iterator::m_inf_matches ()
{
  return (m_filter_target == nullptr
	  || m_filter_target == m_inf->process_target ());
}

/* See thread-iter.h.  */

all_matching_threads_iterator::all_matching_threads_iterator
  (process_stratum_target *filter_target, ptid_t filter_ptid)
  : m_filter_target (filter_target)
{
  if (filter_ptid == minus_one_ptid)
    {
      /* Iterate on all threads of all inferiors, possibly filtering on
	 FILTER_TARGET.  */
      m_mode = mode::ALL_THREADS;

      /* Seek the first thread of the first matching inferior.  */
      for (inferior &inf : inferior_list)
	{
	  m_inf = &inf;

	  if (!m_inf_matches ()
	      || inf.thread_list.empty ())
	    continue;

	  m_thr = &inf.thread_list.front ();
	  return;
	}
    }
  else
    {
      gdb_assert (filter_target != nullptr);

      if (filter_ptid.is_pid ())
	{
	  /* Iterate on all threads of the given inferior.  */
	  m_mode = mode::ALL_THREADS_OF_INFERIOR;

	  m_inf = find_inferior_pid (filter_target, filter_ptid.pid ());
	  if (m_inf != nullptr)
	    m_thr = &m_inf->thread_list.front ();
	}
      else
	{
	  /* Iterate on a single thread.  */
	  m_mode = mode::SINGLE_THREAD;

	  m_thr = filter_target->find_thread (filter_ptid);
	}
    }
}

/* See thread-iter.h.  */

void
all_matching_threads_iterator::advance ()
{
  switch (m_mode)
    {
    case mode::ALL_THREADS:
      {
	intrusive_list<inferior>::iterator inf_iter (m_inf);
	intrusive_list<thread_info>::iterator thr_iter
	  = m_inf->thread_list.iterator_to (*m_thr);

	/* The loop below is written in the natural way as-if we'd always
	   start at the beginning of the inferior list.  This fast forwards
	   the algorithm to the actual current position.  */
	goto start;

	for (; inf_iter != inferior_list.end (); ++inf_iter)
	  {
	    m_inf = &*inf_iter;

	    if (!m_inf_matches ())
	      continue;

	    thr_iter = m_inf->thread_list.begin ();
	    while (thr_iter != m_inf->thread_list.end ())
	      {
		m_thr = &*thr_iter;
		return;

	      start:
		++thr_iter;
	      }
	  }
      }
      m_thr = nullptr;
      break;

    case mode::ALL_THREADS_OF_INFERIOR:
      {
	intrusive_list<thread_info>::iterator thr_iter
	  = m_inf->thread_list.iterator_to (*m_thr);
	++thr_iter;
	if (thr_iter != m_inf->thread_list.end ())
	  m_thr = &*thr_iter;
	else
	  m_thr = nullptr;
	break;
      }

    case mode::SINGLE_THREAD:
      m_thr = nullptr;
      break;

    default:
      gdb_assert_not_reached ("invalid mode value");
    }
}
