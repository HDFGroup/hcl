#include <boost/interprocess/file_mapping.hpp>
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

#ifndef INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_CPP_
#define INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_CPP_

/* Constructor to deallocate the shared memory*/
template<typename KeyType, typename MappedType>
unordered_map<KeyType, MappedType>::~unordered_map() {
    if (is_server) {
        boost::interprocess::file_mapping::remove(backed_file.c_str());
    }
}

template<typename KeyType, typename MappedType>
unordered_map<KeyType, MappedType>::unordered_map(CharStruct name_)
        : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
          num_servers(BASKET_CONF->NUM_SERVERS),
          comm_size(1), my_rank(0), memory_allocated(BASKET_CONF->MEMORY_ALLOCATED),
          name(name_), segment(), myHashMap(), func_prefix(name_),
          backed_file(BASKET_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_+"_"+std::to_string(my_server)),
          server_on_node(BASKET_CONF->SERVER_ON_NODE) {
    // init my_server, num_servers, server_on_node, processor_name from RPC
    AutoTrace trace = AutoTrace("basket::unordered_map");

    /* Initialize MPI rank and size of world */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* create per server name for shared memory. Needed if multiple servers are
       spawned on one node*/
    this->name = this->name + std::string("_") + std::to_string(my_server);
    /* if current rank is a server */
    rpc = Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
    // rpc->copyArgs(&my_server, &num_servers, &server_on_node);
    if (is_server) {
        /* Delete existing instance of shared memory space*/
        boost::interprocess::file_mapping::remove(backed_file.c_str());
        /* allocate new shared memory space */
        segment = boost::interprocess::managed_mapped_file(boost::interprocess::create_only, backed_file.c_str(), memory_allocated);
        mutex = segment.construct<boost::interprocess::interprocess_mutex>( "mtx")();
        /* Construct unordered_map in the shared memory space. */
        myHashMap = segment.construct<MyHashMap>(name.c_str())(
            128, std::hash<KeyType>(), std::equal_to<KeyType>(),
            segment.get_allocator<ValueType>());
        /* Create a RPC server and map the methods to it. */
  switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
  case RPCLIB: {
        std::function<bool(KeyType &, MappedType &)> putFunc(
            std::bind(&unordered_map<KeyType, MappedType>::LocalPut, this,
                      std::placeholders::_1, std::placeholders::_2));
        std::function<std::pair<bool, MappedType>(KeyType &)> getFunc(
            std::bind(&unordered_map<KeyType, MappedType>::LocalGet, this,
                      std::placeholders::_1));
        std::function<std::pair<bool, MappedType>(KeyType &)> eraseFunc(
            std::bind(&unordered_map<KeyType, MappedType>::LocalErase, this,
                      std::placeholders::_1));
        std::function<std::vector<std::pair<KeyType, MappedType>>(void)>
                getAllDataInServerFunc(std::bind(
                    &unordered_map<KeyType, MappedType>::LocalGetAllDataInServer,
                    this));
        rpc->bind(func_prefix+"_Put", putFunc);
        rpc->bind(func_prefix+"_Get", getFunc);
        rpc->bind(func_prefix+"_Erase", eraseFunc);
        rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
	break;
  }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
  case THALLIUM_TCP:
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
  case THALLIUM_ROCE:
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    {

     std::function<void(const tl::request &, KeyType &, MappedType &)> putFunc(
            std::bind(&unordered_map<KeyType, MappedType>::ThalliumLocalPut, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3));
        // std::function<void(const tl::request &, tl::bulk &, KeyType &)> putFunc(
        //     std::bind(&unordered_map<KeyType, MappedType>::ThalliumLocalPut, this,
        //               std::placeholders::_1, std::placeholders::_2,
        //               std::placeholders::_3));
        std::function<void(const tl::request &, KeyType &)> getFunc(
            std::bind(&unordered_map<KeyType, MappedType>::ThalliumLocalGet, this,
                      std::placeholders::_1, std::placeholders::_2));
        std::function<void(const tl::request &, KeyType &)> eraseFunc(
            std::bind(&unordered_map<KeyType, MappedType>::ThalliumLocalErase, this,
                      std::placeholders::_1, std::placeholders::_2));
        std::function<void(const tl::request &)>
                getAllDataInServerFunc(std::bind(
                    &unordered_map<KeyType, MappedType>::ThalliumLocalGetAllDataInServer,
                    this, std::placeholders::_1));

        rpc->bind(func_prefix+"_Put", putFunc);
        rpc->bind(func_prefix+"_Get", getFunc);
        rpc->bind(func_prefix+"_Erase", eraseFunc);
        rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
	break;
    }
#endif
  }
        // srv->suppress_exceptions(true);
    }else if (!is_server && server_on_node) {
        segment = boost::interprocess::managed_mapped_file(boost::interprocess::open_only, backed_file.c_str());
        std::pair<MyHashMap *, boost::interprocess::managed_mapped_file::size_type> res;
        res = segment.find<MyHashMap>(name.c_str());
        myHashMap = res.first;
        size_t size = myHashMap->size();
        std::pair<boost::interprocess::interprocess_mutex *, boost::interprocess::managed_shared_memory::size_type> res2;
        res2 = segment.find<boost::interprocess::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
}

