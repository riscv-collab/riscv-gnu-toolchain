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

#include "defs.h"
#include "tid-parse.h"
#include "inferior.h"
#include "gdbthread.h"
#include <ctype.h>

/* See tid-parse.h.  */

void ATTRIBUTE_NORETURN
invalid_thread_id_error (const char *string)
{
  error (_("Invalid thread ID: %s"), string);
}

/* Wrapper for get_number_trailer that throws an error if we get back
   a negative number.  We'll see a negative value if the number is
   stored in a negative convenience variable (e.g., $minus_one = -1).
   STRING is the parser string to be used in the error message if we
   do get back a negative number.  */

static int
get_positive_number_trailer (const char **pp, int trailer, const char *string)
{
  int num;

  num = get_number_trailer (pp, trailer);
  if (num < 0)
    error (_("negative value: %s"), string);
  return num;
}

/* See tid-parse.h.  */

struct thread_info *
parse_thread_id (const char *tidstr, const char **end)
{
  const char *number = tidstr;
  const char *dot, *p1;
  struct inferior *inf;
  int thr_num;
  int explicit_inf_id = 0;

  dot = strchr (number, '.');

  if (dot != NULL)
    {
      /* Parse number to the left of the dot.  */
      int inf_num;

      p1 = number;
      inf_num = get_positive_number_trailer (&p1, '.', number);
      if (inf_num == 0)
	invalid_thread_id_error (number);

      inf = find_inferior_id (inf_num);
      if (inf == NULL)
	error (_("No inferior number '%d'"), inf_num);

      explicit_inf_id = 1;
      p1 = dot + 1;
    }
  else
    {
      inf = current_inferior ();

      p1 = number;
    }

  thr_num = get_positive_number_trailer (&p1, 0, number);
  if (thr_num == 0)
    invalid_thread_id_error (number);

  thread_info *tp = nullptr;
  for (thread_info *it : inf->threads ())
    if (it->per_inf_num == thr_num)
      {
	tp = it;
	break;
      }

  if (tp == NULL)
    {
      if (show_inferior_qualified_tids () || explicit_inf_id)
	error (_("Unknown thread %d.%d."), inf->num, thr_num);
      else
	error (_("Unknown thread %d."), thr_num);
    }

  if (end != NULL)
    *end = p1;

  return tp;
}

/* See tid-parse.h.  */

tid_range_parser::tid_range_parser (const char *tidlist,
				    int default_inferior)
{
  init (tidlist, default_inferior);
}

/* See tid-parse.h.  */

void
tid_range_parser::init (const char *tidlist, int default_inferior)
{
  m_state = STATE_INFERIOR;
  m_cur_tok = tidlist;
  m_inf_num = 0;
  m_qualified = false;
  m_default_inferior = default_inferior;
}

/* See tid-parse.h.  */

bool
tid_range_parser::finished () const
{
  switch (m_state)
    {
    case STATE_INFERIOR:
      /* Parsing is finished when at end of string or null string,
	 or we are not in a range and not in front of an integer, negative
	 integer, convenience var or negative convenience var.  */
      return (*m_cur_tok == '\0'
	      || !(isdigit (*m_cur_tok)
		   || *m_cur_tok == '$'
		   || *m_cur_tok == '*'));
    case STATE_THREAD_RANGE:
    case STATE_STAR_RANGE:
      return m_range_parser.finished ();
    }

  gdb_assert_not_reached ("unhandled state");
}

/* See tid-parse.h.  */

const char *
tid_range_parser::cur_tok () const
{
  switch (m_state)
    {
    case STATE_INFERIOR:
      return m_cur_tok;
    case STATE_THREAD_RANGE:
    case STATE_STAR_RANGE:
      return m_range_parser.cur_tok ();
    }

  gdb_assert_not_reached ("unhandled state");
}

void
tid_range_parser::skip_range ()
{
  gdb_assert (m_state == STATE_THREAD_RANGE
	      || m_state == STATE_STAR_RANGE);

  m_range_parser.skip_range ();
  init (m_range_parser.cur_tok (), m_default_inferior);
}

/* See tid-parse.h.  */

