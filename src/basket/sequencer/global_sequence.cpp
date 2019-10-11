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

#include <basket/sequencer/global_sequence.h>

namespace basket {

global_sequence::~global_sequence() {
    if (is_server) bip::file_mapping::remove(backed_file.c_str());
}

global_sequence::global_sequence(std::string name_)
        : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
          num_servers(BASKET_CONF->NUM_SERVERS),
          comm_size(1), my_rank(0), memory_allocated(BASKET_CONF->MEMORY_ALLOCATED),
          backed_file(BASKET_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_),
          name(name_), segment(),
          func_prefix(name_),
          server_on_node(BASKET_CONF->SERVER_ON_NODE) {
    AutoTrace trace = AutoTrace("basket::global_sequence");
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    name = name+"_"+std::to_string(my_server);
    rpc = Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
    if (is_server) {
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                std::function<uint64_t(void)> getNextSequence(std::bind(
                    &basket::global_sequence::LocalGetNextSequence, this));
                rpc->bind(func_prefix+"_GetNextSequence", getNextSequence);
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
                    std::function<void(const tl::request &)> getNextSequence(std::bind(
                        &basket::global_sequence::ThalliumLocalGetNextSequence, this,
                        std::placeholders::_1));
                    rpc->bind(func_prefix+"_GetNextSequence", getNextSequence);
                    break;
                }
#endif
        }

        boost::interprocess::file_mapping::remove(backed_file.c_str());
        segment = bip::managed_mapped_file(bip::create_only, backed_file.c_str(), 65536);
        value = segment.construct<uint64_t>(name.c_str())(0);
        mutex = segment.construct<boost::interprocess::interprocess_mutex>(
            "mtx")();
    }else if (!is_server && server_on_node) {
        segment = bip::managed_mapped_file(bip::open_only, backed_file.c_str());
        std::pair<uint64_t*, bip::managed_mapped_file::size_type> res;
        res = segment.find<uint64_t> (name.c_str());
        value = res.first;
        std::pair<bip::interprocess_mutex *,
                  bip::managed_mapped_file::size_type> res2;
        res2 = segment.find<bip::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
}

uint64_t global_sequence::LocalGetNextSequence() {
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    return ++*value;
}

uint64_t global_sequence::GetNextSequence() {
    if (server_on_node) {
        return LocalGetNextSequence();
    }
    else {
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER1("_GetNextSequence", my_server_i, uint64_t);
    }
}

uint64_t global_sequence::GetNextSequenceServer(uint16_t &server) {
    if (my_server == server && server_on_node) {
        return LocalGetNextSequence();
    }
    else {
        return RPC_CALL_WRAPPER1("_GetNextSequence", server, uint64_t);
    }
}

}  // namespace basket
