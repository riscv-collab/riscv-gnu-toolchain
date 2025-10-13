# RISCV Toolchain for ETSOC1

This repository contains the toolchain for the ETSOC1 chip.

To build:

    $ ./configure --prefix=/opt/et --with-arch=rv64imfc --with-abi=lp64f \
                  --with-languages=c,c++ --with-cmodel=medany
    $ make -j $(nproc)

As per main README, Ubuntu dependencies are as following:

    $ sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev

