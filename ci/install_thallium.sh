#!/bin/bash

LOCAL=${HOME}/local
THALLIUM_VERSION=0.8.3
THALLIUM_DIR=thallium-v${THALLIUM_VERSION}
THALLIUM_ARCHIVE=${THALLIUM_DIR}.tar.gz

wget https://xgitlab.cels.anl.gov/sds/thallium/-/archive/v${THALLIUM_VERSION}/${THALLIUM_ARCHIVE}
tar xf ${THALLIUM_ARCHIVE}
pushd ${THALLIUM_DIR}
mkdir build
pushd build
CXXFLAGS="-I${LOCAL}/include"                \
LDFLAGS="-L${LOCAL}/lib"                     \
    cmake                                    \
        -DCMAKE_INSTALL_PREFIX=${LOCAL}      \
        -DCMAKE_PREFIX_PATH=${LOCAL}         \
        -DCMAKE_BUILD_RPATH="${LOCAL}/lib"   \
        -DCMAKE_INSTALL_RPATH="${LOCAL}/lib" \
        -DCMAKE_BUILD_TYPE=Release           \
        -DCMAKE_C_COMPILER=`which gcc`       \
        -DCMAKE_CXX_COMPILER=`which g++`     \
        -DBUILD_SHARED_LIBS=ON               \
        ..

cmake --build . -- -j 2 && make install

popd
popd
