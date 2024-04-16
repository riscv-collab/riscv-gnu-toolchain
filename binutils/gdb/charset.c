/* Character set conversion support for GDB.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "charset.h"
#include "gdbcmd.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbsupport/gdb_wait.h"
#include "charset-list.h"
#include "gdbsupport/environ.h"
#include "arch-utils.h"
#include "gdbsupport/gdb_vecs.h"
#include <ctype.h>

#ifdef USE_WIN32API
#include <windows.h>
#endif

/* How GDB's character set support works

   GDB has three global settings:

   - The `current host character set' is the character set GDB should
     use in talking to the user, and which (hopefully) the user's
     terminal knows how to display properly.  Most users should not
     change this.

   - The `current target character set' is the character set the
     program being debugged uses.

   - The `current target wide character set' is the wide character set
     the program being debugged uses, that is, the encoding used for
     wchar_t.

   There are commands to set each of these, and mechanisms for
   choosing reasonable default values.  GDB has a global list of
   character sets that it can use as its host or target character
   sets.

   The header file `charset.h' declares various functions that
   different pieces of GDB need to perform tasks like:

   - printing target strings and characters to the user's terminal
     (mostly target->host conversions),

   - building target-appropriate representations of strings and
     characters the user enters in expressions (mostly host->target
     conversions),

     and so on.
     
   To avoid excessive code duplication and maintenance efforts,
   GDB simply requires a capable iconv function.  Users on platforms
   without a suitable iconv can use the GNU iconv library.  */


#ifdef PHONY_ICONV

/* Provide a phony iconv that does as little as possible.  Also,
   arrange for there to be a single available character set.  */

#undef GDB_DEFAULT_HOST_CHARSET
#ifdef USE_WIN32API
# define GDB_DEFAULT_HOST_CHARSET "CP1252"
#else
# define GDB_DEFAULT_HOST_CHARSET "ISO-8859-1"
#endif
#define GDB_DEFAULT_TARGET_CHARSET GDB_DEFAULT_HOST_CHARSET 
#define GDB_DEFAULT_TARGET_WIDE_CHARSET "UTF-32"
#undef DEFAULT_CHARSET_NAMES
#define DEFAULT_CHARSET_NAMES GDB_DEFAULT_HOST_CHARSET ,

#undef iconv_t
#define iconv_t int
#undef iconv_open
#define iconv_open phony_iconv_open
#undef iconv
#define iconv phony_iconv
#undef iconv_close
#define iconv_close phony_iconv_close

#undef ICONV_CONST
#define ICONV_CONST const

/* We allow conversions from UTF-32, wchar_t, and the host charset.
   We allow conversions to wchar_t and the host charset.
   Return 1 if we are converting from UTF-32BE, 2 if from UTF32-LE,
   0 otherwise.  This is used as a flag in calls to iconv.  */

static iconv_t
phony_iconv_open (const char *to, const char *from)
{
  if (strcmp (to, "wchar_t") && strcmp (to, GDB_DEFAULT_HOST_CHARSET))
    return -1;

  if (!strcmp (from, "UTF-32BE") || !strcmp (from, "UTF-32"))
    return 1;

  if (!strcmp (from, "UTF-32LE"))
    return 2;

  if (strcmp (from, "wchar_t") && strcmp (from, GDB_DEFAULT_HOST_CHARSET))
    return -1;

  return 0;
}

static int
phony_iconv_close (iconv_t arg)
{
  return 0;
}

