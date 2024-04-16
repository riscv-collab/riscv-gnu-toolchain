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

#ifndef UI_STYLE_H
#define UI_STYLE_H

/* Styles that can be applied to a ui_file.  */
struct ui_file_style
{
  /* One of the basic colors that can be handled by ANSI
     terminals.  */
  enum basic_color
  {
    NONE = -1,
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
  };

  /* Representation of a terminal color.  */
  class color
  {
  public:

    color (basic_color c)
      : m_simple (true),
	m_value (c)
    {
    }

    color (int c)
      : m_simple (true),
	m_value (c)
    {
      gdb_assert (c >= -1 && c <= 255);
    }

    color (uint8_t r, uint8_t g, uint8_t b)
      : m_simple (false),
	m_red (r),
	m_green (g),
	m_blue (b)
    {
    }

    bool operator== (const color &other) const
    {
      if (m_simple != other.m_simple)
	return false;
      if (m_simple)
	return m_value == other.m_value;
      return (m_red == other.m_red && m_green == other.m_green
	      && m_blue == other.m_blue);
    }

    bool operator< (const color &other) const
    {
      if (m_simple != other.m_simple)
	return m_simple < other.m_simple;
      if (m_simple)
	return m_value < other.m_value;
      if (m_red < other.m_red)
	return true;
      if (m_red == other.m_red)
	{
	  if (m_green < other.m_green)
	    return true;
	  if (m_green == other.m_green)
	    return m_blue < other.m_blue;
	}
      return false;
    }

    /* Return true if this is the "NONE" color, false otherwise.  */
    bool is_none () const
    {
      return m_simple && m_value == NONE;
    }

    /* Return true if this is one of the basic colors, false
       otherwise.  */
    bool is_basic () const
    {
      return m_simple && m_value >= BLACK && m_value <= WHITE;
    }

    /* Return the value of a basic color.  */
    int get_value () const
    {
      gdb_assert (is_basic ());
      return m_value;
    }

    /* Fill in RGB with the red/green/blue values for this color.
       This may not be called for basic colors or for the "NONE"
       color.  */
    void get_rgb (uint8_t *rgb) const;

    /* Append the ANSI terminal escape sequence for this color to STR.
       IS_FG indicates whether this is a foreground or background
       color.  Returns true if any characters were written; returns
       false otherwise (which can only happen for the "NONE"
       color).  */
    bool append_ansi (bool is_fg, std::string *str) const;

  private:

    bool m_simple;
    union
    {
      int m_value;
      struct
      {
	uint8_t m_red, m_green, m_blue;
      };
    };
  };

  /* Intensity settings that are available.  */
  enum intensity
  {
    NORMAL = 0,
    BOLD,
    DIM
  };

  ui_file_style () = default;

  ui_file_style (color f, color b, intensity i = NORMAL)
    : m_foreground (f),
      m_background (b),
      m_intensity (i)
  {
  }

  bool operator== (const ui_file_style &other) const
  {
    return (m_foreground == other.m_foreground
	    && m_background == other.m_background
	    && m_intensity == other.m_intensity
	    && m_reverse == other.m_reverse);
  }

  bool operator!= (const ui_file_style &other) const
  {
    return !(*this == other);
  }

  /* Return the ANSI escape sequence for this style.  */
  std::string to_ansi () const;

  /* Return true if this style is the default style; false
     otherwise.  */
  bool is_default () const
  {
    return (m_foreground == NONE
	    && m_background == NONE
	    && m_intensity == NORMAL
	    && !m_reverse);
  }

  /* Return true if this style specified reverse display; false
     otherwise.  */
  bool is_reverse () const
  {
    return m_reverse;
  }

  /* Set/clear the reverse display flag.  */
  void set_reverse (bool reverse)
  {
    m_reverse = reverse;
  }

  /* Return the foreground color of this style.  */
  const color &get_foreground () const
  {
    return m_foreground;
  }

  /* Set the foreground color of this style.  */
  void set_fg (color c)
  {
    m_foreground = c;
  }

  /* Return the background color of this style.  */
  const color &get_background () const
  {
    return m_background;
  }

  /* Set the background color of this style.  */
  void set_bg (color c)
  {
    m_background = c;
  }

  /* Return the intensity of this style.  */
  intensity get_intensity () const
  {
    return m_intensity;
  }

  /* Parse an ANSI escape sequence in BUF, modifying this style.  BUF
     must begin with an ESC character.  Return true if an escape
     sequence was successfully parsed; false otherwise.  In either
     case, N_READ is updated to reflect the number of chars read from
     BUF.  */
  bool parse (const char *buf, size_t *n_read);

  /* We need this because we can't pass a reference via va_args.  */
  const ui_file_style *ptr () const
  {
    return this;
  }

private:

  color m_foreground = NONE;
  color m_background = NONE;
  intensity m_intensity = NORMAL;
  bool m_reverse = false;
};

/* Skip an ANSI escape sequence in BUF.  BUF must begin with an ESC
   character.  Return true if an escape sequence was successfully
   skipped; false otherwise.  If an escape sequence was skipped,
   N_READ is updated to reflect the number of chars read from BUF.  */

extern bool skip_ansi_escape (const char *buf, int *n_read);

#endif /* UI_STYLE_H */
