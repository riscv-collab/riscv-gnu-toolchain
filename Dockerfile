# Docker to build
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev

RUN mkdir -p /home/app

WORKDIR /home/app

RUN git clone https://github.com/riscv-collab/riscv-gnu-toolchain.git

WORKDIR /home/app/riscv-gnu-toolchain
RUN git checkout 2025.01.20
RUN sed -i '/shallow = true/d' .gitmodules
RUN sed -i 's/--depth 1//g' Makefile.in
RUN ./configure --prefix=/opt/riscv --with-arch=rv64gc --with-abi=lp64d
RUN make linux -j 4

# Clean
#RUN rm -rf /home/app


# Final docker
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev

COPY --from=builder /opt/ /opt/

ENV PATH="$PATH:/opt/riscv/bin"


