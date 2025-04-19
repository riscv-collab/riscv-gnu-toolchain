RISC-V GNU Compiler Toolchain
=============================

This is the RISC-V C and C++ cross-compiler. It supports two build modes:
a generic ELF/Newlib toolchain and a more sophisticated Linux-ELF/glibc
toolchain.

###  Getting the sources

This repository uses submodules, but submodules will fetch automatically on demand,
so `--recursive` or `git submodule update --init --recursive` is not needed.

    $ git clone https://github.com/riscv/riscv-gnu-toolchain

**Warning: git clone takes around 6.65 GB of disk and download size**

### Prerequisites

Several standard packages are needed to build the toolchain.  

On Ubuntu, executing the following command should suffice:

    $ sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev

On Fedora/CentOS/RHEL OS, executing the following command should suffice:

    $ sudo yum install autoconf automake python3 libmpc-devel mpfr-devel gmp-devel gawk  bison flex texinfo patchutils gcc gcc-c++ zlib-devel expat-devel libslirp-devel
    
On Arch Linux, executing the following command should suffice:

    $ sudo pacman -Syu curl python3 libmpc mpfr gmp base-devel texinfo gperf patchutils bc zlib expat libslirp

Also available for Arch users on the AUR: [https://aur.archlinux.org/packages/riscv-gnu-toolchain-bin](https://aur.archlinux.org/packages/riscv-gnu-toolchain-bin)

On macOS, you can use [Homebrew](http://brew.sh) to install the dependencies:

    $ brew install python3 gawk gnu-sed make gmp mpfr libmpc isl zlib expat texinfo flock libslirp

When executing the instructions in this README, please use `gmake` instead of `make` to use the newly installed version of make.
To build the glibc (Linux) on macOS, you will need to build within a case-sensitive file
system.  The simplest approach is to create and mount a new disk image with
a case sensitive format.  Make sure that the mount point does not contain spaces. This is not necessary to build newlib or gcc itself on macOS.

This process will start by downloading about 200 MiB of upstream sources, then
will patch, build, and install the toolchain.  If a local cache of the
upstream sources exists in $(DISTDIR), it will be used; the default location
is /var/cache/distfiles.  Your computer will need about 8 GiB of disk space to
complete the process.

### Installation (Newlib)

To build the Newlib cross-compiler, pick an install path (that is writeable).
If you choose, say, `/opt/riscv`, then add `/opt/riscv/bin` to your `PATH`.
Then, simply run the following command:

    ./configure --prefix=/opt/riscv
    make

You should now be able to use riscv64-unknown-elf-gcc and its cousins.

Note: If you're planning to use an external library that replaces part of newlib (for example `libgloss-htif`), [read the FAQ](#ensuring-code-model-consistency).

### Installation (Linux)

To build the Linux cross-compiler, pick an install path (that is writeable).
If you choose, say, `/opt/riscv`, then add `/opt/riscv/bin` to your `PATH`.
Then, simply run the following command:

    ./configure --prefix=/opt/riscv
    make linux

The build defaults to targeting RV64GC (64-bit) with glibc, even on a 32-bit
build environment. To build the 32-bit RV32GC toolchain, use:

    ./configure --prefix=/opt/riscv --with-arch=rv32gc --with-abi=ilp32d
    make linux

In case you prefer musl libc over glibc, configure just like above and opt for
`make musl` instead of `make linux`.

Supported architectures are rv32i or rv64i plus standard extensions (a)tomics,
(m)ultiplication and division, (f)loat, (d)ouble, or (g)eneral for MAFD.

Supported ABIs are ilp32 (32-bit soft-float), ilp32d (32-bit hard-float),
ilp32f (32-bit with single-precision in registers and double in memory, niche
use only), lp64 lp64f lp64d (same but with 64-bit long and pointers).

### Installation (Newlib/Linux multilib)

To build either cross-compiler with support for both 32-bit and
64-bit, run the following command:

    ./configure --prefix=/opt/riscv --enable-multilib
    
And then either `make`, `make linux` or `make musl` for the Newlib, Linux
glibc-based or Linux musl libc-based cross-compiler, respectively.

The multilib compiler will have the prefix riscv64-unknown-elf- or
riscv64-unknown-linux-gnu- but will be able to target both 32-bit and 64-bit
systems.
It will support the most common `-march`/`-mabi` options, which can be seen by
using the `--print-multi-lib` flag on either cross-compiler.

Linux toolchain has an additional option `--enable-default-pie` to control the
default PIE enablement for GCC, which is disable by default.

To customize the enabled languages, use option `--with-languages=`. For example, 
if you want to enable `c,c++,fortran`, use `./configure --with-languages=c,c++,fortran`.
This option only takes effect for the GNU toolchain.

### Troubleshooting Build Problems

Builds work best if installing into an empty directory.  If you build a
hard-float toolchain and then try to build a soft-float toolchain with
the same --prefix directory, then the build scripts may get confused
and exit with a linker error complaining that hard float code can't be
linked with soft float code.  Removing the existing toolchain first, or
using a different prefix for the second build, avoids the problem.  It
is OK to build one newlib and one linux toolchain with the same prefix.
But you should avoid building two newlib or two linux toolchains with
the same prefix.

If building a linux toolchain on a MacOS system, or on a Windows system
using the Linux subsystem or cygwin, you must ensure that the filesystem
is case-sensitive.  A build on a case-insensitive filesystem will fail when
building glibc because \*.os and \*.oS files will clobber each other during
the build eventually resulting in confusing link errors.

CentOS (and RHEL) provide old GNU tools versions that may be too old to build
a RISC-V toolchain.  There is an alternate toolset provided that includes
current versions of the GNU tools.  This is the devtoolset provided as part
of the Software Collection service.  For more info, see the
[devtoolset-7](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/)
URL.  There are various versions of the devtoolset that are available, so you
can also try other versions of it, but we have at least one report that
devtoolset-7 works.

### Advanced Options

There are a number of additional options that may be passed to
configure.  See './configure --help' for more details.

Also you can define extra flags to pass to specific projects: ```BINUTILS_NATIVE_FLAGS_EXTRA, BINUTILS_TARGET_FLAGS_EXTRA, GCC_EXTRA_CONFIGURE_FLAGS, GDB_NATIVE_FLAGS_EXTRA, GDB_TARGET_FLAGS_EXTRA, GLIBC_TARGET_FLAGS_EXTRA, NEWLIB_TARGET_FLAGS_EXTRA```.
Example: ```GCC_EXTRA_CONFIGURE_FLAGS=--with-gmp=/opt/gmp make linux```

#### Set default ISA spec version

`--with-isa-spec=` can specify the default version of the RISC-V Unprivileged
(formerly User-Level) ISA specification.

Possible options are: `2.2`, `20190608` and `20191213`.

The default version is `20191213`.

More details about this option you can refer this post [RISC-V GNU toolchain bumping default ISA spec to 20191213](https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/aE1ZeHHCYf4).

#### Build with customized multi-lib configure.

`--with-multilib-generator=` can specify what multilibs to build.  The argument
is a semicolon separated list of values, possibly consisting of a single value.
Currently only supported for riscv*-*-elf*.  The accepted values and meanings
are given below.

Every config is constructed with four components: architecture string, ABI,
reuse rule with architecture string and reuse rule with sub-extension.

Re-use part support expansion operator (*) to simplify the combination of
different sub-extensions, example 4 demonstrate how it uses and works.

Example 1: Add multi-lib support for rv32i with ilp32.
```
./configure --with-multilib-generator="rv32i-ilp32--"
```

Example 2: Add multi-lib support for rv32i with ilp32 and rv32imafd with ilp32.

```
./configure --with-multilib-generator="rv32i-ilp32--;rv32imafd-ilp32--"
```

Example 3: Add multi-lib support for rv32i with ilp32; rv32im with ilp32 and
rv32ic with ilp32 will reuse this multi-lib set.
```
./configure --with-multilib-generator="rv32i-ilp32-rv32im-c"
```

Example 4: Add multi-lib support for rv64ima with lp64; rv64imaf with lp64,
rv64imac with lp64 and rv64imafc with lp64 will reuse this multi-lib set.
```
./configure --with-multilib-generator="rv64ima-lp64--f*c"
```

#### Enabling QEMU System Targets

The `--enable-qemu-system` configuration flag allows you to include QEMU system emulation targets in addition to the default user-mode emulation.

- **Enabled targets**:
  - `riscv64-linux-user`
  - `riscv32-linux-user`
  - `riscv64-softmmu`
  - `riscv32-softmmu`

- **Default targets** (without this flag):
  - `riscv64-linux-user`
  - `riscv32-linux-user`

Use this option if you need full system emulation for RISC-V. Example configuration:

```bash
./configure --enable-qemu-system --prefix=/opt/riscv
make build-sim SIM=qemu
```

This flag is particularly useful for developers testing and emulating full RISC-V systems rather than just user-space applications.

### Test Suite

The Dejagnu test suite has been ported to RISC-V. This can be run with a
simulator for the elf and linux toolchains. The simulator can be selected
by the SIM variable in the Makefile, e.g. SIM=qemu, SIM=gdb, or SIM=spike
(experimental).In addition, the simulator can also be selected with the 
configure time option `--with-sim=`.However, the testsuite allowlist is 
only maintained for qemu.Other simulators might get extra failures.

#### Additional Prerequisite

A helper script to setup testing environment requires
[pyelftools](https://github.com/eliben/pyelftools).

On newer versions of Ubuntu, executing the following command
should suffice:

    $ sudo apt-get install python3-pyelftools

On newer versions of Fedora and CentOS/RHEL OS (9 or later), executing
the following command should suffice:

    $ sudo yum install python3-pyelftools

On Arch Linux, executing the following command should suffice:

    $ sudo pacman -Syyu python-pyelftools python-sphinx python-sphinx_rtd_theme ninja

If your distribution/OS does not have pyelftools package, you can install
it using PIP.

    # Assuming that PIP is installed
    $ pip3 install --user pyelftools

#### Testing GCC

To test GCC, run the following commands:

    ./configure --prefix=$RISCV --disable-linux --with-arch=rv64ima # or --with-arch=rv32ima
    make newlib
    make report-newlib SIM=gdb # Run with gdb simulator

    ./configure --prefix=$RISCV
    make linux
    make report-linux SIM=qemu # Run with qemu

    ./configure --prefix=$RISCV --with-sim=spike
    make linux
    make report               # Run with spike

Note:
- spike only support rv64* bare-metal/elf toolchain.
- gdb simulator only support bare-metal/elf toolchain.

#### Selecting the tests to run in GCC's regression test suite

By default GCC will execute all tests of its regression test suite.
While running them in parallel (e.g. `make -j$(nproc) report`) will
significantly speed up the execution time on multi-processor systems,
the required time for executing all tests is usually too high for
typical development cycles. Therefore GCC allows to select the tests
that are being executed using the environment variable `RUNTESTFLAGS`.

To restrict a test run to only RISC-V specific tests
the following command can be used:

 RUNTESTFLAGS="riscv.exp" make report

To restrict a test run to only RISC-V specific tests with match the
pattern "zb*.c" and "sm*.c" the following command can be used:

 RUNTESTFLAGS="riscv.exp=zb*.c\ sm*.c" make report

#### Testing GCC, Binutils, and glibc of a Linux toolchain

The default Makefile target to run toolchain tests is `report`.
This will run all tests of the GCC regression test suite.
Alternatively, the following command can be used to do the same:

    make check-gcc

The following command can be used to run the Binutils tests:

    make check-binutils

The command below can be used to run the glibc tests:

    make check-glibc-linux

##### Adding more arch/abi combination for testing without introducing multilib

`--with-extra-multilib-test` can be used when you want to test more combination
of arch/ABI, for example: built a linux toolchain with multilib with
`rv64gc/lp64d` and `rv64imac/lp64`, but you want to test more configuration like
`rv64gcv/lp64d` or `rv64gcv_zba/lp64d`, then you can use --with-extra-multilib-test
to specify that via `--with-extra-multilib-test="rv64gcv-lp64d;rv64gcv_zba-lp64d"`,
then the testing will run for `rv64gc/lp64d`, `rv64imac/lp64`, `rv64gcv/lp64d`
and `rv64gcv_zba/lp64d`.

`--with-extra-multilib-test` support bare-metal and linux toolchain and support
even multilib is disable, but the user must ensure extra multilib test
configuration can be work with existing lib/multilib, e.g. rv32gcv/ilp32 test
can't work if multilib didn't have any rv32 multilib.

`--with-extra-multilib-test` also support more complicated format to fit the
requirements of end-users. First of all, the argument is a list of test
configurations. Each test configuration are separated by `;`. For example:

  `rv64gcv-lp64d;rv64_zvl256b_zvfh-lp64d`

For each test configuration, it has two parts, aka required arch-abi part and
optional build flags. We leverage `:` to separate them with some restrictions.

  * arch-abi should be required and there must be only one at the begining of
    the test configuration.
  * build flags is a array-like flags after the arch-abi, there will be two
    ways to arrange them, aka AND, OR operation.
  * If you would like the flags in build flags array acts on arch-abi
    __simultaneously__, you can use `:` to separate them. For example:

   ```
   rv64gcv-lp64d:--param=riscv-autovec-lmul=dynamic:--param=riscv-autovec-preference=fixed-vlmax
   ```

   will be consider as one target board same as below:

   ```
   riscv-sim/-march=rv64gcv/-mabi=lp64d/-mcmodel=medlow/--param=riscv-autovec-lmul=dynamic/--param=riscv-autovec-preference=fixed-vlmax
   ```

  * If you would like the flags in build flags array acts on arch-abi
    __respectively__, you can use ',' to separate them. For example:

   ```
   rv64gcv-lp64d:--param=riscv-autovec-lmul=dynamic,--param=riscv-autovec-preference=fixed-vlmax
   ```

   will be consider as two target boards same as below:

   ```
   riscv-sim/-march=rv64gcv/-mabi=lp64d/-mcmodel=medlow/--param=riscv-autovec-preference=fixed-vlmax
   riscv-sim/-march=rv64gcv/-mabi=lp64d/-mcmodel=medlow/--param=riscv-autovec-lmul=dynamic
   ```

  * However, you can also leverage AND(`:`), OR(`,`) operator together but the
    OR(`,`) will always have the higher priority. For example:

   ```
   rv64gcv-lp64d:--param=riscv-autovec-lmul=dynamic:--param=riscv-autovec-preference=fixed-vlmax,--param=riscv-autovec-lmul=m2
   ```

   will be consider as tow target boars same as below:

   ```
   riscv-sim/-march=rv64gcv/-mabi=lp64d/-mcmodel=medlow/--param=riscv-autovec-lmul=dynamic/--param=riscv-autovec-preference=fixed-vlmax
   riscv-sim/-march=rv64gcv/-mabi=lp64d/-mcmodel=medlow/--param=riscv-autovec-lmul=m2
   ```

### LLVM / clang

LLVM can be used in combination with the RISC-V GNU Compiler Toolchain
to build RISC-V applications. To build LLVM with C and C++ support the
configure flag `--enable-llvm` can be used.

E.g. to build LLVM on top of a RV64 Linux toolchain the following commands
can be used:

  ./configure --prefix=$RISCV --enable-llvm --enable-linux
  make

Note, that a combination of `--enable-llvm` and multilib configuration flags
is not supported.

Below are examples how to build a rv64gc Linux/newlib toolchain with LLVM support,
how to use it to build a C and a C++ application using clang, and how to
execute the generated binaries using QEMU.

Build Linux toolchain and run examples:

    # Build rv64gc toolchain with LLVM
    ./configure --prefix=$RISCV --enable-llvm --enable-linux --with-arch=rv64gc --with-abi=lp64d
    make -j$(nproc) all build-sim SIM=qemu
    # Build C application with clang
    $RISCV/bin/clang -march=rv64imafdc -o hello_world hello_world.c
    $RISCV/bin/qemu-riscv64 -L $RISCV/sysroot ./hello_world
    # Build C++ application with clang
    $RISCV/bin/clang++ -march=rv64imafdc -stdlib=libc++ -o hello_world_cpp hello_world_cpp.cxx
    $RISCV/bin/qemu-riscv64 -L $RISCV/sysroot ./hello_world_cpp

Build newlib toolchain and run examples (don't work with `--with-multilib-generator=`):

    # Build rv64gc bare-metal toolchain with LLVM
    ./configure --prefix=$RISCV --enable-llvm --disable-linux --with-arch=rv64gc --with-abi=lp64d
    make -j$(nproc) all build-sim SIM=qemu
    # Build C application with clang
    $RISCV/bin/clang -march=rv64imafdc -o hello_world hello_world.c
    $RISCV/bin/qemu-riscv64 -L $RISCV/sysroot ./hello_world
    # Build C++ application with clang using static link
    $RISCV/bin/clang++ -march=rv64imafdc -static -o hello_world_cpp hello_world_cpp.cxx
    $RISCV/bin/qemu-riscv64 -L $RISCV/sysroot ./hello_world_cpp

### Development

This section is only for developer or advanced user, or you want to build
toolchain with your own source tree.

#### Update Source Tree

`riscv-gnu-toolchain` contain stable but not latest source for each submodule,
in case you want to using latest development tree, you can use following command
to upgrade all submodule.

    git submodule update --remote

Or you can upgrade specific submodule only.

    git submodule update --remote <component>

For example, upgrade gcc only, you can using following command:

    git submodule update --remote gcc

#### How to Check Which Branch are Used for Specific submodule

The branch info has recorded in `.gitmodules` file, which can set or update via
`git submodule add -b` or `git submodule set-branch`.

However the only way to check which branch are using is to check `.gitmodules`
file, here is the example for `gcc`, it's using releases/gcc-12 branch, so
it will has a section named `gcc` and has a field `branch` is
`releases/gcc-12`.

```
[submodule "gcc"]
        path = gcc
        url = ../gcc.git
        branch = releases/gcc-12
```

#### Use Source Tree Other Than `riscv-gnu-toolchain`

`riscv-gnu-toolchain` also supports using out-of-tree source to build the toolchain.
There are several configure options for specifying the source tree of each
submodule/component.

For example, if you have GCC sources in `$HOME/gcc`, use `--with-gcc-src` to build the toolchain using those sources:

    ./configure ... --with-gcc-src=$HOME/gcc

Here is the list of configure options for specifying alternative sources for the various submodules/components:

    --with-binutils-src
    --with-dejagnu-src
    --with-gcc-src
    --with-gdb-src
    --with-glibc-src
    --with-linux-headers-src
    --with-llvm-src
    --with-musl-src
    --with-newlib-src
    --with-pk-src
    --with-qemu-src
    --with-spike-src
    --with-uclibc-src

#### Build host GCC to check for compiler warnings

GCC contributions have to meet several requirements to qualify for upstream
inclusion.  Warning free compilation with a compiler build from the same
sources is among them.  The flag `--enable-host-gcc` does exaclty that:

* Initially a host GCC will be built
* This host GCC is then used to build the cross compiler
* The cross compiler will be built with `-Werror` to identify code issues

### FAQ
#### Ensuring Code Model Consistency
If parts of newlib are going to be replaced with an external library (such as with [libgloss-htif](https://github.com/ucb-bar/libgloss-htif) for Berkeley Host-Target Interface), 
you should take care to ensure that both newlib and the external library are built using the same code model. For more information about RISC-V code models, 
[read this SiFive blog article](https://www.sifive.com/blog/all-aboard-part-4-risc-v-code-models).

Errors that indicate a code model mismatch include "relocation overflow" or "relocation truncated" errors from the linker being unable to successfully relocate symbols in the executable.

By default, `riscv-gnu-toolchain` builds newlib with `-mcmodel=medlow`. You can use the alternative `medany` code model (as used in libgloss-htif) by passing `--with-cmodel=medany` to the configure script.
