#!/usr/bin/env python3
#
# Copyright (C) 2013-2024 Free Software Foundation, Inc.
#
# This script is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.

# This script adjusts the copyright notices at the top of source files
# so that they have the form:
#
#   Copyright XXXX-YYYY Free Software Foundation, Inc.
#
# It doesn't change code that is known to be maintained elsewhere or
# that carries a non-FSF copyright.
#
# Pass --this-year to the script if you want it to add the current year
# to all applicable notices.  Pass --quilt if you are using quilt and
# want files to be added to the quilt before being changed.
#
# By default the script will update all directories for which the
# output has been vetted.  You can instead pass the names of individual
# directories, including those that haven't been approved.  So:
#
#    update-copyright.py --this-year
#
# is the command that would be used at the beginning of a year to update
# all copyright notices (and possibly at other times to check whether
# new files have been added with old years).  On the other hand:
#
#    update-copyright.py --this-year libiberty
#
# would run the script on just libiberty/.
#
# This script was copied from gcc's contrib/ and modified to suit
# binutils.  In contrast to the gcc script, this one will update
# the testsuite and --version output strings too.

import os
import re
import sys
import time
import subprocess

class Errors:
    def __init__ (self):
        self.num_errors = 0

    def report (self, filename, string):
        if filename:
            string = filename + ': ' + string
        sys.stderr.write (string + '\n')
        self.num_errors += 1

    def ok (self):
        return self.num_errors == 0

class GenericFilter:
    def __init__ (self):
        self.skip_files = set()
        self.skip_dirs = set()
        self.skip_extensions = set([
                '.png',
                '.pyc',
                ])
        self.fossilised_files = set()
        self.own_files = set()

        self.skip_files |= set ([
                # Skip licence files.
                'COPYING',
                'COPYING.LIB',
                'COPYING3',
                'COPYING3.LIB',
                'COPYING.LIBGLOSS',
                'COPYING.NEWLIB',
                'LICENSE',
                'fdl.texi',
                'gpl_v3.texi',
                'fdl-1.3.xml',
                'gpl-3.0.xml',

                # Skip auto- and libtool-related files
                'aclocal.m4',
                'compile',
                'config.guess',
                'config.sub',
                'depcomp',
                'install-sh',
                'libtool.m4',
                'ltmain.sh',
                'ltoptions.m4',
                'ltsugar.m4',
                'ltversion.m4',
                'lt~obsolete.m4',
                'missing',
                'mkdep',
                'mkinstalldirs',
                'move-if-change',
                'shlibpath.m4',
                'symlink-tree',
                'ylwrap',

                # Skip FSF mission statement, etc.
                'gnu.texi',
                'funding.texi',
                'appendix_free.xml',

                # Skip imported texinfo files.
                'texinfo.tex',
                ])

        self.skip_extensions |= set ([
                # Maintained by the translation project.
                '.po',

                # Automatically-generated.
                '.pot',
                ])

        self.skip_dirs |= set ([
                'autom4te.cache',
                ])


    def get_line_filter (self, dir, filename):
        if filename.startswith ('ChangeLog'):
            # Ignore references to copyright in changelog entries.
            return re.compile ('\t')

        return None

    def skip_file (self, dir, filename):
        if filename in self.skip_files:
            return True

        (base, extension) = os.path.splitext (os.path.join (dir, filename))
        if extension in self.skip_extensions:
            return True

        if extension == '.in':
            # Skip .in files produced by automake.
            if os.path.exists (base + '.am'):
                return True

            # Skip files produced by autogen
            if (os.path.exists (base + '.def')
                and os.path.exists (base + '.tpl')):
                return True

        # Skip configure files produced by autoconf
        if filename == 'configure':
            if os.path.exists (base + '.ac'):
                return True
            if os.path.exists (base + '.in'):
                return True

        return False

    def skip_dir (self, dir, subdir):
        return subdir in self.skip_dirs

    def is_fossilised_file (self, dir, filename):
        if filename in self.fossilised_files:
            return True
        # Only touch current current ChangeLogs.
        if filename != 'ChangeLog' and filename.find ('ChangeLog') >= 0:
            return True
        return False

    def by_package_author (self, dir, filename):
        return filename in self.own_files

