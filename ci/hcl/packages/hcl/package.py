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
#     spack install rpclib
#
# You can edit this file again by typing:
#
#     spack edit rpclib
#
# See the Spack documentation for more information on packaging.
# ----------------------------------------------------------------------------

from spack import *


class Hcl(CMakePackage):
    """FIXME: Put a proper description of your package here."""

    homepage = "https://www.example.com"
    url     = "https://bitbucket.org/scs-io/hcl/get/master.tar.bz2"
    git     = "https://bitbucket.org/scs-io/hcl.git"
    # notify when the package is updated.
    # maintainers = ['github_user1', 'github_user2']
    #version('master', sha256='2857965ed3df95cc87f23c6b7c53b9403ceb87504b7c2be88b1c68ea9525d381')
    version('0.0.4', branch='release/0.0.4')
    version('0.0.5', branch='0.0.5')
    version('master', branch='master')
    depends_on('gcc@9.3.0')
    depends_on('mpi')
    depends_on('rpclib@2.2.1')
    depends_on('boost@1.74.0')
    def url_for_version(self, version):
        url = "https://bitbucket.org/scs-io/hcl/get/{0}.tar.bz2"
        return url.format(version.dashed)

    def cmake_args(self):
        args = ['-DCMAKE_INSTALL_PREFIX={}'.format(self.prefix),
                '-DBASKET_ENABLE_RPCLIB=ON']
        return args
    def set_include(self,env,path):
        env.append_flags('CFLAGS', '-I{}'.format(path))
        env.append_flags('CXXFLAGS', '-I{}'.format(path))
    def set_lib(self,env,path):
        env.prepend_path('LD_LIBRARY_PATH', path)
        env.append_flags('LDFLAGS', '-L{}'.format(path))
    def set_flags(self,env):
        self.set_include(env,'{}/include'.format(self.prefix))
        self.set_include(env,'{}/include'.format(self.prefix))
        self.set_lib(env,'{}/lib'.format(self.prefix))
        self.set_lib(env,'{}/lib64'.format(self.prefix))
    def setup_dependent_environment(self, spack_env, run_env, dependent_spec):
        self.set_flags(spack_env)
    def setup_run_environment(self, env):
        self.set_flags(env)
