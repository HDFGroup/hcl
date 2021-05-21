/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <hcl/clock/global_clock.h>
#include <iostream>
#include <mpi.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int num_servers = 1;
    int ranks_per_server = 1;
    std::string server_lists = "./simple_types/server_list";
    if (argc > 1) ranks_per_server = atoi(argv[1]);
    if (argc > 6) server_lists = argv[6];
    num_servers = size / ranks_per_server;
    if (size < num_servers + 1) {
        std::cout << "MPI ranks should be at least one more than number of " <<
                  "servers" << std::endl;
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    if (rank == 0) {
        std::cout << num_servers << " Server Test" << std::endl;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    bool is_server = (rank + 1) % ranks_per_server == 0;
    int my_server = rank / ranks_per_server;

    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = is_server;
    HCL_CONF->SERVER_LIST_PATH = server_lists;
    hcl::global_clock *clock;
    if (is_server) {
        clock = new hcl::global_clock();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        clock = new hcl::global_clock();
    }

    for (uint16_t i = 0; i < size; i++) {
        if (i == rank) {
            std::cout << "Time rank " << rank << ": " << clock->GetTime() <<
                      std::endl;
            for (uint16_t j = 0; j < num_servers; j++) {
                std::cout << "Time server " << j << " from rank " << rank << ": " <<
                          clock->GetTimeServer(j) << std::endl;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    delete clock;
    MPI_Finalize();
}