class Copyright:
    def __init__ (self, errors):
        self.errors = errors

        # Characters in a range of years.  Include '.' for typos.
        ranges = '[0-9](?:[-0-9.,\s]|\s+and\s+)*[0-9]'

        # Non-whitespace characters in a copyright holder's name.
        name = '[\w.,-]'

        # Matches one year.
        self.year_re = re.compile ('[0-9]+')

        # Matches part of a year or copyright holder.
        self.continuation_re = re.compile (ranges + '|' + name)

        # Matches a full copyright notice:
        self.copyright_re = re.compile (
            # 1: 'Copyright (C)', etc.
            '([Cc]opyright'
            '|[Cc]opyright\s+\([Cc]\)'
            '|[Cc]opyright\s+%s'
            '|[Cc]opyright\s+&copy;'
            '|[Cc]opyright\s+@copyright{}'
            '|@set\s+copyright[\w-]+)'

            # 2: the years.  Include the whitespace in the year, so that
            # we can remove any excess.
            '(\s*(?:' + ranges + ',?'
            '|@value\{[^{}]*\})\s*)'

            # 3: 'by ', if used
            '(by\s+)?'

            # 4: the copyright holder.  Don't allow multiple consecutive
            # spaces, so that right-margin gloss doesn't get caught
            # (e.g. gnat_ugn.texi).
            '(' + name + '(?:\s?' + name + ')*)?')

        # A regexp for notices that might have slipped by.  Just matching
        # 'copyright' is too noisy, and 'copyright.*[0-9]' falls foul of
        # HTML header markers, so check for 'copyright' and two digits.
        self.other_copyright_re = re.compile ('(^|[^\._])copyright[^=]*[0-9][0-9]',
                                              re.IGNORECASE)
        self.comment_re = re.compile('#+|[*]+|;+|%+|//+|@c |dnl ')
        self.holders = { '@copying': '@copying' }
        self.holder_prefixes = set()

        # True to 'quilt add' files before changing them.
        self.use_quilt = False

        # If set, force all notices to include this year.
        self.max_year = None

        # Goes after the year(s).  Could be ', '.
        self.separator = ' '

    def add_package_author (self, holder, canon_form = None):
        if not canon_form:
            canon_form = holder
        self.holders[holder] = canon_form
        index = holder.find (' ')
        while index >= 0:
            self.holder_prefixes.add (holder[:index])
            index = holder.find (' ', index + 1)

    def add_external_author (self, holder):
        self.holders[holder] = None

    class BadYear (Exception):
        def __init__ (self, year):
            self.year = year

        def __str__ (self):
            return 'unrecognised year: ' + self.year

    def parse_year (self, string):
        year = int (string)
        if len (string) == 2:
            if year > 70:
                return year + 1900
        elif len (string) == 4:
            return year
        raise self.BadYear (string)

    def year_range (self, years):
        year_list = [self.parse_year (year)
                     for year in self.year_re.findall (years)]
        assert len (year_list) > 0
        return (min (year_list), max (year_list))

    def set_use_quilt (self, use_quilt):
        self.use_quilt = use_quilt

    def include_year (self, year):
        assert not self.max_year
        self.max_year = year

    def canonicalise_years (self, dir, filename, filter, years):
        # Leave texinfo variables alone.
        if years.startswith ('@value'):
            return years

        (min_year, max_year) = self.year_range (years)

        # Update the upper bound, if enabled.
        if self.max_year and not filter.is_fossilised_file (dir, filename):
            max_year = max (max_year, self.max_year)

        # Use a range.
        if min_year == max_year:
            return '%d' % min_year
        else:
            return '%d-%d' % (min_year, max_year)

    def strip_continuation (self, line):
        line = line.lstrip()
        match = self.comment_re.match (line)
        if match:
            line = line[match.end():].lstrip()
        return line

    def is_complete (self, match):
        holder = match.group (4)
        return (holder
                and (holder not in self.holder_prefixes
                     or holder in self.holders))

    def update_copyright (self, dir, filename, filter, file, line, match):
        orig_line = line
        next_line = None
        pathname = os.path.join (dir, filename)

        intro = match.group (1)
        if intro.startswith ('@set'):
            # Texinfo year variables should always be on one line
            after_years = line[match.end (2):].strip()
            if after_years != '':
                self.errors.report (pathname,
                                    'trailing characters in @set: '
                                    + after_years)
                return (False, orig_line, next_line)
        else:
            # If it looks like the copyright is incomplete, add the next line.
            while not self.is_complete (match):
                try:
                    next_line = file.readline()
                except StopIteration:
                    break

                # If the next line doesn't look like a proper continuation,
                # assume that what we've got is complete.
                continuation = self.strip_continuation (next_line)
                if not self.continuation_re.match (continuation):
                    break

                # Merge the lines for matching purposes.
                orig_line += next_line
                line = line.rstrip() + ' ' + continuation
                next_line = None

                # Rematch with the longer line, at the original position.
                match = self.copyright_re.match (line, match.start())
                assert match

            holder = match.group (4)

            # Use the filter to test cases where markup is getting in the way.
            if filter.by_package_author (dir, filename):
                assert holder not in self.holders

            elif not holder:
                self.errors.report (pathname, 'missing copyright holder')
                return (False, orig_line, next_line)

            elif holder not in self.holders:
                self.errors.report (pathname,
                                    'unrecognised copyright holder: ' + holder)
                return (False, orig_line, next_line)

            else:
                # See whether the copyright is associated with the package
                # author.
                canon_form = self.holders[holder]
                if not canon_form:
                    return (False, orig_line, next_line)

                # Make sure the author is given in a consistent way.
                line = (line[:match.start (4)]
                        + canon_form
                        + line[match.end (4):])

                # Remove any 'by'
                line = line[:match.start (3)] + line[match.end (3):]

        # Update the copyright years.
        years = match.group (2).strip()
        if (self.max_year
            and match.start(0) > 0 and line[match.start(0)-1] == '"'
            and not filter.is_fossilised_file (dir, filename)):
            # A printed copyright date consists of the current year
            canon_form = '%d' % self.max_year
        else:
            try:
                canon_form = self.canonicalise_years (dir, filename, filter, years)
            except self.BadYear as e:
                self.errors.report (pathname, str (e))
                return (False, orig_line, next_line)

        line = (line[:match.start (2)]
                + ' ' + canon_form + self.separator
                + line[match.end (2):])

        # Use the standard (C) form.
        if intro.endswith ('right'):
            intro += ' (C)'
        elif intro.endswith ('(c)'):
            intro = intro[:-3] + '(C)'
        line = line[:match.start (1)] + intro + line[match.end (1):]

        # Strip trailing whitespace
        line = line.rstrip() + '\n'

        return (line != orig_line, line, next_line)

    def guess_encoding (self, pathname):
        for encoding in ('utf8', 'iso8859'):
            try:
                open(pathname, 'r', encoding=encoding).read()
                return encoding
            except UnicodeDecodeError:
                pass
        return None

    def process_file (self, dir, filename, filter):
        pathname = os.path.join (dir, filename)
        if filename.endswith ('.tmp'):
            # Looks like something we tried to create before.
            try:
                os.remove (pathname)
            except OSError:
                pass
            return

        lines = []
        changed = False
        line_filter = filter.get_line_filter (dir, filename)
        mode = None
        encoding = self.guess_encoding(pathname)
        with open (pathname, 'r', encoding=encoding) as file:
            prev = None
            mode = os.fstat (file.fileno()).st_mode
            for line in file:
                while line:
                    next_line = None
                    # Leave filtered-out lines alone.
                    if not (line_filter and line_filter.match (line)):
                        match = self.copyright_re.search (line)
                        if match:
                            res = self.update_copyright (dir, filename, filter,
                                                         file, line, match)
                            (this_changed, line, next_line) = res
                            changed = changed or this_changed

                        # Check for copyright lines that might have slipped by.
                        elif self.other_copyright_re.search (line):
                            self.errors.report (pathname,
                                                'unrecognised copyright: %s'
                                                % line.strip())
                    lines.append (line)
                    line = next_line

        # If something changed, write the new file out.
        if changed and self.errors.ok():
            tmp_pathname = pathname + '.tmp'
            with open (tmp_pathname, 'w', encoding=encoding) as file:
                for line in lines:
                    file.write (line)
                os.fchmod (file.fileno(), mode)
            if self.use_quilt:
                subprocess.call (['quilt', 'add', pathname])
            os.rename (tmp_pathname, pathname)

    def process_tree (self, tree, filter):
        for (dir, subdirs, filenames) in os.walk (tree):
            # Don't recurse through directories that should be skipped.
            for i in range (len (subdirs) - 1, -1, -1):
                if filter.skip_dir (dir, subdirs[i]):
                    del subdirs[i]

            # Handle the files in this directory.
            for filename in filenames:
                if filter.skip_file (dir, filename):
                    sys.stdout.write ('Skipping %s\n'
                                      % os.path.join (dir, filename))
                else:
                    self.process_file (dir, filename, filter)