static size_t
phony_iconv (iconv_t utf_flag, const char **inbuf, size_t *inbytesleft,
	     char **outbuf, size_t *outbytesleft)
{
  if (utf_flag)
    {
      enum bfd_endian endian
	= utf_flag == 1 ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;
      while (*inbytesleft >= 4)
	{
	  unsigned long c
	    = extract_unsigned_integer ((const gdb_byte *)*inbuf, 4, endian);

	  if (c >= 256)
	    {
	      errno = EILSEQ;
	      return -1;
	    }
	  if (*outbytesleft < 1)
	    {
	      errno = E2BIG;
	      return -1;
	    }
	  **outbuf = c & 0xff;
	  ++*outbuf;
	  --*outbytesleft;

	  *inbuf += 4;
	  *inbytesleft -= 4;
	}
      if (*inbytesleft)
	{
	  /* Partial sequence on input.  */
	  errno = EINVAL;
	  return -1;
	}
    }
  else
    {
      /* In all other cases we simply copy input bytes to the
	 output.  */
      size_t amt = *inbytesleft;

      if (amt > *outbytesleft)
	amt = *outbytesleft;
      memcpy (*outbuf, *inbuf, amt);
      *inbuf += amt;
      *outbuf += amt;
      *inbytesleft -= amt;
      *outbytesleft -= amt;
      if (*inbytesleft)
	{
	  errno = E2BIG;
	  return -1;
	}
    }

  /* The number of non-reversible conversions -- but they were all
     reversible.  */
  return 0;
}

#else /* PHONY_ICONV */

/* On systems that don't have EILSEQ, GNU iconv's iconv.h defines it
   to ENOENT, while gnulib defines it to a different value.  Always
   map ENOENT to gnulib's EILSEQ, leaving callers agnostic.  */

static size_t
gdb_iconv (iconv_t utf_flag, ICONV_CONST char **inbuf, size_t *inbytesleft,
	   char **outbuf, size_t *outbytesleft)
{
  size_t ret;

  ret = iconv (utf_flag, inbuf, inbytesleft, outbuf, outbytesleft);
  if (errno == ENOENT)
    errno = EILSEQ;
  return ret;
}

#undef iconv
#define iconv gdb_iconv

#endif /* PHONY_ICONV */


/* The global lists of character sets and translations.  */


#ifndef GDB_DEFAULT_TARGET_CHARSET
#define GDB_DEFAULT_TARGET_CHARSET "ISO-8859-1"
#endif

#ifndef GDB_DEFAULT_TARGET_WIDE_CHARSET
#define GDB_DEFAULT_TARGET_WIDE_CHARSET "UTF-32"
#endif

static const char *auto_host_charset_name = GDB_DEFAULT_HOST_CHARSET;
static const char *host_charset_name = "auto";
static void
show_host_charset_name (struct ui_file *file, int from_tty,
			struct cmd_list_element *c,
			const char *value)
{
  if (!strcmp (value, "auto"))
    gdb_printf (file,
		_("The host character set is \"auto; currently %s\".\n"),
		auto_host_charset_name);
  else
    gdb_printf (file, _("The host character set is \"%s\".\n"), value);
}

static const char *target_charset_name = "auto";
static void
show_target_charset_name (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  if (!strcmp (value, "auto"))
    gdb_printf (file,
		_("The target character set is \"auto; "
		  "currently %s\".\n"),
		gdbarch_auto_charset (get_current_arch ()));
  else
    gdb_printf (file, _("The target character set is \"%s\".\n"),
		value);
}

static const char *target_wide_charset_name = "auto";
static void
show_target_wide_charset_name (struct ui_file *file, 
			       int from_tty,
			       struct cmd_list_element *c, 
			       const char *value)
{
  if (!strcmp (value, "auto"))
    gdb_printf (file,
		_("The target wide character set is \"auto; "
		  "currently %s\".\n"),
		gdbarch_auto_wide_charset (get_current_arch ()));
  else
    gdb_printf (file, _("The target wide character set is \"%s\".\n"),
		value);
}

static const char * const default_charset_names[] =
{
  DEFAULT_CHARSET_NAMES
  0
};

static const char * const *charset_enum;


/* If the target wide character set has big- or little-endian
   variants, these are the corresponding names.  */
static const char *target_wide_charset_be_name;
static const char *target_wide_charset_le_name;

/* The architecture for which the BE- and LE-names are valid.  */
static struct gdbarch *be_le_arch;

