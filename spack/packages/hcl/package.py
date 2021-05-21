# Copyright 2013-2020 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

# ----------------------------------------------------------------------------
# If you submit this package back to Spack as a pull request,
# please first remove this boilerplate and all FIXME comments.
#
# This is a template package file for Spack.  We've put "FIXME"
# next to all the things you'll want to change. Once you've handled
# them, you can save this file and test your package like this:
#
#     spack install hcl
#
# You can edit this file again by typing:
#
#     spack edit hcl
#
# See the Spack documentation for more information on packaging.
# ----------------------------------------------------------------------------

from spack import *


class Hcl(CMakePackage):
    """ HCL provides wrappers for C++ Standard Library (STL) containers, which it
        distributes and manages transparently across nodes. HCL has been designed
        to be easy-to-use, highly programmable, and portable. Data access is optimized
        via a hybrid data model of shared memory and RPC. It supports decoupled and
        ephemeral deployment models, with deployment configured based on application
        requirements. It's primary goals are to provide

        -   a familiar STL-like interface
        -   a flexible programming paradigm
        -   a hybrid data access model optimized for High-Performance Computing (HPC)
        -   a high-performance data container infrastructure that leverages new
            hardware and software innovations (e.g., RDMA, RoCE, one-sided communications)
    """

    homepage = "https://github.com/HDFGroup/hcl"
    git = "https://github.com/HDFGroup/hcl.git"

    version('dev', branch='dev')
    version('0.1', branch='v0.1')

    variant(
        'rpc', default='thallium', description='RPC library to utilize',
        values=('rpclib', 'thallium'), multi=False)
    variant(
        'protocol', default='tcp', description='Network protocol to utilize',
        values=('tcp', 'ib'), multi=False)

    depends_on('gcc@8.3.0')
    depends_on('mpich@3.3.2~fortran')
    depends_on('rpclib@2.2.1', when="rpc=rpclib")
    depends_on('mochi-thallium@0.8:', when="rpc=thallium")
    depends_on('boost@1.74.0')

    def cmake_args(self):
        config_args = ['-DCMAKE_INSTALL_PREFIX={}'.format(self.prefix)]
        if self.spec.variants['rpc'].value == 'rpclib':
            config_args.append('-DBASKET_ENABLE_RPCLIB=ON')
            config_args.append('-DHCL_ENABLE_THALLIUM_ROCE=OFF')
            config_args.append('-DHCL_ENABLE_THALLIUM_TCP=OFF')
        else:
            # Thallium is default
            if self.spec.variants['protocol'].value == 'ib':
                config_args.append('-DHCL_ENABLE_THALLIUM_ROCE=ON')
                config_args.append('-DHCL_ENABLE_THALLIUM_TCP=OFF')
                config_args.append('-DBASKET_ENABLE_RPCLIB=OFF')
            else:
                # tcp is default
                config_args.append('-DHCL_ENABLE_THALLIUM_TCP=ON')
                config_args.append('-DBASKET_ENABLE_RPCLIB=OFF')
                config_args.append('-DHCL_ENABLE_THALLIUM_ROCE=OFF')
        return config_args

    def set_include(self, env, path):
        env.append_flags('CFLAGS', '-I{}'.format(path))
        env.append_flags('CXXFLAGS', '-I{}'.format(path))

    def set_lib(self, env, path):
        env.prepend_path('LD_LIBRARY_PATH', path)
        env.append_flags('LDFLAGS', '-L{}'.format(path))

    def set_flags(self, env):
        self.set_include(env, '{}/include'.format(self.prefix))
        self.set_include(env, '{}/include'.format(self.prefix))
        self.set_lib(env, '{}/lib'.format(self.prefix))
        self.set_lib(env, '{}/lib64'.format(self.prefix))

    def setup_dependent_environment(self, spack_env, run_env, dependent_spec):
        self.set_flags(spack_env)

    def setup_run_environment(self, env):
        self.set_flags(env)