class CmdLine:
    def __init__ (self, copyright = Copyright):
        self.errors = Errors()
        self.copyright = copyright (self.errors)
        self.dirs = []
        self.default_dirs = []
        self.chosen_dirs = []
        self.option_handlers = dict()
        self.option_help = []

        self.add_option ('--help', 'Print this help', self.o_help)
        self.add_option ('--quilt', '"quilt add" files before changing them',
                         self.o_quilt)
        self.add_option ('--this-year', 'Add the current year to every notice',
                         self.o_this_year)

    def add_option (self, name, help, handler):
        self.option_help.append ((name, help))
        self.option_handlers[name] = handler

    def add_dir (self, dir, filter = GenericFilter()):
        self.dirs.append ((dir, filter))

    def o_help (self, option = None):
        sys.stdout.write ('Usage: %s [options] dir1 dir2...\n\n'
                          'Options:\n' % sys.argv[0])
        format = '%-15s %s\n'
        for (what, help) in self.option_help:
            sys.stdout.write (format % (what, help))
        sys.stdout.write ('\nDirectories:\n')

        format = '%-25s'
        i = 0
        for (dir, filter) in self.dirs:
            i += 1
            if i % 3 == 0 or i == len (self.dirs):
                sys.stdout.write (dir + '\n')
            else:
                sys.stdout.write (format % dir)
        sys.exit (0)

    def o_quilt (self, option):
        self.copyright.set_use_quilt (True)

    def o_this_year (self, option):
        self.copyright.include_year (time.localtime().tm_year)

    def main (self):
        for arg in sys.argv[1:]:
            if arg[:1] != '-':
                self.chosen_dirs.append (arg)
            elif arg in self.option_handlers:
                self.option_handlers[arg] (arg)
            else:
                self.errors.report (None, 'unrecognised option: ' + arg)
        if self.errors.ok():
            if len (self.chosen_dirs) == 0:
                self.chosen_dirs = self.default_dirs
            if len (self.chosen_dirs) == 0:
                self.o_help()
            else:
                for chosen_dir in self.chosen_dirs:
                    canon_dir = os.path.join (chosen_dir, '')
                    count = 0
                    for (dir, filter) in self.dirs:
                        if (dir + os.sep).startswith (canon_dir):
                            count += 1
                            self.copyright.process_tree (dir, filter)
                    if count == 0:
                        self.errors.report (None, 'unrecognised directory: '
                                            + chosen_dir)
        sys.exit (0 if self.errors.ok() else 1)

