/* The find command.

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

#include "defs.h"
#include "arch-utils.h"
#include <ctype.h>
#include "gdbcmd.h"
#include "value.h"
#include "target.h"
#include "cli/cli-utils.h"
#include <algorithm>
#include "gdbsupport/byte-vector.h"

/* Copied from bfd_put_bits.  */

static void
put_bits (uint64_t data, gdb::byte_vector &buf, int bits, bfd_boolean big_p)
{
  int i;
  int bytes;

  gdb_assert (bits % 8 == 0);

  bytes = bits / 8;
  size_t last = buf.size ();
  buf.resize (last + bytes);
  for (i = 0; i < bytes; i++)
    {
      int index = big_p ? bytes - i - 1 : i;

      buf[last + index] = data & 0xff;
      data >>= 8;
    }
}

/* Subroutine of find_command to simplify it.
   Parse the arguments of the "find" command.  */

static gdb::byte_vector
parse_find_args (const char *args, ULONGEST *max_countp,
		 CORE_ADDR *start_addrp, ULONGEST *search_space_lenp,
		 bfd_boolean big_p)
{
  /* Default to using the specified type.  */
  char size = '\0';
  ULONGEST max_count = ~(ULONGEST) 0;
  /* Buffer to hold the search pattern.  */
  gdb::byte_vector pattern_buf;
  CORE_ADDR start_addr;
  ULONGEST search_space_len;
  const char *s = args;
  struct value *v;

  if (args == NULL)
    error (_("Missing search parameters."));

  /* Get search granularity and/or max count if specified.
     They may be specified in either order, together or separately.  */

  while (*s == '/')
    {
      ++s;

      while (*s != '\0' && *s != '/' && !isspace (*s))
	{
	  if (isdigit (*s))
	    {
	      max_count = atoi (s);
	      while (isdigit (*s))
		++s;
	      continue;
	    }

	  switch (*s)
	    {
	    case 'b':
	    case 'h':
	    case 'w':
	    case 'g':
	      size = *s++;
	      break;
	    default:
	      error (_("Invalid size granularity."));
	    }
	}

      s = skip_spaces (s);
    }

  /* Get the search range.  */

  v = parse_to_comma_and_eval (&s);
  start_addr = value_as_address (v);

  if (*s == ',')
    ++s;
  s = skip_spaces (s);

  if (*s == '+')
    {
      LONGEST len;

      ++s;
      v = parse_to_comma_and_eval (&s);
      len = value_as_long (v);
      if (len == 0)
	{
	  gdb_printf (_("Empty search range.\n"));
	  return pattern_buf;
	}
      if (len < 0)
	error (_("Invalid length."));
      /* Watch for overflows.  */
      if (len > CORE_ADDR_MAX
	  || (start_addr + len - 1) < start_addr)
	error (_("Search space too large."));
      search_space_len = len;
    }
  else
    {
      CORE_ADDR end_addr;

      v = parse_to_comma_and_eval (&s);
      end_addr = value_as_address (v);
      if (start_addr > end_addr)
	error (_("Invalid search space, end precedes start."));
      search_space_len = end_addr - start_addr + 1;
      /* We don't support searching all of memory
	 (i.e. start=0, end = 0xff..ff).
	 Bail to avoid overflows later on.  */
      if (search_space_len == 0)
	error (_("Overflow in address range "
		 "computation, choose smaller range."));
    }

  if (*s == ',')
    ++s;

  /* Fetch the search string.  */

  while (*s != '\0')
    {
      LONGEST x;
      struct type *t;

      s = skip_spaces (s);

      v = parse_to_comma_and_eval (&s);
      t = v->type ();

      if (size != '\0')
	{
	  x = value_as_long (v);
	  switch (size)
	    {
	    case 'b':
	      pattern_buf.push_back (x);
	      break;
	    case 'h':
	      put_bits (x, pattern_buf, 16, big_p);
	      break;
	    case 'w':
	      put_bits (x, pattern_buf, 32, big_p);
	      break;
	    case 'g':
	      put_bits (x, pattern_buf, 64, big_p);
	      break;
	    }
	}
      else
	{
	  const gdb_byte *contents = v->contents ().data ();
	  pattern_buf.insert (pattern_buf.end (), contents,
			      contents + t->length ());
	}

      if (*s == ',')
	++s;
      s = skip_spaces (s);
    }

  if (pattern_buf.empty ())
    error (_("Missing search pattern."));

  if (search_space_len < pattern_buf.size ())
    error (_("Search space too small to contain pattern."));

  *max_countp = max_count;
  *start_addrp = start_addr;
  *search_space_lenp = search_space_len;

  return pattern_buf;
}

