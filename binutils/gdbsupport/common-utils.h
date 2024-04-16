/* Shared general utility routines for GDB, the GNU debugger.

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

#ifndef COMMON_COMMON_UTILS_H
#define COMMON_COMMON_UTILS_H

#include <string>
#include <vector>
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/gdb_unique_ptr.h"
#include "gdbsupport/array-view.h"
#include "poison.h"
#include <string_view>

#if defined HAVE_LIBXXHASH
#  include <xxhash.h>
#else
#  include "hashtab.h"
#endif

/* xmalloc(), xrealloc() and xcalloc() have already been declared in
   "libiberty.h". */

/* Like xmalloc, but zero the memory.  */
void *xzalloc (size_t);

/* Like asprintf and vasprintf, but return the string, throw an error
   if no memory.  */
gdb::unique_xmalloc_ptr<char> xstrprintf (const char *format, ...)
     ATTRIBUTE_PRINTF (1, 2);
gdb::unique_xmalloc_ptr<char> xstrvprintf (const char *format, va_list ap)
     ATTRIBUTE_PRINTF (1, 0);

/* Like snprintf, but throw an error if the output buffer is too small.  */
int xsnprintf (char *str, size_t size, const char *format, ...)
     ATTRIBUTE_PRINTF (3, 4);

/* Returns a std::string built from a printf-style format string.  */
std::string string_printf (const char* fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

/* Like string_printf, but takes a va_list.  */
std::string string_vprintf (const char* fmt, va_list args)
  ATTRIBUTE_PRINTF (1, 0);

/* Like string_printf, but appends to DEST instead of returning a new
   std::string.  */
std::string &string_appendf (std::string &dest, const char* fmt, ...)
  ATTRIBUTE_PRINTF (2, 3);

/* Like string_appendf, but takes a va_list.  */
std::string &string_vappendf (std::string &dest, const char* fmt, va_list args)
  ATTRIBUTE_PRINTF (2, 0);

/* Make a copy of the string at PTR with LEN characters
   (and add a null character at the end in the copy).
   Uses malloc to get the space.  Returns the address of the copy.  */

char *savestring (const char *ptr, size_t len);

/* Extract the next word from ARG.  The next word is defined as either,
   everything up to the next space, or, if the next word starts with either
   a single or double quote, then everything up to the closing quote.  The
   enclosing quotes are not returned in the result string.  The pointer in
   ARG is updated to point to the first character after the end of the
   word, or, for quoted words, the first character after the closing
   quote.  */

std::string extract_string_maybe_quoted (const char **arg);

/* The strerror() function can return NULL for errno values that are
   out of range.  Provide a "safe" version that always returns a
   printable string.  This version is also thread-safe.  */

extern const char *safe_strerror (int);

/* Version of startswith that takes string_view arguments.  Return
   true if the start of STRING matches PATTERN, false otherwise.  */

static inline bool
startswith (std::string_view string, std::string_view pattern)
{
  return (string.length () >= pattern.length ()
	  && strncmp (string.data (), pattern.data (), pattern.length ()) == 0);
}

/* Return true if the strings are equal.  */

static inline bool
streq (const char *lhs, const char *rhs)
{
  return strcmp (lhs, rhs) == 0;
}

/* Compare C strings for std::sort.  */

static inline bool
compare_cstrings (const char *str1, const char *str2)
{
  return strcmp (str1, str2) < 0;
}

ULONGEST strtoulst (const char *num, const char **trailer, int base);

/* Skip leading whitespace characters in INP, returning an updated
   pointer.  If INP is NULL, return NULL.  */

extern char *skip_spaces (char *inp);

/* A const-correct version of the above.  */

extern const char *skip_spaces (const char *inp);

/* Skip leading non-whitespace characters in INP, returning an updated
   pointer.  If INP is NULL, return NULL.  */

extern char *skip_to_space (char *inp);

/* A const-correct version of the above.  */

extern const char *skip_to_space (const char *inp);

/* Assumes that V is an argv for a program, and iterates through
   freeing all the elements.  */
extern void free_vector_argv (std::vector<char *> &v);

/* Return true if VALUE is in [LOW, HIGH].  */

template <typename T>
static bool
in_inclusive_range (T value, T low, T high)
{
  return value >= low && value <= high;
}

/* Ensure that V is aligned to an N byte boundary (N's assumed to be a
   power of 2).  Round up/down when necessary.  Examples of correct
   use include:

    addr = align_up (addr, 8); -- VALUE needs 8 byte alignment
    write_memory (addr, value, len);
    addr += len;

   and:

    sp = align_down (sp - len, 16); -- Keep SP 16 byte aligned
    write_memory (sp, value, len);

   Note that uses such as:

    write_memory (addr, value, len);
    addr += align_up (len, 8);

   and:

    sp -= align_up (len, 8);
    write_memory (sp, value, len);

   are typically not correct as they don't ensure that the address (SP
   or ADDR) is correctly aligned (relying on previous alignment to
   keep things right).  This is also why the methods are called
   "align_..." instead of "round_..." as the latter reads better with
   this incorrect coding style.  */

extern ULONGEST align_up (ULONGEST v, int n);
extern ULONGEST align_down (ULONGEST v, int n);

/* Convert hex digit A to a number, or throw an exception.  */
extern int fromhex (int a);

/* HEX is a string of characters representing hexadecimal digits.
   Convert pairs of hex digits to bytes and store sequentially into
   BIN.  COUNT is the maximum number of characters to convert.  This
   will convert fewer characters if the number of hex characters
   actually seen is odd, or if HEX terminates before COUNT characters.
   Returns the number of characters actually converted.  */
extern int hex2bin (const char *hex, gdb_byte *bin, int count);

/* Like the above, but return a gdb::byte_vector.  */
gdb::byte_vector hex2bin (const char *hex);

/* Build a string containing the contents of BYTES.  Each byte is
   represented as a 2 character hex string, with spaces separating each
   individual byte.  */

extern std::string bytes_to_string (gdb::array_view<const gdb_byte> bytes);

/* See bytes_to_string above.  This takes a BUFFER pointer and LENGTH
   rather than an array view.  */

static inline std::string bytes_to_string (const gdb_byte *buffer,
					   size_t length)
{
  return bytes_to_string ({buffer, length});
}

/* A fast hashing function.  This can be used to hash data in a fast way
   when the length is known.  If no fast hashing library is available, falls
   back to iterative_hash from libiberty.  START_VALUE can be set to
   continue hashing from a previous value.  */

static inline unsigned int
fast_hash (const void *ptr, size_t len, unsigned int start_value = 0)
{
#if defined HAVE_LIBXXHASH
  return XXH64 (ptr, len, start_value);
#else
  return iterative_hash (ptr, len, start_value);
#endif
}

namespace gdb
{

/* Hash type for std::string_view.

   Even after we switch to C++17 and dump our string_view implementation, we
   might want to keep this hash implementation if it's faster than std::hash
   for std::string_view.  */

struct string_view_hash
{
  std::size_t operator() (std::string_view view) const
  {  return fast_hash (view.data (), view.length ()); }
};

} /* namespace gdb */

#endif /* COMMON_COMMON_UTILS_H */
