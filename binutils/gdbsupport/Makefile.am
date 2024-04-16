## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2020-2024 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.
#

AUTOMAKE_OPTIONS = no-dist foreign
ACLOCAL_AMFLAGS = -I . -I ../config

# Suppress 'configure.ac: error: AM_GNU_GETTEXT used but SUBDIRS not defined'
# from Automake, as gdbsupport uses AM_GNU_GETTEXT through
# ZW_GNU_GETTEXT_SISTER_DIR, but doesn't have any translations (currently).
SUBDIRS =

AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../gdb \
    -I../gnulib/import -I$(srcdir)/../gnulib/import \
    -I.. -I$(srcdir)/.. $(INCINTL) -I../bfd -I$(srcdir)/../bfd \
    @LARGEFILE_CPPFLAGS@

override CXX += $(CXX_DIALECT)

AM_CXXFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)

noinst_LIBRARIES = libgdbsupport.a

if SELFTEST
selftest = selftest.cc
endif

if HAVE_PIPE_OR_PIPE2
eventpipe = event-pipe.cc
endif

libgdbsupport_a_SOURCES = \
    agent.cc \
    btrace-common.cc \
    cleanups.cc \
    common-debug.cc \
    common-exceptions.cc \
    common-inferior.cc \
    common-regcache.cc \
    common-utils.cc \
    environ.cc \
    errors.cc \
    event-loop.cc \
    fileio.cc \
    filestuff.cc \
    format.cc \
    gdb-dlfcn.cc \
    gdb-hashtab.cc \
    gdb_obstack.cc \
    gdb_regex.cc \
    gdb_tilde_expand.cc \
    gdb_wait.cc \
    gdb_vecs.cc \
    job-control.cc \
    netstuff.cc \
    new-op.cc \
    pathstuff.cc \
    print-utils.cc \
    ptid.cc \
    rsp-low.cc \
    run-time-clock.cc \
    safe-strerror.cc \
    scoped_mmap.cc \
    search.cc \
    signals.cc \
    signals-state-save-restore.cc \
    task-group.cc \
    tdesc.cc \
    thread-pool.cc \
    xml-utils.cc \
    ${eventpipe} \
    $(selftest)

# Double-check that no defines are missing from our configury.
check-defines:
	cd $(srcdir) && emacs --script check-defines.el