static void
find_command (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  bfd_boolean big_p = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG;
  /* Command line parameters.
     These are initialized to avoid uninitialized warnings from -Wall.  */
  ULONGEST max_count = 0;
  CORE_ADDR start_addr = 0;
  ULONGEST search_space_len = 0;
  /* End of command line parameters.  */
  unsigned int found_count;
  CORE_ADDR last_found_addr;

  gdb::byte_vector pattern_buf = parse_find_args (args, &max_count,
						  &start_addr,
						  &search_space_len,
						  big_p);

  /* Perform the search.  */

  found_count = 0;
  last_found_addr = 0;

  while (search_space_len >= pattern_buf.size ()
	 && found_count < max_count)
    {
      /* Offset from start of this iteration to the next iteration.  */
      ULONGEST next_iter_incr;
      CORE_ADDR found_addr;
      int found = target_search_memory (start_addr, search_space_len,
					pattern_buf.data (),
					pattern_buf.size (),
					&found_addr);

      if (found <= 0)
	break;

      print_address (gdbarch, found_addr, gdb_stdout);
      gdb_printf ("\n");
      ++found_count;
      last_found_addr = found_addr;

      /* Begin next iteration at one byte past this match.  */
      next_iter_incr = (found_addr - start_addr) + 1;

      /* For robustness, we don't let search_space_len go -ve here.  */
      if (search_space_len >= next_iter_incr)
	search_space_len -= next_iter_incr;
      else
	search_space_len = 0;
      start_addr += next_iter_incr;
    }

  /* Record and print the results.  */

  set_internalvar_integer (lookup_internalvar ("numfound"), found_count);
  if (found_count > 0)
    {
      struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;

      set_internalvar (lookup_internalvar ("_"),
		       value_from_pointer (ptr_type, last_found_addr));
    }

  if (found_count == 0)
    gdb_printf ("Pattern not found.\n");
  else
    gdb_printf ("%d pattern%s found.\n", found_count,
		found_count > 1 ? "s" : "");
}

void _initialize_mem_search ();
void
_initialize_mem_search ()
{
  add_cmd ("find", class_vars, find_command, _("\
Search memory for a sequence of bytes.\n\
Usage:\nfind \
[/SIZE-CHAR] [/MAX-COUNT] START-ADDRESS, END-ADDRESS, EXPR1 [, EXPR2 ...]\n\
find [/SIZE-CHAR] [/MAX-COUNT] START-ADDRESS, +LENGTH, EXPR1 [, EXPR2 ...]\n\
SIZE-CHAR is one of b,h,w,g for 8,16,32,64 bit values respectively,\n\
and if not specified the size is taken from the type of the expression\n\
in the current language.\n\
The two-address form specifies an inclusive range.\n\
Note that this means for example that in the case of C-like languages\n\
a search for an untyped 0x42 will search for \"(int) 0x42\"\n\
which is typically four bytes, and a search for a string \"hello\" will\n\
include the trailing '\\0'.  The null terminator can be removed from\n\
searching by using casts, e.g.: {char[5]}\"hello\".\n\
\n\
The address of the last match is stored as the value of \"$_\".\n\
Convenience variable \"$numfound\" is set to the number of matches."),
	   &cmdlist);
}
