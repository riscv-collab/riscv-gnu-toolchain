#! /usr/bin/env python3

# Copyright (C) 2011-2024 Free Software Foundation, Inc.
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

"""copyright.py

This script updates the list of years in the copyright notices in
most files maintained by the GDB project.

Usage: cd src/gdb && ./copyright.py

Always review the output of this script before committing it!
A useful command to review the output is:
    % filterdiff -x \*.c -x \*.cc -x \*.h -x \*.exp updates.diff
This removes the bulk of the changes which are most likely to be correct.
"""

import argparse
import datetime
import locale
import os
import os.path
import subprocess
import sys
from typing import List, Optional


def get_update_list():
    """Return the list of files to update.

    Assumes that the current working directory when called is the root
    of the GDB source tree (NOT the gdb/ subdirectory!).  The names of
    the files are relative to that root directory.
    """
    result = []
    for gdb_dir in (
        "gdb",
        "gdbserver",
        "gdbsupport",
        "gnulib",
        "sim",
        "include/gdb",
    ):
        for root, dirs, files in os.walk(gdb_dir, topdown=True):
            for dirname in dirs:
                reldirname = "%s/%s" % (root, dirname)
                if (
                    dirname in EXCLUDE_ALL_LIST
                    or reldirname in EXCLUDE_LIST
                    or reldirname in NOT_FSF_LIST
                    or reldirname in BY_HAND
                ):
                    # Prune this directory from our search list.
                    dirs.remove(dirname)
            for filename in files:
                relpath = "%s/%s" % (root, filename)
                if (
                    filename in EXCLUDE_ALL_LIST
                    or relpath in EXCLUDE_LIST
                    or relpath in NOT_FSF_LIST
                    or relpath in BY_HAND
                ):
                    # Ignore this file.
                    pass
                else:
                    result.append(relpath)
    return result


def update_files(update_list):
    """Update the copyright header of the files in the given list.

    We use gnulib's update-copyright script for that.
    """
    # We want to use year intervals in the copyright notices, and
    # all years should be collapsed to one single year interval,
    # even if there are "holes" in the list of years found in the
    # original copyright notice (OK'ed by the FSF, case [gnu.org #719834]).
    os.environ["UPDATE_COPYRIGHT_USE_INTERVALS"] = "2"

    # Perform the update, and save the output in a string.
    update_cmd = ["bash", "gnulib/import/extra/update-copyright"]
    update_cmd += update_list

    p = subprocess.Popen(
        update_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding=locale.getpreferredencoding(),
    )
    update_out = p.communicate()[0]

    # Process the output.  Typically, a lot of files do not have
    # a copyright notice :-(.  The update-copyright script prints
    # a well defined warning when it did not find the copyright notice.
    # For each of those, do a sanity check and see if they may in fact
    # have one.  For the files that are found not to have one, we filter
    # the line out from the output, since there is nothing more to do,
    # short of looking at each file and seeing which notice is appropriate.
    # Too much work! (~4,000 files listed as of 2012-01-03).
    update_out = update_out.splitlines(keepends=False)
    warning_string = ": warning: copyright statement not found"
    warning_len = len(warning_string)

    for line in update_out:
        if line.endswith(warning_string):
            filename = line[:-warning_len]
            if may_have_copyright_notice(filename):
                print(line)
        else:
            # Unrecognized file format. !?!
            print("*** " + line)


def may_have_copyright_notice(filename):
    """Check that the given file does not seem to have a copyright notice.

    The filename is relative to the root directory.
    This function assumes that the current working directory is that root
    directory.

    The algorithm is fairly crude, meaning that it might return
    some false positives.  I do not think it will return any false
    negatives...  We might improve this function to handle more
    complex cases later...
    """
    # For now, it may have a copyright notice if we find the word
    # "Copyright" at the (reasonable) start of the given file, say
    # 50 lines...
    MAX_LINES = 50

    # We don't really know what encoding each file might be following,
    # so just open the file as a byte stream. We only need to search
    # for a pattern that should be the same regardless of encoding,
    # so that should be good enough.
    with open(filename, "rb") as fd:
        for lineno, line in enumerate(fd, start=1):
            if b"Copyright" in line:
                return True
            if lineno > MAX_LINES:
                break
    return False


def get_parser() -> argparse.ArgumentParser:
    """Get a command line parser."""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    return parser


