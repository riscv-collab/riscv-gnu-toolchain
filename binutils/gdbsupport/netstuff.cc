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

#include "common-defs.h"
#include "netstuff.h"
#include <algorithm>

#ifdef USE_WIN32API
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

/* See gdbsupport/netstuff.h.  */

scoped_free_addrinfo::~scoped_free_addrinfo ()
{
  freeaddrinfo (m_res);
}

/* See gdbsupport/netstuff.h.  */

parsed_connection_spec
parse_connection_spec_without_prefix (std::string spec, struct addrinfo *hint)
{
  parsed_connection_spec ret;
  size_t last_colon_pos = 0;
  /* We're dealing with IPv6 if:

     - ai_family is AF_INET6, or
     - ai_family is not AF_INET, and
       - spec[0] is '[', or
       - the number of ':' on spec is greater than 1.  */
  bool is_ipv6 = (hint->ai_family == AF_INET6
		  || (hint->ai_family != AF_INET
		      && (spec[0] == '['
			  || std::count (spec.begin (),
					 spec.end (), ':') > 1)));

  if (is_ipv6)
    {
      if (spec[0] == '[')
	{
	  /* IPv6 addresses can be written as '[ADDR]:PORT', and we
	     support this notation.  */
	  size_t close_bracket_pos = spec.find_first_of (']');

	  if (close_bracket_pos == std::string::npos)
	    error (_("Missing close bracket in hostname '%s'"),
		   spec.c_str ());

	  hint->ai_family = AF_INET6;

	  const char c = spec[close_bracket_pos + 1];

	  if (c == '\0')
	    last_colon_pos = std::string::npos;
	  else if (c != ':')
	    error (_("Invalid cruft after close bracket in '%s'"),
		   spec.c_str ());

	  /* Erase both '[' and ']'.  */
	  spec.erase (0, 1);
	  spec.erase (close_bracket_pos - 1, 1);
	}
      else if (spec.find_first_of (']') != std::string::npos)
	error (_("Missing open bracket in hostname '%s'"),
	       spec.c_str ());
    }

  if (last_colon_pos == 0)
    last_colon_pos = spec.find_last_of (':');

  /* The length of the hostname part.  */
  size_t host_len;

  if (last_colon_pos != std::string::npos)
    {
      /* The user has provided a port.  */
      host_len = last_colon_pos;
      ret.port_str = spec.substr (last_colon_pos + 1);
    }
  else
    host_len = spec.size ();

  ret.host_str = spec.substr (0, host_len);

  /* Default hostname is localhost.  */
  if (ret.host_str.empty ())
    ret.host_str = "localhost";

  return ret;
}

/* See gdbsupport/netstuff.h.  */

parsed_connection_spec
parse_connection_spec (const char *spec, struct addrinfo *hint)
{
  /* Struct to hold the association between valid prefixes, their
     family and socktype.  */
  struct host_prefix
    {
      /* The prefix.  */
      const char *prefix;

      /* The 'ai_family'.  */
      int family;

      /* The 'ai_socktype'.  */
      int socktype;
    };
  static const struct host_prefix prefixes[] =
    {
      { "udp:",  AF_UNSPEC, SOCK_DGRAM },
      { "tcp:",  AF_UNSPEC, SOCK_STREAM },
      { "udp4:", AF_INET,   SOCK_DGRAM },
      { "tcp4:", AF_INET,   SOCK_STREAM },
      { "udp6:", AF_INET6,  SOCK_DGRAM },
      { "tcp6:", AF_INET6,  SOCK_STREAM },
    };

  for (const host_prefix prefix : prefixes)
    if (startswith (spec, prefix.prefix))
      {
	spec += strlen (prefix.prefix);
	hint->ai_family = prefix.family;
	hint->ai_socktype = prefix.socktype;
	hint->ai_protocol
	  = hint->ai_socktype == SOCK_DGRAM ? IPPROTO_UDP : IPPROTO_TCP;
	break;
      }

  return parse_connection_spec_without_prefix (spec, hint);
}
