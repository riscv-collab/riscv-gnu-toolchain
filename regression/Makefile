.SECONDARY:

# The list of target tuples that we want to test.
.PHONY: report
report: report-newlib-rv32i-ilp32-medlow
report: report-newlib-rv32im-ilp32-medlow
report: report-newlib-rv32iac-ilp32-medlow
report: report-newlib-rv32imac-ilp32-medlow
report: report-newlib-rv32imafc-ilp32f-medlow
report: report-newlib-rv64imac-lp64-medlow
report: report-newlib-rv64imafdc-lp64d-medlow
report: report-linux-rv32imac-ilp32-medlow
report: report-linux-rv32imafdc-ilp32-medlow
report: report-linux-rv32imafdc-ilp32d-medlow
report: report-linux-rv64imac-lp64-medlow
report: report-linux-rv64imafdc-lp64-medlow
report: report-linux-rv64imafdc-lp64d-medlow

# This is the link between the report targets and the actual testsuite
# build/test runs.
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
	echo 'export $$PATH="$(abspath $($@_PREFIX)):$$PATH"' > $($@_PREFIX)/enter.bash
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
