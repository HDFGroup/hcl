/*
 * Copyright (C) 2019  Chris Hogan
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

#include <cassert>
#include <cstdlib>
#include <ctime>

#include <mpi.h>

#include "util.h"
#include "hcl/unordered_map/unordered_map.h"

int main (int argc, char* argv[])
{
    MpiData mpi = initMpiData(&argc, &argv);

    int ranks_per_server = mpi.comm_size;
    int num_requests = 512;
    long size_of_request = 1000;
    bool debug = false;
    bool server_on_node = false;

    if(argc > 1) ranks_per_server = atoi(argv[1]);
    if(argc > 2) num_requests = atoi(argv[2]);
    if(argc > 3) size_of_request = (long)atol(argv[3]);
    if(argc > 4) server_on_node = (bool)atoi(argv[4]);
    if(argc > 5) debug = (bool)atoi(argv[5]);

    if (debug) {
        printf("%s/%d: %d\n", mpi.processor_name.c_str(), mpi.rank, getpid());
        if (mpi.rank == 0) {
            printf("%d ready for attach\n", mpi.comm_size);
            fflush(stdout);
            getchar();
        }
    }

    bool is_server = (mpi.rank + 1) % ranks_per_server == 0;
    int my_server = mpi.rank / ranks_per_server;
    int num_servers = mpi.comm_size / ranks_per_server;

    constexpr int array_size= 1000;

    if (size_of_request != array_size) {
        printf("Please set TEST_REQUEST_SIZE in include/hcl/common/constants.h instead."
               "Testing with %d\n", array_size);
    }

    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
    HCL_CONF->SERVER_LIST_PATH = "./server_list";

    using MyArray = std::array<int, array_size>;
    using MyMap = hcl::unordered_map<KeyType, MyArray>;

    char *username = std::getenv("USER");
    std::string shm_string = std::string(username ? username : "") + "_THALLIUM_BULK_TEST";
    CharStruct shm_name(shm_string);

    MyMap *map;
    if (is_server) {
        map = new MyMap(shm_name);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        map = new MyMap(shm_name);
    }

    MPI_Comm client_comm;
    MPI_Comm_split(MPI_COMM_WORLD, !is_server, mpi.rank, &client_comm);
    int client_comm_size;
    MPI_Comm_size(client_comm, &client_comm_size);

    if (!is_server) {
        for(int i = 0; i < num_requests; ++i) {
            auto key = KeyType(i);
            MyArray arr;
            for (auto &x : arr) {
                x = 5;
            }
            map->BulkPut(key, arr);
        }

        MPI_Barrier(client_comm);

        for(int i = 0; i < num_requests; ++i) {
            auto key = KeyType(i);
            auto result = map->BulkGet(key);
            assert(result.first);
            assert(result.second.size() == array_size);
            for (const auto &x : result.second) {
                assert(x == 5);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    delete(map);
    MPI_Finalize();

    return 0;
}
