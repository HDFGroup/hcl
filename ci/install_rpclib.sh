#!/bin/bash

LOCAL=${HOME}/local
git clone https://github.com/qchateau/rpclib.git
pushd rpclib
mkdir build
pushd build

cmake                                  \
    -DCMAKE_INSTALL_PREFIX=${LOCAL}    \
    -DCMAKE_BUILD_TYPE=Release         \
    -DBUILD_SHARED_LIBS=ON             \
    -DCMAKE_CXX_COMPILER=`which g++`   \
    -DCMAKE_C_COMPILER=`which gcc`     \
    ..

cmake --build . -- -j 2 && make install

popd
popd
