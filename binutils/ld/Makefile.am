## Process this file with automake to generate Makefile.in
#
#   Copyright (C) 2012-2024 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = dejagnu no-texinfo.tex no-dist foreign info-in-builddir
ACLOCAL_AMFLAGS = -I .. -I ../config -I ../bfd
TEXINFO_TEX = ../texinfo/texinfo.tex

SUBDIRS = po

tooldir = $(exec_prefix)/$(target_alias)

YACC = `if [ -f ../bison/bison ]; then echo ../bison/bison -y -L$(srcdir)/../bison/; else echo @YACC@; fi`
YFLAGS = -d
LEX = `if [ -f ../flex/flex ]; then echo ../flex/flex; else echo @LEX@; fi`

# Automake 1.10+ disables lex and yacc output file regeneration if
# maintainer mode is disabled.  Avoid this.
am__skiplex =
am__skipyacc =

# This is where we get zlib from.  zlibdir is -L../zlib and zlibinc is
# -I../zlib, unless we were configured with --with-system-zlib, in which
# case both are empty.
ZLIB = @zlibdir@ -lz
ZLIBINC = @zlibinc@

ELF_CFLAGS=-DELF_LIST_OPTIONS=@elf_list_options@ \
	   -DELF_SHLIB_LIST_OPTIONS=@elf_shlib_list_options@ \
	   -DELF_PLT_UNWIND_LIST_OPTIONS=@elf_plt_unwind_list_options@
WARN_CFLAGS = @WARN_CFLAGS@
NO_WERROR = @NO_WERROR@
AM_CFLAGS = $(WARN_CFLAGS) $(ELF_CFLAGS) $(JANSSON_CFLAGS) $(ZSTD_CFLAGS)

# We put the scripts in the directory $(scriptdir)/ldscripts.
# We can't put the scripts in $(datadir) because the SEARCH_DIR
# directives need to be different for native and cross linkers.
scriptdir = $(tooldir)/lib

EMUL = @EMUL@
EMULATION_OFILES = @EMULATION_OFILES@
EMUL_EXTRA_OFILES = @EMUL_EXTRA_OFILES@


# Search path to override the default search path for -lfoo libraries.
# If LIB_PATH is empty, the ones in the script (if any) are left alone.
# (The default is usually /lib:/usr/lib:/usr/local/lib, unless building
# a cross-linker, in which case the default is empty.  See genscripts.sh.)
# Otherwise, they are replaced with the ones given in LIB_PATH,
# which may have the form: LIB_PATH=/lib:/usr/local/lib.  This can be set
# when the linker is configured via the --with-lib-path configure switch.
LIB_PATH = @LIB_PATH@

BASEDIR = $(srcdir)/..
BFDDIR = $(BASEDIR)/bfd
INCDIR	= $(BASEDIR)/include

# What version of the manual to build
DOCVER = gen

# Options to extract the man page from ld.texi
MANCONF = -Dman

TEXI2POD = perl $(BASEDIR)/etc/texi2pod.pl $(AM_MAKEINFOFLAGS)

POD2MAN = pod2man --center="GNU Development Tools" \
	--release="binutils-$(VERSION)" --section=1

# Setup the testing framework, if you have one
EXPECT = expect
RUNTEST = runtest
RUNTESTFLAGS =

CC_FOR_TARGET = ` \
  if [ -f $$r/../gcc/xgcc ] ; then \
    if [ -f $$r/../newlib/Makefile ] ; then \
      echo $$r/../gcc/xgcc -B$$r/../gcc/ -idirafter $$r/../newlib/targ-include -idirafter $${srcroot}/../newlib/libc/include -nostdinc; \
    else \
      echo $$r/../gcc/xgcc -B$$r/../gcc/; \
    fi; \
  else \
    if [ "@host@" = "@target@" ] ; then \
      echo $(CC); \
    else \
      echo gcc | sed '$(transform)'; \
    fi; \
  fi`

CXX_FOR_TARGET = ` \
  if [ -f $$r/../gcc/g++ ] ; then \
    if [ -f $$r/../newlib/Makefile ] ; then \
      echo $$r/../gcc/g++ -B$$r/../gcc/ -idirafter $$r/../newlib/targ-include -idirafter $${srcroot}/../newlib/libc/include -nostdinc; \
    else \
      echo $$r/../gcc/g++ -B$$r/../gcc/; \
    fi; \
  elif [ -f $$r/../gcc/xg++ ] ; then \
    if [ -f $$r/../newlib/Makefile ] ; then \
      echo $$r/../gcc/xg++ -B$$r/../gcc/ -idirafter $$r/../newlib/targ-include -idirafter $${srcroot}/../newlib/libc/include -nostdinc; \
    else \
      echo $$r/../gcc/xg++ -B$$r/../gcc/; \
    fi; \
  else \
    if [ "@host@" = "@target@" ] ; then \
      echo $(CXX); \
    else \
      echo g++ | sed '$(transform)'; \
    fi; \
  fi`

