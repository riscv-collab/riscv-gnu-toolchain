#!/bin/sh

# Build script to build GDB with all targets enabled.

# Copyright (C) 2008-2024 Free Software Foundation, Inc.
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

# Make certain that the script is not running in an internationalized
# environment. The script is grepping for GDB's output.

# Contributed by Markus Deuling <deuling@de.ibm.com>.
# Based on gdb_mbuild.sh from Richard Earnshaw.


LANG=c ; export LANG
LC_ALL=c ; export LC_ALL

# Prints a usage message.
usage()
{
  cat <<EOF
Usage: gdb_buildall.sh [ <options> ... ] <srcdir> <builddir>

Options:

  --bfd64        Enable 64-bit BFD.
  --clean        Delete build directory after check.
  -e <regexp>    Regular expression for selecting the targets to build.
  --force        Force rebuild.
  -j <makejobs>  Run <makejobs> in parallel.  Passed to make.
		 On a single cpu machine, 2 is recommended.
 Arguments:
   <srcdir>       Source code directory.
   <builddir>     Build directory.

 Environment variables examined (with default if not defined):
   MAKE (make)"
EOF
  exit 1
}

### Command line options.
makejobs=
force=false
targexp=""
bfd_flag=""
clean=false
while test $# -gt 0
do
  case "$1" in
  -j )
      # Number of parallel make jobs.
      shift
      test $# -ge 1 || usage
      makejobs="-j $1"
      ;;
      --clean )
	# Shall the build directory be deleted after processing?
	clean=true
	;;
    -e )
      # A regular expression for selecting targets
      shift
      test $# -ge 1 || usage
      targexp="${targexp} -e ${1}"
      ;;
    --force )
      # Force a rebuild
      force=true ;
      ;;
    --bfd64)
      # Enable 64-bit BFD
      bfd_flag="--enable-64-bit-bfd"
      ;;
    -* ) usage ;;
    *) break ;;
  esac
  shift
done

if test $# -ne 2
then
  usage
fi

### Environment.

# Convert these to absolute directory paths.
srcdir=`cd $1 && /bin/pwd` || exit 1
builddir=`cd $2 && /bin/pwd` || exit 1
# Version of make to use
make=${MAKE:-make}
MAKE=${make}
export MAKE
# We don't want GDB do dump cores.
ulimit -c 0

# Just make sure we're in the right directory.
maintainers=${srcdir}/gdb/MAINTAINERS
if [ ! -r ${maintainers} ]
then
    echo Maintainers file ${maintainers} not found
    exit 1
fi


# Build GDB with all targets enabled.
echo "Starting gdb_buildall.sh ..."

trap "exit 1"  1 2 15
dir=${builddir}/ALL

# Should a scratch rebuild be forced, for perhaps the entire build be skipped?
if ${force}
then
  echo ... forcing rebuild
  rm -rf ${dir}
fi

# Did the previous configure attempt fail?  If it did restart from scratch
if test -d ${dir} -a ! -r ${dir}/Makefile
then
  echo ... removing partially configured 
  rm -rf ${dir}
  if test -d ${dir}
  then
    echo "... ERROR: Unable to remove directory ${dir}"
    exit 1
  fi
fi

# Create build directory.
mkdir -p ${dir}
cd ${dir} || exit 1

# Configure GDB.
if test ! -r Makefile
then
  # Default SIMOPTS to GDBOPTS.
  test -z "${simopts}" && simopts="${gdbopts}"

  # The config options.
  __build="--enable-targets=all"
  __enable_gdb_build_warnings=`test -z "${gdbopts}" \
    || echo "--enable-gdb-build-warnings=${gdbopts}"`
  __enable_sim_build_warnings=`test -z "${simopts}" \
    || echo "--enable-sim-build-warnings=${simopts}"`
  __configure="${srcdir}/configure \
    ${__build} ${bfd_flag}\
    ${__enable_gdb_build_warnings} \
    ${__enable_sim_build_warnings}"
  echo ... ${__configure}
  trap "echo Removing partially configured ${dir} directory ...; rm -rf ${dir}; exit 1" 1 2 15
  ${__configure} > Config.log 2>&1
  trap "exit 1"  1 2 15

  # Without Makefile GDB won't build.
  if test ! -r Makefile
  then
    echo "... CONFIG ERROR: GDB couldn't be configured " | tee -a Config.log
    echo "... CONFIG ERROR: see Config.log for details "
    exit 1
  fi
fi

# Build GDB, if not built.
gdb_bin="gdb/gdb"
if test ! -x gdb/gdb -a ! -x gdb/gdb.exe
then
  echo ... ${make} ${makejobs}
  ( ${make} ${makejobs} all-gdb || rm -f gdb/gdb gdb/gdb.exe
  ) > Build.log 2>&1

  # If the build fails, exit.
  if test ! -x gdb/gdb -a ! -x gdb/gdb.exe
  then
    echo "... BUILD ERROR: GDB couldn't be compiled " | tee -a Build.log
    echo "... BUILD ERROR: see Build.log for details "
    exit 1
  fi
  if test -x gdb/gdb.exe
  then
    gdb_bin="gdb/gdb.exe"
  fi
fi


# Retrieve a list of settable architectures by invoking "set architecture"
# without parameters.
cat <<EOF > arch
set architecture
quit
EOF
./gdb/gdb --batch -nx -x arch 2>&1 | cat > gdb_archs
tail -n 1 gdb_archs | sed 's/auto./\n/g' | sed 's/,/\n/g' |  sed 's/Requires an argument. Valid arguments are/\n/g' | sed '/^[ ]*$/d' > arch
mv arch gdb_archs

if test "${targexp}" != ""
then
  alltarg=`cat gdb_archs | grep ${targexp}`
else 
  alltarg=`cat gdb_archs`
fi
rm -f gdb_archs

# Test all architectures available in ALLTARG
echo "maint print architecture for"
echo "$alltarg" | while read target
do
  cat <<EOF > x
set architecture ${target}
maint print architecture
quit
EOF
  log_file=$target.log
  log_file=${log_file//:/_}
  echo -n "... ${target}"
  ./gdb/gdb -batch -nx -x x 2>&1 | cat > $log_file
  # Check GDBs results
  if test ! -s $log_file
  then
    echo " ERR: gdb printed no output" | tee -a $log_file
  elif test `grep -o internal-error $log_file | tail -n 1`
  then
    echo " ERR: gdb panic" | tee -a $log_file
  else
    echo " OK"
  fi
  
  # Create a sed script that cleans up the output from GDB.
  rm -f mbuild.sed
  # Rules to replace <0xNNNN> with the corresponding function's name.
  sed -n -e '/<0x0*>/d' -e 's/^.*<0x\([0-9a-f]*\)>.*$/0x\1/p' $log_file \
  | sort -u \
  | while read addr
  do
    func="`addr2line -f -e ./$gdb_bin -s ${addr} | sed -n -e 1p`"
    echo "s/<${addr}>/<${func}>/g"
  done >> mbuild.sed
  # Rules to strip the leading paths off of file names.
  echo 's/"\/.*\/gdb\//"gdb\//g' >> mbuild.sed
  # Run the script.
  sed -f mbuild.sed $log_file > Mbuild.log

  mv Mbuild.log ${builddir}/$log_file
  rm -rf $log_file x mbuild.sed
done
echo "done."

# Clean up build directory if necessary.
if ${clean}
then
  echo "cleanning up $dir"
  rm -rf ${dir}
fi

exit 0
