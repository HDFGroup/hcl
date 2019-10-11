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

#ifndef INCLUDE_BASKET_PRIORITY_QUEUE_PRIORITY_QUEUE_CPP_
#define INCLUDE_BASKET_PRIORITY_QUEUE_PRIORITY_QUEUE_CPP_

/* Constructor to deallocate the shared memory*/
template<typename MappedType, typename Compare>
priority_queue<MappedType, Compare>::~priority_queue() {
    if (is_server) bip::file_mapping::remove(backed_file.c_str());
}

template<typename MappedType, typename Compare>
priority_queue<MappedType,
               Compare>::priority_queue(std::string name_)
                       : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
                         num_servers(BASKET_CONF->NUM_SERVERS),
                         comm_size(1), my_rank(0), memory_allocated(BASKET_CONF->MEMORY_ALLOCATED),
                         name(name_), segment(), queue(), func_prefix(name_),
                         backed_file(BASKET_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_+"_"+std::to_string(my_server)),
                         server_on_node(BASKET_CONF->SERVER_ON_NODE) {
    AutoTrace trace = AutoTrace("basket::priority_queue");
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
        bip::file_mapping::remove(backed_file.c_str());
        /* allocate new shared memory space */
        segment = bip::managed_mapped_file(bip::create_only, backed_file.c_str(),
                                             memory_allocated);
        ShmemAllocator alloc_inst(segment.get_segment_manager());
        /* Construct priority queue in the shared memory space. */
        queue = segment.construct<Queue>("Queue")(Compare(), alloc_inst);
        mutex = segment.construct<bip::interprocess_mutex>("mtx")();
        /* Create a RPC server and map the methods to it. */
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                std::function<bool(MappedType &)> pushFunc(
                    std::bind(&basket::priority_queue<MappedType,
                              Compare>::LocalPush, this,
                              std::placeholders::_1));
                std::function<std::pair<bool, MappedType>(void)> popFunc(std::bind(
                    &basket::priority_queue<MappedType,
                    Compare>::LocalPop, this));
                std::function<size_t(void)> sizeFunc(std::bind(
                    &basket::priority_queue<MappedType,
                    Compare>::LocalSize, this));
                std::function<std::pair<bool, MappedType>(void)> topFunc(std::bind(
                    &basket::priority_queue<MappedType,
                    Compare>::LocalTop, this));
                rpc->bind(func_prefix+"_Push", pushFunc);
                rpc->bind(func_prefix+"_Pop", popFunc);
                rpc->bind(func_prefix+"_Top", topFunc);
                rpc->bind(func_prefix+"_Size", sizeFunc);
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
                    std::function<void(const tl::request &, MappedType &)> pushFunc(
                        std::bind(&basket::priority_queue<MappedType,
                                  Compare>::ThalliumLocalPush, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &)> popFunc(std::bind(
                        &basket::priority_queue<MappedType,
                        Compare>::ThalliumLocalPop, this, std::placeholders::_1));
                    std::function<void(const tl::request &)> sizeFunc(std::bind(
                        &basket::priority_queue<MappedType,
                        Compare>::ThalliumLocalSize, this, std::placeholders::_1));
                    std::function<void(const tl::request &)> topFunc(std::bind(
                        &basket::priority_queue<MappedType,
                        Compare>::ThalliumLocalTop, this,
                        std::placeholders::_1));
                    rpc->bind(func_prefix+"_Push", pushFunc);
                    rpc->bind(func_prefix+"_Pop", popFunc);
                    rpc->bind(func_prefix+"_Top", topFunc);
                    rpc->bind(func_prefix+"_Size", sizeFunc);
                    break;
                }
#endif
        }
    }else if (!is_server && server_on_node) {
        /* Map the clients to their respective memory pools */
        segment = bip::managed_mapped_file(bip::open_only, backed_file.c_str());
        std::pair<Queue*, bip::managed_mapped_file::size_type> res;
        res = segment.find<Queue> ("Queue");
        queue = res.first;
        std::pair<bip::interprocess_mutex *,
                  bip::managed_mapped_file::size_type> res2;
        res2 = segment.find<bip::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
}