# Strip out sanitization options as we want to test building binaries without any extra paraphernalia
CFLAGS_FOR_TARGET = `echo $(CFLAGS) | sed -e 's/-fsanitize=[^ ]*//g'`
CXXFLAGS_FOR_TARGET = `echo $(CXXFLAGS) | sed -e 's/-fsanitize=[^ ]*//g'`

transform = s/^ld-new$$/$(installed_linker)/;@program_transform_name@
bin_PROGRAMS = ld-new
info_TEXINFOS = ld.texi ldint.texi
ld_TEXINFOS = configdoc.texi
noinst_TEXINFOS = ldint.texi
man_MANS = ld.1

AM_MAKEINFOFLAGS = -I $(srcdir) -I $(BFDDIR)/doc -I ../bfd/doc \
		   -I $(top_srcdir)/../libiberty --no-split
TEXI2DVI = texi2dvi -I $(srcdir) -I $(BFDDIR)/doc -I ../bfd/doc \
		    -I $(top_srcdir)/../libiberty

AM_CPPFLAGS = -I. -I$(srcdir) -I../bfd -I$(BFDDIR) -I$(INCDIR) @zlibinc@ \
	@INCINTL@ $(HDEFINES) $(CFLAGS) @LARGEFILE_CPPFLAGS@ \
	-DLOCALEDIR="\"$(datadir)/locale\""

BFDLIB = ../bfd/libbfd.la
LIBIBERTY = ../libiberty/libiberty.a
if ENABLE_LIBCTF
LIBCTF = ../libctf/libctf.la
else
LIBCTF =
endif

