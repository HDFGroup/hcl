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
ENV HCL_VERSION=dev

ENV HCL_SPEC="hcl@${HCL_VERSION} communication=rpclib"
RUN $spack install --only dependencies ${HCL_SPEC}

## Link Software
RUN $spack view --dependencies yes symlink -i ${INSTALL_DIR} ${HCL_SPEC}

ENV HCL_SPEC="hcl@${HCL_VERSION} communication=thallium"
RUN $spack install --only dependencies ${HCL_SPEC}

## Link Software
RUN $spack view --dependencies yes symlink -i ${INSTALL_DIR} ${HCL_SPEC}

RUN echo "export PATH=${SPACK_ROOT}/bin:$PATH" >> /root/.bashrc
RUN echo ". $SPACK_ROOT/share/spack/setup-env.sh" >> /root/.bashrc

SHELL ["/bin/bash", "-c"]