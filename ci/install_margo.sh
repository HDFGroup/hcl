#!/bin/bash

LOCAL=${HOME}/local
MARGO_VERSION=0.5
MARGO_DIR=margo-v${MARGO_VERSION}
MARGO_ARCHIVE=${MARGO_DIR}.tar.gz

wget https://xgitlab.cels.anl.gov/sds/margo/-/archive/v${MARGO_VERSION}/${MARGO_ARCHIVE}
tar xf ${MARGO_ARCHIVE}
pushd ${MARGO_DIR}
sh autogen.sh

CFLAGS="-I${LOCAL}/include"                    \
LDFLAGS="-L${LOCAL}/lib"                       \
    ./configure                                \
        --prefix=${LOCAL}                      \
        --enable-shared                        \
        PKG_CONFIG_PATH=${LOCAL}/lib/pkgconfig

make -j 2 && make install
popd
