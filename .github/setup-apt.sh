#!/bin/bash

# install OS prerequisites
dpkg --add-architecture i386
apt update
apt install -y autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev \
            libgmp-dev gawk build-essential bison flex texinfo gperf libtool \
            patchutils bc zlib1g-dev libexpat-dev git ninja-build cmake libglib2.0-dev expect \
            device-tree-compiler
