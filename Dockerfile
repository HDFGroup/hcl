FROM ubuntu:18.04

# general environment for docker
ENV        DEBIAN_FRONTEND=noninteractive \
           SPACK_ROOT=/root/spack \
           FORCE_UNSAFE_CONFIGURE=1

# install minimal spack dependencies
RUN        apt-get update \
           && apt-get install -y --no-install-recommends \
              autoconf \
              ca-certificates \
              curl \
              environment-modules \
              git \
              build-essential \
              python \
              nano \
              sudo \
              unzip \
           && rm -rf /var/lib/apt/lists/*

RUN        apt-get update \
           && apt-get install -y gcc g++

# setup paths
ENV HOME=/root
ENV PROJECT_DIR=$HOME/source
ENV INSTALL_DIR=$HOME/install
ENV SPACK_DIR=$HOME/spack
ENV SDS_DIR=$HOME/sds

# install spack
RUN echo $INSTALL_DIR && mkdir -p $INSTALL_DIR
RUN git clone https://github.com/spack/spack ${SPACK_DIR}
RUN git clone https://xgitlab.cels.anl.gov/sds/sds-repo.git ${SDS_DIR}
RUN git clone https://github.com/HDFGroup/hcl ${PROJECT_DIR}

ENV spack=${SPACK_DIR}/bin/spack
RUN . ${SPACK_DIR}/share/spack/setup-env.sh
RUN $spack repo add ${SDS_DIR}
RUN $spack repo add ${PROJECT_DIR}/ci/hcl

# install software
ENV        THALLIUM_VERSION=0.8.3 \
           MERCURY_VERSION=2.0.0 \
           MPICH_VERSION=3.3.2 \
           RPCLIB_VERSION=2.2.1 \
           BOOST_VERSION=1.74.0 \
           GCC_VERSION=8.3.0

ENV GCC_SPEC="gcc@${GCC_VERSION}"
RUN $spack install -y ${GCC_SPEC}

ENV PATH=$SPACK_ROOT/bin:${PATH}

RUN echo "export PATH=${SPACK_ROOT}/bin:$PATH" >> /root/.bashrc
RUN echo ". $SPACK_ROOT/share/spack/setup-env.sh" >> /root/.bashrc

SHELL ["/bin/bash", "-c"]

RUN . $SPACK_ROOT/share/spack/setup-env.sh && spack load ${GCC_SPEC} && spack compiler find

RUN spack compiler list

ENV GCC_SPEC="${GCC_SPEC}%${GCC_SPEC}"
RUN $spack install -y ${GCC_SPEC}

ENV GCC_SPEC="gcc@${GCC_VERSION}"

ENV MPICH_SPEC="mpich@${MPICH_VERSION}%${GCC_SPEC}~fortran"
RUN $spack install -y  ${MPICH_SPEC}

ENV THALLIUM_SPEC="mochi-thallium~cereal@${THALLIUM_VERSION}%${GCC_SPEC}"
RUN $spack install ${THALLIUM_SPEC}

ENV RPCLIB_SPEC="rpclib@${RPCLIB_VERSION}%${GCC_SPEC}"
RUN $spack install ${RPCLIB_SPEC}

ENV BOOST_SPEC="boost@${BOOST_VERSION}%${GCC_SPEC}"
RUN $spack install ${BOOST_SPEC}
#
## Link Software
#
#RUN mkdir -p $INSTALL_DIR
#
RUN $spack view --verbose symlink -i ${INSTALL_DIR} ${GCC_SPEC}%${GCC_SPEC}
RUN $spack view --verbose symlink -i ${INSTALL_DIR} ${MPICH_SPEC}
RUN $spack view --verbose symlink -i ${INSTALL_DIR} ${THALLIUM_SPEC}
RUN $spack view --verbose symlink -i ${INSTALL_DIR} ${RPCLIB_SPEC}
RUN $spack view --verbose symlink -i ${INSTALL_DIR} ${BOOST_SPEC}

# startup
CMD        /bin/bash -l