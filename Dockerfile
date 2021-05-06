# The new base image to contain runtime dependencies

FROM debian:buster AS base

RUN set -ex;         \
    apt-get update;  

FROM base AS builder

RUN set -ex;                                                                      \
    apt-get install -y autoconf cmake curl libtool g++ cpputest pkg-config make;  \
    mkdir -p /usr/src;                                                            

WORKDIR /usr

COPY . /usr

RUN set -ex;    \
    cd /usr;     \
    g++ src/select.cpp -o select;            

EXPOSE 8080
CMD ["/usr/select", "inet"]