bool
tid_range_parser::tid_is_qualified () const
{
  return m_qualified;
}

/* Helper for tid_range_parser::get_tid and
   tid_range_parser::get_tid_range.  Return the next range if THR_END
   is non-NULL, return a single thread ID otherwise.  */

bool
tid_range_parser::get_tid_or_range (int *inf_num,
				    int *thr_start, int *thr_end)
{
  if (m_state == STATE_INFERIOR)
    {
      const char *p;
      const char *space;

      space = skip_to_space (m_cur_tok);

      p = m_cur_tok;
      while (p < space && *p != '.')
	p++;
      if (p < space)
	{
	  const char *dot = p;

	  /* Parse number to the left of the dot.  */
	  p = m_cur_tok;
	  m_inf_num = get_positive_number_trailer (&p, '.', m_cur_tok);
	  if (m_inf_num == 0)
	    return 0;

	  m_qualified = true;
	  p = dot + 1;

	  if (isspace (*p))
	    return false;
	}
      else
	{
	  m_inf_num = m_default_inferior;
	  m_qualified = false;
	  p = m_cur_tok;
	}

      m_range_parser.init (p);
      if (p[0] == '*' && (p[1] == '\0' || isspace (p[1])))
	{
	  /* Setup the number range parser to return numbers in the
	     whole [1,INT_MAX] range.  */
	  m_range_parser.setup_range (1, INT_MAX, skip_spaces (p + 1));
	  m_state = STATE_STAR_RANGE;
	}
      else
	m_state = STATE_THREAD_RANGE;
    }

  *inf_num = m_inf_num;
  *thr_start = m_range_parser.get_number ();
  if (*thr_start < 0)
    error (_("negative value: %s"), m_cur_tok);
  if (*thr_start == 0)
    {
      m_state = STATE_INFERIOR;
      return false;
    }

  /* If we successfully parsed a thread number or finished parsing a
     thread range, switch back to assuming the next TID is
     inferior-qualified.  */
  if (!m_range_parser.in_range ())
    {
      m_state = STATE_INFERIOR;
      m_cur_tok = m_range_parser.cur_tok ();

      if (thr_end != NULL)
	*thr_end = *thr_start;
    }

  /* If we're midway through a range, and the caller wants the end
     value, return it and skip to the end of the range.  */
  if (thr_end != NULL
      && (m_state == STATE_THREAD_RANGE
	  || m_state == STATE_STAR_RANGE))
    {
      *thr_end = m_range_parser.end_value ();

      skip_range ();
    }

  return (*inf_num != 0 && *thr_start != 0);
}

/* See tid-parse.h.  */

bool
tid_range_parser::get_tid_range (int *inf_num,
				 int *thr_start, int *thr_end)
{
  gdb_assert (inf_num != NULL && thr_start != NULL && thr_end != NULL);

  return get_tid_or_range (inf_num, thr_start, thr_end);
}

/* See tid-parse.h.  */

bool
tid_range_parser::get_tid (int *inf_num, int *thr_num)
{
  gdb_assert (inf_num != NULL && thr_num != NULL);

  return get_tid_or_range (inf_num, thr_num, NULL);
}

/* See tid-parse.h.  */

bool
tid_range_parser::in_star_range () const
{
  return m_state == STATE_STAR_RANGE;
}

bool
tid_range_parser::in_thread_range () const
{
  return m_state == STATE_THREAD_RANGE;
}

/* See tid-parse.h.  */

int
tid_is_in_list (const char *list, int default_inferior,
		int inf_num, int thr_num)
{
  if (list == NULL || *list == '\0')
    return 1;

  tid_range_parser parser (list, default_inferior);
  if (parser.finished ())
    invalid_thread_id_error (parser.cur_tok ());
  while (!parser.finished ())
    {
      int tmp_inf, tmp_thr_start, tmp_thr_end;

      if (!parser.get_tid_range (&tmp_inf, &tmp_thr_start, &tmp_thr_end))
	invalid_thread_id_error (parser.cur_tok ());
      if (tmp_inf == inf_num
	  && tmp_thr_start <= thr_num && thr_num <= tmp_thr_end)
	return 1;
    }
  return 0;
}
