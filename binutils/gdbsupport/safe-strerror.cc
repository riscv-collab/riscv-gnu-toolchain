/* Safe version of strerror for GDB, the GNU debugger.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include <string.h>

/* There are two different versions of strerror_r; one is GNU-specific, the
   other XSI-compliant.  They differ in the return type.  This overload lets
   us choose the right behavior for each return type.  We cannot rely on Gnulib
   to solve this for us because IPA does not use Gnulib but uses this
   function.  */

/* Called if we have a XSI-compliant strerror_r.  */
ATTRIBUTE_UNUSED static char *
select_strerror_r (int res, char *buf)
{
  return res == 0 ? buf : nullptr;
}

/* Called if we have a GNU strerror_r.  */
ATTRIBUTE_UNUSED static char *
select_strerror_r (char *res, char *)
{
  return res;
}

/* Implementation of safe_strerror as defined in common-utils.h.  */

const char *
safe_strerror (int errnum)
{
  static thread_local char buf[1024];

  char *res = select_strerror_r (strerror_r (errnum, buf, sizeof (buf)), buf);
  if (res != nullptr)
    return res;

  xsnprintf (buf, sizeof buf, "(undocumented errno %d)", errnum);
  return buf;
}
