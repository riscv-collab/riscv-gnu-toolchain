dnl Copyright (C) 2005-2024 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl
dnl NB: This file is included in sim/configure, so keep settings namespaced.

dnl DEPRECATED
dnl
dnl Instead of defining a `subtarget' macro, code should be checking the value
dnl of {STATE,CPU}_ARCHITECTURE to identify the architecture dnl in question.
AC_MSG_CHECKING([mips subtarget])
SIM_MIPS_SUBTARGET=
AS_CASE([${target}],
  [mips64vr*-*-*],  [SIM_MIPS_SUBTARGET="-DTARGET_ENABLE_FR=1"],
  [mips*tx39*],     [SIM_MIPS_SUBTARGET="-DSUBTARGET_R3900=1"],
  [mips*-sde-elf*], [SIM_MIPS_SUBTARGET="-DTARGET_ENABLE_FR=1"],
  [mips*-mti-elf*], [SIM_MIPS_SUBTARGET="-DTARGET_ENABLE_FR=1"],
  [mipsisa32*-*-*], [SIM_MIPS_SUBTARGET="-DTARGET_ENABLE_FR=1"],
  [mipsisa64*-*-*], [SIM_MIPS_SUBTARGET="-DTARGET_ENABLE_FR=1"])
AC_MSG_RESULT([${SIM_MIPS_SUBTARGET:-none}])
AC_SUBST(SIM_MIPS_SUBTARGET)

dnl Select the bitsize of the target.
AC_MSG_CHECKING([mips bitsize])
SIM_MIPS_BITSIZE=64
AS_CASE([${target}],
  [mips*-sde-elf*], [SIM_MIPS_BITSIZE=64],
  [mips*-mti-elf*], [SIM_MIPS_BITSIZE=64],
  [mips64*-*-*],    [SIM_MIPS_BITSIZE=64],
  [mips16*-*-*],    [SIM_MIPS_BITSIZE=64],
  [mipsisa32*-*-*], [SIM_MIPS_BITSIZE=32],
  [mipsisa64*-*-*], [SIM_MIPS_BITSIZE=64],
  [mips*-*-*],      [SIM_MIPS_BITSIZE=32])
AC_MSG_RESULT([$SIM_MIPS_BITSIZE])
AC_SUBST(SIM_MIPS_BITSIZE)

dnl Select the floating hardware support of the target.
AC_MSG_CHECKING([mips fpu bitsize])
SIM_MIPS_FPU_BITSIZE=64
AS_CASE([${target}],
  [mips*tx39*],     [SIM_MIPS_FPU_BITSIZE=32],
  [mips*-sde-elf*], [SIM_MIPS_FPU_BITSIZE=64],
  [mips*-mti-elf*], [SIM_MIPS_FPU_BITSIZE=64],
  [mipsisa32*-*-*], [SIM_MIPS_FPU_BITSIZE=64],
  [mipsisa64*-*-*], [SIM_MIPS_FPU_BITSIZE=64],
  [mips*-*-*],      [SIM_MIPS_FPU_BITSIZE=32])
AC_MSG_RESULT([$SIM_MIPS_FPU_BITSIZE])
AC_SUBST(SIM_MIPS_FPU_BITSIZE)

dnl Select the IGEN architecture.
SIM_MIPS_GEN=SINGLE
sim_mips_single_machine="-M mipsIV"
sim_mips_m16_machine="-M mips16,mipsIII"
sim_mips_single_filter="32,64,f"
sim_mips_m16_filter="16"
AS_CASE([${target}],
  [mips*tx39*], [dnl
    SIM_MIPS_GEN=SINGLE
    sim_mips_single_filter="32,f"
    sim_mips_single_machine="-M r3900"],
  [mips64vr41*], [dnl
    SIM_MIPS_GEN=M16
    sim_mips_single_machine="-M vr4100"
    sim_mips_m16_machine="-M vr4100"],
  [mips64*], [dnl
    SIM_MIPS_GEN=MULTI
    sim_mips_multi_configs="\
      vr4100:mipsIII,mips16,vr4100:32,64:mips4100,mips4111\
      vr4120:mipsIII,mips16,vr4120:32,64:mips4120\
      vr5000:mipsIV:32,64,f:mips4300,mips5000,mips8000\
      vr5400:mipsIV,vr5400:32,64,f:mips5400\
      vr5500:mipsIV,vr5500:32,64,f:mips5500"
    sim_mips_multi_default=mips5000],
  [mips*-sde-elf* | mips*-mti-elf*], [dnl
    SIM_MIPS_GEN=MULTI
    sim_mips_multi_configs="\
      micromips:micromips64,micromipsdsp:32,64,f:mips_micromips\
      mipsisa64r2:mips64r2,mips16,mips16e,mdmx,dsp,dsp2,mips3d,smartmips:32,64,f:mipsisa32r2,mipsisa64r2,mipsisa32r5,mipsisa64r5\
      mipsisa64r6:mips64r6:32,64,f:mipsisa32r6,mipsisa64r6"
    sim_mips_multi_default=mipsisa64r2],
  [mips16*], [dnl
    SIM_MIPS_GEN=M16],
  [mipsisa32r2*], [dnl
    SIM_MIPS_GEN=MULTI
    sim_mips_multi_configs="\
      micromips:micromips32,micromipsdsp:32,f:mips_micromips\
      mips32r2:mips32r2,mips3d,mips16,mips16e,mdmx,dsp,dsp2,smartmips:32,f:mipsisa32r2"
    sim_mips_multi_default=mipsisa32r2],
  [mipsisa32r6*], [dnl
    SIM_MIPS_GEN=SINGLE
    sim_mips_single_machine="-M mips32r6"
    sim_mips_single_filter="32,f"],
  [mipsisa32*], [dnl
    SIM_MIPS_GEN=M16
    sim_mips_single_machine="-M mips32,mips16,mips16e,smartmips"
    sim_mips_m16_machine="-M mips16,mips16e,mips32"
    sim_mips_single_filter="32,f"],
  [mipsisa64r2*], [dnl
    SIM_MIPS_GEN=M16
    sim_mips_single_machine="-M mips64r2,mips3d,mips16,mips16e,mdmx,dsp,dsp2"
    sim_mips_m16_machine="-M mips16,mips16e,mips64r2"],
  [mipsisa64r6*], [dnl
    SIM_MIPS_GEN=SINGLE
    sim_mips_single_machine="-M mips64r6"],
  [mipsisa64sb1*], [dnl
    SIM_MIPS_GEN=SINGLE
    sim_mips_single_machine="-M mips64,mips3d,sb1"],
  [mipsisa64*], [dnl
    SIM_MIPS_GEN=M16
    sim_mips_single_machine="-M mips64,mips3d,mips16,mips16e,mdmx"
    sim_mips_m16_machine="-M mips16,mips16e,mips64"],
  [mips*lsi*], [dnl
    SIM_MIPS_GEN=M16
    sim_mips_single_machine="-M mipsIII,mips16"
    sim_mips_m16_machine="-M mips16,mipsIII"
    sim_mips_single_filter="32,f"],
  [mips*], [dnl
    SIM_MIPS_GEN=SINGLE
    sim_mips_single_filter="32,f"])

dnl The MULTI generator can combine several simulation engines into one.
dnl executable.  A configuration which uses the MULTI should set two
dnl variables: ${sim_mips_multi_configs} and ${sim_mips_multi_default}.
dnl
dnl ${sim_mips_multi_configs} is the list of engines to build.  Each
dnl space-separated entry has the form NAME:MACHINE:FILTER:BFDMACHS,
dnl where:
dnl
dnl - NAME is a C-compatible prefix for the engine,
dnl - MACHINE is a -M argument,
dnl - FILTER is a -F argument, and
dnl - BFDMACHS is a comma-separated list of bfd machines that the
dnl     simulator can run.
dnl
dnl Each entry will have a separate simulation engine whose prefix is
dnl m32<NAME>.  If the machine list includes "mips16", there will also
dnl be a mips16 engine, prefix m16<NAME>.  The mips16 engine will be
dnl generated using the same machine list as the 32-bit version,
dnl but the filter will be "16" instead of FILTER.
dnl
dnl The simulator compares the bfd mach against BFDMACHS to decide
dnl which engine to use.  Entries in BFDMACHS should be bfd_mach
dnl values with "bfd_mach_" removed.  ${sim_mips_multi_default} says
dnl which entry should be the default.
SIM_MIPS_IGEN_ITABLE_FLAGS=
SIM_MIPS_MULTI_SRC=
SIM_MIPS_MULTI_OBJ=
SIM_MIPS_MULTI_IGEN_CONFIGS=
AS_VAR_IF([SIM_MIPS_GEN], ["MULTI"], [dnl
  dnl Verify the AS_CASE logic above is setup correctly.
  AS_IF([test -z "${sim_mips_multi_configs}" || test -z "${sim_mips_multi_default}"], [dnl
    AC_MSG_ERROR(Error in configure.ac: MULTI simulator not set up correctly)])

  dnl Start in a known state.
  AS_MKDIR_P([mips])
  rm -f mips/multi-include.h mips/multi-run.c
  sim_mips_seen_default=no

  cat << __EOF__ > mips/multi-run.c
/* Main entry point for MULTI simulators.
   Copyright (C) 2003-2023 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   --

   This file was generated by sim/mips/configure.  */

#include "sim-main.h"
#include "multi-include.h"
#include "bfd/elf-bfd.h"
#include "bfd/elfxx-mips.h"
#include "elf/mips.h"

#define SD sd
#define CPU cpu

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,
		int nr_cpus,
		int signal) /* ignore */
{
  int mach;

  if (STATE_ARCHITECTURE (sd) == NULL)
    mach = bfd_mach_${sim_mips_multi_default};
  else if (elf_elfheader (STATE_PROG_BFD (sd))->e_flags
	   & EF_MIPS_ARCH_ASE_MICROMIPS)
    mach = bfd_mach_mips_micromips;
  else
  {
     mach = _bfd_elf_mips_mach (elf_elfheader (STATE_PROG_BFD (sd))->e_flags);
     if (!mach)
       mach = STATE_ARCHITECTURE (SD)->mach;
  }

  switch (mach)
    {
__EOF__

  for fc in ${sim_mips_multi_configs}; do

    dnl Split up the entry.  ${c} contains the first three elements.
    dnl Note: outer sqaure brackets are m4 quotes.
    c=`echo ${fc} | sed ['s/:[^:]*$//']`
    bfdmachs=`echo ${fc} | sed 's/.*://'`
    name=`echo ${c} | sed 's/:.*//'`
    machine=`echo ${c} | sed 's/.*:\(.*\):.*/\1/'`
    filter=`echo ${c} | sed 's/.*://'`

    dnl Build the following lists:
    dnl
    dnl   SIM_MIPS_IGEN_ITABLE_FLAGS: all -M and -F flags used by the simulator
    dnl   SIM_MIPS_MULTI_SRC: all makefile-generated source files
    dnl   SIM_MIPS_MULTI_OBJ: the objects for ${SIM_MIPS_MULTI_SRC}
    dnl   SIM_MIPS_MULTI_IGEN_CONFIGS: igen configuration strings.
    dnl
    dnl Each entry in ${SIM_MIPS_MULTI_IGEN_CONFIGS} is a prefix (m32
    dnl or m16) followed by the NAME, MACHINE and FILTER part of
    dnl the ${sim_mips_multi_configs} entry.
    AS_VAR_APPEND([SIM_MIPS_IGEN_ITABLE_FLAGS], [" -F ${filter} -M ${machine}"])

    dnl Check whether special handling is needed.
    AS_CASE([${c}],
      [*:*mips16*:*], [dnl
	dnl Run igen twice, once for normal mode and once for mips16.
	ws="m32 m16"

	dnl The top-level function for the mips16 simulator is
	dnl in a file m16${name}_run.c, generated by the
	dnl tmp-run-multi Makefile rule.
	AS_VAR_APPEND([SIM_MIPS_MULTI_SRC], [" mips/m16${name}_run.c"])
	AS_VAR_APPEND([SIM_MIPS_MULTI_OBJ], [" mips/m16${name}_run.o"])
	AS_VAR_APPEND([SIM_MIPS_IGEN_ITABLE_FLAGS], [" -F 16"])
	],
      [*:*micromips32*:*], [dnl
	dnl Run igen thrice, once for micromips32, once for micromips16,
	dnl and once for m32.
	ws="micromips_m32 micromips16 micromips32"

	dnl The top-level function for the micromips simulator is
	dnl in a file micromips${name}_run.c, generated by the
	dnl tmp-run-multi Makefile rule.
	AS_VAR_APPEND([SIM_MIPS_MULTI_SRC], [" mips/micromips${name}_run.c"])
	AS_VAR_APPEND([SIM_MIPS_MULTI_OBJ], [" mips/micromips${name}_run.o"])
	AS_VAR_APPEND([SIM_MIPS_IGEN_ITABLE_FLAGS], [" -F 16,32"])
	],
      [*:*micromips64*:*], [dnl
	dnl Run igen thrice, once for micromips64, once for micromips16,
	dnl and once for m64.
	ws="micromips_m64 micromips16 micromips64"

	dnl The top-level function for the micromips simulator is
	dnl in a file micromips${name}_run.c, generated by the
	dnl tmp-run-multi Makefile rule.
	AS_VAR_APPEND([SIM_MIPS_MULTI_SRC], [" mips/micromips${name}_run.c"])
	AS_VAR_APPEND([SIM_MIPS_MULTI_OBJ], [" mips/micromips${name}_run.o"])
	AS_VAR_APPEND([SIM_MIPS_IGEN_ITABLE_FLAGS], [" -F 16,32,64"])
	],
      [ws=m32])

    dnl Now add the list of igen-generated files to ${SIM_MIPS_MULTI_SRC}
    dnl and ${SIM_MIPS_MULTI_OBJ}.
    for w in ${ws}; do
      for base in engine icache idecode model semantics support; do
	AS_VAR_APPEND([SIM_MIPS_MULTI_SRC], [" mips/${w}${name}_${base}.c"])
	AS_VAR_APPEND([SIM_MIPS_MULTI_SRC], [" mips/${w}${name}_${base}.h"])
	AS_VAR_APPEND([SIM_MIPS_MULTI_OBJ], [" mips/${w}${name}_${base}.o"])
      done
      AS_VAR_APPEND([SIM_MIPS_MULTI_IGEN_CONFIGS], [" ${w}${c}"])
    done

    dnl Add an include for the engine.h file.  This file declares the
    dnl top-level foo_engine_run() function.
    echo "#include \"${w}${name}_engine.h\"" >> mips/multi-include.h

    dnl Add case statements for this engine to sim_engine_run().
    for mach in `echo ${bfdmachs} | sed 's/,/ /g'`; do
      echo "    case bfd_mach_${mach}:" >> mips/multi-run.c
      AS_VAR_IF([mach], ["${sim_mips_multi_default}"], [dnl
	echo "    default:" >> mips/multi-run.c
	sim_mips_seen_default=yes
      ])
    done
    echo "      ${w}${name}_engine_run (sd, next_cpu_nr, nr_cpus, signal);" \
      >> mips/multi-run.c
    echo "      break;" >> mips/multi-run.c
  done

  dnl Check whether we added a 'default:' label.
  AS_VAR_IF([sim_mips_seen_default], [no], [dnl
    AC_MSG_ERROR(Error in configure.ac: \${sim_mips_multi_configs} doesn't have an entry for \${sim_mips_multi_default})])

  cat << __EOF__ >> mips/multi-run.c
    }
}
__EOF__
], [dnl
  SIM_MIPS_IGEN_ITABLE_FLAGS='$(SIM_MIPS_SINGLE_FLAGS)'
  AS_VAR_IF([SIM_MIPS_GEN], ["M16"], [AS_VAR_APPEND([SIM_MIPS_IGEN_ITABLE_FLAGS], [' $(SIM_MIPS_M16_FLAGS)'])])
])
SIM_MIPS_SINGLE_FLAGS="-F ${sim_mips_single_filter} ${sim_mips_single_machine}"
SIM_MIPS_M16_FLAGS="-F ${sim_mips_m16_filter} ${sim_mips_m16_machine}"
AC_SUBST(SIM_MIPS_SINGLE_FLAGS)
AC_SUBST(SIM_MIPS_M16_FLAGS)
AC_SUBST(SIM_MIPS_GEN)
AC_SUBST(SIM_MIPS_IGEN_ITABLE_FLAGS)
AC_SUBST(SIM_MIPS_MULTI_IGEN_CONFIGS)
AC_SUBST(SIM_MIPS_MULTI_SRC)
AC_SUBST(SIM_MIPS_MULTI_OBJ)
AM_CONDITIONAL([SIM_MIPS_GEN_MODE_SINGLE], [test "$SIM_MIPS_GEN" = "SINGLE"])
AM_CONDITIONAL([SIM_MIPS_GEN_MODE_M16], [test "$SIM_MIPS_GEN" = "M16"])
AM_CONDITIONAL([SIM_MIPS_GEN_MODE_MULTI], [test "$SIM_MIPS_GEN" = "MULTI"])
