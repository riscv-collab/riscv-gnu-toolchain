/* Styling for ui_file
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
#include "ui-style.h"
#include "gdbsupport/gdb_regex.h"

/* A regular expression that is used for matching ANSI terminal escape
   sequences.  */

static const char ansi_regex_text[] =
  /* Introduction.  */
  "^\033\\["
#define DATA_SUBEXP 1
  /* Capture parameter and intermediate bytes.  */
  "("
  /* Parameter bytes.  */
  "[\x30-\x3f]*"
  /* Intermediate bytes.  */
  "[\x20-\x2f]*"
  /* End the first capture.  */
  ")"
  /* The final byte.  */
#define FINAL_SUBEXP 2
  "([\x40-\x7e])";

/* The number of subexpressions to allocate space for, including the
   "0th" whole match subexpression.  */
#define NUM_SUBEXPRESSIONS 3

/* The compiled form of ansi_regex_text.  */

static regex_t ansi_regex;

/* This maps bright colors to RGB triples.  The index is the bright
   color index, starting with bright black.  The values come from
   xterm.  */

static const uint8_t bright_colors[][3] = {
  { 127, 127, 127 },		/* Black.  */
  { 255, 0, 0 },		/* Red.  */
  { 0, 255, 0 },		/* Green.  */
  { 255, 255, 0 },		/* Yellow.  */
  { 92, 92, 255 },		/* Blue.  */
  { 255, 0, 255 },		/* Magenta.  */
  { 0, 255, 255 },		/* Cyan.  */
  { 255, 255, 255 }		/* White.  */
};

/* See ui-style.h.  */

bool
ui_file_style::color::append_ansi (bool is_fg, std::string *str) const
{
  if (m_simple)
    {
      if (m_value >= BLACK && m_value <= WHITE)
	str->append (std::to_string (m_value + (is_fg ? 30 : 40)));
      else if (m_value > WHITE && m_value <= WHITE + 8)
	str->append (std::to_string (m_value - WHITE + (is_fg ? 90 : 100)));
      else if (m_value != -1)
	{
	  str->append (is_fg ? "38;5;" : "48;5;");
	  str->append (std::to_string (m_value));
	}
      else
	return false;
    }
  else
    {
      str->append (is_fg ? "38;2;" : "48;2;");
      str->append (std::to_string (m_red)
		   + ";" + std::to_string (m_green)
		   + ";" + std::to_string (m_blue));
    }
  return true;
}

/* See ui-style.h.  */

void
ui_file_style::color::get_rgb (uint8_t *rgb) const
{
  if (m_simple)
    {
      /* Can't call this for a basic color or NONE -- those will end
	 up in the assert below.  */
      if (m_value >= 8 && m_value <= 15)
	memcpy (rgb, bright_colors[m_value - 8], 3 * sizeof (uint8_t));
      else if (m_value >= 16 && m_value <= 231)
	{
	  int value = m_value;
	  value -= 16;
	  /* This obscure formula seems to be what terminals actually
	     do.  */
	  int component = value / 36;
	  rgb[0] = component == 0 ? 0 : (55 + component * 40);
	  value %= 36;
	  component = value / 6;
	  rgb[1] = component == 0 ? 0 : (55 + component * 40);
	  value %= 6;
	  rgb[2] = value == 0 ? 0 : (55 + value * 40);
	}
      else if (m_value >= 232)
	{
	  uint8_t v = (m_value - 232) * 10 + 8;
	  rgb[0] = v;
	  rgb[1] = v;
	  rgb[2] = v;
	}
      else
	gdb_assert_not_reached ("get_rgb called on invalid color");
    }
  else
    {
      rgb[0] = m_red;
      rgb[1] = m_green;
      rgb[2] = m_blue;
    }
}

/* See ui-style.h.  */

std::string
ui_file_style::to_ansi () const
{
  std::string result ("\033[");
  bool need_semi = m_foreground.append_ansi (true, &result);
  if (!m_background.is_none ())
    {
      if (need_semi)
	result.push_back (';');
      m_background.append_ansi (false, &result);
      need_semi = true;
    }
  if (m_intensity != NORMAL)
    {
      if (need_semi)
	result.push_back (';');
      result.append (std::to_string (m_intensity));
      need_semi = true;
    }
  if (m_reverse)
    {
      if (need_semi)
	result.push_back (';');
      result.push_back ('7');
    }
  result.push_back ('m');
  return result;
}

/* Read a ";" and a number from STRING.  Return the number of
   characters read and put the number into *NUM.  */

static bool
read_semi_number (const char *string, regoff_t *idx, long *num)
{
  if (string[*idx] != ';')
    return false;
  ++*idx;
  if (string[*idx] < '0' || string[*idx] > '9')
    return false;
  char *tail;
  *num = strtol (string + *idx, &tail, 10);
  *idx = tail - string;
  return true;
}

/* A helper for ui_file_style::parse that reads an extended color
   sequence; that is, and 8- or 24- bit color.  */