def main(argv: List[str]) -> Optional[int]:
    """The main subprogram."""
    parser = get_parser()
    _ = parser.parse_args(argv)
    root_dir = os.path.dirname(os.getcwd())
    os.chdir(root_dir)

    if not (
        os.path.isdir("gdb") and os.path.isfile("gnulib/import/extra/update-copyright")
    ):
        sys.exit("Error: This script must be called from the gdb directory.")

    update_list = get_update_list()
    update_files(update_list)

    # Remind the user that some files need to be updated by HAND...

    if MULTIPLE_COPYRIGHT_HEADERS:
        print()
        print(
            "\033[31m"
            "REMINDER: Multiple copyright headers must be updated by hand:"
            "\033[0m"
        )
        for filename in MULTIPLE_COPYRIGHT_HEADERS:
            print("  ", filename)

    if BY_HAND:
        print()
        print(
            "\033[31mREMINDER: The following files must be updated by hand." "\033[0m"
        )
        for filename in BY_HAND:
            print("  ", filename)


############################################################################
#
# Some constants, placed at the end because they take up a lot of room.
# The actual value of these constants is not significant to the understanding
# of the script.
#
############################################################################

# Files which should not be modified, either because they are
# generated, non-FSF, or otherwise special (e.g. license text,
# or test cases which must be sensitive to line numbering).
#
# Filenames are relative to the root directory.
EXCLUDE_LIST = (
    "gdb/nat/glibc_thread_db.h",
    "gdb/CONTRIBUTE",
    "gdbsupport/Makefile.in",
    "gnulib/doc/gendocs_template",
    "gnulib/doc/gendocs_template_min",
    "gnulib/import",
    "gnulib/config.in",
    "gnulib/Makefile.in",
)

# Files which should not be modified, either because they are
# generated, non-FSF, or otherwise special (e.g. license text,
# or test cases which must be sensitive to line numbering).
#
# Matches any file or directory name anywhere.  Use with caution.
# This is mostly for files that can be found in multiple directories.
# Eg: We want all files named COPYING to be left untouched.

EXCLUDE_ALL_LIST = (
    "COPYING",
    "COPYING.LIB",
    "CVS",
    "configure",
    "copying.c",
    "fdl.texi",
    "gpl.texi",
    "aclocal.m4",
)

# The list of files to update by hand.
BY_HAND = (
    # Nothing at the moment :-).
)

# Files containing multiple copyright headers.  This script is only
# fixing the first one it finds, so we need to finish the update
# by hand.
MULTIPLE_COPYRIGHT_HEADERS = (
    "gdb/doc/gdb.texinfo",
    "gdb/doc/refcard.tex",
    "gdb/syscalls/update-netbsd.sh",
)

