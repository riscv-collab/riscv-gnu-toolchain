# Copyright (C) 2007-2024 Free Software Foundation, Inc.

# This file is part of GDB.

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


# This file requires GNU make!

# This Makefile updates files in ../regformats from their XML master
# copies.  Because this relies on xsltproc, it is not normally run
# during the build process; it should be run by hand when one of the
# affected XML files is changed, and the results should be kept in the
# GDB repository.

# It can also update the C files in the features directory from their
# XML master copies.  This relies on a GDB linked with expat and that
# is configured to include support for the architecture, so the files
# are kept in the GDB repository.
#
# By default we'll generate all C files, and you must point make at a
# GDB that has been configured with --enable-targets=all:
#
#   make GDB=/path/to/gdb cfiles
#
# To regenerate all C files from scratch:
#
#   make GDB=/path/to/gdb clean-cfiles cfiles
#
# To generate specific C files, you can override the XMLTOC and
# FEATURE_XMLFILES make variables:
#
#   make GDB=/path/to/gdb XMLTOC="xml files" FEATURE_XMLFILES="xml files" cfiles

# List of .dat files to create in ../regformats/
WHICH = mips-linux mips-dsp-linux \
	mips64-linux mips64-dsp-linux \
	nios2-linux \
	or1k-linux \
	rs6000/powerpc-32 \
	rs6000/powerpc-32l rs6000/powerpc-altivec32l rs6000/powerpc-e500l \
	rs6000/powerpc-64l rs6000/powerpc-altivec64l rs6000/powerpc-vsx32l \
	rs6000/powerpc-vsx64l \
	rs6000/powerpc-isa205-32l rs6000/powerpc-isa205-64l \
	rs6000/powerpc-isa205-altivec32l rs6000/powerpc-isa205-altivec64l \
	rs6000/powerpc-isa205-vsx32l rs6000/powerpc-isa205-vsx64l \
	rs6000/powerpc-isa205-ppr-dscr-vsx32l \
	rs6000/powerpc-isa205-ppr-dscr-vsx64l \
	rs6000/powerpc-isa207-vsx32l rs6000/powerpc-isa207-vsx64l \
	rs6000/powerpc-isa207-htm-vsx32l \
	rs6000/powerpc-isa207-htm-vsx64l \
	s390-linux32 s390-linux64 s390x-linux64 \
	s390-linux32v1 s390-linux64v1 s390x-linux64v1 \
	s390-linux32v2 s390-linux64v2 s390x-linux64v2 \
	s390-te-linux64 s390x-te-linux64 s390-vx-linux64 s390x-vx-linux64 \
	s390-tevx-linux64 s390x-tevx-linux64 \
	s390-gs-linux64 s390x-gs-linux64 \
	tic6x-c64xp-linux tic6x-c64x-linux tic6x-c62x-linux

# Record which registers should be sent to GDB by default after stop.
mips-expedite = r29,pc
mips-dsp-expedite = r29,pc
mips64-expedite = r29,pc
mips64-dsp-expedite = r29,pc
nios2-linux-expedite = sp,pc
or1k-expedite = r1,npc
powerpc-expedite = r1,pc
s390-linux32-expedite = r14,r15,pswa
s390-linux32v1-expedite = r14,r15,pswa
s390-linux32v2-expedite = r14,r15,pswa
s390-linux64-expedite = r14l,r15l,pswa
s390-linux64v1-expedite = r14l,r15l,pswa
s390-linux64v2-expedite = r14l,r15l,pswa
s390-te-linux64-expedite = r14l,r15l,pswa
s390-vx-linux64-expedite = r14l,r15l,pswa
s390-tevx-linux64-expedite = r14l,r15l,pswa
s390-gs-linux64-expedite = r14,r15,pswa
s390x-expedite = r14,r15,pswa
tic6x-expedite = A15,PC


XSLTPROC = xsltproc
outdir = ../regformats
OUTPUTS = $(patsubst %,$(outdir)/%.dat,$(WHICH))