# These all start with e so 'make clean' can find them.
ALL_EMULATION_SOURCES = \
	eaix5ppc.c \
	eaix5rs6.c \
	eaixppc.c \
	eaixrs6.c \
	ealpha.c \
	ealphavms.c \
	earcelf.c \
	earclinux.c \
	earclinux_nps.c \
	earcv2elf.c \
	earcv2elfx.c \
	earm_wince_pe.c \
	earmelf.c \
	earmelf_fbsd.c \
	earmelf_fuchsia.c \
	earmelf_haiku.c \
	earmelf_linux.c \
	earmelf_linux_eabi.c \
	earmelf_linux_fdpiceabi.c \
	earmelf_nacl.c \
	earmelf_nbsd.c \
	earmelf_phoenix.c \
	earmelf_vxworks.c \
	earmelfb.c \
	earmelfb_fbsd.c \
	earmelfb_fuchsia.c \
	earmelfb_linux.c \
	earmelfb_linux_eabi.c \
	earmelfb_linux_fdpiceabi.c \
	earmelfb_nacl.c \
	earmelfb_nbsd.c \
	earmnto.c \
	earmpe.c \
	eavr1.c \
	eavr2.c \
	eavr25.c \
	eavr3.c \
	eavr31.c \
	eavr35.c \
	eavr4.c \
	eavr5.c \
	eavr51.c \
	eavr6.c \
	eavrtiny.c   \
	eavrxmega1.c \
	eavrxmega2.c \
	eavrxmega2_flmap.c \
	eavrxmega3.c \
	eavrxmega4.c \
	eavrxmega4_flmap.c \
	eavrxmega5.c \
	eavrxmega6.c \
	eavrxmega7.c \
	ecrisaout.c \
	ecriself.c \
	ecrislinux.c \
	ecskyelf.c \
	ecskyelf_linux.c \
	ed10velf.c \
	ed30v_e.c \
	ed30v_o.c \
	ed30velf.c \
	eelf32_dlx.c \
	eelf32_sparc.c \
	eelf32_sparc_sol2.c \
	eelf32_sparc_vxworks.c \
	eelf32_spu.c \
	eelf32_tic6x_be.c \
	eelf32_tic6x_elf_be.c \
	eelf32_tic6x_elf_le.c \
	eelf32_tic6x_le.c \
	eelf32_tic6x_linux_be.c \
	eelf32_tic6x_linux_le.c \
	eelf32am33lin.c \
	eelf32bfin.c \
	eelf32bfinfd.c \
	eelf32cr16.c \
	eelf32crx.c \
	eelf32epiphany.c \
	eelf32epiphany_4x4.c \
	eelf32fr30.c \
	eelf32frv.c \
	eelf32frvfd.c \
	eelf32ft32.c \
	eelf32ip2k.c \
	eelf32iq10.c \
	eelf32iq2000.c \
	eelf32lm32.c \
	eelf32lm32fd.c \
	eelf32lppc.c \
	eelf32lppclinux.c \
	eelf32lppcnto.c \
	eelf32lppcsim.c \
	eelf32m32c.c \
	eelf32mb_linux.c \
	eelf32mbel_linux.c \
	eelf32mcore.c \
	eelf32mep.c \
	eelf32metag.c \
	eelf32microblaze.c \
	eelf32microblazeel.c \
	eelf32moxie.c \
	eelf32mt.c \
	eelf32or1k.c \
	eelf32or1k_linux.c \
	eelf32ppc.c \
	eelf32ppc_fbsd.c \
	eelf32ppchaiku.c \
	eelf32ppclinux.c \
	eelf32ppcnto.c \
	eelf32ppcsim.c \
	eelf32ppcvxworks.c \
	eelf32ppcwindiss.c \
	eelf32rl78.c \
	eelf32rx.c \
	eelf32rx_linux.c \
	eelf32tilegx.c \
	eelf32tilegx_be.c \
	eelf32tilepro.c \
	eelf32vax.c \
	eelf32visium.c \
	eelf32xstormy16.c \
	eelf32xtensa.c \
	eelf32z80.c \
	eelf_i386.c \
	eelf_i386_be.c \
	eelf_i386_fbsd.c \
	eelf_i386_haiku.c \
	eelf_i386_ldso.c \
	eelf_i386_sol2.c \
	eelf_i386_vxworks.c \
	eelf_iamcu.c \
	eelf_s390.c \
	eh8300elf.c \
	eh8300elf_linux.c \
	eh8300helf.c \
	eh8300helf_linux.c \
	eh8300hnelf.c \
	eh8300self.c \
	eh8300self_linux.c \
	eh8300snelf.c \
	eh8300sxelf.c \
	eh8300sxelf_linux.c \
	eh8300sxnelf.c \
	ehppaelf.c \
	ehppalinux.c \
	ehppanbsd.c \
	ehppaobsd.c \
	ei386aout.c \
	ei386beos.c \
	ei386bsd.c \
	ei386go32.c \
	ei386lynx.c \
	ei386moss.c \
	ei386msdos.c \
	ei386nto.c \
	ei386pe.c \
	ei386pe_posix.c \
	em32relf.c \
	em32relf_linux.c \
	em32rlelf.c \
	em32rlelf_linux.c \
	em68hc11elf.c \
	em68hc11elfb.c \
	em68hc12elf.c \
	em68hc12elfb.c \
	em68kelf.c \
	em68kelfnbsd.c \
	em9s12zelf.c \
	emcorepe.c \
	emn10200.c \
	emn10300.c \
	emoxiebox.c \
	emsp430X.c \
	emsp430elf.c \
	ends32belf.c \
	ends32belf16m.c \
	ends32belf_linux.c \
	ends32elf.c \
	ends32elf16m.c \
	ends32elf_linux.c \
	enios2elf.c \
	enios2linux.c \
	ens32knbsd.c \
	epc532macha.c \
	epdp11.c \
	epjelf.c \
	epjlelf.c \
	eppcmacos.c \
	epruelf.c \
	escore3_elf.c \
	escore7_elf.c \
	esh.c \
	eshelf.c \
	eshelf_fd.c \
	eshelf_linux.c \
	eshelf_nbsd.c \
	eshelf_nto.c \
	eshelf_uclinux.c \
	eshelf_vxworks.c \
	eshl.c \
	eshlelf.c \
	eshlelf_fd.c \
	eshlelf_linux.c \
	eshlelf_nbsd.c \
	eshlelf_nto.c \
	eshlelf_vxworks.c \
	eshpe.c \
	etic30coff.c \
	etic3xcoff.c \
	etic3xcoff_onchip.c \
	etic4xcoff.c \
	etic54xcoff.c \
	ev850.c \
	ev850_rh850.c \
	evanilla.c \
	evaxnbsd.c \
	exgateelf.c \
	ez80.c \
	ez8001.c \
	ez8002.c

