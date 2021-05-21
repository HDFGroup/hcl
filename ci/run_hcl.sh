#!/bin/bash

pushd build
PATH=${INSTALL_DIR}/bin:${PATH}
if [ "${HCL_ENABLE_RPCLIB}" = "ON" ]; then
    ctest -VV
fi

if [ "${HCL_ENABLE_THALLIUM_TCP}" = "ON" ]; then
    ctest -VV
fi
popd