# For targets without feature based target descriptions,
# the set of xml files we'll generate .c files for GDB from.  By
# default we'll build all .c files, which requires an
# --enable-targets=all GDB.  You can override this by passing XMLTOC
# to make on the command line.
XMLTOC = \
	microblaze-with-stack-protect.xml \
	microblaze.xml \
	mips-dsp-linux.xml \
	mips-linux.xml \
	mips64-dsp-linux.xml \
	mips64-linux.xml \
	nds32.xml \
	nios2.xml \
	or1k.xml \
	or1k-linux.xml \
	rs6000/powerpc-32.xml \
	rs6000/powerpc-32l.xml \
	rs6000/powerpc-403.xml \
	rs6000/powerpc-403gc.xml \
	rs6000/powerpc-405.xml \
	rs6000/powerpc-505.xml \
	rs6000/powerpc-601.xml \
	rs6000/powerpc-602.xml \
	rs6000/powerpc-603.xml \
	rs6000/powerpc-604.xml \
	rs6000/powerpc-64.xml \
	rs6000/powerpc-64l.xml \
	rs6000/powerpc-7400.xml \
	rs6000/powerpc-750.xml \
	rs6000/powerpc-860.xml \
	rs6000/powerpc-altivec32.xml \
	rs6000/powerpc-altivec32l.xml \
	rs6000/powerpc-altivec64.xml \
	rs6000/powerpc-altivec64l.xml \
	rs6000/powerpc-e500.xml \
	rs6000/powerpc-e500l.xml \
	rs6000/powerpc-isa205-32l.xml \
	rs6000/powerpc-isa205-64l.xml \
	rs6000/powerpc-isa205-altivec32l.xml \
	rs6000/powerpc-isa205-altivec64l.xml \
	rs6000/powerpc-isa205-vsx32l.xml \
	rs6000/powerpc-isa205-vsx64l.xml \
	rs6000/powerpc-isa205-ppr-dscr-vsx32l.xml \
	rs6000/powerpc-isa205-ppr-dscr-vsx64l.xml \
	rs6000/powerpc-isa207-vsx32l.xml \
	rs6000/powerpc-isa207-vsx64l.xml \
	rs6000/powerpc-isa207-htm-vsx32l.xml \
	rs6000/powerpc-isa207-htm-vsx64l.xml \
	rs6000/powerpc-vsx32.xml \
	rs6000/powerpc-vsx32l.xml \
	rs6000/powerpc-vsx64.xml \
	rs6000/powerpc-vsx64l.xml \
	rs6000/rs6000.xml \
	rx.xml \
	s390-linux32.xml \
	s390-linux32v1.xml \
	s390-linux32v2.xml \
	s390-linux64.xml \
	s390-linux64v1.xml \
	s390-linux64v2.xml \
	s390-te-linux64.xml \
	s390x-linux64.xml \
	s390x-linux64v1.xml \
	s390x-linux64v2.xml \
	s390x-te-linux64.xml \
	s390-tevx-linux64.xml \
	s390-vx-linux64.xml \
	s390x-tevx-linux64.xml \
	s390x-vx-linux64.xml \
	s390-gs-linux64.xml \
	s390x-gs-linux64.xml \
	sparc/sparc32-solaris.xml \
	sparc/sparc64-solaris.xml \
	z80.xml

TDESC_CFILES = $(patsubst %.xml,%.c,$(XMLTOC))
GDB = false

#Targets which use feature based target descriptions.
aarch64-feature = 1
arm-feature = 1
i386-feature = 1
loongarch-feature = 1
riscv-feature = 1
tic6x-feature = 1

all: $(OUTPUTS)