/* A helper function which sets the target wide big- and little-endian
   character set names, if possible.  */

static void
set_be_le_names (struct gdbarch *gdbarch)
{
  if (be_le_arch == gdbarch)
    return;
  be_le_arch = gdbarch;

#ifdef PHONY_ICONV
  /* Match the wide charset names recognized by phony_iconv_open.  */
  target_wide_charset_le_name = "UTF-32LE";
  target_wide_charset_be_name = "UTF-32BE";
#else
  int i, len;
  const char *target_wide;

  target_wide_charset_le_name = NULL;
  target_wide_charset_be_name = NULL;

  target_wide = target_wide_charset_name;
  if (!strcmp (target_wide, "auto"))
    target_wide = gdbarch_auto_wide_charset (gdbarch);

  len = strlen (target_wide);
  for (i = 0; charset_enum[i]; ++i)
    {
      if (strncmp (target_wide, charset_enum[i], len))
	continue;
      if ((charset_enum[i][len] == 'B'
	   || charset_enum[i][len] == 'L')
	  && charset_enum[i][len + 1] == 'E'
	  && charset_enum[i][len + 2] == '\0')
	{
	  if (charset_enum[i][len] == 'B')
	    target_wide_charset_be_name = charset_enum[i];
	  else
	    target_wide_charset_le_name = charset_enum[i];
	}
    }
# endif  /* PHONY_ICONV */
}

/* 'Set charset', 'set host-charset', 'set target-charset', 'set
   target-wide-charset', 'set charset' sfunc's.  */

static void
validate (struct gdbarch *gdbarch)
{
  iconv_t desc;
  const char *host_cset = host_charset ();
  const char *target_cset = target_charset (gdbarch);
  const char *target_wide_cset = target_wide_charset_name;

  if (!strcmp (target_wide_cset, "auto"))
    target_wide_cset = gdbarch_auto_wide_charset (gdbarch);

  desc = iconv_open (target_wide_cset, host_cset);
  if (desc == (iconv_t) -1)
    error (_("Cannot convert between character sets `%s' and `%s'"),
	   target_wide_cset, host_cset);
  iconv_close (desc);

  desc = iconv_open (target_cset, host_cset);
  if (desc == (iconv_t) -1)
    error (_("Cannot convert between character sets `%s' and `%s'"),
	   target_cset, host_cset);
  iconv_close (desc);

  /* Clear the cache.  */
  be_le_arch = NULL;
}

/* This is the sfunc for the 'set charset' command.  */
static void
set_charset_sfunc (const char *charset, int from_tty, 
		   struct cmd_list_element *c)
{
  /* CAREFUL: set the target charset here as well.  */
  target_charset_name = host_charset_name;
  validate (get_current_arch ());
}

/* 'set host-charset' command sfunc.  We need a wrapper here because
   the function needs to have a specific signature.  */
static void
set_host_charset_sfunc (const char *charset, int from_tty,
			struct cmd_list_element *c)
{
  validate (get_current_arch ());
}

/* Wrapper for the 'set target-charset' command.  */
static void
set_target_charset_sfunc (const char *charset, int from_tty,
			  struct cmd_list_element *c)
{
  validate (get_current_arch ());
}

/* Wrapper for the 'set target-wide-charset' command.  */
static void
set_target_wide_charset_sfunc (const char *charset, int from_tty,
			       struct cmd_list_element *c)
{
  validate (get_current_arch ());
}

/* sfunc for the 'show charset' command.  */
static void
show_charset (struct ui_file *file, int from_tty, 
	      struct cmd_list_element *c,
	      const char *name)
{
  show_host_charset_name (file, from_tty, c, host_charset_name);
  show_target_charset_name (file, from_tty, c, target_charset_name);
  show_target_wide_charset_name (file, from_tty, c, 
				 target_wide_charset_name);
}


/* Accessor functions.  */

const char *
host_charset (void)
{
  if (!strcmp (host_charset_name, "auto"))
    return auto_host_charset_name;
  return host_charset_name;
}

