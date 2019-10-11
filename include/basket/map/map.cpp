
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

#ifndef INCLUDE_BASKET_MAP_MAP_CPP_
#define INCLUDE_BASKET_MAP_MAP_CPP_

/* Constructor to deallocate the shared memory*/
template<typename KeyType, typename MappedType, typename Compare>
map<KeyType, MappedType, Compare>::~map() {
    if (is_server)
        boost::interprocess::file_mapping::remove(backed_file.c_str());
}

template<typename KeyType, typename MappedType, typename Compare>
map<KeyType, MappedType, Compare>::map(std::string name_)
        : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
          num_servers(BASKET_CONF->NUM_SERVERS),
          comm_size(1), my_rank(0), memory_allocated(BASKET_CONF->MEMORY_ALLOCATED),
          name(name_), segment(), mymap(), func_prefix(name_),
          backed_file(BASKET_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_+"_"+std::to_string(my_server)),
          server_on_node(BASKET_CONF->SERVER_ON_NODE)
{
    AutoTrace trace = AutoTrace("basket::map");
    /* Initialize MPI rank and size of world */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* create per server name for shared memory. Needed if multiple servers are
       spawned on one node*/
    this->name += "_" + std::to_string(my_server);
    /* if current rank is a server */
    rpc = Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
    if (is_server) {
        /* Delete existing instance of shared memory space*/
        boost::interprocess::file_mapping::remove(backed_file.c_str());
        /* allocate new shared memory space */
        segment = boost::interprocess::managed_mapped_file(
            boost::interprocess::create_only, backed_file.c_str(), memory_allocated);
        ShmemAllocator alloc_inst(segment.get_segment_manager());
        /* Construct map in the shared memory space. */
        mymap = segment.construct<MyMap>(name.c_str())(Compare(), alloc_inst);
        mutex = segment.construct<boost::interprocess::interprocess_mutex>(
            "mtx")();
        /* Create a RPC server and map the methods to it. */
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                std::function<bool(KeyType &, MappedType &)> putFunc(
                    std::bind(&map<KeyType, MappedType, Compare>::LocalPut, this,
                              std::placeholders::_1, std::placeholders::_2));
                std::function<std::pair<bool, MappedType>(KeyType &)> getFunc(
                    std::bind(&map<KeyType, MappedType, Compare>::LocalGet, this,
                              std::placeholders::_1));
                std::function<std::pair<bool, MappedType>(KeyType &)> eraseFunc(
                    std::bind(&map<KeyType, MappedType, Compare>::LocalErase, this,
                              std::placeholders::_1));
                std::function<std::vector<std::pair<KeyType, MappedType>>(void)>
                        getAllDataInServerFunc(std::bind(
                            &map<KeyType, MappedType, Compare>::LocalGetAllDataInServer,
                            this));
                std::function<std::vector<std::pair<KeyType, MappedType>>(KeyType &,KeyType&)>
                        containsInServerFunc(std::bind(&map<KeyType, MappedType,
                                                       Compare>::LocalContainsInServer, this,
                                                       std::placeholders::_1, std::placeholders::_2));

                rpc->bind(func_prefix+"_Put", putFunc);
                rpc->bind(func_prefix+"_Get", getFunc);
                rpc->bind(func_prefix+"_Erase", eraseFunc);
                rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                rpc->bind(func_prefix+"_Contains", containsInServerFunc);
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
                        std::bind(&map<KeyType, MappedType, Compare>::ThalliumLocalPut, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3));
                    std::function<void(const tl::request &, KeyType &)> getFunc(
                        std::bind(&map<KeyType, MappedType, Compare>::ThalliumLocalGet, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &, KeyType &)> eraseFunc(
                        std::bind(&map<KeyType, MappedType, Compare>::ThalliumLocalErase, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &)>
                            getAllDataInServerFunc(std::bind(
                                &map<KeyType, MappedType, Compare>::ThalliumLocalGetAllDataInServer,
                                this, std::placeholders::_1));
                    std::function<void(const tl::request &, KeyType &, KeyType &)>
                            containsInServerFunc(std::bind(&map<KeyType, MappedType,
                                                           Compare>::ThalliumLocalContainsInServer, this,
                                                           std::placeholders::_1,
							   std::placeholders::_2,
							   std::placeholders::_3));

                    rpc->bind(func_prefix+"_Put", putFunc);
                    rpc->bind(func_prefix+"_Get", getFunc);
                    rpc->bind(func_prefix+"_Erase", eraseFunc);
                    rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                    rpc->bind(func_prefix+"_Contains", containsInServerFunc);
                    break;
                }
#endif
        }
    }else if (!is_server && server_on_node) {
       /* Map the clients to their respective memory pools */
       segment = boost::interprocess::managed_mapped_file(
            boost::interprocess::open_only, backed_file.c_str());
        std::pair<MyMap*,
                  boost::interprocess::managed_mapped_file::size_type> res;
        res = segment.find<MyMap> (name.c_str());
        mymap = res.first;
        std::pair<boost::interprocess::interprocess_mutex *,
                  boost::interprocess::managed_mapped_file::size_type> res2;
        res2 = segment.find<boost::interprocess::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
}

