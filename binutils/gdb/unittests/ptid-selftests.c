/* Self tests for ptid_t for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/ptid.h"
#include <type_traits>

namespace selftests {
namespace ptid {

/* Check that the ptid_t class is POD.

   This is a requirement for as long as we have ptids embedded in
   structures allocated with malloc. */

static_assert (gdb::And<std::is_standard_layout<ptid_t>,
			std::is_trivial<ptid_t>>::value,
	       "ptid_t is POD");

/* We want to avoid implicit conversion from int to ptid_t.  */

static_assert (!std::is_convertible<int, ptid_t>::value,
	       "constructor is explicit");

/* Build some useful ptids.  */

static constexpr ptid_t pid = ptid_t (1);
static constexpr ptid_t lwp = ptid_t (1, 2, 0);
static constexpr ptid_t tid = ptid_t (1, 0, 2);
static constexpr ptid_t both = ptid_t (1, 2, 2);

/* Build some constexpr version of null_ptid and minus_one_ptid to use in
   static_assert.  Once the real ones are made constexpr, we can get rid of
   these.  */

static constexpr ptid_t null = ptid_t::make_null ();
static constexpr ptid_t minus_one = ptid_t::make_minus_one ();

/* Verify pid.  */

static_assert (pid.pid () == 1, "pid's pid is right");
static_assert (lwp.pid () == 1, "lwp's pid is right");
static_assert (tid.pid () == 1, "tid's pid is right");
static_assert (both.pid () == 1, "both's pid is right");

/* Verify lwp_p.  */

static_assert (!pid.lwp_p (), "pid's lwp_p is right");
static_assert (lwp.lwp_p (), "lwp's lwp_p is right");
static_assert (!tid.lwp_p (), "tid's lwp_p is right");
static_assert (both.lwp_p (), "both's lwp_p is right");

/* Verify lwp.  */

static_assert (pid.lwp () == 0, "pid's lwp is right");
static_assert (lwp.lwp () == 2, "lwp's lwp is right");
static_assert (tid.lwp () == 0, "tid's lwp is right");
static_assert (both.lwp () == 2, "both's lwp is right");

/* Verify tid_p.  */

static_assert (!pid.tid_p (), "pid's tid_p is right");
static_assert (!lwp.tid_p (), "lwp's tid_p is right");
static_assert (tid.tid_p (), "tid's tid_p is right");
static_assert (both.tid_p (), "both's tid_p is right");

/* Verify tid.  */

static_assert (pid.tid () == 0, "pid's tid is right");
static_assert (lwp.tid () == 0, "lwp's tid is right");
static_assert (tid.tid () == 2, "tid's tid is right");
static_assert (both.tid () == 2, "both's tid is right");

/* Verify is_pid.  */

static_assert (pid.is_pid (), "pid is a pid");
static_assert (!lwp.is_pid (), "lwp isn't a pid");
static_assert (!tid.is_pid (), "tid isn't a pid");
static_assert (!both.is_pid (), "both isn't a pid");
static_assert (!null.is_pid (), "null ptid isn't a pid");
static_assert (!minus_one.is_pid (), "minus one ptid isn't a pid");

/* Verify operator ==.  */

static_assert (pid == ptid_t (1, 0, 0), "pid operator== is right");
static_assert (lwp == ptid_t (1, 2, 0), "lwp operator== is right");
static_assert (tid == ptid_t (1, 0, 2), "tid operator== is right");
static_assert (both == ptid_t (1, 2, 2), "both operator== is right");

/* Verify operator !=.  */

static_assert (pid != ptid_t (2, 0, 0), "pid isn't equal to a different pid");
static_assert (pid != lwp, "pid isn't equal to one of its thread");
static_assert (lwp != tid, "lwp isn't equal to tid");
static_assert (both != lwp, "both isn't equal to lwp");
static_assert (both != tid, "both isn't equal to tid");

/* Verify matches against minus_one.  */

static_assert (pid.matches (minus_one), "pid matches minus one");
static_assert (lwp.matches (minus_one), "lwp matches minus one");
static_assert (tid.matches (minus_one), "tid matches minus one");
static_assert (both.matches (minus_one), "both matches minus one");

/* Verify matches against pid.  */

static_assert (pid.matches (pid), "pid matches pid");
static_assert (lwp.matches (pid), "lwp matches pid");
static_assert (tid.matches (pid), "tid matches pid");
static_assert (both.matches (pid), "both matches pid");
static_assert (!ptid_t (2, 0, 0).matches (pid), "other pid doesn't match pid");
static_assert (!ptid_t (2, 2, 0).matches (pid), "other lwp doesn't match pid");
static_assert (!ptid_t (2, 0, 2).matches (pid), "other tid doesn't match pid");
static_assert (!ptid_t (2, 2, 2).matches (pid), "other both doesn't match pid");

/* Verify matches against exact matches.  */

static_assert (!pid.matches (lwp), "pid doesn't match lwp");
static_assert (lwp.matches (lwp), "lwp matches lwp");
static_assert (!tid.matches (lwp), "tid doesn't match lwp");
static_assert (!both.matches (lwp), "both doesn't match lwp");
static_assert (!ptid_t (2, 2, 0).matches (lwp), "other lwp doesn't match lwp");

static_assert (!pid.matches (tid), "pid doesn't match tid");
static_assert (!lwp.matches (tid), "lwp doesn't match tid");
static_assert (tid.matches (tid), "tid matches tid");
static_assert (!both.matches (tid), "both doesn't match tid");
static_assert (!ptid_t (2, 0, 2).matches (tid), "other tid doesn't match tid");

static_assert (!pid.matches (both), "pid doesn't match both");
static_assert (!lwp.matches (both), "lwp doesn't match both");
static_assert (!tid.matches (both), "tid doesn't match both");
static_assert (both.matches (both), "both matches both");
static_assert (!ptid_t (2, 2, 2).matches (both),
	       "other both doesn't match both");


} /* namespace ptid */
} /* namespace selftests */
