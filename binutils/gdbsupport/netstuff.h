/* Operations on network stuff.
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

#ifndef COMMON_NETSTUFF_H
#define COMMON_NETSTUFF_H

#include <string>

/* Like NI_MAXHOST/NI_MAXSERV, but enough for numeric forms.  */
#define GDB_NI_MAX_ADDR 64
#define GDB_NI_MAX_PORT 16

/* Helper class to guarantee that we always call 'freeaddrinfo'.  */

class scoped_free_addrinfo
{
public:
  /* Default constructor.  */
  explicit scoped_free_addrinfo (struct addrinfo *ainfo)
    : m_res (ainfo)
  {
  }

  /* Destructor responsible for free'ing M_RES by calling
     'freeaddrinfo'.  */
  ~scoped_free_addrinfo ();

  DISABLE_COPY_AND_ASSIGN (scoped_free_addrinfo);

private:
  /* The addrinfo resource.  */
  struct addrinfo *m_res;
};

/* The struct we return after parsing the connection spec.  */

struct parsed_connection_spec
{
  /* The hostname.  */
  std::string host_str;

  /* The port, if any.  */
  std::string port_str;
};


/* Parse SPEC (which is a string in the form of "ADDR:PORT") and
   return a 'parsed_connection_spec' structure with the proper fields
   filled in.  Also adjust HINT accordingly.  */
extern parsed_connection_spec
  parse_connection_spec_without_prefix (std::string spec,
					struct addrinfo *hint);

/* Parse SPEC (which is a string in the form of
   "[tcp[6]:|udp[6]:]ADDR:PORT") and return a 'parsed_connection_spec'
   structure with the proper fields filled in.  Also adjust HINT
   accordingly.  */
extern parsed_connection_spec parse_connection_spec (const char *spec,
						     struct addrinfo *hint);

#endif /* COMMON_NETSTUFF_H */