const char *
target_charset (struct gdbarch *gdbarch)
{
  if (!strcmp (target_charset_name, "auto"))
    return gdbarch_auto_charset (gdbarch);
  return target_charset_name;
}

const char *
target_wide_charset (struct gdbarch *gdbarch)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  set_be_le_names (gdbarch);
  if (byte_order == BFD_ENDIAN_BIG)
    {
      if (target_wide_charset_be_name)
	return target_wide_charset_be_name;
    }
  else
    {
      if (target_wide_charset_le_name)
	return target_wide_charset_le_name;
    }

  if (!strcmp (target_wide_charset_name, "auto"))
    return gdbarch_auto_wide_charset (gdbarch);

  return target_wide_charset_name;
}


/* Host character set management.  For the time being, we assume that
   the host character set is some superset of ASCII.  */

char
host_letter_to_control_character (char c)
{
  if (c == '?')
    return 0177;
  return c & 0237;
}


/* Public character management functions.  */

class iconv_wrapper
{
public:

  iconv_wrapper (const char *to, const char *from)
  {
    m_desc = iconv_open (to, from);
    if (m_desc == (iconv_t) -1)
      perror_with_name (_("Converting character sets"));
  }

  ~iconv_wrapper ()
  {
    iconv_close (m_desc);
  }

  size_t convert (ICONV_CONST char **inp, size_t *inleft, char **outp,
		  size_t *outleft)
  {
    return iconv (m_desc, inp, inleft, outp, outleft);
  }

private:

  iconv_t m_desc;
};

void
convert_between_encodings (const char *from, const char *to,
			   const gdb_byte *bytes, unsigned int num_bytes,
			   int width, struct obstack *output,
			   enum transliterations translit)
{
  size_t inleft;
  ICONV_CONST char *inp;
  unsigned int space_request;

  /* Often, the host and target charsets will be the same.  */
  if (!strcmp (from, to))
    {
      obstack_grow (output, bytes, num_bytes);
      return;
    }

  iconv_wrapper desc (to, from);

  inleft = num_bytes;
  inp = (ICONV_CONST char *) bytes;

  space_request = num_bytes;

  while (inleft > 0)
    {
      char *outp;
      size_t outleft, r;
      int old_size;

      old_size = obstack_object_size (output);
      obstack_blank (output, space_request);

      outp = (char *) obstack_base (output) + old_size;
      outleft = space_request;

      r = desc.convert (&inp, &inleft, &outp, &outleft);

      /* Now make sure that the object on the obstack only includes
	 bytes we have converted.  */
      obstack_blank_fast (output, -(ssize_t) outleft);

      if (r == (size_t) -1)
	{
	  switch (errno)
	    {
	    case EILSEQ:
	      {
		int i;

		/* Invalid input sequence.  */
		if (translit == translit_none)
		  error (_("Could not convert character "
			   "to `%s' character set"), to);

		/* We emit escape sequence for the bytes, skip them,
		   and try again.  */
		for (i = 0; i < width; ++i)
		  {
		    char octal[5];

		    xsnprintf (octal, sizeof (octal), "\\%.3o", *inp & 0xff);
		    obstack_grow_str (output, octal);

		    ++inp;
		    --inleft;
		  }
	      }
	      break;

	    case E2BIG:
	      /* We ran out of space in the output buffer.  Make it
		 bigger next time around.  */
	      space_request *= 2;
	      break;

	    case EINVAL:
	      /* Incomplete input sequence.  FIXME: ought to report this
		 to the caller somehow.  */
	      inleft = 0;
	      break;

	    default:
	      perror_with_name (_("Internal error while "
				  "converting character sets"));
	    }
	}
    }
}



/* Create a new iterator.  */
wchar_iterator::wchar_iterator (const gdb_byte *input, size_t bytes, 
				const char *charset, size_t width)
