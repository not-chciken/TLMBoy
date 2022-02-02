FROM ubuntu:focal
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq
RUN apt-get install software-properties-common -yq
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN  apt-get install -yq \
     build-essential \
     cmake \
     git \
     gdb \
     gcovr \
     g++-10 \
     libfmt-dev \
     libgtest-dev \
     libsdl2-dev \
     python3-dev \
     python3-six \
     python-is-python3 \
     python3-pip

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
WORKDIR /tmp/

ENV SYSTEMC_PATH=/usr/local/systemc-2.3.3

ENTRYPOINT bash