/**
 * Put the data into the local unordered map.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType>
bool unordered_map<KeyType, MappedType>::LocalPut(KeyType &key,
                                                  MappedType &data) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>lock(*mutex);
    myHashMap->insert_or_assign(key, data);
    
    return true;
}
/**
 * Put the data into the unordered map. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType>
bool unordered_map<KeyType, MappedType>::Put(KeyType &key,
                                             MappedType &data) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalPut(key, data);
    } else {
// #ifdef BASKET_ENABLE_THALLIUM_ROCE
//         tl::bulk bulk_handle = rpc->prep_rdma_client<MappedType>(data);
//         return RPC_CALL_WRAPPER("_Put", key_int, bool,
//                                 bulk_handle, key);
// #else
        return RPC_CALL_WRAPPER("_Put", key_int, bool,
                                key, data);
// #endif
    }
}

template<typename KeyType, typename MappedType>
template<typename CF, typename ReturnType,typename... ArgsType>
void unordered_map<KeyType, MappedType>::Bind(  CharStruct callback_name,
                                                std::function<ReturnType(ArgsType...)> callback_func,
                                                CharStruct caller_func_name,
                                                CF caller_func) {
    binding_map.insert_or_assign(callback_name, &callback_func);
    rpc->bind(caller_func_name, caller_func);
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,bool>
unordered_map<KeyType, MappedType>::LocalPutWithCallback(KeyType &key, MappedType &data, CharStruct cb_name, CB_Tuple_Args... cb_args){
    auto ret_1=LocalPut(key,data);
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return ret_1;
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<bool,ReturnType>> unordered_map<KeyType, MappedType>::LocalPutWithCallback(KeyType &key, MappedType &data,
                                                                                                                                                CharStruct cb_name,
                                                              CB_Tuple_Args... cb_args) {
    auto ret_1=LocalPut(key,data);
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return std::pair<bool,ReturnType>(ret_1,ret_2);
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<bool,ReturnType>> unordered_map<KeyType, MappedType>::PutWithCallback(KeyType &key, MappedType &data,
                                                                                                                                           CharStruct c_name,
                                                                                                                                           CharStruct cb_name,
                                                         CB_Args... cb_args) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalPutWithCallback<ReturnType>(key, data, cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::pair<bool,ReturnType> ret;
        return RPC_CALL_WRAPPER_CB(c_name, key_int, ret, key, data, cb_name);
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,bool> unordered_map<KeyType, MappedType>::PutWithCallback(KeyType &key, MappedType &data,
                                                                                                                    CharStruct c_name,
                                                                                                                    CharStruct cb_name,
                                                                                                                    CB_Args... cb_args) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalPutWithCallback<ReturnType>(key, data, cb_name, std::forward<CB_Args>(cb_args)...);
    } else {

        return RPC_CALL_WRAPPER_CB(c_name, key_int, bool, key, data, cb_name);
    }
}

/**
 * Get the data in the local unordered map.
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType>
std::pair<bool, MappedType>
unordered_map<KeyType, MappedType>::LocalGet(KeyType &key) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    typename MyHashMap::iterator iterator = myHashMap->find(key);
    if (iterator != myHashMap->end()) {
        return std::pair<bool, MappedType>(true, iterator->second);
    } else {
        return std::pair<bool, MappedType>(false, MappedType());
    }
}

/**
 * Get the data in the unordered map. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType>
std::pair<bool, MappedType>
unordered_map<KeyType, MappedType>::Get(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalGet(key);
    } else {
        typedef std::pair<bool, MappedType> ret_type;
       return RPC_CALL_WRAPPER("_Get", key_int, ret_type,key);
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>>
unordered_map<KeyType, MappedType>::LocalGetWithCallback(KeyType &key, CharStruct cb_name, CB_Tuple_Args... cb_args){
    auto ret_1=LocalGet(key);
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return ret_1;
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>>
        unordered_map<KeyType, MappedType>::LocalGetWithCallback(KeyType &key, CharStruct cb_name, CB_Tuple_Args... cb_args) {
    auto ret_1=LocalGet(key);
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return std::pair<decltype(ret_1),ReturnType>(ret_1,ret_2);
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>> unordered_map<KeyType, MappedType>::GetWithCallback(KeyType &key,
                                                                                                                                                                  CharStruct c_name,
                                                                                                                                                                  CharStruct cb_name,
                                                         CB_Args... cb_args) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalGetWithCallback<ReturnType>(key, cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::pair<std::pair<bool, MappedType>,ReturnType> ret;
        return RPC_CALL_WRAPPER_CB(c_name, key_int, ret, key, cb_name);
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>> unordered_map<KeyType, MappedType>::GetWithCallback(KeyType &key,
                                                                                                                                           CharStruct c_name,
                                                                                                                                                CharStruct cb_name,
                                                                                                                                                CB_Args... cb_args) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalGetWithCallback<ReturnType>(key, cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::pair<bool, MappedType> ret;
        return RPC_CALL_WRAPPER_CB(c_name, key_int, ret, key, cb_name);
    }
}

template<typename KeyType, typename MappedType>
std::pair<bool, MappedType>
unordered_map<KeyType, MappedType>::LocalErase(KeyType &key) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    size_t s = myHashMap->erase(key);
    
    return std::pair<bool, MappedType>(s > 0, MappedType());
}

template<typename KeyType, typename MappedType>
std::pair<bool, MappedType>
unordered_map<KeyType, MappedType>::Erase(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalErase(key);
    } else {
      typedef std::pair<bool, MappedType> ret_type;
      return RPC_CALL_WRAPPER("_Erase", key_int, ret_type,
			      key);
      // return rpc->call(key_int, func_prefix+"_Erase",
      //                  key).template as<std::pair<bool, MappedType>>();
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>>
unordered_map<KeyType, MappedType>::LocalEraseWithCallback(KeyType &key, std::string cb_name, CB_Tuple_Args... cb_args){
    auto ret_1=LocalErase(key);
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return ret_1;
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>> unordered_map<KeyType, MappedType>::LocalEraseWithCallback(KeyType &key,
                                                              std::string cb_name,
                                                              CB_Tuple_Args... cb_args) {
    auto ret_1=LocalErase(key);
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return std::pair<decltype(ret_1),ReturnType>(ret_1,ret_2);
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::pair<bool, MappedType>,ReturnType>> unordered_map<KeyType, MappedType>::EraseWithCallback(KeyType &key,
                                                         std::string c_name,
                                                         std::string cb_name,
                                                         CB_Args... cb_args) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalEraseWithCallback<ReturnType>(key, cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::pair<std::pair<bool, MappedType>,ReturnType> ret;
        return RPC_CALL_WRAPPER_CB(c_name, key_int, ret, key, cb_name);
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,std::pair<bool, MappedType>> unordered_map<KeyType, MappedType>::EraseWithCallback(KeyType &key,
                                                                                                                                                std::string c_name,
                                                                                                                                                std::string cb_name,
                                                                                                                                                CB_Args... cb_args) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalEraseWithCallback<ReturnType>(key, cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::pair<bool, MappedType> ret;
        return RPC_CALL_WRAPPER_CB(c_name, key_int, ret, key, cb_name);
    }
}

template<typename KeyType, typename MappedType>
std::vector<std::pair<KeyType, MappedType>>
unordered_map<KeyType, MappedType>::GetAllData() {
    std::vector<std::pair<KeyType, MappedType>> final_values =
            std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = GetAllDataInServer();
    final_values.insert(final_values.end(), current_server.begin(),
                        current_server.end());
    for (int i = 0; i < num_servers; ++i) {
        if (i != my_server) {
	  
            typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
            auto server = RPC_CALL_WRAPPER1("_GetAllData",i, ret_type);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType>
std::vector<std::pair<KeyType, MappedType>>
unordered_map<KeyType, MappedType>::LocalGetAllDataInServer() {
    std::vector<std::pair<KeyType, MappedType>> final_values =
            std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
                lock(*mutex);
        typename MyHashMap::iterator lower_bound;
        if (myHashMap->size() > 0) {
            lower_bound = myHashMap->begin();
            while (lower_bound != myHashMap->end()) {
                final_values.push_back(std::pair<KeyType, MappedType>(
                    lower_bound->first, lower_bound->second));
                lower_bound++;
            }
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType>
std::vector<std::pair<KeyType, MappedType>>
unordered_map<KeyType, MappedType>::GetAllDataInServer() {
    if (server_on_node) {
        return LocalGetAllDataInServer();
    }
    else {
      typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
      auto my_server_i=my_server;
      return RPC_CALL_WRAPPER1("_GetAllData", my_server_i, ret_type);
      // return rpc->call(
      // 		       my_server, func_prefix+"_GetAllData").template
      // 	as<std::vector<std::pair<KeyType, MappedType>>>();
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,std::vector<std::pair<bool, MappedType>>>
unordered_map<KeyType, MappedType>::LocalGetAllDataInServerWithCallback(std::string cb_name, CB_Tuple_Args... cb_args){
    auto ret_1=LocalGetAllDataInServer();
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return ret_1;
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Tuple_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::vector<std::pair<bool, MappedType>>,ReturnType>>
unordered_map<KeyType, MappedType>::LocalGetAllDataInServerWithCallback(std::string cb_name,
                                                                        CB_Tuple_Args... cb_args) {
    auto ret_1=LocalGetAllDataInServer();
    auto ret_2=Call<ReturnType>(cb_name,std::forward<CB_Tuple_Args>(cb_args)...);
    return std::pair<decltype(ret_1),ReturnType>(ret_1,ret_2);
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<!std::is_void<ReturnType>::value,std::pair<std::vector<std::pair<bool, MappedType>>,ReturnType>>
unordered_map<KeyType, MappedType>::GetAllDataInServerWithCallback(std::string c_name,
                                                                   std::string cb_name,
                                                                   CB_Args... cb_args) {
    if (server_on_node) {
        return LocalGetAllDataInServerWithCallback<ReturnType>(cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::pair<std::vector<std::pair<bool, MappedType>>,ReturnType> ret;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER_CB(c_name, my_server_i, ret, cb_name);
    }
}

template<typename KeyType, typename MappedType>
template<typename ReturnType,typename... CB_Args>
typename std::enable_if_t<std::is_void<ReturnType>::value,std::vector<std::pair<bool, MappedType>>>
unordered_map<KeyType, MappedType>::GetAllDataInServerWithCallback(std::string c_name,
                                                                   std::string cb_name,
                                                                   CB_Args... cb_args) {
    if (server_on_node) {
        return LocalGetAllDataInServerWithCallback<ReturnType>(cb_name, std::forward<CB_Args>(cb_args)...);
    } else {
        typedef std::vector<std::pair<bool, MappedType>> ret;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER_CB(c_name, my_server_i, ret, cb_name);
    }
}
#endif  // INCLUDE_BASKET_UNORDERED_MAP_UNORDERED_MAP_CPP_
