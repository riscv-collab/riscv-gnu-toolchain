/* Disable address space randomization based on inferior personality.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "nat/linux-personality.h"

#include <sys/personality.h>

# if !HAVE_DECL_ADDR_NO_RANDOMIZE
#  define ADDR_NO_RANDOMIZE 0x0040000
# endif /* ! HAVE_DECL_ADDR_NO_RANDOMIZE */

/* See comment on nat/linux-personality.h.  */

maybe_disable_address_space_randomization::
maybe_disable_address_space_randomization (int disable_randomization)
  : m_personality_set (false),
    m_personality_orig (0)
{
  if (disable_randomization)
    {
      errno = 0;
      m_personality_orig = personality (0xffffffff);
      if (errno == 0 && !(m_personality_orig & ADDR_NO_RANDOMIZE))
	{
	  m_personality_set = true;
	  personality (m_personality_orig | ADDR_NO_RANDOMIZE);
	}
      if (errno != 0 || (m_personality_set
			 && !(personality (0xffffffff) & ADDR_NO_RANDOMIZE)))
	warning (_("Error disabling address space randomization: %s"),
		 safe_strerror (errno));
    }
}

maybe_disable_address_space_randomization::
~maybe_disable_address_space_randomization ()
{
  if (m_personality_set)
    {
      errno = 0;
      personality (m_personality_orig);
      if (errno != 0)
	warning (_("Error restoring address space randomization: %s"),
		 safe_strerror (errno));
    }
}
