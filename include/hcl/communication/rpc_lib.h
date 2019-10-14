/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of HCL
 * 
 * HCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_HCL_COMMUNICATION_RPC_LIB_H_
#define INCLUDE_HCL_COMMUNICATION_RPC_LIB_H_

#include <hcl/common/constants.h>
#include <hcl/common/data_structures.h>
#include <hcl/common/debug.h>
#include <hcl/common/macros.h>
#include <hcl/common/singleton.h>
#include <hcl/common/typedefs.h>
#include <mpi.h>

/** RPC Lib Headers**/
#ifdef HCL_ENABLE_RPCLIB
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#endif
/** Thallium Headers **/
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
#include <thallium.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/complex.hpp>
#include <thallium/serialization/stl/deque.hpp>
#include <thallium/serialization/stl/forward_list.hpp>
#include <thallium/serialization/stl/list.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/multimap.hpp>
#include <thallium/serialization/stl/multiset.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/set.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/tuple.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/unordered_multimap.hpp>
#include <thallium/serialization/stl/unordered_multiset.hpp>
#include <thallium/serialization/stl/unordered_set.hpp>
#include <thallium/serialization/stl/vector.hpp>
#endif

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <future>

#if !defined(HCL_ENABLE_RPCLIB) && !defined(HCL_ENABLE_THALLIUM_TCP) && !defined(HCL_ENABLE_THALLIUM_ROCE)
#error "Please define an RPC backend (e.g., -DHCL_ENABLE_RPCLIB)"
#endif

namespace bip = boost::interprocess;
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
namespace tl = thallium;
#endif

class RPC {
private:
    uint16_t server_port;
    std::string name;
#ifdef HCL_ENABLE_RPCLIB
    std::shared_ptr<rpc::server> rpclib_server;
    // We can't use a std::vector<rpc::client> for these since rpc::client is neither copy
    // nor move constructible. See https://github.com/rpclib/rpclib/issues/128
    std::vector<std::unique_ptr<rpc::client>> rpclib_clients;
#endif
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
    std::shared_ptr<tl::engine> thallium_engine;
    CharStruct engine_init_str;
    std::vector<tl::endpoint> thallium_endpoints;
    void init_engine_and_endpoints(CharStruct conf);
    /*std::promise<void> thallium_exit_signal;

      void runThalliumServer(std::future<void> futureObj){

      while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){}
      thallium_engine->wait_for_finalize();
      }*/

#endif
    std::vector<CharStruct> server_list;
  public:
    ~RPC();

    RPC();

    template <typename F>
    void bind(CharStruct str, F func);

    void run(size_t workers = RPC_THREADS);

#ifdef HCL_ENABLE_THALLIUM_ROCE
    template<typename MappedType>
    MappedType prep_rdma_server(tl::endpoint endpoint, tl::bulk &bulk_handle);

    template<typename MappedType>
    tl::bulk prep_rdma_client(MappedType &data);
#endif
    /**
     * Response should be RPCLIB_MSGPACK::object_handle for rpclib and
     * tl::packed_response for thallium/mercury
     */
    template <typename Response, typename... Args>
    Response call(uint16_t server_index,
                  CharStruct const &func_name,
                  Args... args);
    template <typename Response, typename... Args>
    Response callWithTimeout(uint16_t server_index,
                  int timeout_ms,
                  CharStruct const &func_name,
                  Args... args);
    template <typename Response, typename... Args>
    std::future<Response> async_call(
            uint16_t server_index, CharStruct const &func_name, Args... args);

};

#include "rpc_lib.cpp"

#endif  // INCLUDE_HCL_COMMUNICATION_RPC_LIB_H_
