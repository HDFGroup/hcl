#include <boost/interprocess/file_mapping.hpp>
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

#ifndef INCLUDE_HCL_UNORDERED_MAP_UNORDERED_MAP_CPP_
#define INCLUDE_HCL_UNORDERED_MAP_UNORDERED_MAP_CPP_

/* Constructor to deallocate the shared memory*/
template<typename KeyType, typename MappedType, typename Allocator, typename SharedType>
unordered_map<KeyType, MappedType,Allocator,SharedType>::~unordered_map() {
    if (is_server) {
        boost::interprocess::file_mapping::remove(backed_file.c_str());
    }
}

template<typename KeyType, typename MappedType, typename Allocator, typename SharedType>
unordered_map<KeyType, MappedType, Allocator, SharedType>::unordered_map(CharStruct name_)
    : num_servers(HCL_CONF->NUM_SERVERS),
      my_server(HCL_CONF->MY_SERVER),
      memory_allocated(HCL_CONF->MEMORY_ALLOCATED),
      is_server(HCL_CONF->IS_SERVER),
      segment(),
      name(name_),
      func_prefix(name_),
      myHashMap(),
      server_on_node(HCL_CONF->SERVER_ON_NODE),
      backed_file(HCL_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_+"_"+std::to_string(my_server)) {

    // init my_server, num_servers, server_on_node, processor_name from RPC
    AutoTrace trace = AutoTrace("hcl::unordered_map");

    /* create per server name for shared memory. Needed if multiple servers are
       spawned on one node*/
    this->name = this->name + std::string("_") + std::to_string(my_server);
    /* if current rank is a server */
    rpc = Singleton<RPCFactory>::GetInstance()->GetRPC(HCL_CONF->RPC_PORT);
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
  switch (HCL_CONF->RPC_IMPLEMENTATION) {
#ifdef HCL_ENABLE_RPCLIB
      case RPCLIB: {
          std::function<bool(KeyType &, MappedType &)> putFunc(
              std::bind(&unordered_map<KeyType, MappedType>::LocalPut, this,
                        std::placeholders::_1, std::placeholders::_2));
          std::function<bool(KeyType &,MappedType &)> getFunc(
              std::bind(&unordered_map<KeyType, MappedType>::LocalGet, this,
                        std::placeholders::_1,std::placeholders::_2));
          std::function<bool(KeyType &)> eraseFunc(
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
#ifdef HCL_ENABLE_THALLIUM_TCP
      case THALLIUM_TCP:
#endif
#ifdef HCL_ENABLE_THALLIUM_ROCE
      case THALLIUM_ROCE:
#endif
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
      {
          std::function<void(const tl::request &, KeyType &, MappedType &)> putFunc(
              std::bind(&unordered_map<KeyType, MappedType, Allocator, SharedType>::ThalliumLocalPut, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3));
          // std::function<void(const tl::request &, tl::bulk &, KeyType &)> putFunc(
          //     std::bind(&unordered_map<KeyType, MappedType>::ThalliumLocalPut, this,
          //               std::placeholders::_1, std::placeholders::_2,
          //               std::placeholders::_3));
          std::function<void(const tl::request &, KeyType &)> getFunc(
              std::bind(&unordered_map<KeyType, MappedType, Allocator, SharedType>::ThalliumLocalGet, this,
                        std::placeholders::_1, std::placeholders::_2));
          std::function<void(const tl::request &, KeyType &)> eraseFunc(
              std::bind(&unordered_map<KeyType, MappedType, Allocator, SharedType>::ThalliumLocalErase, this,
                        std::placeholders::_1, std::placeholders::_2));
          std::function<void(const tl::request &)> getAllDataInServerFunc(
              std::bind(&unordered_map<KeyType, MappedType, Allocator, SharedType>::ThalliumLocalGetAllDataInServer,
                        this, std::placeholders::_1));

          rpc->bind(func_prefix+"_Put", putFunc);
          rpc->bind(func_prefix+"_Get", getFunc);
          rpc->bind(func_prefix+"_Erase", eraseFunc);
          rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
          break;
      }
#endif
      default:
          break;
  }
        // srv->suppress_exceptions(true);
    } else if (!is_server && server_on_node) {
        segment = boost::interprocess::managed_mapped_file(boost::interprocess::open_only, backed_file.c_str());
        std::pair<MyHashMap *, boost::interprocess::managed_mapped_file::size_type> res;
        res = segment.find<MyHashMap>(name.c_str());
        myHashMap = res.first;
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
template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
bool unordered_map<KeyType, MappedType,Allocator,SharedType>::LocalPut(KeyType &key,
                                                  MappedType &data) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>lock(*mutex);
    auto value = GetData(data);
    myHashMap->insert_or_assign(key, value);
    return true;
}
/**
 * Put the data into the unordered map. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
bool unordered_map<KeyType, MappedType, Allocator, SharedType>::Put(KeyType &key,
                                             MappedType &data) {
    uint16_t key_int = (uint16_t)keyHash(key)% num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalPut(key, data);
    } else {
        return RPC_CALL_WRAPPER("_Put", key_int, bool,
                                key, data);
    }
}

/**
 * Get the data in the local unordered map.
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
std::pair<bool,MappedType>
unordered_map<KeyType, MappedType, Allocator, SharedType>::LocalGet(KeyType &key) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    typename MyHashMap::iterator iterator = myHashMap->find(key);
    if (iterator != myHashMap->end()) {
        return std::pair<bool,MappedType>(true,iterator->second);
    } else {
        return std::pair<bool,MappedType>(false,MappedType());
    }
}

/**
 * Get the data in the unordered map. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
std::pair<bool,MappedType>
unordered_map<KeyType, MappedType, Allocator, SharedType>::Get(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalGet(key);
    } else {
        typedef std::pair<bool,MappedType> ret_type;
       return RPC_CALL_WRAPPER("_Get", key_int, ret_type,key);
    }
}


template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
bool
unordered_map<KeyType, MappedType, Allocator, SharedType>::LocalErase(KeyType &key) {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    size_t s = myHashMap->erase(key);
    return s > 0;
}

template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
bool
unordered_map<KeyType, MappedType, Allocator, SharedType>::Erase(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalErase(key);
    } else {
      typedef bool ret_type;
      return RPC_CALL_WRAPPER("_Erase", key_int, ret_type,
			      key);
    }
}

template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
std::vector<std::pair<KeyType, MappedType>>
unordered_map<KeyType, MappedType, Allocator, SharedType>::GetAllData() {
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

template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
std::vector<std::pair<KeyType, MappedType>>
unordered_map<KeyType, MappedType, Allocator, SharedType>::LocalGetAllDataInServer() {
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

template<typename KeyType, typename MappedType,typename Allocator, typename SharedType>
std::vector<std::pair<KeyType, MappedType>>
unordered_map<KeyType, MappedType, Allocator, SharedType>::GetAllDataInServer() {
    if (server_on_node) {
        return LocalGetAllDataInServer();
    }
    else {
      typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
      auto my_server_i=my_server;
      return RPC_CALL_WRAPPER1("_GetAllData", my_server_i, ret_type);
    }
}
#endif  // INCLUDE_HCL_UNORDERED_MAP_UNORDERED_MAP_CPP_