: m_input (input),
  m_bytes (bytes),
  m_width (width),
  m_out (1)
{
  m_desc = iconv_open (INTERMEDIATE_ENCODING, charset);
  if (m_desc == (iconv_t) -1)
    perror_with_name (_("Converting character sets"));
}

wchar_iterator::~wchar_iterator ()
{
  if (m_desc != (iconv_t) -1)
    iconv_close (m_desc);
}

int
wchar_iterator::iterate (enum wchar_iterate_result *out_result,
			 gdb_wchar_t **out_chars,
			 const gdb_byte **ptr,
			 size_t *len)
{
  size_t out_request;

  /* Try to convert some characters.  At first we try to convert just
     a single character.  The reason for this is that iconv does not
     necessarily update its outgoing arguments when it encounters an
     invalid input sequence -- but we want to reliably report this to
     our caller so it can emit an escape sequence.  */
  out_request = 1;
  while (m_bytes > 0)
    {
      ICONV_CONST char *inptr = (ICONV_CONST char *) m_input;
      char *outptr = (char *) m_out.data ();
      const gdb_byte *orig_inptr = m_input;
      size_t orig_in = m_bytes;
      size_t out_avail = out_request * sizeof (gdb_wchar_t);
      size_t num;
      size_t r = iconv (m_desc, &inptr, &m_bytes, &outptr, &out_avail);

      m_input = (gdb_byte *) inptr;

      if (r == (size_t) -1)
	{
	  switch (errno)
	    {
	    case EILSEQ:
	      /* Invalid input sequence.  We still might have
		 converted a character; if so, return it.  */
	      if (out_avail < out_request * sizeof (gdb_wchar_t))
		break;
	      
	      /* Otherwise skip the first invalid character, and let
		 the caller know about it.  */
	      *out_result = wchar_iterate_invalid;
	      *ptr = m_input;
	      *len = m_width;
	      m_input += m_width;
	      m_bytes -= m_width;
	      return 0;

	    case E2BIG:
	      /* We ran out of space.  We still might have converted a
		 character; if so, return it.  Otherwise, grow the
		 buffer and try again.  */
	      if (out_avail < out_request * sizeof (gdb_wchar_t))
		break;

	      ++out_request;
	      if (out_request > m_out.size ())
		m_out.resize (out_request);
	      continue;

	    case EINVAL:
	      /* Incomplete input sequence.  Let the caller know, and
		 arrange for future calls to see EOF.  */
	      *out_result = wchar_iterate_incomplete;
	      *ptr = m_input;
	      *len = m_bytes;
	      m_bytes = 0;
	      return 0;

	    default:
	      perror_with_name (_("Internal error while "
				  "converting character sets"));
	    }
	}

      /* We converted something.  */
      num = out_request - out_avail / sizeof (gdb_wchar_t);
      *out_result = wchar_iterate_ok;
      *out_chars = m_out.data ();
      *ptr = orig_inptr;
      *len = orig_in - m_bytes;
      return num;
    }

  /* Really done.  */
  *out_result = wchar_iterate_eof;
  return -1;
}

struct charset_vector
{
  ~charset_vector ()
  {
    /* Note that we do not call charset_vector::clear, which would also xfree
       the elements.  This destructor is only called after exit, at which point
       those will be freed anyway on process exit, so not freeing them now is
       not classified as a memory leak.  OTOH, freeing them now might be
       classified as a data race, because some worker thread might still be
       accessing them.  */
    charsets.clear ();
  }

  void clear ()
  {
    for (char *c : charsets)
      xfree (c);

    charsets.clear ();
  }

  std::vector<char *> charsets;
};

static charset_vector charsets;

#ifdef PHONY_ICONV

static void
find_charset_names (void)
{
  charsets.charsets.push_back (xstrdup (GDB_DEFAULT_HOST_CHARSET));
  charsets.charsets.push_back (NULL);
}

#else /* PHONY_ICONV */