ALL_EMULATIONS = $(ALL_EMULATION_SOURCES:.c=.@OBJEXT@)

ALL_64_EMULATION_SOURCES = \
	eaarch64cloudabi.c \
	eaarch64cloudabib.c \
	eaarch64elf.c \
	eaarch64elf32.c \
	eaarch64elf32b.c \
	eaarch64elfb.c \
	eaarch64fbsd.c \
	eaarch64fbsdb.c \
	eaarch64haiku.c \
	eaarch64linux.c \
	eaarch64linux32.c \
	eaarch64linux32b.c \
	eaarch64linuxb.c \
	eaarch64nto.c \
	eaarch64pe.c \
	earm64pe.c \
	eelf32_x86_64.c \
	eelf32b4300.c \
	eelf32bmip.c \
	eelf32bmipn32.c \
	eelf32briscv.c \
	eelf32briscv_ilp32.c \
	eelf32briscv_ilp32f.c \
	eelf32bsmip.c \
	eelf32btsmip.c \
	eelf32btsmip_fbsd.c \
	eelf32btsmipn32.c \
	eelf32btsmipn32_fbsd.c \
	eelf32ebmip.c \
	eelf32ebmipvxworks.c \
	eelf32elmip.c \
	eelf32elmipvxworks.c \
	eelf32kvx.c \
	eelf32l4300.c \
	eelf32lmip.c \
	eelf32loongarch.c \
	eelf32lr5900.c \
	eelf32lr5900n32.c \
	eelf32lriscv.c \
	eelf32lriscv_ilp32.c \
	eelf32lriscv_ilp32f.c \
	eelf32lsmip.c \
	eelf32ltsmip.c \
	eelf32ltsmip_fbsd.c \
	eelf32ltsmipn32.c \
	eelf32ltsmipn32_fbsd.c \
	eelf32mipswindiss.c \
	eelf64_aix.c \
	eelf64_ia64.c \
	eelf64_ia64_fbsd.c \
	eelf64_ia64_vms.c \
	eelf64_s390.c \
	eelf64_sparc.c \
	eelf64_sparc_fbsd.c \
	eelf64_sparc_sol2.c \
	eelf64alpha.c \
	eelf64alpha_fbsd.c \
	eelf64alpha_nbsd.c \
	eelf64bmip.c \
	eelf64bpf.c \
	eelf64briscv.c \
	eelf64briscv_lp64.c \
	eelf64briscv_lp64f.c \
	eelf64btsmip.c \
	eelf64btsmip_fbsd.c \
	eelf64hppa.c \
	eelf64kvx.c \
	eelf64kvx_linux.c \
	eelf64loongarch.c \
	eelf64lppc.c \
	eelf64lppc_fbsd.c \
	eelf64lriscv.c \
	eelf64lriscv_lp64.c \
	eelf64lriscv_lp64f.c \
	eelf64ltsmip.c \
	eelf64ltsmip_fbsd.c \
	eelf64mmix.c \
	eelf64ppc.c \
	eelf64ppc_fbsd.c \
	eelf64rdos.c \
	eelf64tilegx.c \
	eelf64tilegx_be.c \
	eelf_mipsel_haiku.c \
	eelf_x86_64.c \
	eelf_x86_64_cloudabi.c \
	eelf_x86_64_fbsd.c \
	eelf_x86_64_haiku.c \
	eelf_x86_64_sol2.c \
	ehppa64linux.c \
	ei386pep.c \
	emmo.c

ALL_64_EMULATIONS = $(ALL_64_EMULATION_SOURCES:.c=.@OBJEXT@)

ALL_EMUL_EXTRA_OFILES = \
	deffilep.@OBJEXT@ \
	pdb.@OBJEXT@ \
	pe-dll.@OBJEXT@ \
	ldelf.@OBJEXT@ \
	ldelfgen.@OBJEXT@

ALL_64_EMUL_EXTRA_OFILES = \
	pep-dll.@OBJEXT@

CFILES = ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c \
	ldmain.c ldmisc.c ldver.c ldwrite.c lexsup.c \
	mri.c ldcref.c pe-dll.c pep-dll.c ldlex-wrapper.c \
	plugin.c ldbuildid.c ldelf.c ldelfgen.c \
	pdb.c

HFILES = ld.h ldctor.h ldemul.h ldexp.h ldfile.h \
	ldlang.h ldlex.h ldmain.h ldmisc.h ldver.h \
	ldwrite.h mri.h deffile.h pe-dll.h pep-dll.h \
	elf-hints-local.h plugin.h ldbuildid.h ldelf.h ldelfgen.h \
	pdb.h