#----------------------------------------------------------------------------

class TopLevelFilter (GenericFilter):
    def skip_dir (self, dir, subdir):
        return True

class ConfigFilter (GenericFilter):
    def __init__ (self):
        GenericFilter.__init__ (self)

    def skip_file (self, dir, filename):
        if filename.endswith ('.m4'):
            pathname = os.path.join (dir, filename)
            with open (pathname) as file:
                # Skip files imported from gettext.
                if file.readline().find ('gettext-') >= 0:
                    return True
        return GenericFilter.skip_file (self, dir, filename)

class LdFilter (GenericFilter):
    def __init__ (self):
        GenericFilter.__init__ (self)

        self.skip_extensions |= set ([
                # ld testsuite output match files.
                '.ro',
                ])

class BinutilsCopyright (Copyright):
    def __init__ (self, errors):
        Copyright.__init__ (self, errors)

        canon_fsf = 'Free Software Foundation, Inc.'
        self.add_package_author ('Free Software Foundation', canon_fsf)
        self.add_package_author ('Free Software Foundation.', canon_fsf)
        self.add_package_author ('Free Software Foundation Inc.', canon_fsf)
        self.add_package_author ('Free Software Foundation, Inc', canon_fsf)
        self.add_package_author ('Free Software Foundation, Inc.', canon_fsf)
        self.add_package_author ('The Free Software Foundation', canon_fsf)
        self.add_package_author ('The Free Software Foundation, Inc.', canon_fsf)
        self.add_package_author ('Software Foundation, Inc.', canon_fsf)

        self.add_external_author ('Carnegie Mellon University')
        self.add_external_author ('John D. Polstra.')
        self.add_external_author ('Innovative Computing Labs')
        self.add_external_author ('Linaro Ltd.')
        self.add_external_author ('MIPS Computer Systems, Inc.')
        self.add_external_author ('Red Hat Inc.')
        self.add_external_author ('Regents of the University of California.')
        self.add_external_author ('The Regents of the University of California.')
        self.add_external_author ('Third Eye Software, Inc.')
        self.add_external_author ('Ulrich Drepper')
        self.add_external_author ('Synopsys Inc.')
        self.add_external_author ('Alan Woodland')
        self.add_external_author ('Diego Elio Petteno')

