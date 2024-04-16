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

#ifndef NAT_LINUX_PERSONALITY_H
#define NAT_LINUX_PERSONALITY_H

class maybe_disable_address_space_randomization
{
public:

  /* Disable the inferior's address space randomization if
     DISABLE_RANDOMIZATION is not zero and if we have
     <sys/personality.h>. */
  maybe_disable_address_space_randomization (int disable_randomization);

  /* On destruction, re-enable address space randomization.  */
  ~maybe_disable_address_space_randomization ();

  DISABLE_COPY_AND_ASSIGN (maybe_disable_address_space_randomization);

private:

  /* True if the personality was set in the constructor.  */
  bool m_personality_set;

  /* If m_personality_set is true, the original personality value.  */
  int m_personality_orig;
};

#endif /* ! NAT_LINUX_PERSONALITY_H */