GENERATED_CFILES = ldgram.c ldlex.c deffilep.c
GENERATED_HFILES = ldgram.h ldemul-list.h deffilep.h

# Require an early dependency on the generated headers, as the dependency
# tracking will not cause them to be built beforehand.
BUILT_SOURCES = $(GENERATED_HFILES) @MAINT@ $(GENERATED_CFILES) \
	$(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES)

OFILES = ldgram.@OBJEXT@ ldlex-wrapper.@OBJEXT@ lexsup.@OBJEXT@ ldlang.@OBJEXT@ \
	mri.@OBJEXT@ ldctor.@OBJEXT@ ldmain.@OBJEXT@ plugin.@OBJEXT@ \
	ldwrite.@OBJEXT@ ldexp.@OBJEXT@  ldemul.@OBJEXT@ ldver.@OBJEXT@ ldmisc.@OBJEXT@ \
	ldfile.@OBJEXT@ ldcref.@OBJEXT@ ${EMULATION_OFILES} ${EMUL_EXTRA_OFILES} \
	ldbuildid.@OBJEXT@

STAGESTUFF = *.@OBJEXT@ ldscripts/* e*.c

# Disable -Werror, if it has been enabled, since old versions of bison/
# yacc will produce working code which contain compile time warnings.
ldgram.@OBJEXT@: ldgram.c
if am__fastdepCC
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f ldgram.c || echo $(srcdir)/`ldgram.c $(NO_WERROR)
	$(AM_V_at)mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldgram.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(AM_V_CC)$(COMPILE) -c `test -f ldgram.c || echo $(srcdir)/`ldgram.c $(NO_WERROR)
endif

ldlex-wrapper.@OBJEXT@: ldlex-wrapper.c ldlex.c
if am__fastdepCC
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $(srcdir)/ldlex-wrapper.c $(NO_WERROR)
	$(AM_V_at)mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldlex-wrapper.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(AM_V_CC)$(COMPILE) -c $(srcdir)/ldlex-wrapper.c $(NO_WERROR)
endif

deffilep.@OBJEXT@: deffilep.c
if am__fastdepCC
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ `test -f deffilep.c || echo $(srcdir)/`deffilep.c $(NO_WERROR)
	$(AM_V_at)mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='deffilep.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(AM_V_CC)$(COMPILE) -c `test -f deffilep.c || echo $(srcdir)/`deffilep.c $(NO_WERROR)
endif

SRC_POTFILES = $(CFILES) $(HFILES)
BLD_POTFILES = $(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES)

po/SRC-POTFILES.in: @MAINT@ Makefile
	for f in $(SRC_POTFILES); do echo $$f; done | LC_ALL=C sort > $@-tmp \
	  && mv $@-tmp $(srcdir)/po/SRC-POTFILES.in

po/BLD-POTFILES.in: @MAINT@ Makefile
	for f in $(BLD_POTFILES); do echo $$f; done | LC_ALL=C sort > $@-tmp \
	  && mv $@-tmp $(srcdir)/po/BLD-POTFILES.in

ldmain.@OBJEXT@: ldmain.c config.status
if am__fastdepCC
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	  -DDEFAULT_EMULATION='"$(EMUL)"' \
	  -DBINDIR='"$(bindir)"' -DTOOLBINDIR='"$(tooldir)/bin"' \
	  -DTARGET='"@target@"' @TARGET_SYSTEM_ROOT_DEFINE@ \
	  $(srcdir)/ldmain.c
	$(AM_V_at)mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldmain.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(AM_V_CC)$(COMPILE) -c -DDEFAULT_EMULATION='"$(EMUL)"' \
	  -DBINDIR='"$(bindir)"' -DTOOLBINDIR='"$(tooldir)/bin"' \
	  -DTARGET='"@target@"' @TARGET_SYSTEM_ROOT_DEFINE@ \
	  $(srcdir)/ldmain.c
endif

ldfile.@OBJEXT@: ldfile.c config.status
if am__fastdepCC
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	-DSCRIPTDIR='"$(scriptdir)"' -DBINDIR='"$(bindir)"' -DTOOLBINDIR='"$(tooldir)/bin"' \
	 $(srcdir)/ldfile.c
	$(AM_V_at)mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='ldfile.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(AM_V_CC)$(COMPILE) -c -DSCRIPTDIR='"$(scriptdir)"' -DBINDIR='"$(bindir)"' \
	  -DTOOLBINDIR='"$(tooldir)/bin"' \
	  $(srcdir)/ldfile.c
endif

eelf32_spu.@OBJEXT@: eelf32_spu.c
if am__fastdepCC
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ \
	  -DEMBEDSPU="\"`echo embedspu | sed '$(transform)'`\"" eelf32_spu.c
	$(AM_V_at)mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
else
if AMDEP
	source='eelf32_spu.c' object='$@' libtool=no @AMDEPBACKSLASH@
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) @AMDEPBACKSLASH@
endif
	$(AM_V_CC)$(COMPILE) -c -DEMBEDSPU="\"`echo embedspu | sed '$(transform)'`\"" \
	  eelf32_spu.c
endif

ldemul-list.h: Makefile
	(echo "/* This file is automatically generated.  DO NOT EDIT! */";\
	for f in `echo " " ${EMULATION_OFILES} "" \
	 | sed -e 's/ e/ ld/g' -e 's/ ld/ /g' -e 's/[.]o//g'`; do \
	    echo "extern ld_emulation_xfer_type ld_$${f}_emulation;"; \
	done;\
	echo "";\
	echo "#define EMULATION_LIST \\";\
	for f in `echo " " ${EMULATION_OFILES} "" \
	 | sed -e 's/ e/ ld/g' -e 's/ ld/ /g' -e 's/[.]o//g'`; do \
	    echo "  &ld_$${f}_emulation, \\"; \
	done;\
	echo "  0") >ldemul-tmp.h
	mv ldemul-tmp.h ldemul-list.h