class BinutilsCmdLine (CmdLine):
    def __init__ (self):
        CmdLine.__init__ (self, BinutilsCopyright)

        self.add_dir ('.', TopLevelFilter())
        self.add_dir ('bfd')
        self.add_dir ('binutils')
        self.add_dir ('config', ConfigFilter())
        self.add_dir ('cpu')
        self.add_dir ('elfcpp')
        self.add_dir ('etc')
        self.add_dir ('gas')
        self.add_dir ('gdb')
        self.add_dir ('gdbserver')
        self.add_dir ('gdbsupport')
        self.add_dir ('gold')
        self.add_dir ('gprof')
        self.add_dir ('gprofng')
        self.add_dir ('include')
        self.add_dir ('ld', LdFilter())
        self.add_dir ('libbacktrace')
        self.add_dir ('libctf')
        self.add_dir ('libdecnumber')
        self.add_dir ('libiberty')
        self.add_dir ('libsframe')
        self.add_dir ('opcodes')
        self.add_dir ('readline')
        self.add_dir ('sim')

        self.default_dirs = [
            'bfd',
            'binutils',
            'elfcpp',
            'etc',
            'gas',
            'gold',
            'gprof',
            'gprofng',
            'include',
            'ld',
            'libctf',
            'libiberty',
            'libsframe',
            'opcodes',
            ]

BinutilsCmdLine().main()
