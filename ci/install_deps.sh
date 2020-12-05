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
BOOST_VERSION=1.74.0

echo "Installing dependencies at ${INSTALL_DIR}"
mkdir -p ${INSTALL_DIR}
git clone --recursive https://github.com/scs-lab/spack ${SPACK_DIR}

set +x
. ${SPACK_DIR}/share/spack/setup-env.sh
set -x

spack compiler list

set +x
spack repo add ${SPACK_DIR}/var/spack/repos/sds-repo

THALLIUM_SPEC="mochi-thallium~cereal@${THALLIUM_VERSION} ^mercury~boostsys"
#spack install ${THALLIUM_SPEC}
spack install pkg-config
#RPCLIB_SPEC=rpclib@${RPCLIB_VERSION}
#spack install ${RPCLIB_SPEC}

#BOOST_SPEC=boost@${BOOST_VERSION}
#spack install ${BOOST_SPEC}

spack env create hcl
spack env activate hcl
spack install pkg-config # ${THALLIUM_SPEC} # ${RPCLIB_SPEC} ${BOOST_SPEC}
ls ${SPACK_DIR}/var/spack/environments/hcl/.spack-env/view/lib