ldscripts/stamp:
	$(AM_V_GEN)test -d $(@D) || mkdir $(@D); touch $@

if AMDEP
  GENDEPDIR=$(DEPDIR)
else
  GENDEPDIR=
endif
GENSCRIPTS = $(SHELL) $(srcdir)/genscripts.sh "${srcdir}" "${libdir}" "${prefix}" "${exec_prefix}" @host@ @target@ @target_alias@ "$(GENDEPDIR)" "${LIB_PATH}" "@EMULATION_LIBPATH@" "@NATIVE_LIB_DIRS@" @use_sysroot@ @enable_initfini_array@
GEN_DEPENDS = $(srcdir)/genscripts.sh ldscripts/stamp

@TDIRS@

e%.c:
	$(AM_V_GEN)${GENSCRIPTS} $* "$(tdir_$*)"

$(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES): $(GEN_DEPENDS)

#Dummy rule to prevent make considering e%.c as a source for missing
#.Pc files (and they are all missing until the e*.c files are generated).
#For example, we don't want e.deps/emmo.Pc.c to be tried when
#.deps/emmo.Pc is missing.
%.Pc:

@AMDEP_TRUE@-include $(patsubst %.c,./$(DEPDIR)/%.Pc,$(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES))

# We need this for automake to use YLWRAP.
EXTRA_ld_new_SOURCES = deffilep.y ldlex.l
# Allow dependency tracking to work for these files, too.
EXTRA_ld_new_SOURCES += ldelf.c ldelfgen.c pdb.c pep-dll.c pe-dll.c \
	 pep-dll-aarch64.c pep-dll-x86_64.c

ld_new_SOURCES = ldgram.y ldlex-wrapper.c lexsup.c ldlang.c mri.c ldctor.c ldmain.c \
	ldwrite.c ldexp.c ldemul.c ldver.c ldmisc.c ldfile.c ldcref.c plugin.c \
	ldbuildid.c
ld_new_DEPENDENCIES = $(EMULATION_OFILES) $(EMUL_EXTRA_OFILES) \
		      $(BFDLIB) $(LIBCTF) $(LIBIBERTY) $(LIBINTL_DEP) $(JANSSON_LIBS)
ld_new_LDADD = $(EMULATION_OFILES) $(EMUL_EXTRA_OFILES) $(BFDLIB) $(LIBCTF) \
	       $(LIBIBERTY) $(LIBINTL) $(ZLIB) $(ZSTD_LIBS) $(JANSSON_LIBS)

# Dependency tracking for the generated emulation files.
EXTRA_ld_new_SOURCES += $(ALL_EMULATION_SOURCES) $(ALL_64_EMULATION_SOURCES)

# This is the real libbfd.a and libctf.a created by libtool.
TESTBFDLIB = @TESTBFDLIB@
TESTCTFLIB = @TESTCTFLIB@
TESTSFRAMELIB = @TESTSFRAMELIB@

