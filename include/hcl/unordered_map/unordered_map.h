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

#ifndef INCLUDE_HCL_UNORDERED_MAP_UNORDERED_MAP_H_
#define INCLUDE_HCL_UNORDERED_MAP_UNORDERED_MAP_H_

/**
 * Include Headers
 */

/** Standard C++ Headers**/
#include <cstdlib>
#include <iostream>
#include <functional>
#include <utility>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

#include <hcl/communication/rpc_lib.h>
#include <hcl/communication/rpc_factory.h>
#include <hcl/common/singleton.h>
#include <hcl/common/typedefs.h>

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
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered/unordered_map.hpp>

/** Namespaces Uses **/

/** Global Typedefs **/

namespace hcl {
/**
 * This is a Distributed HashMap Class. It uses shared memory + RPC to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the HashMap
 */
template<typename KeyType, typename MappedType, class Allocator=nullptr_t ,class SharedType=nullptr_t>
class unordered_map {
  private:
    std::hash<KeyType> keyHash;
    /** Class Typedefs for ease of use **/
    typedef std::pair<const KeyType, MappedType> ValueType;
    typedef boost::interprocess::allocator<ValueType, boost::interprocess::managed_mapped_file::segment_manager> ShmemAllocator;
    typedef boost::interprocess::managed_mapped_file managed_segment;
    typedef std::unordered_map<KeyType, MappedType, std::hash<KeyType>,
                                                                std::equal_to<KeyType>,
                                                                ShmemAllocator>
                                                                MyHashMap;
    /** Class attributes**/
    int num_servers;
    uint16_t  my_server;
    std::shared_ptr<RPC> rpc;
    really_long memory_allocated;
    bool is_server;
    CharStruct name, func_prefix;
    MyHashMap *myHashMap;
    boost::interprocess::interprocess_mutex* mutex;
    bool server_on_node;
    std::unordered_map<CharStruct, void*> binding_map;
    CharStruct backed_file;
  public:
    boost::interprocess::managed_mapped_file segment;

    ~unordered_map();
    explicit unordered_map(CharStruct name_ = std::string(std::getenv("USER") ? std::getenv("USER") : "") +
                           "_TEST_UNORDERED_MAP");

    template<typename A=Allocator>
    typename std::enable_if_t<std::is_same<A, nullptr_t>::value,MappedType>
    GetData(MappedType & data){
        return std::move(data);
    }

    template<typename A=Allocator>
    typename std::enable_if_t<! std::is_same<A, nullptr_t>::value,SharedType>
    GetData(MappedType & data){
        Allocator allocator(segment.get_segment_manager());
        SharedType value(allocator);
        value = std::move(data);
        return value;
    }

   template<typename ReturnType,typename... CB_Tuple_Args>
   ReturnType Call(CharStruct cb_name, CB_Tuple_Args... cb_args){
       auto iter =  binding_map.find(cb_name);
       if(iter!=binding_map.end()){
           std::function<ReturnType(CB_Tuple_Args...)> *cb_func=(std::function<ReturnType(CB_Tuple_Args...)> *)iter->second;
           (*cb_func)(std::forward<CB_Tuple_Args>(cb_args)...);
       }
   }


    void BindClient(std::string rpc_name);

    bool LocalPut(KeyType &key, MappedType &data);
    std::pair<bool,MappedType> LocalGet(KeyType &key);
    bool LocalErase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> LocalGetAllDataInServer();

#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE(LocalPut, (key,data) ,KeyType &key, MappedType &data)
    THALLIUM_DEFINE(LocalGet, (key) ,KeyType &key)
    THALLIUM_DEFINE(LocalErase, (key), KeyType &key)
    THALLIUM_DEFINE1(LocalGetAllDataInServer)
#endif

    bool Put(KeyType &key, MappedType &data);
    std::pair<bool,MappedType> Get(KeyType &key);
    bool Erase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> GetAllData();
    std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();
};

#include "unordered_map.cpp"

}  // namespace hcl

#endif  // INCLUDE_HCL_UNORDERED_MAP_UNORDERED_MAP_H_
