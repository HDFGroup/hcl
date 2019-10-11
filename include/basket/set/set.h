/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of Basket
 * 
 * Basket is free software: you can redistribute it and/or modify
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

#ifndef INCLUDE_BASKET_SET_SET_H_
#define INCLUDE_BASKET_SET_SET_H_

/**
 * Include Headers
 */

#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <basket/common/debug.h>
#include <basket/communication/rpc_factory.h>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#ifdef BASKET_ENABLE_RPCLIB
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#endif
/** Thallium Headers **/
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
#include <thallium.hpp>
#endif

/** Boost Headers **/
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <memory>
#include <string>
#include <set>
#include <vector>
#include <boost/interprocess/managed_mapped_file.hpp>

namespace basket {
/**
 * This is a Distributed Set Class. It uses shared memory + RPC + MPI to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the Set
 */

template<typename KeyType, typename Compare =
         std::less<KeyType>>
class set {
  private:
    std::hash<KeyType> keyHash;
    /** Class Typedefs for ease of use **/
    typedef boost::interprocess::allocator<KeyType, boost::interprocess::managed_mapped_file::segment_manager>
    ShmemAllocator;
    typedef boost::interprocess::set<KeyType, Compare, ShmemAllocator>
    MySet;
    /** Class attributes**/
    int comm_size, my_rank, num_servers;
    uint16_t  my_server;
    std::shared_ptr<RPC> rpc;
    really_long memory_allocated;
    bool is_server;
    boost::interprocess::managed_mapped_file segment;
    CharStruct name, func_prefix;
    MySet *myset;
    boost::interprocess::interprocess_mutex* mutex;
    bool server_on_node;
    CharStruct backed_file;

  public:
    ~set();

    explicit set(CharStruct name_ = std::string("TEST_SET"));

    bool LocalPut(KeyType &key);
    bool LocalGet(KeyType &key);
    bool LocalErase(KeyType &key);
    std::vector<KeyType> LocalGetAllDataInServer();
    std::vector<KeyType> LocalContainsInServer(KeyType &key_start, KeyType &key_end);
    std::pair<bool, KeyType> LocalSeekFirst();
    std::pair<bool, KeyType> LocalPopFirst();
    size_t LocalSize();
    std::pair<bool, std::vector<KeyType>> LocalSeekFirstN(uint32_t n);


#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE(LocalPut, (key), KeyType &key)
    THALLIUM_DEFINE(LocalGet, (key), KeyType &key)
    THALLIUM_DEFINE(LocalErase, (key), KeyType &key)
    THALLIUM_DEFINE(LocalContainsInServer, (key_start, key_end), KeyType &key_start,
		    KeyType &key_end)
    THALLIUM_DEFINE(LocalSeekFirstN, (n), uint32_t n)

    THALLIUM_DEFINE1(LocalSize)
    THALLIUM_DEFINE1(LocalSeekFirst)
    THALLIUM_DEFINE1(LocalPopFirst)
    THALLIUM_DEFINE1(LocalGetAllDataInServer)
#endif
    
    bool Put(KeyType &key);
    bool Get(KeyType &key);

    bool Erase(KeyType &key);
    std::vector<KeyType> Contains(KeyType &key_start,KeyType &key_end);

    std::vector<KeyType> GetAllData();

    std::vector<KeyType> ContainsInServer(KeyType &key_start,KeyType &key_end);
    std::vector<KeyType> GetAllDataInServer();
    std::pair<bool, KeyType> SeekFirst(uint16_t &key_int);
    std::pair<bool, KeyType> PopFirst(uint16_t &key_int);
    std::pair<bool, std::vector<KeyType>> SeekFirstN(uint16_t &key_int,uint32_t n);
    size_t Size(uint16_t &key_int);
};

#include "set.cpp"

}  // namespace basket

#endif  // INCLUDE_BASKET_SET_SET_H_
