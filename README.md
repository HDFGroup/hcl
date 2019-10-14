# HCL: Hermes Container Library

HCL is a distributed data structure library. It consists of the following
templated data structures:

 * global_clock
 * map
 * multimap
 * priority_queue
 * queue
 * global_sequence (sequencer)
 * set
 * unordered_map

## Compilation

The HCL Library compiles with cmake, so the general procedure is

```bash
cd hcl
mkdir build
cmake -HCL_ENABLE_RPCLIB=true ..
make
sudo make install
```
If you want to install somewhere besides `/usr/local`, then use

```bash
cd hcl
mkdir build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/wherever -DHCL_ENABLE_RPCLIB=true ..
make
make install
```

A flag should be added to cmake to indicate the preferred RPC library, otherwise
compilation will fail. If compiling with RPCLib, use `-DHCL_ENABLE_RPCLIB`. If
compiling with Thallium, use either `-DHCL_ENABLE_THALLIUM_TCP` or
`-DHCL_ENABLE_THALLIUM_ROCE`

### Dependencies
- MPI
- Boost (interprocess module)
- RPC layer (pick one and compile appropriately)
  - rpclib
  - Thallium (wrapper over Mercury)
- glibc (for librt and posix threads)

#### Recommended Versions

HCL has been tested with mpich 3.3.1, boost 1.69.0, rpclib 2.2.1, mercury 1.0.1,
margo 0.5, and thallium 0.4.0. Please consider patching mercury using the patch
specified below, especially if you're using the Thallium RoCE transport. Also of
note is that we uses libfabric (ofi) as the default Mercury transport (for TCP
and RoCE (verbs)). We used libfabric version 1.8.x. If you would rather use a
different Mercury transport, please change the configuration via
include/hcl/common/configuration_manager.h. The TCP_CONF string is used for tcp
via Mercury, and the VERBS_CONF string is for verbs via Mercury. The
VERBS_DOMAIN is the domain used for RoCE.

## Usage

Since libhcl uses MPI, data structures have to be declared on the server and
clients. In the `test` directory, you will find examples of how to use each data
structure, and also for the clock and sequencer. Data structures typically
assume that we are running the server and clients on the same node, but this
need not be the case. You can easily configure clients to work with servers that
are not on their node.

### Structure Initialization

Structure configuration is managed via the following macros:

``` c++
HCL_CONF->IS_SERVER = is_server;
HCL_CONF->MY_SERVER = my_server;
HCL_CONF->NUM_SERVERS = num_servers;
HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
HCL_CONF->SERVER_LIST_PATH = "./server_list";
```

 * `IS_SERVER`: `true` when the data structure in the current process will act as a
   server (store the internal data structure), `false` otherwise.

 * `MY_SERVER`: The relative rank of your server (if servers are on ranks 3 and
   5, server relative ranks are still 0 and 1)

 * `NUM_SERVERS`: The total number of servers.

 * `SERVER_ON_NODE`: `true` when `MY_SERVER` is on the same node as the current process.

 * `SERVER_LIST_PATH`: An absolute or relative path to a file that lists
   `NUM_SERVERS` hostnames or addresses, one per line, of the server processes.

Constructor example:

``` c++
hcl::unordered_map(std::string name);
```

 * `name`: A unique name used to identify the shared memory.
