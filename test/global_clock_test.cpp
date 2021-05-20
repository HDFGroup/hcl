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
  if (argc > 1) num_servers = atoi(argv[1]);

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

  hcl::global_clock *clock = new hcl::global_clock();

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
