/* Self tests for parsing connection specs for GDB, the GNU debugger.

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
#include "gdbsupport/selftest.h"
#include "gdbsupport/netstuff.h"
#include "diagnostics.h"
#ifdef USE_WIN32API
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

namespace selftests {
namespace parse_connection_spec_tests {

/* Auxiliary struct that holds info about a specific test for a
   connection spec.  */

struct parse_conn_test
{
  /* The connection spec.  */
  const char *connspec;

  /* Expected result from 'parse_connection_spec'.  */
  parsed_connection_spec expected_result;

  /* True if this test should fail, false otherwise.  If true, only
     the CONNSPEC field should be considered as valid.  */
  bool should_fail;

  /* The expected AI_FAMILY to be found on the 'struct addrinfo'
     HINT.  */
  int exp_ai_family;

  /* The expected AI_SOCKTYPE to be found on the 'struct addrinfo'
     HINT.  */
  int exp_ai_socktype;

  /* The expected AI_PROTOCOL to be found on the 'struct addrinfo'
     HINT.  */
  int exp_ai_protocol;
};

/* Some defines to help us fill a 'struct parse_conn_test'.  */

/* Initialize a full entry.  */
#define INIT_ENTRY(ADDR, EXP_HOST, EXP_PORT, SHOULD_FAIL, EXP_AI_FAMILY, \
		   EXP_AI_SOCKTYPE, EXP_AI_PROTOCOL)			\
  { ADDR, { EXP_HOST, EXP_PORT }, SHOULD_FAIL, EXP_AI_FAMILY, \
    EXP_AI_SOCKTYPE, EXP_AI_PROTOCOL }

/* Initialize an unprefixed entry.  In this case, we don't expect
   anything on the 'struct addrinfo' HINT.  */
#define INIT_UNPREFIXED_ENTRY(ADDR, EXP_HOST, EXP_PORT) \
  INIT_ENTRY (ADDR, EXP_HOST, EXP_PORT, false, 0, 0, 0)

/* Initialized an unprefixed IPv6 entry.  In this case, we don't
   expect anything on the 'struct addrinfo' HINT.  */
#define INIT_UNPREFIXED_IPV6_ENTRY(ADDR, EXP_HOST, EXP_PORT) \
  INIT_ENTRY (ADDR, EXP_HOST, EXP_PORT, false, AF_INET6, 0, 0)

/* Initialize a prefixed entry.  */
#define INIT_PREFIXED_ENTRY(ADDR, EXP_HOST, EXP_PORT, EXP_AI_FAMILY, \
			    EXP_AI_SOCKTYPE, EXP_AI_PROTOCOL) \
  INIT_ENTRY (ADDR, EXP_HOST, EXP_PORT, false, EXP_AI_FAMILY, \
	      EXP_AI_SOCKTYPE, EXP_AI_PROTOCOL)

/* Initialize an entry prefixed with "tcp4:".  */
#define INIT_PREFIXED_IPV4_TCP(ADDR, EXP_HOST, EXP_PORT) \
  INIT_PREFIXED_ENTRY (ADDR, EXP_HOST, EXP_PORT, AF_INET, SOCK_STREAM, \
		       IPPROTO_TCP)

/* Initialize an entry prefixed with "tcp6:".  */
#define INIT_PREFIXED_IPV6_TCP(ADDR, EXP_HOST, EXP_PORT) \
  INIT_PREFIXED_ENTRY (ADDR, EXP_HOST, EXP_PORT, AF_INET6, SOCK_STREAM, \
		       IPPROTO_TCP)

/* Initialize an entry prefixed with "udp4:".  */
#define INIT_PREFIXED_IPV4_UDP(ADDR, EXP_HOST, EXP_PORT) \
  INIT_PREFIXED_ENTRY (ADDR, EXP_HOST, EXP_PORT, AF_INET, SOCK_DGRAM, \
		       IPPROTO_UDP)

/* Initialize an entry prefixed with "udp6:".  */
#define INIT_PREFIXED_IPV6_UDP(ADDR, EXP_HOST, EXP_PORT) \
  INIT_PREFIXED_ENTRY (ADDR, EXP_HOST, EXP_PORT, AF_INET6, SOCK_DGRAM, \
		       IPPROTO_UDP)

/* Initialize a bogus entry, i.e., a connection spec that should
   fail.  */
#define INIT_BOGUS_ENTRY(ADDR) \
  INIT_ENTRY (ADDR, "", "", true, 0, 0, 0)

/* The variable which holds all of our tests.  */

