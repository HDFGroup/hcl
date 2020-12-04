#!/bin/bash

set -x
set -e
set -o pipefail

INSTALL_DIR="${HOME}/${LOCAL}"
SPACK_DIR=${INSTALL_DIR}/spack
SDS_REPO_DIR=${INSTALL_DIR}/sds-repo
THALLIUM_VERSION=0.8.3
GOTCHA_VERSION=develop
ORTOOLS_VERSION=7.7
RPCLIB_VERSION=2.2.1

echo "Installing dependencies at ${INSTALL_DIR}"
mkdir -p ${INSTALL_DIR}
git clone --recursive https://github.com/scs-lab/spack ${SPACK_DIR}

set +x
. ${SPACK_DIR}/share/spack/setup-env.sh
set -x

spack compiler list

set +x
spack repo add ${SPACK_DIR}/var/spack/repos/sds-repo

THALLIUM_SPEC=mochi-thallium@${THALLIUM_VERSION}
spack install ${THALLIUM_SPEC}

GOTCHA_SPEC=gotcha@${GOTCHA_VERSION}
spack install ${GOTCHA_SPEC}
ORTOOLS_SPEC=gortools@${ORTOOLS_VERSION}
spack install ${ORTOOLS_SPEC}
RPCLIB_SPEC=rpclib@${RPCLIB_VERSION}
spack install ${RPCLIB_SPEC}


SPACK_STAGING_DIR=~/spack_staging
mkdir -p ${SPACK_STAGING_DIR}
spack view --verbose symlink ${SPACK_STAGING_DIR} ${THALLIUM_SPEC} ${GOTCHA_SPEC} ${ORTOOLS_SPEC} ${RPCLIB_SPEC}
set -x

cp -LRnv ${SPACK_STAGING_DIR}/* ${INSTALL_DIR}
