#!/bin/bash

LOCAL=${HOME}/local
MERCURY_VERSION=1.0.1
MERCURY_DIR=mercury-${MERCURY_VERSION}
MERCURY_ARCHIVE=${MERCURY_DIR}.tar.bz2

wget https://github.com/mercury-hpc/mercury/releases/download/v${MERCURY_VERSION}/${MERCURY_ARCHIVE}
tar xf ${MERCURY_ARCHIVE}
pushd ${MERCURY_DIR}
mkdir build
pushd build

CXXFLAGS="-I${LOCAL}/include"                \
LDFLAGS="-L${LOCAL}/lib"                     \
    cmake                                    \
        -DCMAKE_INSTALL_PREFIX=${LOCAL}      \
        -DCMAKE_PREFIX_PATH=${LOCAL}         \
        -DCMAKE_BUILD_RPATH="${LOCAL}/lib"   \
        -DCMAKE_INSTALL_RPATH="${LOCAL}/lib" \
        -DCMAKE_C_COMPILER=`which gcc`       \
        -DCMAKE_BUILD_TYPE=Release           \
        -DBUILD_SHARED_LIBS=ON               \
        -DMERCURY_USE_BOOST_PP=ON            \
        -DMERCURY_USE_SYSTEM_BOOST=ON        \
        -DMERCURY_USE_CHECKSUMS=ON           \
        -DMERCURY_USE_EAGER_BULK=ON          \
        -DMERCURY_USE_SELF_FORWARD=ON        \
        -DNA_USE_OFI=ON                      \
        ..

cmake --build . -- -j 2 && make install

popd
popd
