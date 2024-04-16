# Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

"""Configure GDB using the ELinOS environment."""

import os
import glob
import gdb


def warn(msg):
    print("warning: %s" % msg)


def get_elinos_environment():
    """Return the ELinOS environment.

    If the ELinOS environment is properly set up, return a dictionary
    which contains:
      * The path to the ELinOS project at key 'project';
      * The path to the ELinOS CDK at key 'cdk';
      * The ELinOS target name at key 'target' (Eg. 'i486-linux');
      * A list of Xenomai install prefixes (which could be empty, if
        the ELinOS project does not include Xenomai) at key 'xenomai'.

    If one of these cannot be found, print a warning; the corresponding
    value in the returned dictionary will be None.
    """
    result = {}
    for key in ("project", "cdk", "target"):
        var = "ELINOS_" + key.upper()
        if var in os.environ:
            result[key] = os.environ[var]
        else:
            warn("%s not set" % var)
            result[key] = None

    if result["project"] is not None:
        result["xenomai"] = glob.glob(result["project"] + "/xenomai-[0-9.]*")
    else:
        result["xenomai"] = []

    return result


def elinos_init():
    """Initialize debugger environment for ELinOS.

    Let the debugger know where to find the ELinOS libraries on host. This
    assumes that an ELinOS environment is properly set up. If some environment
    variables are missing, warn about which library may be missing.
    """
    elinos_env = get_elinos_environment()

    solib_dirs = []

    # System libraries
    if None in (elinos_env[key] for key in ("cdk", "target")):
        warn("ELinOS system libraries will not be loaded")
    else:
        solib_prefix = "%s/%s" % (elinos_env["cdk"], elinos_env["target"])
        solib_dirs += ["%s/%s" % (solib_prefix, "lib")]
        gdb.execute("set solib-absolute-prefix %s" % solib_prefix)

    # Xenomai libraries. Those are optional, so have a lighter warning
    # if they cannot be located.
    if elinos_env["project"] is None:
        warn("Xenomai libraries may not be loaded")
    else:
        for dir in elinos_env["xenomai"]:
            solib_dirs += ["%s/%s" % (dir, "xenomai-build/usr/realtime/lib")]

    if len(solib_dirs) != 0:
        gdb.execute("set solib-search-path %s" % ":".join(solib_dirs))


if __name__ == "__main__":
    elinos_init()
