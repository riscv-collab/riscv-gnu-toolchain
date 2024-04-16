# Program to fetch python compilation parameters.
# Copied from python-config of the 2.7 release.

import sys
import os
import getopt
import sysconfig

valid_opts = ["prefix", "exec-prefix", "includes", "libs", "cflags", "ldflags", "help"]


def exit_with_usage(code=1):
    sys.stderr.write(
        "Usage: %s [%s]\n" % (sys.argv[0], "|".join("--" + opt for opt in valid_opts))
    )
    sys.exit(code)


try:
    opts, args = getopt.getopt(sys.argv[1:], "", valid_opts)
except getopt.error:
    exit_with_usage()

if not opts:
    exit_with_usage()

pyver = sysconfig.get_config_var("VERSION")
getvar = sysconfig.get_config_var
abiflags = getattr(sys, "abiflags", "")

opt_flags = [flag for (flag, val) in opts]

if "--help" in opt_flags:
    exit_with_usage(code=0)


def to_unix_path(path):
    """On Windows, returns the given path with all backslashes
    converted into forward slashes.  This is to help prevent problems
    when using the paths returned by this script with cygwin tools.
    In particular, cygwin bash treats backslashes as a special character.

    On Unix systems, returns the path unchanged.
    """
    if os.name == "nt":
        path = path.replace("\\", "/")
    return path


for opt in opt_flags:
    if opt == "--prefix":
        print(to_unix_path(os.path.normpath(sys.prefix)))

    elif opt == "--exec-prefix":
        print(to_unix_path(os.path.normpath(sys.exec_prefix)))

    elif opt in ("--includes", "--cflags"):
        flags = [
            "-I" + sysconfig.get_path("include"),
            "-I" + sysconfig.get_path("platinclude"),
        ]
        if opt == "--cflags":
            flags.extend(getvar("CFLAGS").split())
        print(to_unix_path(" ".join(flags)))

    elif opt in ("--libs", "--ldflags"):
        libs = ["-lpython" + pyver + abiflags]
        if getvar("LIBS") is not None:
            libs.extend(getvar("LIBS").split())
        if getvar("SYSLIBS") is not None:
            libs.extend(getvar("SYSLIBS").split())
        # add the prefix/lib/pythonX.Y/config dir, but only if there is no
        # shared library in prefix/lib/.
        if opt == "--ldflags":
            if not getvar("Py_ENABLE_SHARED"):
                if getvar("LIBPL") is not None:
                    libs.insert(0, "-L" + getvar("LIBPL"))
                elif os.name == "nt":
                    libs.insert(0, "-L" + os.path.normpath(sys.prefix) + "/libs")
            if getvar("LINKFORSHARED") is not None:
                libs.extend(getvar("LINKFORSHARED").split())
        print(to_unix_path(" ".join(libs)))
