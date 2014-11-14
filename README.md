RISC-V GNU Compiler Toolchain
=============================

This is the RISC-V C and C++ cross-compiler. It supports two build modes:
a generic ELF/Newlib toolchain and a more sophisticated Linux-ELF/glibc
toolchain.

Author
------

Andrew Waterman

Contributors
------------

- Yunsup Lee
- Quan Nguyen
- Albert Ou
- Darius Rad
- Matt Thomas
- ultraembedded (github id)

### Prerequisites

Several standard packages are needed to build the toolchain.  On Ubuntu,
executing the following command should suffice:

    $ sudo apt-get install autoconf automake autotools-dev libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo patchutils

On Mac OS, the source and build directories must live in a case-sensitive file
system.  The simplest approach is to create and mount a new disk image with
that property.  Make sure that the mount point does not contain spaces.

### Installation (Newlib)

To build the Newlib cross-compiler, pick an install path.  If you choose,
say, `/opt/riscv`, then add `/opt/riscv/bin` to your `PATH` now.  Then, simply
run the following command:

    ./configure --prefix=/opt/riscv
    make

You should now be able to use riscv-gcc and its cousins.

### Installation (Linux)

To build the Linux cross-compiler, pick an install path.  If you choose,
say, `/opt/riscv`, then add `/opt/riscv/bin` to your `PATH` now.  Then, simply
run the following command:

    ./configure --prefix=/opt/riscv
    make linux

