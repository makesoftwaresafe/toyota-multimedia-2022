FROM ubuntu:20.04 as builder

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y git autoconf libtool pkg-config libglib2.0-dev libdbus-1-dev libxtables-dev libgnutls28-dev libreadline-dev make

RUN git clone git://git.kernel.org/pub/scm/network/connman/connman.git connman-src && \
    cd connman-src/ && \
    git checkout 1.33

WORKDIR /connman-src

RUN ./bootstrap && \
    ./configure --prefix=/connman-uninst --disable-dependency-tracking && \
    make -j$(grep -c '^processor' /proc/cpuinfo) && \
    make install && \
    mkdir -p /connman-uninst/var/lib/connman