/* Sometimes, libiconv redefines iconvlist as libiconvlist -- but
   provides different symbols in the static and dynamic libraries.
   So, configure may see libiconvlist but not iconvlist.  But, calling
   iconvlist is the right thing to do and will work.  Hence we do a
   check here but unconditionally call iconvlist below.  */
#if defined (HAVE_ICONVLIST) || defined (HAVE_LIBICONVLIST)

/* A helper function that adds some character sets to the vector of
   all character sets.  This is a callback function for iconvlist.  */

static int
add_one (unsigned int count, const char *const *names, void *data)
{
  unsigned int i;

  for (i = 0; i < count; ++i)
    charsets.charsets.push_back (xstrdup (names[i]));

  return 0;
}

static void
find_charset_names (void)
{
  iconvlist (add_one, NULL);

  charsets.charsets.push_back (NULL);
}

#else

/* Return non-zero if LINE (output from iconv) should be ignored.
   Older iconv programs (e.g. 2.2.2) include the human readable
   introduction even when stdout is not a tty.  Newer versions omit
   the intro if stdout is not a tty.  */

static int
ignore_line_p (const char *line)
{
  /* This table is used to filter the output.  If this text appears
     anywhere in the line, it is ignored (strstr is used).  */
  static const char * const ignore_lines[] =
    {
      "The following",
      "not necessarily",
      "the FROM and TO",
      "listed with several",
      NULL
    };
  int i;

  for (i = 0; ignore_lines[i] != NULL; ++i)
    {
      if (strstr (line, ignore_lines[i]) != NULL)
	return 1;
    }

  return 0;
}

static void
find_charset_names (void)
{
  struct pex_obj *child;
  const char *args[3];
  int err, status;
  int fail = 1;
  int flags;
  gdb_environ iconv_env = gdb_environ::from_host_environ ();
  char *iconv_program;

  /* Older iconvs, e.g. 2.2.2, don't omit the intro text if stdout is
     not a tty.  We need to recognize it and ignore it.  This text is
     subject to translation, so force LANGUAGE=C.  */
  iconv_env.set ("LANGUAGE", "C");
  iconv_env.set ("LC_ALL", "C");

  child = pex_init (PEX_USE_PIPES, "iconv", NULL);

#ifdef ICONV_BIN
  {
    std::string iconv_dir = relocate_gdb_directory (ICONV_BIN,
						    ICONV_BIN_RELOCATABLE);
    iconv_program
      = concat (iconv_dir.c_str(), SLASH_STRING, "iconv", (char *) NULL);
  }
#else
  iconv_program = xstrdup ("iconv");
#endif
  args[0] = iconv_program;
  args[1] = "-l";
  args[2] = NULL;
  flags = PEX_STDERR_TO_STDOUT;
#ifndef ICONV_BIN
  flags |= PEX_SEARCH;
#endif
  /* Note that we simply ignore errors here.  */
  if (!pex_run_in_environment (child, flags,
			       args[0], const_cast<char **> (args),
			       iconv_env.envp (),
			       NULL, NULL, &err))
    {
      FILE *in = pex_read_output (child, 0);

      /* POSIX says that iconv -l uses an unspecified format.  We
	 parse the glibc and libiconv formats; feel free to add others
	 as needed.  */

      while (in != NULL && !feof (in))
	{
	  /* The size of buf is chosen arbitrarily.  */
	  char buf[1024];
	  char *start, *r;
	  int len;

	  r = fgets (buf, sizeof (buf), in);
	  if (!r)
	    break;
	  len = strlen (r);
	  if (len <= 3)
	    continue;
	  if (ignore_line_p (r))
	    continue;

	  /* Strip off the newline.  */
	  --len;
	  /* Strip off one or two '/'s.  glibc will print lines like
	     "8859_7//", but also "10646-1:1993/UCS4/".  */
	  if (buf[len - 1] == '/')
	    --len;
	  if (buf[len - 1] == '/')
	    --len;
	  buf[len] = '\0';

	  /* libiconv will print multiple entries per line, separated
	     by spaces.  Older iconvs will print multiple entries per
	     line, indented by two spaces, and separated by ", "
	     (i.e. the human readable form).  */
	  start = buf;
	  while (1)
	    {
	      int keep_going;
	      char *p;

	      /* Skip leading blanks.  */
	      for (p = start; *p && *p == ' '; ++p)
		;
	      start = p;
	      /* Find the next space, comma, or end-of-line.  */
	      for ( ; *p && *p != ' ' && *p != ','; ++p)
		;
	      /* Ignore an empty result.  */
	      if (p == start)
		break;
	      keep_going = *p;
	      *p = '\0';
	      charsets.charsets.push_back (xstrdup (start));
	      if (!keep_going)
		break;
	      /* Skip any extra spaces.  */
	      for (start = p + 1; *start && *start == ' '; ++start)
		;
	    }
	}

      if (pex_get_status (child, 1, &status)
	  && WIFEXITED (status) && !WEXITSTATUS (status))
	fail = 0;

    }

  xfree (iconv_program);
  pex_free (child);

  if (fail)
    {
      /* Some error occurred, so drop the vector.  */
      charsets.clear ();
    }
  else
    charsets.charsets.push_back (NULL);
}