$(outdir)/%.dat: %.xml number-regs.xsl sort-regs.xsl gdbserver-regs.xsl
	echo "# THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi :set ro:" > $(outdir)/$*.tmp
	echo "# Generated from: $<" >> $(outdir)/$*.tmp
	echo "name:`echo $(notdir $*) | sed 's/-/_/g'`" >> $(outdir)/$*.tmp
	$(if $($(firstword $(subst /, ,$(subst -, ,$*)))-feature), echo "feature:1") >> $(outdir)/$*.tmp
	echo "xmltarget:$(<F)" >> $(outdir)/$*.tmp
	echo "expedite:$(if $($*-expedite),$($*-expedite),$($(firstword $(subst -, ,$(notdir $*)))-expedite))" \
	  >> $(outdir)/$*.tmp
	$(XSLTPROC) --path "$(PWD)" --xinclude number-regs.xsl $< | \
	  $(XSLTPROC) sort-regs.xsl - | \
	  $(XSLTPROC) gdbserver-regs.xsl - >> $(outdir)/$*.tmp
	sh ../../move-if-change $(outdir)/$*.tmp $(outdir)/$*.dat

# For targets with feature based target descriptions,
# the set of xml files we'll generate .c files for GDB from.
FEATURE_XMLFILES = aarch64-core.xml \
	aarch64-fpu.xml \
	aarch64-pauth.xml \
	aarch64-mte.xml \
	arc/v1-core.xml \
	arc/v1-aux.xml \
	arc/v2-core.xml \
	arc/v2-aux.xml \
	arm/arm-core.xml \
	arm/arm-fpa.xml \
	arm/arm-m-profile.xml \
	arm/arm-m-profile-mve.xml \
	arm/arm-m-system.xml \
	arm/arm-m-profile-with-fpa.xml \
	arm/arm-secext.xml \
	arm/arm-tls.xml \
	arm/arm-vfpv2.xml \
	arm/arm-vfpv3.xml \
	arm/xscale-iwmmxt.xml \
	cskyv2-linux.xml \
	i386/32bit-core.xml \
	i386/32bit-sse.xml \
	i386/32bit-linux.xml \
	i386/32bit-avx.xml \
	i386/32bit-mpx.xml \
	i386/32bit-avx512.xml \
	i386/32bit-segments.xml \
	i386/64bit-avx512.xml \
	i386/64bit-core.xml \
	i386/64bit-mpx.xml \
	i386/64bit-segments.xml \
	i386/64bit-avx.xml \
	i386/64bit-linux.xml \
	i386/64bit-sse.xml \
	i386/pkeys.xml \
	i386/x32-core.xml \
	loongarch/base32.xml \
	loongarch/base64.xml \
	loongarch/fpu.xml \
	riscv/rv32e-xregs.xml \
	riscv/32bit-cpu.xml \
	riscv/32bit-fpu.xml \
	riscv/64bit-cpu.xml \
	riscv/64bit-fpu.xml \
	tic6x-c6xp.xml \
	tic6x-core.xml \
	tic6x-gp.xml

FEATURE_CFILES = $(patsubst %.xml,%.c,$(FEATURE_XMLFILES))

cfiles: $(TDESC_CFILES) $(FEATURE_CFILES)

$(TDESC_CFILES): %.c: %.xml
	$(GDB) -nx -q -batch -ex 'maint print c-tdesc $<' > $@.tmp
	sh ../../move-if-change $@.tmp $@

$(FEATURE_CFILES): %.c: %.xml.tmp
	$(GDB) -nx -q -batch \
	  -ex 'maint print c-tdesc -single-feature $<' > $@.tmp
	sh ../../move-if-change $@.tmp $@
	rm $<

# %.xml is the XML file for each target description feature, and
# %.xml.tmp is the XML file target description which only includes
# one target description feature.

%.xml.tmp: %.xml
	echo "<?xml version=\"1.0\"?>" > $@.tmp
	echo "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">" >> $@.tmp
	echo "<target>" >> $@.tmp
	echo "  <xi:include href=\"$(notdir $<)\"/>" >> $@.tmp
	echo "</target>" >> $@.tmp
	sh ../../move-if-change $@.tmp $@

# 'all' doesn't build the C files, so don't delete them in 'clean'
# either.
clean-cfiles:
	rm -f $(TDESC_CFILES) $(FEATURE_CFILES)

clean:
	rm -f $(OUTPUTS)
