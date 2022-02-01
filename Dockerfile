FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq \
 && apt-get install -yq \
     build-essential \
     cmake \
     git \
     gmake \
     libgtest-dev \
     python3-dev \
     python3-six \
     python-is-python3 \
     git
RUN apt-get install -yq \
     vim \
     wget \
     gdb \
     gcovr \
     python3-pip

# Install SystemC
WORKDIR /tmp
RUN git clone --branch 2.3.3 https://github.com/accellera-official/systemc.git
WORKDIR /tmp/systemc
RUN mkdir objdir
WORKDIR /tmp/systemc/objdir
RUN ../configure --prefix=/usr/local/systemc-2.3.3
RUN gmake
RUN gmake install
WORKDIR /tmp/
RUN rm -rf systemc

ENTRYPOINT bash
