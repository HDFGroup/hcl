#!/bin/bash

LOCAL=${HOME}/local
mkdir build
pushd build

CXXFLAGS="-I${LOCAL}/include"                                  \
LDFLAGS="-L${LOCAL}/lib"                                       \
    cmake                                                      \
        -DCMAKE_INSTALL_PREFIX=${LOCAL}                        \
        -DCMAKE_BUILD_RPATH="${LOCAL}/lib"                     \
        -DCMAKE_INSTALL_RPATH="${LOCAL}/lib"                   \
        -DCMAKE_BUILD_TYPE=Release                             \
        -DCMAKE_CXX_COMPILER=`which mpicxx`                    \
        -DCMAKE_C_COMPILER=`which mpicc`                       \
        -DHCL_ENABLE_RPCLIB=${HCL_ENABLE_RPCLIB}               \
        -DHCL_ENABLE_THALLIUM_TCP=${HCL_ENABLE_THALLIUM_TCP}   \
        -DHCL_ENABLE_THALLIUM_ROCE=${HCL_ENABLE_THALLIUM_ROCE} \
        -DBUILD_TEST=ON                                        \
        ..

cmake --build . -- -j 2 VERBOSE=1 && make install
