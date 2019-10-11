# Basket Library

The Basket Library, or libbasket, is a distributed data structure
library. It consists of several templated data structures built on top
of MPI, including a hashmap, map, multimap, priority queue, and
message queue. There's also a global clock and a sequencer.

## Compilation

The Basket Library compiles with cmake, so the general procedure is

```bash
cd basket
mkdir build
cmake -DBASKET_ENABLE_RPCLIB=true ..
make
sudo make install
```
If you want to install somewhere besides `/usr/local`, then use

```bash
cd basket
mkdir build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/wherever -DBASKET_ENABLE_RPCLIB=true ..
make
make install
```

A flag should be added to cmake to indicate preferred RPC library,
otherwise compilation will fail. If compiling with RPCLib, use
-DBASKET_ENABLE_RPCLIB. If compiling with Thallium, use either
-DBASKET_ENABLE_THALLIUM_TCP or -DBASKET_ENABLE_THALLIUM_ROCE

### Dependencies
- mpi
- boost
- RPC (pick one and compile appropriately)
  - rpclib
  - Thallium (wrapper over Mercury)
- glibc (for librt and posix threads)

#### Recommended Versions
We tested with mpich 3.3.1, boost 1.69.0, rpclib 2.2.1, mercury 1.0.1,
and thallium 0.4.0. Please consider patching mercury using the patch
specified below, especially if you're using the Thallium RoCE
transport. Also of note is that we uses libfabric (ofi) as the default
Mercury transport (for TCP and RoCE (verbs)). We used libfabric
version 1.8.x. If you would rather use a different Mercury transport,
please change the configuration via
include/basket/common/configuration_manager.h. The TCP_CONF string is
what we use for tcp via Mercury, and the VERBS_CONF string is how we
do verbs via Mercury. The VERBS_DOMAIN is the domain used for RoCE.

## Usage

Since libbasket uses MPI, data structures have to be declared on the
server and clients. Each data structure is declared with a name, a
boolean to indicate whether it is on the server or not, the MPI rank
of the server, and the number of servers it utilizes (generally
one). In the test/ directory, you will find examples for how to use
each data structure, and also for the clock and sequencer. Data
structures typically assume that we are running the server and clients
on the same node, but this need not be the case. However, currently
please keep a server on each node in the system, because server lists
are initialized on a shared memory right now, so clients won't be able
to find servers if there is no server to place the server list in
the shared memory on their node. That said, you can easily configure
clients to work with servers that are not on their node.

### Structure Initialization

When creating a basket structure, you currently need to pass a lot of
parameters. For example:

basket::unordered_map(std::string name_, bool is_server_,
                      uint16_t my_server_, int num_servers_,
                      bool server_on_node_,
                      std::string processor_name_ = "");

name should be a unique name used to initialize the shared memory.

is_server should be true when we are on a server, false otherwise.

my_server should be the relative rank of your server (if servers are
on ranks 3 and 5, server relative ranks are still 0 and 1)

server_on_node is true when my_server is on the current node.

processor_name only needs to be initialized when you want to run
servers on a special hostname (i.e. when you're using RoCE). In Ares
we use this to add "-40g" to the name of the processor to switch to
the 40 Gbit network.

### unordered_map

unordered_map makes the assumption that a node is running a server and
multiple clients. the unordered_map_test will perform puts and gets on a
std::unordered_map locally, a basket::unordered_map locally, and a
basket::unordered_map remotely.

### Other Structures

Basket also has queues, priority_queues, multimaps, maps,
global_clocks, and global_sequence (sequencers). However, the tests
for these structures are not ironed out yet. The structures are fully
functional, however, and you should be able to figure them out by
analogy with unordered_map.

### Testing on Ares cluster

When testing on Ares cluster, make sure to change the hostfile at
test/hostfile. It should have two mpi processes on the first node, and
two on the second, as in:

ares-comp-01:2
ares-comp-02:2

You can either run tests using ctest or mpirun. If you aren't familiar
with the tests, ctest is preferred.

`ctest -N` lists the tests you can run.

`ctest -V -R ares_unordered_map_test_MPI_4_2_500_1000_1_0` runs the
unordered_map test.

`ctest -V` runs all tests available.

The command to run a test without ctest is:

LD_PRELOAD=`pwd`/libbasket.so mpirun -f test/hostfile -n 4
test/unordered_map_test 2 500 1000 1 0

The arguments are ranks_per_server, num_requests, size_of_request,
server_on_node, and debug. The variable size_of_request should be
equal to TEST_REQUEST_SIZE in include/basket/common/constants.h, since
we have not yet found a way to make dynamically configurable request
sizes (due to serialization).

## Configure

```bash
$ mkdir $HOME/basket_build
$ cd $HOME/basket_build
$ $HOME/software/install/bin/cmake \
-DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_C_COMPILER=/opt/ohpc/pub/compiler/gcc/7.3.0/bin/gcc \
-DCMAKE_CXX_COMPILER=/opt/ohpc/pub/compiler/gcc/7.3.0/bin/g++ \
"-DCMAKE_CXX_FLAGS=-I${HOME}/software/install/include -L${HOME}/software/install/lib"\
-G "CodeBlocks - Unix Makefiles" $SRC_DIR
```

where $SRC_DIR = source directory (e.g. /tmp/tmp.bYNfITLGMr)

## Compile

```bash
$ $HOME/software/install/bin/cmake --build ./ --target all -- -j 8
```

## Run

```bash
$ cd $HOME/basket_build/test
$ ctest -V
```
## Patching Mercury 1.0.1 to work with RoCE

For Basket to work with Mercury 1.0.1 with RoCE, it needs a patched
version of Mercury. The patch allows you to specify a domain to be
used when connecting to a verbs interface (rather than looking up the
domain), which is the only practical way to use RoCE. This patch will
not work with other versions of Mercury, and it certainly won't work
with previous versions, since it changes lookup syntax, and therefore
assumes that lookup syntax is as in Mercury 1.0.1 to begin with
(lookup syntax was actually changed in Mercury 1.0.1, so this
certainly shouldn't work with older versions).

Assuming that Mercury 1.0.1 has been extracted to `./mercury-1.0.1`
run our patch `mercury-1.0.1-RoCE.patch` in the `RoCE_Patch` folder as
such

```bash
$ patch  -p0 <  mercury-1.0.1-RoCE.patch
```
