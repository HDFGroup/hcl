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

#ifndef INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_H_
#define INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_H_

/**
 * Include Headers
 */

/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

#include <basket/communication/rpc_lib.h>
#include <basket/communication/rpc_factory.h>
#include <basket/common/singleton.h>
#include <basket/common/typedefs.h>


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
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

/** Namespaces Uses **/

/** Global Typedefs **/

namespace basket {
/**
 * This is a Distributed HashMap Class. It uses shared memory + RPC + MPI to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the HashMap
 */
template<typename KeyType, typename MappedType>
class unordered_map {
  private:
    std::hash<KeyType> keyHash;
    /** Class Typedefs for ease of use **/
    typedef std::pair<const KeyType, MappedType> ValueType;
    typedef boost::interprocess::allocator<ValueType, boost::interprocess::managed_mapped_file::segment_manager> ShmemAllocator;
    typedef boost::interprocess::managed_mapped_file managed_segment;
    typedef boost::unordered::unordered_map<KeyType, MappedType, std::hash<KeyType>,
                                                                std::equal_to<KeyType>,
                                                                ShmemAllocator>
                                                                MyHashMap;
    /** Class attributes**/
    int comm_size, my_rank, num_servers;
    uint16_t  my_server;
    std::shared_ptr<RPC> rpc;
    really_long memory_allocated;
    bool is_server;
    boost::interprocess::managed_mapped_file segment;
    CharStruct name, func_prefix;
    MyHashMap *myHashMap;
    boost::interprocess::interprocess_mutex* mutex;
    bool server_on_node;
    std::unordered_map<CharStruct, void*> binding_map;
    CharStruct backed_file;

  public:
    ~unordered_map();

    explicit unordered_map(CharStruct name_ = std::string("TEST_UNORDERED_MAP"));

   /* template <typename F>
    void Bind(std::string rpc_name, F fun);*/

   template<typename CF, typename ReturnType,typename... ArgsType>
   void Bind(CharStruct rpc_name, std::function<ReturnType(ArgsType...)> callback_func, CharStruct caller_func_name, CF caller_func);

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
    std::pair<bool, MappedType> LocalGet(KeyType &key);
    std::pair<bool, MappedType> LocalErase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> LocalGetAllDataInServer();

#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE(LocalPut, (key,data) ,KeyType &key, MappedType &data)

    // void ThalliumLocalPut(const tl::request &thallium_req, tl::bulk &bulk_handle, KeyType key) {
    //     MappedType data = rpc->prep_rdma_server<MappedType>(thallium_req.get_endpoint(), bulk_handle);
    //     thallium_req.respond(LocalPut(key, data));
    // }

    // void ThalliumLocalGet(const tl::request &thallium_req, KeyType key) {
    //     auto retpair = LocalGet(key);
    //     if (!retpair.first) {
    //         printf("error\n");
    //     }
    //     MappedType data = retpair.second;
    //     tl::bulk bulk_handle = rpc->prep_rdma_client<MappedType>(data);
    //     thallium_req.respond(bulk_handle);
    // }

    THALLIUM_DEFINE(LocalGet, (key), KeyType &key)
    THALLIUM_DEFINE(LocalErase, (key), KeyType &key)
    THALLIUM_DEFINE1(LocalGetAllDataInServer)
#endif

    bool Put(KeyType &key, MappedType &data);
    std::pair<bool, MappedType> Get(KeyType &key);
    std::pair<bool, MappedType> Erase(KeyType &key);
    std::vector<std::pair<KeyType, MappedType>> GetAllData();
    std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,bool>
    LocalPutWithCallback(KeyType &key, MappedType &data,
                         CharStruct cb_name,
                         CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<bool,ReturnType>>
    LocalPutWithCallback(KeyType &key, MappedType &data,
                         CharStruct cb_name,
                         CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<bool,ReturnType>>
    PutWithCallback(KeyType &key, MappedType &data,
                    CharStruct c_name,
                    CharStruct cb_name,
                    CB_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
        typename std::enable_if_t<std::is_void<ReturnType>::value,bool>
    PutWithCallback(KeyType &key, MappedType &data,
                    CharStruct c_name,
                    CharStruct cb_name,
                    CB_Args... cb_args);


    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>>
    LocalGetWithCallback(KeyType &key,
                         CharStruct cb_name,
                         CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>>
    LocalGetWithCallback(KeyType &key,
                         CharStruct cb_name,
                         CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>>
    GetWithCallback(KeyType &key,
                    CharStruct c_name,
                    CharStruct cb_name,
                    CB_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>>
    GetWithCallback(KeyType &key,
                    CharStruct c_name,
                    CharStruct cb_name,
                    CB_Args... cb_args);

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>>
    LocalEraseWithCallback(KeyType &key,
                         std::string cb_name,
                         CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>>
    LocalEraseWithCallback(KeyType &key,
                         std::string cb_name,
                         CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>>
    EraseWithCallback(KeyType &key,
                    std::string c_name,
                    std::string cb_name,
                    CB_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>>
    EraseWithCallback(KeyType &key,
                    std::string c_name,
                    std::string cb_name,
                    CB_Args... cb_args);

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,std::vector<std::pair<bool, MappedType>>>
    LocalGetAllDataInServerWithCallback(std::string cb_name,
                                        CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Tuple_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::vector<std::pair<bool, MappedType>>,ReturnType>>
    LocalGetAllDataInServerWithCallback(std::string cb_name,
                                        CB_Tuple_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::vector<std::pair<bool, MappedType>>,ReturnType>>
    GetAllDataInServerWithCallback(std::string c_name,
                                   std::string cb_name,
                                   CB_Args... cb_args);

    template<typename ReturnType,typename... CB_Args>
    typename std::enable_if_t<std::is_void<ReturnType>::value,std::vector<std::pair<bool, MappedType>>>
    GetAllDataInServerWithCallback(std::string c_name,
                                   std::string cb_name,
                                   CB_Args... cb_args);
};

#include "unordered_map.cpp"

}  // namespace basket

#endif  // INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_H_
