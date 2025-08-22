.SECONDARY:
report:

# The list of target tuples that we want to test.
TARGET_BASES = \
	newlib-rv32i-ilp32      \
	newlib-rv32im-ilp32     \
	newlib-rv32iac-ilp32    \
	newlib-rv32imac-ilp32   \
	newlib-rv32imafc-ilp32f \
	newlib-rv64imac-lp64    \
	newlib-rv64imafdc-lp64d \
	linux-rv32imac-ilp32    \
	linux-rv32imafdc-ilp32  \
	linux-rv32imafdc-ilp32d \
	linux-rv64imac-lp64     \
	linux-rv64imafdc-lp64   \
	linux-rv64imafdc-lp64d
TARGET_CODE_MODELS = medlow medany
TARGETS ?= $(foreach CODE_MODEL,$(TARGET_CODE_MODELS), $(addsuffix -$(CODE_MODEL),$(TARGET_BASES)))

# This is the link between the report targets and the actual testsuite
# build/test runs.  It's setup with a level of indirection here to make sure
# that when running "make report" we run all the test suites before running any
# of the reports.  Since the various check targets are never supposed to fail,
# this lets me ensure that re-running "make report" is always fast.
.PHONY: build
build: $(addprefix stamps/build-,$(TARGETS))

.PHONY: check
check: $(addprefix stamps/check-,$(TARGETS))

.PHONY: check-%
check-%: stamps/check-%

.PHONY: build-%
build-%: stamps/build-%

.PHONY: report
report: check
	$(MAKE) $(addprefix report-gcc-,$(TARGETS))

.PHONY: report-gcc-%
report-gcc-%: stamps/check-%
	$(eval $@_BUILDDIR := build/$(patsubst report-gcc-%,%,$(notdir $@)))
	$(MAKE) -C $($@_BUILDDIR) report

.PHONY: report-binutils
report-binutils: check
	$(MAKE) $(addprefix report-binutils-,$(TARGETS))

.PHONY: report-binutils-%
report-binutils-%: stamps/check-%
	$(eval $@_BUILDDIR := build/$(patsubst report-binutils-%,%,$(notdir $@)))
	$(MAKE) -C $($@_BUILDDIR) report-binutils

# These rules call into the above Makefile to actually test the various
# toolchain targets we care about.
stamps/configure-%:
	$(eval $@_BUILDDIR := build/$(patsubst configure-%,%,$(notdir $@)))
	$(eval $@_PREFIX := install/$(patsubst configure-%,%,$(notdir $@)))
	$(eval $@_LINUX := $(if $(filter linux,$(word 2,$(subst -, ,$(notdir $@)))),--enable-linux,--disable-linux))
	$(eval $@_MARCH := $(word 3,$(subst -, ,$(notdir $@))))
	$(eval $@_MABI := $(word 4,$(subst -, ,$(notdir $@))))
	$(eval $@_MCMODEL := $(word 5,$(subst -, ,$(notdir $@))))
	mkdir -p $($@_BUILDDIR)
	cd $($@_BUILDDIR); $(abspath ../configure) \
		$($@_LINUX) \
		--disable-multilib \
		--prefix=$(abspath $($@_PREFIX)) \
		--with-arch=$($@_MARCH) \
		--with-abi=$($@_MABI) \
		--with-cmodel=$($@_MCMODEL)
	mkdir -p $(dir $@)
	date > $@

stamps/build-%: stamps/configure-%
	$(eval $@_BUILDDIR := build/$(patsubst build-%,%,$(notdir $@)))
	$(eval $@_PREFIX := install/$(patsubst build-%,%,$(notdir $@)))
	$(MAKE) -C $($@_BUILDDIR)
	mkdir -p $(dir $@)
	echo 'export PATH="$(abspath $($@_PREFIX))/bin:$$PATH"' > $($@_PREFIX)/enter.bash
	date > $@

stamps/check-%: stamps/build-%
	$(eval $@_BUILDDIR := build/$(patsubst check-%,%,$(notdir $@)))
	$(MAKE) -C $($@_BUILDDIR) check
	mkdir -p $(dir $@)
	date > $@

# Cleans up after a run
.PHONY: clean
clean::
	rm -rf build install stamps
