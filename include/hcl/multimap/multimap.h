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

#ifndef INCLUDE_HCL_MULTIMAP_MULTIMAP_H_
#define INCLUDE_HCL_MULTIMAP_MULTIMAP_H_

/**
 * Include Headers
 */
#include <hcl/communication/rpc_lib.h>
#include <hcl/communication/rpc_factory.h>
#include <hcl/common/singleton.h>
#include <hcl/common/debug.h>
/** RPC Lib Headers**/
#ifdef HCL_ENABLE_RPCLIB
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#endif
/** Thallium Headers **/
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
#include <thallium.hpp>
#endif

/** Boost Headers **/
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
/** Standard C++ Headers**/
#include <cstdlib>
#include <iostream>
#include <functional>
#include <utility>
#include <memory>
#include <string>
#include <vector>

namespace hcl {
/**
 * This is a Distributed MultiMap Class. It uses shared memory + RPC to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the MultiMap
 */
template<typename KeyType, typename MappedType, typename Compare =
         std::less<KeyType>>
class multimap {
  private:
    std::hash<KeyType> keyHash;
    /** Class Typedefs for ease of use **/
    typedef std::pair<const KeyType, MappedType> ValueType;
    typedef boost::interprocess::allocator<
        ValueType, boost::interprocess::managed_mapped_file::segment_manager>
    ShmemAllocator;
    typedef boost::interprocess::multimap<KeyType, MappedType, Compare,
                                          ShmemAllocator> MyMap;
    /** Class attributes**/
    int num_servers;
    uint16_t  my_server;
    std::shared_ptr<RPC> rpc;
    really_long memory_allocated;
    bool is_server;
    boost::interprocess::managed_mapped_file segment;
    std::string name, func_prefix;
    MyMap *mymap;
    boost::interprocess::interprocess_mutex* mutex;
    bool server_on_node;
    CharStruct backed_file;

  public:
    /* Constructor to deallocate the shared memory*/
    ~multimap();

    explicit multimap(std::string name_ = std::string(std::getenv("USER") ? std::getenv("USER") : "") +
                      "_TEST_MULTIMAP");

    bool LocalPut(KeyType &key, MappedType &data);
    std::pair<bool, MappedType> LocalGet(KeyType &key);
    std::pair<bool, MappedType> LocalErase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> LocalContainsInServer(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> LocalGetAllDataInServer();

#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE(LocalPut, (key, data), KeyType &key, MappedType &data)
    THALLIUM_DEFINE(LocalGet, (key), KeyType &key)
    THALLIUM_DEFINE(LocalErase, (key), KeyType &key)
    THALLIUM_DEFINE(LocalContainsInServer, (key), KeyType &key)
    THALLIUM_DEFINE1(LocalGetAllDataInServer)

#endif

    bool Put(KeyType &key, MappedType &data);
    std::pair<bool, MappedType> Get(KeyType &key);

    std::pair<bool, MappedType> Erase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> Contains(KeyType &key);

    std::vector<std::pair<KeyType, MappedType>> GetAllData();

    std::vector<std::pair<KeyType, MappedType>> ContainsInServer(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();
};

#include "multimap.cpp"

}  // namespace hcl

#endif  // INCLUDE_HCL_MULTIMAP_MULTIMAP_H_
