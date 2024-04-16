/* TID parsing for GDB, the GNU debugger.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef TID_PARSE_H
#define TID_PARSE_H

#include "cli/cli-utils.h"

struct thread_info;

/* Issue an invalid thread ID error, pointing at STRING, the invalid
   ID.  */
extern void ATTRIBUTE_NORETURN invalid_thread_id_error (const char *string);

/* Parse TIDSTR as a per-inferior thread ID, in either INF_NUM.THR_NUM
   or THR_NUM form.  In the latter case, the missing INF_NUM is filled
   in from the current inferior.  If ENDPTR is not NULL,
   parse_thread_id stores the address of the first character after the
   thread ID.  Either a valid thread is returned, or an error is
   thrown.  */
struct thread_info *parse_thread_id (const char *tidstr, const char **end);

/* Parse a thread ID or a thread range list.

   A range will be of the form

     <inferior_num>.<thread_number1>-<thread_number2>

   and will represent all the threads of inferior INFERIOR_NUM with
   number between THREAD_NUMBER1 and THREAD_NUMBER2, inclusive.
   <inferior_num> can also be omitted, as in

     <thread_number1>-<thread_number2>

   in which case GDB infers the inferior number from the default
   passed to the constructor or to the last call to the init
   function.  */
class tid_range_parser
{
public:
  /* Default construction.  Must call init before calling get_*.  */
  tid_range_parser () {}

  /* Calls init automatically.  See init for description of
     parameters.  */
  tid_range_parser (const char *tidlist, int default_inferior);

  /* Reinitialize a tid_range_parser.  TIDLIST is the string to be
     parsed.  DEFAULT_INFERIOR is the inferior number to assume if a
     non-qualified thread ID is found.  */
  void init (const char *tidlist, int default_inferior);

  /* Parse a thread ID or a thread range list.

     This function is designed to be called iteratively.  While
     processing a thread ID range list, at each call it will return
     (in the INF_NUM and THR_NUM output parameters) the next thread ID
     in the range (irrespective of whether the thread actually
     exists).

     At the beginning of parsing a thread range, the char pointer
     PARSER->m_cur_tok will be advanced past <thread_number1> and left
     pointing at the '-' token.  Subsequent calls will not advance the
     pointer until the range is completed.  The call that completes
     the range will advance the pointer past <thread_number2>.

     This function advances through the input string for as long you
     call it.  Once the end of the input string is reached, a call to
     finished returns false (see below).

     E.g., with list: "1.2 3.4-6":

     1st call: *INF_NUM=1; *THR_NUM=2 (finished==0)
     2nd call: *INF_NUM=3; *THR_NUM=4 (finished==0)
     3rd call: *INF_NUM=3; *THR_NUM=5 (finished==0)
     4th call: *INF_NUM=3; *THR_NUM=6 (finished==1)

     Returns true if a thread/range is parsed successfully, false
     otherwise.  */
  bool get_tid (int *inf_num, int *thr_num);

  /* Like get_tid, but return a thread ID range per call, rather then
     a single thread ID.

     If the next element in the list is a single thread ID, then
     *THR_START and *THR_END are set to the same value.

     E.g.,. with list: "1.2 3.4-6"

     1st call: *INF_NUM=1; *THR_START=2; *THR_END=2 (finished==0)
     2nd call: *INF_NUM=3; *THR_START=4; *THR_END=6 (finished==1)

     Returns true if parsed a thread/range successfully, false
     otherwise.  */
  bool get_tid_range (int *inf_num, int *thr_start, int *thr_end);

  /* Returns true if processing a star wildcard (e.g., "1.*")
     range.  */
  bool in_star_range () const;

  /* Returns true if processing a thread range (e.g., 1.2-3).  */
  bool in_thread_range () const;

  /* Returns true if parsing has completed.  */
  bool finished () const;

  /* Return the current token being parsed.  When parsing has
     finished, this points past the last parsed token.  */
  const char *cur_tok () const;

  /* When parsing a range, advance past the final token in the
     range.  */
  void skip_range ();

  /* True if the TID last parsed was explicitly inferior-qualified.
     IOW, whether the spec specified an inferior number
     explicitly.  */
  bool tid_is_qualified () const;

private:
  /* No need for these.  They are intentionally not defined anywhere.  */
  tid_range_parser (const tid_range_parser &);
  tid_range_parser &operator= (const tid_range_parser &);

  bool get_tid_or_range (int *inf_num, int *thr_start, int *thr_end);

  /* The possible states of the tid range parser's state machine,
     indicating what sub-component are we expecting.  */
  enum
    {
      /* Parsing the inferior number.  */
      STATE_INFERIOR,

      /* Parsing the thread number or thread number range.  */
      STATE_THREAD_RANGE,

      /* Parsing a star wildcard thread range.  E.g., "1.*".  */
      STATE_STAR_RANGE,
    } m_state;

  /* The string being parsed.  When parsing has finished, this points
     past the last parsed token.  */
  const char *m_cur_tok;

  /* The range parser state when we're parsing the thread number
     sub-component.  */
  number_or_range_parser m_range_parser;

  /* Last inferior number returned.  */
  int m_inf_num;

  /* True if the TID last parsed was explicitly inferior-qualified.
     IOW, whether the spec specified an inferior number
     explicitly.  */
  bool m_qualified;

  /* The inferior number to assume if the TID is not qualified.  */
  int m_default_inferior;
};


/* Accept a string-form list of thread IDs such as is accepted by
   tid_range_parser.  Return true if the INF_NUM.THR.NUM thread is in
   the list.  DEFAULT_INFERIOR is the inferior number to assume if a
   non-qualified thread ID is found in the list.

   By definition, an empty list includes all threads.  This is to be
   interpreted as typing a command such as "info threads" with no
   arguments.  */
extern int tid_is_in_list (const char *list, int default_inferior,
			   int inf_num, int thr_num);

#endif /* TID_PARSE_H */