# The list of file which have a copyright, but not held by the FSF.
# Filenames are relative to the root directory.
NOT_FSF_LIST = (
    "gdb/exc_request.defs",
    "gdb/gdbtk",
    "gdb/testsuite/gdb.gdbtk/",
    "sim/arm/armemu.h",
    "sim/arm/armos.c",
    "sim/arm/gdbhost.c",
    "sim/arm/dbg_hif.h",
    "sim/arm/dbg_conf.h",
    "sim/arm/communicate.h",
    "sim/arm/armos.h",
    "sim/arm/armcopro.c",
    "sim/arm/armemu.c",
    "sim/arm/kid.c",
    "sim/arm/thumbemu.c",
    "sim/arm/armdefs.h",
    "sim/arm/armopts.h",
    "sim/arm/dbg_cp.h",
    "sim/arm/dbg_rdi.h",
    "sim/arm/parent.c",
    "sim/arm/armsupp.c",
    "sim/arm/armrdi.c",
    "sim/arm/bag.c",
    "sim/arm/armvirt.c",
    "sim/arm/main.c",
    "sim/arm/bag.h",
    "sim/arm/communicate.c",
    "sim/arm/gdbhost.h",
    "sim/arm/armfpe.h",
    "sim/arm/arminit.c",
    "sim/common/cgen-fpu.c",
    "sim/common/cgen-fpu.h",
    "sim/common/cgen-accfp.c",
    "sim/mips/m16run.c",
    "sim/mips/sim-main.c",
    "sim/moxie/moxie-gdb.dts",
    # Not a single file in sim/ppc/ appears to be copyright FSF :-(.
    "sim/ppc/filter.h",
    "sim/ppc/gen-support.h",
    "sim/ppc/ld-insn.h",
    "sim/ppc/hw_sem.c",
    "sim/ppc/hw_disk.c",
    "sim/ppc/idecode_branch.h",
    "sim/ppc/sim-endian.h",
    "sim/ppc/table.c",
    "sim/ppc/hw_core.c",
    "sim/ppc/gen-support.c",
    "sim/ppc/gen-semantics.h",
    "sim/ppc/cpu.h",
    "sim/ppc/sim_callbacks.h",
    "sim/ppc/RUN",
    "sim/ppc/Makefile.in",
    "sim/ppc/emul_chirp.c",
    "sim/ppc/hw_nvram.c",
    "sim/ppc/dc-test.01",
    "sim/ppc/hw_phb.c",
    "sim/ppc/hw_eeprom.c",
    "sim/ppc/bits.h",
    "sim/ppc/hw_vm.c",
    "sim/ppc/cap.h",
    "sim/ppc/os_emul.h",
    "sim/ppc/options.h",
    "sim/ppc/gen-idecode.c",
    "sim/ppc/filter.c",
    "sim/ppc/corefile-n.h",
    "sim/ppc/std-config.h",
    "sim/ppc/ld-decode.h",
    "sim/ppc/filter_filename.h",
    "sim/ppc/hw_shm.c",
    "sim/ppc/pk_disklabel.c",
    "sim/ppc/dc-simple",
    "sim/ppc/misc.h",
    "sim/ppc/device_table.h",
    "sim/ppc/ld-insn.c",
    "sim/ppc/inline.c",
    "sim/ppc/emul_bugapi.h",
    "sim/ppc/hw_cpu.h",
    "sim/ppc/debug.h",
    "sim/ppc/hw_ide.c",
    "sim/ppc/debug.c",
    "sim/ppc/gen-itable.h",
    "sim/ppc/interrupts.c",
    "sim/ppc/hw_glue.c",
    "sim/ppc/emul_unix.c",
    "sim/ppc/sim_calls.c",
    "sim/ppc/dc-complex",
    "sim/ppc/ld-cache.c",
    "sim/ppc/registers.h",
    "sim/ppc/dc-test.02",
    "sim/ppc/options.c",
    "sim/ppc/igen.h",
    "sim/ppc/registers.c",
    "sim/ppc/device.h",
    "sim/ppc/emul_chirp.h",
    "sim/ppc/hw_register.c",
    "sim/ppc/hw_init.c",
    "sim/ppc/sim-endian-n.h",
    "sim/ppc/filter_filename.c",
    "sim/ppc/bits.c",
    "sim/ppc/idecode_fields.h",
    "sim/ppc/hw_memory.c",
    "sim/ppc/misc.c",
    "sim/ppc/double.c",
    "sim/ppc/psim.h",
    "sim/ppc/hw_trace.c",
    "sim/ppc/emul_netbsd.h",
    "sim/ppc/psim.c",
    "sim/ppc/powerpc.igen",
    "sim/ppc/tree.h",
    "sim/ppc/README",
    "sim/ppc/gen-icache.h",
    "sim/ppc/gen-model.h",
    "sim/ppc/ld-cache.h",
    "sim/ppc/mon.c",
    "sim/ppc/corefile.h",
    "sim/ppc/vm.c",
    "sim/ppc/INSTALL",
    "sim/ppc/gen-model.c",
    "sim/ppc/hw_cpu.c",
    "sim/ppc/corefile.c",
    "sim/ppc/hw_opic.c",
    "sim/ppc/gen-icache.c",
    "sim/ppc/events.h",
    "sim/ppc/os_emul.c",
    "sim/ppc/emul_generic.c",
    "sim/ppc/main.c",
    "sim/ppc/hw_com.c",
    "sim/ppc/gen-semantics.c",
    "sim/ppc/emul_bugapi.c",
    "sim/ppc/device.c",
    "sim/ppc/emul_generic.h",
    "sim/ppc/tree.c",
    "sim/ppc/mon.h",
    "sim/ppc/interrupts.h",
    "sim/ppc/cap.c",
    "sim/ppc/cpu.c",
    "sim/ppc/hw_phb.h",
    "sim/ppc/device_table.c",
    "sim/ppc/lf.c",
    "sim/ppc/lf.c",
    "sim/ppc/dc-stupid",
    "sim/ppc/hw_pal.c",
    "sim/ppc/ppc-spr-table",
    "sim/ppc/emul_unix.h",
    "sim/ppc/words.h",
    "sim/ppc/basics.h",
    "sim/ppc/hw_htab.c",
    "sim/ppc/lf.h",
    "sim/ppc/ld-decode.c",
    "sim/ppc/sim-endian.c",
    "sim/ppc/gen-itable.c",
    "sim/ppc/idecode_expression.h",
    "sim/ppc/table.h",
    "sim/ppc/dgen.c",
    "sim/ppc/events.c",
    "sim/ppc/gen-idecode.h",
    "sim/ppc/emul_netbsd.c",
    "sim/ppc/igen.c",
    "sim/ppc/vm_n.h",
    "sim/ppc/vm.h",
    "sim/ppc/hw_iobus.c",
    "sim/ppc/inline.h",
    "sim/testsuite/mips/mips32-dsp2.s",
)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
