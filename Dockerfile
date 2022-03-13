FROM ubuntu:focal
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
     libfmt-dev \
     libgtest-dev \
     libsdl2-dev \
     netcat \
     python3-dev \
     python3-six \
     python-is-python3 \
     python3-pip \
     libncurses-dev \
     flex \
     bison \
     texinfo

RUN ln -sf /usr/bin/g++-10 /usr/bin/g++

# Install SystemC
WORKDIR /tmp
RUN git clone --branch 2.3.3 https://github.com/accellera-official/systemc.git
WORKDIR /tmp/systemc
RUN mkdir objdir
WORKDIR /tmp/systemc/objdir
RUN ../configure --prefix=/usr/local/systemc-2.3.3
RUN make
RUN make install
RUN rm -rf /tmp/systemc

# Install Z80 GDB
WORKDIR /tmp
RUN git clone --depth 1 https://github.com/b-s-a/binutils-gdb.git
WORKDIR binutils-gdb
RUN mkdir build
RUN ./configure --target=z80-unknown-elf --prefix=$(pwd)/build --exec-prefix=$(pwd)/build
RUN make
RUN make install
RUN cp -r build /opt/gdb
WORKDIR /tmp
RUN rm -rf binutils-gdb

ENV SYSTEMC_PATH=/usr/local/systemc-2.3.3
ENV LD_LIBRARY_PATH="${SYSTEMC_PATH}/lib-linux64:${LD_LIBRARY_PATH}"
ENV PATH="/opt/gdb/bin:${PATH}"

ENTRYPOINT bash