#endif /* HAVE_ICONVLIST || HAVE_LIBICONVLIST */
#endif /* PHONY_ICONV */

/* The "auto" target charset used by default_auto_charset.  */
static const char *auto_target_charset_name = GDB_DEFAULT_TARGET_CHARSET;

const char *
default_auto_charset (void)
{
  return auto_target_charset_name;
}

const char *
default_auto_wide_charset (void)
{
  return GDB_DEFAULT_TARGET_WIDE_CHARSET;
}


#ifdef USE_INTERMEDIATE_ENCODING_FUNCTION
/* Macro used for UTF or UCS endianness suffix.  */
#if WORDS_BIGENDIAN
#define ENDIAN_SUFFIX "BE"
#else
#define ENDIAN_SUFFIX "LE"
#endif

/* GDB cannot handle strings correctly if this size is different.  */

static_assert (sizeof (gdb_wchar_t) == 2 || sizeof (gdb_wchar_t) == 4);

/* intermediate_encoding returns the charset used internally by
   GDB to convert between target and host encodings. As the test above
   compiled, sizeof (gdb_wchar_t) is either 2 or 4 bytes.
   UTF-16/32 is tested first, UCS-2/4 is tested as a second option,
   otherwise an error is generated.  */

const char *
intermediate_encoding (void)
{
  iconv_t desc;
  static const char *stored_result = NULL;
  gdb::unique_xmalloc_ptr<char> result;

  if (stored_result)
    return stored_result;
  result = xstrprintf ("UTF-%d%s", (int) (sizeof (gdb_wchar_t) * 8),
		       ENDIAN_SUFFIX);
  /* Check that the name is supported by iconv_open.  */
  desc = iconv_open (result.get (), host_charset ());
  if (desc != (iconv_t) -1)
    {
      iconv_close (desc);
      stored_result = result.release ();
      return stored_result;
    }
  /* Second try, with UCS-2 type.  */
  result = xstrprintf ("UCS-%d%s", (int) sizeof (gdb_wchar_t),
		       ENDIAN_SUFFIX);
  /* Check that the name is supported by iconv_open.  */
  desc = iconv_open (result.get (), host_charset ());
  if (desc != (iconv_t) -1)
    {
      iconv_close (desc);
      stored_result = result.release ();
      return stored_result;
    }
  /* No valid charset found, generate error here.  */
  error (_("Unable to find a valid charset for string conversions"));
}

#endif /* USE_INTERMEDIATE_ENCODING_FUNCTION */