static const parse_conn_test conn_test[] =
  {
    /* Unprefixed addresses.  */

    /* IPv4, host and port present.  */
    INIT_UNPREFIXED_ENTRY ("127.0.0.1:1234", "127.0.0.1", "1234"),
    /* IPv4, only host.  */
    INIT_UNPREFIXED_ENTRY ("127.0.0.1", "127.0.0.1", ""),
    /* IPv4, missing port.  */
    INIT_UNPREFIXED_ENTRY ("127.0.0.1:", "127.0.0.1", ""),

    /* IPv6, host and port present, no brackets.  */
    INIT_UNPREFIXED_ENTRY ("::1:1234", "::1", "1234"),
    /* IPv6, missing port, no brackets.  */
    INIT_UNPREFIXED_ENTRY ("::1:", "::1", ""),
    /* IPv6, host and port present, with brackets.  */
    INIT_UNPREFIXED_IPV6_ENTRY ("[::1]:1234", "::1", "1234"),
    /* IPv6, only host, with brackets.  */
    INIT_UNPREFIXED_IPV6_ENTRY ("[::1]", "::1", ""),
    /* IPv6, missing port, with brackets.  */
    INIT_UNPREFIXED_IPV6_ENTRY ("[::1]:", "::1", ""),

    /* Unspecified, only port.  */
    INIT_UNPREFIXED_ENTRY (":1234", "localhost", "1234"),

    /* Prefixed addresses.  */

    /* Prefixed "tcp4:" IPv4, host and port presents.  */
    INIT_PREFIXED_IPV4_TCP ("tcp4:127.0.0.1:1234", "127.0.0.1", "1234"),
    /* Prefixed "tcp4:" IPv4, only port.  */
    INIT_PREFIXED_IPV4_TCP ("tcp4::1234", "localhost", "1234"),
    /* Prefixed "tcp4:" IPv4, only host.  */
    INIT_PREFIXED_IPV4_TCP ("tcp4:127.0.0.1", "127.0.0.1", ""),
    /* Prefixed "tcp4:" IPv4, missing port.  */
    INIT_PREFIXED_IPV4_TCP ("tcp4:127.0.0.1:", "127.0.0.1", ""),

    /* Prefixed "udp4:" IPv4, host and port present.  */
    INIT_PREFIXED_IPV4_UDP ("udp4:127.0.0.1:1234", "127.0.0.1", "1234"),
    /* Prefixed "udp4:" IPv4, only port.  */
    INIT_PREFIXED_IPV4_UDP ("udp4::1234", "localhost", "1234"),
    /* Prefixed "udp4:" IPv4, only host.  */
    INIT_PREFIXED_IPV4_UDP ("udp4:127.0.0.1", "127.0.0.1", ""),
    /* Prefixed "udp4:" IPv4, missing port.  */
    INIT_PREFIXED_IPV4_UDP ("udp4:127.0.0.1:", "127.0.0.1", ""),


    /* Prefixed "tcp6:" IPv6, host and port present.  */
    INIT_PREFIXED_IPV6_TCP ("tcp6:::1:1234", "::1", "1234"),
    /* Prefixed "tcp6:" IPv6, only port.  */
    INIT_PREFIXED_IPV6_TCP ("tcp6::1234", "localhost", "1234"),
    /* Prefixed "tcp6:" IPv6, only host.  */
    //INIT_PREFIXED_IPV6_TCP ("tcp6:::1", "::1", ""),
    /* Prefixed "tcp6:" IPv6, missing port.  */
    INIT_PREFIXED_IPV6_TCP ("tcp6:::1:", "::1", ""),

    /* Prefixed "udp6:" IPv6, host and port present.  */
    INIT_PREFIXED_IPV6_UDP ("udp6:::1:1234", "::1", "1234"),
    /* Prefixed "udp6:" IPv6, only port.  */
    INIT_PREFIXED_IPV6_UDP ("udp6::1234", "localhost", "1234"),
    /* Prefixed "udp6:" IPv6, only host.  */
    //INIT_PREFIXED_IPV6_UDP ("udp6:::1", "::1", ""),
    /* Prefixed "udp6:" IPv6, missing port.  */
    INIT_PREFIXED_IPV6_UDP ("udp6:::1:", "::1", ""),

    /* Prefixed "tcp6:" IPv6 with brackets, host and port present.  */
    INIT_PREFIXED_IPV6_TCP ("tcp6:[::1]:1234", "::1", "1234"),
    /* Prefixed "tcp6:" IPv6 with brackets, only host.  */
    INIT_PREFIXED_IPV6_TCP ("tcp6:[::1]", "::1", ""),
    /* Prefixed "tcp6:" IPv6 with brackets, missing port.  */
    INIT_PREFIXED_IPV6_TCP ("tcp6:[::1]:", "::1", ""),

    /* Prefixed "udp6:" IPv6 with brackets, host and port present.  */
    INIT_PREFIXED_IPV6_UDP ("udp6:[::1]:1234", "::1", "1234"),
    /* Prefixed "udp6:" IPv6 with brackets, only host.  */
    INIT_PREFIXED_IPV6_UDP ("udp6:[::1]", "::1", ""),
    /* Prefixed "udp6:" IPv6 with brackets, missing port.  */
    INIT_PREFIXED_IPV6_UDP ("udp6:[::1]:", "::1", ""),


    /* Bogus addresses.  */
    INIT_BOGUS_ENTRY ("tcp6:[::1]123:44"),
    INIT_BOGUS_ENTRY ("[::1"),
    INIT_BOGUS_ENTRY ("tcp6:::1]:"),
  };

/* Test a connection spec C.  */

static void
test_conn (const parse_conn_test &c)
{
  struct addrinfo hint;
  parsed_connection_spec ret;

  memset (&hint, 0, sizeof (hint));

  try
    {
      ret = parse_connection_spec (c.connspec, &hint);
    }
  catch (const gdb_exception_error &ex)
    {
      /* If we caught an error, we should check if this connection
	 spec was supposed to fail.  */
      SELF_CHECK (c.should_fail);
      return;
    }

  SELF_CHECK (!c.should_fail);
  SELF_CHECK (ret.host_str == c.expected_result.host_str);
  SELF_CHECK (ret.port_str == c.expected_result.port_str);
  SELF_CHECK (hint.ai_family == c.exp_ai_family);
  SELF_CHECK (hint.ai_socktype == c.exp_ai_socktype);
  SELF_CHECK (hint.ai_protocol == c.exp_ai_protocol);
}

/* Run the tests associated with parsing connection specs.  */

static void
run_tests ()
{
  for (const parse_conn_test &c : conn_test)
    test_conn (c);
}
} /* namespace parse_connection_spec_tests */
} /* namespace selftests */

void _initialize_parse_connection_spec_selftests ();
void
_initialize_parse_connection_spec_selftests ()
{
  selftests::register_test ("parse_connection_spec",
			    selftests::parse_connection_spec_tests::run_tests);
}
