/* CLI utilities.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef CLI_CLI_UTILS_H
#define CLI_CLI_UTILS_H

#include "completer.h"

struct cmd_list_element;

/* *PP is a string denoting a number.  Get the number.  Advance *PP
   after the string and any trailing whitespace.

   The string can either be a number, or "$" followed by the name of a
   convenience variable, or ("$" or "$$") followed by digits.

   TRAILER is a character which can be found after the number; most
   commonly this is `-'.  If you don't want a trailer, use \0.  */

extern int get_number_trailer (const char **pp, int trailer);

/* Convenience.  Like get_number_trailer, but with no TRAILER.  */

extern int get_number (const char **);

/* Like the above, but takes a non-const "char **".  */

extern int get_number (char **);

/* Like get_number_trailer, but works with ULONGEST, and throws on
   error instead of returning 0.  */
extern ULONGEST get_ulongest (const char **pp, int trailer = '\0');

/* Throws an error telling the user that ARGS starts with an option
   unrecognized by COMMAND.  */

extern void report_unrecognized_option_error (const char *command,
					      const char *args);


/* Builds the help string for a command documented by PREFIX,
   followed by the extract_info_print_args help for ENTITY_KIND.  If
   DOCUMENT_N_FLAG is true then help text describing the -n flag is also
   included.  */

const char *info_print_args_help (const char *prefix,
				  const char *entity_kind,
				  bool document_n_flag);

/* Parse a number or a range.
   A number will be of the form handled by get_number.
   A range will be of the form <number1> - <number2>, and
   will represent all the integers between number1 and number2,
   inclusive.  */

class number_or_range_parser
{
public:
  /* Default construction.  Must call init before calling
     get_next.  */
  number_or_range_parser () {}

  /* Calls init automatically.  */
  number_or_range_parser (const char *string);

  /* STRING is the string to be parsed.  */
  void init (const char *string);

  /* While processing a range, this fuction is called iteratively; At
     each call it will return the next value in the range.

     At the beginning of parsing a range, the char pointer
     STATE->m_cur_tok will be advanced past <number1> and left
     pointing at the '-' token.  Subsequent calls will not advance the
     pointer until the range is completed.  The call that completes
     the range will advance the pointer past <number2>.  */
  int get_number ();

  /* Setup internal state such that get_next() returns numbers in the
     START_VALUE to END_VALUE range.  END_PTR is where the string is
     advanced to when get_next() returns END_VALUE.  */
  void setup_range (int start_value, int end_value,
		    const char *end_ptr);

  /* Returns true if parsing has completed.  */
  bool finished () const;

  /* Return the string being parsed.  When parsing has finished, this
     points past the last parsed token.  */
  const char *cur_tok () const
  { return m_cur_tok; }

  /* True when parsing a range.  */
  bool in_range () const
  { return m_in_range; }

  /* When parsing a range, the final value in the range.  */
  int end_value () const
  { return m_end_value; }

  /* When parsing a range, skip past the final token in the range.  */
  void skip_range ()
  {
    gdb_assert (m_in_range);
    m_cur_tok = m_end_ptr;
    m_in_range = false;
  }

private:
  /* No need for these.  They are intentionally not defined anywhere.  */
  number_or_range_parser (const number_or_range_parser &);
  number_or_range_parser &operator= (const number_or_range_parser &);

  /* The string being parsed.  When parsing has finished, this points
     past the last parsed token.  */
  const char *m_cur_tok;

  /* Last value returned.  */
  int m_last_retval;

  /* When parsing a range, the final value in the range.  */
  int m_end_value;

  /* When parsing a range, a pointer past the final token in the
     range.  */
  const char *m_end_ptr;

  /* True when parsing a range.  */
  bool m_in_range;
};

/* Accept a number and a string-form list of numbers such as is 
   accepted by get_number_or_range.  Return TRUE if the number is
   in the list.

   By definition, an empty list includes all numbers.  This is to 
   be interpreted as typing a command such as "delete break" with 
   no arguments.  */

extern int number_is_in_list (const char *list, int number);

/* Reverse S to the last non-whitespace character without skipping past
   START.  */

extern const char *remove_trailing_whitespace (const char *start,
					       const char *s);

/* Same, for non-const S.  */

static inline char *
remove_trailing_whitespace (const char *start, char *s)
{
  return (char *) remove_trailing_whitespace (start, (const char *) s);
}

/* A helper function to extract an argument from *ARG.  An argument is
   delimited by whitespace.  The return value is empty if no argument
   was found.  */

extern std::string extract_arg (char **arg);

/* A const-correct version of the above.  */

extern std::string extract_arg (const char **arg);

/* A helper function that looks for an argument at the start of a
   string.  The argument must also either be at the end of the string,
   or be followed by whitespace.  Returns 1 if it finds the argument,
   0 otherwise.  If the argument is found, it updates *STR to point
   past the argument and past any whitespace following the
   argument.  */
extern int check_for_argument (const char **str, const char *arg, int arg_len);

/* Same as above, but ARG's length is computed.  */

static inline int
check_for_argument (const char **str, const char *arg)
{
  return check_for_argument (str, arg, strlen (arg));
}

/* Same, for non-const STR.  */

static inline int
check_for_argument (char **str, const char *arg, int arg_len)
{
  return check_for_argument (const_cast<const char **> (str),
			     arg, arg_len);
}

static inline int
check_for_argument (char **str, const char *arg)
{
  return check_for_argument (str, arg, strlen (arg));
}

/* qcs_flags struct groups the -q, -c, and -s flags parsed by "thread
   apply" and "frame apply" commands */

struct qcs_flags
{
  bool quiet = false;
  bool cont = false;
  bool silent = false;
};

/* Validate FLAGS.  Throws an error if both FLAGS->CONT and
   FLAGS->SILENT are true.  WHICH_COMMAND is included in the error
   message.  */
extern void validate_flags_qcs (const char *which_command, qcs_flags *flags);

#endif /* CLI_CLI_UTILS_H */