check-DEJAGNU: site.exp
	(cd .libs; test -e ldscripts || test ! -e ld-new || $(LN_S) ../ldscripts .)
	srcroot=`cd $(srcdir) && pwd`; export srcroot; \
	r=`pwd`; export r; \
	LC_ALL=C; export LC_ALL; \
	EXPECT=$(EXPECT); export EXPECT; \
	runtest=$(RUNTEST); \
	if $(SHELL) -c "$$runtest --version" > /dev/null 2>&1; then \
	  $$runtest --tool $(DEJATOOL) --srcdir $${srcroot}/testsuite \
		CC="$(CC)" CFLAGS="$(CFLAGS)" \
		CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" \
		CC_FOR_TARGET="$(CC_FOR_TARGET)" \
		CFLAGS_FOR_TARGET="$(filter-out -ffile-prefix-map=%,$(CFLAGS_FOR_TARGET))" \
		CXX_FOR_TARGET="$(CXX_FOR_TARGET)" \
		CXXFLAGS_FOR_TARGET="$(filter-out -ffile-prefix-map=%,$(CXXFLAGS_FOR_TARGET))" \
		OFILES="$(OFILES)" BFDLIB="$(TESTBFDLIB)" CTFLIB="$(TESTCTFLIB) $(ZLIB)" \
		SFRAMELIB="$(TESTSFRAMELIB)" \
		JANSSON_LIBS="$(JANSSON_LIBS)" ZSTD_LIBS="$(ZSTD_LIBS)" \
		LIBIBERTY="$(LIBIBERTY) $(LIBINTL)" LIBS="$(LIBS)" \
		DO_COMPARE="`echo '$(do_compare)' | sed -e 's,\\$$,,g'`" \
		$(RUNTESTFLAGS); \
	else echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi

development.exp: $(BFDDIR)/development.sh
	$(AM_V_GEN)$(EGREP) "(development|experimental)=" $(BFDDIR)/development.sh  \
	  | $(AWK) -F= '{ print "set " $$1 " " $$2 }' > $@

enablings.exp:
	$(AM_V_GEN)echo "set enable_libctf ${enable_libctf}" >> $@
#
#
# Build a dummy plugin using libtool.
#
noinst_LTLIBRARIES = libldtestplug.la libldtestplug2.la \
		     libldtestplug3.la libldtestplug4.la
libldtestplug_la_SOURCES = testplug.c
libldtestplug_la_CFLAGS= -g -O2
libldtestplug_la_LDFLAGS = -no-undefined -rpath /nowhere
libldtestplug2_la_SOURCES = testplug2.c
libldtestplug2_la_CFLAGS= -g -O2
libldtestplug2_la_LDFLAGS = -no-undefined -rpath /nowhere
libldtestplug3_la_SOURCES = testplug3.c
libldtestplug3_la_CFLAGS= -g -O2
libldtestplug3_la_LDFLAGS = -no-undefined -rpath /nowhere
libldtestplug4_la_SOURCES = testplug4.c
libldtestplug4_la_CFLAGS= -g -O2
libldtestplug4_la_LDFLAGS = -no-undefined -rpath /nowhere

bfdplugindir = $(libdir)/bfd-plugins
bfdplugin_LTLIBRARIES = libdep.la
libdep_la_SOURCES = libdep_plugin.c
libdep_la_LDFLAGS = -no-undefined -rpath /nowhere -module -avoid-version
libdep_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) \
	--tag=disable-static \
	$(LIBTOOLFLAGS) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
	$(libdep_la_LDFLAGS) $(LDFLAGS) -o $@

# DOCUMENTATION TARGETS
# Manual configuration file; not usually attached to normal configuration,
# because almost all configs use "gen" version of manual.
#  Set DOCVER above to change.
configdoc.texi:	${DOCVER}-doc.texi
	$(AM_V_GEN)cp ${srcdir}/${DOCVER}-doc.texi ./configdoc.texi
	$(AM_V_at)chmod u+w ./configdoc.texi

