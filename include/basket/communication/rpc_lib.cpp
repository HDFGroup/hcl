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

#ifndef INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_
#define INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_

template <typename F>
void RPC::bind(CharStruct str, F func) {
    switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            rpclib_server->bind(str.c_str(), func);
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
	      thallium_engine->define(str.string(), func);
                break;
            }
#endif
    }
}
template <typename Response, typename... Args>
Response RPC::callWithTimeout(uint16_t server_index, int timeout_ms, CharStruct const &func_name, Args... args) {
    AutoTrace trace = AutoTrace("RPC::call", server_index, func_name);
    int16_t port = server_port + server_index;

    switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            auto *client = rpclib_clients[server_index].get();
            client->set_timeout(timeout_ms);
            Response response = client->call(func_name.c_str(), std::forward<Args>(args)...);
            client->clear_timeout();
            return response;
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
        case THALLIUM_TCP: {
            std::shared_ptr<tl::engine> thallium_client;
            if (BASKET_CONF->IS_SERVER) {
                thallium_client = std::make_shared<tl::engine>(BASKET_CONF->TCP_CONF.c_str(), MARGO_CLIENT_MODE);
            }
            else {
                thallium_client = thallium_engine;
            }

            tl::remote_procedure remote_procedure = thallium_client->define(func_name.c_str());
            // Setup args for RDMA bulk transfer
            // std::vector<std::pair<void*,std::size_t>> segments(num_args);

            return remote_procedure.on(thallium_endpoints[server_index])(std::forward<Args>(args)...);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
        case THALLIUM_ROCE: {
            std::shared_ptr<tl::engine> thallium_client;
            if (BASKET_CONF->IS_SERVER) {
                thallium_client = std::make_shared<tl::engine>(BASKET_CONF->VERBS_CONF.c_str(), MARGO_CLIENT_MODE);
            }
            else {
                thallium_client = thallium_engine;
            }

            tl::remote_procedure remote_procedure = thallium_client->define(func_name.c_str());
            // Setup args for RDMA bulk transfer
            // std::vector<std::pair<void*,std::size_t>> segments(num_args);
            // tl::bulk bulk_handle = remote_procedure.on(server_endpoint)(std::forward<Args>(args)...);
            // return std::make_pair(lookup_str, bulk_handle);

            return remote_procedure.on(thallium_endpoints[server_index])(std::forward<Args>(args)...);
            break;
        }
#endif
    }
}
template <typename Response, typename... Args>
Response RPC::call(uint16_t server_index,
                   CharStruct const &func_name,
                   Args... args) {
    AutoTrace trace = AutoTrace("RPC::call", server_index, func_name);
    int16_t port = server_port + server_index;

    switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            auto *client = rpclib_clients[server_index].get();
            /*client.set_timeout(5000);*/
            return client->call(func_name.c_str(), std::forward<Args>(args)...);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
        case THALLIUM_TCP: {
            std::shared_ptr<tl::engine> thallium_client;
            if (BASKET_CONF->IS_SERVER) {
                thallium_client = std::make_shared<tl::engine>(BASKET_CONF->TCP_CONF.c_str(), MARGO_CLIENT_MODE);
            }
            else {
                thallium_client = thallium_engine;
            }

            tl::remote_procedure remote_procedure = thallium_client->define(func_name.c_str());
            // Setup args for RDMA bulk transfer
            // std::vector<std::pair<void*,std::size_t>> segments(num_args);

            return remote_procedure.on(thallium_endpoints[server_index])(std::forward<Args>(args)...);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
        case THALLIUM_ROCE: {
            std::shared_ptr<tl::engine> thallium_client;
            if (BASKET_CONF->IS_SERVER) {
                thallium_client = std::make_shared<tl::engine>(BASKET_CONF->VERBS_CONF.c_str(), MARGO_CLIENT_MODE);
            }
            else {
                thallium_client = thallium_engine;
            }

            tl::remote_procedure remote_procedure = thallium_client->define(func_name.c_str());

            // Setup args for RDMA bulk transfer
            // std::vector<std::pair<void*,std::size_t>> segments(num_args);
            // tl::bulk bulk_handle = remote_procedure.on(server_endpoint)(std::forward<Args>(args)...);
            // return std::make_pair(lookup_str, bulk_handle);

            return remote_procedure.on(thallium_endpoints[server_index])(std::forward<Args>(args)...);
            break;
        }
#endif
    }
}


template <typename Response, typename... Args>
std::future<Response> RPC::async_call(uint16_t server_index,
                                      CharStruct const &func_name,
                                        Args... args) {
    AutoTrace trace = AutoTrace("RPC::call", server_index, func_name);
    int16_t port = server_port + server_index;

    switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            auto *client = rpclib_clients[server_index].get();
            // client.set_timeout(5000);
            return client->async_call(func_name.c_str(), std::forward<Args>(args)...);
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
        case THALLIUM_TCP: {
            //TODO:NotImplemented error
            break;
        }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
        case THALLIUM_ROCE: {
             //TODO:NotImplemented error
            break;
        }
#endif
    }
}


#ifdef BASKET_ENABLE_THALLIUM_ROCE
// These are still experimental for using RDMA bulk transfers
template<typename MappedType>
MappedType RPC::prep_rdma_server(tl::endpoint endpoint, tl::bulk &bulk_handle) {
    // MappedType buffer;
    std::string buffer;
    buffer.resize(1000000, 'a');
    std::vector<std::pair<void *, size_t>> segments(1);
    segments[0].first = (void *)(&buffer[0]);
    segments[0].second = 1000000 + 1;
    tl::bulk local = thallium_engine->expose(segments, tl::bulk_mode::write_only);
    bulk_handle.on(endpoint) >> local;
    return buffer;
}

template<typename MappedType>
tl::bulk RPC::prep_rdma_client(MappedType &data) {
    MappedType my_buffer = data;
    std::vector<std::pair<void *, std::size_t>> segments(1);
    segments[0].first = (void *)&my_buffer[0];
    segments[0].second = 1000000 + 1;
    return thallium_engine->expose(segments, tl::bulk_mode::read_only);
}
#endif

#endif  // INCLUDE_BASKET_COMMUNICATION_RPC_LIB_CPP_
