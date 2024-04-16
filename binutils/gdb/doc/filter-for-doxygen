#!/bin/sh

# This filters GDB source before Doxygen can get confused by it;
# this script is listed in the doxyfile. The output is not very
# pretty, but at least we get output that Doxygen can understand.
#
# $1 is a source file of some kind. The source we wish doxygen to
# process is put on stdout.

# (Adapted from gcc/contrib/filter_gcc_for_doxygen)

dir=`dirname $0`
perl $dir/filter-params.pl < $1
exit 0