void _initialize_charset ();
void
_initialize_charset ()
{
  /* The first element is always "auto".  */
  charsets.charsets.push_back (xstrdup ("auto"));
  find_charset_names ();

  if (charsets.charsets.size () > 1)
    charset_enum = (const char * const *) charsets.charsets.data ();
  else
    charset_enum = default_charset_names;

#ifndef PHONY_ICONV
#ifdef HAVE_LANGINFO_CODESET
  /* The result of nl_langinfo may be overwritten later.  This may
     leak a little memory, if the user later changes the host charset,
     but that doesn't matter much.  */
  auto_host_charset_name = xstrdup (nl_langinfo (CODESET));
  /* Solaris will return `646' here -- but the Solaris iconv then does
     not accept this.  Darwin (and maybe FreeBSD) may return "" here,
     which GNU libiconv doesn't like (infinite loop).  */
  if (!strcmp (auto_host_charset_name, "646") || !*auto_host_charset_name)
    auto_host_charset_name = "ASCII";
  auto_target_charset_name = auto_host_charset_name;
#elif defined (USE_WIN32API)
  {
    /* "CP" + x<=5 digits + paranoia.  */
    static char w32_host_default_charset[16];

    snprintf (w32_host_default_charset, sizeof w32_host_default_charset,
	      "CP%d", GetACP());
    auto_host_charset_name = w32_host_default_charset;
    auto_target_charset_name = auto_host_charset_name;
  }
#endif
#endif

  /* Recall that the first element is always "auto".  */
  host_charset_name = charset_enum[0];
  gdb_assert (strcmp (host_charset_name, "auto") == 0);
  add_setshow_enum_cmd ("charset", class_support,
			charset_enum, &host_charset_name, _("\
Set the host and target character sets."), _("\
Show the host and target character sets."), _("\
The `host character set' is the one used by the system GDB is running on.\n\
The `target character set' is the one used by the program being debugged.\n\
You may only use supersets of ASCII for your host character set; GDB does\n\
not support any others.\n\
To see a list of the character sets GDB supports, type `set charset <TAB>'."),
			/* Note that the sfunc below needs to set
			   target_charset_name, because the 'set
			   charset' command sets two variables.  */
			set_charset_sfunc,
			show_charset,
			&setlist, &showlist);

  add_setshow_enum_cmd ("host-charset", class_support,
			charset_enum, &host_charset_name, _("\
Set the host character set."), _("\
Show the host character set."), _("\
The `host character set' is the one used by the system GDB is running on.\n\
You may only use supersets of ASCII for your host character set; GDB does\n\
not support any others.\n\
To see a list of the character sets GDB supports, type `set host-charset <TAB>'."),
			set_host_charset_sfunc,
			show_host_charset_name,
			&setlist, &showlist);

  /* Recall that the first element is always "auto".  */
  target_charset_name = charset_enum[0];
  gdb_assert (strcmp (target_charset_name, "auto") == 0);
  add_setshow_enum_cmd ("target-charset", class_support,
			charset_enum, &target_charset_name, _("\
Set the target character set."), _("\
Show the target character set."), _("\
The `target character set' is the one used by the program being debugged.\n\
GDB translates characters and strings between the host and target\n\
character sets as needed.\n\
To see a list of the character sets GDB supports, type `set target-charset'<TAB>"),
			set_target_charset_sfunc,
			show_target_charset_name,
			&setlist, &showlist);

  /* Recall that the first element is always "auto".  */
  target_wide_charset_name = charset_enum[0];
  gdb_assert (strcmp (target_wide_charset_name, "auto") == 0);
  add_setshow_enum_cmd ("target-wide-charset", class_support,
			charset_enum, &target_wide_charset_name,
			_("\
Set the target wide character set."), _("\
Show the target wide character set."), _("\
The `target wide character set' is the one used by the program being debugged.\
\nIn particular it is the encoding used by `wchar_t'.\n\
GDB translates characters and strings between the host and target\n\
character sets as needed.\n\
To see a list of the character sets GDB supports, type\n\
`set target-wide-charset'<TAB>"),
			set_target_wide_charset_sfunc,
			show_target_wide_charset_name,
			&setlist, &showlist);
}
