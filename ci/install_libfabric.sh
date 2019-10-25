#!/bin/bash

LIBFABRIC_VERSION=1.8.1
LIBFABRIC_DIR=libfabric-${LIBFABRIC_VERSION}
LIBFABRIC_ARCHIVE=${LIBFABRIC_DIR}.tar.bz2

wget https://github.com/ofiwg/libfabric/releases/download/v${LIBFABRIC_VERSION}/${LIBFABRIC_ARCHIVE}
tar xf ${LIBFABRIC_ARCHIVE}
pushd ${LIBFABRIC_DIR}
./configure --prefix=$HOME/local && make -j 2 && make install
popd
