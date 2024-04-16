#! /bin/sh

# GDB script to create web ARI page directly from within gdb/ari directory.
#
# Copyright (C) 2012-2024 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Determine directory of current script.
scriptpath=`dirname $0`
# If "scriptpath" is a relative path, then convert it to absolute.
if [ "`echo ${scriptpath} | cut -b1`" != '/' ] ; then
    scriptpath="`pwd`/${scriptpath}"
fi

# update-web-ari.sh script wants four parameters
# 1: directory of checkout src or gdb-RELEASE for release sources.
# 2: a temp directory.
# 3: a directory for generated web page.
# 4: The name of the current package, must be gdb here.
# Here we provide default values for these 4 parameters

# srcdir parameter
if [ -z "${srcdir}" ] ; then
  srcdir=${scriptpath}/../../..
fi

# Determine location of a temporary directory to be used by
# update-web-ari.sh script.
if [ -z "${tempdir}" ] ; then
  if [ ! -z "$TMP" ] ; then
    tempdir=$TMP/create-ari
  elif [ ! -z "$TEMP" ] ; then
    tempdir=$TEMP/create-ari
  else
    tempdir=/tmp/create-ari
  fi
fi

# Default location of generate index.hmtl web page.
if [ -z "${webdir}" ] ; then
# Use 'branch' subdir name if Tag contains branch
  if [ -f "${srcdir}/gdb/CVS/Tag" ] ; then
    tagname=`cat "${srcdir}/gdb/CVS/Tag"`
  elif [ -d "${srcdir}/.git" ] ; then
    tagname=`cd ${srcdir} && git rev-parse --abbrev-ref HEAD`
    if test "$tagname" = "master"; then
      tagname=trunk
    fi
  else
    tagname=trunk
  fi
  if [ "${tagname#branch}" != "${tagname}" ] ; then
    subdir=branch
  else
    subdir=trunk
  fi
  webdir=`pwd`/${subdir}/ari
fi

# Launch update-web-ari.sh in same directory as current script.
${SHELL} ${scriptpath}/update-web-ari.sh ${srcdir} ${tempdir} ${webdir} gdb

if [ -f "${webdir}/index.html" ] ; then
  echo "ARI output can be viewed in file \"${webdir}/index.html\""
else
  echo "ARI script failed to generate file \"${webdir}/index.html\""
fi

