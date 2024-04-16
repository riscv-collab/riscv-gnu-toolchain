#
#   Copyright (C) 2021-2023 Free Software Foundation, Inc.
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
#------------------------------------------------------------------------------
# This script demonstrates how to use gprofng.
#
# After the experiment data has been generated, several views into the data
# are shown.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Define the executable, algorithm parameters and gprofng settings.
#------------------------------------------------------------------------------
exe=../experiments/mxv-pthreads
rows=4000
columns=2000
threads=2
exp_directory=experiment.$threads.thr.er

#------------------------------------------------------------------------------
# Check if gprofng has been installed and can be executed.
#------------------------------------------------------------------------------
which gprofng > /dev/null 2>&1
if (test $? -eq 0) then
  echo  ""
  echo "Version information of the gprofng release used:"
  echo  ""
  gprofng --version
  echo  ""
else
  echo "Error: gprofng cannot be found - if it was installed, check your path"
  exit
fi

#------------------------------------------------------------------------------
# Check if the executable is present.
#------------------------------------------------------------------------------
if (! test -x $exe) then
  echo "Error: executable $exe not found - check the make install command"
  exit
fi

echo "-------------- Collect the experiment data -----------------------------"
gprofng collect app -O $exp_directory $exe -m $rows -n $columns -t $threads

#------------------------------------------------------------------------------
# Make sure that the collect experiment succeeded and created an experiment
# directory with the performance data.
#------------------------------------------------------------------------------
if (! test -d $exp_directory) then
  echo "Error: experiment directory $exp_directory not found"
  exit
fi

echo "-------------- Show the function overview  -----------------------------"
gprofng display text -functions $exp_directory

echo "-------------- Show the function overview limit to the top 5 -----------"
gprofng display text -limit 5 -functions $exp_directory

echo "-------------- Show the source listing of mxv_core ---------------------"
gprofng display text -metrics e.totalcpu -source mxv_core $exp_directory

echo "-------------- Show the disassembly listing of mxv_core ----------------"
gprofng display text -metrics e.totalcpu -disasm mxv_core $exp_directory
