#!/bin/bash

LOCAL=${HOME}/local
ARGOBOTS_VERSION=1.0rc1
ARGOBOTS_DIR=argobots-${ARGOBOTS_VERSION}
ARGOBOTS_ARCHIVE=${ARGOBOTS_DIR}.tar.gz

wget https://github.com/pmodels/argobots/releases/download/v${ARGOBOTS_VERSION}/${ARGOBOTS_ARCHIVE}
tar xf ${ARGOBOTS_ARCHIVE}
pushd ${ARGOBOTS_DIR}

CFLAGS="-I${LOCAL}/include"                    \
LDFLAGS="-L${LOCAL}/lib"                       \
    ./configure                                \
        --prefix=${LOCAL}                      \
        --enable-shared

make -j 2 && make install
popd