/**
 * Push the data into the local priority queue.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename MappedType, typename Compare>
bool priority_queue<MappedType, Compare>::LocalPush(MappedType &data) {
    AutoTrace trace = AutoTrace("basket::priority_queue::Push(local)",
                                data);
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    queue->push(data);
    return true;
}

/**
 * Push the data into the priority queue. Uses key to decide the
 * server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename MappedType, typename Compare>
bool priority_queue<MappedType, Compare>::Push(MappedType &data,
                                               uint16_t &key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalPush(data);
    } else {
        AutoTrace trace = AutoTrace("basket::priority_queue::Push(remote)",
                                    data, key_int);
        return RPC_CALL_WRAPPER("_Push", key_int, bool,
                                data);
    }
}

/**
 * Get the data from the local priority queue.
 * @param key_int, key_int to know which server
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename MappedType, typename Compare>
std::pair<bool, MappedType>
priority_queue<MappedType, Compare>::LocalPop() {
    AutoTrace trace = AutoTrace("basket::priority_queue::Pop(local)");
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    if (queue->size() > 0) {
        MappedType value = queue->top();
        queue->pop();
        return std::pair<bool, MappedType>(true, value);
    }
    return std::pair<bool, MappedType>(false, MappedType());
}

/**
 * Get the data from the priority queue. Uses key_int to decide the
 * server to hash it to,
 * @param key_int, key_int to know which server
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename MappedType, typename Compare>
std::pair<bool, MappedType>
priority_queue<MappedType, Compare>::Pop(uint16_t &key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalPop();
    } else {
        AutoTrace trace = AutoTrace("basket::priority_queue::Pop(remote)",
                                    key_int);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER1("_Pop", key_int, ret_type); 
    }
}

/**
 * Get the data from the local priority queue.
 * @param key_int, key_int to know which server
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename MappedType, typename Compare>
std::pair<bool, MappedType>
priority_queue<MappedType, Compare>::LocalTop() {
    AutoTrace trace = AutoTrace("basket::priority_queue::Top(local)");
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    if (queue->size() > 0) {
        MappedType value = queue->top();
        return std::pair<bool, MappedType>(true, value);
    }
    return std::pair<bool, MappedType>(false, MappedType());;
}

/**
 * Get the data from the priority queue. Uses key_int to decide the
 * server to hash it to,
 * @param key_int, key_int to know which server
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename MappedType, typename Compare>
std::pair<bool, MappedType>
priority_queue<MappedType, Compare>::Top(uint16_t &key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalTop();
    } else {
        AutoTrace trace = AutoTrace("basket::priority_queue::Top(remote)",
                                    key_int);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER1("_Top", key_int, ret_type);
    }
}

/**
 * Get the size of the local priority queue.
 * @param key_int, key_int to know which server
 * @return return a size of the priority queue
 */
template<typename MappedType, typename Compare>
size_t priority_queue<MappedType, Compare>::LocalSize() {
    AutoTrace trace = AutoTrace("basket::priority_queue::Size(local)");
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    size_t value = queue->size();
    return value;
}

/**
 * Get the size of the priority queue. Uses key_int to decide the
 * server to hash it to,
 * @param key_int, key_int to know which server
 * @return return a size of the priority queue
 */
template<typename MappedType, typename Compare>
size_t priority_queue<MappedType, Compare>::Size(uint16_t &key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalSize();
    } else {
        AutoTrace trace = AutoTrace("basket::priority_queue::Top(remote)",
                                    key_int);
        return RPC_CALL_WRAPPER1("_Size", key_int, size_t);
    }
}
#endif  // INCLUDE_BASKET_PRIORITY_QUEUE_PRIORITY_QUEUE_CPP_
