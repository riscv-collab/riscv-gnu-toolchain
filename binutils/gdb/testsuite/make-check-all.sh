#!/bin/bash

# Copyright (C) 2023-2024 Free Software Foundation, Inc.
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

# Run make check with all boards from gdb/testsuite/boards.

# It is recommended to create users on the local system that will act as
#  "remote host" and "remote target", for the boards that use them.
#  Pass their usernames to --host-user and --target-user.  This helps
#  because:
#
#    - remote host/target boards will use $HOME and leave (potentially
#      lots of) files behind,
#    - it enables more strict checking of build/host/target file
#      manipulations,
#    - it prevents a command running on one "machine" to kill or send a
#      signal to a process on another machine.
#
# Recommended usage example:
#
# bash$ cd $objdir/gdb/testsuite
# bash$ $srcdir/testsuite/gdb/make-check-all.sh \
#           --host-user remote-host \
#           --target-user remote-target \
#           gdb.base/advance.exp

set -e

# Boards that run the host tools (compiler, gdb) on a remote host.
remote_host_boards=(
    local-remote-host
    local-remote-host-notty
)

# Boards that use gdbserver to launch target executables on local target.
gdbserver_boards=(
    native-extended-gdbserver
    native-gdbserver
    native-stdio-gdbserver
)

# Boards that use gdbserver to launch target executables on a remote target.
remote_gdbserver_boards=(
    remote-gdbserver-on-localhost
    remote-stdio-gdbserver
)

# Boards that run compiler, gdb and target executables on a remote machine
# that serves both as host and target.
host_target_boards=(
    local-remote-host-native
)

# Boards that run everything on local target and local host.
target_boards=(
    cc-with-gdb-index
    cc-with-index-cache
    cc-with-debug-names
    cc-with-dwz
    cc-with-dwz-m
    cc-with-gnu-debuglink
    debug-types
    dwarf4-gdb-index
    dwarf64
    fission
    fission-dwp
    gold
    gold-gdb-index
    readnow
    stabs
)

# Get RUNTESTFLAGS needed for specific boards.
rtf_for_board ()
{
    local b
    b="$1"

    case $b in
	local-remote-host-native)
	    mkdir -p "$tmpdir/$b"
	    rtf=(
		"${rtf[@]}"
		"HOST_DIR=$tmpdir/$b"
	    )
	    ;;
	remote-stdio-gdbserver)
	    rtf=(
		"${rtf[@]}"
		"REMOTE_HOSTNAME=localhost"
	    )
	    if [ "$target_user" != "" ]; then
		rtf=(
		    "${rtf[@]}"
		    "REMOTE_USERNAME=$target_user"
		)
	    else
		rtf=(
		    "${rtf[@]}"
		    "REMOTE_USERNAME=$USER"
		)
	    fi
	    ;;
	remote-gdbserver-on-localhost)
	    if [ "$target_user" != "" ]; then
		rtf=(
		    "${rtf[@]}"
		    "REMOTE_TARGET_USERNAME=$target_user"
		)
	    fi
	    ;;
	local-remote-host|local-remote-host-notty)
	    if [ "$host_user" != "" ]; then
		rtf=(
		    "${rtf[@]}"
		    "REMOTE_HOST_USERNAME=$host_user"
		)
	    else
		rtf=(
		    "${rtf[@]}"
		    "REMOTE_HOST_USERNAME=$USER"
		)
	    fi
	    ;;
	*)
	    ;;
    esac
}

# Summarize make check output.
summary ()
{
    if $verbose; then
	cat
    else
	# We need the sort -u, because some items, for instance "# of expected
	# passes" are output twice.
	grep -E "^(#|FAIL:|ERROR:|WARNING:)" \
	    | sort -u
    fi
}

# Run make check, and possibly save test results.
do_tests ()
{
    if $debug; then
	echo "RTF: ${rtf[*]}"
    fi

    if $dry_run; then
	return
    fi

    # Run make check.
    make check \
	 RUNTESTFLAGS="${rtf[*]} ${tests[*]}" \
	 2>&1 \
	| summary

    # Save test results.
    if $keep_results; then
	# Set cfg to identifier unique to host/target board combination.
	if [ "$h" = "" ]; then
	    if [ "$b" = "" ]; then
		cfg=local
	    else
		cfg=$b
	    fi
	else
	    cfg=$h-$b
	fi

	local dir
	dir="check-all/$cfg"

	mkdir -p "$dir"
	cp gdb.sum gdb.log "$dir"

	# Record the 'make check' command to enable easy re-running.
	echo "make check RUNTESTFLAGS=\"${rtf[*]} ${tests[*]}\"" \
	     > "$dir/make-check.sh"
    fi
}

# Set default values for global vars and modify according to command line
# arguments.
parse_args ()
{
    # Default values.
    debug=false
    keep_results=false
    keep_tmp=false
    verbose=false
    dry_run=false

    host_user=""
    target_user=""

    # Parse command line arguments.
    while [ $# -gt 0 ]; do
	case "$1" in
	    --debug)
		debug=true
		;;
	    --keep-results)
		keep_results=true
		;;
	    --keep-tmp)
		keep_tmp=true
		;;
	    --verbose)
		verbose=true
		;;
	    --dry-run)
		dry_run=true
		;;
	    --host-user)
		shift
		host_user="$1"
		;;
	    --target-user)
		shift
		target_user="$1"
		;;
	    *)
		break
		;;
	esac
	shift
    done

    tests=("$@")
}

# Cleanup function, scheduled to run on exit.
cleanup ()
{
    if [ "$tmpdir" != "" ]; then
	if $keep_tmp; then
	    echo "keeping tmp dir $tmpdir"
	else
	    rm -Rf "$tmpdir"
	fi
    fi
}

# Top-level function, called with command line arguments of the script.
main ()
{
    # Parse command line arguments.
    parse_args "$@"

    # Create tmpdir and schedule cleanup.
    tmpdir=""
    trap cleanup EXIT
    tmpdir=$(mktemp -d)

    if $debug; then
	echo "TESTS: ${tests[*]}"
    fi

    # Variables that point to current host (h) and target board (b) when
    # executing do_tests.
    h=""
    b=""

    # For reference, run the tests without any explicit host or target board.
    echo "LOCAL:"
    rtf=()
    do_tests

    # Run the boards for local host and local target.
    for b in "${target_boards[@]}"; do
	echo "TARGET BOARD: $b"
	rtf=(
	    --target_board="$b"
	)
	rtf_for_board "$b"
	do_tests
    done

    # Run the boards that use gdbserver, for local host, and for both local and
    # remote target.
    for b in "${gdbserver_boards[@]}" "${remote_gdbserver_boards[@]}"; do
	echo "TARGET BOARD: $b"
	rtf=(
	    --target_board="$b"
	)
	rtf_for_board "$b"
	do_tests
    done

    # Run the boards that use remote host, in combination with boards that use
    # gdbserver on remote target.
    for h in "${remote_host_boards[@]}"; do
	for b in "${remote_gdbserver_boards[@]}"; do
	    echo "HOST BOARD: $h, TARGET BOARD: $b"
	    rtf=(
		--host_board="$h"
		--target_board="$b"
	    )
	    rtf_for_board "$h"
	    rtf_for_board "$b"
	    do_tests
	done
    done
    h=""

    # Run the boards that function as both remote host and remote target.
    for b in "${host_target_boards[@]}"; do
	echo "HOST/TARGET BOARD: $b"
	rtf=(
	    --host_board="$b"
	    --target_board="$b"
	)
	rtf_for_board "$b"
	do_tests
    done
}

# Call top-level function with command line arguments.
main "$@"
