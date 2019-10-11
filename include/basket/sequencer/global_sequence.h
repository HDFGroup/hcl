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

#ifndef INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_
#define INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_

#include <basket/communication/rpc_lib.h>
#include <basket/communication/rpc_factory.h>
#include <basket/common/singleton.h>
#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <utility>
#include <memory>
#include <string>
#include <boost/interprocess/managed_mapped_file.hpp>

namespace bip = boost::interprocess;

namespace basket {
class global_sequence {
  private:
    uint64_t* value;
    bool is_server;
    bip::interprocess_mutex* mutex;
    int my_rank, comm_size, num_servers;
    uint16_t my_server;
    really_long memory_allocated;
    bip::managed_mapped_file segment;
    std::string name, func_prefix;
    std::shared_ptr<RPC> rpc;
    bool server_on_node;
    CharStruct backed_file;

  public:
    ~global_sequence();
    global_sequence(std::string name_ = "TEST_GLOBAL_SEQUENCE");

    uint64_t GetNextSequence();
    uint64_t GetNextSequenceServer(uint16_t &server);

    uint64_t LocalGetNextSequence();

#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE1(LocalGetNextSequence)
#endif

};

}  // namespace basket

#endif  // INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_