/**
 * Put the data into the local map.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType, typename Compare>
bool map<KeyType, MappedType, Compare>::LocalPut(KeyType &key,
                                                 MappedType &data) {
    AutoTrace trace = AutoTrace("basket::map::Put(local)", key, data);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
    mymap->insert_or_assign(key, data);
    /*typename MyMap::iterator iterator = mymap->find(key);
      if (iterator != mymap->end()) {
      mymap->erase(iterator);
      }
      mymap->insert(std::pair<KeyType, MappedType>(key, data));*/
    return true;
}

/**
 * Put the data into the map. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType, typename Compare>
bool map<KeyType, MappedType, Compare>::Put(KeyType &key,
                                            MappedType &data) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalPut(key, data);
    } else {
        AutoTrace trace = AutoTrace("basket::map::Put(remote)", key, data);
        return RPC_CALL_WRAPPER("_Put", key_int, bool,
                                key, data);
    }
}

/**
 * Get the data in the local map.
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then
 * data was found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::LocalGet(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::map::Get(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    typename MyMap::iterator iterator = mymap->find(key);
    if (iterator != mymap->end()) {
        return std::pair<bool, MappedType>(true, iterator->second);
    } else {
        return std::pair<bool, MappedType>(false, MappedType());
    }
}

/**
 * Get the data in the map. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then
 * data was found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::Get(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalGet(key);
    } else {
        AutoTrace trace = AutoTrace("basket::map::Get(remote)", key);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER("_Get", key_int, ret_type,
                                key);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::LocalErase(KeyType &key) {
    AutoTrace trace = AutoTrace("basket::map::Erase(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    size_t s = mymap->erase(key);
    return std::pair<bool, MappedType>(s > 0, MappedType());
}

template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::Erase(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalErase(key);
    } else {
        AutoTrace trace = AutoTrace("basket::map::Erase(remote)", key);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER("_Erase", key_int, ret_type, key);
    }
}

/**
 * Get the data into the map. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::Contains(KeyType &key_start,KeyType &key_end) {
    AutoTrace trace = AutoTrace("basket::map::Contains", key_start,key_end);
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = ContainsInServer(key_start,key_end);
    final_values.insert(final_values.end(), current_server.begin(), current_server.end());
    for (int i = 0; i < num_servers; ++i) {
        if (i != my_server) {
            typedef std::vector<std::pair<KeyType, MappedType>> ret_type;
            auto server = RPC_CALL_WRAPPER("_Contains", i, ret_type, key_start,key_end);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::GetAllData() {
    AutoTrace trace = AutoTrace("basket::map::GetAllData");
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = GetAllDataInServer();
    final_values.insert(final_values.end(), current_server.begin(), current_server.end());
    for (int i = 0; i < num_servers ; ++i) {
        if (i != my_server) {
            typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
            auto server = RPC_CALL_WRAPPER1("_GetAllData", i, ret_type);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::LocalContainsInServer(KeyType &key_start,KeyType &key_end) {
    AutoTrace trace = AutoTrace("basket::map::ContainsInServer", key_start,key_end);
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
        typename MyMap::iterator lower_bound;
        size_t size = mymap->size();
        if (size == 0) {
        } else if (size == 1) {
            lower_bound = mymap->begin();

            if(lower_bound->first > key_start)
                final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(lower_bound->first, lower_bound->second));
        } else {
            lower_bound = mymap->lower_bound(key_start);
            if (lower_bound == mymap->end()) return final_values;
            if (lower_bound != mymap->begin()) {
                --lower_bound;
                if (key_start > lower_bound->first) lower_bound++;
            }
            while (lower_bound != mymap->end()) {
                if (lower_bound->first > key_end) break;
                final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(lower_bound->first, lower_bound->second));
                lower_bound++;
            }
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::ContainsInServer(KeyType &key_start,KeyType &key_end) {
    if (server_on_node) {
        return LocalContainsInServer(key_start,key_end);
    }
    else {
        typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER("_Contains", my_server_i, ret_type, key_start,key_end);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::LocalGetAllDataInServer() {
    AutoTrace trace = AutoTrace("basket::map::GetAllDataInServer", NULL);
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
        typename MyMap::iterator lower_bound;
        lower_bound = mymap->begin();
        while (lower_bound != mymap->end()) {
            final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(
                lower_bound->first, lower_bound->second));
            lower_bound++;
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::GetAllDataInServer() {
    if (server_on_node) {
        return LocalGetAllDataInServer();
    }
    else {
        typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER1("_GetAllData", my_server_i, ret_type);
   }
}
#endif  // INCLUDE_BASKET_MAP_MAP_CPP_
