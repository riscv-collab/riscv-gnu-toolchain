/* The ptid_t type and common functions operating on it.

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

#ifndef COMMON_PTID_H
#define COMMON_PTID_H

/* The ptid struct is a collection of the various "ids" necessary for
   identifying the inferior process/thread being debugged.  This
   consists of the process id (pid), lightweight process id (lwp) and
   thread id (tid).  When manipulating ptids, the constructors,
   accessors, and predicates declared in this file should be used.  Do
   NOT access the struct ptid members directly.

   process_stratum targets that handle threading themselves should
   prefer using the ptid.lwp field, leaving the ptid.tid field for any
   thread_stratum target that might want to sit on top.
*/

#include <functional>
#include <string>
#include "gdbsupport/common-types.h"

class ptid_t
{
public:
  using pid_type = int;
  using lwp_type = long;
  using tid_type = ULONGEST;

  /* Must have a trivial defaulted default constructor so that the
     type remains POD.  */
  ptid_t () noexcept = default;

  /* Make a ptid given the necessary PID, LWP, and TID components.

     A ptid with only a PID (LWP and TID equal to zero) is usually used to
     represent a whole process, including all its lwps/threads.  */

  explicit constexpr ptid_t (pid_type pid, lwp_type lwp = 0, tid_type tid = 0)
    : m_pid (pid), m_lwp (lwp), m_tid (tid)
  {}

  /* Fetch the pid (process id) component from the ptid.  */

  constexpr pid_type pid () const
  { return m_pid; }

  /* Return true if the ptid's lwp member is non-zero.  */

  constexpr bool lwp_p () const
  { return m_lwp != 0; }

  /* Fetch the lwp (lightweight process) component from the ptid.  */

  constexpr lwp_type lwp () const
  { return m_lwp; }

  /* Return true if the ptid's tid member is non-zero.  */

  constexpr bool tid_p () const
  { return m_tid != 0; }

  /* Fetch the tid (thread id) component from a ptid.  */

  constexpr tid_type tid () const
  { return m_tid; }

  /* Return true if the ptid represents a whole process, including all its
     lwps/threads.  Such ptids have the form of (pid, 0, 0), with
     pid != -1.  */

  constexpr bool is_pid () const
  {
    return (*this != make_null ()
	    && *this != make_minus_one ()
	    && m_lwp == 0
	    && m_tid == 0);
  }

  /* Compare two ptids to see if they are equal.  */

  constexpr bool operator== (const ptid_t &other) const
  {
    return (m_pid == other.m_pid
	    && m_lwp == other.m_lwp
	    && m_tid == other.m_tid);
  }

  /* Compare two ptids to see if they are different.  */

  constexpr bool operator!= (const ptid_t &other) const
  {
    return !(*this == other);
  }

  /* Return true if the ptid matches FILTER.  FILTER can be the wild
     card MINUS_ONE_PTID (all ptids match it); can be a ptid representing
     a process (ptid.is_pid () returns true), in which case, all lwps and
     threads of that given process match, lwps and threads of other
     processes do not; or, it can represent a specific thread, in which
     case, only that thread will match true.  The ptid must represent a
     specific LWP or THREAD, it can never be a wild card.  */

  constexpr bool matches (const ptid_t &filter) const
  {
    return (/* If filter represents any ptid, it's always a match.  */
	    filter == make_minus_one ()
	    /* If filter is only a pid, any ptid with that pid
	       matches.  */
	    || (filter.is_pid () && m_pid == filter.pid ())

	    /* Otherwise, this ptid only matches if it's exactly equal
	       to filter.  */
	    || *this == filter);
  }

  /* Return a string representation of the ptid.

     This is only meant to be used in debug messages.  */

  std::string to_string () const;

  /* Make a null ptid.  */

  static constexpr ptid_t make_null ()
  { return ptid_t (0, 0, 0); }

  /* Make a minus one ptid.  */

  static constexpr ptid_t make_minus_one ()
  { return ptid_t (-1, 0, 0); }

private:
  /* Process id.  */
  pid_type m_pid;

  /* Lightweight process id.  */
  lwp_type m_lwp;

  /* Thread id.  */
  tid_type m_tid;
};

namespace std
{
template<>
struct hash<ptid_t>
{
  size_t operator() (const ptid_t &ptid) const
  {
    std::hash<long> long_hash;

    return (long_hash (ptid.pid ())
	    + long_hash (ptid.lwp ())
	    + long_hash (ptid.tid ()));
  }
};
}

/* The null or zero ptid, often used to indicate no process. */

extern const ptid_t null_ptid;

/* The (-1,0,0) ptid, often used to indicate either an error condition
   or a "don't care" condition, i.e, "run all threads."  */

extern const ptid_t minus_one_ptid;

#endif /* COMMON_PTID_H */
