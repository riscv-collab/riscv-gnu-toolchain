/* Copyright 2022-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

set compiler_info "unknown"

#if defined (__GFORTRAN__)
set compiler_info [join {gfortran __GNUC__ __GNUC_MINOR__ __GNUC_PATCHLEVEL__} -]
#endif

/* ARM seems to not define a patch version.  */
#if defined (__ARM_LINUX_COMPILER__)
set compiler_info [join {armflang __armclang_major__ __armclang_minor__ 0} -]
#endif

#if defined (__NVCOMPILER_MAJOR__)
set compiler_info [join {nvfortran __NVCOMPILER_MAJOR__ __NVCOMPILER_MINOR__ __NVCOMPILER_PATCHLEVEL__} -]
#endif

/* Classic flang and LLVM flang emit their respective macros differently.  */

/* LLVM flang complains about non Fortran tokens so we do not use "{" here.  */
#if defined (__flang__)
set major __flang_major__
set minor __flang_minor__
set patch __flang_patchlevel__
set compiler_info [join "flang-llvm $major $minor $patch" -]
#endif

/* Classic Flang.  */
#if defined (__FLANG)
set compiler_info [join {flang-classic __FLANG_MAJOR__ __FLANG_MINOR__ __FLANG_PATCHLEVEL__} -]
#endif

/* Intel LLVM emits a string like 20220100 with version 2021.2.0 and higher.  */
#if defined (__INTEL_LLVM_COMPILER)
set major [string range __INTEL_LLVM_COMPILER 0 3]
set minor [string range __INTEL_LLVM_COMPILER 4 5]
set patch [string range __INTEL_LLVM_COMPILER 6 7]
set compiler_info [join "ifx $major $minor $patch" -]
#elif defined (__INTEL_COMPILER)
/* Starting with 2021 the ifort versioning scheme changed.  Before, Intel ifort
   would define its version as e.g. 19.0.0 or rather __INTEL_COMPILER would be
   emitted as 1900.  With 2021 the versioning became e.g. 2021.1 defined in
   __INTEL_COMPILER.__INTEL_COMPILER_UPDATE.  No patch is emitted since the
   change.  This compiler identification might not work with ifort versions
   smaller than 10.  */
#if (__INTEL_COMPILER < 2021)
set major [string range __INTEL_COMPILER 0 1]
set minor [string range __INTEL_COMPILER 2 2]
#if defined (__INTEL_COMPILER_UPDATE)
set patch __INTEL_COMPILER_UPDATE
#else
set patch [string range __INTEL_COMPILER 3 3]
#endif
#else
set major __INTEL_COMPILER
set minor __INTEL_COMPILER_UPDATE
set patch 0
#endif
set compiler_info [join "ifort $major $minor $patch" -]
#endif