static bool
extended_color (const char *str, regoff_t *idx, ui_file_style::color *color)
{
  long value;

  if (!read_semi_number (str, idx, &value))
    return false;

  if (value == 5)
    {
      /* 8-bit color.  */
      if (!read_semi_number (str, idx, &value))
	return false;

      if (value >= 0 && value <= 255)
	*color = ui_file_style::color (value);
      else
	return false;
    }
  else if (value == 2)
    {
      /* 24-bit color.  */
      long r, g, b;
      if (!read_semi_number (str, idx, &r)
	  || r > 255
	  || !read_semi_number (str, idx, &g)
	  || g > 255
	  || !read_semi_number (str, idx, &b)
	  || b > 255)
	return false;
      *color = ui_file_style::color (r, g, b);
    }
  else
    {
      /* Unrecognized sequence.  */
      return false;
    }

  return true;
}

/* See ui-style.h.  */

bool
ui_file_style::parse (const char *buf, size_t *n_read)
{
  regmatch_t subexps[NUM_SUBEXPRESSIONS];

  int match = regexec (&ansi_regex, buf, ARRAY_SIZE (subexps), subexps, 0);
  if (match == REG_NOMATCH)
    {
      *n_read = 0;
      return false;
    }
  /* Other failures mean the regexp is broken.  */
  gdb_assert (match == 0);
  /* The regexp is anchored.  */
  gdb_assert (subexps[0].rm_so == 0);
  /* The final character exists.  */
  gdb_assert (subexps[FINAL_SUBEXP].rm_eo - subexps[FINAL_SUBEXP].rm_so == 1);

  if (buf[subexps[FINAL_SUBEXP].rm_so] != 'm')
    {
      /* We don't handle this sequence, so just drop it.  */
      *n_read = subexps[0].rm_eo;
      return false;
    }

  /* Examine each setting in the match and apply it to the result.
     See the Select Graphic Rendition section of
     https://en.wikipedia.org/wiki/ANSI_escape_code.  In essence each
     code is just a number, separated by ";"; there are some more
     wrinkles but we don't support them all..  */

  /* "\033[m" means the same thing as "\033[0m", so handle that
     specially here.  */
  if (subexps[DATA_SUBEXP].rm_so == subexps[DATA_SUBEXP].rm_eo)
    *this = ui_file_style ();

  for (regoff_t i = subexps[DATA_SUBEXP].rm_so;
       i < subexps[DATA_SUBEXP].rm_eo;
       ++i)
    {
      if (buf[i] == ';')
	{
	  /* Skip.  */
	}
      else if (buf[i] >= '0' && buf[i] <= '9')
	{
	  char *tail;
	  long value = strtol (buf + i, &tail, 10);
	  i = tail - buf;

	  switch (value)
	    {
	    case 0:
	      /* Reset.  */
	      *this = ui_file_style ();
	      break;
	    case 1:
	      /* Bold.  */
	      m_intensity = BOLD;
	      break;
	    case 2:
	      /* Dim.  */
	      m_intensity = DIM;
	      break;
	    case 7:
	      /* Reverse.  */
	      m_reverse = true;
	      break;
	    case 21:
	      m_intensity = NORMAL;
	      break;
	    case 22:
	      /* Normal.  */
	      m_intensity = NORMAL;
	      break;
	    case 27:
	      /* Inverse off.  */
	      m_reverse = false;
	      break;

	    case 30:
	    case 31:
	    case 32:
	    case 33:
	    case 34:
	    case 35:
	    case 36:
	    case 37:
	      /* Note: not 38.  */
	    case 39:
	      m_foreground = color (value - 30);
	      break;

	    case 40:
	    case 41:
	    case 42:
	    case 43:
	    case 44:
	    case 45:
	    case 46:
	    case 47:
	      /* Note: not 48.  */
	    case 49:
	      m_background = color (value - 40);
	      break;

	    case 90:
	    case 91:
	    case 92:
	    case 93:
	    case 94:
	    case 95:
	    case 96:
	    case 97:
	      m_foreground = color (value - 90 + 8);
	      break;

	    case 100:
	    case 101:
	    case 102:
	    case 103:
	    case 104:
	    case 105:
	    case 106:
	    case 107:
	      m_background = color (value - 100 + 8);
	      break;

	    case 38:
	      /* If we can't parse the extended color, fail.  */
	      if (!extended_color (buf, &i, &m_foreground))
		{
		  *n_read = subexps[0].rm_eo;
		  return false;
		}
	      break;

	    case 48:
	      /* If we can't parse the extended color, fail.  */
	      if (!extended_color (buf, &i, &m_background))
		{
		  *n_read = subexps[0].rm_eo;
		  return false;
		}
	      break;

	    default:
	      /* Ignore everything else.  */
	      break;
	    }
	}
      else
	{
	  /* Unknown, let's just ignore.  */
	}
    }

  *n_read = subexps[0].rm_eo;
  return true;
}

/* See ui-style.h.  */

bool
skip_ansi_escape (const char *buf, int *n_read)
{
  regmatch_t subexps[NUM_SUBEXPRESSIONS];

  int match = regexec (&ansi_regex, buf, ARRAY_SIZE (subexps), subexps, 0);
  if (match == REG_NOMATCH || buf[subexps[FINAL_SUBEXP].rm_so] != 'm')
    return false;

  *n_read = subexps[FINAL_SUBEXP].rm_eo;
  return true;
}

void _initialize_ui_style ();
void
_initialize_ui_style ()
{
  int code = regcomp (&ansi_regex, ansi_regex_text, REG_EXTENDED);
  /* If the regular expression was incorrect, it was a programming
     error.  */
  gdb_assert (code == 0);
}
