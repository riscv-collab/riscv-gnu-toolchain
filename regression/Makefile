.SECONDARY:
report:

# The list of target tuples that we want to test.
TARGETS  =
TARGETS += newlib-rv32i-ilp32-medlow
TARGETS += newlib-rv32im-ilp32-medlow
TARGETS += newlib-rv32iac-ilp32-medlow
TARGETS += newlib-rv32imac-ilp32-medlow
TARGETS += newlib-rv32imafc-ilp32f-medlow
TARGETS += newlib-rv64imac-lp64-medlow
TARGETS += newlib-rv64imafdc-lp64d-medlow
TARGETS += linux-rv32imac-ilp32-medlow
TARGETS += linux-rv32imafdc-ilp32-medlow
TARGETS += linux-rv32imafdc-ilp32d-medlow
TARGETS += linux-rv64imac-lp64-medlow
TARGETS += linux-rv64imafdc-lp64-medlow
TARGETS += linux-rv64imafdc-lp64d-medlow
TARGETS += newlib-rv32i-ilp32-medany
TARGETS += newlib-rv32im-ilp32-medany
TARGETS += newlib-rv32iac-ilp32-medany
TARGETS += newlib-rv32imac-ilp32-medany
TARGETS += newlib-rv32imafc-ilp32f-medany
TARGETS += newlib-rv64imac-lp64-medany
TARGETS += newlib-rv64imafdc-lp64d-medany
TARGETS += linux-rv32imac-ilp32-medany
TARGETS += linux-rv32imafdc-ilp32-medany
TARGETS += linux-rv32imafdc-ilp32d-medany
TARGETS += linux-rv64imac-lp64-medany
TARGETS += linux-rv64imafdc-lp64-medany
TARGETS += linux-rv64imafdc-lp64d-medany

# This is the link between the report targets and the actual testsuite
# build/test runs.  It's setup with a level of indirection here to make sure
# that when running "make report" we run all the test suites before running any
# of the reports.  Since the various check targets are never supposed to fail,
# this lets me ensure that re-running "make report" is always fast.
.PHONY: check
check: $(addprefix stamps/check-,$(TARGETS))

.PHONY: check-%
check-%: stamps/check-%

.PHONY: report
report: check
	$(MAKE) $(addprefix report-,$(TARGETS))

.PHONY: report-%
report-%: stamps/check-%
	$(eval $@_BUILDDIR := build/$(patsubst report-%,%,$(notdir $@)))
	$(MAKE) -C $($@_BUILDDIR) report

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
