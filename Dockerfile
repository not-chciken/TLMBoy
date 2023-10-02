FROM ubuntu:jammy
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq
RUN apt-get install software-properties-common -yq
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN  apt-get install -yq \
     build-essential \
     cmake \
     curl \
     git \
     git-lfs \
     gdb \
     gcovr \
     g++-10 \
     g++-13 \
     libgtest-dev \
     libsdl2-dev \
     netcat \
     libncurses-dev \
     flex \
     bison \
     texinfo

RUN add-apt-repository ppa:deadsnakes/ppa
RUN apt-get -yq install python3.8 python3.8-dev

RUN ln -sf /usr/bin/g++-10 /usr/bin/g++
RUN ln -sf /usr/bin/gcc-10 /usr/bin/gcc

# Install SystemC
WORKDIR /tmp
RUN git clone --branch 2.3.3 https://github.com/accellera-official/systemc.git
WORKDIR /tmp/systemc
RUN mkdir objdir
WORKDIR /tmp/systemc/objdir
RUN ../configure --prefix=/usr/local/systemc-2.3.3
RUN make -j$(nproc)
RUN make install
RUN rm -rf /tmp/systemc

# Install Z80 GDB
WORKDIR /tmp
RUN git clone --depth 1 https://github.com/not-chciken/binutils-gdb.git
WORKDIR binutils-gdb
RUN mkdir build
RUN ./configure --target=z80-unknown-elf --prefix=$(pwd)/build --exec-prefix=$(pwd)/build
RUN make -j$(nproc)
RUN make install
RUN cp -r build /opt/gdb
WORKDIR /tmp
RUN rm -rf binutils-gdb

ENV SYSTEMC_PATH=/usr/local/systemc-2.3.3
ENV LD_LIBRARY_PATH="${SYSTEMC_PATH}/lib-linux64:${LD_LIBRARY_PATH}"
ENV PATH="/opt/gdb/bin:${PATH}"

# More recent g++ for "std::format"
RUN ln -sf /usr/bin/g++-13 /usr/bin/g++
RUN ln -sf /usr/bin/gcc-13 /usr/bin/gcc
RUN ln -sf /usr/bin/gcov-13 /usr/bin/gcov

# TLMBoy; commented out for CI
# WORKDIR /tmp
# RUN git clone https://github.com/not-chciken/TLMBoy.git
# WORKDIR TLMBoy
# RUN git checkout dev
# RUN mkdir build
# WORKDIR build
# RUN cmake tlmboy ..
# RUN cmake --build . --target tlmboy --config Release -j$(nproc)

ENTRYPOINT bash
