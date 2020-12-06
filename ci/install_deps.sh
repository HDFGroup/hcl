#!/bin/bash

set -x
set -e
set -o pipefail

INSTALL_DIR="${HOME}/${LOCAL}"
SPACK_DIR=${INSTALL_DIR}/spack
SDS_REPO_DIR=${INSTALL_DIR}/sds-repo
THALLIUM_VERSION=0.8.3
MERCURY_VERSION=2.0.0
MPICH_VERSION=3.2.1
RPCLIB_VERSION=2.2.1
BOOST_VERSION=1.74.0
GCC_VERSION=9.3.0

echo "Installing dependencies at ${INSTALL_DIR}"
if [ ! -d ${INSTALL_DIR} ] 
then
  mkdir -p ${INSTALL_DIR}
  git clone --recursive https://github.com/scs-lab/spack ${SPACK_DIR}
else
  cd ${SPACK_DIR}
  git pull
  git submodule update --recursive --remote
  count=$(spack env list | grep hcl | wc -l)
  if [ $count -ge 1 ]
  then
  spack env deactivate hcl
  spack env remove -y hcl
  fi
fi

set +x
. ${SPACK_DIR}/share/spack/setup-env.sh
set -x

spack compiler list

set +x
spack repo add ${SPACK_DIR}/var/spack/repos/sds-repo

GCC_SPEC="gcc@${GCC_VERSION}"
spack install -y ${GCC_SPEC}

MPICH_SPEC="mpich@${MPICH_VERSION}%${GCC_SPEC}"
spack install -y  ${MPICH_SPEC}

MERCURY_SPEC="mercury@${MERCURY_VERSION}%${GCC_SPEC}"
spack install -y --no-checksum ${MERCURY_SPEC}

THALLIUM_SPEC="mochi-thallium~cereal@${THALLIUM_VERSION}%${GCC_SPEC}"
spack install ${THALLIUM_SPEC}

RPCLIB_SPEC="rpclib@${RPCLIB_VERSION}%${GCC_SPEC}"
spack install ${RPCLIB_SPEC}

BOOST_SPEC="boost@${BOOST_VERSION}%${GCC_SPEC}"
spack install ${BOOST_SPEC}

spack env create hcl
spack env activate hcl
spack install ${GCC_SPEC} ${THALLIUM_SPEC} ${RPCLIB_SPEC} ${BOOST_SPEC}
ls ${SPACK_DIR}/var/spack/environments/hcl/.spack-env/view
