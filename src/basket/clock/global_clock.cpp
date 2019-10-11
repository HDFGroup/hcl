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

#include <basket/clock/global_clock.h>

namespace basket {
/*
 * Destructor removes shared memory from the server
 */
global_clock::~global_clock() {
    AutoTrace trace = AutoTrace("basket::~global_clock", NULL);
    if (is_server) bip::file_mapping::remove(backed_file.c_str());
}


global_clock::global_clock(std::string name_)
        : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
          num_servers(BASKET_CONF->NUM_SERVERS),
          comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL),
          name(name_), segment(),
          func_prefix(name_),
          backed_file(BASKET_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_),
          server_on_node(BASKET_CONF->SERVER_ON_NODE) {
    AutoTrace trace = AutoTrace("basket::global_clock");
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    name = name+"_"+std::to_string(my_server);
    rpc = Singleton<RPC>::GetInstance();
    if (is_server) {
                switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                std::function<HTime(void)> getTimeFunction(
                    std::bind(&global_clock::LocalGetTime, this));
                rpc->bind(func_prefix+"_GetTime", getTimeFunction);
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
                    std::function<void(const tl::request &)> getTimeFunction(
                        std::bind(&global_clock::ThalliumLocalGetTime, this,
                                  std::placeholders::_1));
                    rpc->bind(func_prefix+"_GetTime", getTimeFunction);
                    break;
                }
#endif
        }

        bip::file_mapping::remove(backed_file.c_str());
        segment = bip::managed_mapped_file(bip::create_only, backed_file.c_str(),
                                             65536);
        start = segment.construct<chrono_time>("Time")(
            std::chrono::high_resolution_clock::now());
        mutex = segment.construct<boost::interprocess::interprocess_mutex>(
            "mtx")();
    }else if (!is_server && server_on_node) {
        segment = bip::managed_mapped_file(bip::open_only, backed_file.c_str());
        std::pair<chrono_time*, bip::managed_mapped_file::size_type> res;
        res = segment.find<chrono_time> ("Time");
        start = res.first;
        std::pair<bip::interprocess_mutex *,
                  bip::managed_mapped_file::size_type> res2;
        res2 = segment.find<bip::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
}


/*
 * GetTime() returns the time locally within a node using chrono
 * high_resolution_clock
 */
HTime global_clock::LocalGetTime() {
    AutoTrace trace = AutoTrace("basket::global_clock::GetTime", NULL);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto t =  std::chrono::duration_cast<std::chrono::microseconds>(
        t2 - *start).count();
    return t;
}

/*
 * GetTime() returns the time on the server
 */
HTime global_clock::GetTime() {
    if (server_on_node) {
        return LocalGetTime();
    }
    else {
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER1("_GetTime", my_server_i, HTime);
    }
}

/*
 * GetTimeServer() returns the time on the requested server using RPC calls, or
 * the local time if the server requested is the current client server
 */
HTime global_clock::GetTimeServer(uint16_t &server) {
    AutoTrace trace = AutoTrace("basket::global_clock::GetTimeServer", server);
    if (my_server == server && server_on_node) {
        return LocalGetTime();
    }
    else {
        return RPC_CALL_WRAPPER1("_GetTime", server, HTime);
    }
}

}  // namespace basket
