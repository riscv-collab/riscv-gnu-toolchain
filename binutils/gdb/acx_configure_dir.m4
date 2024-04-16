# Copyright (C) 1992-2024 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.

# ACX_CONFIGURE_DIR(SRC-DIR-NAME, BUILD-DIR-NAME, EXTRA-ARGS)
# ---------------------------
#
# Configure a subdirectory.  This is an alternative to
# AC_CONFIG_SUBDIRS that allows pointing the source directory
# somewhere else.  The build directory is always a subdirectory of the
# top build directory.  This is heavilly based on Autoconf 2.64's
# _AC_OUTPUT_SUBDIRS.
#
# Inputs:
#   - SRC-DIR-NAME is the source directory, relative to $srcdir.
#   - BUILD-DIR-NAME is `top-build -> build'
#   - EXTRA-ARGS is an optional list of extra arguments to add
#     at the end of the configure command.

AC_DEFUN([ACX_CONFIGURE_DIR],
[
  in_src=$1
  in_build=$2
  in_extra_args=$3

  # Remove --cache-file, --srcdir, and --disable-option-checking arguments
  # so they do not pile up.
  ac_sub_configure_args=
  ac_prev=
  eval "set x $ac_configure_args"
  shift
  for ac_arg
  do
    if test -n "$ac_prev"; then
      ac_prev=
      continue
    fi
    case $ac_arg in
    -cache-file | --cache-file | --cache-fil | --cache-fi \
    | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
      ac_prev=cache_file ;;
    -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
    | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* \
    | --c=*)
      ;;
    --config-cache | -C)
      ;;
    -srcdir | --srcdir | --srcdi | --srcd | --src | --sr)
      ac_prev=srcdir ;;
    -srcdir=* | --srcdir=* | --srcdi=* | --srcd=* | --src=* | --sr=*)
      ;;
    -prefix | --prefix | --prefi | --pref | --pre | --pr | --p)
      ac_prev=prefix ;;
    -prefix=* | --prefix=* | --prefi=* | --pref=* | --pre=* | --pr=* | --p=*)
      ;;
    --disable-option-checking)
      ;;
    *)
      case $ac_arg in
      *\'*) ac_arg=`AS_ECHO(["$ac_arg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
      esac
      AS_VAR_APPEND([ac_sub_configure_args], [" '$ac_arg'"]) ;;
    esac
  done

  # Always prepend --prefix to ensure using the same prefix
  # in subdir configurations.
  ac_arg="--prefix=$prefix"
  case $ac_arg in
  *\'*) ac_arg=`AS_ECHO(["$ac_arg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
  esac
  ac_sub_configure_args="'$ac_arg' $ac_sub_configure_args"

  # Pass --silent
  if test "$silent" = yes; then
    ac_sub_configure_args="--silent $ac_sub_configure_args"
  fi

  # Always prepend --disable-option-checking to silence warnings, since
  # different subdirs can have different --enable and --with options.
  ac_sub_configure_args="--disable-option-checking $ac_sub_configure_args"

  ac_popdir=`pwd`
  ac_dir=$in_build

  ac_msg="=== configuring in $ac_dir (`pwd`/$ac_dir)"
  _AS_ECHO_LOG([$ac_msg])
  _AS_ECHO([$ac_msg])
  AS_MKDIR_P(["$ac_dir"])

  case $srcdir in
  [[\\/]]* | ?:[[\\/]]* )
    ac_srcdir=$srcdir/$in_src ;;
  *) # Relative name.
    ac_srcdir=../$srcdir/$in_src ;;
  esac

  cd "$ac_dir"

  ac_sub_configure=$ac_srcdir/configure

  # Make the cache file name correct relative to the subdirectory.
  case $cache_file in
  [[\\/]]* | ?:[[\\/]]* ) ac_sub_cache_file=$cache_file ;;
  *) # Relative name.
    ac_sub_cache_file=$ac_top_build_prefix$cache_file ;;
  esac

  if test -n "$in_extra_args"; then
    # Add the extra args at the end.
    ac_sub_configure_args="$ac_sub_configure_args $in_extra_args"
  fi

  AC_MSG_NOTICE([running $SHELL $ac_sub_configure $ac_sub_configure_args --cache-file=$ac_sub_cache_file --srcdir=$ac_srcdir])
  # The eval makes quoting arguments work.
  eval "\$SHELL \"\$ac_sub_configure\" $ac_sub_configure_args \
       --cache-file=\"\$ac_sub_cache_file\" --srcdir=\"\$ac_srcdir\"" ||
    AC_MSG_ERROR([$ac_sub_configure failed for $ac_dir])

  cd "$ac_popdir"
])# ACX_CONFIGURE_DIR