# Build the man page from the texinfo file
# The sed command removes the no-adjust Nroff command so that
# the man output looks standard.
ld.1: $(srcdir)/ld.texi configdoc.texi
	$(AM_V_GEN)touch $@
	$(AM_V_at)-$(TEXI2POD) $(MANCONF) < $(srcdir)/ld.texi > ld.pod
	$(AM_V_at)-($(POD2MAN) ld.pod | \
		sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || \
		(rm -f $@.T$$$$ && exit 1)
	$(AM_V_at)rm -f ld.pod

# Build the man page from the texinfo file
# The sed command removes the no-adjust Nroff command so that
# the man output looks standard.
ldint.1: $(srcdir)/ldint.texi configdoc.texi
	$(AM_V_GEN)touch $@
	$(AM_V_at)-$(TEXI2POD) $(MANCONF) < $(srcdir)/ldint.texi > ldint.pod
	$(AM_V_at)-($(POD2MAN) ldint.pod | \
		sed -e '/^.if n .na/d' > $@.T$$$$ && \
		mv -f $@.T$$$$ $@) || \
		(rm -f $@.T$$$$ && exit 1)
	$(AM_V_at)rm -f ldint.pod

MAINTAINERCLEANFILES = configdoc.texi ld.1 ldint.1

# We want to reconfigure if configure.host or configure.tgt changes.
# development.sh is used to determine -Werror default.
CONFIG_STATUS_DEPENDENCIES = $(srcdir)/configure.host $(srcdir)/configure.tgt \
	$(BFDDIR)/development.sh

EXTRA_DEJAGNU_SITE_CONFIG = development.exp enablings.exp

MOSTLYCLEANFILES = $(STAGESTUFF) ld1$(EXEEXT) ld2$(EXEEXT) ld3$(EXEEXT) \
	ldemul-list.h crtbegin.@OBJEXT@ crtend.@OBJEXT@ ld.log ld.sum
mostlyclean-local:
	-rm -rf tmpdir
CLEANFILES = dep.sed DEP DEPA DEP1 DEP2 spu_ovl.s spu_ovl.@OBJEXT@ spu_icache.s spu_icache.@OBJEXT@

.PHONY: install-exec-local install-data-local

install-exec-local: ld-new$(EXEEXT) install-binPROGRAMS
	$(mkinstalldirs) $(DESTDIR)$(tooldir)/bin
	n=`echo $(installed_linker) | sed '$(transform)'`; \
	if test "$(bindir)" != "$(tooldir)/bin"; then \
	  rm -f $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/$(installed_linker)$(EXEEXT); \
	fi; \
	if test "x$(install_as_default)" = "xyes"; then \
	  ld=`echo ld | sed '$(transform)'`; \
	  rm -f $(DESTDIR)$(bindir)/$$ld$(EXEEXT); \
	  ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(bindir)/$$ld$(EXEEXT) >/dev/null 2>/dev/null \
	  || $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(bindir)/$$ld$(EXEEXT); \
	  if test "$(bindir)" != "$(tooldir)/bin"; then \
	    rm -f $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	    ln $(DESTDIR)$(bindir)/$$n$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT) >/dev/null 2>/dev/null \
	    || $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) ld-new$(EXEEXT) $(DESTDIR)$(tooldir)/bin/ld$(EXEEXT); \
	  fi; \
	fi

install-data-local: install-bfdpluginLTLIBRARIES
	$(mkinstalldirs) $(DESTDIR)$(scriptdir)/ldscripts
	for f in ldscripts/* ; do \
	  case $$f in \
	    stamp) ;; \
	    *) $(INSTALL_DATA) $$f $(DESTDIR)$(scriptdir)/$$f ; \
	  esac \
	done
	rm -f $(DESTDIR)$(bfdplugindir)/libdep.la
	rm -f $(DESTDIR)$(bfdplugindir)/libdep.dll.a

# Stuff that should be included in a distribution.  The diststuff
# target is run by the taz target in ../Makefile.in.
EXTRA_DIST = ldgram.c ldgram.h ldlex.c emultempl/spu_ovl.@OBJEXT@_c \
	     emultempl/spu_icache.@OBJEXT@_c deffilep.c deffilep.h $(man_MANS)
diststuff: info $(EXTRA_DIST)

# Both info (ld.info) and ld.1 depend on configdoc.texi.
# But info isn't a direct target. Make info-recursive to depend on
# ld.1 to support parallel build.
info-recursive: ld.1

html-local: doc/ld/index.html
doc/ld/index.html: ld.texi $(ld_TEXINFOS)
	$(AM_V_at)$(MKDIR_P) doc
	$(AM_V_GEN)$(MAKEINFOHTML) $(AM_MAKEINFOHTMLFLAGS) $(MAKEINFOFLAGS) \
	  --split=node -I$(srcdir) $(srcdir)/ld.texi -o doc/ld

DISTCLEANFILES = site.exp development.exp enablings.exp site.bak
distclean-local:
	rm -rf ldscripts

MAINTAINERCLEANFILES += ld.info
